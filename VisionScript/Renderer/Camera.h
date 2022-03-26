#ifndef Camera_h
#define Camera_h

#include "Utilities/Math3D.h"

typedef struct Camera2D
{
	float aspectRatio;
	vec2_t position;
	vec2_t scale;
	float angle;
} Camera2D;

static inline mat4_t Camera2DTransform(Camera2D camera)
{
	mat4_t matrix = mat4_identity;
	matrix = mat4_translate(matrix, vec2_to_vec3(vec2_neg(camera.position)));
	matrix = mat4_scale(matrix, (vec3_t){ camera.scale.x, -camera.scale.y, 1.0 });
	matrix = mat4_rotate_z(matrix, -camera.angle);
	matrix = mat4_scale(matrix, (vec3_t){ 1.0 / camera.aspectRatio, 1.0, 1.0 });
	return matrix;
}

static inline mat4_t Camera2DInverseTransform(Camera2D camera)
{
	mat4_t matrix = mat4_identity;
	matrix = mat4_scale(matrix, (vec3_t){ camera.aspectRatio, 1.0, 1.0 });
	matrix = mat4_rotate_z(matrix, camera.angle);
	matrix = mat4_scale(matrix, (vec3_t){ 1.0 / camera.scale.x, -1.0 / camera.scale.y, 1.0});
	matrix = mat4_translate(matrix, vec2_to_vec3(camera.position));
	return matrix;
}

typedef struct Camera3D
{
	vec3_t position;
	vec3_t scale;
	vec3_t angles;
} Camera3D;

#endif
