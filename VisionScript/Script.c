#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Script.h"

Script * LoadScript(const char * code)
{
	Script * script = malloc(sizeof(Script));
	script->code = code;
	
	String codeStr = StringCreate(code);
	script->tokenStatements = Tokenize(codeStr);
	StringDestroy(codeStr);
	
	script->identifierList = ListCreate(sizeof(Statement *), 65536);
	script->renderList = ListCreate(sizeof(Statement *), 1024);
	script->errorList = ListCreate(sizeof(Statement *), 1024);
	script->identifiers = HashMapCreate(65536);
	
	for (int32_t i = 0; i < ListLength(script->tokenStatements); i++)
	{
		Statement * statement = ParseTokenStatement(script->tokenStatements[i]);
		if (statement->error != SyntaxErrorNone || statement->type == StatementTypeUnknown)
		{
			ListPush((void **)&script->errorList, &statement);
			continue;
		}
		switch (statement->type)
		{
			case StatementTypeVariable:
				ListPush((void **)&script->identifierList, &statement);
				HashMapSet(script->identifiers, statement->declaration.variable.identifier, statement);
				break;
			case StatementTypeFunction:
				ListPush((void **)&script->identifierList, &statement);
				HashMapSet(script->identifiers, statement->declaration.function.identifier, statement);
				break;
			case StatementTypeRender: ListPush((void **)&script->renderList, &statement); break;
			default: break;
		}
	}
	
	return script;
}

static RuntimeError EvaluateOperon(HashMap identifiers, Statement * statement, list(VectorArray) arguments, OperonType operonType, Operon operon, VectorArray * result)
{
	if (operonType == OperonTypeExpression)
	{
		return EvaluateExpression(identifiers, statement, arguments, operon.expression, result);
	}
	else if (operonType == OperonTypeIdentifier)
	{
		if (statement != NULL && statement->type == StatementTypeFunction)
		{
			for (int32_t i = 0; i < ListLength(statement->declaration.function.arguments); i++)
			{
				if (strcmp(operon.identifier, statement->declaration.function.arguments[i]) == 0) { *result = arguments[i]; return RuntimeErrorNone; }
			}
		}
		Statement * idenStatement = HashMapGet(identifiers, operon.identifier);
		if (idenStatement == NULL) { return RuntimeErrorUndefinedIdentifier; }
		if (idenStatement->type == StatementTypeFunction) { return RuntimeErrorIdentifierNotVariable; }
		return EvaluateExpression(identifiers, idenStatement, NULL, idenStatement->expression, result);
	}
	else if (operonType == OperonTypeConstant)
	{
		result->dimensions = 1;
		result->length = 1;
		result->xyzw[0] = malloc(sizeof(float));
		result->xyzw[0][0] = operon.constant;
	}
	else if (operonType == OperonTypeVectorLiteral)
	{
		result->dimensions = ListLength(operon.expressions);
		result->length = -1;
		
		VectorArray components[4];
		for (int32_t i = 0; i < result->dimensions; i++)
		{
			RuntimeError error = EvaluateExpression(identifiers, statement, arguments, operon.expressions[i], components + i);
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
				result->xyzw[i] = malloc(sizeof(float) * result->length);
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
			RuntimeError error = EvaluateExpression(identifiers, statement, arguments, operon.expressions[i], elements + i);
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
				result->xyzw[i] = malloc(sizeof(float) * result->length);
				for (int32_t j = 0, p = 0; j < ListLength(operon.expressions); j++)
				{
					memcpy(result->xyzw[i] + p, elements[j].xyzw[i], elements[j].length * sizeof(float));
					p += elements[j].length;
					free(elements[j].xyzw[i]);
				}
			}
		}
		
		free(elements);
	}
	return RuntimeErrorNone;
}

void EvaluateEllipsis(VectorArray * a, VectorArray * b, VectorArray * result)
{
	int32_t lower = floorf(a->xyzw[0][0] + 0.5);
	int32_t upper = floorf(b->xyzw[0][0] + 0.5);
	if (upper - lower > 1000000 || upper - lower < 0) { *result = (VectorArray){ 0 }; return; }
	result->length = upper - lower + 1;
	result->dimensions = 1;
	result->xyzw[0] = malloc(result->length * sizeof(float));
	for (int32_t i = 0; i < result->length; i++) { result->xyzw[0][i] = i + lower; }
}

static inline float Operation(Operator operator, float a, float b)
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

