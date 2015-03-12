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

	void makeMesh();
	
	SDL_Window *window;
	SDL_Surface *surf;
	netClientData *data;

	//GL vars
	GLuint program;
};

