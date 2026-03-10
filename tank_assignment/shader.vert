#version 120

// Attributes
attribute vec3 aVertexPosition;
attribute vec3 aVertexNormal;
attribute vec2 aVertexTexcoord;

uniform mat4x4 MVMatrix_uniform;
uniform mat4x4 ProjMatrix_uniform;
uniform vec3   LightPosition_uniform;

varying vec3 ViewDirection;
varying vec3 LightDirection;
varying vec3 Normal;
varying vec2 texCoord;
varying vec2 uv;


void main( void )
{
   texCoord = aVertexTexcoord;

   ViewDirection  = -vec3(MVMatrix_uniform * vec4(aVertexPosition, 1.0));
   LightDirection = LightPosition_uniform;
   Normal         = (MVMatrix_uniform * vec4(aVertexNormal,0.0)).xyz;  
   uv = aVertexTexcoord;
   gl_Position = ProjMatrix_uniform * MVMatrix_uniform * vec4(aVertexPosition,1.0);
}
