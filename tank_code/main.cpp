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
#include <queue>
#include <algorithm>
#include "map.h"
#include "drawObj.h"
#include "DrawPlayer.h"
#include "DrawEnemy.h" // ADDED ENEMY HEADER
//#include "DrawEnemy.h"
#include "drawBullet.h"
#include "Coins.h" // ADDED COIN HEADER

// --- Game State ---
enum GameState { MENU, PLAYING };
GameState gameState = MENU;

// ... scroll down to globals ...
Map levelMap; 
std::vector<DrawObj*> gameObjects;
std::vector<Coin*> coins;
std::vector<bullet*> bullets;
std::vector<DrawEnemy*> enemies;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//!Function Prototypes
// --- Add these near your normal keyStates array ---
bool keyStates[256] = {false};
bool specialKeyStates[256] = {false}; // NEW: Tracks the arrow keys!

// --- Add these to your Function Prototypes ---
void specialKeyboard(int key, int x, int y);
void specialKeyUp(int key, int x, int y);
bool initGL(int argc, char** argv);
void display(void);
void keyboard(unsigned char key, int x, int y);
void keyUp(unsigned char key, int x, int y);
void handleKeys();
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void Timer(int value);
void initShader();
void drawTextOnScreen(int x, int y, const std::string& text) ;
void startGame(int level); // NEW: function to start a level

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
GLuint ballTexture;


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
Mesh frontWheelMesh;
Mesh backWheelMesh;
Mesh coinsMesh;Mesh ball;                         

// --- Pointers ---
DrawPlayer* myPlayer;
//DrawEnemy* myEnemy; // Use the new DrawEnemy class!

//! Screen size
int screenWidth = 720;
int screenHeight = 720;


// game physics  constants
int playerLastShootTime = 0;
int enemyLastShootTime = 0;
int shootCooldown = 500; // 500 milliseconds cooldown
float playerSpeed = 0.1f;
float enemySpeed = 0.025f; // Adjust this to make the enemy faster/slower
float rotationSpeed = 2.0f; // Adjust this to make the player turn faster/slower
float gravity = 0.1f; // Adjust this to make the player fall faster/slower
int score = 0; // Player's score based on collected coins


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
            score += 1; // Increase score by 1 for each collected coin

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
        pos.y -= gravity; // Make the tank fall into the abyss!
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
    // If makeRed is true, loop through the image and swap  and Green!
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

void startGame(int level) {
    // 1. Load the new map structure
    levelMap.loadLevel(level);

    // 2. Clear old coins and spawn new ones based on the new map
    for (Coin* c : coins) delete c;
    coins.clear();
    initCoins(coins, &coinsMesh, coinTexture);

    // 3. Clear old bullets
    for (bullet* b : bullets) delete b;
    bullets.clear();

    // 4. Reset Player and Enemy Positions
    myPlayer->setPosition(Vector3f(2.0f, 0.0f, 2.0f));
    myPlayer->setRotation(0.0f);

    // 4.5. Clear old enemies and spawn new ones
    for (DrawEnemy* e : enemies) {
        // Remove from gameObjects
        auto it = std::find(gameObjects.begin(), gameObjects.end(), e);
        if (it != gameObjects.end()) {
            gameObjects.erase(it);
        }
        delete e;
    }
    enemies.clear();

    // Define corner positions for enemies
    std::vector<Vector3f> enemyPositions = {
        Vector3f(76.0f, 0.0f, 76.0f),  // top-right
        Vector3f(2.0f, 0.0f, 76.0f),   // top-left  
        Vector3f(76.0f, 0.0f, 2.0f),   // bottom-right
        Vector3f(2.0f, 0.0f, 2.0f)     // bottom-left
    };

    for(int i = 0; i < level && i < 4; i++) {
        DrawEnemy* newEnemy = new DrawEnemy(enemyPositions[i], &playerBodyMesh, &turretMesh, &frontWheelMesh, &backWheelMesh, enemyTexture);
        newEnemy->setRotation(0.0f);
        enemies.push_back(newEnemy);
        gameObjects.push_back(newEnemy); // Add the new enemy to the game objects list
    }

    // 5. Reset score and state
    score = 0;
    gameState = PLAYING;
}

