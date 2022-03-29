#ifndef Sampler_h
#define Sampler_h

#include "Language/Evaluator.h"
#include "Language/Script.h"
#include "Backend/VertexBuffer.h"

RuntimeError SamplePolygons(Script * script, Statement * statement, VertexLayout layout, VertexBuffer * buffer);

RuntimeError SamplePoints(Script * script, Statement * statement, VertexLayout layout, VertexBuffer * buffer);

#endif
