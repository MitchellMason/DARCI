//TODO put in header
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>



class Network{
private:
	int		port;
	int		ipAddr;
	int		sockfd;
	bool	connectionLess; //Basically, isUDP or isTCP
	
	int initUDP(int port);
	int initTCP(int port);
	
public:
	Network(int port, bool connectionLess);
	~Network();
	int sendData(unsigned char *data[], int dlen, int recip, int recport);
	int getFD();
};

//Begin implementation
Network::Network(int port, bool connectionLess){
	if(connectionLess){
		initUDP(port);
	}
	else{
		initTCP(port);
	}
}

Network::~Network(){
	//TODO figure out how to close the sock connection.
}

int Network::initUDP(int port){
	//name the socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		perror("Cannot name socket");
		return -1;
	}
	
	//Fill out the address structures.
	memset((char *)&myaddr , 0, sizeof(myaddr));
	myaddr.sin_family      = AF_INET;			//IP
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);	//any ip address
	myaddr.sin_port        = htons(port);		//our specified port
	
	//bind the socket
	if(recMode){
		binding = ::bind(sock, (sockaddr *)&myaddr, sizeof(myaddr));
		if(binding < 0){
			perror("Socket not bound");
			return -1;
		}
		printf("Socket bound!\n");
	}
	
	//success!
	return 0;
}

int Network::initTCP(int port){
	return -1;
}

int Network::sendData(unsigned char *data[], int dlen, int recip, int recport){
	return -1;
}

int Network::getFD(){
	return sockfd;
}