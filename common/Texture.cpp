
#ifndef SphericalCameraManipulator_H_
#define SphericalCameraManipulator_H_


#include <Matrix.h>
#include <Vector.h>


class SphericalCameraManipulator
{
public:

    //!
    SphericalCameraManipulator();

    //!
    ~SphericalCameraManipulator();

    //!
    void setPanTiltRadius(float pan, float tilt, float radius);
    
    //!
    Matrix4x4 apply(Matrix4x4 matrix);

    //!
    void handleMouse(int button, int state, int x, int y);
    
    //!
    void handleMouseMotion(int x, int y);    

    //!
    void handleKeyboard(char key, int x, int y);

    //! 
    void setFocus(Vector3f focus);
      
    //!
    float getPan();

    //!
    float getTilt();
    
    //!
    float getRadius();

private:

    //!
    void enforceRanges();    
    
    //!
    Matrix4x4 transform();

private:

    Vector3f focus;

    float pan;
    float tilt;
    float radius;

    float panTiltStep;
    float panTiltMouseStep;
    float tiltRange[2];


    float radiusStep;
    float radiusMouseStep;
    float radiusWheelStep;
    float radiusRange[2];
    
    int currentButton;
    int currentState;
    float previousMousePosition[2];
    bool reset;

};


#endif

