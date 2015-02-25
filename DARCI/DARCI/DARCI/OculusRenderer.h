#pragma once
#include "SDL.h"
#include "dataTypes.h"
#include "OVR_SDL2_app.hpp"

class OculusRenderer
{
public:
	OculusRenderer(netClientData *data);
	~OculusRenderer();
	OVR_SDL2_app *app;

	void run();

private:
	SDL_Window *window;
	SDL_Surface *surf;
	netClientData *data;
};

