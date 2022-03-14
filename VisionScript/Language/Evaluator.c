#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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
		default: return "unknown error";
	}
}

void VectorArrayPrint(VectorArray value)
{
	if (value.length > 1) { printf("["); }
	for (int32_t i = 0; i < value.length; i++)
	{
		if (value.dimensions > 1) { printf("("); }
		for (int32_t j = 0; j < value.dimensions; j++)
		{
			// print as an integer if float is an integer
			if (value.xyzw[j][i] - (int32_t)value.xyzw[j][i] == 0) { printf("%i", (int32_t)value.xyzw[j][i]); }
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

static RuntimeError EvaluateOperon(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, int32_t operonIndex, VectorArray * result)
{
	OperonType operonType = expression->operonTypes[operonIndex];
	Operon operon = expression->operons[operonIndex];
	int32_t operonStart = expression->operonStart[operonIndex];
	int32_t operonEnd = expression->operonEnd[operonIndex];
	
	if (operonType == OperonTypeExpression) { return EvaluateExpression(identifiers, cache, parameters, operon.expression, result); } // recursively call EvaluateExpression
	else if (operonType == OperonTypeIdentifier)
	{
		// first check if the identifier was passed as a parameter
		if (parameters != NULL)
		{
			for (int32_t i = 0; i < ListLength(parameters); i++)
			{
				if (strcmp(operon.identifier, parameters[i].identifier) == 0)
				{
					// copy the contents of parameter if there's a match
					result->dimensions = parameters[i].value.dimensions;
					result->length = parameters[i].value.length;
					for (int8_t d = 0; d < result->dimensions; d++)
					{
						result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
						memcpy(result->xyzw[d], parameters[i].value.xyzw[d], result->length * sizeof(scalar_t));
					}
					return (RuntimeError){ RuntimeErrorNone };
				}
			}
		}
		
		// next check if the identifier was defined in another statement
		Statement * statement = HashMapGet(identifiers, operon.identifier);
		if (statement != NULL)
		{
			// return error if identifier was defined as a function
			if (statement->type == StatementTypeFunction) { return (RuntimeError){ RuntimeErrorIdentifierNotVariable, operonStart, operonEnd }; }
			// check if there's a cached value that can be used
			VectorArray * value = HashMapGet(cache, operon.identifier);
			if (value != NULL)
			{
				// copy contents if there is
				result->length = value->length;
				result->dimensions = value->dimensions;
				for (int8_t d = 0; d < result->dimensions; d++)
				{
					result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
					memcpy(result->xyzw[d], value->xyzw[d], result->length * sizeof(scalar_t));
				}
				return (RuntimeError){ RuntimeErrorNone };
			}
			// otherwise evaluate the expression corresponding to the identifier
			RuntimeError error = EvaluateExpression(identifiers, cache, NULL, statement->expression, result);
			if (error.statement == NULL) { error.statement = statement; } // set the correct statement for error reporting
			return error;
		}
	
		// finally, check if it's a builtin variable
		BuiltinVariable builtin = DetermineBuiltinVariable(operon.identifier);
		if (builtin != BuiltinVariableNone) { return (RuntimeError){ EvaluateBuiltinVariable(builtin, result), operonStart, operonEnd }; }
		
		// otherwise return an error
		return (RuntimeError){ RuntimeErrorUndefinedIdentifier, operonStart, operonEnd };
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
		
		// evaluate each vector component
		VectorArray components[4];
		for (int32_t i = 0; i < result->dimensions; i++)
		{
			RuntimeError error = EvaluateExpression(identifiers, cache, parameters, operon.expressions[i], components + i);
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
	}
	else if (operonType == OperonTypeArrayLiteral)
	{
		result->dimensions = 0;
		result->length = 0;
		
		// evaluate each element of the array and store them in elements temporarily
		VectorArray * elements = malloc(sizeof(VectorArray) * ListLength(operon.expressions));
		for (int32_t i = 0; i < ListLength(operon.expressions); i++)
		{
			RuntimeError error = EvaluateExpression(identifiers, cache, parameters, operon.expressions[i], elements + i);
			if (error.code != RuntimeErrorNone) { return error; }
			// dimension of array is defined to be dimension of the first element
			if (result->dimensions == 0) { result->dimensions = elements[i].dimensions; }
			// if an element's dimension isn't the same as the previous elements, return an error
			if (elements[i].dimensions != result->dimensions) { return (RuntimeError){ RuntimeErrorNonUniformArray, operonStart, operonEnd }; }
			// if an element's length > 1 and it's not from ellipsis or for operator, then that means there's an array inside array so return error
			if (elements[i].length > 1 && operon.expressions[i]->operator != OperatorFor && operon.expressions[i]->operator != OperatorEllipsis) { return (RuntimeError){ RuntimeErrorArrayInsideArray, operonStart, operonEnd }; }
			// increment total length by each element's length
			result->length += elements[i].length;
		}
		
		// return error if array too large
		if (result->length > MAX_ARRAY_LENGTH) { return (RuntimeError){ RuntimeErrorArrayTooLarge, operonStart, operonEnd }; }
		
		// copy each contents of elements into the final array
		for (int32_t i = 0; i < result->dimensions; i++)
		{
			// if there's only one element then just move the pointer to save time
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
	return (RuntimeError){ RuntimeErrorNone };
}

static RuntimeErrorCode EvaluateEllipsis(VectorArray * a, VectorArray * b, VectorArray * result)
{
	int32_t lower = roundf(a->xyzw[0][0]);
	int32_t upper = roundf(b->xyzw[0][0]);
	
	// if range of ellipsis is out of range, return an error
	if (upper - lower >= MAX_ARRAY_LENGTH) { return RuntimeErrorArrayTooLarge; }
	if (upper - lower < 0) { return RuntimeErrorInvalidArrayRange; }
	
	result->length = upper - lower + 1;
	result->dimensions = 1;
	result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
	for (int32_t i = 0; i < result->length; i++) { result->xyzw[0][i] = i + lower; }
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
	int8_t dimensions = a->dimensions > b->dimensions ? a->dimensions : b->dimensions;
	// length is assumed to be lower of the two operons (again, with exception if an operon is of length 1)
	int32_t length = a->length < b->length ? a->length : b->length;
	
	// if one operon's dimension is 1 yet the other is greater than 1, then allocate more dimensions
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
	
	// conditionals for whether a and b are scalars
	bool aScalar = false, bScalar = false;
	if (a->length == 1 && b->length > 1) { length = b->length; aScalar = true; }
	if (b->length == 1 && a->length > 1) { length = a->length; bScalar = true; }
	
	// apply the operation
	for (int8_t d = 0; d < dimensions; d++)
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

static void EvaluateNegate(VectorArray * a)
{
	// apply negation to vector array
	for (int8_t d = 0; d < a->dimensions; d++)
	{
		for (int32_t i = 0; i < a->length; i++)
		{
			a->xyzw[d][i] = -a->xyzw[d][i];
		}
	}
}

static RuntimeError EvaluateFunctionCall(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, VectorArray * result)
{
	String function = expression->operons[0].identifier;
	list(Expression *) expressions = expression->operons[1].expressions;
	int32_t identifierStart = expression->operonStart[0];
	int32_t identifierEnd = expression->operonEnd[0];
	int32_t argumentsStart = expression->operonStart[1];
	int32_t argumentsEnd = expression->operonEnd[1];
	
	// check if identifier was definined in another statement
	Statement * statement = HashMapGet(identifiers, function);
	if (statement != NULL)
	{
		// return error if identifier is a variable
		if (statement->type == StatementTypeVariable) { return (RuntimeError){ RuntimeErrorIdentifierNotFunction, identifierStart, identifierEnd }; }
		// return error if number of arguments needed isn't equal to number of argments given
		if (ListLength(expressions) != ListLength(statement->declaration.function.arguments)) { return (RuntimeError){ RuntimeErrorIncorrectParameterCount, argumentsStart, argumentsEnd }; }
		
		// evaluate each argument and store into parameter list
		list(Parameter) arguments = ListCreate(sizeof(Parameter), ListLength(expressions));
		for (int32_t i = 0; i < ListLength(expressions); i++)
		{
			Parameter parameter = { .identifier = statement->declaration.function.arguments[i] };
			RuntimeError error = EvaluateExpression(identifiers, cache, parameters, expressions[i], &parameter.value);
			if (error.code != RuntimeErrorNone) { return error; }
			ListPush((void **)&arguments, &parameter);
		}
		// evaluate the function
		RuntimeError error = EvaluateExpression(identifiers, cache, arguments, statement->expression, result);
		if (error.statement == NULL) { error.statement = statement; } // set the correct statement for error reporting
		
		// free the arguments
		for (int32_t i = 0; i < ListLength(arguments); i++) { for (int32_t d = 0; d < arguments[i].value.dimensions; d++) { free(arguments[i].value.xyzw[d]); } }
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
			if (ListLength(expressions) > 1) { return (RuntimeError){ RuntimeErrorIncorrectParameterCount, argumentsStart, argumentsEnd }; }
			RuntimeError error = EvaluateExpression(identifiers, cache, parameters, expressions[0], result);
			if (error.code != RuntimeErrorNone) { return error; }
			return (RuntimeError){ EvaluateBuiltinFunction(builtin, NULL, result), identifierStart, argumentsEnd };
		}
		
		// otherwise evaluate each argument and store into parameter list
		list(VectorArray) arguments = ListCreate(sizeof(VectorArray), ListLength(expressions));
		for (int32_t i = 0; i < ListLength(expressions); i++)
		{
			ListPush((void **)&arguments, &(VectorArray){ 0 });
			RuntimeError error = EvaluateExpression(identifiers, cache, parameters, expressions[i], &arguments[i]);
			if (error.code != RuntimeErrorNone) { return error; }
		}
		// evaluate the function
		RuntimeErrorCode error = EvaluateBuiltinFunction(builtin, arguments, result);
		// free the arguments
		for (int32_t i = 0; i < ListLength(arguments); i++) { for (int8_t d = 0; d < arguments[i].dimensions; d++) { free(arguments[i].xyzw[d]); } }
		ListFree(arguments);
		return (RuntimeError){ error, identifierStart, argumentsEnd };
	}

	// if not builtin then return undefined identifier error
	return (RuntimeError){ RuntimeErrorUndefinedIdentifier, identifierStart, identifierEnd };
}

static RuntimeError EvaluateFor(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, VectorArray * result)
{
	OperonType operonType = expression->operonTypes[0];
	Operon operon = expression->operons[0];
	int32_t operonStart = expression->operonStart[0];
	int32_t operonEnd = expression->operonEnd[0];
	ForAssignment assignment = expression->operons[1].assignment;
	
	// first evaluate the assignment
	*result = (VectorArray){ 0 };
	VectorArray assignmentValue = { 0 };
	RuntimeError error = EvaluateExpression(identifiers, cache, parameters, assignment.expression, &assignmentValue);
	if (error.code != RuntimeErrorNone) { return error; }
	
	// next allocate enough values for each element in the assignment
	VectorArray * values = malloc(assignmentValue.length * sizeof(VectorArray));
	// if there doesn't already exist a parameters list, create one or otherwise clone the existing one
	if (parameters == NULL) { parameters = ListCreate(sizeof(Parameter), 1); }
	else { parameters = ListClone(parameters); }
	// add the assignment identifier to the front of the parameter list
	ListInsert((void **)&parameters, &(Parameter){ .identifier = assignment.identifier }, 0);
	parameters[0].value.length = 1;
	parameters[0].value.dimensions = assignmentValue.dimensions;
	
	// go through each assignment element and evaluate the corresponding value
	for (int32_t i = 0; i < assignmentValue.length; i++)
	{
		// change the parameter for each element in the assignment
		for (int8_t d = 0; d < assignmentValue.dimensions; d++) { parameters[0].value.xyzw[d] = &assignmentValue.xyzw[d][i]; }
		RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression, 0, &values[i]);
		if (error.code != RuntimeErrorNone) { return error; }
		
		// set the result dimensions to the dimension of the first element of the array
		if (result->dimensions == 0) { result->dimensions = values[i].dimensions; }
		// return error if inconsistent dimensionality
		if (values[i].dimensions != result->dimensions) { return (RuntimeError){ RuntimeErrorNonUniformArray, operonStart, operonEnd }; }
		// return error if array inside array
		if (values[i].length > 1)
		{
			if (operonType != OperonTypeExpression) { return (RuntimeError){ RuntimeErrorArrayInsideArray, operonStart, operonEnd }; }
			if (operon.expression->operator != OperatorFor) { return (RuntimeError){ RuntimeErrorArrayInsideArray, operonStart, operonEnd }; }
		}
		result->length += values[i].length; // increment final array length
	}
	
	// return error if array too large
	if (result->length > MAX_ARRAY_LENGTH) { return (RuntimeError){ RuntimeErrorArrayTooLarge, expression->operonStart[1], expression->operonEnd[1] }; }
	
	// copy each of the values into the final array
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
	
	return (RuntimeError){ RuntimeErrorNone };
}

static RuntimeError EvaluateIndex(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, VectorArray * result)
{
	if (expression->operonTypes[1] == OperonTypeIdentifier)
	{
		// if the indexing operon is an identifier then check if it's vector swizzling
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
			// evaluate the indexed operon
			VectorArray value = { 0 };
			RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression, 0, &value);
			if (error.code != RuntimeErrorNone) { return error; }
			
			result->dimensions = StringLength(identifier);
			result->length = value.length;
			bool shouldDuplicate[4] = { false, false, false, false }; // keeps track of duplicate vectors (e.g. .xxyz)
			for (int8_t d = 0; d < result->dimensions; d++)
			{
				int8_t component = 0;
				if (identifier[d] == 'x') { component = 0; }
				if (identifier[d] == 'y') { component = 1; }
				if (identifier[d] == 'z') { component = 2; }
				if (identifier[d] == 'w') { component = 3; }
				// return error if trying to access component greater than number of dimensions
				if (component >= value.dimensions) { return (RuntimeError){ RuntimeErrorInvalidSwizzling, expression->operonStart[1], expression->operonEnd[1] }; }
				
				// simply move the pointer if it's not a duplicate component
				if (!shouldDuplicate[component]) { result->xyzw[d] = value.xyzw[component]; shouldDuplicate[component] = true; }
				else
				{
					// otherwise copy the contents
					result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
					memcpy(result->xyzw[d], value.xyzw[component], result->length * sizeof(scalar_t));
				}
			}
			// free any components that were unused
			for (int8_t d = 0; d < value.dimensions; d++)
			{
				if (!shouldDuplicate[d]) { free(value.xyzw[d]); }
			}
			return (RuntimeError){ RuntimeErrorNone };
		}
	}
	
	// evaluate the indexed and indexing operons
	VectorArray value = { 0 }, index = { 0 };
	RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression, 0, &value);
	if (error.code != RuntimeErrorNone) { return error; }
	error = EvaluateOperon(identifiers, cache, parameters, expression, 1, &index);
	if (error.code != RuntimeErrorNone) { return error; }
	
	// return error if indexing with a vector
	if (index.dimensions > 1) { return (RuntimeError){ RuntimeErrorIndexingWithVector, expression->operonStart[1], expression->operonEnd[1] }; }
	// return error if length of new array is too large
	if (index.length > MAX_ARRAY_LENGTH) { return (RuntimeError){ RuntimeErrorArrayTooLarge, expression->operonStart[1], expression->operonEnd[1] }; }
	
	// go through each indexing element and copy its corresponding indexed element to the final result
	result->dimensions = value.dimensions;
	result->length = index.length;
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0; i < result->length; i++)
		{
			int32_t j = roundf(index.xyzw[0][i]);
			// if range is out of bounds then use NAN
			if (j < 0 || j >= value.length) { result->xyzw[d][i] = NAN; }
			else { result->xyzw[d][i] = value.xyzw[d][j]; }
		}
		free(value.xyzw[d]);
	}
	free(index.xyzw[0]);
	
	return (RuntimeError){ RuntimeErrorNone };
}

