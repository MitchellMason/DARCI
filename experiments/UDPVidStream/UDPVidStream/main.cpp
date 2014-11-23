//
//  main.cpp
//  UDPVidStream
//
//  Created by Mitchell Mason on 6/9/14.
//  Copyright (c) 2014 Mitchell Mason. All rights reserved.
//

#include <iostream>
#include "NetData.h"

using namespace NetData;
using namespace std;

int main(int argc, const char * argv[])
{
	cout << "declaring array" << endl;
	unsigned char data[100];
	bzero(data, sizeof(data));
	
	scanlinePacket p = getPacket(data);
	p.printData();
    return 0;
}

