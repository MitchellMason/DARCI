/*
	Implmentation of InputDevice for the Kinect for Windows V2.
	Windows compatable only (for now.)
*/

#include "Kinect2.h"
#include <math.h>

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

int Kinect2::start(){
	//Start by getting the kinect sensor
	GetDefaultKinectSensor(&kinect);
	if (kinect){
		kinect->Open();
	}
	else{
		printf("Kinect device not found.");
		exit(-1);
	}

	//Wait for the kinect to be ready and availible.
	BOOLEAN ready = FALSE;
	while (!ready){
		kinect->get_IsOpen(&ready);
		Sleep(240);
	}
	printf("---Kinect open\n");
	ready = FALSE;
	while (!ready){
		kinect->get_IsAvailable(&ready);
		Sleep(240);
	}
	printf("---Kinect ready\n");

	//Initialize the data types that will hold feeds
	if (initColor() + initDepth() + initCoordMap()!= 0){
		printf("Kinect not started.\n");
		return -1;
	}

	return 0;
}

int Kinect2::initCoordMap(){ 
	hr = kinect->get_CoordinateMapper(&coordMapper);
	if (!SUCCEEDED(hr)){
		printf("Cannot get coordinate mapper\n");
		return -1;
	}
	else{
		return 0;
	}
}

int Kinect2::initColor(){
	hr = kinect->get_ColorFrameSource(&colSource);
	if (SUCCEEDED(hr)){
		hr = colSource->OpenReader(&colReader);
		if (!SUCCEEDED(hr)){
			printf("No color reader.\n");
			return -1;
		}
	}
	else{
		printf("No color source.\n");
		return -1;
	}
	return 0;
}

int Kinect2::initDepth(){
	hr = kinect->get_DepthFrameSource(&depSource);
	if (SUCCEEDED(hr)){
		hr = depSource->OpenReader(&depReader);
		if (!SUCCEEDED(hr)){
			printf("No color reader.\n");
			return -1;
		}
	}
	else{
		printf("No color source.\n");
		return -1;
	}
	return 0;
}

videoAttributes Kinect2::getColSpecs(){
	videoAttributes *result = new videoAttributes;
	result->bytesPerPixel = kColBytesPerPix;
	result->width = kColWidth;
	result->height = kColHeight;
	return *result;
}

videoAttributes Kinect2::getDepSpecs(){
	videoAttributes *result = new videoAttributes;
	result->bytesPerPixel = sizeof(UINT16);
	result->width = kDepWidth;
	result->height = kDepHeight;
	return *result;
}

void Kinect2::getAudio(audioFrame* aframe) {}
void Kinect2::getSkel(skelFrame* sframe)   {}

//stores the color channel of the kinect into the buffer of vframe.
void Kinect2::getColor(videoFrame* vframe) {
	//Color buffer with alpha
	int cByteBuffALen = kColHeight * kColWidth * kColBytesPerPix;
	BYTE *cByteBuffA = new BYTE[cByteBuffALen];

	//ensure the kinect is still open and ready
	BOOLEAN kinectOpen = true;
	BOOLEAN kinectReady= true;
	kinect->get_IsOpen(&kinectOpen);
	kinect->get_IsAvailable(&kinectReady);

	//Get latest frame
	if (kinectOpen && kinectReady){
		//make sure the data holder is clear
		if (kColData != NULL)
			kColData->Release();
		//The big error to watch out for here is E_PENDING. if it happens, just keep trying
		hr = colReader->AcquireLatestFrame(&kColData);
		while (hr == E_PENDING){ hr = colReader->AcquireLatestFrame(&kColData); }

		//Ensure there were no errors
		if (SUCCEEDED(hr) && kColData){
			kColData->CopyConvertedFrameDataToArray(cByteBuffALen, cByteBuffA, ColorImageFormat_Bgra);
		}
		else{
			//Set the screen to green to indicate a waiting sensor
			for (int i = 0; i < cByteBuffALen; i += 4){
				cByteBuffA[i + 0] = 0x00; //R
				cByteBuffA[i + 1] = 0xFF; //G
				cByteBuffA[i + 2] = 0x00; //B
				cByteBuffA[i + 3] = 0XFF; //A
			}
			printf("\rWARNING: KINECT SENSOR WAITING. HRESULT: %x\n", hr);
		}
	}
	else{
		//Set the screen to red to indicate a disconnected sensor
		for (int i = 0; i < cByteBuffALen; i += 4){
			cByteBuffA[i + 0] = 0xFF; //R
			cByteBuffA[i + 1] = 0x00; //G
			cByteBuffA[i + 2] = 0x00; //B
			cByteBuffA[i + 3] = 0XFF; //A
		}
		printf("\rWARNING: KINECT SENSOR NOT KINNECTED.\n");
	}

	//remove the alpha
	int cByteBuffNoALen = kColHeight * kColWidth * (kColBytesPerPix-1); //RGB only
	BYTE *cByteBuffNoA = new BYTE[cByteBuffNoALen];
	for (int i = 0, j = 0; i < cByteBuffALen; j += 3){
		cByteBuffNoA[j + 2] = cByteBuffA[i + 0];
		cByteBuffNoA[j + 1] = cByteBuffA[i + 1];
		cByteBuffNoA[j + 0] = cByteBuffA[i + 2];
		i += 4;
	}

	//copy the buffer and polish up
	vframe->copyBuffer(cByteBuffNoA);
	delete[] cByteBuffA;
	delete[] cByteBuffNoA;
}

