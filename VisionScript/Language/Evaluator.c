#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "Evaluator.h"
#include "Builtin.h"

const char * RuntimeErrorToString(RuntimeErrorCode code) {
	switch (code) {
		case RuntimeErrorCodeNone: return "no error";
		case RuntimeErrorCodeInvalidExpression: return "trying to evaluate invalid expression";
		case RuntimeErrorCodeInvalidArgumentsPlacement: return "unable to evaluate arguments in this context";
		case RuntimeErrorCodeInvalidForAssignmentPlacement: return "unable to evaluate for assignment in this context";
		case RuntimeErrorCodeIdentifierNotVariable: return "trying to treat function as variable";
		case RuntimeErrorCodeUndefinedIdentifier: return "undefined identifier";
		case RuntimeErrorCodeTooManyVectorElements: return "too many vector elements";
		case RuntimeErrorCodeNonUniformArray: return "inconsistent array dimensionality";
		case RuntimeErrorCodeInvalidRangeOperon: return "range bounds must be of length one";
		case RuntimeErrorCodeNonUniformRange: return "inconsistent range dimensionality";
		case RuntimeErrorCodeInvalidRangePlacement: return "unable to evaluate range in this context";
		case RuntimeErrorCodeInvalidForPlacement: return "unable to evaluate for-statement in this context";
		case RuntimeErrorCodeInvalidIfPlacement: return "unable to evaluate if-statement in this context";
		case RuntimeErrorCodeInvalidElsePlacement: return "unable to evaluate else-statement in this context";
		case RuntimeErrorCodeInvalidWhenPlacement: return "unable to evaluate when-statement in this context";
		case RuntimeErrorCodeMissingForAssignment: return "missing for assignment";
		case RuntimeErrorCodeInvalidDimensionOperon: return "invalid dimension indexing";
		case RuntimeErrorCodeInvalidSwizzling: return "vector swizzling out of bounds";
		case RuntimeErrorCodeInvalidIndexDimension: return "invalid indexing dimension";
		case RuntimeErrorCodeUncallableExpression: return "unable to call expression";
		case RuntimeErrorCodeIdentifierNotFunction: return "trying to treat variable as function";
		case RuntimeErrorCodeInvalidArgumentsExpression: return "trying to evaluate invalid arguments";
		case RuntimeErrorCodeIncorrectArgumentCount: return "incorrect argument count";
		case RuntimeErrorCodeDifferingOperonDimensions: return "unable to perform arithmetic on differing dimensionality";
		case RuntimeErrorCodeInvalidRenderDimension: return "render equation is of invalid dimension";
		case RuntimeErrorCodeInvalidParametricEquation: return "invalid parametric equation, must be function of one parameter";
		case RuntimeErrorCodeInvalidParametricDomain: return "invalid parametric domain";
		case RuntimeErrorCodeInvalidColorDimension: return "invalid color dimension";
		case RuntimeErrorCodeInvalidSizeDimension: return "invalid size dimension";
		case RuntimeErrorCodeReachedDepthLimit: return "reached expression depth limit";
		case RuntimeErrorCodeNotImplemented: return "not implemented";
		default: return "unknown error";
	}
}

void PrintRuntimeError(RuntimeError error, List(String) lines) {
	printf("RuntimeError: %s", RuntimeErrorToString(error.code));
	String snippet = StringSub(lines[error.line], error.start, error.end);
	printf(" \"%s\"\n", snippet);
	printf("\tin line: \"%s\"\n", lines[error.line]);
	StringFree(snippet);
}

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

bool TruthyVectorArray(VectorArray value) {
	for (int32_t i = 0; i < value.dimensions; i++) {
		for (int32_t j = 0; j < value.length; j++) {
			if (value.xyzw[i][j]) { return true; }
		}
	}
	return false;
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
	return (Environment) {
		.equations = HashMapCreate(sizeof(Equation)),
		.cache = HashMapCreate(sizeof(VectorArray)),
		.dependents = HashMapCreate(sizeof(List(String))),
	};
}

