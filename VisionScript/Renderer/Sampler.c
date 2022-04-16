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

typedef struct ParametricSample
{
	float t;
	vec2_t position;
	vec2_t screenPosition;
	vec4_t color;
	int32_t next;
} ParametricSample;

static inline float FixFloat(float f)
{
	if (f == INFINITY) { return 1e7; }
	if (f == -INFINITY) { return -1e7; }
	if (isnan(f)) { return 0.0; }
	return f;
}

static RuntimeError EvaluateParametric(Script * script, Expression * expression, list(Parameter) parameters, float t, Camera camera, int32_t index, ParametricSample * samples)
{
	parameters[0].cache = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = &t };
	
	VectorArray result;
	RuntimeError error = EvaluateExpressionIndices(script->identifiers, script->cache, parameters, expression, index < 0 ? NULL : &index, 1, &result);
	if (error.code != RuntimeErrorNone) { return error; }
	if (result.dimensions != 2) { FreeVectorArray(result); return (RuntimeError){ RuntimeErrorInvalidRenderDimensionality }; }
	
	for (int32_t i = 0; i < result.length; i++)
	{
		result.xyzw[0][i] = FixFloat(result.xyzw[0][i]);
		result.xyzw[1][i] = FixFloat(result.xyzw[1][i]);
		samples[i] = (ParametricSample)
		{
			.position = (vec2_t){ result.xyzw[0][i], result.xyzw[1][i] },
			.color = (vec4_t){ 0.0, 0.0, 0.0, 1.0 },
			.t = t,
		};
		samples[i].screenPosition = CameraTransformPoint(camera, samples[i].position);
		samples[i].screenPosition = vec2_mul(samples[i].screenPosition, (vec2_t){ camera.aspectRatio, 1.0 });
	}
	
	FreeVectorArray(result);
	return (RuntimeError){ RuntimeErrorNone };
}

bool SegmentCircleIntersection(vec2_t p1, vec2_t p2, float r)
{
	vec2_t d = vec2_sub(p2, p1);
	vec2_t f = vec2_neg(p1);
	float a = vec2_dot(d, d);
	float b = 2 * vec2_dot(f, d);
	float c = vec2_dot(f, f) - r * r;
	float discriminant = b * b - 4 * a * c;
	if (discriminant < 0) { return false; }
	else
	{
		discriminant = sqrtf(discriminant);
		float t1 = (-b - discriminant) / (2 * a);
		float t2 = (-b + discriminant) / (2 * a);
		return (t1 >= 0 && t1 <= 1) || (t2 >= 0 && t2 <= 1) || (t1 <= 0 && t2 >= 1);
	}
}

RuntimeError SampleParametric(Script * script, Statement * statement, float lower, float upper, Camera camera, VertexBuffer * buffer)
{
	list(Parameter) parameters = ListPush(ListCreate(sizeof(Parameter), 1), &(Parameter){ .identifier = "t", .cached = true });
	
	uint32_t length, dimensions;
	parameters[0].cache = (VectorArray){ .length = 1, .dimensions = 1 };
	RuntimeError error = EvaluateExpressionSize(script->identifiers, script->cache, parameters, statement->expression, &length, &dimensions);
	if (error.code != RuntimeErrorNone) { ListFree(parameters); return error; }
	
	// create sample list for each equation
	list(ParametricSample) * samples = malloc(length * sizeof(list(ParametricSample)));
	int32_t baseSampleCount = 7;
	for (int32_t i = 0; i < length; i++) { samples[i] = ListCreate(sizeof(ParametricSample), baseSampleCount); }
	
	// evaluate each base sample across all equations
	for (int32_t j = 0; j <= baseSampleCount; j++)
	{
		float t = (upper - lower) * ((float)j / baseSampleCount) + lower;
		ParametricSample * baseSamples = malloc(length * sizeof(ParametricSample));
		error = EvaluateParametric(script, statement->expression, parameters, t, camera, -1, baseSamples);
		if (error.code != RuntimeErrorNone) { free(baseSamples); goto free; }
		for (int32_t i = 0; i < length; i++) { samples[i] = ListPush(samples[i], &baseSamples[i]); }
		free(baseSamples);
	}
	
	// assign pointer linkage between base samples for each equation
	for (int32_t i = 0; i < length; i++)
	{
		for (int32_t j = 0; j < baseSampleCount; j++) { samples[i][j].next = j + 1; }
	}
	
	// subdivide samples
	int32_t totalVertexCount = length * (baseSampleCount + 1);
	for (int32_t i = 0; i < length; i++)
	{
		int32_t vertexCount = 0;
		for (int32_t j = 0; j < ListLength(samples[i]); j++)
		{
			if (samples[i][j].next == 0) { continue; }
			ParametricSample left = samples[i][j];
			ParametricSample right = samples[i][left.next];
			if (fabsf(left.t - right.t) < 0.0000001) { continue; }
			float segmentLength = vec2_dist(left.screenPosition, right.screenPosition);
			float radius = 1.0 * vec2_len((vec2_t){ camera.aspectRatio, 1.0 });
			float innerDetail = 1.0 / 256.0;
			for (int32_t k = 0; k < 16; k++)
			{
				if (segmentLength > (k == 0 ? 1.0 : 32.0 * expf(k)) * innerDetail && SegmentCircleIntersection(left.screenPosition, right.screenPosition, expf(k) * radius))
				{
					ParametricSample sample;
					error = EvaluateParametric(script, statement->expression, parameters, (left.t + right.t) / 2.0, camera, i, &sample);
					if (error.code != RuntimeErrorNone) { goto free; }
					if (k == 0)
					{
						vec2_t delta1 = vec2_sub(sample.screenPosition, left.screenPosition);
						vec2_t delta2 = vec2_sub(right.screenPosition, sample.screenPosition);
						if (vec2_dot(delta1, delta2) / (vec2_len(delta1) * vec2_len(delta2)) > 1.0 - expf(-200.0 * segmentLength)) { continue; }
					}
					sample.next = left.next;
					samples[i][j].next = ListLength(samples[i]);
					samples[i] = ListPush(samples[i], &sample);
					vertexCount++;
					j--;
					break;
				}
			}
		}
		totalVertexCount += vertexCount;
	}
	
	// create the vertex buffer
	if (totalVertexCount > buffer->vertexCount)
	{
		if (buffer->vertexCount > 0) { VertexBufferFree(*buffer); }
		*buffer = VertexBufferCreate(renderer.layout, totalVertexCount);
	}
	vertex_t * vertices = VertexBufferMapVertices(*buffer);
	int32_t c = 0;
	for (int32_t i = 0; i < length; i++)
	{
		ParametricSample sample = samples[i][0];
		while (true)
		{
			vertices[c++] = (vertex_t)
			{
				.position = sample.position,
				.color = { 0.0, 0.0, 0.0, 1.0 },
				.size = 6.0,
			};
			if (sample.next == 0) { break; }
			sample = samples[i][sample.next];
		}
	}
	buffer->subVertexCount = c;
	VertexBufferUnmapVertices(*buffer);
	VertexBufferUpload(*buffer);
	
free:
	ListFree(parameters);
	for (int32_t i = 0; i < length; i++) { ListFree(samples[i]); }
	free(samples);
	return error;
}
