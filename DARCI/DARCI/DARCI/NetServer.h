#pragma once

#include "InputDevice.h"
#include "Kinect2.h"
#include <thread>
#include <Windows.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

class NetServer
{
public:
	NetServer(std::string remAddr, int basePort, InputDevice *cam);
	~NetServer();
	void start();

private:
	static void run(NetServer *me);
	int initSockets();
	int initUDPSocket(SOCKET* s, sockaddr_in* sa);
	SOCKET* getSock(SOCKTYPE s);
	sockaddr_in* getSockAddrIn(SOCKTYPE s);

	//The device that gets our data
	InputDevice *camera;
	
	//The thread the server runs on
	std::thread *thread;
	bool threadRunning = true;
	
	//Socket data
	int *port;
	char *addr;
	SOCKET colSock = INVALID_SOCKET;
	SOCKET depSock = INVALID_SOCKET;
	sockaddr_in remColClient;
	sockaddr_in remDepClient;
};

