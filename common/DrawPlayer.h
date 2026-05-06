#ifndef DRAWPLAYER_H
#define DRAWPLAYER_H

#include "DrawObj.h"

class DrawPlayer : public DrawObj {
public:
    int HP = 100; // Player health points

private:
    Mesh* turretMesh;
    Mesh* frontWheelMesh;
    Mesh* backWheelMesh;
    float turretRotation; // The independent angle of the tank gun
    float wheelSpinAngle;
    

public:

    // Update the constructor to accept the new meshes
    DrawPlayer(Vector3f pos, Mesh* bodyM, Mesh* turretM, Mesh* fWheelM, Mesh* bWheelM, GLuint tex) 
        : DrawObj(pos, bodyM, tex), turretMesh(turretM), frontWheelMesh(fWheelM), backWheelMesh(bWheelM), turretRotation(0.0f), wheelSpinAngle(0.0f) {}

    // Add a way to spin the turret
    void rotateTurret(float angle) { turretRotation += angle; }
    float getTurretRotation() const { return turretRotation; }
    void spinWheels(float amount) { wheelSpinAngle += amount; }

    void draw(Matrix4x4 viewMatrix, GLuint mvUniformLoc, GLuint posAttr, GLuint normAttr, GLuint texAttr) override {
        const Vector3f frontWheelPivot(-0.1172f, 0.9650f, 2.0932f);
        const Vector3f backWheelPivot(-0.1172f, 1.0167f, -1.2622f);

        // --- 1. DRAW THE MAIN CHASSIS ---
        Matrix4x4 chassisModel = viewMatrix;
        // Lift the tank out of the ground
        chassisModel.translate(position.x, position.y + 0.5f, position.z);
        
        // Fix: Changed 5.0f to 1.0f
        chassisModel.rotate(rotationY, 0.0f, 1.0f, 0.0f); 
        
        // ---> FIX: SHRINK IT BEFORE YOU DRAW IT! <---
        chassisModel.scale(0.75f, 0.75f, 0.75f);
        
        glUniformMatrix4fv(mvUniformLoc, 1, false, chassisModel.getPtr());
        glBindTexture(GL_TEXTURE_2D, textureID);
        mesh->Draw(posAttr, normAttr, texAttr);

        // --- 2. DRAW THE TURRET (Relative to the chassis) ---
        // Because we copy chassisModel AFTER scaling it, the turret is automatically 0.25 scale too!
        Matrix4x4 turretModel = chassisModel; 
        
        // Move the turret slightly UP so it sits on top of the tank body
        // NOTE: Because the whole tank is shrunk by 0.75, translating up by 1.0 only moves it 0.75 world units! 
        turretModel.translate(0.0f, 0.7f, 0.0f); 
        
        // Rotate the turret independently
        turretModel.rotate(turretRotation, 0.0f, 1.0f, 0.0f); // FIXED axis
        
        turretModel.scale(0.75f, 0.75f, 0.75f); // Make sure the turret is the same size as the chassis
        glUniformMatrix4fv(mvUniformLoc, 1, false, turretModel.getPtr());
        turretMesh->Draw(posAttr, normAttr, texAttr);

        // --- 3. DRAW THE WHEELS ---
        // Because Matrix4x4 appends transforms on the right, we translate the wheel
        // into place first and then apply the spin around the wheel mesh centroid.
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