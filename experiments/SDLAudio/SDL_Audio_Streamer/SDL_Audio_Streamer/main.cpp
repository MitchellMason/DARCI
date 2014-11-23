//
//  main.cpp
//  SDL_Audio_Streamer
//
//  Created by Mitchell Mason on 8/21/14.
//  Copyright (c) 2014 Mitchell Mason. All rights reserved.
//

#include <iostream>
#include <SDL2/SDL.h>
#include <netinet/in.h>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

//net vars
int port;
int sock;
int binding;
sockaddr_in myaddr;  //this computer's address
socklen_t addrlen = sizeof(myaddr);
timeval timeout;
long readyfds;
size_t  recvlen;
Uint8* netBuffer;    //The buffer specifically for audio from UDP
int netBufferPos;	 //The buffer's index in the audioBuffer
fd_set netfd;

//audio vars
bool soundIsPlaying   = false;
SDL_AudioSpec audioSpec;
int audioBlockLen = 8192; //each incoming packet is this many bytes (for now)
Uint32 audioLen; //length, in bytes, of the audio
int	   audioBufIndex = 0;
int    audioMaxBlockLen = 16;
Uint8* audioBuf;
Uint8* audioPos;

//app vars
bool sendMode;
bool recMode;
bool appIsRunning     = true;
bool isConnectedToNet = false;
const string sendModeFlag = "send";
const string recModeFlag  = "rec";
const char* pathToSound  = "./Sound.wav";
SDL_Event event;
SDL_Event previousEvent;
SDL_mutex* audioMutex;

//Initialize the UDP socket. Returns 0 on success, -1 on failure.
int initUDP(int port){
	//name the socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0){
		perror("Cannot name socket");
		return -1;
	}
	
	//Fill out the address structures.
	memset((char *)&myaddr , 0, sizeof(myaddr));
	myaddr.sin_family      = AF_INET;			//IP
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);	//any ip address
	myaddr.sin_port        = htons(port);		//our specified port
	
	//bind the socket
	if(recMode){
		binding = ::bind(sock, (sockaddr *)&myaddr, sizeof(myaddr));
		if(binding < 0){
			perror("Socket not bound");
			return -1;
		}
		printf("Socket bound!\n");
	}
	
	//success!
	return 0;
}

int cyclesWaited = 0;
void listen(){
	//Set the file descriptors
	FD_ZERO(&netfd);
	FD_SET(sock, &netfd); //set the udp socket to the the netfd.
	timeout.tv_sec  = 1; //1 sec
	timeout.tv_usec = 0; //0 ms
	
	//receive data over UDP
	readyfds = select(sock + 1, &netfd, NULL, NULL, &timeout);
	if(readyfds > 0){
		recvlen = recvfrom(sock, netBuffer, audioBlockLen, 0, (struct sockaddr *)&myaddr, &addrlen);
		
		if (recvlen > 0) {
			if(netBufferPos == 15){
				netBufferPos=0;
				printf("Resetting netbufferpos\n");
			}
			
			if(recvlen != audioBlockLen){
				printf("We may have some issues. recvlen: %lu, audioBlockLen: %i \n",recvlen, audioBlockLen);
			}
			
			//printf("\tData availible. Placing in buffer.\n");
			
			//TODO place lock here on audioBuf.
			memcpy(audioPos, netBuffer, audioBlockLen);
			//TODO unlock here
			
			audioPos = (netBufferPos * audioBlockLen) + audioBuf;
			cyclesWaited = 0;
		}
		else{
			printf("\tError, data length is %zi!\n",recvlen);
		}
	}
	//No ready file descriptors, and we have an error code
	else if(readyfds < 0){
		printf("\tError occured with code %i\n",errno);
	}
	//No ready fds
	else if (readyfds == 0){
		printf("\t%i No ready file descriptors\n",cyclesWaited++);
	}
}

//Plays the .wav file from ./sound.wav over UDP
void send(Uint8 *stream, int len){
	sendto(sock, stream, len, 0, (struct sockaddr *)&myaddr, sizeof(sockaddr_in));
}

//Utility function for parsing command line args
bool cmdOptionExists(char** begin, char** end, const string& option)
{
    return find(begin, end, option) != end;
}

//returns true if everything is valid and we can continue
bool parseCommandArgs(int argc, const char* argv[]){
	sendMode = cmdOptionExists((char**)argv, (char**)argv+argc, sendModeFlag);
	recMode  = cmdOptionExists((char**)argv, (char**)argv+argc, recModeFlag);
	if (argc == 3){
		port = atoi(argv[1]);
		if(!sendMode && !recMode){
			cout << "Invalid mode '" << argv[2] << "' selected. Please use 'send' or 'rec'" << endl;
			return false;
		}
		else if(sendMode && recMode){
			cout << "Please just use one mode at a time (on the same process anyways.)";
			return false;
		}
	}
	else{
		printf("Incorrect number of arguments given");
		return false;
	}
	return true; //everything looks good!
}

