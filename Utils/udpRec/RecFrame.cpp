//
//  main.cpp
//  UDPFrameSender
//
//  Created by Mitchell Mason on 6/3/14.
//  Copyright (c) 2014 Mitchell Mason. All rights reserved.
//

#include <iostream>
#include <SDL2/SDL.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <cstdlib>

using std::cout;
using std::endl;
using std::string;

//function prototypes
int main(int argc, const char * argv[]);
int initUDP(int port);
bool draw(SDL_Window* window, SDL_Surface* screenSurface);
void putScanLine(SDL_Surface* surface, int index, Uint32 scanLine[]);


//Graphic vars
int width  = 500;
int height = 500;

//UDP vars
int port;
int sock;
int binding;
sockaddr_in myaddr;  //this computer's address
sockaddr_in remaddr; //remote address
socklen_t addrlen = sizeof(remaddr); //TODO discover what this means...
int buffersize;
long recvlen;
unsigned char* buffer; //byte array that holds messages from other applications

//SDL events
SDL_Event event;

int main(int argc, const char * argv[])
{
	//Parse the arguements
	if(argc < 2 || argc > 4){
		cout << "Usage: RecFrame [port] [optional: display width] [optional: display height]" << endl;
		return 0;
	}
	
	//The user supplied their own graphics dimensions
	if (argc >= 3){
		width = atoi(argv[2]);
	}
	if(argc == 4){
		height = atoi(argv[3]);
	}
	
	//start the UDP listener
	port = atoi(argv[1]);
	
	cout << "Initializing on port " << port << " with graphic dimensions " << width << "x" << height << endl;
	
	if(initUDP(port) < 0){
		perror("Cannot init UDP listener");
		return -1;
	}
	
	//The window we'll be rendering to
    SDL_Window* window = NULL;
    
    //The surface contained by the window
    SDL_Surface* screenSurface = NULL;
	
    //Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        cout << "SDL could not initialize! SDL_Error: %s\n", SDL_GetError();
    }
	else
    {
        //Create window
        window = SDL_CreateWindow("UDP Frame Receiver", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
        if(window == NULL)
        {
            cout << "Window could not be created! SDL_Error: %s\n", SDL_GetError();
        }
		else
        {
            //Get window surface
            screenSurface = SDL_GetWindowSurface(window);
			bool keepDrawing = true;
			SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00)); //clear the framebuffer by filling the surface with white
			cout << "Entering main draw loop" << endl;
			
			/***************MAIN DRAW LOOP***************/
            while(keepDrawing){
				//handle events after drawing
				SDL_PollEvent(&event);
				
				//handle some basic events
				switch(event.type){
					//if the close button is clicked
					case SDL_QUIT:{
						SDL_DestroyWindow(window);
						SDL_Quit();
						return false;
					}
				}
				for (int i =0; i<height; i++) {
					//receive data over UDP
					recvlen = recvfrom(sock, buffer, buffersize, 0, (struct sockaddr *)&remaddr, &addrlen);
					if(recvlen > 0){ //Let's see if we got anything!
						short scanlineIndex;
						unsigned char* pixDataRaw;
						Uint32* pixData;
						
						//get the scanline index
						scanlineIndex = (buffer[1] << 8) | buffer[0];
						//init the pixel data (final)
						pixData = new Uint32[width + 1];
						pixDataRaw = &buffer[2];
						
						//convert raw pixel data
						for(int i=0; i<width; i++){
							int temp = 0xFF000000; //the first 8 bits are all 1's for the alpha channel
							temp = temp | (pixDataRaw[(i*3)] << 16);
							temp = temp | (pixDataRaw[(i*3)+1] << 8);
							temp = temp | (pixDataRaw[(i*3)+2]);
							pixData[i] = temp;
						}
						putScanLine(screenSurface, scanlineIndex, pixData);
					}
				}
				//draw, and decide if we should continue drawing
				keepDrawing = draw(window, screenSurface);
			}
        }
    }
	
	//Done!
    return 0;
}

//Draw doesn't do much here because we just need to refresh the surface. Pixel handling is done in netcode
bool draw(SDL_Window* window, SDL_Surface* screenSurface){
	//Update the surface
	SDL_UpdateWindowSurface(window);
	
	//Keep drawing
	return true;
}

//Initialize the UDP listener
int initUDP(int port){
	//name the socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock > 0){
		cout << "Socket named " << sock << endl;
	}
	else{
		perror("Cannot name socket");
		return -1;
	}
	
	//Fill out the address structures.
	memset((char *)&remaddr, 0, sizeof(myaddr));
	memset((char *)&myaddr , 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;		//UDP
	myaddr.sin_addr.s_addr = htonl(0);	//any ip address
	myaddr.sin_port = htons(port);		//our specified port
	
	//bind the socket
	binding = ::bind(sock, (sockaddr *)&myaddr, sizeof(myaddr));
	if(binding < 0){
		perror("Socket not bound");
		return -1;
	}
	cout << "Socket bound" << endl;
	
	//Fill out the buffer
	buffersize = (width * 3) + 2;
	buffer = new unsigned char [buffersize];
	
	//Set the timeout for recieve operations
	struct timeval tv;
		/* 8 ms Timeout */
	tv.tv_sec  = 0;
	tv.tv_usec = 8 * 1000;
	
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
	
	//success!
	return 0;
}

void putScanLine(SDL_Surface* surface, int index, Uint32 scanLine[]){
	//init stack vars
	int wid, i;
	Uint32* surfacePixels;
	
	//load the pixels in the surface
	surfacePixels = (Uint32 *)surface->pixels;
	wid  = surface->w;
	for(i=0; i<wid; i++)
	{
		surfacePixels[(index * wid) + i] = scanLine[i];
	}
}
