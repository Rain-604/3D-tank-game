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
#include <ctime>
#include "map.h"
#include "drawObj.h"
#include "DrawPlayer.h"
#include "DrawEnemy.h" // ADDED ENEMY HEADER
#include <utility>
//#include "DrawEnemy.h"
#include "drawBullet.h"
#include "Coins.h" // ADDED COIN HEADER

// --- Game State ---
enum GameState { MENU, PLAYING, GAME_OVER , WIN};
GameState gameState = MENU;

enum CameraView { FIRST_PERSON, THIRD_PERSON, TOP_DOWN };
CameraView cameraView = FIRST_PERSON;


// ... scroll down to globals ...
Map levelMap; 
std::vector<DrawObj*> gameObjects;
std::vector<Coin*> coins;
std::vector<bullet*> bullets;
enum BulletOwner { PLAYER_BULLET, ENEMY_BULLET };
std::vector<BulletOwner> bulletOwners;
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
void handleKeys(float deltaSeconds);
bool collidesWithEnemyAt(const Vector3f& pos, float radius);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void Timer(int value);
void initShader();
void drawTextOnScreen(int x, int y, const std::string& text) ;
void drawSunnyBackground();
void drawWorldSun(const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix);
void drawGroundShadows(const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix);
void drawOverlayEllipse(float centerX, float centerY, float radiusX, float radiusY, float red, float green, float blue, float alpha);
Vector3f transformPoint(const Matrix4x4& matrix, const Vector3f& point);
void updateDayNightCycle(int elapsedMs);
Matrix4x4 getViewMatrix();
bool worldToScreen(const Vector3f& worldPos, const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix, int& screenX, int& screenY);
void startGame(int level); // NEW: function to start a level
void changeCamera(); // NEW: function to cycle camera view
void reshape(int width, int height);
Vector3f getProjectileSpawnPosition(const Vector3f& basePos, float turretRad);
Vector3f predictProjectileLandingPoint(const Vector3f& spawnPos, float turretRad);
void drawAimPrediction(const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix);

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
Vector3f sunLightPosition = Vector3f(60.0f, 45.0f, 20.0f);
Vector3f ambient    = Vector3f(0.1, 0.1, 0.1);
Vector3f specular   = Vector3f(1.0, 1.0, 1.0); // Made this white so the whole level isn't green!
float specularPower = 10.0;
float dayNightStrength = 1.0f;
float sunScreenX = 0.0f;
float sunScreenY = 0.0f;
Vector3f skyHorizonColor = Vector3f(0.72f, 0.90f, 1.00f);
Vector3f skyZenithColor = Vector3f(0.40f, 0.74f, 1.00f);
float thirdPersonFollowDistance = 12.0f;
float thirdPersonFollowHeight = 5.5f;
float thirdPersonLookAhead = 10.0f;



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
int screenWidth = 1440;
int screenHeight = 1400;


// game physics  constants
int playerLastShootTime = 0;
std::vector<int> enemyLastShootTimes;
int shootCooldown = 500; // 500 milliseconds cooldown
int EnemyshootCooldown = 1500; // Enemies shoot every 1.5 seconds
float playerMaxSpeed = 12.0f; // Max forward velocity (world units per second)
float playerAcceleration = 3.5f; // Acceleration rate (world units per second^2)
float playerVelocity = 0.0f;
float playerDrag = 0.90f;
float playerReverseSpeed = -6.0f; // Max reverse velocity (world units per second)
float enemySpeed = 0.025f; // Adjust this to make the enemy faster/slower
float rotationSpeed = 1.0f; // Adjust this to make the player turn faster/slower
float gravity = 18.0f; // Gravity acceleration (world units per second^2)
int score = 0; // Player's score based on collected coins
int totalCoins = 0; // Total coins in the level, used for win condition
int levelStartTimeMs = 0;
int levelTimeLimitSeconds = 90;
int remainingTimeSeconds = 90;
bool timeExpired = false;
int previousTimerMs = 0;
bool previousTimerInitialized = false;

Vector3f getProjectileSpawnPosition(const Vector3f& basePos, float turretRad)
{
    const float forwardOffset = 2.0f;
    const float heightOffset = 1.05f;

    return Vector3f(
        basePos.x + (std::sin(turretRad) * forwardOffset),
        basePos.y + heightOffset,
        basePos.z + (std::cos(turretRad) * forwardOffset)
    );
}

