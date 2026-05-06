#ifndef MESH_H_
#define MESH_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <Vector.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

//! Mesh Class input and rendering
class Mesh
{

//
public:

    //! Constructor
    Mesh() : positionBuffer(0), normalBuffer(0), texcoordBuffer(0){};

    //! Destructor
    ~Mesh(){};

	//! Load and OBJ mesh from File
    bool loadOBJ(std::string filename);

	//! Creates geometry for a cube 
	void initCube();
	
	//! Create geometry for triangle
	void initTriangle();

	//! Create geometry for triangle
	void initQuad();
	
	//!Draw Function for Mesh
    void Draw(GLuint vertexPositionAttribute, GLuint vertexNormalAttribute = -1, GLuint vertexTexcordAttribute = -1 );

  	//! Returns Mesh Centroid
	Vector3f getMeshCentroid();
	
//!
private:

	//! Init
	void initBuffers();

	//Face structure
	struct Face
	{
		std::vector<unsigned int> position_index;
		std::vector<unsigned int> normal_index;
		std::vector<unsigned int> texturecoord_index;
	};

	//! Mesh Positions
	std::vector<Vector3f> positions;

	//! Mesh Normals
	std::vector<Vector3f> normals;

	//! Mesh Tex Coords
	std::vector<Vector2f> texcoords;
	
	//! Mesh Faces
	std::vector<Face> faces;

private:

    //! OpenGL Vertex Positon Buffer
    GLuint positionBuffer;
 
    //! OpenGL Vertex Normal Buffer
    GLuint normalBuffer;
     
    //! OpenGL Vertex Tex Coord Buffer
    GLuint texcoordBuffer;

};

#endif
