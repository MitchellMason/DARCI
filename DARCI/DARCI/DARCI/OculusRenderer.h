#pragma once
#include "SDL.h"
#include "dataTypes.h"

class OculusRenderer
{
public:
	OculusRenderer(netClientData *data);
	~OculusRenderer();
	void draw();

private:
	SDL_Window *window;
	SDL_Surface *surf;
	netClientData *data;
};

