#pragma once
#include "SDL.h"
#include "dataTypes.h"

#include "OVR_SDL2_nav.hpp"

class OculusRenderer : public OVR_SDL2_nav
{
public:
	
	OculusRenderer(netClientData *data);
	~OculusRenderer();

protected:

	virtual void step();
	virtual void draw();

private:

	float map(float val, float inStart, float inStop, float outStart, float outStop);
	void makeMesh();
	
	SDL_Window *window;
	SDL_Surface *surf;
	netClientData *data;
	const int POLYCOUNT = 1920 * 1080;

	//GL vars
	GLuint program;
};

