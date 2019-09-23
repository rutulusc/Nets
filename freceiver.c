#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>

#define PAYLOAD_SIZE 1250
#define HEADER_SIZE 4
#define PACKET_SIZE 1254

#define SecToMsec 1000000
#define SecToMisec 1000
#define MAX_SIZE 1000000000
#define TRACK_ARRAY_LENGTH 8000000

int error;
FILE* fp_s;
char filename[50];
size_t filesize;

int end_number;

int start_index, last_index;
int *track_packets;
int packets_num;
unsigned int last_packet_size;
int trace;
int loop_index;

char *filedata;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//Socket variables
socklen_t fromlen;
struct sockaddr_in serv_addr, from;
int sockfd_s, portno;
int sock_options_s, sock_options_s_t;
int total;

int sockfd_t, newsockfd_t, portno_t;
socklen_t clilen_t;
struct sockaddr_in serv_addr_t, cli_addr_t;

typedef struct timeval timestamp;

typedef struct packet_rm{
    int seq_num;
    char payload[PAYLOAD_SIZE+1];
}packet;


#define STR_VALUE(val) #val
#define STR(name) STR_VALUE(name)

#define PATH_LEN 256
#define MD5_LEN 32

void * progressT()
{   
    int temp = 0;
    printf("Progress : ");
    fflush(stdout);
    while(1){
        int temp_old = (total*100)/packets_num;
        
        if (temp_old != temp){
            printf("%d ",temp_old);
            fflush(stdout);
            temp = temp_old;
        }

        if(total == packets_num-1){
            break;
        }
    }
pthread_exit(0);
}

int CalcFileMD5(char *file_name, char *md5_sum)
{
    #define MD5SUM_CMD_FMT "md5sum %." STR(PATH_LEN) "s 2>/dev/null"
    char cmd[PATH_LEN + sizeof (MD5SUM_CMD_FMT)];
    sprintf(cmd, MD5SUM_CMD_FMT, file_name);
    #undef MD5SUM_CMD_FMT

    FILE *p = popen(cmd, "r");
    if (p == NULL) return 0;

    int i, ch;
    for (i = 0; i < MD5_LEN && isxdigit(ch = fgetc(p)); i++) {
        *md5_sum++ = ch;
    }

    *md5_sum = '\0';
    pclose(p);
    return i == MD5_LEN;
}


//int CalcFileMD5(char *, char *)
void receive_packet_s();
void* handleFailures(void *);
int getNackSeqNum();


//threads
pthread_t nack_thread_s, nack_thread_s1;
struct timeval Start_Time, delay;

double get_time(struct timeval End_Time)
{
    double timeu = SecToMisec*(End_Time.tv_sec - Start_Time.tv_sec) +
    ((double)(End_Time.tv_usec - Start_Time.tv_usec))/SecToMisec;
    return timeu;
}

int main(int argc, char *argv[])
{
    uint64_t sock_buffer_size_s = 100000000;
    if (argc < 3) {
         fprintf(stderr,"Please send portno and filename as arguments\n");
         exit(1);
    }
    sockfd_s = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_s < 0){
    perror("ERROR opening socket");
    exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if((setsockopt(sockfd_s,SOL_SOCKET,SO_REUSEADDR,&sock_options_s,sizeof (int))) == -1){
        perror("ERROR setting socket opt");
        exit(1);
    }
    if((setsockopt(sockfd_s,SOL_SOCKET,SO_SNDBUF,&sock_buffer_size_s, sizeof(uint64_t))) == -1){
        perror("ERROR setting socket opt");
        exit(1);
        
    }
    if((setsockopt(sockfd_s,SOL_SOCKET,SO_RCVBUF,&sock_buffer_size_s, sizeof(uint64_t))) == -1){
        perror("ERROR setting socket opt");
        exit(1);
       
    }
    if (bind(sockfd_s, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        perror("ERROR setting socket opt");
        exit(1);    
    }

    fromlen = sizeof(from);
    sockfd_t = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd_t < 0){
        perror("ERROR opening socket");
        exit(1);
     }
    
        if((setsockopt(sockfd_t,SOL_SOCKET,SO_REUSEADDR,&sock_options_s_t,sizeof (int))) == -1){
        perror("ERROR setting socket opt");
        exit(1);
        }
     bzero((char *) &serv_addr_t, sizeof(serv_addr_t));
     portno_t = 50000;
     serv_addr_t.sin_family = AF_INET;
     serv_addr_t.sin_addr.s_addr = INADDR_ANY;
     serv_addr_t.sin_port = htons(portno_t);
     if (bind(sockfd_t, (struct sockaddr *) &serv_addr_t,sizeof(serv_addr_t)) < 0){
        perror("ERROR on binding");
        exit(1);
    }
             
     listen(sockfd_t,2);
     clilen_t = sizeof(cli_addr_t);
     newsockfd_t = accept(sockfd_t,(struct sockaddr *) &cli_addr_t,&clilen_t);
    if (newsockfd_t < 0){
        perror("ERROR on accept");
        exit(1);
    }

    if (read(newsockfd_t,&filesize,255) < 0){
        perror("ERROR reading from socket");
        exit(1);
    }
    close(newsockfd_t);
    close(sockfd_t);

    filedata = (char *) malloc (filesize);
    fp_s = fopen(argv[2] , "w+");
    start_index = 0, last_index = 0, total =0, loop_index = 0;
    packets_num = 0;
    trace = 0;
    if(filesize%PAYLOAD_SIZE != 0){
        packets_num = filesize/PAYLOAD_SIZE + 1;
        last_packet_size = filesize%PAYLOAD_SIZE;
    }
    else{
        packets_num = filesize/PAYLOAD_SIZE;
        last_packet_size = PAYLOAD_SIZE;
    }
    gettimeofday(&Start_Time, 0);
    fprintf(stdout, "%012.3fms: File sending Begins \n",get_time(Start_Time));

    //Thread handling failures
    if((errno = pthread_create(&nack_thread_s, NULL, handleFailures, NULL ))){
        fprintf(stderr, "pthread_create[0] %s\n",strerror(errno));
        pthread_exit(0);
    }

    pthread_create(&nack_thread_s1, NULL, progressT, NULL );

    track_packets = (int *)calloc(packets_num, sizeof (int));
    receive_packet_s();
    pthread_join(nack_thread_s, NULL);
    pthread_join(nack_thread_s1, NULL);

    gettimeofday(&delay, 0);
    fprintf(stdout, "%012.3fms: File Finished Sending \n",get_time(delay));
    printf("Throughput: %f Mbps\n", (filesize*8)/(get_time(delay)*1000));

    fclose(fp_s);
    close(sockfd_s);

    char md5[MD5_LEN + 1];

    if (!CalcFileMD5(argv[2], md5)) {
        puts("Error occured!");
    } else {
        printf("Calculation Done ! MD5 sum is: %s \n", md5);
    }

    return 0;
}

