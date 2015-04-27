#include "NetServer.h"

//Create a new server for remAddr that serves cam's data on several ports, starting from basePort
NetServer::NetServer(std::string remAddr, int basePort, InputDevice *cam)
{
	//set the port
	port = &basePort;

	//set the address
	addr = (char *) remAddr.c_str();

	//Make the camera
	camera = cam;

	//create the sockets
	if (initSockets() == 0){
		return;
	}
	else{
		printf("could not initialize sockets.\n");
		exit(-1);
	}
}

NetServer::~NetServer()
{
	this->threadRunning = false;
	this->thread->join();
	delete camera;
	delete thread;
}

//Spawns a new thread that calls run until the application is closed
void NetServer::start(){
	thread = new std::thread(&NetServer::run, this);
}

void NetServer::run(NetServer *me){
	const float delayTime = (1.0f / 30.0f) * 1000.0f; //30 updates per second
	
	SYSTEMTIME *time = new SYSTEMTIME;
	long startT = 0;
	long endT = 0;
	long opTime = 0;

	videoAttributes colFeedAttributes = me->camera->getColSpecs();
	videoAttributes depFeedAttributes = me->camera->getDepSpecs();

	//get the sockets and client addresses.
	SOCKET cSock = *me->getSock(SOCKTYPE::COLOR);
	sockaddr_in cClient = *me->getSockAddrIn(SOCKTYPE::COLOR);
	SOCKET dSock = *me->getSock(SOCKTYPE::DEPTH);
	sockaddr_in dClient = *me->getSockAddrIn(SOCKTYPE::DEPTH);

	
	//server metrics
	int framesSent = 0;
	int errorCheck = 0;
	int bytesSent;
	
	//useful constants
	const int cScanLinesPerPacket = 8;
	const int cPacketsPerFrame = 53;
	const int dScanLinesPerPacket = 53;
	const int dPacketsPerFrame = 8;
	const int colPacketLen = cScanLinesPerPacket * colFeedAttributes.width * 3 + sizeof(INT32);
	const int depPacketLen = dScanLinesPerPacket * depFeedAttributes.width * 2 + sizeof(INT32);

	//buffer allocation
	videoFrame *cFrame = new videoFrame(colFeedAttributes.width, colFeedAttributes.height, VIDEOTYPE::vCOLOR);
	videoFrame *dFrame = new videoFrame(depFeedAttributes.width, depFeedAttributes.height, VIDEOTYPE::vDEPTH);
	cameraData *cData = (cameraData*) malloc(sizeof(cameraData));
	cData->color = *cFrame;
	cData->depth = *dFrame;
	BYTE *colPacket = new BYTE[colPacketLen];
	BYTE *depPacket = new BYTE[depPacketLen];

	//Server loop
	while (me->threadRunning){
		framesSent++;
		bytesSent = 0;
		
		//mark the beginning of the transfer
		GetSystemTime(time);
		startT = (time->wSecond * 1000) + time->wMilliseconds;
				
		//get the data
		me->camera->getData(cData);
		
		//Color 
		for (INT32 i = 0; i < cPacketsPerFrame; i++){
			//first 4 bytes are the packet index
			memcpy(&colPacket[0], &i, sizeof(INT32));
			
			//copy the data
			memcpy(&colPacket[sizeof(INT32)], 
				cFrame->getBuffer() + (i * cScanLinesPerPacket * cFrame->getWidth() * cFrame->getBytesPerPixel()), 
				colPacketLen);
			
			//send the data
			errorCheck = sendto(cSock, (const char *)colPacket, colPacketLen, 0, (const sockaddr*)&cClient, sizeof(sockaddr));
			
			//ensure the data is sent, despite errors
			while (errorCheck < 0){
				int error = WSAGetLastError();
				if (error == WSAEWOULDBLOCK)
					errorCheck = sendto(cSock, (const char *)colPacket, colPacketLen, 0, (const sockaddr*)&cClient, sizeof(sockaddr));
				else{
					printf("Socket error: %i", error);
				}
			}
			
			//add total data sent
			bytesSent += errorCheck;
		}
				

		//Depth (8 packets)
		for (INT32 i = 0; i < dPacketsPerFrame; i++){
			//first 4 bytes are the packet index
			memcpy(&depPacket[0], &i, sizeof(INT32));

			//copy the data
			memcpy(&depPacket[sizeof(INT32)],
				dFrame->getBuffer() + (i * dScanLinesPerPacket * dFrame->getWidth() * dFrame->getBytesPerPixel()),
				depPacketLen);

			//send the data
			errorCheck = sendto(dSock, (const char *)depPacket, depPacketLen, 0, (const sockaddr*)&dClient, sizeof(sockaddr));

			//ensure the data is sent, despite errors
			while (errorCheck < 0){
				errorCheck = sendto(dSock, (const char *)depPacket, depPacketLen, 0, (const sockaddr*)&dClient, sizeof(sockaddr));
			}

			//add total data sent
			bytesSent += errorCheck;
		}

		//TODO audio
		//TODO skeleton
		
		//check for errors
		if (bytesSent <= 0){
			printf("Error occured on server with code %i\n", WSAGetLastError());
			me->threadRunning = false;
		}

		//mark the end of the transfer. Ensure the server isn't struggling to send all the messages.
		GetSystemTime(time);
		endT = (time->wSecond * 1000) + time->wMilliseconds;
		opTime = endT - startT;
		printf("\r%i frames %i bytes in %i ms: %i bytes / ms.", framesSent, bytesSent, opTime, bytesSent/opTime);
		if (opTime < (long)delayTime){ 
			//Sleep((long)delayTime - opTime);
		}
		else{ 
			//printf("\nWARNING: SERVER IS NOT SENDING CONTENT ON TIME: %i ms.\n",opTime); 
		}
	}
}

