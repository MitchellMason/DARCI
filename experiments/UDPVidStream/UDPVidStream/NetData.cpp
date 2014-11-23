//
//  NetData.cpp
//  UDPVidStream
//
//  Created by Mitchell Mason on 6/9/14.
//  Copyright (c) 2014 Mitchell Mason. All rights reserved.
//

#include "NetData.h"
#include <iostream>

#define byte unsigned char

using namespace NetData;
using namespace std;


//Define the scanlinePacket base class
scanlinePacket::scanlinePacket(){
}

scanlinePacket getPacket(byte rawData[]){
	short headerInfo;
	bool packetID1;
	bool packetID2;
	
	headerInfo = (rawData[1] << 8) | rawData[0];
	packetID1 = (bool)  headerInfo >> 7;
	packetID2 = (bool) (headerInfo >> 6) & 0x01;
	
	//rgb packets
	if(packetID1 && packetID2){
	}
	else if(packetID1 && !packetID2){
	}
	else if(!packetID1 && packetID2){
	}
	else{
	}
	return *new scanlinePacket();
}

void scanlinePacket::printData(){
	cout << this->bit1 << endl;
}