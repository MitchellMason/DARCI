#pragma once
#include "InputDevice.h"
#include "Kinect.h"
#include <algorithm>
#include "opencv2\imgproc.hpp"


class Kinect2 : public InputDevice
{
public:
	Kinect2();
	~Kinect2();

	int start();
	void getData(cameraData *data);
	videoAttributes getColSpecs();
	videoAttributes getDepSpecs();

private:
	//init methods for the feeds
	int initColor();
	int initDepth();
	int initCoordMap();
	void repairDepthData(UINT16* depthFrame);
	void lerp(UINT16 *start, UINT16 elements, UINT16 from, UINT16 to);
	int inline to2Dindex(int x, int y, int width);
	
	//get individual data
	void getColor(videoFrame* vframe);
	void getDepth(videoFrame* dframe);
	void getAudio(audioFrame* aframe);
	void getSkel(skelFrame* sframe);

	//Constants about kinect feed data.
	const int kColWidth       = 1920;
	const int kColHeight      = 1080;
	const int kColBytesPerPix = 4;
	const int kDepWidth       = 512;
	const int kDepHeight      = 424;
	const int kMappedColWidth = 512;
	const int kMappedColHeight = 424;
	
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

	//Coordinate Mapper
	ICoordinateMapper *coordMapper = NULL;

	//data used in coordinate mapping
	videoFrame *rawColor;
	BYTE *mappedColor;
	ColorSpacePoint *cSpace;
	const int cSpacePoints = kDepWidth * kDepHeight;

	//Buffer used in depth interpolation
	UINT16 *correctedDepth;
	UINT16 *histogram;
	const static int gridRadius = 8;
	UINT16 **columnHistogram;
	const int gridElementLen = gridRadius * gridRadius;
};

