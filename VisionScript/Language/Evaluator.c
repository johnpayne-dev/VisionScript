#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "Evaluator.h"

const char * RuntimeErrorToString(RuntimeErrorCode code) {
	switch (code) {
		case RuntimeErrorCodeNone: return "no error";
		case RuntimeErrorCodeInvalidExpression: return "trying to evaluate invalid expression";
		case RuntimeErrorCodeInvalidArgumentsPlacement: return "unable to evaluate arguments in this context";
		case RuntimeErrorCodeInvalidAssignmentPlacement: return "unable to evaluate for assignment in this context";
		default: return "unknown error";
	}
}

void PrintRuntimeError(RuntimeError error);

void PrintVectorArray(VectorArray value) {
	if (value.length > 1) { printf("["); }
	for (int32_t i = 0; i < value.length; i++) {
		if (value.dimensions > 1) { printf("("); }
		for (int32_t j = 0; j < value.dimensions; j++) {
			// print as an integer if float is an integer
			if (value.xyzw[j][i] < LLONG_MAX && value.xyzw[j][i] - floorf(value.xyzw[j][i]) == 0) { printf("%lld", (long long)value.xyzw[j][i]); }
			else { printf("%f", value.xyzw[j][i]); }
			if (j != value.dimensions - 1) { printf(","); }
		}
		if (value.dimensions > 1) { printf(")"); }
		if (i != value.length - 1) { printf(", "); }
		
		// skip to the end if there's too many elements
		if (i == 9 && value.length > 100) {
			printf("..., ");
			i = value.length - 5;
		}
	}
	if (value.length > 1) { printf("]"); }
}

VectorArray CopyVectorArray(VectorArray value) {
	VectorArray result = { .length = value.length, .dimensions = value.dimensions };
	for (int32_t d = 0; d < result.dimensions; d++) {
		result.xyzw[d] = malloc(result.length * sizeof(scalar_t));
		memcpy(result.xyzw[d], value.xyzw[d], result.length * sizeof(scalar_t));
	}
	return result;
}

void FreeVectorArray(VectorArray value) {
	for (int32_t d = 0; d < value.dimensions; d++) { free(value.xyzw[d]); }
}

Binding CreateBinding(const char * identifier, VectorArray value) {
	return (Binding){ .identifier = StringCreate(identifier), .value = CopyVectorArray(value) };
}

void FreeBinding(Binding binding) {
	StringFree(binding.identifier);
	FreeVectorArray(binding.value);
}

Environment CreateEmptyEnvironment() {
	return (Environment){
		.equations = ListCreate(sizeof(Equation), 1),
		.cache = ListCreate(sizeof(Binding), 1),
	};
}

void SetEnvironmentEquation(Environment * environment, Equation equation) {
	bool replaced = false;
	for (int32_t i = 0; i < ListLength(environment->equations); i++) {
		if (StringEquals(equation.declaration.identifier, environment->equations[i].declaration.identifier)) {
			FreeEquation(environment->equations[i]);
			environment->equations[i] = equation;
			replaced = true;
			break;
		}
	}
	if (!replaced) { environment->equations = ListPush(environment->equations, &equation); }
}

void SetEnvironmentCache(Environment * environment, Binding binding) {
	bool replaced = false;
	for (int32_t i = 0; i < ListLength(environment->cache); i++) {
		if (StringEquals(binding.identifier, environment->cache[i].identifier)) {
			FreeBinding(environment->cache[i]);
			environment->cache[i] = binding;
			replaced = true;
			break;
		}
	}
	if (!replaced) { environment->cache = ListPush(environment->cache, &binding); }
}

void FreeEnvironment(Environment environment) {
	for (int32_t i = 0; i < ListLength(environment.equations); i++) { FreeEquation(environment.equations[i]); }
	ListFree(environment.equations);
	for (int32_t i = 0; i < ListLength(environment.cache); i++) { FreeBinding(environment.cache[i]); }
	ListFree(environment.cache);
}

