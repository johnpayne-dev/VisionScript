#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "Evaluator.h"
#include "Builtin.h"

const char * RuntimeErrorToString(RuntimeErrorCode code)
{
	switch (code)
	{
		case RuntimeErrorNone: return "no error";
		case RuntimeErrorUndefinedIdentifier: return "undefined identifier";
		case RuntimeErrorVectorInsideVector: return "vector inside vector";
		case RuntimeErrorNonUniformArray: return "non uniform array dimensionality";
		case RuntimeErrorArrayInsideArray: return "array inside array";
		case RuntimeErrorArrayTooLarge: return "array excedes size limit";
		case RuntimeErrorInvalidArrayRange: return "invalid array range";
		case RuntimeErrorIdentifierNotVariable: return "treating function as variable";
		case RuntimeErrorIdentifierNotFunction: return "treating variable as function";
		case RuntimeErrorIncorrectParameterCount: return "incorrect number of arguments";
		case RuntimeErrorInvalidArgumentType: return "invalid argument type";
		case RuntimeErrorInvalidEllipsisOperon: return "invalid ellipsis operon";
		case RuntimeErrorIndexingWithVector: return "indexing array with vector";
		case RuntimeErrorDifferingLengthVectors: return "operating on differing length vectors";
		case RuntimeErrorInvalidSwizzling: return "accessing nonexistent vector component";
		case RuntimeErrorNotImplemented: return "not implemented";
		case RuntimeErrorInvalidRenderDimensionality: return "invalid number of dimensions for render command";
		default: return "unknown error";
	}
}

void PrintRuntimeError(RuntimeError error, Statement * statement)
{
	printf("RuntimeError: %s", RuntimeErrorToString(error.code));
	list(Token) tokens = (error.statement == NULL ? statement : error.statement)->tokens;
	String snippet = StringSub(tokens[0].line, tokens[error.tokenStart].lineIndexStart, tokens[error.tokenEnd].lineIndexEnd);
	printf(" \"%s\"\n", snippet);
	printf("\tin line: %s\n", tokens[0].line);
	StringFree(snippet);
}

void PrintVectorArray(VectorArray value)
{
	if (value.length > 1) { printf("["); }
	for (int32_t i = 0; i < value.length; i++)
	{
		if (value.dimensions > 1) { printf("("); }
		for (int32_t j = 0; j < value.dimensions; j++)
		{
			// print as an integer if float is an integer
			if (value.xyzw[j][i] < LLONG_MAX && value.xyzw[j][i] - floorf(value.xyzw[j][i]) == 0) { printf("%lld", (long long)value.xyzw[j][i]); }
			else { printf("%f", value.xyzw[j][i]); }
			if (j != value.dimensions - 1) { printf(","); }
		}
		if (value.dimensions > 1) { printf(")"); }
		if (i != value.length - 1) { printf(", "); }
		
		// skip to the end if there's too many elements
		if (i == 9 && value.length > 100)
		{
			printf("..., ");
			i = value.length - 5;
		}
	}
	if (value.length > 1) { printf("]"); }
}

VectorArray CopyVectorArray(VectorArray value, int32_t * indices, int32_t indexCount)
{
	if (indices == NULL || value.length == 1)
	{
		VectorArray result = { .length = value.length, .dimensions = value.dimensions };
		for (int32_t d = 0; d < result.dimensions; d++)
		{
			result.xyzw[d] = malloc(result.length * sizeof(scalar_t));
			memcpy(result.xyzw[d], value.xyzw[d], result.length * sizeof(scalar_t));
		}
		return result;
	}
	else
	{
		VectorArray result = { .length = indexCount, .dimensions = value.dimensions };
		for (int32_t d = 0; d < result.dimensions; d++)
		{
			result.xyzw[d] = malloc(result.length * sizeof(scalar_t));
			for (int32_t i = 0; i < result.length; i++)
			{
				if (indices[i] >= value.length) { result.xyzw[d][i] = NAN; }
				else { result.xyzw[d][i] = value.xyzw[d][indices[i]]; }
			}
		}
		return result;
	}
}

void FreeVectorArray(VectorArray value)
{
	for (int32_t d = 0; d < value.dimensions; d++) { free(value.xyzw[d]); }
}

RuntimeError CreateParameter(HashMap identifiers, HashMap cache, String identifier, Expression * expression, list(Parameter) parameters, bool shouldCache, Parameter * parameter)
{
	*parameter = (Parameter)
	{
		.identifier = identifier,
		.expression = expression,
		.parameters = parameters,
		.cached = shouldCache,
	};
	if (shouldCache) { return EvaluateExpression(identifiers, cache, parameters, expression, &parameter->cache); }
	return (RuntimeError){ RuntimeErrorNone };
}

RuntimeError EvaluateParameterSize(HashMap identifiers, HashMap cache, Parameter parameter, uint32_t * length, uint32_t * dimensions)
{
	if (parameter.cached)
	{
		*length = parameter.cache.length;
		*dimensions = parameter.cache.dimensions;
		return (RuntimeError){ RuntimeErrorNone };
	}
	else { return EvaluateExpressionSize(identifiers, cache, parameter.parameters, parameter.expression, length, dimensions); }
}

RuntimeError EvaluateParameter(HashMap identifiers, HashMap cache, Parameter parameter, int32_t * indices, int32_t indexCount, VectorArray * result)
{
	if (parameter.cached)
	{
		*result = CopyVectorArray(parameter.cache, indices, indexCount);
		return (RuntimeError){ RuntimeErrorNone };
	}
	else { return EvaluateExpressionIndices(identifiers, cache, parameter.parameters, parameter.expression, indices, indexCount, result); }
}

void FreeParameter(Parameter parameter)
{
	if (parameter.cached) { FreeVectorArray(parameter.cache); }
}

