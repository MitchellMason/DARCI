#pragma once
class InputDevice
{
public:
	InputDevice(int colorHeight, int colorWidth, int depthHeight, int depthWidth);
	~InputDevice();
	virtual void getColor() = 0;
	virtual void getDepth() = 0;
	virtual void getAudio() = 0;
	virtual void getSkel()  = 0;
};