//! Main Program Entry
int main(int argc, char** argv)
{
    std::srand(static_cast<unsigned int>(std::time(0)));

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
    frontWheelMesh.loadOBJ("../models/front_wheel.obj");    // Load Front Wheel
    backWheelMesh.loadOBJ("../models/back_wheel.obj");      // Load Back Wheel
    initTexture("../models/hamvee.bmp", playerTexture, false);
    initTexture("../models/hamvee.bmp", enemyTexture, true);

    ball.loadOBJ("../models/ball.obj");
    initTexture("../models/ball.bmp", ballTexture);

    cameraManip.setPanTiltRadius(0.f, 0.5f, 15.f);
    cameraManip.setFocus(mesh.getMeshCentroid());

    levelMap = Map();

    // Give the player the green texture, and the enemy the red texture
    myPlayer = new DrawPlayer(Vector3f(2.0f, 0.0f, 2.0f), &playerBodyMesh, &turretMesh, &frontWheelMesh, &backWheelMesh, playerTexture);
    //myEnemy = new DrawEnemy(Vector3f(76.0f, 0.0f, 76.0f), &playerBodyMesh, &turretMesh, &frontWheelMesh, &backWheelMesh, enemyTexture);    
    gameObjects.push_back(myPlayer);
    //gameObjects.push_back(myEnemy);

    // Initial state is MENU, so we don't init coins yet
    
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
    glutSpecialFunc(specialKeyboard);     // NEW: Listen for arrow keys being pressed
    glutSpecialUpFunc(specialKeyUp);
    glutPassiveMotionFunc(motion);
    glutMotionFunc(motion);
    glutTimerFunc(100, Timer, 0);

    return true;
}

//! Display Loop
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (gameState == MENU) {
        // --- DRAW MAIN MENU ---
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 200, "TANK ASSIGNMENT");
        drawTextOnScreen(screenWidth / 2 - 120, screenHeight - 250, "Select a Level (Press 1-4):");
        
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 300, "1. Preset Level 1 (Holes)");
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 330, "2. Preset Level 2 (City Grid)");
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 360, "3. Preset Level 3 (Spiral)");
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 390, "4. Random Maze Generator");

        drawTextOnScreen(screenWidth / 2 - 120, screenHeight - 450, "Press ESC to Quit");
    } else if (gameState == PLAYING) {
        // --- DRAW THE GAME ---
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

        for (size_t i = 0; i < bullets.size(); i++) {
            bullets[i]->draw(m, MVMatrixUniformLocation,
            vertexPositionAttribute,
            vertexNormalAttribute,
            vertexTexcoordAttribute);
        }

        // --- DRAW TEXT OVERLAY ---
        // Convert your score integer to a string
        std::string scoreText = "Score: " + std::to_string(score);
        
        // Draw it at X: 20, Y: 680 (near the top-left of your 720x720 window)
        drawTextOnScreen(20, screenHeight - 40, scoreText);
        
        // Draw some instructions at the bottom
        drawTextOnScreen(20, 20, "WASD to Move | Q/E to Turn Turret | C to Shoot | M for Menu");

        glUseProgram(0);
    }
    
    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
    if (key == 27) exit(0); // ESC to quit
    
    if (gameState == MENU) {
        if (key == '1') startGame(1);
        if (key == '2') startGame(2);
        if (key == '3') startGame(3);
        if (key == '4') startGame(4);
    } else if (gameState == PLAYING) {
        if (key == 'm' || key == 'M') {
            gameState = MENU; // Press M to return to menu
        }
        keyStates[key] = true;
    }
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

    // 4. Rotate the Turret independently (Left and Right Arrows)
    if(specialKeyStates[GLUT_KEY_LEFT]) {
        myPlayer->rotateTurret(2.0f);
    }
    if(specialKeyStates[GLUT_KEY_RIGHT]) {
        myPlayer->rotateTurret(-2.0f);
    }

    // 5. Shoot (Up Arrow)
    if(specialKeyStates[GLUT_KEY_UP]){
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        if (currentTime - playerLastShootTime >= shootCooldown) {
            // Calculate bullet trajectory based on turret rotation AND tank rotation
            float totalRot = currentRot + myPlayer->getTurretRotation();
            float turretRad = totalRot * (M_PI / 180.0f);

            // Spawn Offset
            float forwardOffset = 2.0f; 
            float heightOffset = 1.0f;  

            float spawnX = currentPos.x + (sin(turretRad) * forwardOffset);
            float spawnY = currentPos.y + heightOffset;
            float spawnZ = currentPos.z + (cos(turretRad) * forwardOffset);

            bullet* b = new bullet(
                Vector3f(spawnX, spawnY, spawnZ),
                &ball,
                ballTexture
            );
            b->setrad(turretRad);
            bullets.push_back(b);
            playerLastShootTime = currentTime;
        }
    }

    // Give the updated position and rotation back to the player
    myPlayer->setPosition(currentPos);
    myPlayer->setRotation(currentRot);
}

