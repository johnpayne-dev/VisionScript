#ifndef Renderer_h
#define Renderer_h

#include "Language/Script.h"
#include "Utilities/Math3D.h"
#include "Backend/Graphics.h"
#include "Camera.h"

typedef struct vertex
{
	vec2_t position;
	vec4_t color;
	float size;
} vertex_t;

typedef struct RenderObject
{
	Statement * statement;
	VertexBuffer buffer;
} RenderObject;

extern struct Renderer
{
	Script * script;
	Camera camera;
	bool testMode;
	int32_t width, height;
	VertexLayout layout;
	VertexBuffer quad;
	list(RenderObject) objects;
} renderer;

void RenderScript(Script * script, bool testMode);

#endif
