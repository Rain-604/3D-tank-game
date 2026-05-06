#ifndef COIN_H
#define COIN_H

#include "DrawObj.h"

class Coin : public DrawObj {
private:
    float spinAngle; // Unique variable just for the coin

public:
    Coin(Vector3f pos, Mesh* m, GLuint tex) : DrawObj(pos, m, tex), spinAngle(0.0f) {}

    void updateSpin() {
        spinAngle += 5.0f; // Adjust this to make it spin faster/slower
        if (spinAngle > 360.0f) spinAngle -= 360.0f;
    }

    void draw(Matrix4x4 viewMatrix, GLuint mvUniformLoc, GLuint posAttr, GLuint normAttr, GLuint texAttr) override {
        Matrix4x4 model = viewMatrix;
        
        // Move to position, and hover it slightly above the ground (0.5f)
        model.translate(position.x, position.y, position.z);
        
        // Spin the coin!
        model.rotate(spinAngle, 0.0f, 1.0f, 0.0f); 
        
        glUniformMatrix4fv(mvUniformLoc, 1, false, model.getPtr());
        glBindTexture(GL_TEXTURE_2D, textureID);
        mesh->Draw(posAttr, normAttr, texAttr);
    }
};

#endif