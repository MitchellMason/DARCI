#include "Kinect2.h"
#include "Kinect.h" //actual MS provided header files

IKinectSensor *kinect;

const int kColWidth  = 1920;
const int kColHeight = 1080;
const int kDepWidth  = 512;
const int kDepHeight = 424;

HRESULT hr = NULL;

Kinect2::Kinect2() : InputDevice(kColHeight, kColWidth, kDepHeight, kDepWidth)
{
	;
}


Kinect2::~Kinect2()
{
	if (kinect){
		kinect->Close();
	}
}
