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
	vec2_t tangent;
	vec2_t concavity;
	vec4_t color;
	float thickness;
	int32_t next;
} ParametricSample;

static RuntimeError EvaluateParametric(Script * script, Expression * expression, list(Parameter) parameters, float t, float dt, Camera camera, int32_t index, ParametricSample * samples)
{
	parameters[0].cache = (VectorArray){ .length = 1, .dimensions = 1 };
	
	VectorArray result, resultdt;
	parameters[0].cache.xyzw[0] = &(float){ t };
	RuntimeError error = EvaluateExpressionIndices(script->identifiers, script->cache, parameters, expression, index < 0 ? NULL : &index, 1, &result);
	parameters[0].cache.xyzw[0] = &(float){ t + dt };
	error = EvaluateExpressionIndices(script->identifiers, script->cache, parameters, expression, index < 0 ? NULL : &index, 1, &resultdt);
	if (error.code != RuntimeErrorNone) { return error; }
	if (result.dimensions != 2 || resultdt.dimensions != 2)
	{
		FreeVectorArray(result);
		FreeVectorArray(resultdt);
		return (RuntimeError){ RuntimeErrorInvalidRenderDimensionality };
	}
	
	for (int32_t i = 0; i < result.length; i++)
	{
		vec2_t pos = (vec2_t){ result.xyzw[0][i], result.xyzw[1][i] };
		vec2_t posdt = (vec2_t){ resultdt.xyzw[0][i], resultdt.xyzw[1][i] };
		samples[i] = (ParametricSample)
		{
			.position = pos,
			.tangent = vec2_normalize(vec2_sub(posdt, pos)),
			.color = (vec4_t){ 0.0, 0.0, 0.0, 1.0 },
			.thickness = 4.0,
			.t = t,
		};
		samples[i].screenPosition = vec2_mul(CameraTransformPoint(camera, samples[i].position), (vec2_t){ camera.aspectRatio, 1.0 });
	}
	
	FreeVectorArray(result);
	FreeVectorArray(resultdt);
	return (RuntimeError){ RuntimeErrorNone };
}

bool SegmentCircleIntersection(vec2_t p1, vec2_t p2, float r)
{
	vec2_t ac = vec2_neg(p1);
	vec2_t ab = vec2_sub(p2, p1);
	vec2_t d = vec2_add(vec2_mulf(ab, vec2_dot(ac, ab) / vec2_dot(ab, ab)), p1);
	vec2_t ad = vec2_sub(d, p1);
	float k = fabsf(ab.x) > fabsf(ab.y) ? ad.x / ab.x : ad.y / ab.y;
	if (k <= 0.0) { return vec2_len(p1) <= r; }
	else if (k >= 1.0) { return vec2_len(p2) <= r; }
	return vec2_len(d) <= r;
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
	int32_t baseSampleCount = 16384 / length;
	if (baseSampleCount < 8) { baseSampleCount = 8; }
	for (int32_t i = 0; i < length; i++) { samples[i] = ListCreate(sizeof(ParametricSample), baseSampleCount); }
	
	// evaluate each base sample across all equations
	for (int32_t j = 0; j <= baseSampleCount; j++)
	{
		float t = (upper - lower) * ((float)j / baseSampleCount) + lower;
		ParametricSample * baseSamples = malloc(length * sizeof(ParametricSample));
		error = EvaluateParametric(script, statement->expression, parameters, t, 0.5 / baseSampleCount, camera, -1, baseSamples);
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
	int32_t totalSampleCount = length * (baseSampleCount + 1);
	for (int32_t i = 0; i < length; i++)
	{
		int32_t sampleCount = 0;
		for (int32_t j = 0; j < ListLength(samples[i]); j++)
		{
			if (samples[i][j].next == 0) { continue; }
			ParametricSample left = samples[i][j];
			ParametricSample right = samples[i][left.next];
			if (fabsf(left.t - right.t) < 1e-7) { continue; }
			float segmentLength = vec2_dist(left.screenPosition, right.screenPosition);
			float radius = vec2_len((vec2_t){ camera.aspectRatio, 1.0 });
			float innerDetail = 1.0 / 128.0;
			if (segmentLength > innerDetail && SegmentCircleIntersection(left.screenPosition, right.screenPosition, radius))
			{
				if (vec2_dot(left.tangent, right.tangent) > 1.0 - 1e-4 / segmentLength) { continue; }
				ParametricSample sample;
				error = EvaluateParametric(script, statement->expression, parameters, (left.t + right.t) / 2.0, fabsf(right.t - left.t) / 4.0, camera, i, &sample);
				if (error.code != RuntimeErrorNone) { goto free; }
				sample.next = left.next;
				samples[i][j].next = ListLength(samples[i]);
				samples[i] = ListPush(samples[i], &sample);
				sampleCount++;
				j--;
			}
		}
		totalSampleCount += sampleCount;
	}
	
	// create the vertex buffer
	if (6 * totalSampleCount > buffer->vertexCount)
	{
		if (buffer->vertexCount > 0) { VertexBufferFree(*buffer); }
		*buffer = VertexBufferCreate(renderer.layout, 6 * totalSampleCount);
	}
	vertex_t * vertices = VertexBufferMapVertices(*buffer);
	int32_t c = 0;
	for (int32_t i = 0; i < length; i++)
	{
		ParametricSample sample = samples[i][0];
		ParametricSample prevSample = sample;
		while (true)
		{
			if (vec2_dist(sample.screenPosition, prevSample.screenPosition) > 10) { prevSample = sample; }
			vertex_t v1 = (vertex_t)
			{
				.position = sample.position,
				.color = sample.color,
				.size = sample.thickness,
				.pair = prevSample.position,
			};
			vertex_t v2 = (vertex_t)
			{
				.position = prevSample.position,
				.color = prevSample.color,
				.size = prevSample.thickness,
				.pair = sample.position,
			};
			vertices[c++] = v1;
			vertices[c++] = v1;
			vertices[c++] = v2;
			vertices[c++] = v2;
			vertices[c++] = v2;
			vertices[c++] = v1;
			if (sample.next == 0) { break; }
			prevSample = sample;
			sample = samples[i][sample.next];
		}
	}
	buffer->subVertexCount = c;
	VertexBufferUnmapVertices(*buffer);
	
free:
	ListFree(parameters);
	for (int32_t i = 0; i < length; i++) { ListFree(samples[i]); }
	free(samples);
	return error;
}