//Fills the audio buffer with sound from the hard drive (pretty standard)
void fillBufferFromDrive(void *udata, Uint8 *stream, int len){
	// Only play if we have data left
	if (audioLen == 0)
		return;
	
	//Set everything in the stream equal to zero.
	memset(stream, 0, len); //HA! Take that SDL examples.
	
	// Mix as much data as possible
	len = ( len > audioLen ? audioLen : len );
	SDL_MixAudio(stream, audioPos, len, SDL_MIX_MAXVOLUME);
	send(stream, len);
	
	audioPos += len;
	audioLen -= len;
	
	//Mute the audio at the last moment for testing purposes.
	memset(stream, 0, len);
}

//Plays audio in a very similar way to the drive. Placing audio in memory is handled in the listen() method.
void fillBufferFromSock(void *udata, Uint8 *stream, int len){
	//Set everything in the stream equal to zero.
	memset(stream, 0, len); //HA! Take that SDL examples.
	//printf("playing\n");
	
	// Mix as much data as possible
	SDL_LockMutex(audioMutex);
	SDL_MixAudio(stream, audioPos, len, SDL_MIX_MAXVOLUME);
	SDL_UnlockMutex(audioMutex);
	
	audioBufIndex++;
	if(audioBufIndex == audioMaxBlockLen){
		audioBufIndex = 0;
	}
}

//Begin here.
int main(int argc, const char * argv[])
{
	//Begin by parsing the command arguments.
	if(parseCommandArgs(argc, argv)){
		//Let the user know what they've selected
		if(sendMode){
			printf("Send audio on port %i\n",port);
		}
		else{
			printf("Listen for audio on port %i\n",port);
		}
	}
	else{
		printf("Command execution failed. Please try again with the pattern %s <int port> <send/recieve>\n" , argv[0]);
	}
	
	//Next, init the UDP socket
	if(initUDP(port) == 0){
		printf(">>>>UDP Acive\n");
	}
	else{
		printf("UDP not active. Quitting");
		return -1;
	}
	
	//Fire up SDL
	if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)){
		printf(">>>>SDL Active\n");
	}
	else{
		printf("SDL Failed with error:\n%s\n",SDL_GetError());
		return -1;
	}
	
	SDL_AudioSpec want, have;
	
	want.freq = 44100;
	want.format = AUDIO_S16;
	want.channels = 2;
	want.samples = 2048;
	
	if(sendMode)	want.callback = fillBufferFromDrive;
	else			want.callback = fillBufferFromSock;
	
	SDL_OpenAudio(&want, &have);
	
	
	//verify that what we want and what we have match up
	if(want.freq == have.freq && want.format == have.format && want.channels == have.channels && want.samples == have.samples){
		printf(">>>>Audio device succefully opened\n");
	}
	else{
		printf("Audio device opened with format conflicts from what we wanted.\nFreq is %i\nChannels are %i\nSamples are %i\nFormat is %i\n",have.freq, have.channels, have.samples, have.format);
		return -1;
	}
		
	
	/**********Done with general initialzations**********/
	if(sendMode){
		//Load the sound file
		printf("Sending audio from file %s\n", pathToSound);
		printf("Pause with the space bar\n");
		want = *SDL_LoadWAV_RW(SDL_RWFromFile(pathToSound, "rb"), 1, &have, &audioBuf, &audioLen);
				
		//Check we opened the audio file with correct settings.
		if(want.freq == have.freq && want.format == have.format && want.channels == have.channels && want.samples == have.samples){
			printf(">>>>Audio device specs match loaded file\n");
		}
		else{
			printf("Audio device opened with format conflicts against the file.\nFreq is %i\nChannels are %i\nSamples are %i\nFormat is %i\n",have.freq, have.channels, have.samples, have.format);
			return -1;
		}
		
		audioPos = audioBuf;
		SDL_PauseAudio(0); //Oddly enough, plays the sound
		while ( audioLen > 0 ) {
			SDL_PollEvent(&event);
			switch(event.type){
				case SDL_QUIT:
					printf("Stopping playback.\n");
					SDL_Quit();
					return 0;
			}
		}
		printf("Sound played. Bye!");
		SDL_Quit();
		return 0;
	}
	
	//recmode
	else{
		printf("\tiniting netbuffer.\n");
		//init the net buffer
		netBuffer = (Uint8 *) malloc(audioBlockLen);
		memset(netBuffer, 0, audioBlockLen);
		netBufferPos = 0;
		
		//Set up the sound buffer to be played from.
		printf("\tiniting audiobuffer\n");
		audioLen = audioBlockLen * audioMaxBlockLen;
		audioBuf = (Uint8*) malloc(audioLen);
		memset(audioBuf, 0, audioLen);
		audioPos = audioBuf;
		
		printf("\tCreating mutex\n");
		audioMutex = SDL_CreateMutex();
		
		printf("\tListening for audio.\n");
		
		SDL_PauseAudio(0);
		while(appIsRunning){
			listen();
			
			//handle SDL events
			SDL_PollEvent(&event);
			switch(event.type){
				case SDL_QUIT:
					printf("Stopping playback.\n");
					appIsRunning = false;
					SDL_Quit();
			}
		}
	}
}