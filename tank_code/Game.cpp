#include "Game.h"
#include <vector>
#include <set>
#include <cmath>

// Initialize game state defaults and runtime variables.
Game::Game() {

    screenWidth = GameConfig::SCREEN_WIDTH;
    screenHeight = GameConfig::SCREEN_HEIGHT;
    gameState = MENU;
    cameraView = FIRST_PERSON;
    myPlayer = nullptr;
    shaderProgramID = 0;

    enemydistroy = 0;
    
    score = 0;
    totalCoins = 0;
    levelStartTimeMs = 0;
    levelTimeLimitSeconds = 90;
    remainingTimeSeconds = 90;
    timeExpired = false;
    previousTimerMs = 0;
    previousTimerInitialized = false;
    lastTileBreakMs = 0;
    playerLastShootTime = 0;
    playerVelocity = 0.0f;
    currentEnemyCooldown = GameConfig::ENEMY_BASE_COOLDOWN;

    currentLevel = 1;
    scoreRecorded = false;
    for (int i=0; i<256; i++) {
        keyStates[i] = false;
        specialKeyStates[i] = false;
    }

    sunLightPosition = Vector3f(60.0f, 45.0f, 20.0f);
    ambient = Vector3f(0.1f, 0.1f, 0.1f);
    specular = Vector3f(1.0f, 1.0f, 1.0f);
    specularPower = 10.0f;
    dayNightStrength = 1.0f;
    skyHorizonColor = Vector3f(0.72f, 0.90f, 1.00f);
    skyZenithColor = Vector3f(0.40f, 0.74f, 1.00f);
    
    std::srand(static_cast<unsigned int>(std::time(0)));
}

// Release owned resources.
Game::~Game() {
    delete myPlayer;
    for (Coin* c : coins) delete c;
    for (ActiveBullet ab : activeBullets) delete ab.obj;
    for (ActiveEnemy ae : activeEnemies) delete ae.obj;
    glDeleteProgram(shaderProgramID);
}

// Enter the GLUT main loop.
void Game::run() {
    glutMainLoop();
}

// Initialize OpenGL, load assets, and set up the scene.
void Game::initGL(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(screenWidth, screenHeight);
    glutInitWindowPosition(200, 200);
    glutCreateWindow("Tank Assignment");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.56f, 0.82f, 0.98f, 1.0f);
    
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        exit(-1);
    }
    
    initShader();
    
    mesh.loadOBJ("../models/cube.obj");    
    initTexture("../models/Crate.bmp", texture);
    coinsMesh.loadOBJ("../models/coin.obj");
    initTexture("../models/coin.bmp", coinTexture);
    
    playerBodyMesh.loadOBJ("../models/chassis.obj");
    turretMesh.loadOBJ("../models/turret.obj");
    frontWheelMesh.loadOBJ("../models/front_wheel.obj");
    backWheelMesh.loadOBJ("../models/back_wheel.obj");
    initTexture("../models/hamvee.bmp", playerTexture, false);
    initTexture("../models/hamvee.bmp", enemyTexture, true);

    ball.loadOBJ("../models/ball.obj");
    initTexture("../models/ball.bmp", ballTexture);

    cameraManip.setPanTiltRadius(0.f, 0.5f, 15.f);
    cameraManip.setFocus(mesh.getMeshCentroid());

    levelMap = Map();
    myPlayer = new DrawPlayer(Vector3f(2.0f, 0.0f, 2.0f), &playerBodyMesh, &turretMesh, &frontWheelMesh, &backWheelMesh, playerTexture);
    gameObjects.push_back(myPlayer);
}

// Load and cache shader uniforms/attributes.
void Game::initShader() {
    shaderProgramID = Shader::LoadFromFile("shader.vert","shader.frag");
    vertexPositionAttribute = glGetAttribLocation(shaderProgramID, "aVertexPosition");
    vertexNormalAttribute = glGetAttribLocation(shaderProgramID, "aVertexNormal");
    vertexTexcoordAttribute = glGetAttribLocation(shaderProgramID, "aVertexTexcoord");

    MVMatrixUniformLocation = glGetUniformLocation(shaderProgramID, "MVMatrix_uniform"); 
    ProjectionUniformLocation = glGetUniformLocation(shaderProgramID, "ProjMatrix_uniform"); 
    LightPositionUniformLocation = glGetUniformLocation(shaderProgramID, "LightPosition_uniform"); 
    AmbientUniformLocation = glGetUniformLocation(shaderProgramID, "Ambient_uniform"); 
    SpecularUniformLocation = glGetUniformLocation(shaderProgramID, "Specular_uniform"); 
    SpecularPowerUniformLocation = glGetUniformLocation(shaderProgramID, "SpecularPower_uniform");
    TextureMapUniformLocation = glGetUniformLocation(shaderProgramID, "TextureMap_uniform"); 
}

