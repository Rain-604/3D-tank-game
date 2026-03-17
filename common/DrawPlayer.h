#ifndef DRAWPLAYER_H
#define DRAWPLAYER_H

#include "DrawObj.h"

class DrawPlayer : public DrawObj {
private:
    Mesh* turretMesh;
    Mesh* wheelMesh;
    float turretRotation; // The independent angle of the tank gun

public:
    // Update the constructor to accept the new meshes
    DrawPlayer(Vector3f pos, Mesh* bodyM, Mesh* turretM, Mesh* wheelM, GLuint tex) 
        : DrawObj(pos, bodyM, tex), turretMesh(turretM), wheelMesh(wheelM), turretRotation(0.0f) {}

    // Add a way to spin the turret
    void rotateTurret(float angle) { turretRotation += angle; }

    void draw(Matrix4x4 viewMatrix, GLuint mvUniformLoc, GLuint posAttr, GLuint normAttr, GLuint texAttr) override {
        // --- 1. DRAW THE MAIN CHASSIS ---
        Matrix4x4 chassisModel = viewMatrix;
        chassisModel.translate(position.x, position.y, position.z);
        
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
        // NOTE: Because the whole tank is shrunk by 0.25, translating up by 1.0 only moves it 0.25 world units! 
        // You might need to change this to 2.0f or 3.0f to make it sit flush on the chassis.
        turretModel.translate(0.0f, 1.0f, 0.0f); 
        
        // Rotate the turret independently
        turretModel.rotate(turretRotation, 0.0f, 0.75f, 0.0f);
        
        turretModel.scale(0.75f, 0.75f, 0.75f); // Make sure the turret is the same size as the chassis
        glUniformMatrix4fv(mvUniformLoc, 1, false, turretModel.getPtr());
        turretMesh->Draw(posAttr, normAttr, texAttr);

        // --- 3. DRAW THE WHEELS ---
    }
};

#endif