//initialize and bind the sockets, as well as the addresses
int NetServer::initSockets(){
	WSADATA *w = new WSAData;
	w->iMaxSockets = 5; //col, dep, audio, skel, events

	if (WSAStartup(0x0101, w) != 0)
	{
		printf("Could not open Windows connection.\n");
		return -1;
	}

	int initResult = 0;
	initResult += initUDPSocket(&colSock, &remColClient);
	initResult += initUDPSocket(&depSock, &remDepClient);

	return initResult;
}

//creates a udp socket along with the address it will send to.
int NetServer::initUDPSocket(SOCKET* s, sockaddr_in* sa){
	//create the socket
	*s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (*s == INVALID_SOCKET || s == NULL){
		printf("Could not create socket.\n");
		return -1;
	}

	if (bind(*s, (const sockaddr *)sa, sizeof(sockaddr)));

	//Set the socket as either blocking or nonblocking.
	u_long enableBlocking = 0;
	u_long enableNonBlocking = !enableBlocking;
	int iResult = ioctlsocket(*s, FIONBIO, &enableNonBlocking);
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);

	//set the socket buffer size to 1 frame of content
	int sendBufferSize = 1920 * 1080 * 3;
	if (0 != setsockopt(*s, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufferSize, sizeof(sendBufferSize))){
		printf("cannot set UDP buffer to requested size: %i. Error code %i\n", sendBufferSize, WSAGetLastError());
	}

	//set the address the socket will send on
	memset(sa, 0, sizeof(sockaddr_in));
	sa->sin_addr.s_addr = inet_addr(addr);
	sa->sin_family = AF_INET;
	sa->sin_port = htons(*port);

	//Increase the working port number
	*port = *port + 1;

	return 0;
}

SOCKET* NetServer::getSock(SOCKTYPE s){
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

sockaddr_in* NetServer::getSockAddrIn(SOCKTYPE s){
	switch (s)
	{
	case COLOR:
		return &remColClient;
		break;
	case DEPTH:
		return &remDepClient;
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
