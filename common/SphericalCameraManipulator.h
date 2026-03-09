#include <SphericalCameraManipulator.h>

#include <GL/glut.h>
#include <math.h>
#include <iostream>
#define PI 3.14159265358979323846

//!
SphericalCameraManipulator::SphericalCameraManipulator()
{
    this->pan                       = 0.0;
    this->tilt                      = 0.0;
    this->radius                    = 1.0;
 
    this->panTiltStep               = 0.1;
    this->panTiltMouseStep          = 0.005;

    this->radiusStep                = 0.20;
    this->radiusMouseStep           = 0.05;
    this->radiusWheelStep           = 0.30;

    this->tiltRange[0]              = -1.56;
    this->tiltRange[1]              = -0.01;
    
    this->radiusRange[0]            = 0.1;
    this->radiusRange[1]            = 20;


    this->currentButton             = 0;
    this->currentState              = 0;
    this->previousMousePosition[0]  = 0;
    this->previousMousePosition[1]  = 0;
}

//!
SphericalCameraManipulator::~SphericalCameraManipulator()
{

}

//!
void SphericalCameraManipulator::setPanTiltRadius(float pan, float tilt, float radius)
{
    this->pan       = pan;
    this->tilt      = tilt;
    this->radius    = radius;
    this->enforceRanges();
}

//!
Matrix4x4 SphericalCameraManipulator::apply(Matrix4x4 matrix)
{
    Matrix4x4 t = this->transform();
    return Matrix4x4::multiply(matrix, t );
}

//!
void SphericalCameraManipulator::handleMouse(int button, int state, int x, int y)
{
    this->currentButton = button;
    this->currentState  = state;
    if(this->currentState == GLUT_UP)
    {
        reset = true;
    }
}

//!
void SphericalCameraManipulator::handleMouseMotion(int x, int y)
{
    //Motion
    float xMotion = (float)x - previousMousePosition[0];
    float yMotion = (float)y - previousMousePosition[1];

    if(reset)
    {
		xMotion = yMotion = 0.0;
		reset = false;
    }

    this->previousMousePosition[0] = (float)x;
    this->previousMousePosition[1] = (float)y;

    //Left Button Action
	if(this->currentButton == GLUT_LEFT_BUTTON && this->currentState == GLUT_DOWN)
	{
	    this->pan  -= xMotion*this->panTiltMouseStep ;
		this->tilt += yMotion*this->panTiltMouseStep ;
	}

    //Right Button Action
    else if(this->currentButton == GLUT_RIGHT_BUTTON && this->currentState == GLUT_DOWN)
    {
	    this->radius -= yMotion*this->radiusMouseStep ;
    }

    //Enforce Ranges
    this->enforceRanges();
} 


//!
void SphericalCameraManipulator::setFocus(Vector3f focus)
{
    this->focus = focus;
}

//!
void SphericalCameraManipulator::enforceRanges()
{
    //Pan Ranges
	while(this->pan<0.0)
		this->pan += 2.0*PI ;
	while(this->pan>=2.0*PI)
		this->pan -= 2.0*PI ;	
	
   // Tilt Ranges 
	if(this->tilt<this->tiltRange[0])
		this->tilt = this->tiltRange[0] ;
	if(this->tilt>this->tiltRange[1])
		this->tilt = this->tiltRange[1] ;

    //Radius Ranges
	if(this->radius<this->radiusRange[0])
		this->radius = this->radiusRange[0] ;
	if(this->radius>this->radiusRange[1])
        this->radius = this->radiusRange[1] ;
}

//!
Matrix4x4 SphericalCameraManipulator::transform()
{
	// note: y axis up

   	// aVec is the displacement from focus point
    Vector3f aVec;
    float tiltSin = sin(this->tilt);
	aVec.x = tiltSin * sin(this->pan) ;
	aVec.y = cos(this->tilt) ;
	aVec.z = tiltSin * cos(this->pan) ;
	
	//camera centre in world coordinates    
	Vector3f cVec;
    cVec = aVec * this->radius;    
    cVec = cVec + this->focus;
	
	// Horizontal (side) Vector
    Vector3f hVec;
	hVec.x = +aVec.z ;
	hVec.y = 0.0 ;
	hVec.z = -aVec.x ;
    hVec = Vector3f::normalise(hVec);

	//Up Vector
    Vector3f uVec;
    uVec = Vector3f::cross(aVec, hVec);
    
	//Construct Matrix
    Matrix4x4 m = Matrix4x4(
    	hVec.x, uVec.x, aVec.x, cVec.x,
    	hVec.y, uVec.y, aVec.y, cVec.y,
    	hVec.z, uVec.z, aVec.z, cVec.z,
    	0,      0,      0,      1.0);
    m = m.inverse();

    return m;
}


//!
float SphericalCameraManipulator::getPan()
{
    return this->pan;
}

//!
float SphericalCameraManipulator::getTilt()
{
    return this->tilt;
}
        
//!
float SphericalCameraManipulator::getRadius()
{
    return this->radius;
}


