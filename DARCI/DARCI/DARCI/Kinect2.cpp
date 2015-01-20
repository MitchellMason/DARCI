/*
	Implmentation of InputDevice for the Kinect for Windows V2.
	Windows compatable only (for now.)
*/

#include "Kinect2.h"

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
	if (initColor() + initDepth() != 0){
		printf("Kinect not started.\n");
		return -1;
	}

	return 0;
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

	//Color buffer without alpha
	int cByteBuffNoALen = vframe->getBytesPerPixel() * vframe->getWidth() * vframe->getHeight();
	BYTE *cByteBuffNoA = new BYTE[cByteBuffNoALen];

	//ensure the kinect is still open and ready
	BOOLEAN kinectOpen = true;
	BOOLEAN kinectReady= true;
	kinect->get_IsOpen(&kinectOpen);
	kinect->get_IsAvailable(&kinectReady);

	//Get latest frame
	if (kinectOpen && kinectReady){
		hr = colReader->AcquireLatestFrame(&kColData);

		//Ensure there were no errors
		if (SUCCEEDED(hr) && kColData){
			kColData->CopyConvertedFrameDataToArray(cByteBuffALen, cByteBuffA, ColorImageFormat_Bgra);
		}
		else{
			//Set the screen to green to indicate a waiting sensor
			for (int i = 0; i < cByteBuffNoALen; i += 3){
				cByteBuffNoA[i + 0] = 0x00; //R
				cByteBuffNoA[i + 1] = 0xFF; //G
				cByteBuffNoA[i + 2] = 0x00; //B
			}
		}
	}
	else{
		//Set the screen to red to indicate a disconnected sensor
		for (int i = 0; i < cByteBuffNoALen; i += 3){
			cByteBuffNoA[i + 0] = 0xFF; //R
			cByteBuffNoA[i + 1] = 0x00; //G
			cByteBuffNoA[i + 2] = 0x00; //B
		}
		printf("WARNING: KINECT SENSOR NOT KINNECTED.\n");
	}

	//copy the RGB channels from full buffer into the non-alpha one
	int i = 0, j = 0;
	bool done = false;
	while (!done){
		cByteBuffNoA[j] = cByteBuffA[i];
		i++;
		j++;
		cByteBuffNoA[j] = cByteBuffA[i];
		i++;
		j++;
		cByteBuffNoA[j] = cByteBuffA[i];
		i+=2; //Skip the alpha channel
		j++;
		if (i > cByteBuffALen || j > cByteBuffNoALen){
			done = true;
		}
	}
	vframe->copyBuffer(cByteBuffNoA);
	if (kColData)
		kColData->Release();
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
	BYTE *depByteBuff = new BYTE[totalPixels * sizeof(UINT16)];
	if (kinectOpen && kinectReady){
		//Get latest frame
		depReader->AcquireLatestFrame(&kDepData);
		if (kDepData){
			kDepData->CopyFrameDataToArray(kDepHeight * kDepWidth, depBuff);
		}
		else{
			//Set to white while waiting for depth channel to spin up.
			memset(depByteBuff, 0xFF, totalPixels * sizeof(UINT16));
			totalPixels = 0;
		}
	}
	else{
		//Set to black to indicate a loss of the Kinect sensor.
		memset(depBuff, 0, kDepHeight * kDepWidth);
		totalPixels = 0;
	}
	
	UINT16 pix = 0;
	for (short i = 0; i < totalPixels * sizeof(UINT16); i+=2){
		pix = depByteBuff[i];
		pix = pix << 8;
		pix = pix | depByteBuff[i + 1];
	}

	dframe->copyBuffer(depByteBuff);
	delete[] depBuff;
	delete[] depByteBuff;
}