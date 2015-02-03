#include "OculusRenderer.h"


OculusRenderer::OculusRenderer(netClientData *data)
{
	this->data = data;
	window = SDL_CreateWindow("DARCI", 0, 0, 1920, 1080, SDL_WINDOWPOS_CENTERED);
	surf = SDL_GetWindowSurface(window);
}


OculusRenderer::~OculusRenderer()
{
}

void OculusRenderer::draw(){
	memcpy(surf->pixels, data->colorBuff, data->cAttrib.bytesPerPixel * data->cAttrib.width *data->cAttrib.height);
	SDL_UpdateWindowSurface(window);
}
