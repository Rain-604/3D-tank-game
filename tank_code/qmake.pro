TEMPLATE = app

# Executable Name
TARGET = TankAssignment
CONFIG += debug

# Force .exe extension on Windows
win32 {
    TARGET = TankAssignment
}

# Destination
DESTDIR = ./
OBJECTS_DIR = ./build/

# Headers
HEADERS += ../common/Shader.h \
           ../common/Vector.h \
           ../common/Matrix.h \
           ../common/Mesh.h \
           ../common/Texture.h \
           ../common/SphericalCameraManipulator.h \
           ../common/map.h \
           ../common/DrawObj.h \
           ../common/DrawPlayer.h \
           ../common/DrawEnemy.h \
           ../common/drawBullet.h \
           ../common/Coins.h \
           Game.h \
           drawBullet.h \


# Sources
SOURCES += main.cpp \
           Game.cpp \
           ../common/Shader.cpp \
           ../common/Vector.cpp \
           ../common/Matrix.cpp \
           ../common/Mesh.cpp \
           ../common/Texture.cpp \
           ../common/SphericalCameraManipulator.cpp

# Include Paths
INCLUDEPATH += ./ \
               ../common/
        
# Libraries for Windows/MSYS2
LIBS += -lglew32 -lfreeglut -lopengl32