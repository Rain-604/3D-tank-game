//!Includes
#include <GL/glew.h>
#include <GL/glut.h>
#include <Shader.h>
#include <Vector.h>
#include <Matrix.h>
#include <Mesh.h>
#include <Texture.h>
#include <SphericalCameraManipulator.h>
#include <iostream>
#include <math.h>
#include <string>
#include "map.h"
#include "drawObj.h"
#include "DrawPlayer.h"
#include "DrawEnemy.h" // ADDED ENEMY HEADER
//#include "DrawEnemy.h"
#include "Coins.h" // ADDED COIN HEADER
// ... scroll down to globals ...
Map levelMap; 
std::vector<DrawObj*> gameObjects;
std::vector<Coin*> coins;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//!Function Prototypes
bool initGL(int argc, char** argv);
void display(void);
void keyboard(unsigned char key, int x, int y);
void keyUp(unsigned char key, int x, int y);
void handleKeys();
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void Timer(int value);
void initShader();

//Global Variables
GLuint vertexTexcoordAttribute;
GLuint shaderProgramID;                             
GLuint vertexPositionAttribute;                     
GLuint vertexNormalAttribute;                       
GLuint LightPositionUniformLocation;                
Vector3f colour;                                    
GLuint ColourUniformLocation;                       

GLuint AmbientUniformLocation;
GLuint SpecularUniformLocation;
GLuint SpecularPowerUniformLocation;

//Material Properties
Vector3f lightPosition = Vector3f(20.0, 20.0, 20.0);   
Vector3f ambient    = Vector3f(0.1, 0.1, 0.1);
Vector3f specular   = Vector3f(1.0, 1.0, 1.0); // Made this white so the whole level isn't green!
float specularPower = 10.0;



GLuint TextureMapUniformLocation;                   
GLuint texture;        
GLuint playerTexture;        
GLuint enemyTexture;
GLuint coinTexture;


//Viewing
SphericalCameraManipulator cameraManip;
Matrix4x4 ModelViewMatrix;                          
GLuint MVMatrixUniformLocation;                     
Matrix4x4 ProjectionMatrix;                         
GLuint ProjectionUniformLocation;                   

//Mesh
Mesh mesh;                                          
Mesh playerBodyMesh;   
Mesh turretMesh;
Mesh wheelMesh;    
Mesh coinsMesh;                         

// --- Pointers ---
DrawPlayer* myPlayer;
DrawEnemy* myEnemy; // Use the new DrawEnemy class!

//! Screen size
int screenWidth = 720;
int screenHeight = 720;

//! Array of key states
bool keyStates[256];

// game physics constants
float playerSpeed = 0.1f;
float enemySpeed = 0.025f; // Adjust this to make the enemy faster/slower
float rotationSpeed = 2.0f; // Adjust this to make the player turn faster/slower
float gravaity = 0.1f; // Adjust this to make the player fall faster/slower


void collectCoins(float playerX, float playerY, float playerZ, std::vector<Coin*> &coinList)
{
    float radius = 1.5f;

    for(size_t i = 0; i < coinList.size(); i++)
    {
        Vector3f coinPos = coinList[i]->getPosition();

        float dx = playerX - coinPos.x;
        float dz = playerZ - coinPos.z;

        float dist = sqrt(dx*dx + dz*dz);

        if(dist < radius)
        {
            std::cout << "Collected coin!" << std::endl;

            delete coinList[i];
            coinList.erase(coinList.begin() + i); // restart the list
            i--;
        }
    }
}

void applyGravity()
{
    Vector3f pos = myPlayer->getPosition();

    // Convert the player's 3D world X and Z coordinates into Grid coordinates
    // We divide by 2.0f because the cubes are spaced by 2 units
    int gridX = round(pos.x / 2.0f);
    int gridZ = round(pos.z / 2.0f);

    bool onBlock = false;

    // First, check if the tank is still inside the 40x40 map area
    if (gridX >= 0 && gridX < 40 && gridZ >= 0 && gridZ < 40) {
        // If there is a 1 underneath us, we are safe!
        if (levelMap.grid[gridX][gridZ] == 1) {
            onBlock = true;
        }
    }

    // Apply the physics!
    if (onBlock) {
        pos.y = 0.0f; // Keep the tank safely on top of the block
    } else {
        pos.y -= gravaity; // Make the tank fall into the abyss!
    }

    // Save the new position
    myPlayer->setPosition(pos);
}

