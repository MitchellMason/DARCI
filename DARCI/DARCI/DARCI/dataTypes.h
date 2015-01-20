/*
	Provides some convienience classes and methods for frames of information like color,
	depth, sound, and skeleton data.
*/
#pragma once
#include <iostream>

typedef struct {
	unsigned int width;
	unsigned int height;
	unsigned char bytesPerPixel;
} videoAttributes;

typedef struct {
	bool colorLock;
	unsigned char *colorBuff;
	videoAttributes cAttrib;
	bool depthLock;
	unsigned char *depthBuff;
	videoAttributes dAttrib;
} netClientData;

enum VIDEOTYPE{
	vCOLOR, vDEPTH
};

enum SOCKTYPE
{
	COLOR, DEPTH, AUDIO, SKELETON, REMOTE_EVENTS
};


class videoFrame
{
public:
	videoFrame(int width, int height, VIDEOTYPE vt);
	~videoFrame();
	int getWidth();
	int getHeight();
	int getBytesPerPixel();
	int getBufferByteLength();
	unsigned char* getBuffer();
	void copyBuffer(unsigned char *data);

private:
	videoAttributes *res;
	unsigned char *pixels;
};

class audioFrame{};
class skelFrame{};