static RuntimeError EvaluateIdentifierSize(HashMap identifiers, HashMap cache, list(Parameter) parameters, Operon operon, int32_t operonStart, int32_t operonEnd, uint32_t * length, uint32_t * dimensions)
{
	// check if the identifier was passed as a parameter
	if (parameters != NULL)
	{
		for (int32_t i = 0; i < ListLength(parameters); i++)
		{
			if (strcmp(operon.identifier, parameters[i].identifier) == 0) { return EvaluateParameterSize(identifiers, cache, parameters[i], length, dimensions); }
		}
	}
	
	// check if the identifier was defined in another statement
	Statement * statement = HashMapGet(identifiers, operon.identifier);
	if (statement != NULL)
	{
		if (statement->type == StatementTypeFunction) { return (RuntimeError){ RuntimeErrorIdentifierNotVariable, operonStart, operonEnd }; }
		RuntimeError error = EvaluateExpressionSize(identifiers, cache, NULL, statement->expression, length, dimensions);
		if (error.statement == NULL) { error.statement = statement; } // set the correct statement for error reporting
		return error;
	}

	// check if it's a builtin variable
	BuiltinVariable variable = DetermineBuiltinVariable(operon.identifier);
	if (variable != BuiltinVariableNone) { return (RuntimeError){ EvaluateBuiltinVariableSize(cache, variable, length, dimensions), operonStart, operonEnd }; }
	if (DetermineBuiltinFunction(operon.identifier) != BuiltinFunctionNone) { return (RuntimeError){ RuntimeErrorIdentifierNotVariable, operonStart, operonEnd }; }
	
	// otherwise return an error
	return (RuntimeError){ RuntimeErrorUndefinedIdentifier, operonStart, operonEnd };
}

static RuntimeError EvaluateIdentifier(HashMap identifiers, HashMap cache, list(Parameter) parameters, Operon operon, int32_t operonStart, int32_t operonEnd, int32_t * indices, int32_t indexCount, VectorArray * result)
{
	// first check if the identifier was passed as a parameter
	if (parameters != NULL)
	{
		for (int32_t i = 0; i < ListLength(parameters); i++)
		{
			if (strcmp(operon.identifier, parameters[i].identifier) == 0) { return EvaluateParameter(identifiers, cache, parameters[i], indices, indexCount, result); }
		}
	}
	
	// next check if the identifier was defined in another statement
	Statement * statement = HashMapGet(identifiers, operon.identifier);
	if (statement != NULL)
	{
		if (statement->type == StatementTypeFunction) { return (RuntimeError){ RuntimeErrorIdentifierNotVariable, operonStart, operonEnd }; }
		// check if there's a cached value that can be used, otherwise evaluate the expression corresponding to the identifier
		VectorArray * value = HashMapGet(cache, operon.identifier);
		if (value != NULL)
		{
			*result = CopyVectorArray(*value, indices, indexCount);
			return (RuntimeError){ RuntimeErrorNone };
		}
	
		RuntimeError error = EvaluateExpressionIndices(identifiers, cache, NULL, statement->expression, indices, indexCount, result);
		if (error.statement == NULL) { error.statement = statement; } // set the correct statement for error reporting
		if (error.code == RuntimeErrorNone && indices == NULL)
		{
			// cache the result if it was evaluated successfully
			VectorArray * cached = malloc(sizeof(VectorArray));
			*cached = CopyVectorArray(*result, indices, indexCount);
			HashMapSet(cache, operon.identifier, cached);
		}
		return error;
	}

	// check if it's a builtin variable
	BuiltinVariable builtin = DetermineBuiltinVariable(operon.identifier);
	if (builtin != BuiltinVariableNone) { return (RuntimeError){ EvaluateBuiltinVariable(cache, builtin, result), operonStart, operonEnd }; }
	if (DetermineBuiltinFunction(operon.identifier) != BuiltinFunctionNone) { return (RuntimeError){ RuntimeErrorIdentifierNotVariable, operonStart, operonEnd }; }
	return (RuntimeError){ RuntimeErrorUndefinedIdentifier, operonStart, operonEnd };
}

static RuntimeError EvaluateConstantSize(Operon operon, uint32_t * length, uint32_t * dimensions)
{
	*length = 1;
	*dimensions = 1;
	return (RuntimeError){ RuntimeErrorNone };
}

static RuntimeError EvaluateConstant(Operon operon, VectorArray * result)
{
	result->dimensions = 1;
	result->length = 1;
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = operon.constant;
	return (RuntimeError){ RuntimeErrorNone };
}

static RuntimeError EvaluateVectorLiteralSize(HashMap identifiers, HashMap cache, list(Parameter) parameters, Operon operon, int32_t operonStart, int32_t operonEnd, uint32_t * length, uint32_t * dimensions)
{
	*dimensions = ListLength(operon.expressions);
	*length = -1; // uint -1
	for (int32_t i = 0; i < *dimensions; i++)
	{
		uint32_t len, dimen;
		RuntimeError error = EvaluateExpressionSize(identifiers, cache, parameters, operon.expressions[i], &len, &dimen);
		if (error.code != RuntimeErrorNone) { return error; }
		if (dimen > 1) { return (RuntimeError){ RuntimeErrorVectorInsideVector, operonStart, operonEnd }; }
		// length is set to the smallest length of each component not including length 1 (since length 1 will assume length of the rest of the vector)
		if (len > 1) { *length = len < *length ? len : *length; }
	}
	// if all the component lengths are 1 then result->length will still be -1, so set it to 1
	if (*length == -1) { *length = 1; }
	return (RuntimeError){ RuntimeErrorNone };
}

static RuntimeError EvaluateVectorLiteral(HashMap identifiers, HashMap cache, list(Parameter) parameters, Operon operon, int32_t operonStart, int32_t operonEnd, int32_t * indices, int32_t indexCount, VectorArray * result)
{
	result->dimensions = ListLength(operon.expressions);
	result->length = -1; // uint -1
	
	// evaluate each vector component
	VectorArray components[4];
	for (int32_t i = 0; i < result->dimensions; i++)
	{
		RuntimeError error = EvaluateExpressionIndices(identifiers, cache, parameters, operon.expressions[i], indices, indexCount, components + i);
		if (error.code != RuntimeErrorNone) { return error; }
		// make sure there's no vector inside vector
		if (components[i].dimensions > 1) { return (RuntimeError){ RuntimeErrorVectorInsideVector, operonStart, operonEnd }; }
		// length is set to the smallest length of each component not including length 1 (since length 1 will assume length of the rest of the vector)
		if (components[i].length > 1) { result->length = components[i].length < result->length ? components[i].length : result->length; }
		result->xyzw[i] = components[i].xyzw[0];
	}
	// if all the component lengths are 1 then result->length will still be -1, so set it to 1
	if (result->length == -1) { result->length = 1; }
	
	// go through all the components of length 1 extend it to be same length as the rest of the vector
	for (int32_t i = 0; i < result->dimensions; i++)
	{
		if (components[i].length == 1 && result->length > 1)
		{
			result->xyzw[i] = malloc(sizeof(scalar_t) * result->length);
			for (int32_t j = 0; j < result->length; j++) { result->xyzw[i][j] = components[i].xyzw[0][0]; }
			free(components[i].xyzw[0]);
		}
	}
	return (RuntimeError){ RuntimeErrorNone };
}