void receive_packet_s()
{
    int n = 0;
    packet rcvPacket;
    long write_pos;
    //int temp = 0;
    while (1)
    {
        n = recvfrom(sockfd_s,&rcvPacket,1500,0,(struct sockaddr *) &from,&fromlen);
        if (n < 0) {
        perror("ERROR in recv");
        exit(1);
    }
           
        pthread_mutex_lock(&lock);
        if (rcvPacket.seq_num >= 0 && rcvPacket.seq_num < packets_num && track_packets[rcvPacket.seq_num] == 0){
            track_packets[rcvPacket.seq_num] = 1;
                if(rcvPacket.seq_num > last_index)
                    last_index = rcvPacket.seq_num;
                write_pos = rcvPacket.seq_num * PAYLOAD_SIZE;
                fseek( fp_s , write_pos , SEEK_SET  );
                if(rcvPacket.seq_num == (packets_num - 1) ){
                    fwrite(&rcvPacket.payload , last_packet_size , 1 , fp_s);
                    fflush(fp_s);
                 }
                else{
                    fwrite(&rcvPacket.payload , PAYLOAD_SIZE , 1 , fp_s);
                    fflush(fp_s);
                 }
                 total ++;
        }
        pthread_mutex_unlock(&lock);
        
    

        if(total == packets_num){
            printf("\nBingo ! Entire file received.\n");    
            
            int end_data = -1;
            int i;
            
            for (i = 0 ; i < 10 ; i++){
            sendto(sockfd_s,&end_data,sizeof(int), 0,(struct sockaddr *) &from,fromlen);
            }
            
            close(sockfd_s);
            break;
        }
       
        //20% loss in line 
       if(last_index >= 0.8 * packets_num){
            trace = 1;
       }

       }

}

void* handleFailures(void *a)
{
    while(1)
    {
        if(trace){
            usleep(140);
            int actual_last_index = 0;
            if(last_index > 0.8 * packets_num)
                actual_last_index = last_index + 0.2 * packets_num;
            else
                actual_last_index = last_index;
            if(total == packets_num){
                pthread_exit(0);
            }
            int i;
            for(i =  start_index; i<= actual_last_index && i < packets_num ; i++)
                {
                if(track_packets[i] == 1){
                start_index ++;
                }
            else break;
            }

            int reqSeqNum = getNackSeqNum();

            if(reqSeqNum >= 0 && reqSeqNum < packets_num){
            int n = sendto(sockfd_s, &reqSeqNum, sizeof(int), 0,(struct sockaddr *) &from,fromlen);
            if (n < 0) {
            perror("sendto");
            exit(1);
            }
            }
        }
    }
}



int getNackSeqNum(){

    if (track_packets == NULL) return -1;
    int i;

    for (i = loop_index; i < packets_num ; i++)
    {
        if(track_packets[i] == 0){
            if( i == packets_num - 1) loop_index = start_index;
            else loop_index = i+1;
            return i;
        }
    }
    loop_index = start_index;
    return -1;

}

