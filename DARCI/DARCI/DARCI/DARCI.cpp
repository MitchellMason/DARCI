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
#include <limits>
#include "SDL.h"

SDL_Event event;
int basePort = 9000; //the port all data communication begins at
NetServer *server;
NetClient *client;
InputDevice *camera;
netClientData *data; //The data passed to the rendering object
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
	data = new netClientData();

	//initialize the data holder
	data->cAttrib = *new videoAttributes;
	data->cAttrib.bytesPerPixel = 3;
	data->cAttrib.height = camera->getColSpecs().height; 
	data->cAttrib.width = camera->getColSpecs().width;
	data->colorBuff = new BYTE[data->cAttrib.height * data->cAttrib.width * data->cAttrib.bytesPerPixel];

	data->dAttrib = *new videoAttributes;
	data->dAttrib.bytesPerPixel = 2;
	data->dAttrib.height = camera->getDepSpecs().height;
	data->dAttrib.width = camera->getDepSpecs().width;

	data->depthBuff = new BYTE[data->dAttrib.height * data->dAttrib.width * data->dAttrib.bytesPerPixel];

	//start receiving data
	client->start(data);

	//start the renderer
	printf("-Starting renderer.\n");
	renderer = new OculusRenderer(data);
	renderer->run();
}

//Runs forever while application is active
void loop(){
	//wait to join the active threads
	Sleep(32);
}

//Entry point for application. 
int wmain(int argc, char **argv[]){
	printf("-----------------Welcome to DARCI------------------\n");
	printf("-------------Written by Mitchell Mason-------------\n\n");
	init();
	while (renderer->isRunning()){
		loop();
	}
	delete server;
	delete client;
	delete renderer;
	printf("\n----------------------Closing----------------------\n");
	printf("Press return to exit");
	std::cin.ignore(2, '\n'); //just wait for the user to finish reading
	return 0;
}