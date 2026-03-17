#ifndef DRAWOBJ_H
#define DRAWOBJ_H

#include <GL/glew.h>
#include "Vector.h"
#include "Matrix.h"
#include "Mesh.h"

class DrawObj {
protected:
    Vector3f position;
    float rotationY;   // Track the angle the object is facing
    Mesh* mesh;        
    GLuint textureID;  

public:
    DrawObj(Vector3f pos, Mesh* m, GLuint tex) : position(pos), rotationY(0.0f), mesh(m), textureID(tex) {}
    
    virtual ~DrawObj() {}
    virtual void draw(Matrix4x4 viewMatrix, GLuint mvUniformLoc, GLuint posAttr, GLuint normAttr, GLuint texAttr) = 0;

    void setPosition(Vector3f pos) { position = pos; }
    Vector3f getPosition() const { return position; }

    void setRotation(float rot) { rotationY = rot; }
    float getRotation() const { return rotationY; }
};

#endif