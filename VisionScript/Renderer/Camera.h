#ifndef Camera_h
#define Camera_h

#include "Utilities/Math3D.h"

typedef struct Camera
{
	float aspectRatio;
	vec2_t position;
	vec2_t scale;
	float angle;
} Camera;

static inline mat4_t CameraMatrix(Camera camera)
{
	mat4_t matrix = mat4_identity;
	matrix = mat4_translate(matrix, vec2_to_vec3(vec2_neg(camera.position)));
	matrix = mat4_scale(matrix, (vec3_t){ camera.scale.x, -camera.scale.y, 1.0 });
	matrix = mat4_rotate_z(matrix, -camera.angle);
	matrix = mat4_scale(matrix, (vec3_t){ 1.0 / camera.aspectRatio, 1.0, 1.0 });
	return matrix;
}

static inline mat4_t CameraInverseMatrix(Camera camera)
{
	mat4_t matrix = mat4_identity;
	matrix = mat4_scale(matrix, (vec3_t){ camera.aspectRatio, 1.0, 1.0 });
	matrix = mat4_rotate_z(matrix, camera.angle);
	matrix = mat4_scale(matrix, (vec3_t){ 1.0 / camera.scale.x, -1.0 / camera.scale.y, 1.0});
	matrix = mat4_translate(matrix, vec2_to_vec3(camera.position));
	return matrix;
}

static inline vec2_t CameraTransformPoint(Camera camera, vec2_t p)
{
	p = vec2_sub(p, camera.position);
	p = vec2_mul(p, (vec2_t){ camera.scale.x, -camera.scale.y });
	p = (vec2_t){ p.x * cosf(-camera.angle) - p.y * sinf(-camera.angle), p.x * sinf(-camera.angle) + p.y * cosf(-camera.angle) };
	p = vec2_mul(p, (vec2_t){ 1.0 / camera.aspectRatio, 1.0 });
	return p;
}

static inline bool CameraIsPointVisible(Camera camera, vec2_t p)
{
	p = CameraTransformPoint(camera, p);
	return p.x >= -camera.aspectRatio && p.x <= camera.aspectRatio && p.y >= -1.0 && p.y <= 1.0;
}

#endif
