#include "OculusRenderer.h"

static const char *vshader = R"(
	#version 150

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 proj;
	
	in vec4 vPosition;
	in vec2 vertexTexCoord;
	in int depthMap;
	out int depth;

	void main(){
		depth = depthMap;
		gl_Position = proj * view * model * vPosition;
		gl_Position.z = depthMap * 100;
	}
)";

static const char *fshader = R"(
	#version 150
	
	uniform isampler2D colorTexture;

	out vec4 fOutput;

	void main(){
		fOutput = vec4(depth,0.0,0.0,1.0);
	}
)";

OculusRenderer::OculusRenderer(netClientData *data){
	this->data = data;

	if ((program = init_program(init_shader(GL_VERTEX_SHADER, vshader),
		init_shader(GL_FRAGMENT_SHADER, fshader)))){
		glUseProgram(program);
		
		//create and load the mesh data
		init();

		//get attribute pointers and assign them
		GLuint vPosLoc = glGetAttribLocation(program, (const GLchar *) "vPosition");
		glVertexAttribPointer(vPosLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(vPosLoc);

		GLuint vertexTexCoordLoc = glGetAttribLocation(program, (const GLchar *) "vertexTexCoord");
		glVertexAttribPointer(vertexTexCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(vertexTexCoordLoc);
		
		//clear the screen
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

		//enable depth test and face culling
		glEnable(GL_DEPTH_TEST);

		//get the positions of shader data
		ModelMatrixLoc =      glGetUniformLocation(program, "model");
		ProjectionMatrixLoc = glGetUniformLocation(program, "proj");
		ViewMatrixLoc =       glGetUniformLocation(program, "view");
		TextureLoc =          glGetUniformLocation(program, "colorTexture");
		DepthMapLoc =		  glGetUniformLocation(program, "depthMap");
	}
	else{
		printf("Could not create opengl program.\n");
		exit(-1);
	}
}


OculusRenderer::~OculusRenderer(){
	glDeleteProgram(program);
}

bool OculusRenderer::isRunning(){
	return running;
}

void OculusRenderer::init(){
	/*Create the mesh on the CPU side*/

	//verticies
	int verticesLen = MESH_WIDTH * MESH_HEIGHT * 3;
	float *vertices = new float[verticesLen];
	int r = 0; //x-index
	int c = 0; //z-index

	for (int i = 0; i < verticesLen; i += 3) {
		vertices[i + 0] = map(c, 0, MESH_WIDTH  - 1, -1.0, 1.0); //x
		vertices[i + 1] = map(r, 0, MESH_HEIGHT - 1, -1.0, 1.0); //y
		vertices[i + 2] = 0.0f; //z, replaced by depth map

		//prepare for the next vert
		c++;
		if (c == MESH_WIDTH) {
			r++;
			c = 0;
		}
	}

	//triangles
	int trisLen = 2 * ((MESH_WIDTH - 1) * (MESH_HEIGHT - 1)) * 3;
	int *tris = new int[trisLen];
	r = 0;
	c = 0;
	for (int i = 0; i < trisLen; i += 6) {
		//triangle 1
		tris[i + 0] = (r + 0) * MESH_WIDTH + (c + 0);
		tris[i + 1] = (r + 0) * MESH_WIDTH + (c + 1);
		tris[i + 2] = (r + 1) * MESH_WIDTH + (c + 0);
		//triangle 2
		tris[i + 3] = (r + 0) * MESH_WIDTH + (c + 1);
		tris[i + 4] = (r + 1) * MESH_WIDTH + (c + 0);
		tris[i + 5] = (r + 1) * MESH_WIDTH + (c + 1);

		//prepare for the next round of triangles
		c++;
		if (c == MESH_WIDTH - 1) {
			r++;
			c = 0;
		}
	}
	

	/*pipe the data over the bus to the GPU*/
	//create the buffers
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);
	glGenBuffers((GLsizei)1, &vertexBuffer);
	glGenBuffers((GLsizei)1, &triarrBuffer);
	
	//bind the buffers and load the data
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)verticesLen * sizeof(float), (const void *)vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triarrBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)trisLen * sizeof(int), (const void *)tris, GL_STATIC_DRAW);

	/*clean up*/
	delete[] vertices;
	delete[] tris;

	/*Establish texture buffers*/

}

//utility function that remaps a value between 2 points
float OculusRenderer::map(float val, float inStart, float inStop, float outStart, float outStop){
	return outStart + (outStop - outStart) * ((val - inStart) / (inStop - inStart));
}

//draw logic here
void OculusRenderer::draw(){
	
	//send the color texture
	while (data->colorLock == true){ /* wait on lock */ }
	data->colorLock = true;
		//TODO buffer color data here
	data->colorLock = false;
	
	//send the depth map
	while (data->depthLock == true){ /* wait on lock */ }
	data->depthLock = true;
		//TODO buffer depth data here
	glBindBuffer(GL_UNSIGNED_SHORT, DepthMapLoc);
	glBufferData(GL_UNSIGNED_SHORT, (GLsizeiptr)data->dAttrib.bytesPerPixel, data->depthBuff, GL_STATIC_DRAW);
	data->depthLock = false;
	

	//Clear the buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);
	glBindVertexArray(vertexArray);

	//calculate and update the projection and view matricies
	mat4 P = projection();
	mat4 V = view();

	glUniformMatrix4fv(ProjectionMatrixLoc, 1, GL_TRUE, P);
	glUniformMatrix4fv(ViewMatrixLoc      , 1, GL_TRUE, V);
	
	//calculate and update the model matrix
	mat4 M = translation(vec3(0.0f, 0.5f, -5.0f));
	M = M * scale(vec3(MESH_WIDTH / 100.0f, MESH_HEIGHT /100.0f, 1.0f));
	glUniformMatrix4fv(ModelMatrixLoc, 1, GL_TRUE, M);

	//draw the triangles
	//glDrawElements(GL_TRIANGLES, 2 * ((MESH_WIDTH - 1) * (MESH_HEIGHT - 1)) * 3, GL_UNSIGNED_INT, 0);
	glDrawArrays(GL_POINTS, 0, MESH_WIDTH * MESH_HEIGHT);

	//clean up
	glBindVertexArray(0);
	glUseProgram(0);
}
