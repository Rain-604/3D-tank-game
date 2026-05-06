#ifndef drawBullet_H
#define drawBullet_H

#include "drawObj.h"

class bullet : public DrawObj {
public:
    static constexpr float PROJECTILE_SPEED = 30.0f;
    static constexpr float PROJECTILE_GRAVITY = 9.81f;
    static constexpr float PROJECTILE_START_VERTICAL_VELOCITY = 0.0f;

private:
    float spinAngle; // Unique variable just for the coin
    float speed = PROJECTILE_SPEED;
    float rad;
    float verticalVelocity;
    float gravity = PROJECTILE_GRAVITY;
    float floorY;

public:
    bullet(Vector3f pos, Mesh* m, GLuint tex)
        : DrawObj(pos, m, tex), spinAngle(0.0f), rad(0.0f), verticalVelocity(PROJECTILE_START_VERTICAL_VELOCITY), gravity(PROJECTILE_GRAVITY), floorY(0.0f) {}

    void update(float deltaSeconds) {
        spinAngle += 5.0f; // Adjust this to make it spin faster/slower
        position.x += speed * sin(rad) * deltaSeconds;
        position.z += speed * cos(rad) * deltaSeconds;
        verticalVelocity -= gravity * deltaSeconds;
        position.y += verticalVelocity * deltaSeconds;

        if (spinAngle > 360.0f) spinAngle -= 360.0f;
    }
    void setrad(float rads){
        rad = rads ; 
    }

    void draw(Matrix4x4 viewMatrix, GLuint mvUniformLoc, GLuint posAttr, GLuint normAttr, GLuint texAttr) override {
        Matrix4x4 model = viewMatrix;

        // Draw the projectile at its simulated position.
        model.translate(position.x, position.y+0.5f, position.z);
        
        // Spin the coin!
       // model.rotate(spinAngle, 0.0f, 1.0f, 0.0f);
        
        model.scale(0.5f, 0.5f, 0.5f); // Scale down the bullet

        glUniformMatrix4fv(mvUniformLoc, 1, false, model.getPtr());
        glBindTexture(GL_TEXTURE_2D, textureID);
        mesh->Draw(posAttr, normAttr, texAttr);
    }
};

#endif
