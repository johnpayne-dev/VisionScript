#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "Evaluator.h"
#include "Builtin.h"

static RuntimeError EvaluateOperon(HashMap identifiers, HashMap cache, list(Parameter) parameters, OperonType operonType, Operon operon, VectorArray * result)
{
	if (operonType == OperonTypeExpression) { return EvaluateExpression(identifiers, cache, parameters, operon.expression, result); }
	else if (operonType == OperonTypeIdentifier)
	{
		if (parameters != NULL)
		{
			for (int32_t i = 0; i < ListLength(parameters); i++)
			{
				if (strcmp(operon.identifier, parameters[i].identifier) == 0)
				{
					result->dimensions = parameters[i].value.dimensions;
					result->length = parameters[i].value.length;
					for (int8_t d = 0; d < result->dimensions; d++)
					{
						result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
						memcpy(result->xyzw[d], parameters[i].value.xyzw[d], result->length * sizeof(scalar_t));
					}
					return RuntimeErrorNone;
				}
			}
		}
		Statement * statement = HashMapGet(identifiers, operon.identifier);
		if (statement == NULL)
		{
			BuiltinVariable builtin = DetermineBuiltinVariable(operon.identifier);
			if (builtin != BuiltinVariableNone) { return EvaluateBuiltinVariable(builtin, result); }
			return RuntimeErrorUndefinedIdentifier;
		}
		if (statement->type == StatementTypeFunction) { return RuntimeErrorIdentifierNotVariable; }
		VectorArray * value = HashMapGet(cache, operon.identifier);
		if (value != NULL)
		{
			result->length = value->length;
			result->dimensions = value->dimensions;
			for (int8_t d = 0; d < result->dimensions; d++)
			{
				result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
				memcpy(result->xyzw[d], value->xyzw[d], result->length * sizeof(scalar_t));
			}
			return RuntimeErrorNone;
		}
		return EvaluateExpression(identifiers, cache, NULL, statement->expression, result);
	}
	else if (operonType == OperonTypeConstant)
	{
		result->dimensions = 1;
		result->length = 1;
		result->xyzw[0] = malloc(sizeof(scalar_t));
		result->xyzw[0][0] = operon.constant;
	}
	else if (operonType == OperonTypeVectorLiteral)
	{
		result->dimensions = ListLength(operon.expressions);
		result->length = -1;
		
		VectorArray components[4];
		for (int32_t i = 0; i < result->dimensions; i++)
		{
			RuntimeError error = EvaluateExpression(identifiers, cache, parameters, operon.expressions[i], components + i);
			if (error != RuntimeErrorNone) { return error; }
			
			if (components[i].dimensions > 1) { return RuntimeErrorVectorInsideVector; }
			
			if (components[i].length > 1) { result->length = components[i].length < result->length ? components[i].length : result->length; }
			
			result->xyzw[i] = components[i].xyzw[0];
		}
		if (result->length == -1) { result->length = 1; }
		
		for (int32_t i = 0; i < result->dimensions; i++)
		{
			if (components[i].length == 1 && result->length > 1)
			{
				result->xyzw[i] = malloc(sizeof(scalar_t) * result->length);
				for (int32_t j = 0; j < result->length; j++) { result->xyzw[i][j] = components[i].xyzw[0][0]; }
				free(components[i].xyzw[0]);
			}
		}
	}
	else if (operonType == OperonTypeArrayLiteral)
	{
		result->dimensions = 0;
		result->length = 0;
		
		VectorArray * elements = malloc(sizeof(VectorArray) * ListLength(operon.expressions));
		for (int32_t i = 0; i < ListLength(operon.expressions); i++)
		{
			RuntimeError error = EvaluateExpression(identifiers, cache, parameters, operon.expressions[i], elements + i);
			if (error != RuntimeErrorNone) { return error; }
			
			if (result->dimensions == 0) { result->dimensions = elements[i].dimensions; }
			if (elements[i].dimensions != result->dimensions) { return RuntimeErrorNonUniformArray; }
			if (elements[i].length > 1 && operon.expressions[i]->operator != OperatorFor && operon.expressions[i]->operator != OperatorEllipsis) { return RuntimeErrorArrayInsideArray; }
			result->length += elements[i].length;
		}
		
		for (int32_t i = 0; i < result->dimensions; i++)
		{
			if (ListLength(operon.expressions) == 1) { result->xyzw[i] = elements[0].xyzw[i]; }
			else
			{
				result->xyzw[i] = malloc(sizeof(scalar_t) * result->length);
				for (int32_t j = 0, p = 0; j < ListLength(operon.expressions); j++)
				{
					memcpy(result->xyzw[i] + p, elements[j].xyzw[i], elements[j].length * sizeof(scalar_t));
					p += elements[j].length;
					free(elements[j].xyzw[i]);
				}
			}
		}
		
		free(elements);
	}
	return RuntimeErrorNone;
}

