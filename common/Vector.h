#ifndef VECTOR_H_
#define VECTOR_H_



/**
 * Vector 2 Dimensions floatings
 */
class Vector2f
{

public:

	//!
	Vector2f()
	:x(0),y(0){};

	//!
	~Vector2f(){};
	
	//!
	Vector2f(float x, float y)
		:x(x), y(y){};

	float x, y; 
};


/**
 * Vector 3 Dimensions floatings
 */
class Vector3f
{
	

public:

	//!
	Vector3f()
	:x(0),y(0),z(0){};

	//!
	~Vector3f(){};
	
	//!
	Vector3f(float x, float y, float z)
		:x(x),y(y),z(z){};
		
	//!
	Vector3f operator-(Vector3f rhs);
	
	//!
	Vector3f operator+(Vector3f rhs);

	//! 
	Vector3f operator/(float rhs);

    //!    
	Vector3f operator*(float rhs);

	//! get length of vector
	float length();
	
	//! cross product funstion
	static Vector3f cross(Vector3f v1, Vector3f v2);
	
	//! dot product function
	static float dot(Vector3f v1, Vector3f v2);

	//! Normalise function
	static Vector3f normalise(Vector3f v);

	//! Values
	float x,y,z;

};


#endif
