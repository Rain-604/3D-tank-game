#ifndef MAP_H
#define MAP_H

#include <vector>
#include <cstdlib>
#include <ctime>
#include <utility> // For std::swap
#include <fstream>
#include <string>

// Using a struct for clarity instead of a raw int[3]
struct Vec3 {
    int x, y, z;
};

class Map {
public:
    int grid[40][40];
    std::vector<Vec3> cubePositions;
    std::vector<Vec3> targetPositions;

    Map() {
        // Default level
        loadLevel(4);
    }

    bool loadFromFile(const std::string& filePath) {
        std::ifstream file(filePath.c_str());
        if (!file.is_open()) {
            return false;
        }

        std::vector<int> values;
        int value = 0;
        while (file >> value) {
            values.push_back(value);
        }
        if (values.size() < 40 * 40) {
            return false;
        }

        size_t index = 0;
        for (int i = 0; i < 40; i++) {
            for (int j = 0; j < 40; j++) {
                int cell = values[index++];
                if (cell != 0 && cell != 1 && cell != 2) {
                    cell = 0;
                }
                grid[i][j] = cell;
            }
        }

        generatePositions();
        return true;
    }

    bool saveToFile(const std::string& filePath) const {
        std::ofstream file(filePath.c_str());
        if (!file.is_open()) {
            return false;
        }

        for (int i = 0; i < 40; i++) {
            for (int j = 0; j < 40; j++) {
                file << grid[i][j];
                if (j < 39) {
                    file << ' ';
                }
            }
            file << '\n';
        }
        return true;
    }

    const std::vector<Vec3>& getTargetPositions() const {
        return targetPositions;
    }

    void setTarget(int x, int z) {
        if (x >= 0 && x < 40 && z >= 0 && z < 40) {
            grid[x][z] = 2;
        }
    }

    void loadLevel(int level) {
        // Reset grid to 0 (abyss)
        for (int i = 0; i < 40; i++) {
            for (int j = 0; j < 40; j++) {
                grid[i][j] = 0;
            }
        }

        if (level == 1) {
            // Preset 1: Solid floor with some holes
            for (int i = 0; i < 40; i++) {
                for (int j = 0; j < 40; j++) {
                    grid[i][j] = 1;
                }
            }
            // Add square holes
            for (int i = 5; i < 35; i += 6) {
                for (int j = 5; j < 35; j += 6) {
                    grid[i][j] = 0;
                    grid[i+1][j] = 0;
                    grid[i][j+1] = 0;
                    grid[i+1][j+1] = 0;
                }
            }
        } else if (level == 2) {
            // Preset 2: Grid path (streets and blocks)
            for (int i = 0; i < 40; i++) {
                for (int j = 0; j < 40; j++) {
                    if (i % 4 == 0 || j % 4 == 0 || i % 4 == 1 || j % 4 == 1) {
                        grid[i][j] = 1;
                    }
                }
            }
        } else if (level == 3) {
            // Preset 3: Spiral Maze
            int minX = 0, maxX = 39, minZ = 0, maxZ = 39;
            while (minX <= maxX && minZ <= maxZ) {
                for (int i = minX; i <= maxX; i++) { grid[i][minZ] = 1; grid[i][minZ+1] = 1; }
                for (int i = minZ; i <= maxZ; i++) { grid[maxX][i] = 1; grid[maxX-1][i] = 1; }
                for (int i = maxX; i >= minX; i--) { grid[i][maxZ] = 1; grid[i][maxZ-1] = 1; }
                for (int i = maxZ; i >= minZ + 4; i--) { grid[minX][i] = 1; grid[minX+1][i] = 1; }
                minX += 4;
                maxX -= 4;
                minZ += 4;
                maxZ -= 4;
                if (minX <= maxX) { 
                    grid[minX-1][minZ] = 1; grid[minX-2][minZ] = 1; 
                    grid[minX-1][minZ+1] = 1; grid[minX-2][minZ+1] = 1; 
                } // Connect spirals
            }
        } else if (level == 4) {
            // Random Maze using DFS algorithm
            // 1. Generate a mini 19x19 maze first (pass 19 as the upper bound)
            generateMaze(1, 1, 19); 
            
            // 2. Scale the maze up 2x so paths and gaps are BOTH 2 blocks wide
            int tempGrid[40][40] = {0};
            for (int i = 0; i < 20; i++) {
                for (int j = 0; j < 20; j++) {
                    // If it's a path tile, turn it into a 2x2 chunk of path tiles!
                    if (grid[i][j] == 1) {
                        tempGrid[i * 2][j * 2] = 1;
                        tempGrid[i * 2 + 1][j * 2] = 1;
                        tempGrid[i * 2][j * 2 + 1] = 1;
                        tempGrid[i * 2 + 1][j * 2 + 1] = 1;
                    }
                }
            }
            
            // 3. Copy the nicely upscaled maze back to the main grid
            for (int i = 0; i < 40; i++) {
                for (int j = 0; j < 40; j++) {
                    grid[i][j] = tempGrid[i][j];
                }
            }
        }

        // Ensure safe spawn zones for Player (bottom-left) and Enemy (top-right)
        // I increased this to 6x6 so it seamlessly connects to your newly widened paths
        for(int i=0; i<6; i++) for(int j=0; j<6; j++) grid[i][j] = 1;
        for(int i=34; i<40; i++) for(int j=34; j<40; j++) grid[i][j] = 1;

        generatePositions();
    }

private:
    // Notice I added 'int maxBound' so we can restrict the generator to a smaller area
    void generateMaze(int cx, int cz, int maxBound = 39) {
        grid[cx][cz] = 1;
        
        // Directions: Up, Down, Left, Right (step by 2 to leave walls)
        int dirs[4][2] = {{0, 2}, {0, -2}, {2, 0}, {-2, 0}};
        
        // Shuffle directions
        for (int i = 0; i < 4; i++) {
            int r = i + (std::rand() % (4 - i));
            std::swap(dirs[i][0], dirs[r][0]);
            std::swap(dirs[i][1], dirs[r][1]);
        }
        
        for (int i = 0; i < 4; i++) {
            int nx = cx + dirs[i][0];
            int nz = cz + dirs[i][1];
            
            // Check boundaries using our new maxBound parameter!
            if (nx > 0 && nx < maxBound && nz > 0 && nz < maxBound && grid[nx][nz] == 0) {
                grid[cx + dirs[i][0]/2][cz + dirs[i][1]/2] = 1; // Carve path
                generateMaze(nx, nz, maxBound);
            }
        }
    }

    void generatePositions() {
        cubePositions.clear();
        targetPositions.clear();
        for (int i = 0; i < 40; i++) {      // Row (X)
            for (int j = 0; j < 40; j++) {  // Col (Z)
                if (grid[i][j] == 1 || grid[i][j] == 2) {
                    // Spacing of 2 units
                    cubePositions.push_back({ i * 2, 0, j * 2 });
                    if (grid[i][j] == 2) {
                        targetPositions.push_back({ i, 0, j });
                    }
                }
            }
        }
    }
};

#endif
