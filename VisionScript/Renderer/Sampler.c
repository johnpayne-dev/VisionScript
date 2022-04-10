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

static RuntimeError EvaluateParametric(Script * script, Expression * expression, list(Parameter) parameters, float t, int32_t index, ParametricSample * samples)
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
		samples[i].screenPosition = CameraTransformPoint(renderer.camera, samples[i].position);
		samples[i].screenPosition = vec2_mul(samples[i].screenPosition, (vec2_t){ renderer.camera.aspectRatio, 1.0 });
	}
	
	FreeVectorArray(result);
	return (RuntimeError){ RuntimeErrorNone };
}

bool SegmentCircleIntersection(vec2_t a, vec2_t b, float r)
{
	vec2_t ac = vec2_neg(a);
	vec2_t ab = vec2_sub(b, a);
	vec2_t d = vec2_add(a, vec2_mulf(vec2_normalize(ab), vec2_dot(ac, ab)));
	vec2_t ad = vec2_sub(d, a);
	float k = fabsf(ab.x) > fabsf(ab.y) ? ad.x / ab.x : ad.y / ab.y;
	if (k <= 0.0) { return vec2_len(a) < r; }
	if (k >= 1.0) { return vec2_len(b) < r; }
	return vec2_len(d) < r;
}

RuntimeError SampleParametric(Script * script, Statement * statement, float lower, float upper, VertexBuffer * buffer)
{
	list(Parameter) parameters = ListPush(ListCreate(sizeof(Parameter), 1), &(Parameter){ .identifier = "t", .cached = true });
	
	uint32_t length, dimensions;
	parameters[0].cache = (VectorArray){ .length = 1, .dimensions = 1 };
	RuntimeError error = EvaluateExpressionSize(script->identifiers, script->cache, parameters, statement->expression, &length, &dimensions);
	if (error.code != RuntimeErrorNone) { ListFree(parameters); return error; }
	
	// create sample list for each equation
	list(ParametricSample) * samples = malloc(length * sizeof(list(ParametricSample)));
	int32_t baseSampleCount = 4;
	for (int32_t i = 0; i < length; i++) { samples[i] = ListCreate(sizeof(ParametricSample), baseSampleCount); }
	
	// evaluate each base sample across all equations
	for (int32_t j = 0; j <= baseSampleCount; j++)
	{
		ParametricSample * baseSamples = malloc(length * sizeof(ParametricSample));
		error = EvaluateParametric(script, statement->expression, parameters, (upper - lower) * (float)j / baseSampleCount + lower, -1, baseSamples);
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
			float radius = 1.0 * vec2_len((vec2_t){ renderer.camera.aspectRatio, 1.0 });
			float innerDetail = 3.0 / 720.0;
			for (int32_t k = 0; k < 16; k++)
			{
				if (segmentLength > expf(1.5 * k) * innerDetail && SegmentCircleIntersection(left.screenPosition, right.screenPosition, expf(k) * radius))
				{
					ParametricSample sample;
					error = EvaluateParametric(script, statement->expression, parameters, (left.t + right.t) / 2.0, i, &sample);
					if (error.code != RuntimeErrorNone) { goto free; }
					if (k == 0 || k == 1 || k == 2)
					{
						vec2_t delta1 = vec2_sub(sample.screenPosition, left.screenPosition);
						vec2_t delta2 = vec2_sub(right.screenPosition, sample.screenPosition);
						if (vec2_dot(delta1, delta2) / (vec2_len(delta1) * vec2_len(delta2)) > 1.0 - 0.01 * expf(-4.0 * k)) { continue; }
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
		while (sample.next != 0)
		{
			vertices[c] = (vertex_t)
			{
				.position = sample.position,
				.color = { 0.0, 0.0, 0.0, 1.0 },
				.size = 6.0,
			};
			sample = samples[i][sample.next];
			c++;
		}
	}
	buffer->subVertexCount = c;
	VertexBufferUnmapVertices(*buffer);
	VertexBufferUpload(*buffer);
	
free:
	ListFree(parameters);
	for (int32_t i = 0; i < length; i++) { ListFree(samples[i]); }
	free(samples);
	return (RuntimeError){ RuntimeErrorNone };
}
