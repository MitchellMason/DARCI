#pragma once
#include "InputDevice.h"
#include "Kinect.h" //actual MS provided header files

class Kinect2 : public InputDevice
{
public:
	Kinect2();
	~Kinect2();

	int start();
	void getColor(videoFrame* vframe);
	void getDepth(videoFrame* dframe);
	void getAudio(audioFrame* aframe);
	void getSkel(skelFrame* sframe);
	videoAttributes getColSpecs();
	videoAttributes getDepSpecs();

private:
	//init methods for the feeds
	int initColor();
	int initDepth();
	void onColorFrameArrive();

	//Constants about kinect feed data.
	const int kColWidth       = 1920;
	const int kColHeight      = 1080;
	const int kColBytesPerPix = 4;
	const int kDepWidth       = 512;
	const int kDepHeight      = 424;
	
	//MS type that stores results from API calls.
	HRESULT hr = NULL;

	//Basic kinect sensor
	IKinectSensor *kinect;

	//Color Frame Data
	IColorFrameSource *colSource = NULL;
	IColorFrameReader *colReader = NULL;
	IColorFrame        *kColData = NULL;

	//Depth Frame Data
	IDepthFrameSource *depSource = NULL;
	IDepthFrameReader *depReader = NULL;
	IDepthFrame       *kDepData  = NULL;
};

