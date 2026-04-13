#ifndef drawBullet_H
#define drawBullet_H

#include "drawObj.h"

class bullet : public DrawObj {
private:
    float spinAngle; // Unique variable just for the coin
    float speed = 0.5f; // Decreased speed
    float rad;

public:
    bullet(Vector3f pos, Mesh* m, GLuint tex) : DrawObj(pos, m, tex), spinAngle(0.0f), rad(0.0f) {}

    void update() {
        spinAngle += 5.0f; // Adjust this to make it spin faster/slower
        position.x += speed * sin(rad); 
        position.z += speed * cos(rad);
        if (spinAngle > 360.0f) spinAngle -= 360.0f;
    }
    void setrad(float rads){
        rad = rads ; 
    }

    void draw(Matrix4x4 viewMatrix, GLuint mvUniformLoc, GLuint posAttr, GLuint normAttr, GLuint texAttr) override {
        Matrix4x4 model = viewMatrix;
        
        // Move to position, and hover it slightly above the ground (0.5f)

        model.translate(position.x, position.y + 0.5f, position.z);
        
        // Spin the coin!
        model.rotate(spinAngle, 0.0f, 1.0f, 0.0f);
        
        model.scale(0.25f, 0.25f, 0.25f); // Scale down the bullet

        glUniformMatrix4fv(mvUniformLoc, 1, false, model.getPtr());
        glBindTexture(GL_TEXTURE_2D, textureID);
        mesh->Draw(posAttr, normAttr, texAttr);
    }
};

#endif