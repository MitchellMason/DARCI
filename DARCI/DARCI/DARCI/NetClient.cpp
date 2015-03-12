#include "NetClient.h"

bool running = false;
//Feed the pointers to the data that get updated from the network
NetClient::NetClient(int port)
{
	this->port = port;

	//create the sockets
	colSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	depSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	//bind them to listen for data
	struct sockaddr_in myaddr;

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(port);

	if (bind(colSock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		printf("Color socket bind failed.\n");
		exit(-1);
	}

	myaddr.sin_port = htons(port + 1);

	if (bind(depSock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		printf("Depth socket bind failed.\n");
		exit(-1);
	}
}

NetClient::~NetClient()
{
	running = false;
}

//Spawns a new thread and calls run() until the application quits
void NetClient::start(netClientData *data){
	running = true;
	thread = new std::thread(&NetClient::run, this, data);
}

//Gets new information from the socket and updates the nessary data
void NetClient::run(NetClient *me, netClientData *data){
	
	timeval *timeout = new timeval;
	timeout->tv_sec = 0;
	timeout->tv_usec = 32 * 1000; //32 ms

	fd_set *socks = new fd_set;
	
	SOCKET *cSock = me->getSock(COLOR);
	SOCKET *dSock = me->getSock(DEPTH);

	//Get the highest numbered file descriptor
	unsigned int highestFD = 0;
	if (highestFD < *cSock) highestFD = *cSock;
	if (highestFD < *dSock) highestFD = *dSock;

	BYTE *recvBuff = new BYTE[data->cAttrib.bytesPerPixel * data->cAttrib.width * 10];
	bool newColData = false;
	bool newDepData = false;
	while (me->running){
		FD_ZERO(socks);
		FD_SET(*cSock, socks);
		FD_SET(*dSock, socks);

		//See what's readable
		select(highestFD + 1, socks, NULL, NULL, timeout);

		//read whatever data is ready
		if (FD_ISSET(*cSock, socks)){
			newColData = true;
			//while (data->colorLock){ /* wait on lock */ }
			data->colorLock = true;
			recv(*cSock, (char *) data->colorBuff, 1920 * 1080 * 3, 0);
			data->colorLock = false;
		}
		if (FD_ISSET(*dSock, socks)){
			newDepData = true;
			//while (data->colorLock){ /* wait on lock */ }
			data->colorLock = true;
			recv(*cSock, (char *)data->colorBuff, 512 * 424 * 2, 0);
			data->colorLock = false;
		}
		if (newColData){
			//copy buffers
		}
		if (newDepData){
			//copy buffers
		}
		newColData = false;
		newDepData = false;
	}
	delete[] recvBuff;
}
SOCKET* NetClient::getSock(SOCKTYPE s){
	switch (s)
	{
	case COLOR:
		return &colSock;
		break;
	case DEPTH:
		return &depSock;
		break;
	case AUDIO:
		break;
	case SKELETON:
		break;
	case REMOTE_EVENTS:
		break;
	default:
		return NULL;
		break;
	}
	return NULL;
}