static RuntimeError EvaluateArrayLiteralSize(HashMap identifiers, HashMap cache, list(Parameter) parameters, Operon operon, int32_t operonStart, int32_t operonEnd, uint32_t * length, uint32_t * dimensions)
{
	*dimensions = 0;
	*length = 0;
	for (int32_t i = 0; i < ListLength(operon.expressions); i++)
	{
		uint32_t len, dimen;
		RuntimeError error = EvaluateExpressionSize(identifiers, cache, parameters, operon.expressions[i], &len, &dimen);
		if (error.code != RuntimeErrorNone) { return error; }
		
		// dimension of array is defined to be dimension of the first element
		if (*dimensions == 0) { *dimensions = dimen; }
		if (dimen != *dimensions) { return (RuntimeError){ RuntimeErrorNonUniformArray, operonStart, operonEnd }; }
		// if an element's length > 1 and it's not from ellipsis or for operator, then that means there's an array inside array so return error
		if (len > 1 && operon.expressions[i]->operator != OperatorFor && operon.expressions[i]->operator != OperatorEllipsis)
		{
			return (RuntimeError){ RuntimeErrorArrayInsideArray, operonStart, operonEnd };
		}
		// increment total length by each element's length
		*length += len;
	}
	return (RuntimeError){ RuntimeErrorNone };

}

static RuntimeError EvaluateArrayLiteral(HashMap identifiers, HashMap cache, list(Parameter) parameters, Operon operon, int32_t operonStart, int32_t operonEnd, int32_t * indices, int32_t indexCount, VectorArray * result)
{
	if (indices == NULL)
	{
		if (ListLength(operon.expressions) == 1) { return EvaluateExpressionIndices(identifiers, cache, parameters, operon.expressions[0], indices, indexCount, result); }
		
		RuntimeError error = EvaluateArrayLiteralSize(identifiers, cache, parameters, operon, operonStart, operonEnd, &result->length, &result->dimensions);
		if (error.code != RuntimeErrorNone) { return error; }
		for (int32_t d = 0; d < result->dimensions; d++) { result->xyzw[d] = malloc(result->length * sizeof(scalar_t)); }
		
		for (int32_t i = 0, offset = 0; i < ListLength(operon.expressions); i++)
		{
			VectorArray element;
			RuntimeError error = EvaluateExpressionIndices(identifiers, cache, parameters, operon.expressions[i], indices, indexCount, &element);
			if (error.code != RuntimeErrorNone) { FreeVectorArray(*result); return error; }
			for (int32_t d = 0; d < result->dimensions; d++) { memcpy(result->xyzw[d] + offset, element.xyzw[d], element.length * sizeof(scalar_t)); }
			offset += element.length;
		}
	}
	else
	{
		*result = (VectorArray){ .length = indexCount };
		RuntimeError error = EvaluateArrayLiteralSize(identifiers, cache, parameters, operon, operonStart, operonEnd, &(uint32_t){ 0 }, &result->dimensions);
		if (error.code != RuntimeErrorNone){ return error; }
		for (int32_t d = 0; d < result->dimensions; d++) { result->xyzw[d] = malloc(result->length * sizeof(scalar_t)); }
		
		for (int32_t i = 0; i < indexCount; i++)
		{
			for (int32_t j = 0, offset = 0; j < ListLength(operon.expressions); j++)
			{
				uint32_t length;
				RuntimeError error = EvaluateExpressionSize(identifiers, cache, parameters, operon.expressions[j], &length, &(uint32_t){ 0 });
				if (error.code != RuntimeErrorNone) { FreeVectorArray(*result); return error; }
				
				if (offset + length > indices[i])
				{
					VectorArray element;
					RuntimeError error = EvaluateExpressionIndices(identifiers, cache, parameters, operon.expressions[j], &(int32_t){ indices[i] - offset }, 1, &element);
					if (error.code != RuntimeErrorNone) { FreeVectorArray(*result); return error; }
					for (int32_t d = 0; d < result->dimensions; d++) { result->xyzw[d][i] = element.xyzw[d][0]; }
					break;
				}
				
				offset += length;
				if (offset == result->length)
				{
					for (int32_t d = 0; d < result->dimensions; d++) { result->xyzw[d][i] = NAN; }
				}
			}
		}
	}
	return (RuntimeError){ RuntimeErrorNone };
}

static RuntimeError EvaluateOperonSize(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, int32_t operonIndex, uint32_t * length, uint32_t * dimensions)
{
	OperonType operonType = expression->operonTypes[operonIndex];
	Operon operon = expression->operons[operonIndex];
	int32_t operonStart = expression->operonStart[operonIndex];
	int32_t operonEnd = expression->operonEnd[operonIndex];
	if (operonType == OperonTypeExpression) { return EvaluateExpressionSize(identifiers, cache, parameters, operon.expression, length, dimensions); }
	else if (operonType == OperonTypeIdentifier) { return EvaluateIdentifierSize(identifiers, cache, parameters, operon, operonStart, operonEnd, length, dimensions); }
	else if (operonType == OperonTypeConstant) { return EvaluateConstantSize(operon, length, dimensions); }
	else if (operonType == OperonTypeVectorLiteral) { return EvaluateVectorLiteralSize(identifiers, cache, parameters, operon, operonStart, operonEnd, length, dimensions); }
	else if (operonType == OperonTypeArrayLiteral) { return EvaluateArrayLiteralSize(identifiers, cache, parameters, operon, operonStart, operonEnd, length, dimensions); }
	return (RuntimeError){ RuntimeErrorNone };

}

