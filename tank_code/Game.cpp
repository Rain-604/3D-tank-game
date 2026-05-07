#include "Game.h"

// Initialize game state defaults and runtime variables.
Game::Game() {
    screenWidth = GameConfig::SCREEN_WIDTH;
    screenHeight = GameConfig::SCREEN_HEIGHT;
    gameState = MENU;
    cameraView = FIRST_PERSON;
    myPlayer = nullptr;
    shaderProgramID = 0;
    
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

    int width, height;
    char* data;
    Texture::LoadBMP(filename, width, height, data);

    if (makeRed) {
        int totalBytes = width * height * 3;
        for (int i = 0; i < totalBytes; i += 3) {
            char temp = data[i];
            data[i] = data[i+1];
            data[i+1] = temp;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    delete[] data;
}

// Reset state and spawn level content.
void Game::startGame(int level) {
    levelMap.loadLevel(level);

    for (Coin* c : coins) delete c;
    coins.clear();
    initCoins(level * 10);
    totalCoins = static_cast<int>(coins.size());

    for (ActiveBullet ab : activeBullets) delete ab.obj;
    activeBullets.clear();

    myPlayer->setPosition(Vector3f(2.0f, 0.0f, 2.0f));
    myPlayer->setRotation(0.0f);
    myPlayer->HP = 100;

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
    remainingTimeSeconds = std::max(0, levelTimeLimitSeconds - (elapsedMs / 1000));
    updateDayNightCycle(elapsedMs);

    if (remainingTimeSeconds <= 0 || myPlayer->getPosition().y < -10.0f) {
        timeExpired = (remainingTimeSeconds <= 0);
        playerVelocity = 0.0f;
        gameState = GAME_OVER;
        glutPostRedisplay();
        return;
    }

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
            Point2D nextStep = getNextStepBFS({eGridX, eGridZ}, {pGridX, pGridZ});
            Vector3f targetPos = {nextStep.x * GameConfig::TILE_SIZE, enemyPos.y, nextStep.z * GameConfig::TILE_SIZE};
            float stepDx = targetPos.x - enemyPos.x;
            float stepDz = targetPos.z - enemyPos.z;
            float stepDist = sqrt(stepDx*stepDx + stepDz*stepDz);
            
            if (stepDist > 0.1f) {
                float speed = 0.05f; 
                enemyPos.x += (stepDx / stepDist) * speed;
                enemyPos.z += (stepDz / stepDist) * speed;
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
            
            float turnSpeed = 1.0f;
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
        int gx = std::rand() % 40;
        int gz = std::rand() % 40;
        attempts++;

        if (!canBreakTile(gx, gz)) {
            continue;
        }
        if (!areAllCoinsReachableAfterBreak(gx, gz)) {
            continue;
        }

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
void Game::handleBulletCollisions() {
    for (size_t i = 0; i < activeBullets.size(); i++) {
        Vector3f bulletPos = activeBullets[i].obj->getPosition();
        
        if (activeBullets[i].owner == PLAYER_BULLET) {
            if (activeEnemies.empty()) gameState = WIN;
            for (size_t j = 0; j < activeEnemies.size(); j++) {
                DrawEnemy* e = activeEnemies[j].obj;
                Vector3f enemyPos = e->getPosition();
                float dx = bulletPos.x - enemyPos.x, dy = bulletPos.y - enemyPos.y, dz = bulletPos.z - enemyPos.z;
                if (sqrt(dx*dx + dy*dy + dz*dz) < GameConfig::COLLISION_RADIUS) {
                    e->HP -= 20;
                    if (e->HP <= 0) {
                        auto it = std::find(gameObjects.begin(), gameObjects.end(), e);
                        if (it != gameObjects.end()) gameObjects.erase(it);
                        delete e;
                        activeEnemies.erase(activeEnemies.begin() + j);
                    }
                    removeBulletAt(i); i--; break;
                }
            }
        } else {
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
void Game::display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawSunnyBackground();

    if (gameState == MENU) {
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 200, "TANK ASSIGNMENT");
        drawTextOnScreen(screenWidth / 2 - 120, screenHeight - 250, "Select a Level (Press 1-4):");
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 300, "1. Preset Level 1 (Holes)");
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 330, "2. Preset Level 2 (City Grid)");
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 360, "3. Preset Level 3 (Spiral)");
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 390, "4. Random Maze Generator");
        drawTextOnScreen(screenWidth / 2 - 120, screenHeight - 450, "Press ESC to Quit");
    } else if (gameState == GAME_OVER) {
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 200, timeExpired ? "TIME'S UP!" : "GAME OVER");
        drawTextOnScreen(screenWidth / 2 - 120, screenHeight - 250, "Press R to Restart or ESC to Quit");
    } else if (gameState == PLAYING) {
        glUseProgram(shaderProgramID);
        ProjectionMatrix.perspective(90, (float)screenWidth / (float)screenHeight, 0.0001, 100.0);
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

        drawTextOnScreen(20, screenHeight - 40, "Score: " + std::to_string(score));
        drawTextOnScreen(20, screenHeight - 70, "HP: " + std::to_string(myPlayer->HP));
        drawTextOnScreen(20, screenHeight - 100, "Time: " + std::to_string(remainingTimeSeconds));
        drawTextOnScreen(screenWidth - 220, screenHeight - 40, "Enemies: " + std::to_string(activeEnemies.size()));

        for (ActiveEnemy ae : activeEnemies) {
            Vector3f labelPos = ae.obj->getPosition();
            labelPos.y += 3.0f;
            int lx = 0, ly = 0;
            if (worldToScreen(labelPos, m, ProjectionMatrix, lx, ly)) {
                drawTextOnScreen(lx - 20, ly, "HP: " + std::to_string(std::max(0, ae.obj->HP)));
            }
        }

        if (score >= totalCoins && totalCoins > 0) gameState = WIN;
        drawTextOnScreen(20, 20, "WASD Move | Arrow L/R Turret | Arrow Up Shoot | C Camera | M Menu");
        glUseProgram(0);
    } else if (gameState == WIN) {
        drawTextOnScreen(screenWidth / 2 - 100, screenHeight - 200, "YOU WIN!");
        drawTextOnScreen(screenWidth / 2 - 120, screenHeight - 250, "Press R to Restart or ESC to Quit");
    }
    glutSwapBuffers();
}

