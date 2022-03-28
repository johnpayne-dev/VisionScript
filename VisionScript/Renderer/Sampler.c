#include "Sampler.h"

RuntimeError SamplePolygon(Script * script, Statement * statement, VectorArray * result)
{
	RuntimeError error = EvaluateExpression(script->identifiers, script->cache, NULL, statement->expression, result);
	if (error.code != RuntimeErrorNone) { return error; }
	if (result->dimensions != 2 && result->dimensions != 3) { return (RuntimeError){ RuntimeErrorInvalidPolygonDimensionality }; }
	return (RuntimeError){ RuntimeErrorNone };
}
