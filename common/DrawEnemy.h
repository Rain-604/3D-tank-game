#ifndef DRAWENEMY_H
#define DRAWENEMY_H

#include "DrawObj.h"

class DrawEnemy : public DrawObj {
public:
    DrawEnemy(Vector3f pos, Mesh* m, GLuint tex) : DrawObj(pos, m, tex) {}

    void draw(Matrix4x4 viewMatrix, GLuint mvUniformLoc, GLuint posAttr, GLuint normAttr, GLuint texAttr) override {
        Matrix4x4 model = viewMatrix;
        
        model.translate(position.x, position.y, position.z);
        model.rotate(rotationY, 0.0f, 1.0f, 0.0f); 
        
        glUniformMatrix4fv(mvUniformLoc, 1, false, model.getPtr());

        glBindTexture(GL_TEXTURE_2D, textureID);
        mesh->Draw(posAttr, normAttr, texAttr);
    }
};

#endif