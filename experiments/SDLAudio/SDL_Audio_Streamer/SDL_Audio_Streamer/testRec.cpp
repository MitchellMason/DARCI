#include <iostream>
#include <netinet/in.h>
//#include <arpa/inet.h>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

using namespace std;

//net vars
int port;
int sock;
int binding;
sockaddr_in myaddr;  //this computer's address
sockaddr_in remaddr;
socklen_t addrlen = sizeof(myaddr);
timeval   timeout;
fd_set netfd;
int buffersize;
long recvlen;
unsigned char* buffer; //byte array that holds messages from other applications

//Initialize the UDP socket. Returns 0 on success, -1 on failure.
int initUDP(int port){
	//name the socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock > 0){
		cout << "Socket named " << sock << endl;
	}
	else{
		perror("Cannot name socket");
		return -1;
	}
	
	//Fill out the address structures.
	memset((char *)&remaddr, 0, sizeof(myaddr));
	memset((char *)&myaddr , 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;		//UDP
	myaddr.sin_addr.s_addr = htonl(0);	//any ip address
	myaddr.sin_port = htons(port);		//our specified port
	
	//bind the socket
	binding = ::bind(sock, (sockaddr *)&myaddr, sizeof(myaddr));
	if(binding < 0){
		perror("Socket not bound");
		return -1;
	}
	cout << "Socket bound" << endl;
	
	//Fill out the buffer
	buffersize = 100;
	buffer = new unsigned char [buffersize];
	
	//Set the timeout for recieve operations
	struct timeval tv;
	/* 500 ms Timeout */
	tv.tv_sec  = 0;
	tv.tv_usec = 500 * 1000;
	
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
	
	//success!
	return 0;
}
int main(int argc, const char* argv[]){
	if(argc < 2){
		printf("Supply a port");
		return -1;
	}
	port = atoi(argv[1]);
	if(initUDP(port) != 0){
		printf("Socket cannot be obtained.\n");
		return -1;
	}
	printf("Done UDP\n");
	while(recvlen <= 0){
		FD_ZERO(&netfd);
		FD_SET(sock, &netfd); //set the udp socket to the the netfd.
		timeout.tv_sec  = 1; //1 sec
		timeout.tv_usec = 500 * 1000; //500 ms
		recvlen = select(sock + 1, &netfd, NULL, NULL, &timeout);
		if(recvlen > 0){
			printf("got something.\t");
		}
		else{
			printf("Nothing...");
		}
		if(FD_ISSET(sock, &netfd)){
			printf(" and fd is set with buffer %s\n",buffer[0]);
		}
		else{
			printf(" and fd isn't set\t trying recvfrom()\t");
			recvlen = recvfrom(sock,buffer,buffersize,0,(struct sockaddr*)&remaddr, &addrlen);
			if(recvlen < 0){
				printf("Still no...\n");
			}
			else{
				printf("Success!\n");
				printf("buffer is %s\n",buffer);
				if(!FD_ISSET(sock,&netfd)){
					printf("It seems select() is not working\n");
				}
			}
		}
	}
}
