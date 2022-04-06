#include <stdlib.h>
#include <stdio.h>
#include "Sampler.h"
#include "Renderer.h"
#include "Camera.h"

#define MAX_PARAMETRIC_VERTICES 1048576

RuntimeError SamplePolygons(Script * script, Statement * statement, VertexBuffer * buffer)
{
	VectorArray result;
	RuntimeError error = EvaluateExpression(script->identifiers, script->cache, NULL, statement->expression, &result);
	if (error.code != RuntimeErrorNone) { return error; }
	if (result.dimensions != 2) { return (RuntimeError){ RuntimeErrorInvalidRenderDimensionality }; }
	
	if (result.length > buffer->vertexCount)
	{
		if (buffer->vertexCount > 0) { VertexBufferFree(*buffer); }
		*buffer = VertexBufferCreate(renderer.layout, result.length);
	}
	vertex_t * vertices = VertexBufferMapVertices(*buffer);
	for (int32_t i = 0; i < result.length; i++)
	{
		vertices[i] = (vertex_t)
		{
			.position = { result.xyzw[0][i], result.xyzw[1][i] },
			.color = { 0.0, 0.0, 0.0, 1.0 },
		};
	}
	buffer->subVertexCount = result.length;
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
	
	if (result.length > buffer->vertexCount)
	{
		if (buffer->vertexCount > 0) { VertexBufferFree(*buffer); }
		*buffer = VertexBufferCreate(renderer.layout, result.length);
	}
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
	buffer->subVertexCount = result.length;
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

static RuntimeError ReadSamples(Script * script, Statement * statement, list(Parameter) parameters, float t, Sample ** samples, int32_t * length)
{
	parameters[0].cache = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = &t };
	VectorArray result;
	RuntimeError error = EvaluateExpression(script->identifiers, script->cache, parameters, statement->expression, &result);
	if (error.code != RuntimeErrorNone) { return error; }
	if (result.dimensions != 2) { return (RuntimeError){ RuntimeErrorInvalidRenderDimensionality }; }
	*length = result.length;
	
	*samples = malloc(sizeof(Sample) * result.length);
	for (int32_t i = 0; i < result.length; i++)
	{
		(*samples)[i] = (Sample)
		{
			.position = (vec2_t){ result.xyzw[0][i], result.xyzw[1][i] },
			.color = (vec4_t){ 0.0, 0.0, 0.0, 1.0 },
			.t = t,
			.next = NULL,
		};
		(*samples)[i].value = CameraTransformPoint(renderer.camera, (*samples)[i].position);
		(*samples)[i].value = vec2_mul((*samples)[i].value, (vec2_t){ renderer.camera.aspectRatio, 1.0 });
	}
	
	FreeVectorArray(result);
	return (RuntimeError){ RuntimeErrorNone };
}

static RuntimeError ReadSample(Script * script, Statement * statement, list(Parameter) parameters, float t, int32_t i, Sample ** sample)
{
	parameters[0].cache = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = &t };
	VectorArray result;
	RuntimeError error = EvaluateExpressionIndices(script->identifiers, script->cache, parameters, statement->expression, &i, 1, &result);
	if (error.code != RuntimeErrorNone) { return error; }
	if (result.dimensions != 2) { return (RuntimeError){ RuntimeErrorInvalidRenderDimensionality }; }
	
	*sample = malloc(sizeof(Sample));
	**sample = (Sample)
	{
		.position = (vec2_t){ result.xyzw[0][0], result.xyzw[1][0] },
		.color = (vec4_t){ 0.0, 0.0, 0.0, 1.0 },
		.t = t,
		.next = NULL,
	};
	(*sample)->value = CameraTransformPoint(renderer.camera, (*sample)->position);
	(*sample)->value = vec2_mul((*sample)->value, (vec2_t){ renderer.camera.aspectRatio, 1.0 });
	
	FreeVectorArray(result);
	return error;
}

RuntimeError SampleParametric(Script * script, Statement * statement, float lower, float upper, VertexBuffer * buffer)
{
	list(Parameter) parameters = ListPush(ListCreate(sizeof(Parameter), 1), &(Parameter){ .identifier = "t", .cached = true });
	
	// get initial start sample
	Sample * starts;
	int32_t length;
	RuntimeError error = ReadSamples(script, statement, parameters, lower, &starts, &length);
	
	// create 128 base samples across the lower-upper range
	int32_t baseSampleCount = 1024;
	Sample * samples = starts;
	for (int32_t i = 0; i < baseSampleCount; i++)
	{
		Sample * nexts;
		int32_t nextLength;
		RuntimeError sampleError = ReadSamples(script, statement, parameters, (upper - lower) * (float)(i + 1) / baseSampleCount + lower, &nexts, &nextLength);
		if (nextLength != length) { error = (RuntimeError){ RuntimeErrorNotImplemented }; }
		if (sampleError.code != RuntimeErrorNone){ error = sampleError; }
		
		for (int32_t j = 0; j < length; j++) { samples[j].next = nexts + j; }
		samples = nexts;
	}
	
	// subdivide based on visibility
	int32_t totalVertexCount = 0;
	for (int32_t i = 0; i < length; i++)
	{
		int32_t vertexCount = baseSampleCount + 1;
		while (true)
		{
			if (totalVertexCount + vertexCount > MAX_PARAMETRIC_VERTICES)
			{
				error = (RuntimeError){ RuntimeErrorNotImplemented };
				break;
			}
			int32_t prevVertexCount = vertexCount;
			Sample * left = starts + i;
			while (left->next != NULL)
			{
				Sample * right = left->next;
				float ds = vec2_dist(left->value, right->value);
				float radius = vec2_len((vec2_t){ renderer.camera.aspectRatio, 1.0 });
				if (ds > 10.0 / renderer.height && (vec2_len(left->value) < radius || vec2_len(right->value) < radius))
				{
					Sample * sample;
					RuntimeError sampleError = ReadSample(script, statement, parameters, (left->t + right->t) / 2.0, i, &sample);
					if (sampleError.code != RuntimeErrorNone) { error = sampleError; }
					left->next = sample;
					sample->next = right;
					vertexCount++;
				}
				left = right;
			}
			if (vertexCount <= prevVertexCount){ break; }
		}
		totalVertexCount += vertexCount;
	}
	
	if (error.code == RuntimeErrorNone)
	{
		if (totalVertexCount > buffer->vertexCount)
		{
			if (buffer->vertexCount > 0) { VertexBufferFree(*buffer); }
			*buffer = VertexBufferCreate(renderer.layout, totalVertexCount);
		}
		vertex_t * vertices = VertexBufferMapVertices(*buffer);
		int32_t c = 0;
		for (int32_t i = 0; i < length; i++)
		{
			Sample * sample = starts + i;
			while (sample != NULL)
			{
				vertices[c] = (vertex_t)
				{
					.position = sample->position,
					.color = { 0.0, 0.0, 0.0, 1.0 },
					.size = 8.0,
				};
				sample = sample->next;
				c++;
			}
		}
		buffer->subVertexCount = c;
		VertexBufferUnmapVertices(*buffer);
		VertexBufferUpload(*buffer);
	}
	ListFree(parameters);
	return error;
}
