/*
	Implmentation of InputDevice for the Kinect for Windows V2.
	Windows compatable only (for now.)
*/

#include "Kinect2.h"

Kinect2::Kinect2() : InputDevice(kColHeight, kColWidth, kDepHeight, kDepWidth)
{

}

Kinect2::~Kinect2()
{
	if (kinect){
		kinect->Close();
	}
	delete[] cSpace;
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

	//Initialize the coordinate mapping buffer
	cSpace = new ColorSpacePoint[cSpacePoints];
	mappedColor = new BYTE[kMappedColHeight * kMappedColWidth * 3 * sizeof(BYTE)]; //RGB byte buffer
	rawColor = new videoFrame(kColWidth, kColHeight, vCOLOR);

	//initialize buffer used in depth map correction
	correctedDepth = new UINT16[kDepHeight * kDepWidth];
	histogram = new UINT16[gridElementLen];
	columnHistogram = new UINT16*[kDepWidth];
	for (int i = 0; i < kDepWidth; i++){
		columnHistogram[i] = new UINT16[gridRadius];
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
	result->width = kMappedColWidth;
	result->height = kMappedColHeight;
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
		start[i] = static_cast<UINT16>((i * interval) + from);
	}
}

int inline Kinect2::to2Dindex(int x, int y, int width){ return (y * width) + x; }

#define DEBUG FALSE

//cleans up depth data errors using the Fast Median filter
void Kinect2::repairDepthData(UINT16* depthFrame){
#if DEBUG > 0
	printf("DEPTH FRAME\n");
	for (int y = 0; y < gridRadius + 1; y++){
		for (int x = 0; x < gridRadius + 1; x++){
			printf("%i ",depthFrame[to2Dindex(x, y, kDepWidth)]);
		}
		printf("\n");
	}
#endif

	//initialize data containers
	UINT16 *sortedHistogram = new UINT16[gridElementLen];
	for (int i = 0; i < kDepWidth; i++){
		for (int j = 0; j < gridRadius; j++){
			columnHistogram[i][j] = depthFrame[(j*kDepWidth) + i];
		}
	}
	
	for (int i = 0; i < gridRadius; i++){
		for (int j = 0; j < gridRadius; j++){
			histogram[(i*gridRadius) + j] = columnHistogram[i][j];
		}
	}

	//determine the first value
	/*
	memcpy((void *)sortedHistogram, (void *)histogram, gridElementLen * sizeof(UINT16));
	std::sort(&(sortedHistogram[0]), &(sortedHistogram[gridElementLen - 1]));
	correctedDepth[gridRadius / 2] = sortedHistogram[gridElementLen / 2];
	*/
	for (int i = 0; i < kDepHeight; i++){
		for (int j = 0; j < kDepWidth; j++){
#if DEBUG > 0
			printf("\nColumn %i\n", j);
			printf("Before:[");
			for (int k = 0; k < gridRadius; k++){
				printf("%i ", columnHistogram[j][k]);
			}
			printf("]\n");
#endif
			//remove the first element in the current column
			memcpy(&(columnHistogram[j][0]), &(columnHistogram[j][1]), (gridRadius - 1) * sizeof(UINT16));
			
#if DEBUG > 0
			printf("After: [");
			for (int k = 0; k < gridRadius; k++){
				printf("%i ", columnHistogram[j][k]);
			}
			printf("]\n");
#endif
			//replace it with the new element to consider
			columnHistogram[j][gridRadius - 1] = depthFrame[to2Dindex(i, j + gridRadius, kDepWidth)];
			
#if DEBUG > 0
			printf("Final: [");
			for (int k = 0; k < gridRadius; k++){
				printf("%i ", columnHistogram[j][k]);
			}
			printf("]\n");
			
			printf("\nHistogram algorithm\n");
			printf("Before: {\n");
			for (int k1 = 0; k1 < gridRadius; k1++){
				printf("[");
				for (int k2 = 0; k2 < gridRadius; k2++){
					printf("%i ", histogram[to2Dindex(k2, k1, gridRadius)]);
				}
				printf("]\n");
			}
			printf("}\n");
#endif
			//delete the last column in the histogram and replace with the fresh column just calculated
			//for (int h = 0; h < gridRadius; h++){
			//	histogram[to2Dindex(0, h, gridRadius)] = columnHistogram[i+gridRadius][j+h];
			//}
			memcpy(&(histogram[0]), &(histogram[gridRadius]), (gridElementLen - gridRadius) * sizeof(UINT16));
			memcpy(&(histogram[gridElementLen - gridRadius]), &(columnHistogram[i + gridRadius][j]), gridRadius * sizeof(UINT16));

#if DEBUG > 0
			printf("After: {\n");
			for (int k1 = 0; k1 < gridRadius; k1++){
				printf("[");
				for (int k2 = 0; k2 < gridRadius; k2++){
					printf("%i ", histogram[to2Dindex(k2, k1, gridRadius)]);
				}
				printf("]\n");
			}
			printf("}\n");
#endif

			

			//move it to be sorted
			memcpy(sortedHistogram, histogram, gridElementLen * sizeof(UINT16));
			std::sort(&(sortedHistogram[0]), &(sortedHistogram[gridElementLen - 1]));

#if DEBUG > 0
			printf("output list: ");
			for (int i = 0; i < gridElementLen; i++){
				printf("%i ", sortedHistogram[i]);
			}
			printf("\n");

			printf("Final median value: %i", sortedHistogram[gridElementLen / 2]);
#endif
			correctedDepth[to2Dindex(j,i, kDepWidth) ] = sortedHistogram[gridElementLen / 2];
		}
#if DEBUG > 0
		printf("NEXT ROW: %i\n", i);
#endif
	}

	//finish up
	memcpy(depthFrame, correctedDepth, kDepWidth * kDepHeight * sizeof(UINT16));
	delete[] sortedHistogram;
}


