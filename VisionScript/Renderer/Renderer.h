#ifndef Renderer_h
#define Renderer_h

#include "Language/Script.h"
#include "Utilities/Math3D.h"

typedef struct vertex
{
	vec2_t position;
	vec4_t color;
	float size;
} vertex_t;

void RenderScript(Script * script, bool testMode);

#endif
