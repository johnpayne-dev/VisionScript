#ifndef Sampler_h
#define Sampler_h

#include "Language/Evaluator.h"
#include "Language/Script.h"

RuntimeError SamplePolygons(Script * script, Statement * statement, VectorArray * result);

RuntimeError SamplePoints(Script * script, Statement * statement, VectorArray * result);

#endif