Vector3f predictProjectileLandingPoint(const Vector3f& spawnPos, float turretRad)
{
    const float gravityValue = bullet::PROJECTILE_GRAVITY;
    const float initialVerticalVelocity = bullet::PROJECTILE_START_VERTICAL_VELOCITY;
    const float floorY = 0.0f;
    const float speed = bullet::PROJECTILE_SPEED;

    const float verticalDistance = spawnPos.y - floorY;
    if (verticalDistance <= 0.0f) {
        return Vector3f(spawnPos.x, floorY, spawnPos.z);
    }

    const float discriminant = (initialVerticalVelocity * initialVerticalVelocity) + (2.0f * gravityValue * verticalDistance);
    if (discriminant < 0.0f) {
        return Vector3f(spawnPos.x, floorY, spawnPos.z);
    }

    const float flightTime = (initialVerticalVelocity + std::sqrt(discriminant)) / gravityValue;
    const float vx = speed * std::sin(turretRad);
    const float vz = speed * std::cos(turretRad);

    return Vector3f(
        spawnPos.x + vx * flightTime,
        floorY,
        spawnPos.z + vz * flightTime
    );
}

void drawAimPrediction(const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix)
{
    if (gameState != PLAYING || !myPlayer) {
        return;
    }

    const float turretRad = (myPlayer->getRotation() + myPlayer->getTurretRotation()) * (M_PI / 180.0f);
    const Vector3f spawnPos = getProjectileSpawnPosition(myPlayer->getPosition(), turretRad);
    const Vector3f landingPos = predictProjectileLandingPoint(spawnPos, turretRad);

    int screenX = 0;
    int screenY = 0;
    if (!worldToScreen(landingPos, viewMatrix, projMatrix, screenX, screenY)) {
        return;
    }

    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(screenWidth), 0.0, static_cast<double>(screenHeight), -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    const float markerX = static_cast<float>(screenX);
    const float markerY = static_cast<float>(screenY);
    drawOverlayEllipse(markerX, markerY, 14.0f, 14.0f, 1.0f, 0.35f, 0.15f, 0.35f);
    drawOverlayEllipse(markerX, markerY, 7.0f, 7.0f, 1.0f, 0.75f, 0.20f, 0.8f);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void removeBulletAt(size_t index)
{
    delete bullets[index];
    bullets.erase(bullets.begin() + index);
    if (index < bulletOwners.size()) {
        bulletOwners.erase(bulletOwners.begin() + index);
    }
}


void bullet_hit(){
    // Check for bullet collisions with enemies/player based on ownership.
    for (size_t i = 0; i < bullets.size(); i++) {
        bullet* b = bullets[i];
        Vector3f bulletPos = b->getPosition();
        BulletOwner owner = (i < bulletOwners.size()) ? bulletOwners[i] : PLAYER_BULLET;

        if (owner == PLAYER_BULLET) {
            for (size_t j = 0; j < enemies.size(); j++) {
                DrawEnemy* e = enemies[j];
                Vector3f enemyPos = e->getPosition();

                float dx = bulletPos.x - enemyPos.x;
                float dy = bulletPos.y - enemyPos.y;
                float dz = bulletPos.z - enemyPos.z;
                float dist = sqrt(dx*dx + dy*dy + dz*dz);

                if (dist < 1.5f) {
                    std::cout << "Enemy hit!" << std::endl;
                    e->HP -= 20;
                    if (e->HP <= 0) {
                        auto it = std::find(gameObjects.begin(), gameObjects.end(), e);
                        if (it != gameObjects.end()) {
                            gameObjects.erase(it);
                        }
                        delete e;
                        enemies.erase(enemies.begin() + j);
                        if (j < enemyLastShootTimes.size()) {
                            enemyLastShootTimes.erase(enemyLastShootTimes.begin() + j);
                        }
                    }

                    removeBulletAt(i);
                    return;
                }
            }
        } else {
            DrawPlayer* p = myPlayer;
            Vector3f playerPos = p->getPosition();

            float dx = bulletPos.x - playerPos.x;
            float dy = bulletPos.y - playerPos.y;
            float dz = bulletPos.z - playerPos.z;
            float dist = sqrt(dx*dx + dy*dy + dz*dz);

            if (dist < 1.5f) {
                std::cout << "Player hit!" << std::endl;
                p->HP -= 20;

                // Always remove the bullet first, even on lethal hits.
                removeBulletAt(i);

                if (p->HP <= 0) {
                    std::cout << "Game Over! Final Score: " << score << std::endl;
                    score = 0;
                    gameState = GAME_OVER;
                }
                return;
            }
        }
    }

    // If bullet hits the map boundary, remove it.
    for (size_t i = 0; i < bullets.size(); i++) {
        Vector3f bulletPos = bullets[i]->getPosition();
        if (bulletPos.x < 0 || bulletPos.x > 80 || bulletPos.z < 0 || bulletPos.z > 80) {
            removeBulletAt(i);
            i--;
        }
    }
}



void collectCoins(float playerX, float playerZ, std::vector<Coin*> &coinList)
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
// random posistions for enemy tanks
void generateEnemyPositions(std::vector<DrawEnemy*> &enemies, int numEnemies)
{
    for (int i = 0; i < numEnemies; i++)
    {
        bool validPosition = false;
        int attempts = 0;
        float x, z;

        while (!validPosition && attempts < 100) {
            x = static_cast<float>(rand() % 40); // Random X between 0 and 40
            z = static_cast<float>(rand() % 40); // Random Z between 0 and 40
            attempts++;

            // Check if position is walkable and not at player spawn location
            if (levelMap.grid[static_cast<int>(x)][static_cast<int>(z)] != 1 || (x < 25.0f && z < 25.0f)) {
                continue;
            }

            // Check if another enemy is too close to this position
            bool tooCloseToEnemy = false;
            for (size_t j = 0; j < enemies.size(); j++) {
                Vector3f existingEnemyPos = enemies[j]->getPosition();
                float dx = x - existingEnemyPos.x;
                float dz = z - existingEnemyPos.z;
                float dist = sqrt(dx * dx + dz * dz);
                
                if (dist < 10.0f) { // Minimum distance of 10 units between enemies
                    tooCloseToEnemy = true;
                    break;
                }
            }

            if (!tooCloseToEnemy) {
                validPosition = true;
            }
        }

        if (validPosition) {
            DrawEnemy* enemy = new DrawEnemy(Vector3f(x, 0.0f, z), &playerBodyMesh, &turretMesh, &frontWheelMesh, &backWheelMesh, enemyTexture);
            enemies.push_back(enemy);
            gameObjects.push_back(enemy);
            enemyLastShootTimes.push_back(0); // Initialize shoot time for each enemy
        }
    }
}

void applyGravity(float deltaSeconds)
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
        pos.y -= gravity * deltaSeconds; // Make the tank fall into the abyss!
    }

    // Save the new position
    myPlayer->setPosition(pos);
}

