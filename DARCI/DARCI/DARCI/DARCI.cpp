/*
	This is the main entry point for the whole application. 
	An init once, loop forever model is used for the entire application
	because I find that it keeps things pretty simple.
*/

#include "DARCI.h"
#include "NetServer.h"
#include "NetClient.h"
#include <iostream>

bool appIsRunning = true;
NetServer *server;
NetClient *client;

//Runs once at the beginning of the execution
void init(){
	//start the server
	server = new NetServer();
	server->start();
	//start the client
	client = new NetClient();
	client->start();
	//start the renderer
}

//Runs forever while application is active
void loop(){
	//Get and handle sdl events TODO
}

//Entry point for application. 
int main(int argc, char **argv[]){
	printf("Welcome to DARCI.\n");
	printf("Written by Mitchell Mason.\n");
	init();
	while (appIsRunning){
		loop();
	}
}