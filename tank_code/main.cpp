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

//!Function Prototypes
bool initGL(int argc, char** argv);
void display(void);
void keyboard(unsigned char key, int x, int y);
void keyUp(unsigned char key, int x, int y);
void handleKeys();
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void Timer(int value);
void initShader();
void initGeometry();					            //Function to init Geometry 
void drawGeometry();					            //Function to draw Geometry

//Global Variables
GLuint vertexTexcoordAttribute;
GLuint shaderProgramID;			                    // Shader Program ID
GLuint vertexPositionAttribute;		                // Vertex Position Attribute Location
GLuint vertexNormalAttribute;					                // Light Position 
GLuint LightPositionUniformLocation;                // Light Position Uniform   
Vector3f colour;					                // Colour Variable 
GLuint ColourUniformLocation;		                // Colour Uniform - Explained in later excercise

GLuint AmbientUniformLocation;
GLuint SpecularUniformLocation;
GLuint SpecularPowerUniformLocation;

//Material Properties
Vector3f lightPosition= Vector3f(20.0,20.0,20.0);   // Light Position 
Vector3f ambient    = Vector3f(0.1,0.1,0.1);
Vector3f specular   = Vector3f(0.0,1.0,0.0);
float specularPower = 10.0;

int cubePositions[5][3] =
{
    {-2,0,2},
    {-4,0,4},
    {0,0,6},
    {0,0,8},
    {3,0,10},
};


GLuint TextureMapUniformLocation;               	// Texture Map Location
GLuint texture;				                    	// OpenGL Texture
//Viewing
SphericalCameraManipulator cameraManip;
Matrix4x4 ModelViewMatrix;		                    // ModelView Matrix
GLuint MVMatrixUniformLocation;		                // ModelView Matrix Uniform
Matrix4x4 ProjectionMatrix;		                    // Projection Matrix
GLuint ProjectionUniformLocation;	                // Projection Matrix Uniform Location

//Mesh
Mesh mesh;				                            // Mesh
Mesh fron_wheel;



//! Screen size
int screenWidth = 720;
int screenHeight = 720;

//! Array of key states
bool keyStates[256];

void initTexture(std::string filename, GLuint & textureID)
{
	//Generate texture and bind
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
    
	//Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 

	//Get texture Data
	int width, height;
	char* data;
	Texture::LoadBMP(filename, width, height, data);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);

    //Cleanup data - copied to GPU
    delete[] data;
}


//! Main Program Entry
int main(int argc, char** argv)
{
    //init OpenGL
	if(!initGL(argc, argv))
		return -1;
    
    //Init OpenGL Shader
    initShader();
	
	//Init Mesh Geometry
	//mesh.initCube();
    mesh.loadOBJ("../models/cube.obj");    
    initTexture("../models/Crate.bmp", texture);

    //Init Camera Manipultor
	cameraManip.setPanTiltRadius(0.f,0.f,3.f);
	cameraManip.setFocus(mesh.getMeshCentroid());

	//Start main loop
	glutMainLoop();
	//
	//Clean-Up
	glDeleteProgram(shaderProgramID);

    return 0;
}

void initShader()
{
//Create shader
   //Create shader
    shaderProgramID = Shader::LoadFromFile("shader.vert","shader.frag");
    
    // Get a handle for our vertex position buffer
	vertexPositionAttribute = glGetAttribLocation(shaderProgramID,  "aVertexPosition");
	vertexNormalAttribute = glGetAttribLocation(shaderProgramID,    "aVertexNormal");
	vertexTexcoordAttribute = glGetAttribLocation(shaderProgramID, "aVertexTexcoord");

    //!
	MVMatrixUniformLocation         = glGetUniformLocation(shaderProgramID, "MVMatrix_uniform"); 
	ProjectionUniformLocation       = glGetUniformLocation(shaderProgramID, "ProjMatrix_uniform"); 
	LightPositionUniformLocation    = glGetUniformLocation(shaderProgramID, "LightPosition_uniform"); 
	AmbientUniformLocation          = glGetUniformLocation(shaderProgramID, "Ambient_uniform"); 
	SpecularUniformLocation         = glGetUniformLocation(shaderProgramID, "Specular_uniform"); 
	SpecularPowerUniformLocation    = glGetUniformLocation(shaderProgramID, "SpecularPower_uniform");
    TextureMapUniformLocation       = glGetUniformLocation(shaderProgramID, "TextureMap_uniform"); 

}