static RuntimeError EvaluateOperon(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, int32_t operonIndex, int32_t * indices, int32_t indexCount, VectorArray * result)
{
	OperonType operonType = expression->operonTypes[operonIndex];
	Operon operon = expression->operons[operonIndex];
	int32_t operonStart = expression->operonStart[operonIndex];
	int32_t operonEnd = expression->operonEnd[operonIndex];
	if (operonType == OperonTypeExpression) { return EvaluateExpressionIndices(identifiers, cache, parameters, operon.expression, indices, indexCount, result); }
	else if (operonType == OperonTypeIdentifier) { return EvaluateIdentifier(identifiers, cache, parameters, operon, operonStart, operonEnd, indices, indexCount, result); }
	else if (operonType == OperonTypeConstant) { return EvaluateConstant(operon, result); }
	else if (operonType == OperonTypeVectorLiteral) { return EvaluateVectorLiteral(identifiers, cache, parameters, operon, operonStart, operonEnd, indices, indexCount, result); }
	else if (operonType == OperonTypeArrayLiteral) { return EvaluateArrayLiteral(identifiers, cache, parameters, operon, operonStart, operonEnd, indices, indexCount, result); }
	return (RuntimeError){ RuntimeErrorNone };
}

static RuntimeErrorCode EvaluateEllipsisSize(VectorArray * a, VectorArray * b, uint32_t * length, uint32_t * dimensions)
{
	int32_t lower = roundf(a->xyzw[0][0]);
	int32_t upper = roundf(b->xyzw[0][0]);
	if (upper - lower < 0) { return RuntimeErrorInvalidArrayRange; }
	*length = upper - lower + 1;
	*dimensions = 1;
	return RuntimeErrorNone;
}

static RuntimeErrorCode EvaluateEllipsis(VectorArray * a, VectorArray * b, int32_t * indices, int32_t indexCount, VectorArray * result)
{
	int32_t lower = roundf(a->xyzw[0][0]);
	int32_t upper = roundf(b->xyzw[0][0]);
	if (upper - lower < 0) { return RuntimeErrorInvalidArrayRange; }
	
	if (indices == NULL)
	{
		result->length = upper - lower + 1;
		result->dimensions = 1;
		result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0; i < result->length; i++) { result->xyzw[0][i] = i + lower; }
	}
	else
	{
		result->length = indexCount;
		result->dimensions = 1;
		result->xyzw[0] = malloc(indexCount * sizeof(scalar_t));
		for (int32_t i = 0; i < result->length; i++) { result->xyzw[0][i] = indices[i] > upper - lower || indices[i] < 0 ? NAN : indices[i] + lower; }
	}
	return RuntimeErrorNone;
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
	// dimension is assumed to be the higher of the two operons
	// (differing length vectors return error prior to this unless it's of dimension 1, then consider it a scalar)
	int32_t dimensions = a->dimensions > b->dimensions ? a->dimensions : b->dimensions;
	// length is assumed to be lower of the two operons (again, with exception if an operon is of length 1)
	int32_t length = a->length < b->length ? a->length : b->length;
	// conditionals for whether a and b are scalars
	bool aScalar = false, bScalar = false;
	if (a->length == 1 && b->length > 1) { length = b->length; aScalar = true; }
	if (b->length == 1 && a->length > 1) { length = a->length; bScalar = true; }
	
	// if one operon's dimension is 1 yet the other is greater than 1, then allocate more dimensions
	if (a->dimensions == 1 && b->dimensions > 1)
	{
		for (int32_t d = 1; d < b->dimensions; d++)
		{
			a->xyzw[d] = malloc(a->length * sizeof(scalar_t));
			memcpy(a->xyzw[d], a->xyzw[0], a->length * sizeof(scalar_t));
		}
		a->dimensions = b->dimensions;
	}
	if (b->dimensions == 1 && a->dimensions > 1)
	{
		for (int32_t d = 1; d < a->dimensions; d++)
		{
			b->xyzw[d] = malloc(b->length * sizeof(scalar_t));
			memcpy(b->xyzw[d], b->xyzw[0], b->length * sizeof(scalar_t));
		}
		b->dimensions = a->dimensions;
	}
	
	// apply the operation
	for (int32_t d = 0; d < dimensions; d++)
	{
		for (int32_t i = 0; i < length; i++)
		{
			if (a->length == 1 && b->length > 1) { b->xyzw[d][bScalar ? 0 : i] = Operation(operator, a->xyzw[d][aScalar ? 0 : i], b->xyzw[d][bScalar ? 0 : i]); }
			else { a->xyzw[d][aScalar ? 0 : i] = Operation(operator, a->xyzw[d][aScalar ? 0 : i], b->xyzw[d][bScalar ? 0 : i]); }
		}
	}
	
	// update the length
	if (a->length == 1 && b->length > 1) { b->length = length; }
	else { a->length = length; }
}

static void EvaluateNegation(VectorArray * a)
{
	for (int32_t d = 0; d < a->dimensions; d++)
	{
		for (int32_t i = 0; i < a->length; i++) { a->xyzw[d][i] = -a->xyzw[d][i]; }
	}
}

static RuntimeError EvaluateFunctionCallSize(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, uint32_t * length, uint32_t * dimensions)
{
	String function = expression->operons[0].identifier;
	list(Expression *) expressions = expression->operons[1].expressions;
	int32_t identifierStart = expression->operonStart[0];
	int32_t identifierEnd = expression->operonEnd[0];
	int32_t argumentsEnd = expression->operonEnd[1];
	
	// check if identifier was definined in another statement
	Statement * statement = HashMapGet(identifiers, function);
	if (statement != NULL)
	{
		if (statement->type == StatementTypeVariable) { return (RuntimeError){ RuntimeErrorIdentifierNotFunction, identifierStart, identifierEnd }; }
		if (ListLength(expressions) != ListLength(statement->declaration.function.arguments)) { return (RuntimeError){ RuntimeErrorIncorrectParameterCount, identifierStart, argumentsEnd + 1 }; }
		
		// evaluate each argument's size and store into parameter list
		list(Parameter) arguments = ListCreate(sizeof(Parameter), ListLength(expressions));
		for (int32_t i = 0; i < ListLength(expressions); i++)
		{
			Parameter parameter;
			RuntimeError error = CreateParameter(identifiers, cache, statement->declaration.function.arguments[i], expressions[i], parameters, false, &parameter);
			if (error.code != RuntimeErrorNone) { ListFree(arguments); return error; }
			arguments = ListPush(arguments, &parameter);
		}
		RuntimeError error = EvaluateExpressionSize(identifiers, cache, arguments, statement->expression, length, dimensions);
		if (error.statement == NULL) { error.statement = statement; } // set the correct statement for error reporting
		
		ListFree(arguments);
		return error;
	}
	
	// otherwise check if it's a builtin function
	BuiltinFunction builtin = DetermineBuiltinFunction(function);
	if (builtin != BuiltinFunctionNone)
	{
		// if it's a single argument builtin then supply result as both the parameter and result of the function
		if (IsFunctionSingleArgument(builtin))
		{
			if (ListLength(expressions) != 1) { return (RuntimeError){ RuntimeErrorIncorrectParameterCount, identifierStart, argumentsEnd + 1 }; }
			RuntimeError error = EvaluateExpressionSize(identifiers, cache, parameters, expressions[0], length, dimensions);
			if (error.code != RuntimeErrorNone) { return error; }
			return (RuntimeError){ EvaluateBuiltinFunctionSize(builtin, NULL, length, dimensions), identifierStart, argumentsEnd + 1 };
		}
		
		// otherwise evaluate each argument and store into parameter list
		list(VectorArray) arguments = ListCreate(sizeof(VectorArray), ListLength(expressions));
		for (int32_t i = 0; i < ListLength(expressions); i++)
		{
			arguments = ListPush(arguments, &(VectorArray){ 0 });
			RuntimeError error = EvaluateExpressionSize(identifiers, cache, parameters, expressions[i], &arguments[i].length, &arguments[i].dimensions);
			if (error.code != RuntimeErrorNone) { ListFree(arguments); return error; }
		}
		RuntimeErrorCode error = EvaluateBuiltinFunctionSize(builtin, arguments, length, dimensions);
		ListFree(arguments);
		return (RuntimeError){ error, identifierStart, argumentsEnd + 1 };
	}
	
	return (RuntimeError){ RuntimeErrorUndefinedIdentifier, identifierStart, identifierEnd };
}

