/*
	This class is responsible for fetching nessesary data for the
	client. This includes color, depth, audio, and skeleton data.
	Yes, this will get used primarily for the kinect2 as of now,
	but if support for the kinect 1 is later desired, this makes
	it easier to patch in.

	The goal of the getters here should be to obtain and process
	data so that it can be sent over the sockets without any
	alteration. This means excess data is processed away,
	any compression is done (if implemented) and any other time-
	consuming tasks are done so that rendering is less complex.
*/

#pragma once
#include "dataTypes.h"

class InputDevice
{
public:
	InputDevice(int colorHeight, int colorWidth, int depthHeight, int depthWidth);
	~InputDevice();

	//overwritten methods for each possible device
	virtual int start() = 0;
	virtual void getData(cameraData* data) = 0;
	virtual videoAttributes getColSpecs() = 0;
	virtual videoAttributes getDepSpecs() = 0;
};