void Kinect2::getData(cameraData* data){
	//get the raw data
	getColor(rawColor);
	getDepth(&(data->depth));
	unsigned char *rawColorBuffer = rawColor->getBuffer();
	unsigned char *depthBuffer = data->depth.getBuffer();

	//Reconstruct the image for point clouds
	coordMapper->Release();
	hr = kinect->get_CoordinateMapper(&coordMapper);
	if (SUCCEEDED(hr)){
		//Get the coordinate mapping object
		hr = coordMapper->MapDepthFrameToColorSpace(
			data->depth.getHeight() * data->depth.getWidth(),
			(UINT16*) data->depth.getBuffer(),
			cSpacePoints,
			cSpace
		);

		if (SUCCEEDED(hr)){
			//create the color frame from the whole image using the coordinate mapping data.
			unsigned char* pix = new unsigned char[3];
			for (int i = 0; i < cSpacePoints; i++){
				//Current color space coordinate
				ColorSpacePoint point = cSpace[i];

				if (point.X != -std::numeric_limits<float>::infinity() && 
					point.Y != -std::numeric_limits<float>::infinity()){
					//get the x and y index of the unmapped pixel to use
					int x = static_cast<float>(point.X + 0.5f);
					int y = static_cast<float>(point.Y + 0.5f);
					
					if (x > 0 && x < kColWidth && y > 0 && y < kColHeight){
						//get the index of that pixel in the unmapped image
						int pixIndex = (y * rawColor->getWidth()) + x;
						memcpy((void *)pix, (void *)&(rawColorBuffer[pixIndex * 3]), 3);
					}
				}
				else{
					depthBuffer[i] = 0;
				}

				//copy the RGB values from the mapped version into the new texture
				memcpy((void *)&(data->color.getBuffer()[i * 3]), (void *)pix, sizeof(BYTE) * 3);
			}
			delete[] pix;
		}
		else{
			printf("Unknown error: %#x\n", hr);
		}

	}
	else{
		printf("Cannot map data. Unable to get new coordinate mapper. hr is %i\n", hr);
	}

	//clean up the depth data for better rendering
	//for test input
#if DEBUG > 0
	printf("Generating test input\n");
	for (int i = 0; i < kDepWidth * kDepHeight; i++){
		((UINT16*)depthBuffer)[i] = i;
	}
	printf("Done\n");
#endif
	//repairDepthData((UINT16*)depthBuffer);
	cv::medianBlur()
}