static RuntimeError EvaluateFunctionCall(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, int32_t * indices, int32_t indexCount, VectorArray * result)
{
	String function = expression->operons[0].identifier;
	list(Expression *) expressions = expression->operons[1].expressions;
	int32_t identifierStart = expression->operonStart[0];
	int32_t identifierEnd = expression->operonEnd[0];
	int32_t argumentsEnd = expression->operonEnd[1];
	
	// check if identifier was definined in another statement
	Statement * statement = HashMapGet(identifiers, function);
	if (statement != NULL)
	{
		if (statement->type == StatementTypeVariable) { return (RuntimeError){ RuntimeErrorIdentifierNotFunction, identifierStart, identifierEnd }; }
		if (ListLength(expressions) != ListLength(statement->declaration.function.arguments)) { return (RuntimeError){ RuntimeErrorIncorrectParameterCount, identifierStart, argumentsEnd + 1 }; }
		
		// evaluate each argument and store into parameter list
		list(Parameter) arguments = ListCreate(sizeof(Parameter), ListLength(expressions));
		for (int32_t i = 0; i < ListLength(expressions); i++)
		{
			Parameter parameter;
			RuntimeError error = CreateParameter(identifiers, cache, statement->declaration.function.arguments[i], expressions[i], parameters, indices == NULL, &parameter);
			if (error.code != RuntimeErrorNone) { ListFree(arguments); return error; }
			arguments = ListPush(arguments, &parameter);
		}
		RuntimeError error = EvaluateExpressionIndices(identifiers, cache, arguments, statement->expression, indices, indexCount, result);
		if (error.statement == NULL) { error.statement = statement; } // set the correct statement for error reporting
		
		for (int32_t i = 0; i < ListLength(arguments); i++) { FreeParameter(arguments[i]); }
		ListFree(arguments);
		return error;
	}
	
	// otherwise check if it's a builtin function
	BuiltinFunction builtin = DetermineBuiltinFunction(function);
	if (builtin != BuiltinFunctionNone)
	{
		// if it's a single argument builtin then supply result as both the parameter and result of the function
		if (IsFunctionSingleArgument(builtin))
		{
			if (ListLength(expressions) != 1) { return (RuntimeError){ RuntimeErrorIncorrectParameterCount, identifierStart, argumentsEnd + 1 }; }
			RuntimeError error = EvaluateExpressionIndices(identifiers, cache, parameters, expressions[0], IsFunctionIndexable(builtin) ? indices : NULL, indexCount, result);
			if (error.code != RuntimeErrorNone) { return error; }
			return (RuntimeError){ EvaluateBuiltinFunction(builtin, NULL, result), identifierStart, argumentsEnd + 1 };
		}
		
		// otherwise evaluate each argument and store into parameter list
		list(VectorArray) arguments = ListCreate(sizeof(VectorArray), ListLength(expressions));
		for (int32_t i = 0; i < ListLength(expressions); i++)
		{
			arguments = ListPush(arguments, &(VectorArray){ 0 });
			RuntimeError error = EvaluateExpressionIndices(identifiers, cache, parameters, expressions[i], IsFunctionIndexable(builtin) ? indices : NULL, indexCount, &arguments[i]);
			if (error.code != RuntimeErrorNone)
			{
				for (int32_t j = 0; j < i; j++) { FreeVectorArray(arguments[j]); }
				ListFree(arguments);
				return error;
			}
		}
		VectorArray value;
		RuntimeErrorCode error = EvaluateBuiltinFunction(builtin, arguments, &value);
		*result = CopyVectorArray(value, indices, indexCount);
		FreeVectorArray(value);
		// free the arguments
		for (int32_t i = 0; i < ListLength(arguments); i++) { FreeVectorArray(arguments[i]); }
		ListFree(arguments);
		return (RuntimeError){ error, identifierStart, argumentsEnd + 1 };
	}
	
	// if not builtin then return undefined identifier error
	return (RuntimeError){ RuntimeErrorUndefinedIdentifier, identifierStart, identifierEnd };
}

