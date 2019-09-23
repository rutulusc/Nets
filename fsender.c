#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h>

#define STR_VALUE(val) #val
#define STR(name) STR_VALUE(name)

#define PATH_LEN 256
#define MD5_LEN 32


#define PAYLOAD_SIZE 1250
#define HEADER_SIZE 4
#define PACKET_SIZE 1254

pthread_t resend_thread;

typedef struct timeval timestamp;

typedef struct packet_data{
    int seq_num;

    char payload[PAYLOAD_SIZE+1];
}packet;
#define name_of_file_SIZE 50
double out = 400000.0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

char *name_of_file,*data;
int fp,error,sockfd,no_of_packets,portno,off=0,seqNum=0;
struct stat statbuf;
size_t filesize;
int NakPackets = 0;
//Socket variables
struct sockaddr_in serv_addr;
socklen_t fromlen;
struct hostent *server,*server_t;;

void send_packets(packet temp_packet)
{
    usleep(100);
    int n;
    n = sendto(sockfd,&temp_packet,PACKET_SIZE, 0,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
    if (n < 0){
        perror("sendto");
        exit(0);
    }   
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

void makesocket()
{
    //create a udp socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int sock_options;
    if (sockfd < 0){
        perror("ERROR opening socket");
    exit(0);
    }
    uint64_t sock_buffer_size = 500000000;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    fromlen=(sizeof(serv_addr));
    if((setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&sock_options,sizeof (int))) == -1){
        perror("ERROR setting socket opt");
    exit(0);
    }
    if((setsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,&sock_buffer_size, sizeof(uint64_t))) == -1){
        perror("ERROR setting socket opt");
    exit(0);
    }
    if((setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&sock_buffer_size, sizeof(uint64_t))) == -1){
        perror("ERROR setting socket opt");
    exit(0);
    }
}


void* resend_packet(void* a)
{

    //int fla = 0;
    //fflush(stdout);

    while(1){
        int n,seq,size=PAYLOAD_SIZE;
        n = recvfrom(sockfd,&seq,sizeof(int),0,(struct sockaddr *)&serv_addr,&fromlen);

        if (n < 0){
            perror("recvfrom");
            exit(0);
        }

        

        if(seq == -1){
            printf("Entire file transmitted\n");
            pthread_exit(0);
        }
        // if(fla == 0){
        //         printf("Obtained NAK for: ");
        //         fla = 1;
        // }

        //printf("%d ", seq);
        //fflush(stdout);

        NakPackets++;

        if((seq == (no_of_packets-1)) && (0 != filesize % PAYLOAD_SIZE))
            size = filesize % PAYLOAD_SIZE;
        packet packet2;
        memset(packet2.payload,'\0',PAYLOAD_SIZE+1);
        packet2.seq_num = seq;
        memcpy(packet2.payload,data+(seq*PAYLOAD_SIZE),size);
        send_packets(packet2);
    }
}


void mapfile(){
    pthread_mutex_lock(&lock);
    if ((fp = open (name_of_file, O_RDONLY)) < 0){
        fprintf(stderr,"can't open %s for reading", name_of_file);
        pthread_mutex_unlock(&lock);
        exit(0);
    }
    filesize = lseek(fp, 0, SEEK_END);
    printf("Filesize is %zu\n",filesize);
    data = mmap((caddr_t)0, filesize, PROT_READ, MAP_SHARED, fp, 0);
    if (data == (caddr_t)(-1)) {
        perror("mmap");
        pthread_mutex_unlock(&lock);
        exit(0);
    }
    pthread_mutex_unlock(&lock);
}


int main(int argc, char *argv[])
{
    if (argc < 4) {
       fprintf(stderr,"%s", "please send the name of the file, host-name and portno as arguments");
       exit(0);
    }
    server_t = gethostbyname(argv[2]);
    server = gethostbyname(argv[2]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    if (server_t == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    name_of_file = (char *) malloc(name_of_file_SIZE);
    strcpy(name_of_file,argv[1]);
    portno = atoi(argv[3]);
    makesocket();
    mapfile();

    int sockfd_t, n_t, portno_t = 50000;
    sockfd_t = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_t < 0){
        perror("ERROR opening socket");
    exit(0);
    }

    struct sockaddr_in serv_addr_t;
    bzero((char *) &serv_addr_t, sizeof(serv_addr_t));
    serv_addr_t.sin_family = AF_INET;
    bcopy((char *)server_t->h_addr,(char *)&serv_addr_t.sin_addr.s_addr,server_t->h_length);
    serv_addr_t.sin_port = htons(portno_t);
    if (connect(sockfd_t,(struct sockaddr *) &serv_addr_t,sizeof(serv_addr_t)) < 0){
        perror("ERROR connecting");
    exit(0);
    }

    n_t = write(sockfd_t,(void *)&filesize,sizeof(filesize));
    if (n_t < 0){
        perror("ERROR writing to socket");
    exit(0);
    }
    close(sockfd_t);

    if((error=pthread_create(&resend_thread,NULL,resend_packet,NULL))){
        fprintf(stderr, "error in creating pthread: %s\n",strerror(error));
        exit(1);
    }

    packet packet1;
    memset(packet1.payload,'\0',PAYLOAD_SIZE+1);
    if((filesize % PAYLOAD_SIZE) != 0)
        no_of_packets = (filesize/PAYLOAD_SIZE)+1;
    else
        no_of_packets = (filesize/PAYLOAD_SIZE);
    while(seqNum < no_of_packets){
        packet1.seq_num = seqNum;
        if((seqNum == (no_of_packets-1)) && ((filesize % PAYLOAD_SIZE) != 0)){
            memcpy(packet1.payload,data+off,(filesize % PAYLOAD_SIZE));
            }
        else{
            memcpy(packet1.payload,data+off,PAYLOAD_SIZE);
            }
        seqNum++;
        memcpy(packet1.payload,data+off,PAYLOAD_SIZE);
        off = off + PAYLOAD_SIZE;
        send_packets(packet1);
    }
    pthread_join(resend_thread,NULL);
    munmap(data, filesize);
    close(fp);

    printf("Percentage packet drop: %f \n", ((float)NakPackets/no_of_packets));

    char md5[MD5_LEN + 1];

    if (!CalcFileMD5(name_of_file, md5)) {
        puts("Error occured!");
    } else {
        printf("Success! MD5 sum is: %s\n", md5);
    }
    return 1;
}