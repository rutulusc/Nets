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

int main()
{
struct sockaddr_in server_address;
server_address.sin_family = AF_INET;
server_address.sin_port = htons(PORT);
server_address.sin_addr.s_addr = INADDR_ANY;

//Creating a client socket
int c_socket=socket(AF_INET, SOCK_STREAM, 0); //socket(int domain, int type, int protocol)

if(c_socket < 0){
	cout<<"Error while creating socket! \n";
	return -1;
}
else{
	cout<<"Socket created!\n";
}

if(inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr)<=0)  
    { 
        printf("\nInvalid address\n"); 
        return -1; 
    }

int conn_status = connect(c_socket, (struct sockaddr *) &server_address,sizeof(server_address)); //connect(int socket, struct sockaddr *address, int address_len)
//Check for error with connection
if(conn_status==0){
	cout << "Connection successful ! \n";
}
else{
	cout<<"Connection failed ! \n";
	return -1;
}

while(1)
{
int value=0,buffer;
cout<<"Please provide an integer in the range [0,214783647] \n";
cin >> value;
//Checking if the entered number is valid
if(value > 2147483647)
{
	cout<<"Please enter number in valid range !";
	exit(0);
}
else{
send(c_socket,&value,sizeof(value),0);
cout<<"Sent "<< value << " to server on port "<< PORT <<"\n";
read(c_socket, &buffer, sizeof(buffer));
cout<<"Received the following result from server: \n"<<buffer<<"\n" <<"Done!\n \n";
}
}
return 0;
}