void mouse(int button, int state, int x, int y)
{
    if (gameState == PLAYING) {
        cameraManip.handleMouse(button, state, x, y);
    }
}

void motion(int x, int y)
{
    if (gameState == PLAYING) {
        cameraManip.handleMouseMotion(x, y);
    }
}

struct Point2D {
    int x, z;
    bool operator==(const Point2D& other) const { return x == other.x && z == other.z; }
    bool operator!=(const Point2D& other) const { return x != other.x || z != other.z; }
};

Point2D getNextStepBFS(Point2D start, Point2D goal) {
    if (start == goal) return start;

    int parentX[40][40];
    int parentZ[40][40];
    for(int i=0; i<40; i++) {
        for(int j=0; j<40; j++) {
            parentX[i][j] = -1;
            parentZ[i][j] = -1;
        }
    }

    std::queue<Point2D> q;
    q.push(start);
    parentX[start.x][start.z] = start.x;
    parentZ[start.x][start.z] = start.z;

    int dirs[4][2] = {{0,1}, {1,0}, {0,-1}, {-1,0}};

    bool found = false;
    while (!q.empty()) {
        Point2D curr = q.front();
        q.pop();

        if (curr == goal) {
            found = true;
            break;
        }

        for (int i = 0; i < 4; i++) {
            int nx = curr.x + dirs[i][0];
            int nz = curr.z + dirs[i][1];

            if (nx >= 0 && nx < 40 && nz >= 0 && nz < 40) {
                if (levelMap.grid[nx][nz] == 1 && parentX[nx][nz] == -1) {
                    parentX[nx][nz] = curr.x;
                    parentZ[nx][nz] = curr.z;
                    q.push({nx, nz});
                }
            }
        }
    }

    if (!found) return start; // No path found

    // Backtrack to find the first step
    Point2D step = goal;
    while (parentX[step.x][step.z] != start.x || parentZ[step.x][step.z] != start.z) {
        int px = parentX[step.x][step.z];
        int pz = parentZ[step.x][step.z];
        step = {px, pz};
    }

    return step;
}

