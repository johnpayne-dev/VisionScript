#ifndef Sampler_h
#define Sampler_h

#include "Language/Evaluator.h"
#include "Language/Script.h"
#include "Renderer.h"
#include "Camera.h"

RuntimeError SamplePolygons(Script * script, Equation equation, RenderObject * object);

RuntimeError SamplePoints(Script * script, Equation equation, RenderObject * object);

RuntimeError SampleParametric(Script * script, Equation equation, float lower, float upper, Camera camera, RenderObject * object);

#endif
