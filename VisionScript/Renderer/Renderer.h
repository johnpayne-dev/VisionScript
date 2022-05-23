#ifndef Renderer_h
#define Renderer_h

#include <sokol_app.h>
#include <OpenGL/GL3.h>
#include "Language/Script.h"
#include "Utilities/Math3D.h"
#include "Utilities/Threads.h"
#include "Camera.h"

typedef struct vertex {
	vec2_t position;
	vec4_t color;
	float size;
	vec2_t pair;
} vertex_t;

typedef struct RenderObject {
	Equation equation;
	GLuint buffer;
	vertex_t * vertices;
	uint64_t vertexCount;
	bool needsUpload;
} RenderObject;

extern struct Renderer {
	Script script;
	Camera camera;
	bool testMode;
	int32_t width;
	int32_t height;
	GLuint layout;
	GLuint quad;
	List(RenderObject) objects;
	bool samplerRunning;
	pthread_t samplerThread;
} renderer;

sapp_desc RenderScript(Script script, bool testMode);

#endif