static void EvaluateEllipsis(VectorArray * a, VectorArray * b, VectorArray * result)
{
	int32_t lower = roundf(a->xyzw[0][0]);
	int32_t upper = roundf(b->xyzw[0][0]);
	if (upper - lower > (1 << 24) || upper - lower < 0) { *result = (VectorArray){ 0 }; return; }
	result->length = upper - lower + 1;
	result->dimensions = 1;
	result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
	for (int32_t i = 0; i < result->length; i++) { result->xyzw[0][i] = i + lower; }
}

static inline scalar_t Operation(Operator operator, scalar_t a, scalar_t b)
{
	switch (operator)
	{
		case OperatorAdd: return a + b;
		case OperatorSubtract: return a - b;
		case OperatorMultiply: return a * b;
		case OperatorDivide: return a / b;
		case OperatorModulo: return fmodf(a, b);
		case OperatorPower: return powf(a, b);
		default: return 0.0;
	}
}

static void EvaluateOperation(Operator operator, VectorArray * a, VectorArray * b)
{
	int8_t dimensions = a->dimensions > b->dimensions ? a->dimensions : b->dimensions;
	int32_t length = a->length < b->length ? a->length : b->length;
	
	if (a->dimensions == 1 && b->dimensions > 1)
	{
		for (int8_t d = 1; d < b->dimensions; d++)
		{
			a->xyzw[d] = malloc(length * sizeof(scalar_t));
			memcpy(a->xyzw[d], a->xyzw[0], length * sizeof(scalar_t));
		}
		a->dimensions = b->dimensions;
	}
	if (b->dimensions == 1 && a->dimensions > 1)
	{
		for (int8_t d = 1; d < a->dimensions; d++)
		{
			b->xyzw[d] = malloc(length * sizeof(scalar_t));
			memcpy(b->xyzw[d], b->xyzw[0], length * sizeof(scalar_t));
		}
		b->dimensions = a->dimensions;
	}
	
	bool aScalar = false, bScalar = false;
	if (a->length == 1 && b->length > 1) { length = b->length; aScalar = true; }
	if (b->length == 1 && a->length > 1) { length = a->length; bScalar = true; }

	for (int8_t d = 0; d < dimensions; d++)
	{
		for (int32_t i = 0; i < length; i++)
		{
			if (a->length == 1 && b->length > 1) { b->xyzw[d][bScalar ? 0 : i] = Operation(operator, a->xyzw[d][aScalar ? 0 : i], b->xyzw[d][bScalar ? 0 : i]); }
			else { a->xyzw[d][aScalar ? 0 : i] = Operation(operator, a->xyzw[d][aScalar ? 0 : i], b->xyzw[d][bScalar ? 0 : i]); }
		}
	}
	
	if (a->length == 1 && b->length > 1) { b->length = length; }
	else { a->length = length; }
}

static void EvaluateNegate(VectorArray * a)
{
	for (int8_t d = 0; d < a->dimensions; d++)
	{
		for (int32_t i = 0; i < a->length; i++)
		{
			a->xyzw[d][i] = -a->xyzw[d][i];
		}
	}
}

