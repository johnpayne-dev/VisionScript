#ifndef Sampler_h
#define Sampler_h

#include "Language/Evaluator.h"
#include "Language/Script.h"
#include "Backend/VertexBuffer.h"
#include "Camera.h"

RuntimeError SamplePolygons(Script * script, Statement * statement, VertexBuffer * buffer);

RuntimeError SamplePoints(Script * script, Statement * statement, VertexBuffer * buffer);

RuntimeError SampleParametric(Script * script, Statement * statement, float lower, float upper, Camera camera, VertexBuffer * buffer);

#endif
