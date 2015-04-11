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

	//specifiy that the sockets are blocking
	u_long enableBlocking = 0;
	int iResult = ioctlsocket(colSock, FIONBIO, &enableBlocking);
	if (iResult != NO_ERROR){
		printf("ioctlsocket failed with error %ld\n", iResult);
	}

	iResult = ioctlsocket(depSock, FIONBIO, &enableBlocking);
	if (iResult != NO_ERROR){
		printf("ioctlsocket failed with error %ld\n", iResult);
	}
	int recvBufferSize = 1080 * 1920 * 3;
	if (0 != setsockopt(colSock, SOL_SOCKET, SO_RCVBUF, (const char*)&recvBufferSize, sizeof(recvBufferSize))){
		printf("cannot set UDP buffer to requested size: %i. Error code %i\n", recvBufferSize, WSAGetLastError());
	}
	if (0 != setsockopt(depSock, SOL_SOCKET, SO_RCVBUF, (const char*)&recvBufferSize, sizeof(recvBufferSize))){
		printf("cannot set UDP buffer to requested size: %i. Error code %i\n", recvBufferSize, WSAGetLastError());
	}
}

NetClient::~NetClient()
{
	running = false;
	this->thread->join();
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

	//Helpers for receiving
	const int cPacketLen = data->cAttrib.bytesPerPixel * (data->cAttrib.height / 108) * data->cAttrib.width + sizeof(INT32);
	const int dPacketLen = data->dAttrib.bytesPerPixel * (data->dAttrib.height / 8) * data->dAttrib.width + sizeof(INT32);
	const int recvBuffLen = cPacketLen * 108 * 3; //Allocate more than a few packets in memory, just in case
	BYTE *recvBuff = new BYTE[recvBuffLen];
	int received = 0;
	INT32 offset = 0;

	while (me->running){
		FD_ZERO(socks);
		FD_SET(*cSock, socks);
		FD_SET(*dSock, socks);

		//See what's readable
		select(highestFD + 1, socks, NULL, NULL, timeout);

		//read whatever data is ready
		if (FD_ISSET(*cSock, socks)){
			received = recv(*cSock, (char *)recvBuff, recvBuffLen, 0);
			if (received > 0){
				for (int i = 0; i < received; i += cPacketLen){
					//first 4 bytes contain the index of the scanlines
					memcpy(&offset, &recvBuff[i], sizeof(INT32));
					offset *= (cPacketLen - sizeof(INT32));
					
					memcpy(data->colorBuff + offset, &recvBuff[i + sizeof(INT32)], cPacketLen - sizeof(INT32));
				}
			}
			else{
				printf("COLOR RECV ERROR CODE: %i, bytes received: %i\n", WSAGetLastError(), received);
			}
		}
		if (FD_ISSET(*dSock, socks)){
			received = recv(*dSock, (char *)recvBuff, recvBuffLen, 0);
			//copy the buffer into the texture at the index of the first byte
			if (received > 0){
				//printf("Got %f packets\n", (1.0f * received) / (1.0f * dPacketLen));
				for (int i = 0; i < received; i += dPacketLen){
					//first 4 bytes contain the index of the scanlines
					memcpy(&offset, &recvBuff[i], sizeof(INT32));
					offset *= (dPacketLen - sizeof(INT32));

					//copy the data into the buffer
					memcpy(data->depthBuff + offset, &recvBuff[i + sizeof(INT32)], dPacketLen - sizeof(INT32));
				}
			}
			else{
				printf("DEPTH RECV ERROR CODE: %i, bytes received: %i\n", WSAGetLastError(), received);
			}
		}
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