static RuntimeError EvaluateForSize(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, uint32_t * length, uint32_t * dimensions)
{
	*length = 0;
	*dimensions = 0;
	Operon operon = expression->operons[0];
	int32_t operonStart = expression->operonStart[0];
	int32_t operonEnd = expression->operonEnd[0];
	ForAssignment assignment = expression->operons[1].assignment;
	
	// evaluate the assignment size
	uint32_t assignmentLen, assignmentDimen;
	RuntimeError error = EvaluateExpressionSize(identifiers, cache, parameters, assignment.expression, &assignmentLen, &assignmentDimen);
	if (error.code != RuntimeErrorNone) { return error; }
	if (operon.expression->operator != OperatorFor) { *length = assignmentLen; }

	// if there doesn't already exist a parameters list, create one or otherwise clone the existing one
	list(Parameter) originalParams = parameters;
	if (parameters == NULL) { parameters = ListCreate(sizeof(Parameter), 1); }
	else { parameters = ListClone(parameters); }
	parameters = ListInsert(parameters, &(Parameter){ .identifier = assignment.identifier, .cached = true }, 0);
	parameters[0].cache.length = 1;
	parameters[0].cache.dimensions = assignmentDimen;
	
	// go through each assignment element and evaluate the corresponding value
	for (int32_t i = 0; i < assignmentLen; i++)
	{
		if (operon.expression->operator == OperatorFor)
		{
			RuntimeError error = EvaluateExpressionIndices(identifiers, cache, originalParams, assignment.expression, &i, 1, &parameters[0].cache);
			if (error.code != RuntimeErrorNone) { return error; }
		}
		
		uint32_t len, dimen;
		RuntimeError error = EvaluateOperonSize(identifiers, cache, parameters, expression, 0, &len, &dimen);
		if (error.code != RuntimeErrorNone) { ListFree(parameters); return error; }
		
		if (*dimensions == 0)
		{
			*dimensions = dimen;
			if (operon.expression->operator != OperatorFor) { *dimensions = dimen; return (RuntimeError){ RuntimeErrorNone }; }
		}
		if (dimen != *dimensions) { ListFree(parameters); return (RuntimeError){ RuntimeErrorNonUniformArray, operonStart, operonEnd }; }
		if (len > 1 && operon.expression->operator != OperatorFor) { ListFree(parameters); return (RuntimeError){ RuntimeErrorArrayInsideArray, operonStart, operonEnd }; }
		*length += len;
		
		if (operon.expression->operator == OperatorFor) { FreeVectorArray(parameters[0].cache); }
	}
	ListFree(parameters);
	return (RuntimeError){ RuntimeErrorNone };
}

static RuntimeError EvaluateFor(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, int32_t * indices, int32_t indexCount, VectorArray * result)
{
	*result = (VectorArray){ 0 };
	ForAssignment assignment = expression->operons[1].assignment;
	
	VectorArray assignmentValue;
	RuntimeError error;
	if (expression->operons[0].expression->operator != OperatorFor) { error = EvaluateExpression(identifiers, cache, parameters, assignment.expression, &assignmentValue); }
	else { error = EvaluateExpressionIndices(identifiers, cache, parameters, assignment.expression, indices, indexCount, &assignmentValue); }
	if (error.code != RuntimeErrorNone) { return error; }
	
	// if there doesn't already exist a parameters list, create one or otherwise clone the existing one
	if (parameters == NULL) { parameters = ListCreate(sizeof(Parameter), 1); }
	else { parameters = ListClone(parameters); }
	parameters = ListInsert(parameters, &(Parameter){ .identifier = assignment.identifier, .cached = true }, 0);
	parameters[0].cache.length = 1;
	parameters[0].cache.dimensions = 1;
	
	if (indices == NULL)
	{
		RuntimeError error = EvaluateForSize(identifiers, cache, parameters, expression, &result->length, &result->dimensions);
		if (error.code != RuntimeErrorNone) { FreeVectorArray(assignmentValue); return error; }
		for (int32_t d = 0; d < result->dimensions; d++) { result->xyzw[d] = malloc(result->length * sizeof(scalar_t)); }
		
		// go through each assignment element and evaluate the corresponding value
		for (int32_t i = 0, offset = 0; i < assignmentValue.length; i++)
		{
			parameters[0].cache.xyzw[0] = &assignmentValue.xyzw[0][i];
			
			VectorArray element;
			RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression, 0, NULL, 0, &element);
			if (error.code != RuntimeErrorNone) { FreeVectorArray(assignmentValue); FreeVectorArray(*result); return error; }
			
			for (int32_t d = 0; d < result->dimensions; d++) { memcpy(result->xyzw[d] + offset, element.xyzw[d], element.length * sizeof(scalar_t)); }
			offset += element.length;
		}
	}
	else
	{
		*result = (VectorArray){ .length = indexCount };
		RuntimeError error = EvaluateForSize(identifiers, cache, parameters, expression, &(uint32_t){ 0 }, &result->dimensions);
		if (error.code != RuntimeErrorNone) { return error; }
		for (int32_t d = 0; d < result->dimensions; d++) { result->xyzw[d] = malloc(result->length * sizeof(scalar_t)); }

		for (int32_t i = 0; i < indexCount; i++)
		{
			for (int32_t j = 0, offset = 0; j < assignmentValue.length; j++)
			{
				parameters[0].cache.xyzw[0] = &assignmentValue.xyzw[0][j];
				
				uint32_t length;
				RuntimeError error = EvaluateOperonSize(identifiers, cache, parameters, expression, 0, &length, &(uint32_t){ 0 });
				if (error.code != RuntimeErrorNone) { FreeVectorArray(assignmentValue); FreeVectorArray(*result); return error; }
				
				if (offset + length > indices[i])
				{
					VectorArray element;
					RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression, 0, &(int32_t){ indices[i] - offset }, 1, &element);
					if (error.code != RuntimeErrorNone) { FreeVectorArray(assignmentValue); FreeVectorArray(*result); return error; }
					for (int32_t d = 0; d < result->dimensions; d++) { result->xyzw[d][i] = element.xyzw[d][0]; }
					break;
				}
				
				offset += length;
				if (offset == result->length)
				{
					for (int32_t d = 0; d < result->dimensions; d++) { result->xyzw[d][i] = NAN; }
				}
			}
		}
	}
	
	FreeVectorArray(assignmentValue);
	ListFree(parameters);
	return (RuntimeError){ RuntimeErrorNone };
}

static bool IsIdentifierSwizzling(String identifier)
{
	for (int32_t i = 0; i < StringLength(identifier); i++)
	{
		if (i >= 4 || (identifier[i] != 'x' && identifier[i] != 'y' && identifier[i] != 'z' && identifier[i] != 'w')) { return false; }
	}
	return true;
}

