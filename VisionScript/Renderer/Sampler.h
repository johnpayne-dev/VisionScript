#ifndef Sampler_h
#define Sampler_h

#include "Language/Evaluator.h"
#include "Language/Script.h"

RuntimeError SamplePolygon(Script * script, Statement * statement, VectorArray * result);

#endif
