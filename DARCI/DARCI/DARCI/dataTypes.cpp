#include "dataTypes.h"

/*
	Video frame is used for both depth and color information.
	They had extremely similar methods, so it didn't make much
	sense to separate the two.
*/
videoFrame::videoFrame(int width, int height, VIDEOTYPE vt){
	res = new videoAttributes;
	res->height = height;
	res->width = width;

	switch (vt)
	{
	case vCOLOR:
		res->bytesPerPixel = 3; //RGBA
		break;
	case vDEPTH:
		res->bytesPerPixel = 2; //assuming 16 bits of accuracy
		break;
	default:
		printf("WARNING: UNIMPLEMENTED VIDEO TYPE.\n");
		res->bytesPerPixel = 0;
		break;
	}

	pixels = new unsigned char[getBufferByteLength()];
}


videoFrame::~videoFrame(){
	delete[] pixels;
	delete res;
}
int videoFrame::getWidth(){ return res->width; }
int videoFrame::getHeight(){ return res->height; }
int videoFrame::getBytesPerPixel(){ return res->bytesPerPixel; }
int videoFrame::getBufferByteLength(){ return res->bytesPerPixel * res->width * res->height; }
unsigned char* videoFrame::getBuffer(){ return pixels; }
void videoFrame::copyBuffer(unsigned char *data){
	memcpy(pixels, data, getBufferByteLength());
}
