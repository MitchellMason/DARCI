#include "NetClient.h"

bool running = false;
//Feed the pointers to the data that get updated from the network
NetClient::NetClient()
{
}

NetClient::~NetClient()
{
	running = false;
}

//Spawns a new thread and calls run() until the application quits
void NetClient::start(){
	
}

//Gets new information from the socket and updates the nesseary data
void NetClient::run(){
	
}
