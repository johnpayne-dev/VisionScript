#include <stdlib.h>
#include <stdio.h>
#include "Evaluator.h"
#include "Script.h"

static list(Statement *) FindParents(HashMap identifiers, Expression * expression, list(String) parameters)
{
	list(Statement *) parents = ListCreate(sizeof(Statement *), 1);
	
	// if it's a for operator, add the assignment to the parameters list
	if (expression->operator == OperatorFor)
	{
		// create parameters list if doesn't exist otherwise create a copy
		if (parameters == NULL) { parameters = ListCreate(sizeof(String), 1); }
		else { parameters = ListClone(parameters); }
		ListPush((void **)&parameters, &expression->operons[1].assignment.identifier);
	}
	
	// go through one or both operons
	int32_t operons = expression->operator == OperatorFor || expression->operator == OperatorNone || expression->operator == OperatorNegative || expression->operator == OperatorPositive ? 1 : 2;
	for (int32_t i = 0; i < operons; i++)
	{
		if (expression->operonTypes[i] == OperonTypeArrayLiteral || expression->operonTypes[i] == OperonTypeVectorLiteral || expression->operonTypes[i] == OperonTypeArguments)
		{
			// recursively go through each expression and add its parents to the list
			for (int32_t j = 0; j < ListLength(expression->operons[i].expressions); j++)
			{
				list(Statement *) argDependents = FindParents(identifiers, expression->operons[i].expressions[j], parameters);
				for (int32_t k = 0; k < ListLength(argDependents); k++) { ListPush((void **)&parents, &argDependents[k]); }
				ListFree(argDependents);
			}
		}
		else if (expression->operonTypes[i] == OperonTypeExpression)
		{
			// recursively go through this expression and find its parents
			list(Statement *) expDependents = FindParents(identifiers, expression->operons[i].expression, parameters);
			for (int32_t k = 0; k < ListLength(expDependents); k++) { ListPush((void **)&parents, &expDependents[k]); }
			ListFree(expDependents);
		}
		else if (expression->operonTypes[i] == OperonTypeIdentifier)
		{
			// skip identifier if it's a parameter from a for assignment
			if (parameters != NULL)
			{
				bool isParameter = false;
				for (int32_t j = 0; j < ListLength(parameters); j++) { if (StringEquals(parameters[j], expression->operons[i].identifier) == 0) { isParameter = true; break; } }
				if (isParameter) { continue; }
			}
			// otherwise add the identifier's statement to the list if it's not null, not a function, and not already in the list
			Statement * statement = HashMapGet(identifiers, expression->operons[i].identifier);
			if (statement != NULL && statement->type != StatementTypeFunction && !ListContains(parents, statement)) { ListPush((void **)&parents, &statement); }
		}
	}
	// free list that was cloned in for assignment
	if (expression->operator == OperatorFor) { ListFree(parameters); }
	
	return parents;
}

static RuntimeError InitializeCache(HashMap identifiers, HashMap cache, Statement * statement)
{
	// if there's a value already cached then return
	VectorArray * value = HashMapGet(cache, statement->declaration.variable.identifier);
	if (value != NULL) { return (RuntimeError){ RuntimeErrorNone }; }
	
	// initialize cache of parents first
	list(Statement *) parents = FindParents(identifiers, statement->expression, NULL);
	for (int32_t i = 0; i < ListLength(parents); i++)
	{
		RuntimeError error = InitializeCache(identifiers, cache, parents[i]);
		if (error.code != RuntimeErrorNone) { return error; }
	}
	
	value = malloc(sizeof(VectorArray));
	RuntimeError error = EvaluateExpression(identifiers, cache, NULL, statement->expression, value);
	if (error.code == RuntimeErrorNone) { HashMapSet(cache, statement->declaration.variable.identifier, value); }
	return error;
}

Script * LoadScript(const char * code, int32_t varLimit)
{
	Script * script = malloc(sizeof(Script));
	script->code = code;
	
	String codeStr = StringCreate(code);
	script->tokenLines = TokenizeCode(codeStr);
	StringFree(codeStr);
	
	script->identifierList = ListCreate(sizeof(Statement *), varLimit);
	script->renderList = ListCreate(sizeof(Statement *), 1024);
	script->errorList = ListCreate(sizeof(Statement *), 1024);
	script->identifiers = HashMapCreate(varLimit);
	script->cache = HashMapCreate(varLimit);
	script->dependents = HashMapCreate(varLimit);
	
	// go through each statement, parse it and add it to the internal data structures
	for (int32_t i = 0; i < ListLength(script->tokenLines); i++)
	{
		Statement * statement = ParseTokenLine(script->tokenLines[i]);
		if (statement->error.code != SyntaxErrorNone || statement->type == StatementTypeUnknown)
		{
			ListPush((void **)&script->errorList, &statement);
			continue;
		}
		switch (statement->type)
		{
			case StatementTypeVariable:
				ListPush((void **)&script->identifierList, &statement);
				HashMapSet(script->identifiers, statement->declaration.variable.identifier, statement);
				// allocate dependents list
				list(Statement *) * dependents = malloc(sizeof(list(Statement *)));
				*dependents = ListCreate(sizeof(Statement *), 1);
				HashMapSet(script->dependents, statement->declaration.variable.identifier, dependents);
				break;
			case StatementTypeFunction:
				ListPush((void **)&script->identifierList, &statement);
				HashMapSet(script->identifiers, statement->declaration.function.identifier, statement);
				break;
			case StatementTypeRender: ListPush((void **)&script->renderList, &statement); break;
			default: break;
		}
	}
	
	// go through each statement again and set dependents and initialize cache
	for (int32_t i = 0; i < ListLength(script->identifierList); i++)
	{
		if (script->identifierList[i]->type == StatementTypeVariable)
		{
			list(Statement *) parents = FindParents(script->identifiers, script->identifierList[i]->expression, NULL);
			for (int32_t j = 0; j < ListLength(parents); j++)
			{
				ListPush(HashMapGet(script->dependents, parents[j]->declaration.variable.identifier), script->identifierList[i]);
			}
			ListFree(parents);
			RuntimeError error = InitializeCache(script->identifiers, script->cache, script->identifierList[i]);
			
			// todo: handle runtime errors properly
			if (error.code != RuntimeErrorNone) { abort(); }
			else
			{
				printf("%s: ", script->identifierList[i]->declaration.variable.identifier);
				PrintVectorArray(*(VectorArray *)HashMapGet(script->cache, script->identifierList[i]->declaration.variable.identifier));
				printf("\n");
			}
		}
	}
	
	return script;
}



void FreeScript(Script * script)
{
	for (int32_t i = 0; i < ListLength(script->errorList); i++) { FreeStatement(script->errorList[i]); }
	for (int32_t i = 0; i < ListLength(script->renderList); i++) { FreeStatement(script->renderList[i]); }
	for (int32_t i = 0; i < ListLength(script->identifierList); i++) { FreeStatement(script->identifierList[i]); }
	ListFree(script->errorList);
	ListFree(script->renderList);
	ListFree(script->identifierList);
	HashMapFree(script->identifiers);
}
