#ifndef Sampler_h
#define Sampler_h

#include "Language/Evaluator.h"
#include "Language/Script.h"
#include "Backend/VertexBuffer.h"

RuntimeError SamplePolygons(Script * script, Statement * statement, VertexBuffer * buffer);

RuntimeError SamplePoints(Script * script, Statement * statement, VertexBuffer * buffer);

RuntimeError SampleParametric(Script * script, Statement * statement, float lower, float upper, VertexBuffer * buffer);

#endif
