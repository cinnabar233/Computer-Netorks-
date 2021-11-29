import socket
import threading
import os,sys
welcomeSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serverPort = 12001
welcomeSocket.bind(('',serverPort))
welcomeSocket.listen(5)
recvSockets = {} #maps client usernames to client sockets   'string' -> (socket(recv))


def well_formed_username(username):
    for c in username:
        if (not c.isdigit()) and (not c.isalpha()):
            return False
    return True

def validate_message(message):
    if len(message) < 15:  ## to avoid segmentation fault
        return False
    message = message.split()
    if len(message) < 3:
        return False
    return True

def validate_received_message(message):
    if len(message) < 6:
        return False
    if message[:4] != 'SEND':
        return False
    l = message[5:].split('\n')
    if len(l) < 2 :
        return False
    content_desc = l[1].split(' ')
    if len(content_desc) < 2 :
        return False
    if content_desc[0] != 'Content-length:':
        return False
    return True


def server(connectionSocket):
    message = (connectionSocket.recv(1024)).decode()
    print(message)
    message = message.split()
    request_type = message[1]
    username = message[2]

    if not well_formed_username(username):
        error_message = 'ERROR 100 Malformed username \n\n'
        print('From server to', username, ':', error_message)
        connectionSocket.send(bytes(error_message, 'ascii'))
        connectionSocket.close()
        return

    if message[0] == 'REGISTER':
        if request_type == 'TORECV':
            recvSockets[username] = connectionSocket
            ack_to_receiver = 'REGISTERED TORECV '+username+"\n\n"
            print('From server to',username,':',ack_to_receiver)
            connectionSocket.send(bytes(ack_to_receiver, 'ascii'))
            sys.exit()
            return

        elif request_type == 'TOSEND':
            ack_to_sender = 'REGISTERED TOSEND '+username+'\n\n'
            print('From server to', username, ':', ack_to_sender)
            connectionSocket.send(bytes(ack_to_sender, 'ascii'))
            while True:
                message = (connectionSocket.recv(1024)).decode()
                print('From',username,':',message)
                is_valid = validate_received_message(message)
                if is_valid:
                    message = message.split()
                    recipient = message[1]
                    content_length = message[3]
                    content = ' '.join(message[4:])
                    is_valid = (len(content) == int(content_length))
                    if username in recvSockets.keys():  ##connection should be on both sockets before start
                        if is_valid :
                            if recipient in recvSockets.keys():
                                receiverSocket = recvSockets[recipient]
                                fwd_to_recipient = 'FORWARD '+username+'\n'+'Content-length: '+str(len(content))+'\n\n'+content
                                print('From server to',recipient,fwd_to_recipient)
                                receiverSocket.send(bytes(fwd_to_recipient,'ascii'))
                                message = (receiverSocket.recv(1024)).decode()
                                print('From',recipient,':',message)
                                ack_from_recipient = message.split()
                                status = ack_from_recipient[0]
                                sender_name = ack_from_recipient[1]
                                if status == 'RECEIVED':
                                    ack_to_sender = 'SENT ' + recipient+'\n'
                                    print('To', username,':', ack_to_sender)
                                    connectionSocket.send(bytes(ack_to_sender,'ascii'))
                                else:
                                    print('To',username,':',message)
                                    connectionSocket.send(bytes(message,'ascii'))
                            elif recipient == 'ALL':
                                valid_recipients = set()
                                for recipient in recvSockets.keys():
                                    if not recipient == username:
                                        receiverSocket = recvSockets[recipient]
                                        fwd_to_recipient = 'FORWARD ' + username + '\n' + 'Content-length: ' + str(
                                            len(content)) + '\n\n' + content
                                        receiverSocket.send(bytes(fwd_to_recipient, 'ascii'))
                                        print('From server to', recipient,':', fwd_to_recipient)
                                        ack_from_recipient = (receiverSocket.recv(1024)).decode()
                                        print('From', recipient, ':', ack_from_recipient)
                                        ack_from_recipient = ack_from_recipient.split()
                                        status = ack_from_recipient[0]
                                        sender_name = ack_from_recipient[1]
                                        if status == 'RECEIVED':
                                            valid_recipients.add(recipient)
                                if len(valid_recipients) == len(recvSockets.keys()) - 1 :
                                    ack_to_sender = 'SENT ALL'+ '\n'
                                    print('To', username, ':', ack_to_sender)
                                    connectionSocket.send(bytes(ack_to_sender, 'ascii'))
                                else:
                                    #not all recipients got the message
                                    error_message = 'ERROR 102 Unable to send\n\n'
                                    print('To', username, ':', error_message)
                                    connectionSocket.send(bytes(error_message, 'ascii'))
                            else:
                                error_message = 'ERROR 102 Unable to send\n\n'
                                print('To', username, ':', error_message)
                                connectionSocket.send(bytes(error_message,'ascii'))
                        else:
                            error_message = 'ERROR 103 Header incomplete\n\n'
                            print('To', username, ':', error_message)
                            connectionSocket.send(bytes(error_message, 'ascii'))
                            connectionSocket.close()
                            recvSockets[username].close()
                            sys.exit()
                            return  #  ##connection should not be on
                    else:
                        error_message = 'ERROR 101 No user registered'
                        print('To', username, ':', error_message)
                        connectionSocket.send(bytes(error_message, 'ascii'))
                else:
                    error_message = 'ERROR 103 Header incomplete\n\n'
                    print('To', username, ':', error_message)
                    connectionSocket.send(bytes(error_message, 'ascii'))
                    recvSockets[username].close()
                    connectionSocket.close()
                    sys.exit()
                    return

        else:
            error_message = 'ERROR 103 Header incomplete\n\n'
            print('To', username, ':', error_message)
            connectionSocket.send(bytes(error_message, 'ascii'))
            connectionSocket.close()
            recvSockets[username].close()
            sys.exit()
            return
    else:
        error_message = 'ERROR 101 No user registered'
        print('To', username, ':', error_message)
        connectionSocket.send(bytes(error_message, 'ascii'))
        return


if __name__ == '__main__':
    print('Application Log')
    while True:
        connectionSocket,clientAddr = welcomeSocket.accept()
        t = threading.Thread(target=server,args=(connectionSocket,))  ##connection socket can be connected to the send or recv socket of the client
        t.start()