bool worldToScreen(const Vector3f& worldPos, const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix, int& screenX, int& screenY)
{
    const float* view = viewMatrix.getPtr();
    const float* proj = projMatrix.getPtr();

    float viewX = view[0] * worldPos.x + view[4] * worldPos.y + view[8]  * worldPos.z + view[12];
    float viewY = view[1] * worldPos.x + view[5] * worldPos.y + view[9]  * worldPos.z + view[13];
    float viewZ = view[2] * worldPos.x + view[6] * worldPos.y + view[10] * worldPos.z + view[14];
    float viewW = view[3] * worldPos.x + view[7] * worldPos.y + view[11] * worldPos.z + view[15];

    float clipX = proj[0] * viewX + proj[4] * viewY + proj[8]  * viewZ + proj[12] * viewW;
    float clipY = proj[1] * viewX + proj[5] * viewY + proj[9]  * viewZ + proj[13] * viewW;
    float clipZ = proj[2] * viewX + proj[6] * viewY + proj[10] * viewZ + proj[14] * viewW;
    float clipW = proj[3] * viewX + proj[7] * viewY + proj[11] * viewZ + proj[15] * viewW;

    if (clipW <= 0.0f) {
        return false;
    }

    float ndcX = clipX / clipW;
    float ndcY = clipY / clipW;
    float ndcZ = clipZ / clipW;

    if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f || ndcZ < -1.0f || ndcZ > 1.0f) {
        return false;
    }

    screenX = static_cast<int>((ndcX * 0.5f + 0.5f) * static_cast<float>(screenWidth));
    screenY = static_cast<int>((ndcY * 0.5f + 0.5f) * static_cast<float>(screenHeight));
    return true;
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


void initCoins(std::vector<Coin*> &coinList, Mesh* coinMesh, GLuint coinTex, int numCoins) {
    coinList.clear();

    const int mapSize = 40;
    const int startX = 1; // player spawn world x=2 -> grid x=1
    const int startZ = 1; // player spawn world z=2 -> grid z=1

    bool visited[40][40] = {false};
    std::queue<std::pair<int, int>> q;
    std::vector<std::pair<int, int>> reachableTiles;

    if (levelMap.grid[startX][startZ] != 1) {
        return;
    }

    q.push(std::make_pair(startX, startZ));
    visited[startX][startZ] = true;

    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    while (!q.empty()) {
        std::pair<int, int> cell = q.front();
        q.pop();

        int x = cell.first;
        int z = cell.second;

        // Keep spawn tiles clear
        bool isPlayerSpawn = (x == startX && z == startZ);
        bool isEnemySpawn = (x == 38 && z == 38) || (x == 1 && z == 38) || (x == 38 && z == 1) || (x == 3 && z == 1);

        if (!isPlayerSpawn && !isEnemySpawn) {
            reachableTiles.push_back(cell);
        }

        for (int d = 0; d < 4; d++) {
            int nx = x + dirs[d][0];
            int nz = z + dirs[d][1];

            if (nx >= 0 && nx < mapSize && nz >= 0 && nz < mapSize &&
                !visited[nx][nz] && levelMap.grid[nx][nz] == 1) {
                visited[nx][nz] = true;
                q.push(std::make_pair(nx, nz));
            }
        }
    }

    // Shuffle reachable tiles (Fisher-Yates)
    for (size_t i = reachableTiles.size(); i > 1; --i) {
        size_t j = static_cast<size_t>(std::rand() % i);
        std::swap(reachableTiles[i - 1], reachableTiles[j]);
    }

    int spawnCount = std::min(numCoins, static_cast<int>(reachableTiles.size()));
    for (int i = 0; i < spawnCount; i++) {
        int gx = reachableTiles[i].first;
        int gz = reachableTiles[i].second;
        coinList.push_back(new Coin(Vector3f(gx * 2.0f, 2.0f, gz * 2.0f), coinMesh, coinTex));
    }
}

