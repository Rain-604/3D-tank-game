#include "Game.h"

// 1. Create a global pointer for our wrapper functions
Game* myGameInstance = nullptr;

// 2. GLUT Callbacks (Wrappers)
void displayWrapper() { myGameInstance->display(); }
void timerWrapper(int value) {
    if (myGameInstance) {
        myGameInstance->timer(value);
    }
    glutTimerFunc(10, timerWrapper, 0);
}
void keyboardWrapper(unsigned char key, int x, int y) { myGameInstance->keyboard(key, x, y); }
void keyUpWrapper(unsigned char key, int x, int y) { myGameInstance->keyUp(key, x, y); }
void specialWrapper(int key, int x, int y) { myGameInstance->specialKeyboard(key, x, y); }
void specialUpWrapper(int key, int x, int y) { myGameInstance->specialKeyUp(key, x, y); }
void mouseWrapper(int button, int state, int x, int y) { myGameInstance->mouse(button, state, x, y); }
void motionWrapper(int x, int y) { myGameInstance->motion(x, y); }
void reshapeWrapper(int w, int h) { myGameInstance->reshape(w, h); }

// 3. Application Entry Point
int main(int argc, char** argv) {
    // Instantiate the Game class
    Game myGame;
    myGameInstance = &myGame;

    // Initialize the window and OpenGL context
    myGame.initGL(argc, argv);

    // Register all the GLUT wrappers
    glutDisplayFunc(displayWrapper);
    glutReshapeFunc(reshapeWrapper);
    glutKeyboardFunc(keyboardWrapper);
    glutKeyboardUpFunc(keyUpWrapper); 
    glutSpecialFunc(specialWrapper);
    glutSpecialUpFunc(specialUpWrapper);
    glutMouseFunc(mouseWrapper);
    glutPassiveMotionFunc(motionWrapper);
    glutMotionFunc(motionWrapper);
    glutTimerFunc(10, timerWrapper, 0);

    // Start the game loop
    myGame.run();

    return 0;
}