static RuntimeError EvaluateFunctionCall(HashMap identifiers, HashMap cache, list(Parameter) parameters, String function, list(Expression *) expressions, VectorArray * result)
{
	Statement * statement = HashMapGet(identifiers, function);
	if (statement == NULL)
	{
		BuiltinFunction builtin = DetermineBuiltinFunction(function);
		if (builtin == BuiltinFunctionNone) { return RuntimeErrorUndefinedIdentifier; }
		
		if (IsFunctionSingleArgument(builtin))
		{
			if (ListLength(expressions) > 1) { return RuntimeErrorIncorrectParameterCount; }
			RuntimeError error = EvaluateExpression(identifiers, cache, parameters, expressions[0], result);
			if (error != RuntimeErrorNone) { return error; }
			return EvaluateBuiltinFunction(builtin, NULL, result);
		}
		
		list(VectorArray) arguments = ListCreate(sizeof(VectorArray), ListLength(expressions));
		for (int32_t i = 0; i < ListLength(expressions); i++)
		{
			ListPush((void **)&arguments, &(VectorArray){ 0 });
			RuntimeError error = EvaluateExpression(identifiers, cache, parameters, expressions[i], &arguments[i]);
			if (error != RuntimeErrorNone) { return error; }
		}
		RuntimeError error = EvaluateBuiltinFunction(builtin, arguments, result);
		for (int32_t i = 0; i < ListLength(arguments); i++) { for (int8_t d = 0; d < arguments[i].dimensions; d++) { free(arguments[i].xyzw[d]); } }
		ListDestroy(arguments);
		return error;
	}
	else
	{
		if (statement->type == StatementTypeVariable) { return RuntimeErrorIdentifierNotFunction; }
		if (ListLength(expressions) != ListLength(statement->declaration.function.arguments)) { return RuntimeErrorIncorrectParameterCount; }
		
		list(Parameter) arguments = ListCreate(sizeof(Parameter), ListLength(expressions));
		for (int32_t i = 0; i < ListLength(expressions); i++)
		{
			Parameter parameter = { .identifier = statement->declaration.function.arguments[i] };
			RuntimeError error = EvaluateExpression(identifiers, cache, parameters, expressions[i], &parameter.value);
			if (error != RuntimeErrorNone) { return error; }
			ListPush((void **)&arguments, &parameter);
		}
		RuntimeError error = EvaluateExpression(identifiers, cache, arguments, statement->expression, result);
		if (error != RuntimeErrorNone) { return error; }
		ListDestroy(arguments);
		return error;
	}
}

static RuntimeError EvaluateFor(HashMap identifiers, HashMap cache, list(Parameter) parameters, OperonType operonType, Operon operon, struct ForAssignment assignment, VectorArray * result)
{
	*result = (VectorArray){ 0 };
	VectorArray assignmentValue = { 0 };
	RuntimeError error = EvaluateExpression(identifiers, cache, parameters, assignment.expression, &assignmentValue);
	if (error != RuntimeErrorNone) { return error; }
	
	VectorArray * values = malloc(assignmentValue.length * sizeof(VectorArray));
	if (parameters == NULL) { parameters = ListCreate(sizeof(Parameter), 1); }
	else { parameters = ListClone(parameters); }
	ListInsert((void **)&parameters, &(Parameter){ .identifier = assignment.identifier }, 0);
	parameters[0].value.length = 1;
	parameters[0].value.dimensions = assignmentValue.dimensions;
	
	for (int32_t i = 0; i < assignmentValue.length; i++)
	{
		for (int8_t d = 0; d < assignmentValue.dimensions; d++) { parameters[0].value.xyzw[d] = &assignmentValue.xyzw[d][i]; }
		RuntimeError error = EvaluateOperon(identifiers, cache, parameters, operonType, operon, &values[i]);
		if (error != RuntimeErrorNone) { return error; }
		if (result->dimensions == 0) { result->dimensions = values[i].dimensions; }
		if (values[i].dimensions != result->dimensions) { return RuntimeErrorNonUniformArray; }
		if (values[i].length > 1)
		{
			if (operonType != OperonTypeExpression) { return RuntimeErrorArrayInsideArray; }
			if (operon.expression->operator != OperatorFor) { return RuntimeErrorArrayInsideArray; }
		}
		result->length += values[i].length;
	}
	
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0, p = 0; i < assignmentValue.length; i++)
		{
			memcpy(result->xyzw[d] + p, values[i].xyzw[d], values[i].length * sizeof(scalar_t));
			p += values[i].length;
			free(values[i].xyzw[d]);
		}
	}
	free(values);
	for (int8_t d = 0; d < assignmentValue.dimensions; d++) { free(assignmentValue.xyzw[d]); }
	
	return RuntimeErrorNone;
}