// Load a BMP texture and optionally swap channels for tinting.
void Game::initTexture(std::string filename, GLuint & textureID, bool makeRed) {
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 

    int width = 0, height = 0;
    char* data = nullptr;
    bool ok = Texture::LoadBMP(filename, width, height, data);
    if (!ok || data == nullptr) {
        std::cerr << "Game::initTexture - failed to load '" << filename << "', creating placeholder texture" << std::endl;
        unsigned char pixel[3] = {255, 0, 255}; // magenta placeholder
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel);
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    if (makeRed) {
        int totalBytes = width * height * 3;
        for (int i = 0; i < totalBytes; i += 3) {
            // Safely cast to unsigned to do math without weird color glitches
            unsigned char c1 = data[i];
            unsigned char c2 = data[i+1];
            unsigned char c3 = data[i+2];

            // 1. Convert pixel to grayscale to preserve the tank's shadows and metal details
            float gray = (c1 + c2 + c3) / 3.0f;

            // 2. Apply a high-contrast tint! (Currently set to Neon Magenta/Purple)
            // Multiplying by > 1.0 boosts the color, < 1.0 darkens it.
            data[i]   = static_cast<char>(std::min(255.0f, gray * 1.8f)); 
            data[i+1] = static_cast<char>(gray * 0.2f);                   
            data[i+2] = static_cast<char>(std::min(255.0f, gray * 1.8f)); 
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    delete[] data;
}

// Reset state and spawn level content.
void Game::startGame(int level) {
    // Clear any carry-over state so the new run starts from a clean baseline.
    currentLevel = level;
    scoreRecorded = false;
    levelMap.loadLevel(level);

    for (Coin* c : coins) delete c;
    coins.clear();
    initCoins(level * 10);
    totalCoins = static_cast<int>(coins.size());

    for (ActiveBullet ab : activeBullets) delete ab.obj;
    activeBullets.clear();

    myPlayer->setPosition(Vector3f(2.0f, 0.0f, 2.0f));
    myPlayer->setRotation(0.0f);
    myPlayer->HP = 150;

    for (ActiveEnemy ae : activeEnemies) {
        auto it = std::find(gameObjects.begin(), gameObjects.end(), ae.obj);
        if (it != gameObjects.end()) gameObjects.erase(it);
        delete ae.obj;
    }
    activeEnemies.clear();

    for(int i = 0; i < level && i < 4; i++) {
        generateEnemyPositions(1);
    }

    score = 0;
    playerLastShootTime = 0;
    playerVelocity = 0.0f;
    timeExpired = false;
    levelTimeLimitSeconds = 30 + (level * 30);
    if (level == 5) {
        levelTimeLimitSeconds = 30; // Survival mode has a fixed time limit
    }
    currentEnemyCooldown = std::max(200, GameConfig::ENEMY_BASE_COOLDOWN - (level * 350));
    remainingTimeSeconds = levelTimeLimitSeconds;
    levelStartTimeMs = glutGet(GLUT_ELAPSED_TIME);
    lastTileBreakMs = 0;
    gameState = PLAYING;
}

// Fixed-timestep game update, called by the GLUT timer.
void Game::timer(int /*value*/) {
    if (gameState != PLAYING) {
        previousTimerInitialized = false;
        glutPostRedisplay();
        return; // Will be re-triggered by wrapper
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
    // Mission time drives both the countdown and the lighting cycle.
    remainingTimeSeconds = std::max(0, levelTimeLimitSeconds - (elapsedMs / 1000));
    updateDayNightCycle(elapsedMs);

    if (remainingTimeSeconds <= 0 || myPlayer->getPosition().y < -10.0f) {
        timeExpired = (remainingTimeSeconds <= 0);
        playerVelocity = 0.0f;
        gameState = GAME_OVER;
        glutPostRedisplay();
        return;
    }

    // --- Survival Mode: Continuous Enemy Spawning ---
    if (currentLevel == 5 && activeEnemies.size() < 5) {
        // Simple logic to keep at least 5 enemies in the arena
        generateEnemyPositions(5 - (int)activeEnemies.size());
    }

    // Break tiles on a schedule so the map gradually becomes more dangerous.
    maybeBreakTiles(elapsedMs);

    // 1. Player Update
    handleKeys(deltaSeconds);
    myPlayer->spinWheels(std::fabs(playerVelocity) * deltaSeconds * 30.0f);
    applyGravity(deltaSeconds);
    collectCoins(myPlayer->getPosition().x, myPlayer->getPosition().z);

    // 2. Bullet Update
    for (size_t i = 0; i < activeBullets.size(); i++) {
        activeBullets[i].obj->update(deltaSeconds);
        if (activeBullets[i].obj->getPosition().y <= 0.0f ||
            abs(activeBullets[i].obj->getPosition().x) > 100.0f || 
            abs(activeBullets[i].obj->getPosition().z) > 100.0f) {
            removeBulletAt(i);
            i--;
        }
    }
    handleBulletCollisions();

    // 3. Enemy Update
    Vector3f target = myPlayer->getPosition();
    for(size_t i = 0; i < activeEnemies.size(); i++) {
        DrawEnemy* myEnemy = activeEnemies[i].obj;
        Vector3f enemyPos = myEnemy->getPosition();
        
        float dx = target.x - enemyPos.x;
        float dz = target.z - enemyPos.z;
        float dist = sqrt(dx*dx + dz*dz);
        float currentRotEny = myEnemy->getRotation();

        int eGridX = std::max(0, std::min(39, (int)round(enemyPos.x / GameConfig::TILE_SIZE)));
        int eGridZ = std::max(0, std::min(39, (int)round(enemyPos.z / GameConfig::TILE_SIZE)));
        int pGridX = std::max(0, std::min(39, (int)round(target.x / GameConfig::TILE_SIZE)));
        int pGridZ = std::max(0, std::min(39, (int)round(target.z / GameConfig::TILE_SIZE)));

        if (dist > 10.0f) {
            Point2D nextStep = getNextStepAStar({eGridX, eGridZ}, {pGridX, pGridZ});
            Vector3f targetPos = {nextStep.x * GameConfig::TILE_SIZE, enemyPos.y, nextStep.z * GameConfig::TILE_SIZE};
            float stepDx = targetPos.x - enemyPos.x;
            float stepDz = targetPos.z - enemyPos.z;
            float stepDist = sqrt(stepDx*stepDx + stepDz*stepDz);
            
            if (stepDist > 0.1f) {
                float speed = (currentLevel == 5) ? 0.10f : 0.05f; 
                enemyPos.x += (stepDx / stepDist) * speed;
                enemyPos.z += (stepDz / stepDist) * speed;

                // --- NEW: Enemy-Enemy Physical Collision ---
                // If they get too close despite pathfinding redirection, push them apart.
                for (size_t k = 0; k < activeEnemies.size(); k++) {
                    if (i == k) continue;
                    Vector3f otherPos = activeEnemies[k].obj->getPosition();
                    float distK = sqrt(pow(enemyPos.x - otherPos.x, 2) + pow(enemyPos.z - otherPos.z, 2));
                    if (distK < GameConfig::COLLISION_RADIUS * 1.5f) { // Soft radius check
                        float pushX = (enemyPos.x - otherPos.x) / distK;
                        float pushZ = (enemyPos.z - otherPos.z) / distK;
                        enemyPos.x += pushX * 0.5f; // Slight push away
                        enemyPos.z += pushZ * 0.5f;
                    }
                }

                myEnemy->spinWheels(stepDist * 12.0f);
                myEnemy->setRotation(atan2(stepDx, stepDz) * (180.0f / M_PI));
            }
        } else {
            float targetWorldAngle = atan2(dx, dz) * (180.0f / M_PI);
            float targetTurretAngle = targetWorldAngle - currentRotEny;
            float currentTurretAngle = myEnemy->getTurretRotation();
            float diff = targetTurretAngle - currentTurretAngle;
            
            while (diff > 180.0f) diff -= 360.0f;
            while (diff < -180.0f) diff += 360.0f;
            
            float turnSpeed = 2.0f;
            if (diff > turnSpeed) myEnemy->rotateTurret(turnSpeed);
            else if (diff < -turnSpeed) myEnemy->rotateTurret(-turnSpeed);
            else {
                myEnemy->rotateTurret(diff);
                if (currentTimerMs - activeEnemies[i].lastShootTime >= currentEnemyCooldown) {
                    float turretRad = targetWorldAngle * (M_PI / 180.0f);
                    Vector3f spawnPos = getProjectileSpawnPosition(enemyPos, turretRad);
                    bullet* b = new bullet(spawnPos, &ball, ballTexture);
                    b->setrad(turretRad); 
                    activeBullets.push_back({b, ENEMY_BULLET});
                    activeEnemies[i].lastShootTime = currentTimerMs;
                }
            }
        }
        myEnemy->setPosition(enemyPos);
    }
    glutPostRedisplay();
}

// Break tiles at fixed intervals while keeping a path to all coins.
void Game::maybeBreakTiles(int elapsedMs) {
    // Use elapsed mission time instead of frame count so the timing stays stable.
    if (elapsedMs - lastTileBreakMs < GameConfig::TILE_BREAK_INTERVAL_MS) {
        return;
    }

    while (elapsedMs - lastTileBreakMs >= GameConfig::TILE_BREAK_INTERVAL_MS) {
        lastTileBreakMs += GameConfig::TILE_BREAK_INTERVAL_MS;
        breakRandomTiles();
    }
}

// Randomly remove 1-2 walkable tiles if constraints are satisfied.
void Game::breakRandomTiles() {
    const int tilesToBreak = 1 + (std::rand() % 2);
    int broken = 0;
    int attempts = 0;

    while (broken < tilesToBreak && attempts < 400) {
        // Keep sampling until we find a tile that is safe to remove.
        int gx = std::rand() % 40;
        int gz = std::rand() % 40;
        attempts++;

        if (!canBreakTile(gx, gz)) {
            continue;
        }
        if (!areAllCoinsReachableAfterBreak(gx, gz)) {
            continue;
        }

        // Remove the tile only after all validation checks pass.
        levelMap.grid[gx][gz] = 0;
        broken++;
    }

    if (broken > 0) {
        rebuildMapPositions();
    }
}

// Check whether a tile is eligible for breaking.
bool Game::canBreakTile(int gx, int gz) {
    if (gx <= 0 || gx >= 39 || gz <= 0 || gz >= 39) {
        return false; // needs all 4 neighbors in bounds
    }
    if (levelMap.grid[gx][gz] != 1) {
        return false;
    }

    Vector3f playerPos = myPlayer->getPosition();
    int pX = static_cast<int>(std::round(playerPos.x / GameConfig::TILE_SIZE));
    int pZ = static_cast<int>(std::round(playerPos.z / GameConfig::TILE_SIZE));
    pX = std::max(0, std::min(39, pX));
    pZ = std::max(0, std::min(39, pZ));
    if (gx == pX && gz == pZ) {
        return false;
    }
    if (isCoinTile(gx, gz)) {
        return false;
    }

    if (levelMap.grid[gx + 1][gz] != 1) return false;
    if (levelMap.grid[gx - 1][gz] != 1) return false;
    if (levelMap.grid[gx][gz + 1] != 1) return false;
    if (levelMap.grid[gx][gz - 1] != 1) return false;

    return true;
}

// Ensure the player can still reach every remaining coin after a break.
bool Game::areAllCoinsReachableAfterBreak(int breakX, int breakZ) {
    if (coins.empty()) {
        return true;
    }

    Vector3f playerPos = myPlayer->getPosition();
    int startX = static_cast<int>(std::round(playerPos.x / GameConfig::TILE_SIZE));
    int startZ = static_cast<int>(std::round(playerPos.z / GameConfig::TILE_SIZE));
    startX = std::max(0, std::min(39, startX));
    startZ = std::max(0, std::min(39, startZ));

    if (startX == breakX && startZ == breakZ) {
        return false;
    }
    if (levelMap.grid[startX][startZ] != 1) {
        return false;
    }

    bool visited[40][40] = {false};
    std::queue<Point2D> q;
    q.push({startX, startZ});
    visited[startX][startZ] = true;

    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!q.empty()) {
        Point2D curr = q.front();
        q.pop();

        for (int d = 0; d < 4; d++) {
            int nx = curr.x + dirs[d][0];
            int nz = curr.z + dirs[d][1];
            if (nx < 0 || nx >= 40 || nz < 0 || nz >= 40) {
                continue;
            }
            if (visited[nx][nz]) {
                continue;
            }
            if (nx == breakX && nz == breakZ) {
                continue;
            }
            if (levelMap.grid[nx][nz] != 1) {
                continue;
            }
            visited[nx][nz] = true;
            q.push({nx, nz});
        }
    }

    for (Coin* c : coins) {
        Vector3f cpos = c->getPosition();
        int cx = static_cast<int>(std::round(cpos.x / GameConfig::TILE_SIZE));
        int cz = static_cast<int>(std::round(cpos.z / GameConfig::TILE_SIZE));
        if (cx < 0 || cx >= 40 || cz < 0 || cz >= 40) {
            return false;
        }
        if (cx == breakX && cz == breakZ) {
            return false;
        }
        if (!visited[cx][cz]) {
            return false;
        }
    }

    return true;
}

// Check whether a coin occupies the given tile.
bool Game::isCoinTile(int gx, int gz) {
    for (Coin* c : coins) {
        Vector3f cpos = c->getPosition();
        int cx = static_cast<int>(std::round(cpos.x / GameConfig::TILE_SIZE));
        int cz = static_cast<int>(std::round(cpos.z / GameConfig::TILE_SIZE));
        if (cx == gx && cz == gz) {
            return true;
        }
    }
    return false;
}

// Rebuild cube positions after the grid changes.
void Game::rebuildMapPositions() {
    levelMap.cubePositions.clear();
    levelMap.targetPositions.clear();
    const int tileSpacing = static_cast<int>(GameConfig::TILE_SIZE);

    for (int i = 0; i < 40; i++) {
        for (int j = 0; j < 40; j++) {
            if (levelMap.grid[i][j] == 1 || levelMap.grid[i][j] == 2) {
                levelMap.cubePositions.push_back({ i * tileSpacing, 0, j * tileSpacing });
                if (levelMap.grid[i][j] == 2) {
                    levelMap.targetPositions.push_back({ i, 0, j });
                }
            }
        }
    }
}

// Resolve bullet hits against enemies/player.
// Resolve bullet hits against enemies/player.
void Game::handleBulletCollisions() {
    for (size_t i = 0; i < activeBullets.size(); i++) {
        Vector3f bulletPos = activeBullets[i].obj->getPosition();
        
        if (activeBullets[i].owner == PLAYER_BULLET) {
            // Player bullets can damage any enemy tank in range.
            // Check collisions against all enemies
            for (size_t j = 0; j < activeEnemies.size(); j++) {
                DrawEnemy* e = activeEnemies[j].obj;
                Vector3f enemyPos = e->getPosition();
                float dx = bulletPos.x - enemyPos.x, dy = bulletPos.y - enemyPos.y, dz = bulletPos.z - enemyPos.z;
                if (sqrt(dx*dx + dy*dy + dz*dz) < GameConfig::COLLISION_RADIUS) {
                    e->HP -= 20;
                    if (e->HP <= 0) {
                        auto it = std::find(gameObjects.begin(), gameObjects.end(), e);
                        enemydistroy ++;
                        if (it != gameObjects.end()) gameObjects.erase(it);
                        delete e;
                        activeEnemies.erase(activeEnemies.begin() + j);
                    }
                    removeBulletAt(i); i--; break;
                }
            }
        } else {
            // Enemy bullets only apply damage to the player.
            // Enemy bullets hitting the player
            Vector3f playerPos = myPlayer->getPosition();
            float dx = bulletPos.x - playerPos.x, dy = bulletPos.y - playerPos.y, dz = bulletPos.z - playerPos.z;
            if (sqrt(dx*dx + dy*dy + dz*dz) < GameConfig::COLLISION_RADIUS) {
                myPlayer->HP -= 20;
                removeBulletAt(i); i--;
                if (myPlayer->HP <= 0) gameState = GAME_OVER;
            }
        }
    }
}

// Delete a bullet and remove it from the active list.
void Game::removeBulletAt(size_t index) {
    delete activeBullets[index].obj;
    activeBullets.erase(activeBullets.begin() + index);
}

// Render the frame and HUD.
// Render the frame and HUD.
void Game::display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawSunnyBackground();

    if (gameState == MENU) {
        // Shared button metrics
        const int buttonX = (screenWidth / 2) - 190;
        const int buttonWidth = 380;
        const int buttonHeight = 48;
        const int buttonGap = 16;
        const int firstButtonY = screenHeight - 360;
        
        // Dynamic Panel Box wrapping around the buttons and title
        const int panelW = buttonWidth + 100;
        const int panelH = 450;
        const int panelX = (screenWidth - panelW) / 2;
        const int panelY = screenHeight - 500;

        const char* labels[5] = {
            "1. Preset Level 1 (Holes)",
            "2. Preset Level 2 (City Grid)",
            "3. Preset Level 3 (Spiral)",
            "4. Random Maze Generator",
            "5. Survival Arena (Infinite Flat)"
        };

        glUseProgram(0);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Set up 2D Orthographic projection for UI
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // 1. Full-screen Dark Overlay
        glColor4f(0.0f, 0.0f, 0.0f, 0.65f);
        glBegin(GL_QUADS);
        glVertex2i(0, 0);                 glVertex2i(screenWidth, 0);
        glVertex2i(screenWidth, screenHeight); glVertex2i(0, screenHeight);
        glEnd();

        // 2. Main Panel Drop-Shadow
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
        glBegin(GL_QUADS);
        glVertex2i(panelX + 12, panelY - 12);           glVertex2i(panelX + panelW + 12, panelY - 12);
        glVertex2i(panelX + panelW + 12, panelY + panelH - 12); glVertex2i(panelX + 12, panelY + panelH - 12);
        glEnd();

        // 3. Main Panel Background
        glColor4f(0.08f, 0.12f, 0.18f, 0.95f); 
        glBegin(GL_QUADS);
        glVertex2i(panelX, panelY);                   
        glVertex2i(panelX + panelW, panelY);
        glVertex2i(panelX + panelW, panelY + panelH); 
        glVertex2i(panelX, panelY + panelH);
        glEnd();

        // 4. Main Panel Border
        glColor4f(0.3f, 0.6f, 0.9f, 0.6f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2i(panelX, panelY);           glVertex2i(panelX + panelW, panelY);
        glVertex2i(panelX + panelW, panelY + panelH); glVertex2i(panelX, panelY + panelH);
        glEnd();
        glLineWidth(1.0f);

        // 5. Draw the Buttons
        for (int i = 0; i < 5; i++) {
            int by = firstButtonY - i * (buttonHeight + buttonGap);
            
            // Button Drop Shadow
            glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
            glBegin(GL_QUADS);
            glVertex2i(buttonX + 4, by - 4);                  
            glVertex2i(buttonX + buttonWidth + 4, by - 4);
            glVertex2i(buttonX + buttonWidth + 4, by + buttonHeight - 4); 
            glVertex2i(buttonX + 4, by + buttonHeight - 4);
            glEnd();

            // Button Background
            glColor4f(0.16f, 0.35f, 0.55f, 0.9f);
            glBegin(GL_QUADS);
            glVertex2i(buttonX, by);                  
            glVertex2i(buttonX + buttonWidth, by);
            glVertex2i(buttonX + buttonWidth, by + buttonHeight); 
            glVertex2i(buttonX, by + buttonHeight);
            glEnd();

            // Button Border Highlight
            glColor4f(0.5f, 0.8f, 1.0f, 0.8f);
            glBegin(GL_LINE_LOOP);
            glVertex2i(buttonX, by);                  
            glVertex2i(buttonX + buttonWidth, by);
            glVertex2i(buttonX + buttonWidth, by + buttonHeight); 
            glVertex2i(buttonX, by + buttonHeight);
            glEnd();
        }

        // Restore Matrices
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        // 6. Draw the text layers
        drawTextOnScreen(screenWidth / 2 - 85, screenHeight - 140, "TANK ASSIGNMENT");
        drawTextOnScreen(screenWidth / 2 - 140, screenHeight - 180, "Select a Level (Press 1-5 or click):");

        for (int i = 0; i < 5; i++) {
            int by = firstButtonY - i * (buttonHeight + buttonGap);
            drawTextOnScreen(buttonX + 20, by + 16, labels[i]);
        }

        drawTextOnScreen(screenWidth / 2 - 70, 30, "Press ESC to Quit");

        recordScoreIfNeeded();
        drawScoreTable(30, screenHeight - 60);
        drawScoreHistory(screenWidth - 360, screenHeight - 60, 5);
    
    } else if (gameState == GAME_OVER) {
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 200, timeExpired ? "TIME'S UP!" : "GAME OVER");
        drawTextOnScreen(screenWidth / 2 - 120, screenHeight - 250, "Press R to Restart or ESC to Quit");
        recordScoreIfNeeded();
        drawScoreTable(30, 250);
        drawScoreHistory(screenWidth - 360, 300, 5);
        
    } else if (gameState == PLAYING) {
        // Draw the 3D world first, then overlay the HUD and labels on top.
        glUseProgram(shaderProgramID);
        ProjectionMatrix.perspective(90, (float)screenWidth / (float)screenHeight, 0.0001, 100.0);
        glUniformMatrix4fv(ProjectionUniformLocation, 1, false, ProjectionMatrix.getPtr());

        Matrix4x4 m = getViewMatrix();
        Vector3f sunInView = transformPoint(m, sunLightPosition);
        glUniform3f(LightPositionUniformLocation, sunInView.x, sunInView.y, sunInView.z);
        glUniform4f(AmbientUniformLocation, 0.25f, 0.28f, 0.35f, 1.0f); // More realistic sky-blue ambient
        glUniform4f(SpecularUniformLocation, 0.6f, 0.6f, 0.6f, 1.0f);
        glUniform1f(SpecularPowerUniformLocation, 64.0f); // Sharper highlights
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(TextureMapUniformLocation, 0);

        for(size_t i = 0; i < levelMap.cubePositions.size(); i++) {
            Matrix4x4 model = m;
            model.translate(levelMap.cubePositions[i].x, levelMap.cubePositions[i].y, levelMap.cubePositions[i].z); 
            glUniformMatrix4fv(MVMatrixUniformLocation, 1, false, model.getPtr());
            mesh.Draw(vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
        }

        for (Coin* c : coins) { c->updateSpin(); c->draw(m, MVMatrixUniformLocation, vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute); }
        for (DrawObj* obj : gameObjects) obj->draw(m, MVMatrixUniformLocation, vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
        for (ActiveBullet ab : activeBullets) ab.obj->draw(m, MVMatrixUniformLocation, vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);

        drawGroundShadows(m, ProjectionMatrix);
        drawWorldSun(m, ProjectionMatrix);
        drawAimPrediction(m, ProjectionMatrix);

        // HUD shows live mission state and progression information.
        drawTextOnScreen(20, screenHeight - 40, "Coins: " + std::to_string(score) + " / " + std::to_string(totalCoins));
        drawTextOnScreen(20, screenHeight - 70, "HP: " + std::to_string(myPlayer->HP));
        drawTextOnScreen(20, screenHeight - 100, "Time: " + std::to_string(remainingTimeSeconds));
        drawTextOnScreen(20, screenHeight - 160, "Level: " + std::to_string(currentLevel));
        drawTextOnScreen(20, screenHeight - 190, "Enemies Destroyed: " + std::to_string(enemydistroy));
        
        int playerLevel = (score / 3) + 1; 
        drawTextOnScreen(20, screenHeight - 130, "Tank Level: " + std::to_string(playerLevel));
        
        drawTextOnScreen(screenWidth - 220, screenHeight - 40, "Enemies: " + std::to_string(activeEnemies.size()));

        for (ActiveEnemy ae : activeEnemies) {
            Vector3f labelPos = ae.obj->getPosition();
            labelPos.y += 3.0f;
            int lx = 0, ly = 0;
            if (worldToScreen(labelPos, m, ProjectionMatrix, lx, ly)) {
                drawTextOnScreen(lx - 20, ly, "HP: " + std::to_string(std::max(0, ae.obj->HP)));
            }
        }

        // Win when the mission objective is complete or the enemy force is cleared.
        if ((score >= totalCoins && totalCoins > 0) || activeEnemies.empty()) {
            gameState = WIN;
        }

        drawTextOnScreen(20, 20, "WASD Move | Arrow L/R Turret | Arrow Up Shoot | C Camera | M Menu");
        glUseProgram(0);
        
    } else if (gameState == WIN) {
        drawTextOnScreen(screenWidth / 2 - 50, screenHeight - 180, "YOU WIN!");
        
        // --- NEW: DISPLAY THE REASON FOR WINNING ---
        std::string winReason = "";
        if (activeEnemies.empty()) {
            winReason = "Victory: All Enemies Defeated!";
        } else if (score >= totalCoins && totalCoins > 0) {
            winReason = "Victory: All Coins Collected!";
        }
        
        // Center the dynamic reason text based on its length
        drawTextOnScreen(screenWidth / 2 - 130, screenHeight - 230, winReason);
        drawTextOnScreen(screenWidth / 2 - 150, screenHeight - 270, "Press R to Restart or ESC to Quit");
        
        recordScoreIfNeeded();
        drawScoreTable(30, 250);
        drawScoreHistory(screenWidth - 360, 300, 5);
    }
    glutSwapBuffers();
}

// Handle regular key input.
void Game::keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0);
    if (gameState == MENU) {
        if (key >= '1' && key <= '5') startGame(key - '0');
    } else if (gameState == PLAYING) {
        if (key == 'm' || key == 'M') { playerVelocity = 0.0f; gameState = MENU; }
        if (key == 'c' || key == 'C') changeCamera();
        keyStates[key] = true;
    } else if (gameState == GAME_OVER || gameState == WIN) {
        if (key == 'r' || key == 'R') gameState = MENU;
    }
}
// Clear key state on release.
void Game::keyUp(unsigned char key, int x, int y) { keyStates[key] = false; }
// Handle special keys (arrows, etc.).
void Game::specialKeyboard(int key, int x, int y) { if (key >= 0 && key < 256) specialKeyStates[key] = true; }
// Clear special key state on release.
void Game::specialKeyUp(int key, int x, int y) { if (key >= 0 && key < 256) specialKeyStates[key] = false; }
// Forward mouse presses to the camera controller.
void Game::mouse(int button, int state, int x, int y) {
    if (gameState == PLAYING) {
        cameraManip.handleMouse(button, state, x, y);
        return;
    }

    if (gameState == MENU && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        const int buttonX = (screenWidth / 2) - 190;
        const int buttonWidth = 380;
        const int buttonHeight = 48;
        const int buttonGap = 16;
        const int firstButtonY = screenHeight - 360;
        const int clickY = screenHeight - y;

        if (x >= buttonX && x <= buttonX + buttonWidth) {
            for (int i = 0; i < 5; i++) {
                int by = firstButtonY - i * (buttonHeight + buttonGap);
                if (clickY >= by && clickY <= by + buttonHeight) {
                    startGame(i + 1);
                    return;
                }
            }
        }
    }
}
// Forward mouse motion to the camera controller.
void Game::motion(int x, int y) { if (gameState == PLAYING) cameraManip.handleMouseMotion(x, y); }
// Handle window resize.
void Game::reshape(int w, int h) { screenWidth = w; screenHeight = h; glViewport(0, 0, w, h); }
// Cycle through camera modes.
void Game::changeCamera() { cameraView = static_cast<CameraView>((cameraView + 1) % 3); }

// Apply movement and firing based on current key states.
void Game::handleKeys(float deltaSeconds) {
    Vector3f pos = myPlayer->getPosition();
    float rot = myPlayer->getRotation();
    if(keyStates['a']) rot += GameConfig::TURN_SPEED;
    if(keyStates['d']) rot -= GameConfig::TURN_SPEED;
    
    float rad = rot * (static_cast<float>(M_PI) / 180.0f);
    if (keyStates['w']) playerVelocity += GameConfig::PLAYER_ACCEL * deltaSeconds;
    else if (keyStates['s']) playerVelocity -= GameConfig::PLAYER_ACCEL * deltaSeconds;
    else {
        playerVelocity *= std::pow(GameConfig::PLAYER_DRAG, deltaSeconds * 60.0f);
        if (fabs(playerVelocity) < 0.0005f) playerVelocity = 0.0f;
    }
    
    playerVelocity = std::max(GameConfig::PLAYER_REVERSE, std::min(GameConfig::PLAYER_MAX_SPEED, playerVelocity));
    
    Vector3f nextPos = pos;
    nextPos.x = std::max(0.0f, std::min(GameConfig::MAP_BOUND, pos.x + playerVelocity * deltaSeconds * static_cast<float>(std::sin(rad))));
    nextPos.z = std::max(0.0f, std::min(GameConfig::MAP_BOUND, pos.z + playerVelocity * deltaSeconds * static_cast<float>(std::cos(rad))));

    if (!collidesWithEnemyAt(nextPos, 2.0f)) pos = nextPos;
    else playerVelocity = 0.0f;

    if(specialKeyStates[GLUT_KEY_LEFT]) myPlayer->rotateTurret(2.0f);
    if(specialKeyStates[GLUT_KEY_RIGHT]) myPlayer->rotateTurret(-2.0f);

if(specialKeyStates[GLUT_KEY_UP]){
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        
        // --- LEVEL UP LOGIC (Shooting Rate Increase) ---
        int playerLevel = score / 3;
        // Decrease cooldown by 75ms per level. 
        // We use std::max so the cooldown never drops below 100ms (preventing infinite fire rate)
        int dynamicCooldown = std::max(100, GameConfig::PLAYER_SHOOT_COOLDOWN - (playerLevel * 75));

        if (currentTime - playerLastShootTime >= dynamicCooldown) {
            float turretRad = (rot + myPlayer->getTurretRotation()) * (M_PI / 180.0f);
            bullet* b = new bullet(getProjectileSpawnPosition(pos, turretRad), &ball, ballTexture);
            b->setrad(turretRad);
            activeBullets.push_back({b, PLAYER_BULLET});
            playerLastShootTime = currentTime;
        }
    }
    myPlayer->setPosition(pos);
    myPlayer->setRotation(rot);
}
// Apply gravity and clamp to walkable tiles.
void Game::applyGravity(float deltaSeconds) {
    Vector3f pos = myPlayer->getPosition();
    int gX = round(pos.x / GameConfig::TILE_SIZE), gZ = round(pos.z / GameConfig::TILE_SIZE);
    if (gX >= 0 && gX < 40 && gZ >= 0 && gZ < 40 && levelMap.grid[gX][gZ] == 1) pos.y = 0.0f;
    else pos.y -= GameConfig::GRAVITY * deltaSeconds;
    myPlayer->setPosition(pos);
}

// Collect coins within pickup radius.
// Collect coins within pickup radius.
void Game::collectCoins(float px, float pz) {
    for(size_t i = 0; i < coins.size(); i++) {
        float dx = px - coins[i]->getPosition().x, dz = pz - coins[i]->getPosition().z;
        if(sqrt(dx*dx + dz*dz) < GameConfig::COLLISION_RADIUS) {
            score += 1;
            
            // --- LEVEL UP LOGIC (HP Increase) ---
            // Every 3 coins collected, increase HP by 20
            if (score > 0 && score % 3 == 0) {
                myPlayer->HP += 20;
            }
            
            delete coins[i];
            coins.erase(coins.begin() + i);
            i--;
        }
    }
}

// Check if a position overlaps any enemy.
bool Game::collidesWithEnemyAt(const Vector3f& pos, float radius) {
    for (ActiveEnemy ae : activeEnemies) {
        float dx = pos.x - ae.obj->getPosition().x, dz = pos.z - ae.obj->getPosition().z;
        if (sqrt(dx*dx + dz*dz) < radius) return true;
    }
    return false;
}

// Spawn enemies on valid tiles with spacing.
void Game::generateEnemyPositions(int numEnemies) {
    for (int i = 0; i < numEnemies; i++) {
        int gx = 0, gz = 0;
        bool valid = false;
        for (int attempts = 0; attempts < 100 && !valid; attempts++) {
            gx = std::rand() % 40; gz = std::rand() % 40;
            if (levelMap.grid[gx][gz] != 1) continue;

            float wx = gx * GameConfig::TILE_SIZE;
            float wz = gz * GameConfig::TILE_SIZE;
            Vector3f ppos = myPlayer ? myPlayer->getPosition() : Vector3f(0,0,0);
            float pdx = wx - ppos.x, pdz = wz - ppos.z;
            if (sqrt(pdx*pdx + pdz*pdz) < GameConfig::ENEMY_SPAWN_DIST) continue;

            valid = true;
            for (const ActiveEnemy &ae : activeEnemies) {
                float dx = wx - ae.obj->getPosition().x;
                float dz = wz - ae.obj->getPosition().z;
                if (sqrt(dx*dx + dz*dz) < GameConfig::ENEMY_SPAWN_DIST) { valid = false; break; }
            }
        }
        if (valid) {
            float wx = gx * GameConfig::TILE_SIZE;
            float wz = gz * GameConfig::TILE_SIZE;
            DrawEnemy* e = new DrawEnemy(Vector3f(wx, 0, wz), &playerBodyMesh, &turretMesh, &frontWheelMesh, &backWheelMesh, enemyTexture);
            activeEnemies.push_back({e, 0});
            gameObjects.push_back(e);
        }
    }
}

// Spawn coins on reachable tiles.
void Game::initCoins(int numCoins) {
    std::vector<std::pair<int, int>> reachable;
    bool visited[40][40] = {false};
    std::queue<std::pair<int, int>> q;
    
    if (levelMap.grid[1][1] == 1) {
        q.push({1, 1}); visited[1][1] = true;
        int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
        while (!q.empty()) {
            auto cell = q.front(); q.pop();
            int x = cell.first, z = cell.second;
            if (!(x==1&&z==1) && !(x==38&&z==38)) reachable.push_back(cell);
            for (auto d : dirs) {
                int nx = x + d[0], nz = z + d[1];
                if (nx>=0 && nx<40 && nz>=0 && nz<40 && !visited[nx][nz] && levelMap.grid[nx][nz]==1) {
                    visited[nx][nz] = true; q.push({nx, nz});
                }
            }
        }
    }
    for (size_t i = reachable.size(); i > 1; --i) std::swap(reachable[i - 1], reachable[std::rand() % i]);
    for (int i = 0; i < std::min(numCoins, (int)reachable.size()); i++) {
        coins.push_back(new Coin(Vector3f(reachable[i].first * GameConfig::TILE_SIZE, GameConfig::TILE_SIZE, reachable[i].second * GameConfig::TILE_SIZE), &coinsMesh, coinTexture));
    }
}

// Find the next A* step toward the target.
struct AStarNode {
    Point2D pos;
    float g, h;
    Point2D parent;
    bool operator>(const AStarNode& other) const { return (g + h) > (other.g + other.h); }
};

Point2D Game::getNextStepAStar(Point2D start, Point2D goal) {
    if (start == goal) return start;

    auto heuristic = [](Point2D a, Point2D b) {
        return (float)(std::abs(a.x - b.x) + std::abs(a.z - b.z)); // Manhattan distance
    };

    // Mark current enemy positions to avoid congestion
    bool isEnemyOccupied[40][40];
    for(int i=0; i<40; i++) for(int j=0; j<40; j++) isEnemyOccupied[i][j] = false;
    for(auto& e : activeEnemies) {
        int ex = (int)((e.obj->getPosition().x + GameConfig::MAP_BOUND) / GameConfig::TILE_SIZE);
        int ez = (int)((e.obj->getPosition().z + GameConfig::MAP_BOUND) / GameConfig::TILE_SIZE);
        if(ex >= 0 && ex < 40 && ez >= 0 && ez < 40) isEnemyOccupied[ex][ez] = true;
    }

    auto runAStar = [&](bool ignoreEnemies) -> Point2D {
        std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;
        float gScore[40][40];
        Point2D parent[40][40];
        for(int i=0; i<40; i++) for(int j=0; j<40; j++) { gScore[i][j] = 1e9f; parent[i][j] = {-1, -1}; }

        gScore[start.x][start.z] = 0;
        openSet.push({start, 0, heuristic(start, goal), {-1, -1}});

        int dirs[4][2] = {{0,1}, {1,0}, {0,-1}, {-1,0}};
        bool found = false;

        while(!openSet.empty()) {
            AStarNode current = openSet.top(); openSet.pop();
            if(current.pos == goal) { found = true; break; }

            for(auto d : dirs) {
                int nx = current.pos.x + d[0], nz = current.pos.z + d[1];
                if(nx >= 0 && nx < 40 && nz >= 0 && nz < 40 && levelMap.grid[nx][nz] == 1) {
                    if(!ignoreEnemies && isEnemyOccupied[nx][nz] && (nx != goal.x || nz != goal.z)) continue;

                    float tentativeG = gScore[current.pos.x][current.pos.z] + 1.0f;
                    if(tentativeG < gScore[nx][nz]) {
                        parent[nx][nz] = current.pos;
                        gScore[nx][nz] = tentativeG;
                        openSet.push({{nx, nz}, tentativeG, heuristic({nx, nz}, goal), current.pos});
                    }
                }
            }
        }

        if(found) {
            Point2D step = goal;
            while(parent[step.x][step.z].x != -1 && (parent[step.x][step.z].x != start.x || parent[step.x][step.z].z != start.z)) {
                step = parent[step.x][step.z];
            }
            return step;
        }
        return start;
    };

    Point2D next = runAStar(false);
    if(next == start) next = runAStar(true);
    return next;
}

// Compute projectile spawn offset from the turret.
Vector3f Game::getProjectileSpawnPosition(const Vector3f& basePos, float turretRad) {
    return Vector3f(basePos.x + (std::sin(turretRad) * 2.0f), basePos.y + 1.05f, basePos.z + (std::cos(turretRad) * 2.0f));
}

// Predict where a projectile will land on the ground.
Vector3f Game::predictProjectileLandingPoint(const Vector3f& spawnPos, float turretRad) {
    float vD = spawnPos.y, disc = pow(bullet::PROJECTILE_START_VERTICAL_VELOCITY, 2) + (2.0f * bullet::PROJECTILE_GRAVITY * vD);
    if (vD <= 0.0f || disc < 0.0f) return Vector3f(spawnPos.x, 0, spawnPos.z);
    float t = (bullet::PROJECTILE_START_VERTICAL_VELOCITY + std::sqrt(disc)) / bullet::PROJECTILE_GRAVITY;
    return Vector3f(spawnPos.x + bullet::PROJECTILE_SPEED * std::sin(turretRad) * t, 0, spawnPos.z + bullet::PROJECTILE_SPEED * std::cos(turretRad) * t);
}

// Build the camera view matrix for the current mode.
Matrix4x4 Game::getViewMatrix() {
    Matrix4x4 view; view.toIdentity();
    Vector3f pos = myPlayer->getPosition();
    float r = myPlayer->getRotation() * (M_PI / 180.0f);
    Vector3f fwd(sin(r), 0, cos(r)), right(cos(r), 0, -sin(r));
    if (cameraView == FIRST_PERSON) view.lookAt(pos + (fwd * 1.5f) + (right * 1.3f) + Vector3f(0, 2.3f, 0), pos + (fwd * 15.0f), Vector3f(0, 1, 0));
    // TOP_DOWN: use a non-parallel up vector; (0,1,0) is colinear with the view dir here.
    else if (cameraView == TOP_DOWN) {
        Vector3f eye = pos + Vector3f(0.0f, 40.0f, 0.0f);
        view.lookAt(eye, pos, Vector3f(0.0f, 0.0f, -1.0f));
    }
    else {
        float cRad = (myPlayer->getRotation() + myPlayer->getTurretRotation()) * (M_PI / 180.0f);
        Vector3f cFwd(sin(cRad), 0, cos(cRad)), cRight(cos(cRad), 0, -sin(cRad));
        view.lookAt(pos + (cRight * -1.5f) + Vector3f(0, 7.0f, 0) - (cFwd * 8.0f), pos + Vector3f(0, 2.5f, 0) + (cFwd * 25.0f), Vector3f(0, 1, 0));
    }
    return view;
}

// Draw UI text at pixel coordinates.
void Game::drawTextOnScreen(int x, int y, const std::string& text) {
    glUseProgram(0); glDisable(GL_DEPTH_TEST); glColor3f(1, 1, 1); glWindowPos2i(x, y);
    for (char c : text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    glEnable(GL_DEPTH_TEST);
}

// Draw a filled 2D ellipse on the overlay.
void Game::drawOverlayEllipse(float cX, float cY, float rX, float rY, float r, float g, float b, float a) {
    glColor4f(r, g, b, a); glBegin(GL_TRIANGLE_FAN); glVertex2f(cX, cY);
    for (int i = 0; i <= 40; ++i) glVertex2f(cX + cosf((i / 40.0f) * 2.0f * M_PI) * rX, cY + sinf((i / 40.0f) * 2.0f * M_PI) * rY);
    glEnd();
}

// Project a 3D point into 2D screen space.
bool Game::worldToScreen(const Vector3f& w, const Matrix4x4& v, const Matrix4x4& p, int& sx, int& sy) {
    const float* vm = v.getPtr(), *pm = p.getPtr();
    float vx = vm[0]*w.x + vm[4]*w.y + vm[8]*w.z + vm[12], vy = vm[1]*w.x + vm[5]*w.y + vm[9]*w.z + vm[13], vz = vm[2]*w.x + vm[6]*w.y + vm[10]*w.z + vm[14], vw = vm[3]*w.x + vm[7]*w.y + vm[11]*w.z + vm[15];
    float cx = pm[0]*vx + pm[4]*vy + pm[8]*vz + pm[12]*vw, cy = pm[1]*vx + pm[5]*vy + pm[9]*vz + pm[13]*vw, cz = pm[2]*vx + pm[6]*vy + pm[10]*vz + pm[14]*vw, cw = pm[3]*vx + pm[7]*vy + pm[11]*vz + pm[15]*vw;
    if (cw <= 0.0f) return false;
    float nx = cx/cw, ny = cy/cw, nz = cz/cw;
    if (nx<-1.0f || nx>1.0f || ny<-1.0f || ny>1.0f || nz<-1.0f || nz>1.0f) return false;
    sx = (nx*0.5f + 0.5f)*screenWidth; sy = (ny*0.5f + 0.5f)*screenHeight; return true;
}

// Transform a point by a matrix (no perspective divide).
Vector3f Game::transformPoint(const Matrix4x4& matrix, const Vector3f& point) {
    const float* m = matrix.getPtr();
    return Vector3f(m[0]*point.x + m[4]*point.y + m[8]*point.z + m[12], m[1]*point.x + m[5]*point.y + m[9]*point.z + m[13], m[2]*point.x + m[6]*point.y + m[10]*point.z + m[14]);
}

void Game::recordScoreIfNeeded() {
    if (scoreRecorded) return;
    if (gameState != WIN && gameState != GAME_OVER) return;

    scoreHistory.push_back({currentLevel, score, std::max(0, myPlayer->HP), remainingTimeSeconds});

    std::sort(scoreHistory.begin(), scoreHistory.end(),
        [](const ScoreEntry& a, const ScoreEntry& b) {
            if (a.score != b.score) return a.score > b.score;
            return a.timeLeft > b.timeLeft;
        });

    if (scoreHistory.size() > 10) scoreHistory.resize(10);
    scoreRecorded = true;
}

void Game::drawScoreTable(int x, int y) {
    drawTextOnScreen(x, y, "=== RUN STATS ===");
    drawTextOnScreen(x, y - 30, "Level : " + std::to_string(currentLevel));
    drawTextOnScreen(x, y - 60, "Score : " + std::to_string(score));
    drawTextOnScreen(x, y - 90, "HP    : " + std::to_string(std::max(0, myPlayer->HP)));
    drawTextOnScreen(x, y - 120, "Time  : " + std::to_string(std::max(0, remainingTimeSeconds)));
}

void Game::drawScoreHistory(int x, int y, int maxRows) {
    drawTextOnScreen(x, y, "=== TOP RUNS ===");
    drawTextOnScreen(x, y - 30, "Lvl  Score  HP  Time");

    int rows = std::min(maxRows, static_cast<int>(scoreHistory.size()));
    for (int i = 0; i < rows; ++i) {
        const ScoreEntry& e = scoreHistory[i];
        std::ostringstream row;
        row << std::setw(3) << e.level << "  "
            << std::setw(5) << e.score << "  "
            << std::setw(3) << e.hp << "  "
            << std::setw(4) << e.timeLeft;
        drawTextOnScreen(x, y - 60 - (i * 25), row.str());
    }
}

// Update lighting and sky colors based on elapsed time.
void Game::updateDayNightCycle(int elapsedMs) {
    if (levelTimeLimitSeconds <= 0) return;

    constexpr float dayRatio = 0.7f;
    float p = std::fmod((float)elapsedMs / (levelTimeLimitSeconds * 1000.0f), 1.0f);
    float arc;
    
    if (p < dayRatio) {
        float pDay = p / dayRatio;
        arc = std::sin(pDay * M_PI); // 0.0 at horizon, 1.0 at zenith
    } else {
        arc = 0.0f; // Night
    }
    
    dayNightStrength = std::max(0.0f, arc);

    // Calculate a "Golden Hour" factor. Peaks when sun is low (arc around 0.2)
    float sunriseFactor = 0.0f;
    if (arc > 0.0f) {
        sunriseFactor = std::max(0.0f, 1.0f - (std::fabs(arc - 0.2f) * 3.5f));
    }

    sunLightPosition = Vector3f(76.0f + (4.0f - 76.0f)*p, 8.0f + (50.0f - 8.0f)*arc, 20.0f);
    
    ambient = Vector3f(0.03f + 0.12f*dayNightStrength, 0.03f + 0.11f*dayNightStrength, 0.05f + 0.13f*dayNightStrength);
    specularPower = 4.0f + 16.0f*dayNightStrength;

    // --- Dynamic Sky Colors ---
    // 1. Base transition (Night Blue to Day Blue)
    float hR = 0.06f + 0.46f*dayNightStrength;
    float hG = 0.08f + 0.72f*dayNightStrength;
    float hB = 0.14f + 0.84f*dayNightStrength;

    float zR = 0.02f + 0.10f*dayNightStrength;
    float zG = 0.04f + 0.41f*dayNightStrength;
    float zB = 0.10f + 0.80f*dayNightStrength;

    // 2. Add Sunrise/Sunset Burst (Oranges and Magentas)
    // Blend towards a fiery orange at the horizon
    hR = hR + (1.00f - hR) * sunriseFactor;
    hG = hG + (0.35f - hG) * sunriseFactor;
    hB = hB + (0.10f - hB) * sunriseFactor;

    // Blend towards a purplish tone at the zenith
    zR = zR + (0.50f - zR) * sunriseFactor;
    zG = zG + (0.20f - zG) * sunriseFactor;
    zB = zB + (0.40f - zB) * sunriseFactor;

    skyHorizonColor = Vector3f(hR, hG, hB);
    skyZenithColor = Vector3f(zR, zG, zB);
}

// Render the sky gradient background.
// Render the 3D Skybox background in Perspective Projection.
void Game::drawSunnyBackground() {
    glUseProgram(0); 
    
    // We turn OFF depth writing so the skybox is always drawn behind everything else
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); 

    // 1. Setup Perspective Projection (3D)
    glMatrixMode(GL_PROJECTION); 
    glPushMatrix(); 
    // Rebuild the perspective matrix just in case the window resized
    ProjectionMatrix.perspective(90.0f, (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    glLoadMatrixf(ProjectionMatrix.getPtr());
    
    // 2. Setup the View Matrix (The Skybox Trick)
    glMatrixMode(GL_MODELVIEW); 
    glPushMatrix(); 
    
    Matrix4x4 view = getViewMatrix();
    float* m = view.getPtr();
    // TRICK: Zero out the translation (X, Y, Z position) so the sky follows the camera perfectly!
    m[12] = 0.0f; m[13] = 0.0f; m[14] = 0.0f; 
    glLoadMatrixf(m);

    // 3. Draw a Massive 3D Sky Box
    float size = 50.0f; // How big the skybox is
    float horizonY = -size * 0.2f; // Push the horizon slightly down
    
    glBegin(GL_QUADS);
    // Top Face (All Zenith Color to seal the roof)
    glColor3f(skyZenithColor.x, skyZenithColor.y, skyZenithColor.z);
    glVertex3f(-size, size, -size);
    glVertex3f(-size, size, size);
    glVertex3f(size, size, size);
    glVertex3f(size, size, -size);

    // Bottom Face (All Horizon Color to seal the floor)
    glColor3f(skyHorizonColor.x, skyHorizonColor.y, skyHorizonColor.z);
    glVertex3f(-size, horizonY, size);
    glVertex3f(-size, horizonY, -size);
    glVertex3f(size, horizonY, -size);
    glVertex3f(size, horizonY, size);
    

    // Front Face
    glColor3f(skyZenithColor.x, skyZenithColor.y, skyZenithColor.z);
    glVertex3f(-size, size, -size);
    glVertex3f(size, size, -size);
    glColor3f(skyHorizonColor.x, skyHorizonColor.y, skyHorizonColor.z);
    glVertex3f(size, horizonY, -size);
    glVertex3f(-size, horizonY, -size);

    // Back Face
    glColor3f(skyZenithColor.x, skyZenithColor.y, skyZenithColor.z);
    glVertex3f(size, size, size);
    glVertex3f(-size, size, size);
    glColor3f(skyHorizonColor.x, skyHorizonColor.y, skyHorizonColor.z);
    glVertex3f(-size, horizonY, size);
    glVertex3f(size, horizonY, size);

    // Left Face
    glColor3f(skyZenithColor.x, skyZenithColor.y, skyZenithColor.z);
    glVertex3f(-size, size, size);
    glVertex3f(-size, size, -size);
    glColor3f(skyHorizonColor.x, skyHorizonColor.y, skyHorizonColor.z);
    glVertex3f(-size, horizonY, -size);
    glVertex3f(-size, horizonY, size);

    // Right Face
    glColor3f(skyZenithColor.x, skyZenithColor.y, skyZenithColor.z);
    glVertex3f(size, size, -size);
    glVertex3f(size, size, size);
    glColor3f(skyHorizonColor.x, skyHorizonColor.y, skyHorizonColor.z);
    glVertex3f(size, horizonY, size);
    glVertex3f(size, horizonY, -size);
    
    // Top Face (All Zenith Color to seal the roof)
    glColor3f(skyZenithColor.x, skyZenithColor.y, skyZenithColor.z);
    glVertex3f(-size, size, -size);
    glVertex3f(-size, size, size);
    glVertex3f(size, size, size);
    glVertex3f(size, size, -size);
    glEnd();

    // 4. Draw 3D Stars mapped to a sphere!
    if (dayNightStrength < 0.3f) { 
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        
        for (int i = 0; i < 200; i++) {
            // Spherical math to wrap the stars into a 3D dome
            float theta = (i * 123.456f); 
            float phi = fmod((i * 789.123f), M_PI / 2.0f); // Upper hemisphere only
            
            float r = 48.0f; // Push stars just inside the walls of the skybox
            float sx = r * sin(phi) * cos(theta);
            float sy = r * cos(phi);
            float sz = r * sin(phi) * sin(theta);
            
            float starAlpha = (0.3f - dayNightStrength) * 3.3f; 
            float twinkle = (sin(glutGet(GLUT_ELAPSED_TIME) * 0.005f + i) + 1.0f) * 0.5f;
            glColor4f(1.0f, 1.0f, 1.0f, starAlpha * twinkle);
            
            // Draw star in 3D space
            glVertex3f(sx, sy, sz); 
        }
        glEnd();
        glDisable(GL_BLEND);
    }

    // 5. Safely restore the matrices and depth buffer
    glPopMatrix(); 
    glMatrixMode(GL_PROJECTION); 
    glPopMatrix(); 
    glMatrixMode(GL_MODELVIEW); 
    
    glDepthMask(GL_TRUE); // Re-enable depth writing for your tanks and map
    glEnable(GL_DEPTH_TEST);
}
// Draw the sun in screen space.
void Game::drawWorldSun(const Matrix4x4& v, const Matrix4x4& p) {
    if (dayNightStrength <= 0.02f) return;
    int sx=0, sy=0; 
    if (!worldToScreen(sunLightPosition, v, p, sx, sy)) return;

    glUseProgram(0); 
    glDisable(GL_DEPTH_TEST); 
    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glMatrixMode(GL_PROJECTION); 
    glPushMatrix(); 
    glLoadIdentity(); 
    glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
    
    glMatrixMode(GL_MODELVIEW); 
    glPushMatrix(); 
    glLoadIdentity();

    // Base size
    float baseRad = std::max(16.0f, std::min(screenWidth, screenHeight) * 0.03f);
    
    // Sun gets larger and more orange when near the horizon (Golden Hour)
    float sunriseFactor = std::max(0.0f, 1.0f - (std::fabs(dayNightStrength - 0.2f) * 3.5f));
    float sunScale = 1.0f + (1.5f * sunriseFactor); // Sun is up to 2.5x bigger at sunrise
    
    float rad = baseRad * sunScale;
    
    // Sun color transitions from deep orange/red to bright yellow/white
    float sunR = 1.0f;
    float sunG = 0.98f - (0.50f * sunriseFactor); // Drops green to make it orange/red
    float sunB = 0.80f - (0.70f * sunriseFactor); // Drops blue
    
    float alpha = 0.08f + 0.18f * dayNightStrength;

    // Massive soft atmospheric glow (only visible near horizon)
    if (sunriseFactor > 0.0f) {
        drawOverlayEllipse(sx, sy, rad*4.0f, rad*4.0f, 1.0f, 0.4f, 0.1f, 0.15f * sunriseFactor);
        drawOverlayEllipse(sx, sy, rad*2.5f, rad*2.5f, 1.0f, 0.5f, 0.15f, 0.3f * sunriseFactor);
    }

    // Standard sun rings
    drawOverlayEllipse(sx, sy, rad*1.8f, rad*1.8f, sunR, sunG * 0.9f, sunB * 0.5f, alpha*0.6f);
    drawOverlayEllipse(sx, sy, rad*1.25f, rad*1.25f, sunR, sunG, sunB * 0.8f, alpha);
    drawOverlayEllipse(sx, sy, rad, rad, sunR, std::min(1.0f, sunG + 0.2f), std::min(1.0f, sunB + 0.2f), 0.95f * dayNightStrength);

    glPopMatrix(); 
    glMatrixMode(GL_PROJECTION); 
    glPopMatrix(); 
    glMatrixMode(GL_MODELVIEW); 
    glDisable(GL_BLEND); 
    glEnable(GL_DEPTH_TEST);
}

// Draw soft shadow blobs under objects.
void Game::drawGroundShadows(const Matrix4x4& v, const Matrix4x4& p) {
    glUseProgram(0); glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    auto drawShad = [&](const Vector3f& w, float rX, float rY, float a) {
        Vector3f g=w; g.y=0.05f; int sx=0, sy=0; if (!worldToScreen(g, v, p, sx, sy)) return;
        float dx=g.x-sunLightPosition.x, dz=g.z-sunLightPosition.z, l=std::max(0.001f, std::sqrt(dx*dx+dz*dz));
        drawOverlayEllipse(sx + (dx/l)*16.0f, sy + (dz/l)*8.0f, rX, rY, 0.08f, 0.08f, 0.10f, a*(0.25f + 0.75f*dayNightStrength));
    };
    drawShad(myPlayer->getPosition(), 22.0f, 10.0f, 0.28f);
    for (ActiveEnemy ae : activeEnemies) drawShad(ae.obj->getPosition(), 20.0f, 9.0f, 0.26f);
    for (Coin* c : coins) drawShad(c->getPosition(), 10.0f, 5.0f, 0.18f);
    for (ActiveBullet ab : activeBullets) drawShad(ab.obj->getPosition(), 5.0f, 3.0f, 0.16f);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

// Draw a landing marker for the current aim.
void Game::drawAimPrediction(const Matrix4x4& v, const Matrix4x4& p) {
    if (gameState != PLAYING || !myPlayer) return;
    float rad = (myPlayer->getRotation() + myPlayer->getTurretRotation()) * (M_PI / 180.0f);
    Vector3f land = predictProjectileLandingPoint(getProjectileSpawnPosition(myPlayer->getPosition(), rad), rad);
    int sx=0, sy=0; if (!worldToScreen(land, v, p, sx, sy)) return;
    glUseProgram(0); glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    drawOverlayEllipse(sx, sy, 14.0f, 14.0f, 1.0f, 0.35f, 0.15f, 0.35f);
    drawOverlayEllipse(sx, sy, 7.0f, 7.0f, 1.0f, 0.75f, 0.20f, 0.8f);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}