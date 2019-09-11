#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <time.h>

#define PORT 19267

//structures
typedef struct packet{
    char character[1];
    char data[1024];
}Packet;

typedef struct frame{
    int frame_kind;//-1-SYNACK 3-DATA 4-FIN
    int sq_no;
    Packet packet;
}Frame;

typedef struct Acknow{
int ackNum;
int frame_kind;
}Acknow;

//sending function built on UDP
void SAWUDP_send(char *input){
    int sockfd;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
            perror("socket creation failed"); 
             exit(EXIT_FAILURE); 
        } 

//Handle a timer on socket
    struct timeval timeout;      
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;

    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0)
        perror("setsockopt failed\n");

Frame frame_send;
frame_send.frame_kind =1;
Acknow ackRecv;

memset(&serverAddr,'\0',sizeof(serverAddr));
serverAddr.sin_family = AF_INET;
serverAddr.sin_port=htons(PORT);
serverAddr.sin_addr.s_addr=inet_addr("10.0.0.2");
frame_send.sq_no = 0;
int r,recv_size;

while(1){

    printf("Establishing a connection to receiver... (sending SYN)\n");
    strcpy(frame_send.packet.data,input);
    sendto(sockfd,&frame_send, sizeof(Frame),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
    recv_size = recvfrom(sockfd,&ackRecv,sizeof(Acknow),0,(struct sockaddr*)&serverAddr,&addr_size);

    if(recv_size > 0 && ackRecv.frame_kind==-1){
    printf("Ack Recieved with SEQ# %d\n", ackRecv.ackNum);
    frame_send.sq_no = frame_send.sq_no + 1;
    break;
    }
}


for(r=0;r<strlen(input);r++){
    frame_send.frame_kind = 3;
    frame_send.packet.character[0]=input[r];
    //Sending character one by one in loop
    while(1){
    printf("Sending character '%c'\n",frame_send.packet.character[0]);
    sendto(sockfd,&frame_send, sizeof(Frame),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr)); 
    
    int res = recvfrom(sockfd,&ackRecv,sizeof(Acknow),0,(struct sockaddr*)&serverAddr,&addr_size);
    if (res > 0 && ackRecv.frame_kind == 2){
        printf("Ack Recieved with SEQ# %d\n",ackRecv.ackNum);
        frame_send.sq_no=frame_send.sq_no+1;
        break;
        }
    }
}



time_t time1, time2;
double dif_sec;
time(&time1);

while(1){
    frame_send.frame_kind = 4;
    printf("Terminating connection.....(sending FYN)\n");
    sendto(sockfd,&frame_send, sizeof(Frame),0,(struct sockaddr*)&serverAddr,sizeof(serverAddr)); 
    int res = recvfrom(sockfd,&ackRecv, sizeof(Acknow), 0,(struct sockaddr*)&serverAddr,&addr_size);

    if (ackRecv.frame_kind == 4){
        printf("ACK recieved with SEQ # %d\n",frame_send.sq_no);
        printf("Done!\n");
        pclose(sockfd);
        break;
        }


//Timer Window     
  time(&time2);
        if (difftime(time2, time1) > 5.00){
                        printf("Window timed-out.......");
                        break;
                    }
    }
}

//Main Program
void main(){
char input[100];   
printf("Please provide a string with atleast 20 characters: ");
LOOP:scanf("%s",input);
//Check the string length
if(strlen(input)>20)
{
SAWUDP_send(input);
}
else{
    printf("The string is less than 20 characters. Please enter atleast 20 characters");
    goto LOOP;
}
}