RuntimeError EvaluateExpression(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, VectorArray * result)
{
	if (expression->operator == OperatorNone) { return EvaluateOperon(identifiers, cache, parameters, expression, 0, result); }
	if (expression->operator == OperatorFunctionCall) { return EvaluateFunctionCall(identifiers, cache, parameters, expression, result); }
	if (expression->operator == OperatorFor) { return EvaluateFor(identifiers, cache, parameters, expression, result); }
	if (expression->operator == OperatorIndex) { return EvaluateIndex(identifiers, cache, parameters, expression, result); }
	
	VectorArray value = { 0 };
	if (expression->operator == OperatorPositive || expression->operator == OperatorNegative)
	{
		// evaluate only the first operon
		RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression, 0, result);
		if (error.code != RuntimeErrorNone) { return error; }
	}
	else
	{
		// evaluate both operons (evaluate the left one into result)
		RuntimeError error = EvaluateOperon(identifiers, cache, parameters, expression, 0, result);
		if (error.code != RuntimeErrorNone) { return error; }
		error = EvaluateOperon(identifiers, cache, parameters, expression, 1, &value);
		if (error.code != RuntimeErrorNone) { return error; }
	}
	
	if (expression->operator == OperatorEllipsis)
	{
		// return error if trying to use ellipsis with array or vector
		if (result->dimensions > 1 || result->length > 1) { return (RuntimeError){ RuntimeErrorInvalidEllipsisOperon, expression->operonStart[0], expression->operonEnd[0] }; }
		if (value.dimensions > 1 || value.length > 1) { return (RuntimeError){ RuntimeErrorInvalidEllipsisOperon, expression->operonStart[1], expression->operonEnd[1] }; }
		VectorArray lower = *result;
		RuntimeErrorCode error = EvaluateEllipsis(&lower, &value, result);
		free(lower.xyzw[0]);
		if (error != RuntimeErrorNone) { return (RuntimeError){ error, expression->operonStart[0], expression->operonEnd[1] }; }
	}
	else if (expression->operator == OperatorNegative) { EvaluateNegate(result); }
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
	if (value.length > 0) { for (int32_t i = 0; i < value.dimensions; i++) { free(value.xyzw[i]); } }
	return (RuntimeError){ RuntimeErrorNone };
}