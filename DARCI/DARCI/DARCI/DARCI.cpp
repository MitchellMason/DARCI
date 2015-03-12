/*
	This is the main entry point for the whole application. 
	An init once, loop forever model is used for the entire application
	because I find that it keeps things pretty simple.
*/

#include "DARCI.h"
#include "NetServer.h"
#include "NetClient.h"
#include "OculusRenderer.h"
#include <iostream>
#include <stdio.h>
#include "SDL.h"

bool appIsRunning = true;
int basePort = 9000; //the port all data communication begins at
NetServer *server;
NetClient *client;
InputDevice *camera;
netClientData *ncData; //The data passed to the rendering object
OculusRenderer *renderer;

//Runs once at the beginning of the execution
void init(){
	//start the camera
	printf("-Opening camera.\n");
	camera = new Kinect2();
	camera->start();

	//start the server
	const char* remAddr = "127.0.0.1";
	printf("-Creating server.\n");
	server = new NetServer(remAddr, basePort, camera);
	printf("--hosting to %s\n",remAddr);
	server->start();

	//start the client
	printf("-Starting client.\n");
	client = new NetClient(basePort);
	ncData = new netClientData;

	client->start(ncData);

	//start the renderer
	printf("-Starting renderer.\n");
	renderer = new OculusRenderer(ncData);
	renderer->run();
}

//Runs forever while application is active
void loop(){
	Sleep(32);
}

//Entry point for application. 
int wmain(int argc, char **argv[]){
	printf("-----------------Welcome to DARCI------------------\n");
	printf("-------------Written by Mitchell Mason-------------\n\n");
	init();
	printf("-Done with init.\n");
	while (appIsRunning){
		loop();
	}
	delete server;
	delete client;
	delete renderer;
	return 0;
}