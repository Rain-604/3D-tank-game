#ifndef DRAWENEMY_H
#define DRAWENEMY_H

#include "DrawObj.h"

class DrawEnemy : public DrawObj {
public:
    int HP =200; // Enemy health points
private:
    Mesh* turretMesh;
    Mesh* frontWheelMesh;
    Mesh* backWheelMesh;
    float turretRotation; // The independent angle of the tank gun
    float wheelSpinAngle;

public:
    DrawEnemy(Vector3f pos, Mesh* bodyM, Mesh* turretM, Mesh* fWheelM, Mesh* bWheelM, GLuint tex) 
        : DrawObj(pos, bodyM, tex), turretMesh(turretM), frontWheelMesh(fWheelM), backWheelMesh(bWheelM), turretRotation(0.0f), wheelSpinAngle(0.0f) {}

    // Add a way to spin the turret
    void rotateTurret(float angle) { turretRotation += angle; }
    void setTurretRotation(float angle) { turretRotation = angle; }
    float getTurretRotation() const { return turretRotation; }
    void spinWheels(float amount) { wheelSpinAngle += amount; }

    void draw(Matrix4x4 viewMatrix, GLuint mvUniformLoc, GLuint posAttr, GLuint normAttr, GLuint texAttr) override {
        const Vector3f frontWheelPivot(-0.1172f, 0.9650f, 2.0932f);
        const Vector3f backWheelPivot(-0.1172f, 1.0167f, -1.2622f);

        // --- 1. DRAW THE MAIN CHASSIS ---
        Matrix4x4 chassisModel = viewMatrix;
        
        // Lift up
        chassisModel.translate(position.x, position.y + 0.5f, position.z);
        chassisModel.rotate(rotationY, 0.0f, 1.0f, 0.0f); 
        
        // Make the enemy the same size as the player (0.75f instead of 0.5f)
        chassisModel.scale(0.75f, 0.75f, 0.75f);

        glUniformMatrix4fv(mvUniformLoc, 1, false, chassisModel.getPtr());

        glBindTexture(GL_TEXTURE_2D, textureID);
        mesh->Draw(posAttr, normAttr, texAttr);

        // --- 2. DRAW THE TURRET ---
        if (turretMesh) {
            Matrix4x4 turretModel = chassisModel;
            turretModel.translate(0.0f, 0.75f, 0.0f);
            turretModel.rotate(turretRotation, 0.0f, 1.0f, 0.0f);
            turretModel.scale(0.75f, 0.75f, 0.75f);
            glUniformMatrix4fv(mvUniformLoc, 1, false, turretModel.getPtr());
            turretMesh->Draw(posAttr, normAttr, texAttr);
        }

        // --- 3. DRAW THE WHEELS ---
        if (frontWheelMesh) {
            Matrix4x4 frontWheelModel = chassisModel;
            frontWheelModel.translate(frontWheelPivot.x, frontWheelPivot.y, frontWheelPivot.z);
            frontWheelModel.rotate(wheelSpinAngle, 1.0f, 0.0f, 0.0f);
            frontWheelModel.translate(-frontWheelPivot.x, -frontWheelPivot.y, -frontWheelPivot.z);
            glUniformMatrix4fv(mvUniformLoc, 1, false, frontWheelModel.getPtr());
            frontWheelMesh->Draw(posAttr, normAttr, texAttr);
        }
        if (backWheelMesh) {
            Matrix4x4 backWheelModel = chassisModel;
            backWheelModel.translate(backWheelPivot.x, backWheelPivot.y, backWheelPivot.z);
            backWheelModel.rotate(wheelSpinAngle, 1.0f, 0.0f, 0.0f);
            backWheelModel.translate(-backWheelPivot.x, -backWheelPivot.y, -backWheelPivot.z);
            glUniformMatrix4fv(mvUniformLoc, 1, false, backWheelModel.getPtr());
            backWheelMesh->Draw(posAttr, normAttr, texAttr);
        }
    }
};

#endif