#include "Mesh.h"

//
bool Mesh::loadOBJ(std::string filename)
{
	/**
	 * OBJ file format:
	 * '#'  = comments
	 * 'v'  = vertex coordinates: 3 floats x-y-z
	 * 'vt' = vertex texture coordinates: 2 floats u-v
	 * 'vn' = vertex normals: 3 floats x-y-z
	 * 'f'  = faces are represented by a set of id numbers separated by a "/" and space :vertex_id/texture_id/normal_id
	 *  For example: f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
	 */

	std::ifstream filestream;
	filestream.open(filename.c_str());
	
	std::string line_stream;
	while(std::getline(filestream, line_stream))
	{
		std::stringstream str_stream(line_stream);
		std::string type_str;
		str_stream >> type_str;
	
		if(type_str == "v")
		{
			Vector3f position;
			str_stream >> position.x >> position.y >> position.z;
			positions.push_back(position);
		}
		else if(type_str == "vt")
		{
			Vector2f texture;
			str_stream >> texture.x >> texture.y;
			texcoords.push_back(texture);
		}
		else if(type_str == "vn")
		{
			Vector3f normal;
			str_stream >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if(type_str == "f")
		{
			char temp;
			Face face;
			unsigned int v1,v2,v3;
			for(int i = 0; i < 3; ++i)
			{
				str_stream >> v1 >> temp >> v2  >> temp >> v3;
				face.position_index.push_back(v1-1);
				face.texturecoord_index.push_back(v2-1);
				face.normal_index.push_back(v3-1);
			}
			faces.push_back(face);
		}
	}
	
	// Explicit closing of the file
	filestream.close();

	//Report Input
	std::cout 	<< "Loaded " 			<< filename 		<< "\n" 
				<< "\t Positions: " 	<< positions.size() << "\n" 
				<< "\t Normals: " 		<< normals.size() 	<< "\n" 
				<< "\t Tex Coords: " 	<< texcoords.size() << "\n" 
				<< "\t Faces: " 		<< faces.size() 	<< "\n" << std::endl;
				
				
	initBuffers();
	
	return true;
}

//! Init Vertex array Buffers
void Mesh::initBuffers()
{
	std::cout << "Start Init Mesh Buffers" << std::endl;
	
	// init buffers
	glGenBuffers(1, &positionBuffer);
	glGenBuffers(1, &normalBuffer);
	glGenBuffers(1, &texcoordBuffer);
	
	//Data
	std::vector<GLfloat> vertexPositionData;
	std::vector<GLfloat> vertexNormalData;
	std::vector<GLfloat> vertexTexcoordData;

	//Go through each face and add to to lists
	for(int face_i = 0 ; face_i < faces.size(); face_i++)
	{
		Face face = faces[face_i];
		
		for(int vertex_i = 0 ; vertex_i < 3; vertex_i++)
		{
			if(positions.size() > 0)
			{
				Vector3f v = positions[face.position_index[vertex_i]];
				vertexPositionData.push_back(v.x);
				vertexPositionData.push_back(v.y);
				vertexPositionData.push_back(v.z);
			}
			
			//Add Normals
			if(normals.size() > 0)
			{
				Vector3f n = normals[face.normal_index[vertex_i]];
				vertexNormalData.push_back(n.x);
				vertexNormalData.push_back(n.y);
				vertexNormalData.push_back(n.z);
			}
			
			//Add texture Coords
			if(texcoords.size() > 0)
			{
				Vector2f t = texcoords[face.texturecoord_index[vertex_i]];
				vertexTexcoordData.push_back(t.x);
				vertexTexcoordData.push_back(t.y);
			}
		}
	}
	
	//Set Data for Position buffer
	if(positions.size() > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		glBufferData(GL_ARRAY_BUFFER, vertexPositionData.size() * sizeof(GLfloat), &vertexPositionData[0], GL_STATIC_DRAW);
	}
	
	//Set Data for Normal buffer
	if(normals.size() > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
		glBufferData(GL_ARRAY_BUFFER, vertexNormalData.size() * sizeof(GLfloat), &vertexNormalData[0], GL_STATIC_DRAW);
	}

	//set data for texcoord buffer
	if(texcoords.size() > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
		glBufferData(GL_ARRAY_BUFFER, vertexTexcoordData.size() * sizeof(GLfloat), &vertexTexcoordData[0], GL_STATIC_DRAW);
	}
	std::cout << "End Init Mesh Buffers" << std::endl;

}

//Function to draw a mesh 
void Mesh::Draw(GLuint vertexPositionAttribute, GLuint vertexNormalAttribute, GLuint vertexTexcordAttribute)
{
	// Vertex Position attribute and buffer
	if(positions.size() > 0)
	{
		glEnableVertexAttribArray(vertexPositionAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		glVertexAttribPointer(
			vertexPositionAttribute, 		// The attribute we want to configure
			3,                  			// size
			GL_FLOAT,        			    // type
			GL_FALSE,           			// normalized?
			0,                  			// stride
			(void*)0            			// array buffer offset
		);
	}

	if(normals.size() > 0 && vertexNormalAttribute != -1)
	{
		glEnableVertexAttribArray(vertexNormalAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
		glVertexAttribPointer(
			vertexNormalAttribute, 		// The attribute we want to configure
			3,                 		 	// size
			GL_FLOAT,           		// type
			GL_FALSE,           		// normalized?
			0,                  		// stride
			(void*)0           			// array buffer offset
		);
	}

	if(texcoords.size() > 0 && vertexTexcordAttribute != -1)
	{
		glEnableVertexAttribArray(vertexTexcordAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
		glVertexAttribPointer(
			vertexTexcordAttribute, 	// The attribute we want to configure
			2,                  		// size
			GL_FLOAT,           		// type
			GL_FALSE,          			// normalized?
			0,                 			// stride
			(void*)0           			// array buffer offset
		);
	}

	//Draw Arrays
	glDrawArrays(GL_TRIANGLES, 0, faces.size() * 3); 
	
	//Disable Vertex Position Array
	if(positions.size() > 0)
	{
		glDisableVertexAttribArray(vertexPositionAttribute);
	}
	
	//Disable Vertex Normal Array
	if(normals.size() > 0  && vertexNormalAttribute != -1)
	{
		glDisableVertexAttribArray(vertexNormalAttribute);
	}

	//Disable Vertex TexCoord Array
	if(texcoords.size() > 0 && vertexTexcordAttribute != -1)
	{
		glDisableVertexAttribArray(vertexTexcordAttribute);
	}
}

//! Returns Mesh Centroid
Vector3f Mesh::getMeshCentroid()
{
	//Variables for average positions
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

	//Accumulate values of all position
	for(int i = 0 ; i < positions.size(); i++)
	{
		Vector3f v = positions[i];
		x += v.x;
		y += v.y;
		z += v.z;
	}

	//Average each positon
	x/= positions.size();
	y/= positions.size();
	z/= positions.size();
	
	// return result
	return	Vector3f(x,y,z);
}


void Mesh::initTriangle()
{
	positions.push_back(Vector3f(-0.5,	-0.5,	0.0));
	positions.push_back(Vector3f( 0.5,	-0.5,	0.0));
	positions.push_back(Vector3f( 0.0,	 0.5,	0.0));
	
	normals.push_back(Vector3f(0.0,0.0,1.0));

	texcoords.push_back(Vector2f(0.0,1.0));
	texcoords.push_back(Vector2f(0.0,0.0));
	texcoords.push_back(Vector2f(0.0,1.0));
	
	Face face;
	face.position_index.push_back(0);		face.position_index.push_back(1);		face.position_index.push_back(2);
	face.normal_index.push_back(0);			face.normal_index.push_back(0);			face.normal_index.push_back(0);
	face.texturecoord_index.push_back(0);	face.texturecoord_index.push_back(1);	face.texturecoord_index.push_back(2);
	faces.push_back(face);

	initBuffers();
}

//! Create geometry for triangle
void Mesh::initQuad()
{
	positions.push_back(Vector3f( 0.0,	1.0,	0.0));
	positions.push_back(Vector3f( 0.0,	0.0,	0.0));
	positions.push_back(Vector3f( 1.0,	0.0,	0.0));
	positions.push_back(Vector3f( 1.0,	1.0,	0.0));

	normals.push_back(Vector3f(0.0,0.0,1.0));

	texcoords.push_back(Vector2f(0.0,1.0));
	texcoords.push_back(Vector2f(0.0,0.0));
	texcoords.push_back(Vector2f(1.0,0.0));
	texcoords.push_back(Vector2f(1.0,1.0));

	//
	Face face1;
	face1.position_index.push_back(0);		face1.position_index.push_back(1);		face1.position_index.push_back(2);
	face1.normal_index.push_back(0);		face1.normal_index.push_back(0);		face1.normal_index.push_back(0);
	face1.texturecoord_index.push_back(0);	face1.texturecoord_index.push_back(1);	face1.texturecoord_index.push_back(2);
	faces.push_back(face1);
	Face face2;
	face2.position_index.push_back(0);		face2.position_index.push_back(2);		face2.position_index.push_back(3);
	face2.normal_index.push_back(0);		face2.normal_index.push_back(0);		face2.normal_index.push_back(0);
	face2.texturecoord_index.push_back(0);	face2.texturecoord_index.push_back(2);	face2.texturecoord_index.push_back(3);
	faces.push_back(face2);

	initBuffers();
}


void Mesh::initCube()
{
	// Vertex Data
	positions.push_back(Vector3f( -1.0,  1.0,  1.0));
	positions.push_back(Vector3f( -1.0, -1.0,  1.0));
	positions.push_back(Vector3f(  1.0, -1.0,  1.0));
	positions.push_back(Vector3f(  1.0,  1.0,  1.0));
	positions.push_back(Vector3f( -1.0,  1.0, -1.0));
	positions.push_back(Vector3f(  1.0,  1.0, -1.0));
	positions.push_back(Vector3f( -1.0, -1.0, -1.0));
	positions.push_back(Vector3f(  1.0, -1.0, -1.0));
	
	// Normals
	normals.push_back(Vector3f(  0.0,  0.0,  1.0));
	normals.push_back(Vector3f(  1.0,  0.0,  0.0));
	normals.push_back(Vector3f(  0.0,  0.0, -1.0));
	normals.push_back(Vector3f( -1.0,  0.0,  0.0));
	normals.push_back(Vector3f(  0.0,  1.0,  0.0));
	normals.push_back(Vector3f(  0.0, -1.0,  0.0));

	// TexCoords
	texcoords.push_back(Vector2f( 0.0, 1.0));
	texcoords.push_back(Vector2f( 0.0, 0.0));
	texcoords.push_back(Vector2f( 0.0, 1.0));
	texcoords.push_back(Vector2f( 1.0, 1.0));

	//
	Face face1a;
	face1a.position_index.push_back(0);		face1a.position_index.push_back(1);		face1a.position_index.push_back(2);
	face1a.normal_index.push_back(0);		face1a.normal_index.push_back(0);		face1a.normal_index.push_back(0);
	face1a.texturecoord_index.push_back(0);	face1a.texturecoord_index.push_back(1);	face1a.texturecoord_index.push_back(2);
	faces.push_back(face1a);
	Face face1b;
	face1b.position_index.push_back(0);		face1b.position_index.push_back(2);		face1b.position_index.push_back(3);
	face1b.normal_index.push_back(0);		face1b.normal_index.push_back(0);		face1b.normal_index.push_back(0);
	face1b.texturecoord_index.push_back(0);	face1b.texturecoord_index.push_back(2);	face1b.texturecoord_index.push_back(3);
	faces.push_back(face1b);
	
	//
	Face face2a;
	face2a.position_index.push_back(3);		face2a.position_index.push_back(2);		face2a.position_index.push_back(7);
	face2a.normal_index.push_back(1);		face2a.normal_index.push_back(1);		face2a.normal_index.push_back(1);
	face2a.texturecoord_index.push_back(0);	face2a.texturecoord_index.push_back(1);	face2a.texturecoord_index.push_back(2);
	faces.push_back(face2a);
	Face face2b;
	face2b.position_index.push_back(3);		face2b.position_index.push_back(7);		face2b.position_index.push_back(5);
	face2b.normal_index.push_back(1);		face2b.normal_index.push_back(1);		face2b.normal_index.push_back(1);
	face2b.texturecoord_index.push_back(0);	face2b.texturecoord_index.push_back(2);	face2b.texturecoord_index.push_back(3);
	faces.push_back(face2b);
	
	//
	Face face3a;
	face3a.position_index.push_back(5);		face3a.position_index.push_back(7);		face3a.position_index.push_back(6);
	face3a.normal_index.push_back(2);		face3a.normal_index.push_back(2);		face3a.normal_index.push_back(2);
	face3a.texturecoord_index.push_back(0);	face3a.texturecoord_index.push_back(1);	face3a.texturecoord_index.push_back(2);
	faces.push_back(face3a);
	Face face3b;
	face3b.position_index.push_back(5);		face3b.position_index.push_back(6);		face3b.position_index.push_back(4);
	face3b.normal_index.push_back(2);		face3b.normal_index.push_back(2);		face3b.normal_index.push_back(2);
	face3b.texturecoord_index.push_back(0);	face3b.texturecoord_index.push_back(2);	face3b.texturecoord_index.push_back(3);
	faces.push_back(face3b);
	
	//
	Face face4a;
	face4a.position_index.push_back(4);		face4a.position_index.push_back(6);		face4a.position_index.push_back(1);
	face4a.normal_index.push_back(3);		face4a.normal_index.push_back(3);		face4a.normal_index.push_back(3);
	face4a.texturecoord_index.push_back(0);	face4a.texturecoord_index.push_back(1);	face4a.texturecoord_index.push_back(2);
	faces.push_back(face4a);
	Face face4b;
	face4b.position_index.push_back(4);		face4b.position_index.push_back(1);		face4b.position_index.push_back(0);
	face4b.normal_index.push_back(3);		face4b.normal_index.push_back(3);		face4b.normal_index.push_back(3);
	face4b.texturecoord_index.push_back(0);	face4b.texturecoord_index.push_back(2);	face4b.texturecoord_index.push_back(3);
	faces.push_back(face4b);
	
	//
	Face face5a;
	face5a.position_index.push_back(5);		face5a.position_index.push_back(4);		face5a.position_index.push_back(0);
	face5a.normal_index.push_back(4);		face5a.normal_index.push_back(4);		face5a.normal_index.push_back(4);
	face5a.texturecoord_index.push_back(0);	face5a.texturecoord_index.push_back(1);	face5a.texturecoord_index.push_back(2);
	faces.push_back(face5a);
	Face face5b;
	face5b.position_index.push_back(5);		face5b.position_index.push_back(0);		face5b.position_index.push_back(3);
	face5b.normal_index.push_back(4);		face5b.normal_index.push_back(4);		face5b.normal_index.push_back(4);
	face5b.texturecoord_index.push_back(0);	face5b.texturecoord_index.push_back(2);	face5b.texturecoord_index.push_back(3);
	faces.push_back(face5b);
	
	//
	Face face6a;
	face6a.position_index.push_back(2);		face6a.position_index.push_back(1);		face6a.position_index.push_back(6);
	face6a.normal_index.push_back(5);		face6a.normal_index.push_back(5);		face6a.normal_index.push_back(5);
	face6a.texturecoord_index.push_back(0);	face6a.texturecoord_index.push_back(1);	face6a.texturecoord_index.push_back(2);
	faces.push_back(face6a);
	Face face6b;
	face6b.position_index.push_back(2);		face6b.position_index.push_back(6);		face6b.position_index.push_back(7);
	face6b.normal_index.push_back(5);		face6b.normal_index.push_back(5);		face6b.normal_index.push_back(5);
	face6b.texturecoord_index.push_back(0);	face6b.texturecoord_index.push_back(2);	face6b.texturecoord_index.push_back(3);
	faces.push_back(face6b);
	
	
	initBuffers();
}

