#include "NetServer.h"

NetServer::NetServer()
{
	//camera = new Kinect2(); TODO
}

NetServer::~NetServer()
{
}

//Spawns a new thread that calls run until the application is closed
void NetServer::start(){
	running = true;
	std::thread(run);
}

void NetServer::run(){
	printf("Hello from new thread!\n");
	while (running){
		//do stuff
	}
}