Flow of program
::Sender::
1-socket()
2-sendto()
3-recvfrom()
::Receiver::
1-socket()
2-bind()
3-recvfrom()
4-sendto()

Credit: https://www.geeksforgeeks.org/udp-server-client-implementation-c/

Method:
Ask for input from user on sender side.
Check for 20 characters and send the string to SAWUDP_send function.
In the function, socket is created and a synchronization frame is sent by defining frame_kind.
In return, acknowledgment is received by receiver.(The receiver is binded to a socket)
After that, the string is split into characters and one by one sent to the receiver. The succedding character is only sent after the acknowledgement is received for that particular character with sequence number.
In case, if the character is not received within pre-determined interval, timer times out and the character is resend from sender.
Once all the characters are sent from sender, sender sends FIN bit as frame_kind to notify the receiver for ending.

References: http://pubs.opengroup.org/onlinepubs/009695399/functions/setsockopt.html
https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html