void AddEnvironmentEquation(Environment * environment, Equation equation) {
	Equation * oldEquation = HashMapGet(environment->equations, equation.declaration.identifier);
	if (oldEquation != NULL) { FreeEquation(*oldEquation); }
	HashMapSet(environment->equations, equation.declaration.identifier, &equation);
}

Equation * GetEnvironmentEquation(Environment * environment, const char * identifier) {
	return HashMapGet(environment->equations, identifier);
}

void SetEnvironmentCache(Environment * environment, const char * identifier, VectorArray value) {
	VectorArray * oldCache = HashMapGet(environment->cache, identifier);
	if (oldCache != NULL) { FreeVectorArray(*oldCache); }
	HashMapSet(environment->cache, identifier, &value);
}

VectorArray * GetEnvironmentCache(Environment * environment, const char * identifier) {
	return HashMapGet(environment->cache, identifier);
}

void InitializeEnvironmentDependents(Environment * environment) {
	List(String) keys = HashMapKeys(environment->equations);
	for (int32_t i = 0; i < ListLength(keys); i++) {
		Equation * equation = HashMapGet(environment->equations, keys[i]);
		List(String) parents = ListCreate(sizeof(String), 1);
		FindExpressionParents(*environment, equation->expression, equation->declaration.parameters, &parents);
		for (int32_t j = 0; j < ListLength(parents); j++) {
			List(Equation) * dependents = HashMapGet(environment->dependents, parents[j]);
			if (dependents == NULL) {
				HashMapSet(environment->dependents, parents[j], &(List(Equation)){ ListCreate(sizeof(Equation), 1) });
				dependents = HashMapGet(environment->dependents, parents[j]);
			}
			if (!ListContains(*dependents, equation)) { *dependents = ListPush(*dependents, equation); }
		}
		ListFree(parents);
	}
	ListFree(keys);
}

void FreeEnvironment(Environment environment) {
	List(String) keys = HashMapKeys(environment.equations);
	for (int32_t i = 0; i < ListLength(keys); i++) { FreeEquation(*(Equation *)HashMapGet(environment.equations, keys[i])); }
	ListFree(keys);
	HashMapFree(environment.equations);
	
	keys = HashMapKeys(environment.cache);
	for (int32_t i = 0; i < ListLength(keys); i++) { FreeVectorArray(*(VectorArray *)HashMapGet(environment.cache, keys[i])); }
	ListFree(keys);
	HashMapFree(environment.cache);
	
	keys = HashMapKeys(environment.dependents);
	for (int32_t i = 0; i < ListLength(keys); i++) {
		List(String) * identifiers = HashMapGet(environment.dependents, keys[i]);
		for (int32_t j = 0; j < ListLength(*identifiers); j++) { StringFree((*identifiers)[j]); }
		ListFree(*identifiers);
 	}
	ListFree(keys);
	HashMapFree(environment.dependents);
}

static RuntimeError _EvaluateExpression(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result);