static RuntimeError EvaluateIndex(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, VectorArray * result)
{
	if (expression->operonTypes[1] == OperonTypeIdentifier)
	{
		String identifier = expression->operons[1].identifier;
		bool isSwizzling = true;
		for (int32_t i = 0; i < StringLength(identifier); i++)
		{
			if (i >= 4 || (identifier[i] != 'x' && identifier[i] != 'y' && identifier[i] != 'z' && identifier[i] != 'w'))
			{
				isSwizzling = false;
				break;
			}
		}
		if (isSwizzling)
		{
			VectorArray value = { 0 };
			RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression->operonTypes[0], expression->operons[0], &value);
			if (error != RuntimeErrorNone) { return error; }
			
			result->dimensions = StringLength(identifier);
			result->length = value.length;
			bool shouldDuplicate[4] = { false, false, false, false };
			for (int8_t d = 0; d < result->dimensions; d++)
			{
				int8_t component = 0;
				if (identifier[d] == 'x') { component = 0; }
				if (identifier[d] == 'y') { component = 1; }
				if (identifier[d] == 'z') { component = 2; }
				if (identifier[d] == 'w') { component = 3; }
				if (component >= value.dimensions) { return RuntimeErrorInvalidSwizzling; }
				
				if (!shouldDuplicate[component]) { result->xyzw[d] = value.xyzw[component]; shouldDuplicate[component] = true; }
				else
				{
					result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
					memcpy(result->xyzw[d], value.xyzw[component], result->length * sizeof(scalar_t));
				}
			}
			for (int8_t d = 0; d < value.dimensions; d++)
			{
				if (!shouldDuplicate[d]) { free(value.xyzw[d]); }
			}
			return RuntimeErrorNone;
		}
	}
	
	VectorArray value = { 0 }, index = { 0 };
	RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression->operonTypes[0], expression->operons[0], &value);
	if (error != RuntimeErrorNone) { return error; }
	error = EvaluateOperon(identifiers, cache, parameters, expression->operonTypes[1], expression->operons[1], &index);
	if (error != RuntimeErrorNone) { return error; }
	if (index.dimensions > 1) { return RuntimeErrorIndexingWithVector; }
	
	result->dimensions = value.dimensions;
	result->length = index.length;
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0; i < result->length; i++)
		{
			int32_t j = roundf(index.xyzw[0][i]);
			if (j < 0 || j >= value.length) { result->xyzw[d][i] = NAN; }
			else { result->xyzw[d][i] = value.xyzw[d][j]; }
		}
		free(value.xyzw[d]);
	}
	free(index.xyzw[0]);
	
	return RuntimeErrorNone;
}

RuntimeError EvaluateExpression(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, VectorArray * result)
{
	if (expression->operator == OperatorNone) { return EvaluateOperon(identifiers, cache, parameters, expression->operonTypes[0], expression->operons[0], result); }
	if (expression->operator == OperatorFunctionCall) { return EvaluateFunctionCall(identifiers, cache, parameters, expression->operons[0].identifier, expression->operons[1].expressions, result); }
	if (expression->operator == OperatorFor) { return EvaluateFor(identifiers, cache, parameters, expression->operonTypes[0], expression->operons[0], expression->operons[1].forAssignment, result); }
	if (expression->operator == OperatorIndex) { return EvaluateIndex(identifiers, cache, parameters, expression, result); }
	
	VectorArray value = { 0 };
	if (expression->operator == OperatorPositive || expression->operator == OperatorNegative)
	{
		RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression->operonTypes[0], expression->operons[0], result);
		if (error != RuntimeErrorNone) { return error; }
	}
	else
	{
		RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression->operonTypes[0], expression->operons[0], result);
		if (error != RuntimeErrorNone) { return error; }
		error = EvaluateOperon(identifiers, cache, parameters, expression->operonTypes[1], expression->operons[1], &value);
		if (error != RuntimeErrorNone) { return error; }
	}
	
	if (expression->operator == OperatorEllipsis)
	{
		if (result->dimensions > 1 || result->length > 1 || value.dimensions > 1 || value.length > 1) { return RuntimeErrorInvalidEllipsisOperon; }
		VectorArray lower = *result;
		EvaluateEllipsis(&lower, &value, result);
		free(lower.xyzw[0]);
	}
	else if (expression->operator == OperatorNegative) { EvaluateNegate(result); }
	else if (expression->operator == OperatorPositive) {}
	else
	{
		if (result->dimensions != value.dimensions && result->dimensions != 1 && value.dimensions != 1) { return RuntimeErrorDifferingLengthVectors; }
		
		EvaluateOperation(expression->operator, result, &value);
		if (result->length == 1 && value.length > 1)
		{
			VectorArray swap = *result;
			*result = value;
			value = swap;
		}
	}
	if (value.length > 0) { for (int32_t i = 0; i < value.dimensions; i++) { free(value.xyzw[i]); } }
	return RuntimeErrorNone;
}