void startGame(int level) {
    // 1. Load the new map structure
    srand(static_cast<unsigned int>(time(0)));
    
    // 1. Load the new map structure

    levelMap.loadLevel(level);

    // 2. Clear old coins and spawn new ones based on the new map
    for (Coin* c : coins) delete c;
    coins.clear();
    int requestedCoins = level * 10;
    initCoins(coins, &coinsMesh, coinTexture, requestedCoins);
    totalCoins = static_cast<int>(coins.size());
    // 3. Clear old bullets
    for (bullet* b : bullets) delete b;
    bullets.clear();
    bulletOwners.clear();

    // 4. Reset Player and Enemy Positions
    myPlayer->setPosition(Vector3f(2.0f, 0.0f, 2.0f));
    myPlayer->setRotation(0.0f);
    myPlayer->HP = 100;

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
    enemyLastShootTimes.clear();

    // Define corner positions for enemies
    /*std::vector<Vector3f> enemyPositions = {
        Vector3f(76.0f, 0.0f, 76.0f),  // top-right
        Vector3f(2.0f, 0.0f, 76.0f),   // top-left  
        Vector3f(76.0f, 0.0f, 2.0f),   // bottom-right
        Vector3f(6.0f, 0.0f, 2.0f)     // bottom-left shifted away from player spawn
    };*/

    for(int i = 0; i < level && i < 4; i++) {
        generateEnemyPositions(enemies, 1); // Spawn 2 enemies per level, up to 4 enemies
    }

    // 5. Reset score and state
    score = 0;
    playerLastShootTime = 0;
    playerVelocity = 0.0f;
    timeExpired = false;
    levelTimeLimitSeconds = 30 + (level * 30);
    EnemyshootCooldown = std::max(200, 1500 - (level * 350)); // Enemies shoot faster each level, down to a minimum of 500ms
    remainingTimeSeconds = levelTimeLimitSeconds;
    levelStartTimeMs = glutGet(GLUT_ELAPSED_TIME);
    gameState = PLAYING;
}

