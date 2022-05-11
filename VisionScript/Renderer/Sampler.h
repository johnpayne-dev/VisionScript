#ifndef Sampler_h
#define Sampler_h

#include "Language/Evaluator.h"
#include "Language/Script.h"
#include "Renderer.h"
#include "Camera.h"

RuntimeError SamplePolygons(Script * script, Statement * statement, RenderObject * object);

RuntimeError SamplePoints(Script * script, Statement * statement, RenderObject * object);

RuntimeError SampleParametric(Script * script, Statement * statement, float lower, float upper, Camera camera, RenderObject * object);

#endif
