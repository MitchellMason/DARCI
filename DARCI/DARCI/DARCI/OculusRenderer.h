#pragma once
#include "SDL.h"
#include "dataTypes.h"

#include "OVR_SDL2_nav.hpp"

class OculusRenderer : public OVR_SDL2_nav
{
public:
	
	OculusRenderer(netClientData *data);
	~OculusRenderer();

	bool isRunning();

protected:

	virtual void draw();

private:

	float map(float val, float inStart, float inStop, float outStart, float outStop);
	void init();
	
	SDL_Window *window;
	SDL_Surface *surf;
	SDL_Event event;
	netClientData *data;
	const int MESH_WIDTH = 512;
	const int MESH_HEIGHT  = 424;

	//GL vars
	GLuint program;
	GLuint vertexArray;

	GLuint vertexBuffer;
	GLuint triarrBuffer;
	GLuint texCoordBuffer;

	GLuint depthTexture;
	GLuint colorTexture;
	
	mat4 modelView;
	mat4 Projection;

	GLint vTexCoord;
	GLint vPositionLoc;
	GLint vTexCoordLoc;

	GLint ModelMatrixLoc;
	GLint ViewMatrixLoc;
	GLint ProjectionMatrixLoc;
	GLint ColorMapLoc;
	GLint DepthMapLoc;
	GLint FarClipFloatLoc;
};

