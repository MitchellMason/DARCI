// Send a frame out over UDP
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(__WINDOWS__) || defined(__TOS_WIN__)

  #include <windows.h>

  inline void delay( unsigned long ms )
    {
		Sleep( ms );
    }

#else  /* presume POSIX */

  #include <unistd.h>

  inline void delay( unsigned long ms )
    {
		usleep( ms * 1000 );
    }

#endif 

using namespace std;

//network vars
string ipAddr;
int port;
int sock;
int binding;
sockaddr_in myaddr;

//Function prototypes
int send(unsigned char, unsigned char, unsigned char);

//Graphics vars
unsigned short width  = 500;
unsigned short height = 500;
struct ScanlinePacket{
	//The index of each scanline to render.
	unsigned short int index;
	//The scanline data to be sent. Each byte is a color val
	unsigned char* data;
};

int main(int argc, char* argv[]){
	//Parse the arguements
	if(argc < 2 || argc > 4){
		cout << "Usage: SendFrame [port] [optional: display width] [optional: display height]" << endl;
		return 0;
	}
	
	//The user supplied their own graphics dimensions
	if (argc >= 3){
		width = atoi(argv[2]);
	}
	
	else if(argc == 4){
		height = atoi(argv[3]);
	}	
	
	port = atoi(argv[1]);

	//create the socket
	cout << "Creating socket." << endl;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock > 0){
		cout << "Socket created: " << sock << endl;
	}
	else{
		perror("Cannot create socket.");
		return -1;
	}
	
	// init myaddr
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(0);
	myaddr.sin_port = htons(port);

	//main input loop
	cout << "Ready to go." << endl << "give me a color to send (input something <0 to exit.)" << endl;
	int r = 1;
	int g = 1;
	int b = 1;
	while(true){
		cout << "color (r g b) -> ";
		cin  >> r >> g >> b;
		if(r<0 || b<0 || g < 0){
			break;
		}
		cout << "Sending (" << r << "," << g << "," << b << ")" <<endl;
		if(send(r,g,b) < 0){
			return -1;
		}
	}
	cout << "Done." << endl;
	return 0;
}

int send(unsigned char r, unsigned char g, unsigned char b){
	cout << "Forming packets" << endl;
	ScanlinePacket packet;
	packet.data = new unsigned char[width];
	bzero((void *)&packet,sizeof(packet));
	cout << "initialized packets" <<  endl;
	for(unsigned short int i=0; i<height; i++){
		packet.index = i;
		for(int j=0; j<width; j++){
			//TODO fix color order on Processing side
			packet.data[(j*3)]   = b;
			packet.data[(j*3)+1] = r;
			packet.data[(j*3)+2] = g;
		}
		if(sendto(sock, (void *)&packet, sizeof(packet), 0, (struct sockaddr *)&myaddr,
		sizeof(myaddr)) < 0){
			perror("Cannot send ");
			return -1;
		}
	}
	cout << endl << "sent." << endl;
	return 0;
}
