#include <iostream>
using namespace std;
#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define PORT 19267

int main(){
cout<<"Server is running and ready to receive connections on port " << PORT<<"\n";


struct sockaddr_in server_address;
server_address.sin_family = AF_INET;
server_address.sin_port = htons(PORT);
server_address.sin_addr.s_addr = INADDR_ANY;

//Creating a socket
int s_socket=socket(AF_INET, SOCK_STREAM, 0); //socket(int domain, int type, int protocol)
if(s_socket < 0){
	cout<<"Error while creating socket! \n";
	return -1;
}


//Binding the socket to specified IP
//bind(int socket, struct sockaddr *name, int namelen)
int bin_status=bind(s_socket,(struct sockaddr *) &server_address,sizeof(server_address));
if(bin_status = 0){
	cout<<"Error while binding! \n";
	return -1;
}

//Listening to
//listen(int socket, backlog_number); 
if(listen(s_socket,8) < 0)
{	
	cout<<"Error while listening! \n";
	return -1;
}

int new_socket=accept(s_socket,NULL, NULL);
//accept(int socket, &clientaddr, &addrlen);
if(new_socket<0){
	cout<<"Error while accepting! \n";
	return -1;
 }


while(1){
	int buffer=0;
	read(new_socket,&buffer,sizeof(buffer));
	cout<<"Received number "<<buffer<<" from some client. \n";
	cout<<"Computing the number of 1's in binary representation...\n";
	int num=buffer;
	unsigned int count=0;
	while(num){
		num &= (num-1);
		count++;
	}
	//send(client_socket,server_message,sizeof(server_message),0);
	send(new_socket, &count,sizeof(count),0);
	cout<<"The number of 1's in the binary representation of "<<buffer<<" is: " <<count<<"\n";
	cout<<"Sent result back to that client. \n \n";
	cout<<"Server is running and ready to receive connections on PORT "<<PORT<<"...\n";
}
return 0;
}