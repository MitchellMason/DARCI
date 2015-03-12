#include "OculusRenderer.h"


static const char *vshader = R"(
	#version 150

	void main(){
		gl_Position = vec4(0,0,0,0);
	}
)";

static const char *fshader = R"(
	#version 150

	out vec4 fOutput;

	void main(){
		fOutput = vec4(0,0,0,0);
	}
)";

OculusRenderer::OculusRenderer(netClientData *data){
	this->data = data;

	if ((program = init_program(init_shader(GL_VERTEX_SHADER, vshader),
		init_shader(GL_FRAGMENT_SHADER, fshader)))){
		glUseProgram(program);
	}

	//clear the screen
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

	//enable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

}


OculusRenderer::~OculusRenderer(){
	glDeleteProgram(program);
}

void OculusRenderer::makeMesh(){
	
}

//animate here
void OculusRenderer::step(){
	//get the new depth data and send it to the GPU
}

//draw logic here
void OculusRenderer::draw(){
	//Clear the buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//draw the triangles
	glDrawArrays(GL_TRIANGLES, 0, 0);
}