static RuntimeError EvaluateConstant(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	result->length = 1;
	result->dimensions = 1;
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = expression.constant;
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError EvaluateIdentifier(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	if (parameters != NULL) {
		for (int32_t i = 0; i < ListLength(parameters); i++) {
			if (StringEquals(parameters[i].identifier, expression.identifier)) {
				*result = CopyVectorArray(parameters[i].value);
				return (RuntimeError){ RuntimeErrorCodeNone };
			}
		}
	}
	
	for (int32_t i = 0; i < ListLength(environment->cache); i++) {
		if (StringEquals(environment->cache[i].identifier, expression.identifier)) {
			*result = CopyVectorArray(environment->cache[i].value);
			return (RuntimeError){ RuntimeErrorCodeNone };
		}
	}
	
	for (int32_t i = 0; i < ListLength(environment->equations); i++) {
		if (StringEquals(environment->equations[i].declaration.identifier, expression.identifier)) {
			if (environment->equations[i].type == EquationTypeFunction) { return (RuntimeError){ RuntimeErrorCodeIdentifierNotVariable, expression.start, expression.end }; }
			RuntimeError error = EvaluateExpression(environment, NULL, environment->equations[i].expression, result);
			if (error.code == RuntimeErrorCodeNone) { SetEnvironmentCache(environment, CreateBinding(expression.identifier, *result)); }
			return error;
		}
	}
	
	return (RuntimeError){ RuntimeErrorCodeUndefinedIdentifier, expression.start, expression.end };
}

static RuntimeError EvaluateVectorLiteral(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	if (ListLength(expression.list) > 4) { return (RuntimeError){ RuntimeErrorCodeTooManyVectorElements, expression.start, expression.end }; }
	result->dimensions = ListLength(expression.list);
	result->length = -1; // uint -1
	
	VectorArray components[4];
	for (int32_t i = 0; i < result->dimensions; i++) {
		RuntimeError error = EvaluateExpression(environment, parameters, expression.list[i], &components[i]);
		if (error.code != RuntimeErrorCodeNone) { return error; }
		if (components[i].dimensions > 1) { return (RuntimeError){ RuntimeErrorCodeVectorInsideVector, expression.list[i].start, expression.list[i].end }; }
		
		// length is set to the smallest length of each component not including length 1 (since length 1 will assume length of the rest of the vector)
		if (components[i].length > 1) { result->length = components[i].length < result->length ? components[i].length : result->length; }
		result->xyzw[i] = components[i].xyzw[0];
	}
	if (result->length == -1) { result->length = 1; } // if all the component lengths are 1 then result->length will still be -1, so set it to 1
	
	// go through all the components of length 1 extend it to be same length as the rest of the vector
	for (int32_t i = 0; i < result->dimensions; i++) {
		if (components[i].length == 1 && result->length > 1) {
			result->xyzw[i] = malloc(sizeof(scalar_t) * result->length);
			for (int32_t j = 0; j < result->length; j++) { result->xyzw[i][j] = components[i].xyzw[0][0]; }
			free(components[i].xyzw[0]);
		}
	}
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError EvaluateArrayLiteral(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	return (RuntimeError){ RuntimeErrorCodeNotImplemented };
}

static RuntimeError EvaluateUnary(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	return (RuntimeError){ RuntimeErrorCodeNotImplemented };
}

static RuntimeError EvaluateBinary(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	return (RuntimeError){ RuntimeErrorCodeNotImplemented };
}

static RuntimeError EvaluateTernary(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	return (RuntimeError){ RuntimeErrorCodeNotImplemented };
}

RuntimeError EvaluateExpression(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	switch (expression.type) {
		case ExpressionTypeUnknown: return (RuntimeError){ RuntimeErrorCodeInvalidExpression, expression.start, expression.end };
		case ExpressionTypeConstant: return EvaluateConstant(environment, parameters, expression, result);
		case ExpressionTypeIdentifier: return EvaluateIdentifier(environment, parameters, expression, result);
		case ExpressionTypeVectorLiteral: return EvaluateVectorLiteral(environment, parameters, expression, result);
		case ExpressionTypeArrayLiteral: return EvaluateArrayLiteral(environment, parameters, expression, result);
		case ExpressionTypeArguments: return (RuntimeError){ RuntimeErrorCodeInvalidArgumentsPlacement, expression.start, expression.end };
		case ExpressionTypeForAssignment: return (RuntimeError){ RuntimeErrorCodeInvalidAssignmentPlacement, expression.start, expression.end };
		case ExpressionTypeUnary: return EvaluateUnary(environment, parameters, expression, result);
		case ExpressionTypeBinary: return EvaluateBinary(environment, parameters, expression, result);
		case ExpressionTypeTernary: return EvaluateTernary(environment, parameters, expression, result);
	}
}
