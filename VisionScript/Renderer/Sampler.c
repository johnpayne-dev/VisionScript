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
				.size = 16.0,
			};
		}
		object->vertexCount = result.length;
		object->needsUpload = true;
	}
	
	FreeVectorArray(result);
	return (RuntimeError){ RuntimeErrorCodeNone };
}

typedef struct ParametricSample {
	float t;
	vec2_t position;
	vec2_t screenPosition;
	vec2_t tangent;
	vec2_t concavity;
	vec4_t color;
	float thickness;
	int32_t next;
} ParametricSample;

static RuntimeError EvaluateParametric(Script * script, Expression expression, List(Binding) parameters, float t, float dt, Camera camera, int32_t index, ParametricSample * samples) {
	parameters[0].value = (VectorArray){ .length = 1, .dimensions = 1 };
	
	VectorArray result, resultdt;
	parameters[0].value.xyzw[0] = &(float){ t };
	RuntimeError error = EvaluateExpression(&script->environment, parameters, expression, &result);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	
	parameters[0].value.xyzw[0] = &(float){ t + dt };
	error = EvaluateExpression(&script->environment, parameters, expression, &resultdt);
	if (error.code != RuntimeErrorCodeNone) {
		FreeVectorArray(result);
		return error;
	}
	if (result.dimensions != 2 || resultdt.dimensions != 2) {
		FreeVectorArray(result);
		FreeVectorArray(resultdt);
		return (RuntimeError){ RuntimeErrorCodeInvalidRenderDimensionality, expression.start, expression.end, expression.line };
	}
	
	if (index < 0) {
		for (int32_t i = 0; i < result.length; i++) {
			vec2_t pos = (vec2_t){ result.xyzw[0][i], result.xyzw[1][i] };
			vec2_t posdt = (vec2_t){ resultdt.xyzw[0][i], resultdt.xyzw[1][i] };
			samples[i] = (ParametricSample) {
				.position = pos,
				.tangent = vec2_normalize(vec2_sub(posdt, pos)),
				.color = (vec4_t){ 0.0, 0.0, 0.0, 1.0 },
				.thickness = 6.0,
				.t = t,
			};
			samples[i].screenPosition = vec2_mul(CameraTransformPoint(camera, samples[i].position), (vec2_t){ camera.aspectRatio, 1.0 });
		}
	} else {
		vec2_t pos = (vec2_t){ result.xyzw[0][index], result.xyzw[1][index] };
		vec2_t posdt = (vec2_t){ resultdt.xyzw[0][index], resultdt.xyzw[1][index] };
		samples[0] = (ParametricSample) {
			.position = pos,
			.tangent = vec2_normalize(vec2_sub(posdt, pos)),
			.color = (vec4_t){ 0.0, 0.0, 0.0, 1.0 },
			.thickness = 6.0,
			.t = t,
		};
		samples[0].screenPosition = vec2_mul(CameraTransformPoint(camera, samples[0].position), (vec2_t){ camera.aspectRatio, 1.0 });
	}
	
	FreeVectorArray(result);
	FreeVectorArray(resultdt);
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static bool SegmentCircleIntersection(vec2_t p1, vec2_t p2, float r) {
	vec2_t ac = vec2_neg(p1);
	vec2_t ab = vec2_sub(p2, p1);
	vec2_t d = vec2_add(vec2_mulf(ab, vec2_dot(ac, ab) / vec2_dot(ab, ab)), p1);
	vec2_t ad = vec2_sub(d, p1);
	float k = fabsf(ab.x) > fabsf(ab.y) ? ad.x / ab.x : ad.y / ab.y;
	if (k <= 0.0) { return vec2_len(p1) <= r; }
	else if (k >= 1.0) { return vec2_len(p2) <= r; }
	return vec2_len(d) <= r;
}

RuntimeError SampleParametric(Script * script, Equation equation, float lower, float upper, Camera camera, RenderObject * object) {
	List(Binding) parameters = ListPush(ListCreate(sizeof(Binding), 1), &(Binding){ 0 });
	parameters[0].identifier = StringCreate("t");
	parameters[0].value = (VectorArray){ .length = 1, .dimensions = 1 };
	
	VectorArray initial;
	parameters[0].value.xyzw[0] = &lower;
	RuntimeError error = EvaluateExpression(&script->environment, parameters, equation.expression, &initial);
	if (error.code != RuntimeErrorCodeNone) {
		ListFree(parameters);
		return error;
	}
	
	// create sample list for each equation
	List(ParametricSample) * samples = malloc(initial.length * sizeof(List(ParametricSample)));
	int32_t baseSampleCount = 16384 / initial.length;
	if (baseSampleCount < 8) { baseSampleCount = 8; }
	for (int32_t i = 0; i < initial.length; i++) { samples[i] = ListCreate(sizeof(ParametricSample), baseSampleCount); }
	
	// evaluate each base sample across all equations
	for (int32_t j = 0; j <= baseSampleCount; j++) {
		float t = (upper - lower) * ((float)j / baseSampleCount) + lower;
		ParametricSample * baseSamples = malloc(initial.length * sizeof(ParametricSample));
		error = EvaluateParametric(script, equation.expression, parameters, t, 0.5 / baseSampleCount, camera, -1, baseSamples);
		if (error.code != RuntimeErrorCodeNone) {
			free(baseSamples);
			goto free;
		}
		for (int32_t i = 0; i < initial.length; i++) { samples[i] = ListPush(samples[i], &baseSamples[i]); }
		free(baseSamples);
	}
	
	// assign pointer linkage between base samples for each equation
	for (int32_t i = 0; i < initial.length; i++) {
		for (int32_t j = 0; j < baseSampleCount; j++) { samples[i][j].next = j + 1; }
	}
	
	// subdivide samples
	int32_t totalSampleCount = initial.length * (baseSampleCount + 1);
	for (int32_t i = 0; i < initial.length; i++) {
		int32_t sampleCount = 0;
		for (int32_t j = 0; j < ListLength(samples[i]); j++) {
			if (samples[i][j].next == 0) { continue; }
			ParametricSample left = samples[i][j];
			ParametricSample right = samples[i][left.next];
			if (fabsf(left.t - right.t) < 1e-7) { continue; }
			float segmentLength = vec2_dist(left.screenPosition, right.screenPosition);
			float radius = vec2_len((vec2_t){ camera.aspectRatio, 1.0 });
			float innerDetail = 1.0 / 128.0;
			if (segmentLength > innerDetail && SegmentCircleIntersection(left.screenPosition, right.screenPosition, radius)) {
				if (vec2_dot(left.tangent, right.tangent) > 1.0 - 1e-4 / segmentLength) { continue; }
				ParametricSample sample;
				error = EvaluateParametric(script, equation.expression, parameters, (left.t + right.t) / 2.0, fabsf(right.t - left.t) / 4.0, camera, i, &sample);
				if (error.code != RuntimeErrorCodeNone) { goto free; }
				sample.next = left.next;
				samples[i][j].next = ListLength(samples[i]);
				samples[i] = ListPush(samples[i], &sample);
				sampleCount++;
				j--;
			}
		}
		totalSampleCount += sampleCount;
	}
	
	if (!object->needsUpload) {
		if (6 * totalSampleCount > object->vertexCount) {
			if (object->vertexCount == 0) { object->vertices = malloc(6 * totalSampleCount* sizeof(vertex_t)); }
			else { object->vertices = realloc(object->vertices, 6 * totalSampleCount * sizeof(vertex_t)); }
		}
		
		int32_t c = 0;
		for (int32_t i = 0; i < initial.length; i++) {
			ParametricSample sample = samples[i][0];
			ParametricSample prevSample = sample;
			while (true) {
				if (vec2_dist(sample.screenPosition, prevSample.screenPosition) > 10) { prevSample = sample; }
				vertex_t v1 = (vertex_t) {
					.position = sample.position,
					.color = sample.color,
					.size = sample.thickness,
					.pair = prevSample.position,
				};
				vertex_t v2 = (vertex_t) {
					.position = prevSample.position,
					.color = prevSample.color,
					.size = prevSample.thickness,
					.pair = sample.position,
				};
				object->vertices[c++] = v1;
				object->vertices[c++] = v1;
				object->vertices[c++] = v2;
				object->vertices[c++] = v2;
				object->vertices[c++] = v2;
				object->vertices[c++] = v1;
				if (sample.next == 0) { break; }
				prevSample = sample;
				sample = samples[i][sample.next];
			}
		}
		object->vertexCount = 6 * totalSampleCount;
		object->needsUpload = true;
	}
	
free:
	ListFree(parameters);
	for (int32_t i = 0; i < initial.length; i++) { ListFree(samples[i]); }
	free(samples);
	return error;
}
