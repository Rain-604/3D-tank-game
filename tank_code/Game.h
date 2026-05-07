#ifndef GAME_H
#define GAME_H

#include <GL/glew.h>
#include <GL/glut.h>
#include <Shader.h>
#include <Vector.h>
#include <Matrix.h>
#include <Mesh.h>
#include <Texture.h>
#include <SphericalCameraManipulator.h>
#include <iostream>
#include <cmath>
#include <string>
#include <queue>
#include <algorithm>
#include <ctime>
#include "map.h"
#include "drawObj.h"
#include "DrawPlayer.h"
#include "DrawEnemy.h"
#include "drawBullet.h"
#include "Coins.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Configuration ---
namespace GameConfig {
    const int SCREEN_WIDTH = 1440;
    const int SCREEN_HEIGHT = 1400;
    const float TILE_SIZE = 2.0f;
    const float MAP_BOUND = 79.5f;
    const float GRAVITY = 18.0f;
    const float COLLISION_RADIUS = 1.5f;
    const float ENEMY_SPAWN_DIST = 10.0f;
    const int PLAYER_SHOOT_COOLDOWN = 500;
    const int ENEMY_BASE_COOLDOWN = 1500;
    const int TILE_BREAK_INTERVAL_MS = 5000;
    const float PLAYER_MAX_SPEED = 12.0f;
    const float PLAYER_ACCEL = 3.5f;
    const float PLAYER_DRAG = 0.90f;
    const float PLAYER_REVERSE = -6.0f;
    const float TURN_SPEED = 1.0f;
}

// --- Enums & Structs ---
enum GameState { MENU, PLAYING, GAME_OVER, WIN };
enum CameraView { FIRST_PERSON, THIRD_PERSON, TOP_DOWN };
enum BulletOwner { PLAYER_BULLET, ENEMY_BULLET };

struct ActiveBullet {
    bullet* obj;
    BulletOwner owner;
};

struct ActiveEnemy {
    DrawEnemy* obj;
    int lastShootTime;
};

struct Point2D {
    int x, z;
    bool operator==(const Point2D& other) const { return x == other.x && z == other.z; }
    bool operator!=(const Point2D& other) const { return x != other.x || z != other.z; }
};

// --- Game Class ---
class Game {
public:
    Game();
    ~Game();

    void initGL(int argc, char** argv);
    void run();

    // GLUT Callbacks
    void display();
    void timer(int value);
    void keyboard(unsigned char key, int x, int y);
    void keyUp(unsigned char key, int x, int y);
    void specialKeyboard(int key, int x, int y);
    void specialKeyUp(int key, int x, int y);
    void mouse(int button, int state, int x, int y);
    void motion(int x, int y);
    void reshape(int width, int height);

private:
    // Core Logic
    void initShader();
    void startGame(int level);
    void changeCamera();
    void handleKeys(float deltaSeconds);
    void applyGravity(float deltaSeconds);
    void collectCoins(float playerX, float playerZ);
    void handleBulletCollisions();
    void removeBulletAt(size_t index);
    bool collidesWithEnemyAt(const Vector3f& pos, float radius);
    void generateEnemyPositions(int numEnemies);
    void initCoins(int numCoins);
    void initTexture(std::string filename, GLuint &textureID, bool makeRed = false);
    void maybeBreakTiles(int elapsedMs);
    void breakRandomTiles();
    bool canBreakTile(int gx, int gz);
    bool areAllCoinsReachableAfterBreak(int breakX, int breakZ);
    bool isCoinTile(int gx, int gz);
    void rebuildMapPositions();
    
    // Helpers
    Point2D getNextStepBFS(Point2D start, Point2D goal);
    Vector3f getProjectileSpawnPosition(const Vector3f& basePos, float turretRad);
    Vector3f predictProjectileLandingPoint(const Vector3f& spawnPos, float turretRad);
    bool worldToScreen(const Vector3f& worldPos, const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix, int& screenX, int& screenY);
    Vector3f transformPoint(const Matrix4x4& matrix, const Vector3f& point);
    Matrix4x4 getViewMatrix();
    void updateDayNightCycle(int elapsedMs);

    // Rendering Helpers
    void drawTextOnScreen(int x, int y, const std::string& text);
    void drawSunnyBackground();
    void drawWorldSun(const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix);
    void drawGroundShadows(const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix);
    void drawOverlayEllipse(float centerX, float centerY, float radiusX, float radiusY, float red, float green, float blue, float alpha);
    void drawAimPrediction(const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix);

    // --- Variables ---
    int screenWidth, screenHeight;
    GameState gameState;
    CameraView cameraView;

    Map levelMap; 
    std::vector<DrawObj*> gameObjects;
    std::vector<Coin*> coins;
    std::vector<ActiveBullet> activeBullets;
    std::vector<ActiveEnemy> activeEnemies;

    DrawPlayer* myPlayer;

    bool keyStates[256];
    bool specialKeyStates[256];

    int score;
    int totalCoins;
    int levelStartTimeMs;
    int levelTimeLimitSeconds;
    int remainingTimeSeconds;
    bool timeExpired;
    int previousTimerMs;
    bool previousTimerInitialized;
    int lastTileBreakMs;

    int playerLastShootTime;
    float playerVelocity;
    int currentEnemyCooldown;

    // OpenGL & Rendering
    SphericalCameraManipulator cameraManip;
    Matrix4x4 ModelViewMatrix;                          
    Matrix4x4 ProjectionMatrix;                         
    GLuint shaderProgramID;                             
    GLuint vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute;
    GLuint MVMatrixUniformLocation, ProjectionUniformLocation, LightPositionUniformLocation;
    GLuint AmbientUniformLocation, SpecularUniformLocation, SpecularPowerUniformLocation, TextureMapUniformLocation;

    Vector3f sunLightPosition, ambient, specular;
    float specularPower, dayNightStrength;
    Vector3f skyHorizonColor, skyZenithColor;

    Mesh mesh, playerBodyMesh, turretMesh, frontWheelMesh, backWheelMesh, coinsMesh, ball;
    GLuint texture, playerTexture, enemyTexture, coinTexture, ballTexture;
};

#endif