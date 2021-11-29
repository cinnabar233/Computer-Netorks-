import os, sys
import matplotlib.pyplot as plt
import subprocess as sp

'''
Algorithm:
Send ping requests to the server with increasing ttl values from 1,2,3,... till either the host is reached or a max ttl limit is reached
The ping with ttl set as i returns an error statement with the IP address of the i-th node on the path. To get the RTT to this ip address
, ping this ip address and 'grep'
'''
#Run with python2 only
E_ARGS = 2
max_ttl = 30
cnt = 3
def get_ip(ip):
    ip = ip.strip(':')
    ip = ip.strip(')')
    ip = ip.strip('(')
    return ip

if (len(sys.argv) != E_ARGS):
    print('Usage: python3 traceroute.py <name of the webpage>')
else:
    page = sys.argv[1]
    ip_address = []
    rtt = [0]
    ttl = 1
    while ttl <= max_ttl:
        out = sp.Popen(['ping', '-c',str(cnt), '-m ' + str(ttl), page], stdout=sp.PIPE)
        tokens = out.communicate()[0].split()
        #print(tokens)
        if 'exceeded' in tokens:
            ip = tokens[tokens.index('from') + 1]
            if(ip[0].isalpha()):
                ip = tokens[tokens.index('from') + 2]
            ip = get_ip(ip)
            print(ip)
            ip_address.append(ip)
        # print(tokens)
        elif '100.0%' in tokens:
            ip_address.append('_')
            print('Request timeout')
        elif 'Unknown' in tokens:
            print('Unknown host')
            break
        else:
            ip = tokens[2]
            ip = get_ip(ip)
            print(ip)
            ip_address.append(ip)
            break
        ttl = ttl + 1

    ttl = max_ttl
    for ip in ip_address:
        if not ip == '_':
            out = sp.Popen(['ping', '-c', str(cnt), '-m ' + str(ttl), ip], stdout=sp.PIPE)
            tokens = out.communicate()[0].split()
            #print(tokens)
            if tokens[-1] == 'ms':
                #print(tokens[-2].split('/')[1])
                avg_time = float(tokens[-2].split('/')[1])     #to get the average time of 3 pings
                rtt.append(avg_time)
            else:
                rtt.append(0)
        else:
            rtt.append(0);

    print(rtt)
    plt.xlabel("Hops")
    plt.ylabel("RTT(ms)")
    plt.plot(rtt)
    plt.savefig('traceroute_'+page+'.png')
    plt.show()
    # os.system('ping -c 1 -m '+str(ttl)+' '+page+' > output' )