void Timer(int value)
{
    // 1. Move the Player based on WASD keys & check for shooting
    handleKeys();

    // 2. Apply Gravity
    applyGravity();

    // 3. UPDATE BULLETS & DELETE OUT-OF-BOUNDS BULLETS
    for (size_t i = 0; i < bullets.size(); i++) {
        bullets[i]->update(); // Move the bullet forward

        // If the bullet flies past 100 units, destroy it to save memory!
        if (abs(bullets[i]->getPosition().x) > 100.0f || abs(bullets[i]->getPosition().z) > 100.0f) {
            delete bullets[i];
            bullets.erase(bullets.begin() + i);
            i--; // Adjust index after erasing
        }
    }

    // 4. Check for Coin Collisions
    Vector3f playerPos = myPlayer->getPosition();
    collectCoins(playerPos.x, playerPos.y, playerPos.z, coins);

    // 5. Enemy AI Tracking
    Vector3f target = myPlayer->getPosition();
    for(size_t i = 0; i < enemies.size(); i++) {
        DrawEnemy* myEnemy = enemies[i];
        Vector3f enemyPos = myEnemy->getPosition();
    
    float dx = target.x - enemyPos.x;
    float dz = target.z - enemyPos.z;
    float dist = sqrt(dx*dx + dz*dz);
    Vector3f currentPosEny = myEnemy->getPosition();
    float currentRotEny = myEnemy->getRotation(); // Get current rotation in Degrees

    // Grid coordinates
    int eGridX = round(enemyPos.x / 2.0f);
    int eGridZ = round(enemyPos.z / 2.0f);
    int pGridX = round(target.x / 2.0f);
    int pGridZ = round(target.z / 2.0f);

    // Keep enemy in bounds
    eGridX = std::max(0, std::min(39, eGridX));
    eGridZ = std::max(0, std::min(39, eGridZ));
    pGridX = std::max(0, std::min(39, pGridX));
    pGridZ = std::max(0, std::min(39, pGridZ));

    if (dist > 10.0f) {
        Point2D nextStep = getNextStepBFS({eGridX, eGridZ}, {pGridX, pGridZ});
        Vector3f targetPos = {nextStep.x * 2.0f, enemyPos.y, nextStep.z * 2.0f};
        
        float stepDx = targetPos.x - enemyPos.x;
        float stepDz = targetPos.z - enemyPos.z;
        float stepDist = sqrt(stepDx*stepDx + stepDz*stepDz);
        
        float speed = 0.05f; 
        if (stepDist > 0.1f) {
            enemyPos.x += (stepDx / stepDist) * speed;
            enemyPos.z += (stepDz / stepDist) * speed;
            
            float angle = atan2(stepDx, stepDz) * (180.0f / M_PI);
            myEnemy->setRotation(angle);
        }
    } else {
        // --- SMOOTH TURRET AIMING ---
        // 1. Calculate the absolute world angle to the player
        float targetWorldAngle = atan2(dx, dz) * (180.0f / M_PI);
        
        // 2. Figure out the turret's target angle RELATIVE to the enemy's chassis
        float targetTurretAngle = targetWorldAngle - currentRotEny;
        
        // 3. Get the turret's CURRENT relative angle
        float currentTurretAngle = myEnemy->getTurretRotation();
        
        // 4. Calculate the difference between where it is pointing and where it wants to point
        float diff = targetTurretAngle - currentTurretAngle;
        
        // 5. Normalize the difference to ensure it takes the shortest path (-180 to 180 degrees)
        while (diff > 180.0f) diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;
        
        // 6. Turn the turret smoothly (2 degrees per frame) towards the target!
        float turnSpeed = 2.0f;
        if (diff > turnSpeed) {
            myEnemy->rotateTurret(turnSpeed);
        } else if (diff < -turnSpeed) {
            myEnemy->rotateTurret(-turnSpeed);
        } else {
            // If it's very close to perfectly aimed, just snap exactly to the difference so it doesn't jitter
            myEnemy->rotateTurret(diff);
            
            // --- ENEMY SHOOTING ---
            // Only shoot if the turret is actually aimed at the player!
            int currentTime = glutGet(GLUT_ELAPSED_TIME);
            if (currentTime - enemyLastShootTime >= shootCooldown) {
                float turretRad = targetWorldAngle * (M_PI / 180.0f);

                float forwardOffset = 2.0f; 
                float heightOffset = 1.0f;  

                float spawnX = currentPosEny.x + (sin(turretRad) * forwardOffset);
                float spawnY = currentPosEny.y + heightOffset;
                float spawnZ = currentPosEny.z + (cos(turretRad) * forwardOffset);

                bullet* b = new bullet(
                    Vector3f(spawnX, spawnY, spawnZ),
                    &ball,
                    ballTexture
                );
                b->setrad(turretRad); // Make sure the bullet travels along the absolute world angle
                bullets.push_back(b);
                enemyLastShootTime = currentTime;
            }
        }
    }
    
    myEnemy->setPosition(enemyPos);
    }

    // 6. Trigger a redraw and loop
    glutPostRedisplay();
    glutTimerFunc(10, Timer, 0);
}
void drawTextOnScreen(int x, int y, const std::string& text) 
{
    // 1. Temporarily turn off shaders so we can use standard GLUT colors
    glUseProgram(0);

    // 2. Disable depth testing so the text always draws ON TOP of the 3D world
    glDisable(GL_DEPTH_TEST);

    // 3. Set the text color (e.g., White)
    glColor3f(1.0f, 1.0f, 1.0f);

    // 4. Set the position in pixel coordinates (x=0, y=0 is the bottom-left corner)
    glWindowPos2i(x, y);

    // 5. Loop through the string and draw each character
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // 6. Turn depth testing back on for the next 3D frame!
    glEnable(GL_DEPTH_TEST);
}

void specialKeyboard(int key, int x, int y)
{
    // GLUT passes in macros like GLUT_KEY_LEFT, which are safely between 0 and 255
    if (key >= 0 && key < 256 && gameState == PLAYING) {
        specialKeyStates[key] = true;
    }
}

void specialKeyUp(int key, int x, int y)
{
    if (key >= 0 && key < 256) {
        specialKeyStates[key] = false;
    }
}