void reshape(int width, int height)
{
    screenWidth = width;
    screenHeight = height;
    glViewport(0, 0, width, height);
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
    glClearColor(0.56f, 0.82f, 0.98f, 1.0f);
    
    if (glewInit() != GLEW_OK) 
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);             // NEW: Handle window resizing
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
    drawSunnyBackground();

    if (gameState == MENU) {
        // --- DRAW MAIN MENU ---
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 200, "TANK ASSIGNMENT");
        drawTextOnScreen(screenWidth / 2 - 120, screenHeight - 250, "Select a Level (Press 1-4):");
        
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 300, "1. Preset Level 1 (Holes)");
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 330, "2. Preset Level 2 (City Grid)");
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 360, "3. Preset Level 3 (Spiral)");
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 390, "4. Random Maze Generator");

        drawTextOnScreen(screenWidth / 2 - 120, screenHeight - 450, "Press ESC to Quit");
    }else if (gameState == GAME_OVER) {
        // --- DRAW GAME OVER SCREEN ---
        if (timeExpired) {
            drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 200, "TIME'S UP!");
        } else {
            drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 200, "GAME OVER");
        }
        drawTextOnScreen(screenWidth / 2 - 120, screenHeight - 250, "Press R to Restart or ESC to Quit");
    }
    else if (gameState == PLAYING) {
        // --- DRAW THE GAME ---
        glUseProgram(shaderProgramID);
        
        float aspectRatio = (float)screenWidth / (float)screenHeight;
        ProjectionMatrix.perspective(90, aspectRatio, 0.0001, 100.0);
        glUniformMatrix4fv(ProjectionUniformLocation, 1, false, ProjectionMatrix.getPtr());

        Matrix4x4 m = getViewMatrix();
        
        Vector3f sunInView = transformPoint(m, sunLightPosition);
        glUniform3f(LightPositionUniformLocation, sunInView.x, sunInView.y, sunInView.z);
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

       drawGroundShadows(m, ProjectionMatrix);
       drawWorldSun(m, ProjectionMatrix);
    drawAimPrediction(m, ProjectionMatrix);

        // --- DRAW TEXT OVERLAY ---
        // Convert your score integer to a string
        std::string scoreText = "Score: " + std::to_string(score);
        
        // Draw it at X: 20, Y: 680 (near the top-left of your 720x720 window)
        drawTextOnScreen(20, screenHeight - 40, scoreText);

        std::string hpText = "HP: " + std::to_string(myPlayer->HP);
        drawTextOnScreen(20, screenHeight - 70, hpText);

        std::string timeText = "Time: " + std::to_string(remainingTimeSeconds);
        drawTextOnScreen(20, screenHeight - 100, timeText);

        // Show alive enemies and their current HP on the right side of the HUD.
        std::string enemyCountText = "Enemies: " + std::to_string(enemies.size());
        drawTextOnScreen(screenWidth - 220, screenHeight - 40, enemyCountText);

        for (size_t i = 0; i < enemies.size(); i++) {
            int enemyHP = enemies[i]->HP;
            if (enemyHP < 0) {
                enemyHP = 0;
            }

            Vector3f labelPos = enemies[i]->getPosition();
            labelPos.y += 3.0f; // lift label above the tank body

            int labelX = 0;
            int labelY = 0;
            if (worldToScreen(labelPos, m, ProjectionMatrix, labelX, labelY)) {
                std::string enemyHpText = "HP: " + std::to_string(enemyHP);
                drawTextOnScreen(labelX - 20, labelY, enemyHpText);
            }
        }

        if (score >= totalCoins && totalCoins > 0) {
            gameState = WIN;
        }

        if (cameraView == FIRST_PERSON) {
            drawTextOnScreen(20, screenHeight - 130, "Camera: First Person");
        } else if (cameraView == THIRD_PERSON) {
            drawTextOnScreen(20, screenHeight - 130, "Camera: Third Person");
        } else if (cameraView == TOP_DOWN) {
            drawTextOnScreen(20, screenHeight - 130, "Camera: Top Down");
        }
        
        // Draw some instructions at the bottom
        drawTextOnScreen(20, 20, "WASD Move | Arrow Left/Right Turret | Arrow Up Shoot | C Camera | M Menu");

        glUseProgram(0);
    }
    else if (gameState == WIN) {
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 200, "YOU WIN!");
        drawTextOnScreen(screenWidth / 2 - 120, screenHeight - 250, "Press R to Restart or ESC to Quit");
    }
    
    glutSwapBuffers();
}

void keyboard(unsigned char key, int /*x*/, int /*y*/)
{
    if (key == 27) exit(0); // ESC to quit
    
    if (gameState == MENU) {
        if (key == '1') startGame(1);
        if (key == '2') startGame(2);
        if (key == '3') startGame(3);
        if (key == '4') startGame(4);
    } else if (gameState == PLAYING) {
        if (key == 'm' || key == 'M') {
            playerVelocity = 0.0f;
            gameState = MENU; // Press M to return to menu
        }
        if (key == 'c' || key == 'C') {
            changeCamera(); // Cycle camera view
        }
        keyStates[key] = true;
    }else if (gameState == GAME_OVER) {
        if (key == 'r' || key == 'R') {
            gameState = MENU; // go to menu to select level again
        }
    } else if (gameState == WIN) {
        if (key == 'r' || key == 'R') {
            gameState = MENU; // go to menu to select level again
        }
    }

}

void keyUp(unsigned char key, int /*x*/, int /*y*/)
{
    keyStates[key] = false;
}

bool collidesWithEnemyAt(const Vector3f& pos, float radius)
{
    for (size_t i = 0; i < enemies.size(); i++) {
        Vector3f enemyPos = enemies[i]->getPosition();
        float dx = pos.x - enemyPos.x;
        float dz = pos.z - enemyPos.z;
        if (sqrt(dx * dx + dz * dz) < radius) {
            return true;
        }
    }
    return false;
}


