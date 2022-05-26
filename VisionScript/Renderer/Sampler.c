#include <stdlib.h>
#include <stdio.h>
#include "Sampler.h"
#include "Camera.h"

#define MAX_PARAMETRIC_VERTICES 1048576

static RuntimeError SamplePositions(Environment * environment, Equation equation, RenderObject * object) {
	VectorArray positions;
	RuntimeError error = EvaluateExpression(environment, NULL, equation.expression, &positions);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	if (positions.dimensions != 2) { return (RuntimeError){ RuntimeErrorCodeInvalidRenderDimension, 0, equation.end, equation.line }; }
	
	if (positions.length > object->vertexCount) {
		if (object->vertexCount == 0) { object->vertices = malloc(positions.length * sizeof(vertex_t)); }
		else { object->vertices = realloc(object->vertices, positions.length * sizeof(vertex_t)); }
	}
	for (int32_t i = 0; i < positions.length; i++) {
		object->vertices[i] = (vertex_t) {
			.position = { positions.xyzw[0][i], positions.xyzw[1][i] },
			.color = { 0.0, 0.0, 0.0, 1.0 },
		};
	}
	object->vertexCount = positions.length;
	
	FreeVectorArray(positions);
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError SampleColor(Environment * environment, Equation equation, RenderObject * object) {
	String identifier = StringCreate(equation.declaration.identifier);
	StringConcat(&identifier, ":color");
	Equation * color = GetEnvironmentEquation(environment, identifier);
	StringFree(identifier);
	if (color == NULL) {
		for (int32_t i = 0; i < object->vertexCount; i++) {
			object->vertices[i].color = (vec4_t){ 0.0, 0.0, 0.0, 1.0 };
		}
	} else {
		VectorArray colors;
		RuntimeError error = EvaluateExpression(environment, NULL, color->expression, &colors);
		if (error.code != RuntimeErrorCodeNone) { return error; }
		if (colors.dimensions != 3 && colors.dimensions != 4) {
			FreeVectorArray(colors);
			return (RuntimeError){ RuntimeErrorCodeInvalidColorDimension, color->expression.start, color->expression.end, color->line };
		}
		for (int32_t i = 0; i < object->vertexCount; i++) {
			int32_t index = i < colors.length ? i : colors.length - 1;
			object->vertices[i].color = (vec4_t){
				colors.xyzw[0][index],
				colors.xyzw[1][index],
				colors.xyzw[2][index],
				colors.dimensions == 4 ? colors.xyzw[3][index] : 1.0,
			};
		}
		FreeVectorArray(colors);
	}
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError SampleSize(Environment * environment, Equation equation, RenderObject * object) {
	String identifier = StringCreate(equation.declaration.identifier);
	StringConcat(&identifier, ":size");
	Equation * size = GetEnvironmentEquation(environment, identifier);
	StringFree(identifier);
	if (size == NULL) {
		for (int32_t i = 0; i < object->vertexCount; i++) {
			object->vertices[i].size = 8.0;
		}
	} else {
		VectorArray sizes;
		RuntimeError error = EvaluateExpression(environment, NULL, size->expression, &sizes);
		if (error.code != RuntimeErrorCodeNone) { return error; }
		if (sizes.dimensions != 1) {
			FreeVectorArray(sizes);
			return (RuntimeError){ RuntimeErrorCodeInvalidSizeDimension, size->expression.start, size->expression.end, size->line };
		}
		for (int32_t i = 0; i < object->vertexCount; i++) {
			int32_t index = i < sizes.length ? i : sizes.length - 1;
			object->vertices[i].size = sizes.xyzw[0][index];
		}
		FreeVectorArray(sizes);
	}
	return (RuntimeError){ RuntimeErrorCodeNone };
}

RuntimeError SamplePolygons(Script * script, Equation equation, RenderObject * object) {
	if (!object->needsUpload) {
		RuntimeError error = SamplePositions(&script->environment, equation, object);
		if (error.code != RuntimeErrorCodeNone) { return error; }
		error = SampleColor(&script->environment, equation, object);
		if (error.code != RuntimeErrorCodeNone) { return error; }
		object->needsUpload = true;
	}
	return (RuntimeError){ RuntimeErrorCodeNone };
}

RuntimeError SamplePoints(Script * script, Equation equation, RenderObject * object) {
	if (!object->needsUpload) {
		RuntimeError error = SamplePositions(&script->environment, equation, object);
		if (error.code != RuntimeErrorCodeNone) { return error; }
		error = SampleColor(&script->environment, equation, object);
		if (error.code != RuntimeErrorCodeNone) { return error; }
		error = SampleSize(&script->environment, equation, object);
		if (error.code != RuntimeErrorCodeNone) { return error; }
		object->needsUpload = true;
	}
	return (RuntimeError){ RuntimeErrorCodeNone };
}

typedef struct ParametricSample {
	float t;
	vec2_t position;
	vec2_t screenPosition;
	vec2_t tangent;
	vec4_t color;
	float thickness;
	int32_t next;
} ParametricSample;

static RuntimeError SampleParametricDomain(Environment * environment, Equation equation, float * lower, float * upper) {
	String identifier = StringCreate(equation.declaration.identifier);
	StringConcat(&identifier, ":domain");
	Equation * domain = GetEnvironmentEquation(environment, identifier);
	StringFree(identifier);
	if (domain == NULL) {
		*lower = 0.0;
		*upper = 1.0;
	} else {
		VectorArray value;
		RuntimeError error = EvaluateExpression(environment, NULL, domain->expression, &value);
		if (error.code != RuntimeErrorCodeNone) { return error; }
		if (value.length != 2 || value.dimensions != 1) {
			FreeVectorArray(value);
			return (RuntimeError){ RuntimeErrorCodeInvalidParametricDomain, domain->expression.start, domain->expression.end, domain->expression.line };
		} else {
			*lower = value.xyzw[0][0];
			*upper = value.xyzw[0][1];
			FreeVectorArray(value);
		}
	}
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError SampleParametricPosition(Environment * environment, Equation equation, List(Binding) parameters, float t, float dt, Camera camera, int32_t index, ParametricSample * samples) {
	VectorArray result, resultdt;
	parameters[0].value.xyzw[0] = &t;
	RuntimeError error = EvaluateExpression(environment, parameters, equation.expression, &result);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	parameters[0].value.xyzw[0] = &(float){ t + dt };
	error = EvaluateExpression(environment, parameters, equation.expression, &resultdt);
	if (error.code != RuntimeErrorCodeNone) {
		FreeVectorArray(result);
		return error;
	}
	if (result.dimensions != 2 || resultdt.dimensions != 2) {
		FreeVectorArray(result);
		FreeVectorArray(resultdt);
		return (RuntimeError){ RuntimeErrorCodeInvalidRenderDimension, equation.expression.start, equation.expression.end, equation.line };
	}
	
	if (index >= 0) {
		VectorArray old = result;
		result = VectorArrayAtIndex(old, index);
		FreeVectorArray(old);
		old = resultdt;
		resultdt = VectorArrayAtIndex(old, index);
		FreeVectorArray(old);
	}
	
	for (int32_t i = 0; i < result.length; i++) {
		vec2_t pos = (vec2_t){ result.xyzw[0][i], result.xyzw[1][i] };
		vec2_t posdt = (vec2_t){ resultdt.xyzw[0][i], resultdt.xyzw[1][i] };
		samples[i] = (ParametricSample) {
			.position = pos,
			.tangent = vec2_normalize(vec2_sub(posdt, pos)),
			.thickness = 6.0,
			.t = t,
		};
		samples[i].screenPosition = vec2_mul(CameraTransformPoint(camera, samples[i].position), (vec2_t){ camera.aspectRatio, 1.0 });
	}
	
	FreeVectorArray(result);
	FreeVectorArray(resultdt);
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError SampleParametricColor(Environment * environment, Equation equation, List(Binding) parameters, float t, int32_t index, int32_t sampleCount, ParametricSample * samples) {
	String identifier = StringCreate(equation.declaration.identifier);
	StringConcat(&identifier, ":color");
	Equation * color = GetEnvironmentEquation(environment, identifier);
	StringFree(identifier);
	if (color == NULL) {
		for (int32_t i = 0; i < sampleCount; i++) {
			samples[i].color = (vec4_t){ 0.0, 0.0, 0.0, 1.0 };
		}
	} else  {
		VectorArray colors;
		parameters[0].value.xyzw[0] = &t;
		RuntimeError error = EvaluateExpression(environment, parameters, color->expression, &colors);
		if (error.code != RuntimeErrorCodeNone) { return error; }
		if (colors.dimensions != 3 && colors.dimensions != 4) {
			FreeVectorArray(colors);
			return (RuntimeError){ RuntimeErrorCodeInvalidColorDimension, color->expression.start, color->expression.end, color->line };
		}
		
		for (int32_t i = 0; i < sampleCount; i++) {
			int32_t c = index >= 0 ? index : i;
			if (c > colors.length) { c = colors.length - 1; }
			samples[i].color = (vec4_t){
				colors.xyzw[0][c],
				colors.xyzw[1][c],
				colors.xyzw[2][c],
				colors.dimensions == 4 ? colors.xyzw[3][c] : 1.0,
			};
		}
		FreeVectorArray(colors);
	}
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

RuntimeError SampleParametric(Script * script, Equation equation, Camera camera, RenderObject * object) {
	if (equation.type != EquationTypeFunction || ListLength(equation.declaration.parameters) != 1) {
		return (RuntimeError){ RuntimeErrorCodeInvalidParametricEquation, 0, equation.end, equation.line };
	}
	
	float lower, upper;
	RuntimeError error = SampleParametricDomain(&script->environment, equation, &lower, &upper);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	
	List(Binding) parameters = ListPush(ListCreate(sizeof(Binding), 1), &(Binding){ 0 });
	parameters[0].identifier = equation.declaration.parameters[0];
	parameters[0].value = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = &lower };
	
	VectorArray initial;
	error = EvaluateExpression(&script->environment, parameters, equation.expression, &initial);
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
		error = SampleParametricPosition(&script->environment, equation, parameters, t, 0.5 / baseSampleCount, camera, -1, baseSamples);
		if (error.code != RuntimeErrorCodeNone) {
			free(baseSamples);
			goto free;
		}
		error = SampleParametricColor(&script->environment, equation, parameters, t, -1, initial.length, baseSamples);
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
				error = SampleParametricPosition(&script->environment, equation, parameters, (left.t + right.t) / 2.0, fabsf(right.t - left.t) / 4.0, camera, i, &sample);
				if (error.code != RuntimeErrorCodeNone) { goto free; }
				error = SampleParametricColor(&script->environment, equation, parameters, (left.t + right.t) / 2.0, i, 1, &sample);
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
	
free:
	ListFree(parameters);
	for (int32_t i = 0; i < initial.length; i++) { ListFree(samples[i]); }
	free(samples);
	return error;
}