void initTexture(std::string filename, GLuint & textureID, bool makeRed = false)
{
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 

    int width, height;
    char* data;
    Texture::LoadBMP(filename, width, height, data);

    // --- THE PIXEL HACK ---
    // If makeRed is true, loop through the image and swap Red and Green!
    if (makeRed) {
        int totalBytes = width * height * 3; // 3 bytes per pixel (R, G, B)
        for (int i = 0; i < totalBytes; i += 3) {
            char temp = data[i];       // Save Red byte
            data[i] = data[i+1];       // Move Green into Red
            data[i+1] = temp;          // Move Red into Green
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    delete[] data;
}


void initCoins(std::vector<Coin*> &coinList, Mesh* coinMesh, GLuint coinTex) {
    for(int i = 0; i < 40; i++) {
        for(int j = 0; j < 40; j++) {
            // Spawn a coin on empty spaces (0), but only on every 3rd tile to space them out
            if(levelMap.grid[i][j] == 1 && (i + j) % 3 == 0) { 
                coinList.push_back(new Coin(Vector3f(i * 2.0f, 2.0f, j * 2.0f), coinMesh, coinTex));
            }
        }
    }
}
//! Main Program Entry
int main(int argc, char** argv)
{
    if(!initGL(argc, argv)) return -1;
    
    initShader();
    
    mesh.loadOBJ("../models/cube.obj");    
    initTexture("../models/Crate.bmp", texture);

    coinsMesh.loadOBJ("../models/coin.obj");
    initTexture("../models/coin.bmp", coinTexture);
    
    // Load Player Assets once
    // Load Player Assets (Standard Green)
    playerBodyMesh.loadOBJ("../models/chassis.obj");
    turretMesh.loadOBJ("../models/turret.obj");             // Load Turret
    wheelMesh.loadOBJ("../models/front_wheel.obj");         // Load Wheel
    initTexture("../models/hamvee.bmp", playerTexture, false);
    initTexture("../models/hamvee.bmp", enemyTexture, true);

    cameraManip.setPanTiltRadius(0.f, 0.5f, 15.f); 
    cameraManip.setFocus(mesh.getMeshCentroid());

    levelMap = Map(); 
    initCoins(coins, &coinsMesh, coinTexture); // Initialize coins based on the map

    // Give the player the green texture, and the enemy the red texture
    myPlayer = new DrawPlayer(Vector3f(2.0f, 0.0f, 2.0f), &playerBodyMesh, &turretMesh, &wheelMesh, playerTexture);
    myEnemy = new DrawEnemy(Vector3f(10.0f, 0.0f, 10.0f), &playerBodyMesh, enemyTexture);
    
    gameObjects.push_back(myPlayer);
    gameObjects.push_back(myEnemy);
    
    glutMainLoop();
    
    glDeleteProgram(shaderProgramID);
    return 0;
}

void initShader()
{
    shaderProgramID = Shader::LoadFromFile("shader.vert","shader.frag");
    
    vertexPositionAttribute = glGetAttribLocation(shaderProgramID,  "aVertexPosition");
    vertexNormalAttribute = glGetAttribLocation(shaderProgramID,    "aVertexNormal");
    vertexTexcoordAttribute = glGetAttribLocation(shaderProgramID, "aVertexTexcoord");

    MVMatrixUniformLocation         = glGetUniformLocation(shaderProgramID, "MVMatrix_uniform"); 
    ProjectionUniformLocation       = glGetUniformLocation(shaderProgramID, "ProjMatrix_uniform"); 
    LightPositionUniformLocation    = glGetUniformLocation(shaderProgramID, "LightPosition_uniform"); 
    AmbientUniformLocation          = glGetUniformLocation(shaderProgramID, "Ambient_uniform"); 
    SpecularUniformLocation         = glGetUniformLocation(shaderProgramID, "Specular_uniform"); 
    SpecularPowerUniformLocation    = glGetUniformLocation(shaderProgramID, "SpecularPower_uniform");
    TextureMapUniformLocation       = glGetUniformLocation(shaderProgramID, "TextureMap_uniform"); 
}

bool initGL(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(screenWidth, screenHeight);
    glutInitWindowPosition(200, 200);
    glutCreateWindow("Tank Assignment");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    
    if (glewInit() != GLEW_OK) 
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyUp); 
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(motion);
    glutMotionFunc(motion);
    glutTimerFunc(100, Timer, 0);

    return true;
}

//! Display Loop
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    
    ProjectionMatrix.perspective(90, 1.0, 0.0001, 100.0);
    glUniformMatrix4fv(ProjectionUniformLocation, 1, false, ProjectionMatrix.getPtr());

    cameraManip.setFocus(myPlayer->getPosition());
    ModelViewMatrix.toIdentity();
    Matrix4x4 m = cameraManip.apply(ModelViewMatrix);
    
    glUniform3f(LightPositionUniformLocation, lightPosition.x, lightPosition.y, lightPosition.z);
    glUniform4f(AmbientUniformLocation, ambient.x, ambient.y, ambient.z, 1.0);
    glUniform4f(SpecularUniformLocation, specular.x, specular.y, specular.z, 1.0);
    glUniform1f(SpecularPowerUniformLocation, specularPower);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(TextureMapUniformLocation, 0);

    for(size_t i = 0; i < levelMap.cubePositions.size(); i++)
    {
        Matrix4x4 model = m;
        model.translate(levelMap.cubePositions[i].x,
                        levelMap.cubePositions[i].y,
                        levelMap.cubePositions[i].z); 
        glUniformMatrix4fv(MVMatrixUniformLocation, 1, false, model.getPtr());
        mesh.Draw(vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
    }

    for (size_t i = 0; i < coins.size(); i++) {
        coins[i]->updateSpin(); 
        // Tell the coin to draw itself!
        coins[i]->draw(m, MVMatrixUniformLocation, vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
    }
    for (size_t i = 0; i < gameObjects.size(); i++) {
        gameObjects[i]->draw(m, MVMatrixUniformLocation, vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
    }

    /*Vector3f playerPos = myPlayer->getPosition();
    collectCoins(playerPos.x, playerPos.y, playerPos.z, coins);

    Vector3f target = myPlayer->getPosition();
    Vector3f EnemyPos = myEnemy->getPosition();
    Vector3f displacement = target-EnemyPos;
    float speed1 = 0.025f;
    EnemyPos.x += speed1*displacement.x ;
    EnemyPos.z += speed1*displacement.z ;
    myEnemy->setPosition(EnemyPos);*/

    applyGravity();




    
    
    glUseProgram(0);
    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
    if (key == 27) exit(0);
    keyStates[key] = true;
}

void keyUp(unsigned char key, int x, int y)
{
    keyStates[key] = false;
}


void handleKeys()
{
    Vector3f currentPos = myPlayer->getPosition();
    float currentRot = myPlayer->getRotation(); // Get current rotation in Degrees
    float speed = 0.1f; 

    // 1. Steer left and right (Degrees)
    if(keyStates['a']) {
        currentRot += 2.0f; // Increased slightly so it turns a bit faster!
    }
    if(keyStates['d']) {
        currentRot -= 2.0f; 
    }

    // 2. CONVERT DEGREES TO RADIANS for the math functions!
    float rad = currentRot * (M_PI / 180.0f);

    // 3. Drive forward and backward using the Radians
    if(keyStates['w']) {
        currentPos.x += speed * sin(rad); 
        currentPos.z += speed * cos(rad);
    }
    if(keyStates['s']) {
        currentPos.x -= speed * sin(rad); 
        currentPos.z -= speed * cos(rad);
    }

    // Give the updated position and rotation back to the player
    myPlayer->setPosition(currentPos);
    myPlayer->setRotation(currentRot);
}
void mouse(int button, int state, int x, int y)
{
    cameraManip.handleMouse(button, state, x, y);
}

void motion(int x, int y)
{
    cameraManip.handleMouseMotion(x, y);
}

void Timer(int value)
{
    // 1. Move the Player based on WASD keys
    handleKeys();

    // 2. APPLY GRAVITY AND FALLING LOGIC!
    applyGravity();

    // 3. Check for Coin Collisions
    Vector3f playerPos = myPlayer->getPosition();
    collectCoins(playerPos.x, playerPos.y, playerPos.z, coins);

    // 4. Enemy AI Tracking
   /*/ Vector3f target = myPlayer->getPosition();
    Vector3f enemyPos = myEnemy->getPosition();
    
    float dx = target.x - enemyPos.x;
    float dz = target.z - enemyPos.z;
    float dist = sqrt(dx*dx + dz*dz);

    if (dist > 0.1f) {
        float speed = 0.05f; 
        enemyPos.x += (dx / dist) * speed;
        enemyPos.z += (dz / dist) * speed;
        
        float angle = atan2(dx, dz) * (180.0f / M_PI);
        myEnemy->setRotation(angle);
    }
    
    myEnemy->setPosition(enemyPos);*/

    // 5. Trigger a redraw and loop
    glutPostRedisplay();
    glutTimerFunc(10, Timer, 0);
}
