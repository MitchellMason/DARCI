#pragma once

#include "InputDevice.h"
#include "Kinect2.h"
#include <thread>

class NetServer
{
public:
	NetServer();
	~NetServer();
	void start();

private:
	void run();

	InputDevice *camera;
	int port;
	char *addr;
	bool running = false;
};

