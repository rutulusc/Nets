#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <stdbool.h>
#include <time.h>

#define PORT 19267

typedef struct packet{
    char character[1];
    char data[1024];
}Packet;

typedef struct frame{
    int frame_kind;
    int seq_num;
    Packet packet;
}Frame;

typedef struct Acknow{
int ackNum;
int frame_kind;
}Acknow;

Frame frame_recv;


char* SAWUDP_receive(){
    int sockfd,count,i;
    struct sockaddr_in serverAddr, newAddr;
    socklen_t addr_size;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
        }

        while(1){
 
        printf("Receiver is running and ready to receive connections on port %d... \n",PORT);
        int recv_size;
        int index=0;
        memset(&serverAddr,'\0',sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port=htons(PORT);
        serverAddr.sin_addr.s_addr=inet_addr("10.0.0.2");
        bind(sockfd, (struct sockaddr*)&serverAddr,sizeof(serverAddr));
        addr_size=sizeof(serverAddr);

        bool finEnd = false;

        struct timeval timeout;      
        timeout.tv_sec = 0;
        timeout.tv_usec = 250000;
        Acknow ackSend;

        while(1){
            
            recv_size = recvfrom(sockfd,&frame_recv,sizeof(Frame),0,(struct sockaddr*)&serverAddr,&addr_size);

            if(recv_size>0 && frame_recv.frame_kind == 1){
                    printf("Connection request received from <IP 10.0.0.1: PORT # %d>\n",PORT);
                    ackSend.frame_kind = -1;
                    ackSend.ackNum = frame_recv.seq_num;
                    printf("Sending ACK with SEQ # %d, expecting SEQ # %d\n",ackSend.ackNum,ackSend.ackNum+1);
                    sendto(sockfd,&ackSend,sizeof(Acknow), 0,(struct sockaddr*)&serverAddr,addr_size);
            }
            if(recv_size>0 && frame_recv.frame_kind == 3 ){
                    ackSend.frame_kind=2;
                    ackSend.ackNum = frame_recv.seq_num;
                    printf("Recieved '%c'\n",frame_recv.packet.character[0]);
                    printf("Sending ACK with SEQ # %d, expecting SEQ # %d\n",ackSend.ackNum,ackSend.ackNum+1);
                    sendto(sockfd,&ackSend,sizeof(Acknow),0,(struct sockaddr*)&serverAddr,addr_size);
            }

            if(frame_recv.frame_kind== 4){
                printf("Sender is terminating with FIN...\n");
                ackSend.ackNum = frame_recv.seq_num;
                ackSend.frame_kind = 4;
                printf("Sending ACK with SEQ # %d\n",ackSend.ackNum);
                if (!finEnd){
                    printf("Reception Complete:\n'%s'\n",frame_recv.packet.data);
                    finEnd = true;
                }
                sendto(sockfd,&ackSend,sizeof(Acknow),0,(struct sockaddr*)&serverAddr,addr_size);
                time_t time1, time2;
                time(&time1);

                if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
                        error("setsockopt failed\n");
                while (1){
                    recv_size = recvfrom(sockfd,&frame_recv,sizeof(Frame),0,(struct sockaddr*)&serverAddr,&addr_size);
                    sendto(sockfd,&ackSend,sizeof(Acknow),0,(struct sockaddr*)&serverAddr,addr_size);
                    time(&time2);

                    if (difftime(time2, time1) < 5.00){
                        frame_recv.frame_kind = 5;
                        break;
                    }}
                break;
            }}}
        return frame_recv.packet.data;
}
void main(){
    while(1){
        char* str = SAWUDP_receive();
    }}