void handleKeys(float deltaSeconds)
{
    Vector3f currentPos = myPlayer->getPosition();
    float currentRot = myPlayer->getRotation(); // Get current rotation in Degrees

    // 1. Steer left and right (Degrees)
    if(keyStates['a']) {
        currentRot += rotationSpeed;
    }
    if(keyStates['d']) {
        currentRot -= rotationSpeed;
    }

    // 2. CONVERT DEGREES TO RADIANS for the math functions!
    float rad = currentRot * (M_PI / 180.0f);

    // 3. Acceleration-based drive model with simple drag.
    if (keyStates['w']) {
        playerVelocity += playerAcceleration * deltaSeconds;
    } else if (keyStates['s']) {
        playerVelocity -= playerAcceleration * deltaSeconds;
    } else {
        playerVelocity *= std::pow(playerDrag, deltaSeconds * 60.0f);
        if (fabs(playerVelocity) < 0.0005f) {
            playerVelocity = 0.0f;
        }
    }

    playerVelocity = std::min(playerMaxSpeed, playerVelocity);
    playerVelocity = std::max(playerReverseSpeed, playerVelocity);

    Vector3f nextPos = currentPos;
    nextPos.x += playerVelocity * deltaSeconds * sin(rad);
    nextPos.z += playerVelocity * deltaSeconds * cos(rad);

    // Keep player inside the maze world bounds.
    nextPos.x = std::max(0.0f, std::min(79.5f, nextPos.x));
    nextPos.z = std::max(0.0f, std::min(79.5f, nextPos.z));

    // Basic collision: prevent tank overlap with enemies.
    if (!collidesWithEnemyAt(nextPos, 2.0f)) {
        currentPos = nextPos;
    } else {
        playerVelocity = 0.0f;
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

            Vector3f spawnPos = getProjectileSpawnPosition(currentPos, turretRad);

            bullet* b = new bullet(
                spawnPos,
                &ball,
                ballTexture
            );
            b->setrad(turretRad);
            bullets.push_back(b);
            bulletOwners.push_back(PLAYER_BULLET);
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

void Timer(int /*value*/)
{
    if (gameState != PLAYING) {
        previousTimerInitialized = false;
        glutPostRedisplay();
        glutTimerFunc(10, Timer, 0);
        return;
    }

    int currentTimerMs = glutGet(GLUT_ELAPSED_TIME);
    if (!previousTimerInitialized) {
        previousTimerMs = currentTimerMs;
        previousTimerInitialized = true;
    }
    float deltaSeconds = static_cast<float>(currentTimerMs - previousTimerMs) / 1000.0f;
    previousTimerMs = currentTimerMs;
    deltaSeconds = std::max(0.001f, std::min(deltaSeconds, 0.05f));

    int elapsedMs = currentTimerMs - levelStartTimeMs;
    int elapsedSeconds = elapsedMs / 1000;
    remainingTimeSeconds = std::max(0, levelTimeLimitSeconds - elapsedSeconds);
    updateDayNightCycle(elapsedMs);
    if (remainingTimeSeconds <= 0) {
        timeExpired = true;
        playerVelocity = 0.0f;
        gameState = GAME_OVER;
        glutPostRedisplay();
        glutTimerFunc(10, Timer, 0);
        return;
    }

    // 1. Move the Player based on WASD keys & check for shooting
    handleKeys(deltaSeconds);
    myPlayer->spinWheels(std::fabs(playerVelocity) * deltaSeconds * 30.0f);

    // 2. Apply Gravity
    applyGravity(deltaSeconds);


    // 3. UPDATE BULLETS & DELETE OUT-OF-BOUNDS BULLETS
    for (size_t i = 0; i < bullets.size(); i++) {
        bullets[i]->update(deltaSeconds); // Move the bullet forward

        if (bullets[i]->getPosition().y <= 0.0f) {
            removeBulletAt(i);
            i--;
            continue;
        }

        // If the bullet flies past 100 units, destroy it to save memory!
        if (abs(bullets[i]->getPosition().x) > 100.0f || abs(bullets[i]->getPosition().z) > 100.0f) {
            removeBulletAt(i);
            i--; // Adjust index after erasing
        }
    }

    // 4. Check for Coin Collisions
    Vector3f playerPos = myPlayer->getPosition();

    if(playerPos.y < -10.0f) {
        gameState = GAME_OVER;
        playerVelocity = 0.0f;
        glutPostRedisplay();
        glutTimerFunc(10, Timer, 0);
    }

    collectCoins(playerPos.x, playerPos.z, coins);

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
            myEnemy->spinWheels(stepDist * 12.0f);
            
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
        float turnSpeed = 1.0f;
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
            if (i < enemyLastShootTimes.size() && currentTime - enemyLastShootTimes[i] >= EnemyshootCooldown) {
                float turretRad = targetWorldAngle * (M_PI / 180.0f);

                Vector3f spawnPos = getProjectileSpawnPosition(currentPosEny, turretRad);

                bullet* b = new bullet(
                    spawnPos,
                    &ball,
                    ballTexture
                );
                b->setrad(turretRad); // Make sure the bullet travels along the absolute world angle
                bullets.push_back(b);
                bulletOwners.push_back(ENEMY_BULLET);
                enemyLastShootTimes[i] = currentTime;
            }
        }
    }
    
    myEnemy->setPosition(enemyPos);
    }

    // 6. Resolve bullet collisions in the simulation loop.
    bullet_hit();

    // 7. Trigger a redraw and loop
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

