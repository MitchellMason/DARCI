/*
	This class is responsible for fetching nessesary data for the 
	client. This includes color, depth, audio, and skeleton data.
	Yes, this will get used primarily for the kinect2 as of now,
	but if support for the kinect 1 is later desired, this makes
	it easier to patch in.
*/

#include "InputDevice.h"

int cHeight, cWidth, dHeight, dWidth;
bool started = false;

InputDevice::InputDevice(int colorHeight, int colorWidth, int depthHeight, int depthWidth)
{
	//initialize constant values
	cHeight = colorHeight;
	cWidth = colorWidth;
	dHeight = depthHeight;
	dWidth = depthWidth;
}

InputDevice::~InputDevice()
{
}