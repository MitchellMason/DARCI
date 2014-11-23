//
//  NetData.h
//  UDPVidStream
//
//  Created by Mitchell Mason on 6/9/14.
//  Copyright (c) 2014 Mitchell Mason. All rights reserved.
//

#ifndef __UDPVidStream__NetTypes__
#define __UDPVidStream__NetTypes__

#include <iostream>

#endif /* defined(__UDPVidStream__NetTypes__) */
#define byte unsigned char
/*
 These are some data types we can receive from connections. All packets contain a
 header that gives a scanline index, and some array of either bytes or ints that
 contains that data.
 */
namespace NetData{
	class scanlinePacket{
	protected:
		//these bits will be used to determine packet type and other options
		bool bit1, bit2, bit3, bit4, bit5;
		short lineIndex;
		void* data[];
	public:
		scanlinePacket();
		virtual void printData();
	};
	
	class rgbPacket: scanlinePacket{};
	class rgbaPacket: scanlinePacket{};
	class meshPacket: scanlinePacket{};
	
	scanlinePacket getPacket(byte rawData[]);
}