Vector3f transformPoint(const Matrix4x4& matrix, const Vector3f& point)
{
    const float* m = matrix.getPtr();
    return Vector3f(
        m[0] * point.x + m[4] * point.y + m[8]  * point.z + m[12],
        m[1] * point.x + m[5] * point.y + m[9]  * point.z + m[13],
        m[2] * point.x + m[6] * point.y + m[10] * point.z + m[14]
    );
}

void drawOverlayEllipse(float centerX, float centerY, float radiusX, float radiusY, float red, float green, float blue, float alpha)
{
    const int segments = 40;
    glColor4f(red, green, blue, alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(centerX, centerY);
    for (int i = 0; i <= segments; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(segments)) * 2.0f * static_cast<float>(M_PI);
        glVertex2f(centerX + cosf(angle) * radiusX, centerY + sinf(angle) * radiusY);
    }
    glEnd();
}

void drawSunnyBackground()
{
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(screenWidth), 0.0, static_cast<double>(screenHeight), -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Sky gradient from horizon to zenith, driven by the timer.
    glBegin(GL_QUADS);
    glColor3f(skyHorizonColor.x, skyHorizonColor.y, skyHorizonColor.z);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(static_cast<float>(screenWidth), 0.0f);
    glColor3f(skyZenithColor.x, skyZenithColor.y, skyZenithColor.z);
    glVertex2f(static_cast<float>(screenWidth), static_cast<float>(screenHeight));
    glVertex2f(0.0f, static_cast<float>(screenHeight));
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void drawWorldSun(const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix)
{
    if (dayNightStrength <= 0.02f) {
        return;
    }

    // --- NEW: Project the 3D sun position onto the 2D screen ---
    int sx = 0;
    int sy = 0;
    if (!worldToScreen(sunLightPosition, viewMatrix, projMatrix, sx, sy)) {
        // If worldToScreen returns false, the sun is behind the camera.
        // We return early so we don't draw it!
        return;
    }

    float drawX = static_cast<float>(sx);
    float drawY = static_cast<float>(sy);
    // -----------------------------------------------------------

    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(screenWidth), 0.0, static_cast<double>(screenHeight), -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    const float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
    const float radius = std::max(16.0f, minDim * 0.03f);
    float glowAlpha = 0.08f + 0.18f * dayNightStrength;
    
    // Use the newly calculated drawX and drawY coordinates
    drawOverlayEllipse(drawX, drawY, radius * 1.8f, radius * 1.8f, 1.0f, 0.90f, 0.45f, glowAlpha * 0.6f);
    drawOverlayEllipse(drawX, drawY, radius * 1.25f, radius * 1.25f, 1.0f, 0.96f, 0.62f, glowAlpha);
    drawOverlayEllipse(drawX, drawY, radius, radius, 1.0f, 0.98f, 0.80f, 0.95f * dayNightStrength);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void drawGroundShadows(const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix)
{
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(screenWidth), 0.0, static_cast<double>(screenHeight), -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    auto drawShadowForObject = [&](const Vector3f& worldPos, float radiusX, float radiusY, float alpha) {
        Vector3f groundPos = worldPos;
        groundPos.y = 0.05f;

        int screenX = 0;
        int screenY = 0;
        if (!worldToScreen(groundPos, viewMatrix, projMatrix, screenX, screenY)) {
            return;
        }

        float dx = groundPos.x - sunLightPosition.x;
        float dz = groundPos.z - sunLightPosition.z;
        float length = std::sqrt(dx * dx + dz * dz);
        if (length < 0.001f) {
            length = 1.0f;
        }

        const float shadowOffset = 16.0f;
        screenX += static_cast<int>((dx / length) * shadowOffset);
        screenY += static_cast<int>((dz / length) * shadowOffset * 0.5f);

        drawOverlayEllipse(static_cast<float>(screenX), static_cast<float>(screenY), radiusX, radiusY, 0.08f, 0.08f, 0.10f, alpha * (0.25f + 0.75f * dayNightStrength));
    };

    drawShadowForObject(myPlayer->getPosition(), 22.0f, 10.0f, 0.28f);
    for (size_t i = 0; i < enemies.size(); ++i) {
        drawShadowForObject(enemies[i]->getPosition(), 20.0f, 9.0f, 0.26f);
    }
    for (size_t i = 0; i < coins.size(); ++i) {
        drawShadowForObject(coins[i]->getPosition(), 10.0f, 5.0f, 0.18f);
    }
    for (size_t i = 0; i < bullets.size(); ++i) {
        drawShadowForObject(bullets[i]->getPosition(), 5.0f, 3.0f, 0.16f);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void updateDayNightCycle(int elapsedMs)
{
    if (levelTimeLimitSeconds <= 0) {
        return;
    }

    float totalMs = static_cast<float>(levelTimeLimitSeconds) * 1000.0f;
    float progress = std::fmod(static_cast<float>(elapsedMs) / totalMs, 1.0f);
    if (progress < 0.0f) {
        progress += 1.0f;
    }

    float startX = 76.0f;
    float endX = 4.0f;
    float lowY = 8.0f;
    float highY = 50.0f;
    float arc = std::sin(progress * static_cast<float>(M_PI));
    dayNightStrength = std::max(0.0f, arc);

    sunLightPosition = Vector3f(
        startX + (endX - startX) * progress,
        lowY + (highY - lowY) * arc,
        20.0f
    );

    //sunScreenX = static_cast<float>(screenWidth) * (0.90f - 0.80f * progress);
    //sunScreenY = static_cast<float>(screenHeight) * (0.22f + 0.56f * arc);

    ambient = Vector3f(
        0.03f + 0.12f * dayNightStrength,
        0.03f + 0.11f * dayNightStrength,
        0.05f + 0.13f * dayNightStrength
    );

    specularPower = 4.0f + 16.0f * dayNightStrength;

    skyHorizonColor = Vector3f(
        0.06f + 0.66f * dayNightStrength,
        0.08f + 0.82f * dayNightStrength,
        0.14f + 0.86f * dayNightStrength
    );

    skyZenithColor = Vector3f(
        0.02f + 0.38f * dayNightStrength,
        0.04f + 0.56f * dayNightStrength,
        0.10f + 0.90f * dayNightStrength
    );
}

Matrix4x4 getViewMatrix()
{
    Matrix4x4 view;
    view.toIdentity();

    Vector3f playerPos = myPlayer->getPosition();
    float playerRad = myPlayer->getRotation() * (M_PI / 180.0f);
    Vector3f forward = Vector3f(sin(playerRad), 0.0f, cos(playerRad));
    Vector3f right = Vector3f(cos(playerRad), 0.0f, -sin(playerRad));
    
    if (cameraView == FIRST_PERSON) {
        // Position camera at driver's position with slight forward offset
        Vector3f eye = playerPos + (forward * 0.5f) + (right * 0.3f) + Vector3f(0.0f, 2.3f, 0.0f);
        Vector3f center = eye + (forward * 15.0f);
        view.lookAt(eye, center, Vector3f(0.0f, 1.0f, 0.0f));
    } else if (cameraView == TOP_DOWN) {
        // True top-down view - looking straight down at the player
        Vector3f eye = playerPos + Vector3f(0.0f, 40.0f, 0.0f);
        view.lookAt(eye, playerPos, Vector3f(0.0f, 1.0f, 0.0f));
    } else {
        // THIRD_PERSON: Follow cannon direction with improved aiming view
        float cannonWorldRad = (myPlayer->getRotation() + myPlayer->getTurretRotation()) * (M_PI / 180.0f);
        Vector3f cannonForward = Vector3f(sin(cannonWorldRad), 0.0f, cos(cannonWorldRad));
        Vector3f cannonRight = Vector3f(cos(cannonWorldRad), 0.0f, -sin(cannonWorldRad));

        // Position camera higher and closer for better aiming visibility
        Vector3f eye = playerPos + (cannonRight * -1.5f) + Vector3f(0.0f, 7.0f, 0.0f) - (cannonForward * 8.0f);
        Vector3f center = playerPos + Vector3f(0.0f, 2.5f, 0.0f) + (cannonForward * 25.0f);
        view.lookAt(eye, center, Vector3f(0.0f, 1.0f, 0.0f));
    }

    return view;
}

void specialKeyboard(int key, int /*x*/, int /*y*/)
{
    // GLUT passes in macros like GLUT_KEY_LEFT, which are safely between 0 and 255
    if (key >= 0 && key < 256 && gameState == PLAYING) {
        specialKeyStates[key] = true;
    }
}

void changeCamera() {
    int nextView = static_cast<int>(cameraView) + 1;
    if (nextView > static_cast<int>(TOP_DOWN)) {
        nextView = static_cast<int>(FIRST_PERSON);
    }
    cameraView = static_cast<CameraView>(nextView);
}

void specialKeyUp(int key, int /*x*/, int /*y*/)
{
    if (key >= 0 && key < 256) {
        specialKeyStates[key] = false;
    }
}