static RuntimeError EvaluateConstant(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	result->length = 1;
	result->dimensions = 1;
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = expression.constant;
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError EvaluateIdentifier(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	if (parameters != NULL) {
		for (int32_t i = 0; i < ListLength(parameters); i++) {
			if (StringEquals(parameters[i].identifier, expression.identifier)) {
				*result = CopyVectorArray(parameters[i].value);
				return (RuntimeError){ RuntimeErrorCodeNone };
			}
		}
	}
	
	VectorArray * cached = GetEnvironmentCache(environment, expression.identifier);
	if (cached != NULL) {
		*result = CopyVectorArray(*cached);
		return (RuntimeError){ RuntimeErrorCodeNone };
	}
	
	Equation * equation = GetEnvironmentEquation(environment, expression.identifier);
	if (equation != NULL) {
		if (equation->type == EquationTypeFunction) { return (RuntimeError){ RuntimeErrorCodeIdentifierNotVariable, expression.start, expression.end, expression.line }; }
		RuntimeError error = _EvaluateExpression(environment, NULL, equation->expression, depth + 1, result);
		if (error.code == RuntimeErrorCodeNone) { SetEnvironmentCache(environment, expression.identifier, CopyVectorArray(*result)); }
		return error;
	}
	
	BuiltinVariable variable = DetermineBuiltinVariable(expression.identifier);
	if (variable != BuiltinVariableNone) {
		return (RuntimeError){ EvaluateBuiltinVariable(*environment, variable, result), expression.start, expression.end, expression.line };
	}
	
	return (RuntimeError){ RuntimeErrorCodeUndefinedIdentifier, expression.start, expression.end, expression.line };
}

static RuntimeError EvaluateVectorLiteral(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	if (ListLength(expression.list) > 4) { return (RuntimeError){ RuntimeErrorCodeTooManyVectorElements, expression.start, expression.end, expression.line }; }
	result->dimensions = 0;
	result->length = -1; // uint -1
	
	VectorArray components[4];
	for (int32_t i = 0, d = 0; i < ListLength(expression.list); i++) {
		RuntimeError error = _EvaluateExpression(environment, parameters, expression.list[i], depth + 1, &components[i]);
		if (error.code != RuntimeErrorCodeNone) {
			for (int32_t j = 0; j < i; j++) { FreeVectorArray(components[j]); }
			return error;
		}
		result->dimensions += components[i].dimensions;
		if (result->dimensions > 4) {
			for (int32_t j = 0; j <= i; j++) { FreeVectorArray(components[j]); }
			return (RuntimeError){ RuntimeErrorCodeTooManyVectorElements, expression.list[i].start, expression.list[i].end, expression.line };
		}
		
		// length is set to the smallest length of each component not including length 1 (since length 1 will assume length of the rest of the vector)
		if (components[i].length > 1) { result->length = components[i].length < result->length ? components[i].length : result->length; }
		for (int32_t j = 0; j < components[i].dimensions; j++) {
			result->xyzw[d++] = components[i].xyzw[j];
		}
	}
	if (result->length == -1) { result->length = 1; } // if all the component lengths are 1 then result->length will still be -1, so set it to 1
	
	// go through all the components of length 1 extend it to be same length as the rest of the vector
	for (int32_t i = 0, d = 0; i < ListLength(expression.list); i++) {
		if (components[i].length == 1 && result->length > 1) {
			for (int32_t j = 0; j < components[i].dimensions; j++) {
				result->xyzw[d + j] = malloc(sizeof(scalar_t) * result->length);
				for (int32_t k = 0; k < result->length; k++) { result->xyzw[d + j][k] = components[i].xyzw[j][0]; }
				free(components[i].xyzw[j]);
			}
		}
		d += components[i].dimensions;
	}
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError EvaluateRange(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	VectorArray left, right;
	RuntimeError error = _EvaluateExpression(environment, parameters, *expression.binary.left, depth + 1, &left);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	error = _EvaluateExpression(environment, parameters, *expression.binary.right, depth + 1, &right);
	if (error.code != RuntimeErrorCodeNone) {
		FreeVectorArray(left);
		return error;
	}
	if (left.length != 1) { return (RuntimeError){ RuntimeErrorCodeInvalidRangeOperon, expression.binary.left->start, expression.binary.left->end, expression.line }; }
	if (right.length != 1) { return (RuntimeError){ RuntimeErrorCodeInvalidRangeOperon, expression.binary.right->start, expression.binary.right->end, expression.line }; }
	if (left.dimensions != right.dimensions) { return (RuntimeError){ RuntimeErrorCodeNonUniformRange, expression.start, expression.end, expression.line }; }
	
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

static RuntimeError EvaluateFor(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	Expression * left, * right;
	if (expression.type == ExpressionTypeTernary) {
		left = expression.ternary.left;
		right = expression.ternary.middle;
	} else {
		left = expression.binary.left;
		right = expression.binary.right;
	}
	
	if (right->type != ExpressionTypeForAssignment) { return (RuntimeError){ RuntimeErrorCodeMissingForAssignment, right->start, right->end, expression.line }; }
	VectorArray assignment;
	RuntimeError error = _EvaluateExpression(environment, parameters, *right->assignment.expression, depth + 1, &assignment);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	
	if (parameters == NULL) { parameters = ListCreate(sizeof(Binding), 1); }
	else { parameters = ListClone(parameters); }
	parameters = ListInsert(parameters, &(Binding){ .identifier = right->assignment.identifier }, 0);
	
	result->dimensions = 0;
	result->length = 0;
	VectorArray * values = malloc(assignment.length * sizeof(VectorArray));
	int32_t c = 0;
	for (int32_t i = 0; i < assignment.length; i++) {
		parameters[0].value.length = 1;
		parameters[0].value.dimensions = assignment.dimensions;
		for (int32_t j = 0; j < assignment.dimensions; j++) { parameters[0].value.xyzw[j] = &assignment.xyzw[j][i]; }
		
		if (expression.type == ExpressionTypeTernary) {
			VectorArray condition;
			RuntimeError error = _EvaluateExpression(environment, parameters, *expression.ternary.right, depth + 1, &condition);
			if (error.code != RuntimeErrorCodeNone) { return error; }
			if (!TruthyVectorArray(condition)) {
				FreeVectorArray(condition);
				continue;
			}
			FreeVectorArray(condition);
		}
		
		RuntimeError error;
		if (left->type == ExpressionTypeBinary && left->binary.operator == OperatorRange) { error = EvaluateRange(environment, parameters, *left, depth, &values[c]); }
		else if (left->type == ExpressionTypeBinary && left->binary.operator == OperatorFor) { error = EvaluateFor(environment, parameters, *left, depth, &values[c]); }
		else if (left->type == ExpressionTypeTernary && left->ternary.leftOperator == OperatorFor) { error = EvaluateFor(environment, parameters, *left, depth, &values[c]); }
		else { error = _EvaluateExpression(environment, parameters, *left, depth + 1, &values[c]); }
		if (error.code != RuntimeErrorCodeNone) { goto free; }
		if (result->dimensions == 0) { result->dimensions = values[c].dimensions; }
		if (values[c].dimensions != result->dimensions) {
			error = (RuntimeError){ RuntimeErrorCodeNonUniformArray, left->start, left->end, expression.line };
			goto free;
		}
		
		result->length += values[c].length;
		c++;
		continue;
	free:
		FreeVectorArray(assignment);
		for (int32_t j = 0; j < c; j++) { FreeVectorArray(values[j]); }
		ListFree(parameters);
		return error;
	}
	
	for (int32_t i = 0; i < result->dimensions; i++) {
		result->xyzw[i] = malloc(result->length * sizeof(scalar_t));
		for (int32_t j = 0, p = 0; j < c; j++) {
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

static RuntimeError EvaluateArrayLiteral(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	result->dimensions = 0;
	result->length = 0;
	
	// evaluate each element of the array and store them in elements temporarily
	VectorArray * elements = malloc(sizeof(VectorArray) * ListLength(expression.list));
	for (int32_t i = 0; i < ListLength(expression.list); i++) {
		RuntimeError error;
		if (expression.list[i].type == ExpressionTypeBinary && expression.list[i].binary.operator == OperatorRange) {
			error = EvaluateRange(environment, parameters, expression.list[i], depth, &elements[i]);
		} else if (expression.list[i].type == ExpressionTypeBinary && expression.list[i].binary.operator == OperatorFor) {
			error = EvaluateFor(environment, parameters, expression.list[i], depth, &elements[i]);
		} else if (expression.list[i].type == ExpressionTypeTernary && expression.list[i].ternary.leftOperator == OperatorFor) {
			error = EvaluateFor(environment, parameters, expression.list[i], depth, &elements[i]);
		} else {
			error = _EvaluateExpression(environment, parameters, expression.list[i], depth + 1, &elements[i]);
		}
		if (error.code != RuntimeErrorCodeNone) { goto free; }
		
		if (result->dimensions == 0) { result->dimensions = elements[i].dimensions; } // dimension of array is defined to be dimension of the first element
		if (elements[i].dimensions != result->dimensions) {
			error = (RuntimeError){ RuntimeErrorCodeNonUniformArray, expression.list[i].start, expression.list[i].end, expression.line };
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

static RuntimeError EvaluateUnary(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	RuntimeError error = _EvaluateExpression(environment, parameters, *expression.unary.expression, depth + 1, result);
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

static RuntimeError EvaluateDimension(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	if (expression.binary.right->type != ExpressionTypeIdentifier || !IsIdentifierSwizzling(expression.binary.right->identifier)) {
		return (RuntimeError){ RuntimeErrorCodeInvalidDimensionOperon, expression.binary.right->start, expression.binary.right->end, expression.line };
	}
	
	VectorArray indexed;
	RuntimeError error = _EvaluateExpression(environment, parameters, *expression.binary.left, depth + 1, &indexed);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	
	result->dimensions = StringLength(expression.binary.right->identifier);
	result->length = indexed.length;
	bool shouldDuplicate[4] = { false, false, false, false }; // keeps track of duplicate vectors (e.g. .xxyz)
	for (int32_t i = 0; i < result->dimensions; i++) {
		int32_t component = expression.binary.right->identifier[i] - 'x';
		if (component >= indexed.dimensions) {
			FreeVectorArray(indexed);
			return (RuntimeError){ RuntimeErrorCodeInvalidSwizzling, expression.binary.right->start, expression.binary.right->end, expression.line };
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

static RuntimeError EvaluateIndex(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	VectorArray indexed, indices;
	RuntimeError error = _EvaluateExpression(environment, parameters, *expression.binary.right, depth + 1, &indices);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	if (indices.dimensions > 1) { return (RuntimeError){ RuntimeErrorCodeInvalidIndexDimension, expression.binary.right->start, expression.binary.right->end, expression.line }; }
	error = _EvaluateExpression(environment, parameters, *expression.binary.left, depth + 1, &indexed);
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

static RuntimeError EvaluateArguments(Environment * environment, List(Binding) parameters, Expression expression, List(String) variables, int32_t depth, List(Binding) * arguments) {
	if (ListLength(variables) != ListLength(expression.binary.right->list)) {
		return (RuntimeError){ RuntimeErrorCodeIncorrectArgumentCount, expression.binary.right->start, expression.binary.right->end, expression.line };
	}
	
	for (int32_t i = 0; i < ListLength(expression.binary.right->list); i++) {
		VectorArray argument;
		RuntimeError error = _EvaluateExpression(environment, parameters, expression.binary.right->list[i], depth + 1, &argument);
		if (error.code != RuntimeErrorCodeNone) {
			for (int32_t j = 0; j < i; j++) { FreeBinding((*arguments)[j]); }
			return error;
		}
		*arguments = ListPush(*arguments, &(Binding){ .identifier = StringCreate(variables[i]), .value = argument });
	}
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError EvaluateCall(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	if (expression.binary.left->type != ExpressionTypeIdentifier) {
		return (RuntimeError){ RuntimeErrorCodeUncallableExpression, expression.binary.left->start, expression.binary.left->end, expression.line };
	}
	if (expression.binary.right->type != ExpressionTypeArguments) {
		return (RuntimeError){ RuntimeErrorCodeInvalidArgumentsExpression, expression.binary.right->start, expression.binary.right->end, expression.line };
	}
	
	Equation * equation = GetEnvironmentEquation(environment, expression.binary.left->identifier);
	if (equation != NULL) {
		if (equation->type == EquationTypeVariable) {
			return (RuntimeError){ RuntimeErrorCodeIdentifierNotFunction, expression.binary.left->start, expression.binary.left->end, expression.line };
		}
		
		List(Binding) arguments = ListCreate(sizeof(Binding), 1);
		RuntimeError error = EvaluateArguments(environment, parameters, expression, equation->declaration.parameters, depth, &arguments);
		if (error.code == RuntimeErrorCodeNone) { error = _EvaluateExpression(environment, arguments, equation->expression, depth + 1, result); }
		for (int32_t j = 0; j < ListLength(arguments); j++) { FreeBinding(arguments[j]); }
		ListFree(arguments);
		return error;
	}
	
	BuiltinFunction function = DetermineBuiltinFunction(expression.binary.left->identifier);
	if (function != BuiltinFunctionNone) {
		if (IsFunctionSingleArgument(function)) {
			if (ListLength(expression.binary.right->list) != 1) {
				return (RuntimeError){ RuntimeErrorCodeIncorrectArgumentCount, expression.binary.right->start, expression.binary.right->end, expression.line };
			}
			RuntimeError error = _EvaluateExpression(environment, parameters, expression.binary.right->list[0], depth + 1, result);
			if (error.code != RuntimeErrorCodeNone) { return error; }
			return (RuntimeError){ EvaluateBuiltinFunction(function, NULL, result), expression.start, expression.end, expression.line };
		} else {
			List(VectorArray) arguments = ListCreate(sizeof(VectorArray), 1);
			for (int32_t i = 0; i < ListLength(expression.binary.right->list); i++) {
				arguments = ListPush(arguments, &(VectorArray){ 0 });
				RuntimeError error = _EvaluateExpression(environment, parameters, expression.binary.right->list[i], depth + 1, &arguments[i]);
				if (error.code != RuntimeErrorCodeNone) {
					for (int32_t j = 0; j < i; j++) { FreeVectorArray(arguments[j]); }
					ListFree(arguments);
					return error;
				}
			}
			RuntimeErrorCode code = EvaluateBuiltinFunction(function, arguments, result);
			for (int32_t j = 0; j < ListLength(expression.binary.right->list); j++) { FreeVectorArray(arguments[j]); }
			ListFree(arguments);
			return (RuntimeError){ code, expression.start, expression.end, expression.line };
		}
	}
	
	return (RuntimeError){ RuntimeErrorCodeUndefinedIdentifier, expression.binary.left->start, expression.binary.left->end, expression.line };
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

static RuntimeError EvaluateBinaryArithmetic(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	VectorArray left, right;
	RuntimeError error = _EvaluateExpression(environment, parameters, *expression.binary.left, depth + 1, &left);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	error = _EvaluateExpression(environment, parameters, *expression.binary.right, depth + 1, &right);
	if (error.code != RuntimeErrorCodeNone) {
		FreeVectorArray(left);
		return error;
	}
	if (left.dimensions != right.dimensions && left.dimensions != 1 && right.dimensions != 1) {
		FreeVectorArray(left);
		FreeVectorArray(right);
		return (RuntimeError){ RuntimeErrorCodeDifferingOperonDimensions, expression.start, expression.end, expression.line };
	}
	
	if (left.length == 1) { result->length = right.length; }
	else if (right.length == 1) { result->length = left.length; }
	else { result->length = left.length < right.length ? left.length : right.length; }
	
	if (left.dimensions == 1) { result->dimensions = right.dimensions; }
	else { result->dimensions = left.dimensions; }
	
	for (int32_t i = 0; i < result->dimensions; i++) {
		result->xyzw[i] = malloc(sizeof(scalar_t) * result->length);
		for (int32_t j = 0; j < result->length; j++) {
			scalar_t a = left.xyzw[left.dimensions == 1 ? 0 : i][left.length == 1 ? 0 : j];
			scalar_t b = right.xyzw[right.dimensions == 1 ? 0 : i][right.length == 1 ? 0 : j];
			result->xyzw[i][j] = ApplyBinaryArithmetic(a, b, expression.binary.operator);
		}
	}
	
	FreeVectorArray(left);
	FreeVectorArray(right);
	return (RuntimeError){ RuntimeErrorCodeNone };
}

static RuntimeError EvaluateBinary(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	switch (expression.binary.operator) {
		case OperatorRange: return (RuntimeError){ RuntimeErrorCodeInvalidRangePlacement, expression.start, expression.end, expression.line };
		case OperatorFor: return (RuntimeError){ RuntimeErrorCodeInvalidForPlacement, expression.start, expression.end, expression.line };
		case OperatorDimension: return EvaluateDimension(environment, parameters, expression, depth, result);
		case OperatorIndexStart: return EvaluateIndex(environment, parameters, expression, depth, result);
		case OperatorCallStart: return EvaluateCall(environment, parameters, expression, depth, result);
		case OperatorIf: return (RuntimeError){ RuntimeErrorCodeInvalidIfPlacement, expression.start, expression.end, expression.line };
		case OperatorElse: return (RuntimeError){ RuntimeErrorCodeInvalidElsePlacement, expression.start, expression.end, expression.line };
		case OperatorWhen: return (RuntimeError){ RuntimeErrorCodeInvalidWhenPlacement, expression.start, expression.end, expression.line };
		default: return EvaluateBinaryArithmetic(environment, parameters, expression, depth, result);
	}
}

static RuntimeError EvaluateIfElse(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	VectorArray condition;
	RuntimeError error = _EvaluateExpression(environment, parameters, *expression.ternary.middle, depth + 1, &condition);
	if (error.code != RuntimeErrorCodeNone) { return error; }
	
	if (TruthyVectorArray(condition)) {
		FreeVectorArray(condition);
		return _EvaluateExpression(environment, parameters, *expression.ternary.left, depth + 1, result);
	}
	FreeVectorArray(condition);
	return _EvaluateExpression(environment, parameters, *expression.ternary.right, depth + 1, result);
}

static RuntimeError EvaluateTernary(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	if (expression.ternary.leftOperator == OperatorIf && expression.ternary.rightOperator == OperatorElse) {
		return EvaluateIfElse(environment, parameters, expression, depth, result);
	}
	if (expression.ternary.leftOperator == OperatorFor && expression.ternary.rightOperator == OperatorWhen) {
		return (RuntimeError){ RuntimeErrorCodeInvalidForPlacement, expression.start, expression.end, expression.line };
	}
	return (RuntimeError){ RuntimeErrorCodeNotImplemented, expression.start, expression.end, expression.line };
}

static RuntimeError _EvaluateExpression(Environment * environment, List(Binding) parameters, Expression expression, int32_t depth, VectorArray * result) {
	if (depth >= EVALUATOR_MAX_DEPTH) {
		return (RuntimeError){ RuntimeErrorCodeReachedDepthLimit, expression.start, expression.end, expression.line };
	}
	switch (expression.type) {
		case ExpressionTypeUnknown: return (RuntimeError){ RuntimeErrorCodeInvalidExpression, expression.start, expression.end, expression.line };
		case ExpressionTypeConstant: return EvaluateConstant(environment, parameters, expression, depth, result);
		case ExpressionTypeIdentifier: return EvaluateIdentifier(environment, parameters, expression, depth, result);
		case ExpressionTypeVectorLiteral: return EvaluateVectorLiteral(environment, parameters, expression, depth, result);
		case ExpressionTypeArrayLiteral: return EvaluateArrayLiteral(environment, parameters, expression, depth, result);
		case ExpressionTypeArguments: return (RuntimeError){ RuntimeErrorCodeInvalidArgumentsPlacement, expression.start, expression.end, expression.line };
		case ExpressionTypeForAssignment: return (RuntimeError){ RuntimeErrorCodeInvalidForAssignmentPlacement, expression.start, expression.end, expression.line };
		case ExpressionTypeUnary: return EvaluateUnary(environment, parameters, expression, depth, result);
		case ExpressionTypeBinary: return EvaluateBinary(environment, parameters, expression, depth, result);
		case ExpressionTypeTernary: return EvaluateTernary(environment, parameters, expression, depth, result);
	}
}

RuntimeError EvaluateExpression(Environment * environment, List(Binding) parameters, Expression expression, VectorArray * result) {
	return _EvaluateExpression(environment, parameters, expression, 0, result);
}

void FindExpressionParents(Environment environment, Expression expression, List(String) parameters, List(String) * identifiers) {
	if (expression.type == ExpressionTypeIdentifier) {
		if (parameters != NULL) {
			for (int32_t i = 0; i < ListLength(parameters); i++) {
				if (StringEquals(parameters[i], expression.identifier)) { return; }
			}
		}
		if (!ListContains(*identifiers, &expression.identifier)) {
			*identifiers = ListPush(*identifiers, &expression.identifier);
			Equation * equation = GetEnvironmentEquation(&environment, expression.identifier);
			if (equation != NULL) { FindExpressionParents(environment, equation->expression, equation->declaration.parameters, identifiers); }
		}
	}
	if (expression.type == ExpressionTypeVectorLiteral || expression.type == ExpressionTypeArrayLiteral || expression.type == ExpressionTypeArguments) {
		for (int32_t i = 0; i < ListLength(expression.list); i++) { FindExpressionParents(environment, expression.list[i], parameters, identifiers); }
	}
	if (expression.type == ExpressionTypeUnary) {
		FindExpressionParents(environment, *expression.unary.expression, parameters, identifiers);
	}
	if (expression.type == ExpressionTypeBinary) {
		if (expression.binary.operator == OperatorFor && expression.binary.right->type == ExpressionTypeForAssignment) {
			parameters = parameters == NULL ? ListCreate(sizeof(String), 1) : ListClone(parameters);
			parameters = ListInsert(parameters, &expression.binary.right->assignment.identifier, 0);
			FindExpressionParents(environment, *expression.binary.left, parameters, identifiers);
			ListFree(parameters);
		} else if (expression.binary.operator == OperatorCallStart && expression.binary.left->type == ExpressionTypeIdentifier) {
			if (!ListContains(*identifiers, &expression.binary.left->identifier)) {
				*identifiers = ListPush(*identifiers, &expression.binary.left->identifier);
				Equation * equation = GetEnvironmentEquation(&environment, expression.binary.left->identifier);
				if (equation != NULL) { FindExpressionParents(environment, equation->expression, equation->declaration.parameters, identifiers); }
			}
			FindExpressionParents(environment, *expression.binary.right, parameters, identifiers);
		} else {
			FindExpressionParents(environment, *expression.binary.left, parameters, identifiers);
			FindExpressionParents(environment, *expression.binary.right, parameters, identifiers);
		}
	}
	if (expression.type == ExpressionTypeTernary) {
		if (expression.ternary.leftOperator == OperatorFor && expression.ternary.middle->type == ExpressionTypeForAssignment) {
			parameters = ListClone(parameters);
			parameters = ListInsert(parameters, &expression.ternary.middle->assignment.identifier, 0);
			FindExpressionParents(environment, *expression.ternary.left, parameters, identifiers);
			FindExpressionParents(environment, *expression.ternary.right, parameters, identifiers);
			ListFree(parameters);
		} else {
			FindExpressionParents(environment, *expression.ternary.left, parameters, identifiers);
			FindExpressionParents(environment, *expression.ternary.middle, parameters, identifiers);
			FindExpressionParents(environment, *expression.ternary.right, parameters, identifiers);
		}
	}
}
