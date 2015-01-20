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
	delete camera;
	delete thread;
	delete port;
	delete addr;
}

//Spawns a new thread that calls run until the application is closed
void NetServer::start(){
	thread = new std::thread(&NetServer::run, this);
}

void NetServer::run(NetServer *me){
	bool threadRunning = true;
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

	//data used during transfer
	int bytesSent = 0;

	//serve content
	int framesSent = 0;
	while (threadRunning){
		framesSent++;
		
		//mark the beginning of the transfer
		GetSystemTime(time);
		startT = (time->wSecond * 1000) + time->wMilliseconds;

		//get the data
		videoFrame *cFrame = new videoFrame(colFeedAttributes.width, colFeedAttributes.height, VIDEOTYPE::vCOLOR);
		videoFrame *dFrame = new videoFrame(depFeedAttributes.width, depFeedAttributes.height, VIDEOTYPE::vDEPTH);
		me->camera->getColor(cFrame);
		me->camera->getDepth(dFrame);
		
		/***send the data***/
		
		//Color
		int colPacketLen = (cFrame->getWidth() * cFrame->getBytesPerPixel()) + sizeof(unsigned short);
		BYTE *colPacket = new BYTE[colPacketLen];
		int vBuffWid = cFrame->getWidth() * 3;    //In bytes
		int vBuffHei = cFrame->getHeight();       //In pixels
		unsigned char *vBuffLoc = cFrame->getBuffer();
		for (unsigned short i = 0; i < vBuffHei; i++){
			//append the scanline index
			colPacket[0] = (BYTE)((i & 0xFF00) >> 8);
			colPacket[1] = (BYTE)(i & 0x00FF);

			//Copy the color data into the packet buffer
			memcpy(&colPacket[2], vBuffLoc + (i * vBuffWid), vBuffWid);

			//Down the series of tubes
			bytesSent += sendto(cSock, (const char *)colPacket, colPacketLen, 0, (const sockaddr*)&cClient, sizeof(sockaddr));
		}
		
		delete cFrame;
		delete[] colPacket;

		//Depth
		int depPacketLen = depFeedAttributes.bytesPerPixel * depFeedAttributes.width + sizeof(short);
		BYTE *depPacket = new BYTE[depPacketLen];
		int dBuffWid = depFeedAttributes.width * depFeedAttributes.bytesPerPixel; //in bytes
		int dBuffHei = depFeedAttributes.height;
		unsigned char *dBuffLoc = dFrame->getBuffer();

		for (unsigned short i = 0; i < depFeedAttributes.height; i++){
			//append the scanline index
			depPacket[0] = (BYTE)((i & 0xFF00) >> 8);
			depPacket[1] = (BYTE)(i & 0x00FF);

			//Copy into the packet buffer
			BYTE* scline = dBuffLoc + (i * dBuffWid);
			memcpy(&depPacket[2], scline, dBuffWid);

			//Bye!
			bytesSent += sendto(dSock, (const char *)depPacket, depPacketLen, 0, (const sockaddr *)&dClient, sizeof(sockaddr));
		}

		delete dFrame;
		delete[] depPacket;

		
		//check for errors
		if (bytesSent <= 0){
			printf("Error occured on server with code %i\n", WSAGetLastError());
			threadRunning = false;
		}

		//mark the end of the transfer. Ensure the server isn't struggling to send all the messages.
		GetSystemTime(time);
		endT = (time->wSecond * 1000) + time->wMilliseconds;
		opTime = endT - startT;
		// printf("Color done. %i bytes sent in %i ms for %i bytes per ms.\n", bytesSent, opTime, bytesSent/opTime);
		bytesSent = 0;
		if (opTime < (long)delayTime){ Sleep((long)delayTime - opTime); }
		else{ printf("WARNING: SERVER IS NOT SENDING CONTENT ON TIME: %i ms.\n",opTime); }
	}
	printf("Server not running.\n");
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

	//Set the socket as either blocking or nonblocking.
	u_long enableBlocking = 0;
	u_long enableNonBlocking = !enableBlocking;
	int iResult = ioctlsocket(*s, FIONBIO, &enableBlocking);
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);

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
