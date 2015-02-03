#include "OculusRenderer.h"


OculusRenderer::OculusRenderer(netClientData *data)
{
	this->data = data;
	window = SDL_CreateWindow("DARCI", 0, 0, 1920, 1080, SDL_WINDOWPOS_CENTERED | SDL_WINDOW_FULLSCREEN);
}


OculusRenderer::~OculusRenderer()
{
}

void OculusRenderer::draw(){
	SDL_Surface *surf = SDL_GetWindowSurface(window);
	memcpy(surf->pixels, data->colorBuff, 1920 * 1080 * 4); 
}
