#ifndef Renderer_h
#define Renderer_h

#include <sokol_app.h>
#include <OpenGL/GL3.h>
#include "Language/Script.h"
#include "Utilities/Math3D.h"
#include "Utilities/Threads.h"
#include "Camera.h"

typedef struct vertex
{
	vec2_t position;
	vec4_t color;
	float size;
	vec2_t pair;
} vertex_t;

typedef struct RenderObject
{
	Statement * statement;
	GLuint buffer;
} RenderObject;

extern struct Renderer
{
	Script * script;
	Camera camera;
	bool testMode;
	int32_t width, height;
	GLuint layout;
	GLuint quad;
	list(RenderObject) objects;
	bool samplerRunning;
	pthread_t samplerThread;
} renderer;

sapp_desc RenderScript(Script * script, bool testMode);

#endif
