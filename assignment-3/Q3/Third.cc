/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FifthScriptExample");

// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                5 Mbps, 2 ms
//
//
// We want to look at changes in the ns-3 TCP congestion window.  We need
// to crank up a flow and hook the CongestionWindow attribute on the socket
// of the sender.  Normally one would use an on-off application to generate a
// flow, but this has a couple of problems.  First, the socket of the on-off
// application is not created until Application Start time, so we wouldn't be
// able to hook the socket (now) at configuration time.  Second, even if we
// could arrange a call after start time, the socket is not public so we
// couldn't get at it.
//
// So, we can cook up a simple version of the on-off application that does what
// we want.  On the plus side we don't need all of the complexity of the on-off
// application.  On the minus side, we don't have a helper, so we have to get
// a little more involved in the details, but this is trivial.
//
// So first, we create a socket and do the trace connect on it; then we pass
// this socket into the constructor of our simple application which we then
// install in the source node.
// ===========================================================================
//
class MyApp : public Application
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  //if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange (Ptr<OutputStreamWrapper> stream,uint32_t oldCwnd, uint32_t newCwnd)
{
    *stream->GetStream () << Simulator::Now ().GetSeconds () << "," <<  newCwnd << std::endl;
}

static void
RxDrop ( int* TotalPacketsDropped, Ptr<const Packet> p)
{
  *TotalPacketsDropped = *TotalPacketsDropped + 1 ;
}


int
main (int argc, char *argv[])
{
  CommandLine cmd;
  std::string ConfigurationId = "1" ;
  cmd.AddValue ("config", "configuration-id", ConfigurationId) ;
  cmd.Parse (argc, argv);
  //int config = stoi(ConfigurationId);
  NodeContainer nodes;
  nodes.Create (3);
    
  PointToPointHelper pointToPoint_1,pointToPoint_2;
  pointToPoint_1.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint_1.SetChannelAttribute ("Delay", StringValue ("3ms"));
  pointToPoint_2.SetDeviceAttribute ("DataRate", StringValue ("9Mbps"));
  pointToPoint_2.SetChannelAttribute ("Delay", StringValue ("3ms"));

  NetDeviceContainer devices_1;
  devices_1 = pointToPoint_1.Install(NodeContainer(nodes.Get(0),nodes.Get(2)));
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  devices_1.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  // n1------n3 channel made
  NetDeviceContainer devices_2;
  devices_2 = pointToPoint_2.Install(NodeContainer(nodes.Get(1),nodes.Get(2)));
  Ptr<RateErrorModel> em2 = CreateObject<RateErrorModel> ();
  em2->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  devices_2.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em2));
  // n2 --- n3 channel made
  // error added to that channel
  if(ConfigurationId == "3")
  {
	  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewRenoCSE"));
	  InternetStackHelper stack ; 
	  stack.Install(nodes);
  }
  else 
  {
	  InternetStackHelper stack_2; 
	  stack_2.Install(nodes);
  }
  /*else if(ConfigurationId == "2")
  {
	  InternetStackHelper stack_3 ;
	  stack_3.Install(NodeContainer(nodes.Get(0),nodes.Get(1)));
	  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewRenoCSE"));
	  InternetStackHelper stack_4; 
	  stack_4.Install(nodes.Get(2));
	  

  }*/
  //InternetStackHelper stack;
  //stack.Install (nodes);
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewRenoCSE"));
  Ipv4AddressHelper address_1,address_2;
  address_1.SetBase ("10.1.1.0", "255.255.255.252");
  address_2.SetBase ("10.1.2.0", "255.255.255.252");
  if(ConfigurationId == "2")
  {
	  TypeId typ  = TypeId::LookupByName("ns3::TcpNewRenoCSE");
	  std::stringstream node; 
	  node << nodes.Get(1)->GetId() ; 
	  std::string s  = "/NodeList/"+node.str()+"/$ns3::TcpL4Protocol/SocketType";
	  Config::Set(s,TypeIdValue(typ));
  }
  Ipv4InterfaceContainer interfaces_1 = address_1.Assign (devices_1);
  Ipv4InterfaceContainer interfaces_2 = address_2.Assign (devices_2);

  uint16_t sinkPort_1 = 8080,sinkPort_2=8081,sinkPort_3=8082;
  Address sinkAddress_1(InetSocketAddress (interfaces_1.GetAddress (1), sinkPort_1));
  PacketSinkHelper packetSinkHelper_1("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort_1));
  ApplicationContainer sinkApps_1 = packetSinkHelper_1.Install (nodes.Get (2));
  // sink installed in node-3
  sinkApps_1.Start (Seconds (1.));
  sinkApps_1.Stop (Seconds (20.));
  Address sinkAddress_2(InetSocketAddress (interfaces_1.GetAddress (1), sinkPort_2));
  PacketSinkHelper packetSinkHelper_2("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort_2));
  ApplicationContainer sinkApps_2 = packetSinkHelper_2.Install (nodes.Get (2));
  // sink installed in node-3
  sinkApps_2.Start (Seconds (5.));
  sinkApps_2.Stop (Seconds (25.));
  Address sinkAddress_3(InetSocketAddress (interfaces_2.GetAddress (1), sinkPort_3));
  PacketSinkHelper packetSinkHelper_3("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort_3));
  ApplicationContainer sinkApps_3 = packetSinkHelper_3.Install (nodes.Get (2));
  // sink installed in node-3
  sinkApps_3.Start (Seconds (15.));
  sinkApps_3.Stop (Seconds (30.));
  
  AsciiTraceHelper ascii;


  Ptr<MyApp> app = CreateObject<MyApp> ();
  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("Connection-1-Config-"+ConfigurationId+ ".cwnd");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange,stream));
  app->Setup (ns3TcpSocket, sinkAddress_1,3000, 1000, DataRate ("1.5Mbps"));
  nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (20.));
    
  app = CreateObject<MyApp> ();
  ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  stream = ascii.CreateFileStream ("Connection-2-Config-"+ConfigurationId+ ".cwnd");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange,stream));
  app->Setup (ns3TcpSocket, sinkAddress_2,3000, 1000, DataRate ("1.5Mbps"));
  nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (5.));
  app->SetStopTime (Seconds (25.));
    
 
  app = CreateObject<MyApp> ();
  ns3TcpSocket = Socket::CreateSocket (nodes.Get (1), TcpSocketFactory::GetTypeId ());
  stream = ascii.CreateFileStream ("Connection-3-Config-"+ConfigurationId+ ".cwnd");
   ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange,stream));
  app->Setup (ns3TcpSocket, sinkAddress_3,3000, 1000, DataRate ("1.5Mbps"));
  nodes.Get (1)->AddApplication (app);
  app->SetStartTime (Seconds (15.));
  app->SetStopTime (Seconds (30.));

  int TotalpacketsDropped_1=0,TotalpacketsDropped_2=0;
  devices_1.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop,&TotalpacketsDropped_1));
  devices_2.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeBoundCallback (&RxDrop,&TotalpacketsDropped_2));
  
  Simulator::Stop (Seconds (30.));
  Simulator::Run ();
  Simulator::Destroy ();
  std::cout<<"Total packets dropped in first channel:" << TotalpacketsDropped_1 << "\n" ;
  std::cout<<"Total packets dropped in second  channel:" << TotalpacketsDropped_2 << "\n" ;
  return 0;
}