void EvaluateOperation(Operator operator, VectorArray * a, VectorArray * b)
{
	int8_t dimensions = a->dimensions > b->dimensions ? a->dimensions : b->dimensions;
	int32_t length = a->length < b->length ? a->length : b->length;
	bool aScalar = false, bScalar = false;
	if (a->length == 1 && b->length > 1) { length = b->length; aScalar = true; }
	if (b->length == 1 && a->length > 1) { length = a->length; bScalar = true; }
	
	if (a->dimensions == 1 && b->dimensions > 1)
	{
		for (int8_t d = 1; d < b->dimensions; d++)
		{
			a->xyzw[d] = malloc(length * sizeof(float));
			memcpy(a->xyzw[d], a->xyzw[0], length * sizeof(float));
		}
		a->dimensions = b->dimensions;
	}
	if (b->dimensions == 1 && a->dimensions > 1)
	{
		for (int8_t d = 1; d < a->dimensions; d++)
		{
			b->xyzw[d] = malloc(length * sizeof(float));
			memcpy(b->xyzw[d], b->xyzw[0], length * sizeof(float));
		}
		b->dimensions = a->dimensions;
	}

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

void EvaluateNegate(VectorArray * a)
{
	for (int8_t d = 0; d < a->dimensions; d++)
	{
		for (int32_t i = 0; i < a->length; i++)
		{
			a->xyzw[d][i] = -a->xyzw[d][i];
		}
	}
}

static RuntimeError EvaluateFunctionCall(HashMap identifiers, Statement * statement, list(VectorArray) arguments, String function, list(Expression *) expressions, VectorArray * result)
{
	Statement * idenStatement = HashMapGet(identifiers, function);
	if (idenStatement == NULL) { return RuntimeErrorUndefinedIdentifier; }
	if (idenStatement->type == StatementTypeVariable) { return RuntimeErrorIdentifierNotFunction; }
	
	list(VectorArray) args = ListCreate(sizeof(VectorArray), ListLength(expressions));
	for (int32_t i = 0; i < ListLength(expressions); i++)
	{
		ListPush((void **)args, &(VectorArray){ 0 });
		RuntimeError error = EvaluateExpression(identifiers, statement, arguments, expressions[i], args + i);
		if (error != RuntimeErrorNone) { return error; }
	}
	RuntimeError error = EvaluateExpression(identifiers, idenStatement, args, idenStatement->expression, result);
	ListDestroy(args);
	return error;
}

RuntimeError EvaluateExpression(HashMap identifiers, Statement * statement, list(VectorArray) arguments, Expression * expression, VectorArray * result)
{
	if (expression->operator == OperatorNone) { return EvaluateOperon(identifiers, statement, arguments, expression->operonTypes[0], expression->operons[0], result); }
	if (expression->operator == OperatorFunctionCall) { return EvaluateFunctionCall(identifiers, statement, arguments, expression->operons[0].identifier, expression->operons[1].expressions, result); }
	if (expression->operator == OperatorFor) { return RuntimeErrorNotImplemented; }
	if (expression->operator == OperatorIndex) { return RuntimeErrorNotImplemented; }
	
	VectorArray value = { 0 };
	if (expression->operator == OperatorPositive || expression->operator == OperatorNegative)
	{
		RuntimeError error = EvaluateOperon(identifiers, statement, arguments, expression->operonTypes[0], expression->operons[0], result);
		if (error != RuntimeErrorNone) { return error; }
	}
	else
	{
		RuntimeError error = EvaluateOperon(identifiers, statement, arguments, expression->operonTypes[0], expression->operons[0], result);
		if (error != RuntimeErrorNone) { return error; }
		error = EvaluateOperon(identifiers, statement, arguments, expression->operonTypes[1], expression->operons[1], &value);
		if (error != RuntimeErrorNone) { return error; }
	}
	
	if (expression->operator == OperatorFor)
	{
		
	}
	else if (expression->operator == OperatorEllipsis)
	{
		if (result->dimensions > 1 || result->length > 1 || value.dimensions > 1 || value.length > 1) { return RuntimeErrorInvalidEllipsisOperon; }
		VectorArray lower = *result;
		EvaluateEllipsis(&lower, &value, result);
		free(lower.xyzw[0]);
	}
	else if (expression->operator == OperatorNegative) { EvaluateNegate(result); }
	else if (expression->operator == OperatorIndex)
	{
		if (value.dimensions > 1) { return RuntimeErrorIndexingWithVector; }
	}
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

void DestroyScript(Script * script)
{
	for (int32_t i = 0; i < ListLength(script->errorList); i++) { DestroyStatement(script->errorList[i]); }
	for (int32_t i = 0; i < ListLength(script->renderList); i++) { DestroyStatement(script->renderList[i]); }
	for (int32_t i = 0; i < ListLength(script->identifierList); i++) { DestroyStatement(script->identifierList[i]); }
	ListDestroy(script->errorList);
	ListDestroy(script->renderList);
	ListDestroy(script->identifierList);
	HashMapDestroy(script->identifiers);
}
