#ifndef Camera_h
#define Camera_h

#include "Utilities/Math3D.h"

typedef struct Camera2D
{
	vec2_t position;
	vec2_t scale;
	float angle;
} Camera2D;

typedef struct Camera3D
{
	vec3_t position;
	vec3_t scale;
	vec3_t angles;
} Camera3D;

#endif
