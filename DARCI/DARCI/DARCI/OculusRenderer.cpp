#include "OculusRenderer.h"

/*
	Mesh-generation in javascript

	vertices = new Float32Array(n * n * 3);
    var r = 0; //x-index
    var c = 0; //z-index
    for (var i = 0; i < vertices.length; i += 3) {
        //determine x and z first, becuase y is the dependent var
        vertices[i] = map(c, 0, n - 1, -1.0, 1.0); //x
        vertices[i + 2] = map(r, 0, n - 1, 1.0, -1.0); //z
        vertices[i + 1] = CalculateY(vertices[i], vertices[i + 2]); //y

        //prepare for the next vert
        c++;
        if (c == n) {
            r++;
            c = 0.0;
        }
    }
    //Done! print the table for reference
    if (debug) logVertTable(vertices);

    //Set the triangles element array
    var tris = new Uint16Array(2 * ((n - 1) * (n - 1)) * 3);
    r = 0;
    c = 0;
    for (var i = 0; i < tris.length; i += 6) {
        //triangle 1
        tris[i] = (r + 0) * n + (c + 0);
        tris[i + 1] = (r + 1) * n + (c + 0);
        tris[i + 2] = (r + 0) * n + (c + 1);
        //triangle 2
        tris[i + 3] = (r + 0) * n + (c + 1);
        tris[i + 4] = (r + 1) * n + (c + 0);
        tris[i + 5] = (r + 1) * n + (c + 1);

        //prepare for the next round of triangles
        c++;
        if (c == n - 1) {
            r++;
            c = 0.0;
        }
    }
 */

static const char *vshader = R"(
	#version 150

	uniform mat4 model;
	uniform mat4 proj;
	
	in  vec4 vPosition;
	
	void main(){
		gl_Position = proj * model * vPosition;
	}
)";

static const char *fshader = R"(
	#version 150

	out vec4 fOutput;

	void main(){
		fOutput = vec4(0,0,0,1.0);
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
}


OculusRenderer::~OculusRenderer(){
	glDeleteProgram(program);
}

void OculusRenderer::makeMesh(){
	/*Create the mesh on the CPU side*/

	//verticies
	int verticesLen = POLYCOUNT * POLYCOUNT * 3;
	float *vertices = new float[verticesLen];
	int r = 0; //x-index
	int c = 0; //z-index

	for (int i = 0; i < verticesLen; i += 3) {
		vertices[i] = map(c, 0, POLYCOUNT - 1, -1.0, 1.0); //x
		vertices[i + 2] = map(r, 0, POLYCOUNT - 1, 1.0, -1.0); //z
		vertices[i + 1] = 0.0f; //y, replaced by depth map

		//prepare for the next vert
		c++;
		if (c == POLYCOUNT) {
			r++;
			c = 0.0;
		}
	}

	//triangles
	int trisLen = 2 * ((POLYCOUNT - 1) * (POLYCOUNT - 1)) * 3;
	short *tris = new short[trisLen];
	r = 0;
	c = 0;
	for (int i = 0; i < trisLen; i += 6) {
		//triangle 1
		tris[i] = (r + 0) * POLYCOUNT + (c + 0);
		tris[i + 1] = (r + 1) * POLYCOUNT + (c + 0);
		tris[i + 2] = (r + 0) * POLYCOUNT + (c + 1);
		//triangle 2
		tris[i + 3] = (r + 0) * POLYCOUNT + (c + 1);
		tris[i + 4] = (r + 1) * POLYCOUNT + (c + 0);
		tris[i + 5] = (r + 1) * POLYCOUNT + (c + 1);

		//prepare for the next round of triangles
		c++;
		if (c == POLYCOUNT - 1) {
			r++;
			c = 0.0;
		}
	}
	

	/*pipe the data over the bus to the GPU*/
	GLuint *vertexBuffer = NULL;
	GLuint *triarrBuffer = NULL;
	//create the buffers
	glGenBuffers((GLsizei)1, vertexBuffer);
	glGenBuffers((GLsizei)1, triarrBuffer);
	//bind the buffers
	glBindBuffer(GL_ARRAY_BUFFER, *vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *triarrBuffer);
	//send the data
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)verticesLen, (const void *)vertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)trisLen, (const void *)tris, GL_STATIC_DRAW);
	//get attribute pointers and assign them
	GLuint v_pos = glGetAttribLocation(program, (const GLchar *) "v_Position");
	glVertexAttribPointer(v_pos, POLYCOUNT, GL_FLOAT, GL_TRUE, 0, NULL);
	glEnableVertexAttribArray(v_pos);

	/*clean up*/
	delete[] vertices;
	delete[] tris;
}

float OculusRenderer::map(float val, float inStart, float inStop, float outStart, float outStop){
	return outStart + (outStop - outStart) * ((val - inStart) / (inStop - inStart));
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
	glDrawArrays(GL_TRIANGLES, 0, 1920*1080);
}