static RuntimeError EvaluateIndexSize(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, uint32_t * length, uint32_t * dimensions)
{
	if (expression->operonTypes[1] == OperonTypeIdentifier && IsIdentifierSwizzling(expression->operons[1].identifier))
	{
		RuntimeError error = EvaluateOperonSize(identifiers, cache, parameters, expression, 0, length, &(uint32_t){ 0 });
		if (error.code != RuntimeErrorNone) { return error; }
		*dimensions = StringLength(expression->operons[1].identifier);
		return (RuntimeError){ RuntimeErrorNone };
	}
	
	// evaluate length from indexing, dimension from indexed
	RuntimeError error = EvaluateOperonSize(identifiers, cache, parameters, expression, 0, &(uint32_t){ 0 }, dimensions);
	error = EvaluateOperonSize(identifiers, cache, parameters, expression, 1, length, &(uint32_t){ 0 });
	if (error.code != RuntimeErrorNone) { return error; }
	return (RuntimeError){ RuntimeErrorNone };
}

static RuntimeError EvaluateIndex(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, int32_t * indices, int32_t indexCount, VectorArray * result)
{
	// evaluates vector swizzling
	if (expression->operonTypes[1] == OperonTypeIdentifier && IsIdentifierSwizzling(expression->operons[1].identifier))
	{
		String identifier = expression->operons[1].identifier;
		// evaluate the indexed operon
		VectorArray indexed;
		RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression, 0, indices, indexCount, &indexed);
		if (error.code != RuntimeErrorNone) { return error; }
		
		result->dimensions = StringLength(identifier);
		result->length = indexed.length;
		bool shouldDuplicate[4] = { false, false, false, false }; // keeps track of duplicate vectors (e.g. .xxyz)
		for (int32_t d = 0; d < result->dimensions; d++)
		{
			int32_t component = 0;
			if (identifier[d] == 'x') { component = 0; }
			if (identifier[d] == 'y') { component = 1; }
			if (identifier[d] == 'z') { component = 2; }
			if (identifier[d] == 'w') { component = 3; }
			// return error if trying to access component greater than number of dimensions
			if (component >= indexed.dimensions)
			{
				FreeVectorArray(indexed);
				return (RuntimeError){ RuntimeErrorInvalidSwizzling, expression->operonStart[1], expression->operonEnd[1] };
			}
			
			// simply move the pointer if it's not a duplicate component
			if (!shouldDuplicate[component]) { result->xyzw[d] = indexed.xyzw[component]; shouldDuplicate[component] = true; }
			else
			{
				// otherwise copy the contents
				result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
				memcpy(result->xyzw[d], indexed.xyzw[component], result->length * sizeof(scalar_t));
			}
		}
		// free any components that went unused
		for (int32_t d = 0; d < indexed.dimensions; d++)
		{
			if (!shouldDuplicate[d]) { free(indexed.xyzw[d]); }
		}
		return (RuntimeError){ RuntimeErrorNone };
	}
	
	VectorArray indexing;
	RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression, 1, indices, indexCount, &indexing);
	if (error.code != RuntimeErrorNone) { return error; }
	
	// return error if indexing with a vector
	if (indexing.dimensions > 1)
	{
		FreeVectorArray(indexing);
		return (RuntimeError){ RuntimeErrorIndexingWithVector, expression->operonStart[1], expression->operonEnd[1] };
	}
	
	int32_t * nextIndices = malloc(indexing.length * sizeof(int32_t));
	for (int32_t i = 0; i < indexing.length; i++) { nextIndices[i] = roundf(indexing.xyzw[0][i]); }
	error = EvaluateOperon(identifiers, cache, parameters, expression, 0, nextIndices, indexing.length, result);
	free(nextIndices);
	return error;
}

RuntimeError EvaluateExpressionSize(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, uint32_t * length, uint32_t * dimensions)
{
	*length = 0;
	*dimensions = 0;
	if (expression->operator == OperatorNone) { return EvaluateOperonSize(identifiers, cache, parameters, expression, 0, length, dimensions); }
	if (expression->operator == OperatorFunctionCall) { return EvaluateFunctionCallSize(identifiers, cache, parameters, expression, length, dimensions); }
	if (expression->operator == OperatorFor) { return EvaluateForSize(identifiers, cache, parameters, expression, length, dimensions); }
	if (expression->operator == OperatorIndex) { return EvaluateIndexSize(identifiers, cache, parameters, expression, length, dimensions); }
	if (expression->operator == OperatorPositive || expression->operator == OperatorNegative) { return EvaluateOperonSize(identifiers, cache, parameters, expression, 0, length, dimensions); }
	
	uint32_t len1, dimen1;
	uint32_t len2, dimen2;
	RuntimeError error = EvaluateOperonSize(identifiers, cache, parameters, expression, 0, &len1, &dimen1);
	error = EvaluateOperonSize(identifiers, cache, parameters, expression, 1, &len2, &dimen2);
	if (error.code != RuntimeErrorNone) { return error; }
	
	if (expression->operator == OperatorEllipsis)
	{
		// return error if trying to use ellipsis with array or vector
		if (len1 > 1 || dimen1 > 1) { return (RuntimeError){ RuntimeErrorInvalidEllipsisOperon, expression->operonStart[0], expression->operonEnd[0] }; }
		if (len2 > 1 || dimen2 > 1) { return (RuntimeError){ RuntimeErrorInvalidEllipsisOperon, expression->operonStart[1], expression->operonEnd[1] }; }
		VectorArray left, right;
		RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression, 0, NULL, 0, &left);
		error = EvaluateOperon(identifiers, cache, parameters, expression, 1, NULL, 0, &right);
		if (error.code != RuntimeErrorNone) { return error; }
		error = (RuntimeError){ EvaluateEllipsisSize(&left, &right, length, dimensions), expression->operonStart[0], expression->operonEnd[1] };
		FreeVectorArray(left);
		FreeVectorArray(right);
		return error;
	}
	
	if (dimen1 != dimen2 && dimen1 != 1 && dimen2 != 1) { return (RuntimeError){ RuntimeErrorDifferingLengthVectors, expression->operonStart[0], expression->operonEnd[1] }; }
	if (len1 == 1) { len1 = len2; }
	if (len2 == 1) { len2 = len1; }
	*dimensions = dimen1 > dimen2 ? dimen1 : dimen2;
	*length = len1 < len2 ? len1 : len2;
	return (RuntimeError){ RuntimeErrorNone };
}

