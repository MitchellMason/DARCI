#include "OculusRenderer.h"
#include <ctime>

static const char *vshader = R"(
	#version 150
	#line 1

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 proj;
	uniform sampler2D depthTex;
	
	in vec4 vPosition;
	in vec2 vTexCoord;

	out vec2 fTexCoord;
	out float depLookup; //delete

	void main(){
		fTexCoord = vTexCoord;
		vec4 displacedvPosition = vPosition;

		//Each pixel represents distance, in mm from the Kinect sensor
		depLookup = texture(depthTex, vTexCoord);
		displacedvPosition.z = depLookup * -275.0f;
		//displacedvPosition.z = 1.0f;
		//displacedvPosition.z = depLookup;

		gl_Position = proj * view * model * displacedvPosition;
	}
)";

static const char *fshader = R"(
	#version 150
	#line 1
	
	in vec2 fTexCoord;
	in float depLookup;

	uniform sampler2D colorTex;

	out vec4 fOutput;

	void main(){
		//if the depth map input was 0, it's depth is unknown, thus we shouldn't draw it.
		vec4 colorMap = texture2D(colorTex, fTexCoord);
		if(depLookup == 0){
			fOutput = vec4(0.0,0.0,0.0,0.0);
			//fOutput = vec4(colorMap.r, colorMap.g, colorMap.b, 1.0);
		}
		else{
			fOutput = vec4(colorMap.r, colorMap.g, colorMap.b, 1.0);
			//fOutput = vec4(mod(depLookup, 1.0),mod(depLookup, 1.0),mod(depLookup, 1.0),1.0);
			//fOutput = vec4(0.0,0.0,0.0,1.0);
		}
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
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		vPositionLoc = glGetAttribLocation(program, (const GLchar *) "vPosition");
		glVertexAttribPointer(vPositionLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(vPositionLoc);

		glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
		vTexCoordLoc = glGetAttribLocation(program, (const GLchar *) "vTexCoord");
		glVertexAttribPointer(vTexCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(vTexCoordLoc);
		
		//clear the screen
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

		//enable depth test and face culling
		glEnable(GL_DEPTH_TEST);

		//get the positions of shader data
		ModelMatrixLoc =      glGetUniformLocation(program, "model");
		ViewMatrixLoc =       glGetUniformLocation(program, "view");
		ProjectionMatrixLoc = glGetUniformLocation(program, "proj");
		ColorMapLoc =         glGetUniformLocation(program, "colorTex");
		DepthMapLoc =		  glGetUniformLocation(program, "depthTex");

		glPixelStorei(GL_PACK_SWAP_BYTES, GL_TRUE);

		//initialize textures
		glGenTextures((GLsizei)1, &depthTexture);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


		glGenTextures((GLsizei)1, &colorTexture);
		glBindTexture(GL_TEXTURE_2D, colorTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//Set the point sizes
		glPointSize(1.5f);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_POINT_SMOOTH);

		//Enable alpha so we can not draw unused points
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//set the texture registers
		glUniform1i(ColorMapLoc, 0);
		glUniform1i(DepthMapLoc, 1);

		//perform the first clear operation
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
		vertices[i + 0] = map(c, 0, MESH_WIDTH - 1, -1.0, 1.0); //x
		vertices[i + 1] = map(r, 0, MESH_HEIGHT - 1, -1.0, 1.0); //y
		vertices[i + 2] = 0.0f; //z, replaced by depth map

		//prepare for the next vert
		c++;
		if (c == MESH_WIDTH) {
			r++;
			c = 0;
		}
	}

	//UV coordinates
	int texCoordLen = MESH_HEIGHT * MESH_WIDTH * 2;
	float *texCoords = new float[texCoordLen];
	for (int i = 0, r = 0, c = 0; i < texCoordLen; i += 2, c++){
		texCoords[i + 0] = map(c, 0, MESH_WIDTH,  1, 0);
		texCoords[i + 1] = map(r, 0, MESH_HEIGHT, 1, 0);
		if (c == MESH_WIDTH){
			c = 0;
			r++;
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
	glGenBuffers((GLsizei)1, &texCoordBuffer);


	//bind the buffers and load the data
	
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
	glBufferData(GL_ARRAY_BUFFER, texCoordLen * sizeof(float), texCoords, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)verticesLen * sizeof(float), (const void *)vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triarrBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)trisLen * sizeof(int), (const void *)tris, GL_STATIC_DRAW);

	/*clean up*/
	delete[] vertices;
	delete[] texCoords;
	delete[] tris;

	/*Establish texture buffers*/

}

//utility function that remaps a value between 2 points
float OculusRenderer::map(float val, float inStart, float inStop, float outStart, float outStop){
	return outStart + (outStop - outStart) * ((val - inStart) / (inStop - inStart));
}

//draw logic here
void OculusRenderer::draw(){
	//Clear the buffer
	glUseProgram(program);
	glBindVertexArray(vertexArray);
	
	//only pipe new data to the GPU
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//color
	if (data->newData){
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	glTexImage2D(
		GL_TEXTURE_2D,					//target
		(GLint)0,						//level
		GL_RGB,							//internalformat
		(GLsizei)data->cAttrib.width,	//width
		(GLsizei)data->cAttrib.height,	//height
		(GLint)0,						//border
		GL_RGB,							//format
		GL_UNSIGNED_BYTE,				//type
		data->colorBuff					//pixels 
		);
	
	//depth
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(
		GL_TEXTURE_2D,					//target
		(GLint)0,						//level
		GL_R16,							//internalformat
		(GLsizei)data->dAttrib.width,	//width
		(GLsizei)data->dAttrib.height,	//height
		(GLint)0,						//border
		GL_RED,							//format
		GL_UNSIGNED_SHORT,				//type
		data->depthBuff					//pixels 
		);
	}

	//calculate and update the projection and view matricies
	mat4 P = projection();
	mat4 V = view();

	glUniformMatrix4fv(ProjectionMatrixLoc, 1, GL_TRUE, P);
	glUniformMatrix4fv(ViewMatrixLoc      , 1, GL_TRUE, V);
	
	//calculate and update the model matrix
	mat4 M = translation(vec3(0.0f, 0.5f, -5.0f));
	M = M * scale(vec3(MESH_WIDTH / 100.0f, MESH_HEIGHT / 100.0f, 1.0f));
	glUniformMatrix4fv(ModelMatrixLoc, 1, GL_TRUE, M);

	//draw the triangles
	//glDrawElements(GL_TRIANGLES, 2 * ((MESH_WIDTH - 1) * (MESH_HEIGHT - 1)) * 3, GL_UNSIGNED_INT, 0);
	glDrawArrays(GL_POINTS, 0, MESH_WIDTH * MESH_HEIGHT);

	//clean up
	glBindVertexArray(0);
	glUseProgram(0);
}
