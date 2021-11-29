import socket
import threading
import os,sys

def validate_user_input(user_input):
    l = user_input.split()
    if len(l) < 2 :
        return False
    if l[0][0] != '@':
        return False
    if l[0][-1] != ':':
        return False
    return True

def sending_side(sendSocket,recvSocket):
    while True:
        print('@<Sender>: <Message>')
        user_input = input()
        if not validate_user_input(user_input):
            print('Invalid input format')
            continue

        keywords = user_input.split()
        recipient = keywords[0][1:-1]
        message = ' '.join(keywords[1:])
        message_to_server = "SEND "+recipient+"\n"+"Content-length: "+str(len(message))+"\n\n"+message
        sendSocket.send(bytes(message_to_server,'ascii'))
        response = (sendSocket.recv(1024)).decode()
        print('From server:', response)
        response = response.split()
        if response[1] == '103':
            break

    sendSocket.close()
    sys.exit()

def validate_received_message(message):
    if len(message) < 9:
        return False
    if message[:7] != 'FORWARD':
        return False
    l = message[8:].split('\n')
    if len(l) < 2 :
        return False
    content_desc = l[1].split(' ')
    if len(content_desc) < 1 :
        return False
    if content_desc[0] != 'Content-length:':
        return False
    return True


def receiving_side(sendSocket,recvSocket):
    while True:
        forwarded_message = (recvSocket.recv(1024)).decode()
        if len(forwarded_message) == 0:
            break
        if not validate_received_message(forwarded_message):
            message_to_server = 'ERROR 103 Header Incomplete\n\n'
            recvSocket.send(bytes(message_to_server, 'ascii'))
            break
        else:
            keywords = forwarded_message.split()
            sender = keywords[1]
            content_length = int(keywords[3])
            message = ' '.join(keywords[4:])
            if len(message) != content_length:
                message_to_server = 'ERROR 103 Header Incomplete\n\n'
                recvSocket.send(bytes(message_to_server, 'ascii'))
                break
            print('Received from @'+sender+": "+message)
            message_to_server = "RECEIVED "+sender+"\n\n"
            recvSocket.send(bytes(message_to_server,'ascii'))
    recvSocket.close()
    sys.exit()

if __name__ == '__main__':

    server = ''
    name =  ''
    serverPort = 12000 ##TCP welcoming socket of the server?(idts)
    while True:
        s = input('Type:<Name> <Server-IP>\n')
        sendSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  ##socket to send messages to server
        portSend = 12345  ##port number of the send socket
        [name,server] = s.split()   ##could do an error check here but not necessary in this assignment, i guess
        sendSocket.connect((server,serverPort))
        reg_request_send = "REGISTER TOSEND "+name+"\n\n"
        sendSocket.send(bytes(reg_request_send,'ascii'))
        response = (sendSocket.recv(1024)).decode()
        print(response)
        response = response.split()
        status = response[0]
        if status == 'REGISTERED':
            if response[1] == 'TOSEND':
                break
            else:
                continue
        else:
            continue
    while True:
        portRecv = 12346
        recvSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        recvSocket.connect((server,serverPort))
        reg_request_recv = "REGISTER TORECV "+name+"\n\n"
        recvSocket.send(bytes(reg_request_recv,'ascii'))
        response = (recvSocket.recv(1024)).decode()
        print(response)
        response = response.split()
        status = response[0]
        if status == 'REGISTERED':
            assert (response[1] == 'TORECV')
            break
        else:
            continue


    t1 = threading.Thread(target=receiving_side,args=(sendSocket,recvSocket))
    t2 = threading.Thread(target=sending_side,args=(sendSocket,recvSocket))
    t1.start()
    t2.start()
