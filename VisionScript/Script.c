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

static RuntimeError EvaluateOperon(Script * script, Statement * statement, list(VectorArray) arguments, OperonType operonType, Operon operon, VectorArray * result)
{
	if (operonType == OperonTypeExpression)
	{
		return EvaluateExpression(script, statement, arguments, operon.expression, result);
	}
	else if (operonType == OperonTypeIdentifier)
	{
		if (statement->type == StatementTypeFunction)
		{
			for (int32_t i = 0; i < ListLength(statement->declaration.function.arguments); i++)
			{
				if (strcmp(operon.identifier, statement->declaration.function.arguments[i]) == 0) { *result = arguments[i]; return RuntimeErrorNone; }
			}
		}
		Statement * idenStatement = HashMapGet(script->identifiers, operon.identifier);
		if (idenStatement == NULL) { return RuntimeErrorUndefinedIdentifier; }
		if (idenStatement->type == StatementTypeFunction) { return RuntimeErrorIdentifierNotVariable; }
		return EvaluateExpression(script, idenStatement, NULL, idenStatement->expression, result);
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
		
		for (int32_t i = 0; i < result->dimensions; i++)
		{
			VectorArray component;
			RuntimeError error = EvaluateExpression(script, statement, arguments, operon.expressions[i], &component);
			if (error != RuntimeErrorNone) { return error; }
			
			if (component.dimensions > 1) { return RuntimeErrorVectorInsideVector; }
			
			result->length = component.length < result->length ? component.length : result->length;
			result->xyzw[i] = component.xyzw[0];
		}
	}
	else if (operonType == OperonTypeArrayLiteral)
	{
		result->dimensions = 0;
		result->length = 0;
		
		VectorArray * elements = malloc(sizeof(VectorArray) * ListLength(operon.expressions));
		for (int32_t i = 0; i < ListLength(operon.expressions); i++)
		{
			RuntimeError error = EvaluateExpression(script, statement, arguments, operon.expressions[i], elements + i);
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
					memcpy(result->xyzw[i] + p, elements[j].xyzw[i], elements[j].length);
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
	for (int32_t i = lower; i <= upper; i++) { result->xyzw[0][i] = i; }
}

void EvaluateAdd(VectorArray * a, VectorArray * b)
{
	for (int8_t d = 0; d < a->dimensions; d++)
	{
		for (int32_t i = 0; i < a->length < b->length ? a->length : b->length; i++)
		{
			a->xyzw[d][i] += b->xyzw[d][i];
		}
	}
}

void EvaluateSubtract(VectorArray * a, VectorArray * b)
{
	for (int8_t d = 0; d < a->dimensions; d++)
	{
		for (int32_t i = 0; i < a->length < b->length ? a->length : b->length; i++)
		{
			a->xyzw[d][i] -= b->xyzw[d][i];
		}
	}
}

void EvaluateMultiply(VectorArray * a, VectorArray * b)
{
	for (int8_t d = 0; d < a->dimensions; d++)
	{
		for (int32_t i = 0; i < a->length < b->length ? a->length : b->length; i++)
		{
			a->xyzw[d][i] *= b->xyzw[d][i];
		}
	}
}

void EvaluateDivide(VectorArray * a, VectorArray * b)
{
	for (int8_t d = 0; d < a->dimensions; d++)
	{
		for (int32_t i = 0; i < a->length < b->length ? a->length : b->length; i++)
		{
			a->xyzw[d][i] /= b->xyzw[d][i];
		}
	}
}

void EvaluateModulo(VectorArray * a, VectorArray * b)
{
	for (int8_t d = 0; d < a->dimensions; d++)
	{
		for (int32_t i = 0; i < a->length < b->length ? a->length : b->length; i++)
		{
			a->xyzw[d][i] = fmodf(a->xyzw[d][i], b->xyzw[d][i]);
		}
	}
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

void EvaluatePower(VectorArray * a, VectorArray * b)
{
	for (int8_t d = 0; d < a->dimensions; d++)
	{
		for (int32_t i = 0; i < a->length < b->length ? a->length : b->length; i++)
		{
			a->xyzw[d][i] = powf(a->xyzw[d][i], b->xyzw[d][i]);
		}
	}
}

static RuntimeError EvaluateFunctionCall(Script * script, Statement * statement, list(VectorArray) arguments, String function, list(Expression *) expressions, VectorArray * result)
{
	Statement * idenStatement = HashMapGet(script->identifiers, function);
	if (idenStatement == NULL) { return RuntimeErrorUndefinedIdentifier; }
	if (idenStatement->type == StatementTypeVariable) { return RuntimeErrorIdentifierNotFunction; }
	
	list(VectorArray) args = ListCreate(sizeof(VectorArray), ListLength(expressions));
	for (int32_t i = 0; i < ListLength(expressions); i++)
	{
		ListPush((void **)args, &(VectorArray){ 0 });
		RuntimeError error = EvaluateExpression(script, statement, arguments, expressions[i], args + i);
		if (error != RuntimeErrorNone) { return error; }
	}
	RuntimeError error = EvaluateExpression(script, idenStatement, args, idenStatement->expression, result);
	ListDestroy(args);
	return error;
}

RuntimeError EvaluateExpression(Script * script, Statement * statement, list(VectorArray) arguments, Expression * expression, VectorArray * result)
{
	if (expression->operator == OperatorNone) { return EvaluateOperon(script, statement, arguments, expression->operonTypes[0], expression->operons[0], result); }
	if (expression->operator == OperatorFunctionCall) { return EvaluateFunctionCall(script, statement, arguments, expression->operons[0].identifier, expression->operons[1].expressions, result); }
	if (expression->operator == OperatorFor) { return RuntimeErrorNotImplemented; }
	if (expression->operator == OperatorIndex) { return RuntimeErrorNotImplemented; }
	
	VectorArray value;
	if (expression->operator == OperatorPositive || expression->operator == OperatorNegative)
	{
		RuntimeError error = EvaluateOperon(script, statement, arguments, expression->operonTypes[0], expression->operons[0], result);
		if (error != RuntimeErrorNone) { return error; }
	}
	else
	{
		RuntimeError error = EvaluateOperon(script, statement, arguments, expression->operonTypes[0], expression->operons[0], result);
		if (error != RuntimeErrorNone) { return error; }
		error = EvaluateOperon(script, statement, arguments, expression->operonTypes[1], expression->operons[1], &value);
		if (error != RuntimeErrorNone) { return error; }
	}
	
	if (expression->operator == OperatorFor)
	{
		
	}
	else if (expression->operator == OperatorEllipsis)
	{
		if (result->dimensions > 1 || result->length > 1 || value.dimensions > 1 || value.length > 1) { return RuntimeErrorInvalidEllipsisOperon; }
		VectorArray lower;
		memcpy(&lower, result, sizeof(VectorArray));
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
		if (result->dimensions != value.dimensions) { return RuntimeErrorDifferingLengthVectors; }
		switch (expression->operator)
		{
			case OperatorAdd: EvaluateAdd(result, &value); break;
			case OperatorSubtract: EvaluateSubtract(result, &value); break;
			case OperatorMultiply: EvaluateMultiply(result, &value); break;
			case OperatorDivide: EvaluateDivide(result, &value); break;
			case OperatorModulo: EvaluateModulo(result, &value); break;
			case OperatorPower: EvaluatePower(result, &value); break;
			default: break;
		}
	}
	for (int32_t i = 0; i < value.dimensions; i++) { free(value.xyzw[i]); }
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