RuntimeError EvaluateExpressionIndices(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, int32_t * indices, int32_t indexCount, VectorArray * result)
{
	*result = (VectorArray){ 0 };
	if (expression->operator == OperatorNone) { return EvaluateOperon(identifiers, cache, parameters, expression, 0, indices, indexCount, result); }
	if (expression->operator == OperatorFunctionCall) { return EvaluateFunctionCall(identifiers, cache, parameters, expression, indices, indexCount, result); }
	if (expression->operator == OperatorFor) { return EvaluateFor(identifiers, cache, parameters, expression, indices, indexCount, result); }
	if (expression->operator == OperatorIndex) { return EvaluateIndex(identifiers, cache, parameters, expression, indices, indexCount, result); }
	
	VectorArray value = { 0 };
	if (expression->operator == OperatorPositive || expression->operator == OperatorNegative)
	{
		// evaluate only the first operon
		RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression, 0, indices, indexCount, result);
		if (error.code != RuntimeErrorNone) { return error; }
	}
	else
	{
		// evaluate both operons (evaluate the left one into result)
		RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression, 0, indices, indexCount, result);
		if (error.code != RuntimeErrorNone) { return error; }
		error = EvaluateOperon(identifiers, cache, parameters, expression, 1, indices, indexCount, &value);
		if (error.code != RuntimeErrorNone) { return error; }
	}
	
	if (expression->operator == OperatorEllipsis)
	{
		// return error if trying to use ellipsis with array or vector
		if (result->dimensions > 1 || result->length > 1) { return (RuntimeError){ RuntimeErrorInvalidEllipsisOperon, expression->operonStart[0], expression->operonEnd[0] }; }
		if (value.dimensions > 1 || value.length > 1) { return (RuntimeError){ RuntimeErrorInvalidEllipsisOperon, expression->operonStart[1], expression->operonEnd[1] }; }
		VectorArray lower = *result;
		RuntimeErrorCode error = EvaluateEllipsis(&lower, &value, indices, indexCount, result);
		free(lower.xyzw[0]);
		if (error != RuntimeErrorNone) { return (RuntimeError){ error, expression->operonStart[0], expression->operonEnd[1] }; }
	}
	else if (expression->operator == OperatorNegative) { EvaluateNegation(result); }
	else if (expression->operator == OperatorPositive) {}
	else
	{
		// return error if trying to operate with differing length vectors
		if (result->dimensions != value.dimensions && result->dimensions != 1 && value.dimensions != 1) { return (RuntimeError){ RuntimeErrorDifferingLengthVectors, expression->operonStart[0], expression->operonEnd[1] }; }
		
		// evaluate operator
		EvaluateOperation(expression->operator, result, &value);
		// swap result with value if a was operated onto b
		if (result->length == 1 && value.length > 1)
		{
			VectorArray swap = *result;
			*result = value;
			value = swap;
		}
	}
	if (value.length > 0) { FreeVectorArray(value); }
	return (RuntimeError){ RuntimeErrorNone };
}

RuntimeError EvaluateExpression(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, VectorArray * result)
{
	return EvaluateExpressionIndices(identifiers, cache, parameters, expression, NULL, 0, result);
}

static int CompareIdentifier(const void * a, const void * b)
{
	return strcmp(*(String *)a, *(String *)b);
}

list(String) FindExpressionParents(HashMap identifiers, Expression * expression, list(String) parameters)
{
	list(String) parents = ListCreate(sizeof(Statement *), 1);
	
	// create parameters list if doesn't exist otherwise create a copy
	if (parameters == NULL) { parameters = ListCreate(sizeof(String), 1); }
	else { parameters = ListClone(parameters); }
	
	// if it's a for operator, add the assignment to the parameters list
	if (expression->operator == OperatorFor) { parameters = ListPush(parameters, &expression->operons[1].assignment.identifier); }
	
	// go through one or both operons
	int32_t operons = expression->operator == OperatorFor || expression->operator == OperatorNone || expression->operator == OperatorNegative || expression->operator == OperatorPositive ? 1 : 2;
	for (int32_t i = 0; i < operons; i++)
	{
		if (expression->operonTypes[i] == OperonTypeArrayLiteral || expression->operonTypes[i] == OperonTypeVectorLiteral || expression->operonTypes[i] == OperonTypeArguments)
		{
			// recursively go through each expression and add its parents to the list
			for (int32_t j = 0; j < ListLength(expression->operons[i].expressions); j++)
			{
				list(String) argDependents = FindExpressionParents(identifiers, expression->operons[i].expressions[j], parameters);
				for (int32_t k = 0; k < ListLength(argDependents); k++) { parents = ListPush(parents, &argDependents[k]); }
				ListFree(argDependents);
			}
		}
		else if (expression->operonTypes[i] == OperonTypeExpression)
		{
			// recursively go through this expression and find its parents
			list(String) expDependents = FindExpressionParents(identifiers, expression->operons[i].expression, parameters);
			for (int32_t k = 0; k < ListLength(expDependents); k++) { parents = ListPush(parents, &expDependents[k]); }
			ListFree(expDependents);
		}
		else if (expression->operonTypes[i] == OperonTypeIdentifier)
		{
			// skip identifier if it's a parameter from a for assignment
			if (parameters != NULL)
			{
				bool isParameter = false;
				for (int32_t j = 0; j < ListLength(parameters); j++) { if (StringEquals(parameters[j], expression->operons[i].identifier)) { isParameter = true; break; } }
				if (isParameter) { continue; }
			}
			
			Statement * statement = HashMapGet(identifiers, expression->operons[i].identifier);
			if (statement != NULL)
			{
				if (statement->type == StatementTypeFunction)
				{
					for (int32_t j = 0; j < ListLength(statement->declaration.function.arguments); j++) { parameters = ListPush(parameters, &statement->declaration.function.arguments[j]); }
				}
				if (statement->type == StatementTypeVariable)
				{
					parents = ListPush(parents, &expression->operons[i].identifier);
				}
				list(String) subParents = FindExpressionParents(identifiers, statement->expression, NULL);
				for (int32_t j = 0; j < ListLength(subParents); j++) { parents = ListPush(parents, &subParents[j]); }
				ListFree(subParents);
			}
			else if (DetermineBuiltinVariable(expression->operons[i].identifier) != BuiltinVariableNone) { parents = ListPush(parents, &expression->operons[i].identifier); }
		}
	}
	
	// free list that was cloned
	ListFree(parameters);
	
	// remove duplicates from list
	qsort(parents, ListLength(parents), ListElementSize(parents), CompareIdentifier);
	for (int32_t i = 1; i < ListLength(parents); i++)
	{
		if (strcmp(parents[i], parents[i - 1]) == 0)
		{
			parents = ListRemove(parents, i);
			i--;
		}
	}
	
	return parents;
}