// =======================================================================
// Remaining Helper Implementations (Kept practically identical to your original)
// =======================================================================

// Handle regular key input.
void Game::keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0);
    if (gameState == MENU) {
        if (key >= '1' && key <= '4') startGame(key - '0');
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
void Game::mouse(int button, int state, int x, int y) { if (gameState == PLAYING) cameraManip.handleMouse(button, state, x, y); }
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
    nextPos.x = std::max(0.0f, std::min(GameConfig::MAP_BOUND, pos.x + playerVelocity * deltaSeconds * std::sinf(rad)));
    nextPos.z = std::max(0.0f, std::min(GameConfig::MAP_BOUND, pos.z + playerVelocity * deltaSeconds * std::cosf(rad)));

    if (!collidesWithEnemyAt(nextPos, 2.0f)) pos = nextPos;
    else playerVelocity = 0.0f;

    if(specialKeyStates[GLUT_KEY_LEFT]) myPlayer->rotateTurret(2.0f);
    if(specialKeyStates[GLUT_KEY_RIGHT]) myPlayer->rotateTurret(-2.0f);

    if(specialKeyStates[GLUT_KEY_UP]){
        int currentTime = glutGet(GLUT_ELAPSED_TIME);
        if (currentTime - playerLastShootTime >= GameConfig::PLAYER_SHOOT_COOLDOWN) {
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
void Game::collectCoins(float px, float pz) {
    for(size_t i = 0; i < coins.size(); i++) {
        float dx = px - coins[i]->getPosition().x, dz = pz - coins[i]->getPosition().z;
        if(sqrt(dx*dx + dz*dz) < GameConfig::COLLISION_RADIUS) {
            score += 1;
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
        float x = 0, z = 0;
        bool valid = false;
        for (int attempts = 0; attempts < 100 && !valid; attempts++) {
            x = rand() % 40; z = rand() % 40;
            if (levelMap.grid[(int)x][(int)z] != 1 || (x < 25 && z < 25)) continue;
            valid = true;
            for (ActiveEnemy ae : activeEnemies) {
                if (sqrt(pow(x - ae.obj->getPosition().x, 2) + pow(z - ae.obj->getPosition().z, 2)) < GameConfig::ENEMY_SPAWN_DIST) valid = false;
            }
        }
        if (valid) {
            DrawEnemy* e = new DrawEnemy(Vector3f(x, 0, z), &playerBodyMesh, &turretMesh, &frontWheelMesh, &backWheelMesh, enemyTexture);
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

// Find the next BFS step toward the target.
Point2D Game::getNextStepBFS(Point2D start, Point2D goal) {
    if (start == goal) return start;
    int pX[40][40], pZ[40][40];
    for(int i=0; i<40; i++) for(int j=0; j<40; j++) { pX[i][j] = -1; pZ[i][j] = -1; }
    std::queue<Point2D> q; q.push(start); pX[start.x][start.z] = start.x; pZ[start.x][start.z] = start.z;
    int dirs[4][2] = {{0,1}, {1,0}, {0,-1}, {-1,0}};
    bool found = false;
    while (!q.empty() && !found) {
        Point2D c = q.front(); q.pop();
        if (c == goal) found = true;
        else for (auto d : dirs) {
            int nx = c.x + d[0], nz = c.z + d[1];
            if (nx>=0 && nx<40 && nz>=0 && nz<40 && levelMap.grid[nx][nz]==1 && pX[nx][nz]==-1) {
                pX[nx][nz] = c.x; pZ[nx][nz] = c.z; q.push({nx, nz});
            }
        }
    }
    if (!found) return start;
    Point2D step = goal;
    while (pX[step.x][step.z] != start.x || pZ[step.x][step.z] != start.z) {
        step = {pX[step.x][step.z], pZ[step.x][step.z]};
    }
    return step;
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

// Update lighting and sky colors based on elapsed time.
void Game::updateDayNightCycle(int elapsedMs) {
    if (levelTimeLimitSeconds <= 0) return;
    float p = std::fmod((float)elapsedMs / (levelTimeLimitSeconds * 1000.0f), 1.0f);
    float arc = std::sin(p * M_PI);
    dayNightStrength = std::max(0.0f, arc);
    sunLightPosition = Vector3f(76.0f + (4.0f - 76.0f)*p, 8.0f + (50.0f - 8.0f)*arc, 20.0f);
    ambient = Vector3f(0.03f + 0.12f*dayNightStrength, 0.03f + 0.11f*dayNightStrength, 0.05f + 0.13f*dayNightStrength);
    specularPower = 4.0f + 16.0f*dayNightStrength;
    skyHorizonColor = Vector3f(0.06f + 0.66f*dayNightStrength, 0.08f + 0.82f*dayNightStrength, 0.14f + 0.86f*dayNightStrength);
    skyZenithColor = Vector3f(0.02f + 0.38f*dayNightStrength, 0.04f + 0.56f*dayNightStrength, 0.10f + 0.90f*dayNightStrength);
}

// Render the sky gradient background.
void Game::drawSunnyBackground() {
    glUseProgram(0); glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glBegin(GL_QUADS); glColor3f(skyHorizonColor.x, skyHorizonColor.y, skyHorizonColor.z); glVertex2f(0, 0); glVertex2f(screenWidth, 0); glColor3f(skyZenithColor.x, skyZenithColor.y, skyZenithColor.z); glVertex2f(screenWidth, screenHeight); glVertex2f(0, screenHeight); glEnd();
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

// Draw the sun in screen space.
void Game::drawWorldSun(const Matrix4x4& v, const Matrix4x4& p) {
    if (dayNightStrength <= 0.02f) return;
    int sx=0, sy=0; if (!worldToScreen(sunLightPosition, v, p, sx, sy)) return;
    glUseProgram(0); glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    float rad = std::max(16.0f, std::min(screenWidth, screenHeight) * 0.03f), alpha = 0.08f + 0.18f*dayNightStrength;
    drawOverlayEllipse(sx, sy, rad*1.8f, rad*1.8f, 1, 0.90f, 0.45f, alpha*0.6f);
    drawOverlayEllipse(sx, sy, rad*1.25f, rad*1.25f, 1, 0.96f, 0.62f, alpha);
    drawOverlayEllipse(sx, sy, rad, rad, 1, 0.98f, 0.80f, 0.95f*dayNightStrength);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
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