#include "OculusRenderer.h"


OculusRenderer::OculusRenderer(netClientData *data){
	this->app = new OVR_SDL2_app();
	this->data = data;
}


OculusRenderer::~OculusRenderer(){
}

void OculusRenderer::run(){
	this->app->run();
}
