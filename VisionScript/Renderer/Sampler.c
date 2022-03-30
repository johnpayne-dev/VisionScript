#include <stdlib.h>
#include <stdio.h>
#include "Sampler.h"
#include "Renderer.h"
#include "Camera.h"

#define MAX_PARAMETRICS 8192

RuntimeError SamplePolygons(Script * script, Statement * statement, VertexBuffer * buffer)
{
	VectorArray result;
	RuntimeError error = EvaluateExpression(script->identifiers, script->cache, NULL, statement->expression, &result);
	if (error.code != RuntimeErrorNone) { return error; }
	if (result.dimensions != 2) { return (RuntimeError){ RuntimeErrorInvalidRenderDimensionality }; }
	
	*buffer = VertexBufferCreate(renderer.layout, result.length);
	vertex_t * vertices = VertexBufferMapVertices(*buffer);
	for (int32_t i = 0; i < result.length; i++)
	{
		vertices[i] = (vertex_t)
		{
			.position = { result.xyzw[0][i], result.xyzw[1][i] },
			.color = { 0.0, 0.0, 0.0, 1.0 },
		};
	}
	VertexBufferUnmapVertices(*buffer);
	VertexBufferUpload(*buffer);
	
	FreeVectorArray(result);
	return (RuntimeError){ RuntimeErrorNone };
}

RuntimeError SamplePoints(Script * script, Statement * statement, VertexBuffer * buffer)
{
	VectorArray result;
	RuntimeError error = EvaluateExpression(script->identifiers, script->cache, NULL, statement->expression, &result);
	if (error.code != RuntimeErrorNone) { return error; }
	if (result.dimensions != 2) { return (RuntimeError){ RuntimeErrorInvalidRenderDimensionality }; }
	
	*buffer = VertexBufferCreate(renderer.layout, result.length);
	vertex_t * vertices = VertexBufferMapVertices(*buffer);
	for (int32_t i = 0; i < result.length; i++)
	{
		vertices[i] = (vertex_t)
		{
			.position = { result.xyzw[0][i], result.xyzw[1][i] },
			.color = { 0.0, 0.0, 0.0, 1.0 },
			.size = 8.0,
		};
	}
	VertexBufferUnmapVertices(*buffer);
	VertexBufferUpload(*buffer);
	
	FreeVectorArray(result);
	return (RuntimeError){ RuntimeErrorNone };
}

typedef struct Sample
{
	float t;
	int32_t length;
		vec2_t value;
		vec2_t position;
		vec4_t color;
		struct Sample * next;
} Sample;

static RuntimeError CreateSample(Script * script, Statement * statement, list(Parameter) parameters, float t, int32_t length, Sample * sample)
{
	sample->next = NULL;
	sample->t = t;
	parameters[0].value = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = &t };
	
	VectorArray value;
	RuntimeError error = EvaluateExpression(script->identifiers, script->cache, parameters, statement->expression, &value);
	if (error.code != RuntimeErrorNone) { return error; }
	if (value.dimensions != 2) { return (RuntimeError){ RuntimeErrorInvalidRenderDimensionality }; }
	if (value.length > MAX_PARAMETRICS) { return (RuntimeError){ RuntimeErrorNotImplemented }; }
	if (length > 0 && value.length != length) { return (RuntimeError){ RuntimeErrorNotImplemented }; }
	
	if (length == 0) { sample->length = value.length; }
	sample->position = (vec2_t){ value.xyzw[0][0], value.xyzw[1][0] };
	sample->color = (vec4_t){ 0.0, 0.0, 0.0, 1.0 };
	sample->value = CameraTransformPoint(renderer.camera, sample->position);
	
	FreeVectorArray(value);
	return error;
}

RuntimeError SampleParametric(Script * script, Statement * statement, float lower, float upper, VertexBuffer * buffer)
{
	list(Parameter) parameters = ListPush(ListCreate(sizeof(Parameter), 1), &(Parameter){ .identifier = "t" });
	
	Sample start;
	RuntimeError error = CreateSample(script, statement, parameters, lower, 0, &start);
	
	int32_t baseSampleCount = 128;
	
	Sample * sample = &start;
	for (int32_t j = 0; j < baseSampleCount; j++)
	{
		sample->next = malloc(sizeof(Sample));
		RuntimeError sampleError = CreateSample(script, statement, parameters, (upper - lower) * (float)(j + 1) / baseSampleCount + lower, start.length, sample->next);
		if (sampleError.code != RuntimeErrorNone) { error = sampleError; }
		sample = sample->next;
	}
	
	int32_t vertexCount = baseSampleCount;
	while (vertexCount < 8192)
	{
		int32_t prevVertexCount = vertexCount;
		Sample * left = &start;
		while (left->next != NULL)
		{
			Sample * right = left->next;
			float ds = vec2_dist(left->value, right->value);
			float radius = vec2_len((vec2_t){ renderer.camera.aspectRatio, 1.0 });
			if (ds > 5.0 / renderer.height && (vec2_len(left->value) < radius || vec2_len(right->value) < radius))
			{
				Sample * sample = malloc(sizeof(Sample));
				RuntimeError sampleError = CreateSample(script, statement, parameters, (left->t + right->t) / 2.0, start.length, sample);
				if (sampleError.code != RuntimeErrorNone) { error = sampleError; }
				left->next = sample;
				sample->next = right;
				vertexCount++;
			}
			left = right;
		}
		if (vertexCount == prevVertexCount) { break; }
	}
	
	if (error.code == RuntimeErrorNone)
	{
		printf("%i\n", vertexCount);
		*buffer = VertexBufferCreate(renderer.layout, vertexCount);
		vertex_t * vertices = VertexBufferMapVertices(*buffer);
		int32_t i = 0;
		sample = &start;
		while (sample->next != NULL)
		{
			vertices[i] = (vertex_t)
			{
				.position = sample->position,
				.color = { 0.0, 0.0, 0.0, 1.0 },
			};
			sample = sample->next;
			i++;
		}
		VertexBufferUnmapVertices(*buffer);
		VertexBufferUpload(*buffer);
	}
	ListFree(parameters);
	return error;
}
