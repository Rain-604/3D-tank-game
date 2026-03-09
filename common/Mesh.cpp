#ifndef MATRIX_H_
#define MATRIX_H_

#include <string>

//Forward class declaration
class Vector3f;

/**
 * 4x4 Matrix class
 */
class Matrix4x4
{

public:
	
	//!Constructor
	Matrix4x4();

   	//! Assignment Constructor
	Matrix4x4(
			float v00,float v10,float v20,float v30,
			float v01,float v11,float v21,float v31,
			float v02,float v12,float v22,float v32,
			float v03,float v13,float v23,float v33);

	//!Destructor
	~Matrix4x4();

	//! Creates Identity Matrix
	void toIdentity();

    	//! Set Matrix values
	void set(Matrix4x4 matrix);

    	//Return Pointer to first value in matrix - used when passing to opengl uniform
	float * getPtr();

	//! Static multiply function
	static Matrix4x4 multiply(Matrix4x4 & lhs, Matrix4x4 & rhs);

    //!
    Matrix4x4 inverse();
    
    //!
    Matrix4x4 transpose();
    
    //!
    float determinant();

	//! Multiply Function
	Matrix4x4 operator*(Matrix4x4 rhs);
	
	//
    Matrix4x4 operator/(float scale);

	//! Print Out Matrix	
	void print(std::string message = "");

	//! Translate Function
	void translate(float x, float y, float z);

	//! Rotate Function
	void rotate(float angle, float x, float y, float z);
	
	//!Scale Function
	void scale(float x, float y, float z);
	
	//! Orthographic Projection Matrix Function 
	void ortho(float left, float right, float bottom, float top, float zNear, float zFar);

	//! Perspective Projection Matrix Function
	void perspective(float fovy, float aspect, float near, float far);

	//! LookAt function
	void lookAt(Vector3f eye, Vector3f center, Vector3f up);	

private:

	//! 2D Array containing values: accessed val[COLUMN][ROW]
	float val[4][4];

};


#endif
