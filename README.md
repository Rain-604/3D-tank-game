# Tank Assignment Game

A 3D tank-based game built with OpenGL, where you control a tank to navigate terrain, collect coins, and battle AI-controlled enemies.

## Overview

This is an interactive 3D tank game featuring:
- **Player-controlled tank** with independent turret rotation
- **Multiple AI enemies** with intelligent pathfinding and aiming
- **Grid-based terrain** with destructible/varied landscape
- **Physics system** including gravity and collision detection
- **Coin collection** for scoring
- **Multiple difficulty levels** with increasing enemy count
- **Real-time 3D rendering** using OpenGL with textures

## Game Features

- **4 Preset Levels**: Generate different maps with increasing complexity
  - Level 1: Holes (learning map)
  - Level 2: City Grid (classic layout)
  - Level 3: Spiral (challenging terrain)
  - Level 4: Random Maze (procedurally generated)

- **Tower Defense Elements**:
  - Enemies spawn in corners
  - More enemies appear on higher difficulty levels
  - Enemies use BFS (Breadth-First Search) pathfinding

- **Combat System**:
  - Shoot bullets with cooldown mechanics
  - Smooth turret aiming for both player and enemies
  - Collision detection and physics-based movement

- **Score Tracking**:
  - Collect coins scattered throughout the map
  - Score increases by 1 per coin
  - Score displayed in-game

## Controls

### Main Menu
- **1-4**: Select difficulty level (determines number of enemies)
- **ESC**: Quit

### In-Game
- **W**: Move forward
- **S**: Move backward
- **A**: Rotate tank left
- **D**: Rotate tank right
- **← / →** (Left/Right Arrow): Rotate turret
- **↑** (Up Arrow): Shoot
- **M**: Return to menu
- **Mouse**: Control camera view (orbit around tank)

## Project Structure

```
assignment_tank/
├── tank_assignment/
│   ├── main.cpp              # Main game loop and logic
│   ├── Makefile              # Build configuration
│   ├── shader.vert           # Vertex shader
│   ├── shader.frag           # Fragment shader
│   ├── qmake.pro             # Qt project file
│   ├── drawBullet.h          # Bullet rendering
│   └── build/                # Compiled binaries
├── common/
│   ├── DrawPlayer.h          # Player tank class
│   ├── DrawEnemy.h           # Enemy tank class
│   ├── Coins.h               # Coin collection system
│   ├── drawObj.h             # Base drawable object
│   ├── map.h                 # Map generation and grid
│   ├── Shader.h/cpp          # Shader utilities
│   ├── Mesh.h/cpp            # 3D mesh loading/rendering
│   ├── Texture.h/cpp         # Texture loading
│   ├── Matrix.h/cpp          # Matrix math
│   ├── Vector.h/cpp          # Vector math
│   └── SphericalCameraManipulator.h/cpp  # Camera controls
└── models/                   # 3D model and texture files
    ├── chassis.obj           # Player body
    ├── turret.obj            # Tank turret
    ├── front_wheel.obj       # Front wheel
    ├── back_wheel.obj        # Back wheel
    ├── cube.obj              # Terrain cube
    ├── coin.obj              # Coin model
    ├── ball.obj              # Bullet model
    ├── hamvee.bmp            # Tank textures
    ├── coin.bmp              # Coin texture
    └── ball.bmp              # Bullet texture
```

## Building

### Prerequisites
- OpenGL/GLEW
- GLUT (FreeGLUT)
- C++ compiler (GCC/Clang/MSVC)
- Make or Qt Creator

### Linux/Mac
```bash
cd tank_assignment
make
```

### Windows (using Qt)
```bash
qmake qmake.pro
make
```

## Running

```bash
./TankAssignment
```

Or from Debug/Release folder:
```bash
./debug/TankAssignment
./release/TankAssignment
```

## Game Mechanics

### Terrain & Movement
- The game world is a 40×40 grid with 2-unit spacing
- Tank can only move on valid terrain tiles (marked as 1)
- Gravity pulls the tank down if it leaves a valid tile
- Tank rotates independently from turret for strategic aiming

### AI Enemies
- Use pathfinding (BFS algorithm) to navigate terrain toward player
- When close enough, they smoothly aim their turret
- Automatically fire when turret is pointed at player
- Respect cooldown timers to prevent rapid fire

### Coins
- Spawn randomly across valid terrain tiles
- Collect by moving the tank over them
- Award 1 point each
- New coins spawn on level transitions

### Combat
- Bullets spawn in front of turret with offset
- Travel in a straight line until out of bounds
- Use physics-based movement
- Both player and enemies have shoot cooldown (500ms default)

## Game State Management

- **MENU State**: Display level selection screen
- **PLAYING State**: Active gameplay with rendering and physics

## Performance Notes

- Bullets are destroyed when they travel >100 units for memory efficiency
- Efficient grid-based collision detection
- Smooth camera tracking around player tank
- Real-time 60+ FPS rendering

## Customization

Key game constants in `main.cpp`:
- `playerSpeed = 0.1f` - Player movement speed
- `enemySpeed = 0.025f` - Enemy movement speed
- `rotationSpeed = 2.0f` - Tank rotation speed
- `gravity = 0.1f` - Gravity strength
- `shootCooldown = 500` - Time between shots (milliseconds)

## Technical Details

- **Rendering**: Modern OpenGL with shader pipeline
- **Lighting**: Phong lighting model with ambient, diffuse, and specular components
- **Texturing**: BMP texture support with optional color channel manipulation (texture remapping for enemies)
- **Math**: Custom vector and matrix libraries for transformations
- **Camera**: Spherical camera manipulator for smooth orbiting views

## Credits

Assignment-based project featuring:
- OpenGL graphics rendering
- GLUT windowing and input
- Custom math libraries
- BFS pathfinding algorithm
