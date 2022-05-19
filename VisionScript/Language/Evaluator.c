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
		case RuntimeErrorCodeInvalidForAssignmentPlacement: return "unable to evaluate for assignment in this context";
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

static bool IsNestedListAllowed(Expression expression) {
	if (expression.type == ExpressionTypeBinary) { return expression.binary.operator == OperatorFor || expression.binary.operator == OperatorRange; }
	if (expression.type == ExpressionTypeTernary) { return expression.ternary.leftOperator == OperatorFor; }
	return false;
}

static RuntimeError EvaluateRange(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	VectorArray left, right;
	RuntimeError error = EvaluateExpression(environment, parameters, *expression.binary.left, &left);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	error = EvaluateExpression(environment, parameters, *expression.binary.right, &right);
	if (error.code != RuntimeErrorCodeNone) {
		FreeVectorArray(left);
		return error;
	}
	if (left.length != 1) { return (RuntimeError){ RuntimeErrorCodeInvalidRangeOperon, expression.binary.left->start, expression.binary.left->end }; }
	if (right.length != 1) { return (RuntimeError){ RuntimeErrorCodeInvalidRangeOperon, expression.binary.right->start, expression.binary.right->end }; }
	if (left.dimensions != right.dimensions) { return (RuntimeError){ RuntimeErrorCodeNonUniformRange, expression.start, expression.end }; }
	
	result->dimensions = left.dimensions;
	result->length = 1;
	for (int32_t i = 0; i < result->dimensions; i++) {
		result->length *= fabsf(roundf(right.xyzw[i][0]) - roundf(left.xyzw[i][0])) + 1;
	}
	
	for (int32_t i = 0, p = 1; i < result->dimensions; i++) {
		result->xyzw[i] = malloc(sizeof(scalar_t) * result->length);
		int32_t start = roundf(left.xyzw[i][0]);
		int32_t end = roundf(right.xyzw[i][0]);
		int32_t len = abs(end - start) + 1;
		if (start <= end) {
			for (int32_t j = 0; j < result->length; j++) { result->xyzw[i][j] = (j / p) % len + start; }
		} else {
			for (int32_t j = 0; j < result->length; j++) { result->xyzw[i][j] = start - (j / p) % len; }
		}
		p *= len;
	}
	
	FreeVectorArray(left);
	FreeVectorArray(right);
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError EvaluateFor(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	if (expression.type == ExpressionTypeTernary) {
		
	}
	if (expression.type == ExpressionTypeBinary) {
		if (expression.binary.right->type != ExpressionTypeForAssignment) {
			return (RuntimeError){ RuntimeErrorCodeInvalidForAssignment, expression.binary.right->start, expression.binary.right->end };
		}
		VectorArray assignment;
		RuntimeError error = EvaluateExpression(environment, parameters, *expression.binary.right->assignment.expression, &assignment);
		if (error.code != RuntimeErrorCodeNone) { return error; }
		
		if (parameters == NULL) { parameters = ListCreate(sizeof(Binding), 1); }
		else { parameters = ListClone(parameters); }
		parameters = ListInsert(parameters, &(Binding){ .identifier = expression.binary.right->assignment.identifier }, 0);
		
		result->dimensions = 0;
		result->length = 0;
		VectorArray * values = malloc(assignment.length * sizeof(VectorArray));
		for (int32_t i = 0; i < assignment.length; i++) {
			parameters[0].value.length = 1;
			parameters[0].value.dimensions = assignment.dimensions;
			for (int32_t j = 0; j < assignment.dimensions; j++) { parameters[0].value.xyzw[j] = &assignment.xyzw[j][i]; }
			
			RuntimeError error;
			if (expression.binary.left->type == ExpressionTypeBinary && expression.binary.left->binary.operator == OperatorRange) {
				error = EvaluateRange(environment, parameters, *expression.binary.left, &values[i]);
			} else if (expression.binary.left->type == ExpressionTypeBinary && expression.binary.left->binary.operator == OperatorFor) {
				error = EvaluateFor(environment, parameters, *expression.binary.left, &values[i]);
			} else if (expression.binary.left->type == ExpressionTypeTernary && expression.binary.left->ternary.leftOperator == OperatorFor) {
				error = EvaluateFor(environment, parameters, *expression.binary.left, &values[i]);
			} else {
				error = EvaluateExpression(environment, parameters, *expression.binary.left, &values[i]);
			}
			if (error.code != RuntimeErrorCodeNone) { goto free; }
			if (result->dimensions == 0) { result->dimensions = values[i].dimensions; }
			if (values[i].dimensions != result->dimensions) {
				error = (RuntimeError){ RuntimeErrorCodeNonUniformArray, expression.binary.left->start, expression.binary.left->end };
				goto free;
			}
			if (values[i].length > 1 && !IsNestedListAllowed(*expression.binary.left)) {
				error = (RuntimeError){ RuntimeErrorCodeArrayInsideArray, expression.binary.left->start, expression.binary.left->end };
				goto free;
			}
			
			result->length += values[i].length;
			continue;
		free:
			FreeVectorArray(assignment);
			for (int32_t j = 0; j < i; j++) { FreeVectorArray(values[j]); }
			ListFree(parameters);
			return error;
		}
		
		for (int32_t i = 0; i < result->dimensions; i++) {
			result->xyzw[i] = malloc(result->length * sizeof(scalar_t));
			for (int32_t j = 0, p = 0; j < assignment.length; j++) {
				memcpy(result->xyzw[i] + p, values[j].xyzw[i], values[j].length * sizeof(scalar_t));
				p += values[j].length;
				free(values[j].xyzw[i]);
			}
		}
		free(values);
		FreeVectorArray(assignment);
		ListFree(parameters);
		
		return (RuntimeError){ RuntimeErrorCodeNone };
	}
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError EvaluateArrayLiteral(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	result->dimensions = 0;
	result->length = 0;
	
	// evaluate each element of the array and store them in elements temporarily
	VectorArray * elements = malloc(sizeof(VectorArray) * ListLength(expression.list));
	for (int32_t i = 0; i < ListLength(expression.list); i++) {
		RuntimeError error;
		if (expression.list[i].type == ExpressionTypeBinary && expression.list[i].binary.operator == OperatorRange) {
			error = EvaluateRange(environment, parameters, expression.list[i], &elements[i]);
		} else if (expression.list[i].type == ExpressionTypeBinary && expression.list[i].binary.operator == OperatorFor) {
			error = EvaluateFor(environment, parameters, expression.list[i], &elements[i]);
		} else if (expression.list[i].type == ExpressionTypeTernary && expression.list[i].ternary.leftOperator == OperatorFor) {
			error = EvaluateFor(environment, parameters, expression.list[i], &elements[i]);
		} else {
			error = EvaluateExpression(environment, parameters, expression.list[i], &elements[i]);
		}
		if (error.code != RuntimeErrorCodeNone) { goto free; }
		
		if (result->dimensions == 0) { result->dimensions = elements[i].dimensions; } // dimension of array is defined to be dimension of the first element
		if (elements[i].dimensions != result->dimensions) {
			error = (RuntimeError){ RuntimeErrorCodeNonUniformArray, expression.list[i].start, expression.list[i].end };
			goto free;
		}
		// if an element's length > 1 and it's not from range or for operator, then that means there's an array inside array
		if (elements[i].length > 1 && !IsNestedListAllowed(expression.list[i])) {
			error = (RuntimeError){ RuntimeErrorCodeArrayInsideArray, expression.list[i].start, expression.list[i].end };
			goto free;
		}
		result->length += elements[i].length;
		continue;
	free:
		for (int32_t j = 0; j < i; j++) { FreeVectorArray(elements[j]); }
		free(elements);
		return error;
	}
	
	for (int32_t i = 0; i < result->dimensions; i++) {
		if (ListLength(expression.list) == 1) { result->xyzw[i] = elements[0].xyzw[i]; } // if there's only one element then just move the pointer to save time
		else {
			result->xyzw[i] = malloc(sizeof(scalar_t) * result->length);
			for (int32_t j = 0, p = 0; j < ListLength(expression.list); j++) {
				memcpy(result->xyzw[i] + p, elements[j].xyzw[i], elements[j].length * sizeof(scalar_t));
				p += elements[j].length;
				free(elements[j].xyzw[i]);
			}
		}
	}
	
	free(elements);
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static inline scalar_t ApplyUnaryArithmetic(scalar_t value, Operator operator) {
	switch (operator) {
		case OperatorNegate: return -value;
		case OperatorNot: return !value;
		case OperatorFactorial: return tgammaf(value + 1.0);
		default: return NAN;
	}
}

static RuntimeError EvaluateUnary(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	RuntimeError error = EvaluateExpression(environment, parameters, *expression.unary.expression, result);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	for (int32_t i = 0; i < result->dimensions; i++) {
		for (int32_t j = 0; j < result->length; j++) {
			result->xyzw[i][j] = ApplyUnaryArithmetic(result->xyzw[i][j], expression.unary.operator);
		}
	}
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static bool IsIdentifierSwizzling(String identifier) {
	for (int32_t i = 0; i < StringLength(identifier); i++) {
		if (i >= 4 || (identifier[i] != 'x' && identifier[i] != 'y' && identifier[i] != 'z' && identifier[i] != 'w')) { return false; }
	}
	return true;
}

static RuntimeError EvaluateDimension(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	if (expression.binary.right->type != ExpressionTypeIdentifier || !IsIdentifierSwizzling(expression.binary.right->identifier)) {
		return (RuntimeError){ RuntimeErrorCodeInvalidDimensionOperon, expression.binary.right->start, expression.binary.left->start };
	}
	
	VectorArray indexed;
	RuntimeError error = EvaluateExpression(environment, parameters, *expression.binary.left, &indexed);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	
	result->dimensions = StringLength(expression.binary.right->identifier);
	result->length = indexed.length;
	bool shouldDuplicate[4] = { false, false, false, false }; // keeps track of duplicate vectors (e.g. .xxyz)
	for (int32_t i = 0; i < result->dimensions; i++) {
		int32_t component = expression.binary.right->identifier[i] - 'x';
		if (component >= indexed.dimensions) {
			FreeVectorArray(indexed);
			return (RuntimeError){ RuntimeErrorCodeInvalidSwizzling, expression.binary.right->start, expression.binary.right->end };
		}
		
		if (!shouldDuplicate[component]) {
			result->xyzw[i] = indexed.xyzw[component];
			shouldDuplicate[component] = true;
		} else {
			result->xyzw[i] = malloc(result->length * sizeof(scalar_t));
			memcpy(result->xyzw[i], indexed.xyzw[component], result->length * sizeof(scalar_t));
		}
 	}
	
	for (int32_t i = 0; i < indexed.dimensions; i++) {
		if (!shouldDuplicate[i]) { free(indexed.xyzw[i]); }
	}
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError EvaluateIndex(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	VectorArray indexed, indices;
	RuntimeError error = EvaluateExpression(environment, parameters, *expression.binary.right, &indices);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	if (indices.dimensions > 1) { return (RuntimeError){ RuntimeErrorCodeInvalidIndexDimension, expression.binary.right->start, expression.binary.right->end }; }
	error = EvaluateExpression(environment, parameters, *expression.binary.left, &indexed);
	if (error.code != RuntimeErrorCodeNone) {
		FreeVectorArray(indices);
		return error;
	}
	
	result->length = indices.length;
	result->dimensions = indexed.dimensions;
	for (int32_t i = 0; i < result->dimensions; i++) {
		result->xyzw[i] = malloc(result->length * sizeof(scalar_t));
		for (int32_t j = 0; j < indices.length; j++) {
			int32_t index = roundf(indices.xyzw[0][j]);
			if (index < 0 || index >= indexed.length) { result->xyzw[i][j] = NAN; }
			else { result->xyzw[i][j] = indexed.xyzw[i][index]; }
		}
	}
	
	FreeVectorArray(indexed);
	FreeVectorArray(indices);
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError EvaluateArguments(Environment * environment, list(Binding) parameters, Expression expression, list(String) variables, list(Binding) * arguments) {
	if (expression.binary.right->type != ExpressionTypeArguments) {
		return (RuntimeError){ RuntimeErrorCodeInvalidArgumentsExpression, expression.binary.right->start, expression.binary.right->end };
	}
	if (ListLength(variables) != ListLength(expression.binary.right->list)) {
		return (RuntimeError){ RuntimeErrorCodeIncorrectArgumentCount, expression.binary.right->start, expression.binary.right->end };
	}
	
	for (int32_t i = 0; i < ListLength(expression.binary.right->list); i++) {
		VectorArray argument;
		RuntimeError error = EvaluateExpression(environment, parameters, expression.binary.right->list[i], &argument);
		if (error.code != RuntimeErrorCodeNone) {
			for (int32_t j = 0; j < i; j++) { FreeBinding((*arguments)[j]); }
			return error;
		}
		*arguments = ListPush(*arguments, &(Binding){ .identifier = StringCreate(variables[i]), .value = argument });
	}
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError EvaluateCall(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	if (expression.binary.left->type != ExpressionTypeIdentifier) {
		return (RuntimeError){ RuntimeErrorCodeUncallableExpression, expression.binary.left->start, expression.binary.left->end };
	}
	
	for (int32_t i = 0; i < ListLength(environment->equations); i++) {
		if (StringEquals(environment->equations[i].declaration.identifier, expression.binary.left->identifier)) {
			if (environment->equations[i].type == EquationTypeVariable) {
				return (RuntimeError){ RuntimeErrorCodeIdentifierNotFunction, expression.binary.left->start, expression.binary.left->end };
			}
			
			list(Binding) arguments = ListCreate(sizeof(Binding), 1);
			RuntimeError error = EvaluateArguments(environment, parameters, expression, environment->equations[i].declaration.parameters, &arguments);
			if (error.code == RuntimeErrorCodeNone) {
				error = EvaluateExpression(environment, arguments, environment->equations[i].expression, result);
			}
			for (int32_t j = 0; j < ListLength(arguments); j++) { FreeBinding(arguments[j]); }
			ListFree(arguments);
			return error;
		}
	}
	
	return (RuntimeError){ RuntimeErrorCodeUndefinedIdentifier, expression.binary.left->start, expression.binary.left->end };
}

static inline scalar_t ApplyBinaryArithmetic(scalar_t a, scalar_t b, Operator operator) {
	switch (operator) {
		case OperatorAdd: return a + b;
		case OperatorSubtract: return a - b;
		case OperatorMultiply: return a * b;
		case OperatorDivide: return a / b;
		case OperatorModulo: return fmodf(a, b);
		case OperatorPower: return powf(a, b);
		case OperatorEqual: return a == b;
		case OperatorNotEqual: return a != b;
		case OperatorGreater: return a > b;
		case OperatorGreaterEqual: return a >= b;
		case OperatorLess: return a < b;
		case OperatorLessEqual: return a <= b;
		default: return NAN;
	}
}

static RuntimeError EvaluateBinary(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result) {
	switch (expression.binary.operator) {
		case OperatorRange: return (RuntimeError){ RuntimeErrorCodeInvalidRangePlacement, expression.start, expression.end };
		case OperatorFor: return (RuntimeError){ RuntimeErrorCodeInvalidForPlacement, expression.start, expression.end };
		case OperatorDimension: return EvaluateDimension(environment, parameters, expression, result);
		case OperatorIndexStart: return EvaluateIndex(environment, parameters, expression, result);
		case OperatorCallStart: return EvaluateCall(environment, parameters, expression, result);
		default: break;
	}
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
		case ExpressionTypeForAssignment: return (RuntimeError){ RuntimeErrorCodeInvalidForAssignmentPlacement, expression.start, expression.end };
		case ExpressionTypeUnary: return EvaluateUnary(environment, parameters, expression, result);
		case ExpressionTypeBinary: return EvaluateBinary(environment, parameters, expression, result);
		case ExpressionTypeTernary: return EvaluateTernary(environment, parameters, expression, result);
	}
}