void Kinect2::getDepth(videoFrame* dframe) {
	//ensure the kinect is still open and ready
	BOOLEAN kinectOpen = true;
	BOOLEAN kinectReady = true;
	kinect->get_IsOpen(&kinectOpen);
	kinect->get_IsAvailable(&kinectReady);

	int totalPixels = kDepHeight * kDepWidth;
	UINT16 *depBuff = new UINT16[totalPixels];
	if (kinectOpen && kinectReady){
		//Get latest frame
		hr = depReader->AcquireLatestFrame(&kDepData);
		while (hr == E_PENDING) hr = depReader->AcquireLatestFrame(&kDepData);
		if (SUCCEEDED(hr) && kDepData != NULL){
			kDepData->CopyFrameDataToArray(totalPixels, depBuff);

			//interpolate all depth errors out of the buffer
			for (int i = 1; i < totalPixels; i++){
				//any values equal to zero are errors.
				if (depBuff[i] == 0){
					//we know the leftmost good value, now we need the right
					int end = i;
					while (depBuff[end] == 0 && end < totalPixels-1){
						end++;
					}

					//smooth out the errors
					//printf("smoothing from %i to %i\n", i-1, end);
					//printf("Old values: ");
					//for (int k = i-1; k <= end; k++){
					//	printf(" %i", depBuff[k]);
					//}
					//printf("\n");
					lerp(&depBuff[i], end - i, depBuff[i-1], depBuff[end]);
					//printf("New values: ");
					//for (int k = i-1; k <= end; k++){
					//	printf(" %i", depBuff[k]);
					//}
					//printf("\n");
					i = end; //no need to check the values we just interpolated.
				}
			}
		}
		else{
			printf("Unknown error: %#x\n", hr);
		}
	}
	else{
		//Set to black to indicate a loss of the Kinect sensor.
		memset(depBuff, 0xFF, kDepHeight * kDepWidth);
		totalPixels = 0;
		printf("no kinect sensor\n");
	}

	if (kDepData) kDepData->Release();
	dframe->copyBuffer((unsigned char*) depBuff);
	delete[] depBuff;
}

//smooths out errors in the Depth buffer using linear interpolation
void Kinect2::lerp(UINT16 *start, UINT16 elements, UINT16 from, UINT16 to){
	double interval = ((double)(to - from)) / elements;
	for (int i = 0; i < elements; i++){
		start[i] = (i * interval) + from;
	}
}

void Kinect2::getData(cameraData* data){
	getColor(&(data->color));
	getDepth(&(data->depth));

	coordMapper->Release();
	hr = kinect->get_CoordinateMapper(&coordMapper);
	if (SUCCEEDED(hr)){
		int totalDepthPoints = data->depth.getHeight() * data->depth.getWidth();
		DepthSpacePoint *depthPoints = new DepthSpacePoint[totalDepthPoints];
		coordMapper->MapColorFrameToDepthSpace(
			totalDepthPoints,
			(const UINT16*)data->depth.getBuffer(),
			totalDepthPoints,
			depthPoints
			);
		float temp = depthPoints[0].X;
		for (int i = 1; i < totalDepthPoints; i++){
			if (depthPoints[i].X != temp)
				printf("Break! %f\n", depthPoints[i]);
		}
		//printf("Done mapping\n");
		delete[] depthPoints;
	}
	else{
		printf("Cannot map data. Unable to get new coordinate mapper. hr is %i\n", hr);
	}
}