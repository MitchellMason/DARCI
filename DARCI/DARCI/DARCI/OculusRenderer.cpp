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

OculusRenderer::OculusRenderer(netClientData *data){
	this->app = new OVR_SDL2_app();
	this->data = data;
}


OculusRenderer::~OculusRenderer(){
}

void OculusRenderer::run(){
	this->app->run();
}
