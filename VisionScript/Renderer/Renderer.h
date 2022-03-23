#ifndef Renderer_h
#define Renderer_h

#include "Language/Script.h"

typedef enum RendererType
{
	RendererType2D,
	RendererType3D,
} RendererType;

void RenderScript(Script * script, RendererType type, bool debug);

#endif
