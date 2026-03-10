TEMPLATE = app

#Executable Name
TARGET = TankAssignment
CONFIG = debug

#Destination
DESTDIR = ./
OBJECTS_DIR = ./build/

HEADERS	+= 	../common/Shader.h	    	\	
		../common/Vector.h		        \	
		../common/Matrix.h		        \
		../common/Mesh.h		        \
        ../common/Texture.h             \		
        ../common/SphericalCameraManipulator.h   \

#Sources
SOURCES += 	main.cpp			        \
  		../common/Shader.cpp		    \
		../common/Vector.cpp		    \
		../common/Matrix.cpp		    \
		../common/Mesh.cpp		        \
        ../common/Texture.cpp           \
        ../common/SphericalCameraManipulator.cpp \

INCLUDEPATH += 	./ 				    \
		        ../common/ 			\

		
#Library Libraries
LIBS +=	-lGLEW			    	    	        \
		-lglut			        		        \
        -lGL             	                  	\  
