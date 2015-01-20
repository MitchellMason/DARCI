#pragma once
#include <thread> 
#include <Windows.h>
#include "dataTypes.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

class NetClient
{
public:
	NetClient(int port);
	~NetClient();
	void start(netClientData *data);
private:
	static void run(NetClient *me, netClientData *data);
	SOCKET* NetClient::getSock(SOCKTYPE s);

	//The thread the server runs on
	std::thread *thread;

	int port;
	bool running = false;
	SOCKET colSock = INVALID_SOCKET;
	SOCKET depSock = INVALID_SOCKET;
};

