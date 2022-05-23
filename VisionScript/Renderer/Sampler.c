#include <stdlib.h>
#include <stdio.h>
#include "Sampler.h"
#include "Camera.h"

#define MAX_PARAMETRIC_VERTICES 1048576

RuntimeError SamplePolygons(Script * script, Equation equation, RenderObject * object) {
	VectorArray result;
	RuntimeError error = EvaluateExpression(&script->environment, NULL, equation.expression, &result);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	if (result.dimensions != 2) { return (RuntimeError){ RuntimeErrorCodeInvalidRenderDimensionality, equation.expression.start, equation.expression.end, equation.expression.line }; }
	
	if (!object->needsUpload) {
		if (result.length > object->vertexCount) {
			if (object->vertexCount == 0) { object->vertices = malloc(result.length * sizeof(vertex_t)); }
			else { object->vertices = realloc(object->vertices, result.length * sizeof(vertex_t)); }
		}
		for (int32_t i = 0; i < result.length; i++) {
			object->vertices[i] = (vertex_t) {
				.position = { result.xyzw[0][i], result.xyzw[1][i] },
				.color = { 0.0, 0.0, 0.0, 1.0 },
			};
		}
		object->vertexCount = result.length;
		object->needsUpload = true;
	}
	
	FreeVectorArray(result);
	return (RuntimeError){ RuntimeErrorCodeNone };
}

RuntimeError SamplePoints(Script * script, Equation equation, RenderObject * object) {
	VectorArray result;
	RuntimeError error = EvaluateExpression(&script->environment, NULL, equation.expression, &result);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	if (result.dimensions != 2) { return (RuntimeError){ RuntimeErrorCodeInvalidRenderDimensionality, equation.expression.start, equation.expression.end, equation.expression.line }; }
	
	if (!object->needsUpload) {
		if (result.length > object->vertexCount) {
			if (object->vertexCount == 0) { object->vertices = malloc(result.length * sizeof(vertex_t)); }
			else { object->vertices = realloc(object->vertices, result.length * sizeof(vertex_t)); }
		}
		for (int32_t i = 0; i < result.length; i++) {
			object->vertices[i] = (vertex_t) {
				.position = { result.xyzw[0][i], result.xyzw[1][i] },
				.color = { 0.0, 0.0, 0.0, 1.0 },
				.size = 8.0,
			};
		}
		object->vertexCount = result.length;
		object->needsUpload = true;
	}
	
	FreeVectorArray(result);
	return (RuntimeError){ RuntimeErrorCodeNone };
}

RuntimeError SampleParametric(Script * script, Equation equation, float lower, float upper, Camera camera, RenderObject * object) {
	return (RuntimeError){ RuntimeErrorCodeNotImplemented, equation.expression.start, equation.expression.end, equation.expression.line };
}