//! Function to Initialise OpenGL
bool initGL(int argc, char** argv)
{
	//Init GLUT
    glutInit(&argc, argv);
    
	//Set Display Mode
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);

	//Set Window Size
    glutInitWindowSize(screenWidth, screenHeight);
    
    // Window Position
    glutInitWindowPosition(200, 200);

	//Create Window
    glutCreateWindow("Tank Assignment");

	glEnable(GL_DEPTH_TEST);
    
    // Init GLEW
	if (glewInit() != GLEW_OK) 
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return false;
	}
	
	//Set Display function
    glutDisplayFunc(display);
	
	//Set Keyboard Interaction Functions
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyUp); 

	//Set Mouse Interaction Functions
	glutMouseFunc(mouse);
	glutPassiveMotionFunc(motion);
	glutMotionFunc(motion);

    //Start start timer function after 100 milliseconds
    glutTimerFunc(100,Timer, 0);

	return true;
}




//! Display Loop
void display(void)
{
   // Clear the screen
	glClearColor(1.0,0.0,0.0,0.0);	
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


  	//Use shadermodels
	glUseProgram(shaderProgramID);
	
    //Projection Matrix - Perspective Projection
    ProjectionMatrix.perspective(90, 1.0, 0.0001, 100.0);
   
    //Set Projection Matrix
    glUniformMatrix4fv(	
		ProjectionUniformLocation,  //Uniform location
		1,							//Number of Uniforms
		false,						//Transpose Matrix
		ProjectionMatrix.getPtr());	//Pointer to ModelViewMatrixValues

	//Apply Camera Manipluator to Set Model View Matrix on GPU
    ModelViewMatrix.toIdentity();
    Matrix4x4 m = cameraManip.apply(ModelViewMatrix);
	glUniformMatrix4fv(	
		MVMatrixUniformLocation,  	//Uniform location
		1,					        //Number of Uniforms
		false,				        //Transpose Matrix
	    m.getPtr());	        //Pointer to Matrix Values
    
	glUniform3f(LightPositionUniformLocation, lightPosition.x,lightPosition.y,lightPosition.z);
    glUniform4f(AmbientUniformLocation, ambient.x, ambient.y, ambient.z, 1.0);
    glUniform4f(SpecularUniformLocation, specular.x, specular.y, specular.z, 1.0);
    glUniform1f(SpecularPowerUniformLocation, specularPower);
	
    //Set Colour after program is in use
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(TextureMapUniformLocation, 0);

    //Call Draw Geometry Function
    for(int i = 0; i < 5; i++)
	{
    	Matrix4x4 model = m;

    	Vector3f pos(cubePositions[i][0], cubePositions[i][1], cubePositions[i][2]);

    	model.translate(cubePositions[i][0],
                cubePositions[i][1],
                cubePositions[i][2]);

    	glUniformMatrix4fv(
        	MVMatrixUniformLocation,
        	1,
        	false,
        	model.getPtr()
    	);

    	mesh.Draw(vertexPositionAttribute, vertexNormalAttribute, vertexTexcoordAttribute);
	}
	
	//Unuse Shader
	glUseProgram(0);
	
	//Redraw frame
	glutPostRedisplay();
    glutSwapBuffers();
}



//! Keyboard Interaction
void keyboard(unsigned char key, int x, int y)
{
	//Quits program when esc is pressed
	if (key == 27)	//esc key code
	{
		exit(0);
	}
    
    //Set key status
    keyStates[key] = true;
}

//! Handle key up situation
void keyUp(unsigned char key, int x, int y)
{
    keyStates[key] = false;
}


//! Handle Keys
void handleKeys()
{
    //keys should be handled here
	if(keyStates['a'])
    {
        
    }
}

//! Mouse Interaction
void mouse(int button, int state, int x, int y)
{
    cameraManip.handleMouse(button, state,x,y);
    glutPostRedisplay(); 
}

// Mouse Interaction
void motion(int x, int y)
{
    cameraManip.handleMouseMotion(x,y);
    glutPostRedisplay(); 
}

//! Timer Function
void Timer(int value)
{
    
    //Call function again after 10 milli seconds
	glutTimerFunc(10,Timer, 0);
}



