#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Evaluator.h"
#include "Script.h"

static int CompareStatementIdentifier(const void * a, const void * b)
{
	return strcmp((*(Statement **)a)->declaration.variable.identifier, (*(Statement **)b)->declaration.variable.identifier);
}

static list(Statement *) FindParents(HashMap identifiers, Expression * expression, list(String) parameters)
{
	list(Statement *) parents = ListCreate(sizeof(Statement *), 1);
	
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
				list(Statement *) argDependents = FindParents(identifiers, expression->operons[i].expressions[j], parameters);
				for (int32_t k = 0; k < ListLength(argDependents); k++) { parents = ListPush(parents, &argDependents[k]); }
				ListFree(argDependents);
			}
		}
		else if (expression->operonTypes[i] == OperonTypeExpression)
		{
			// recursively go through this expression and find its parents
			list(Statement *) expDependents = FindParents(identifiers, expression->operons[i].expression, parameters);
			for (int32_t k = 0; k < ListLength(expDependents); k++) { parents = ListPush(parents, &expDependents[k]); }
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
			
			Statement * statement = HashMapGet(identifiers, expression->operons[i].identifier);
			if (statement != NULL)
			{
				// add the identifier's statement to the list if it's a variable and not already in the list
				if (statement->type == StatementTypeVariable) { parents = ListPush(parents, &statement); }
				// if it's a function, look for parents in there
				if (statement->type == StatementTypeFunction)
				{
					for (int32_t j = 0; j < ListLength(statement->declaration.function.arguments); j++) { parameters = ListPush(parameters, &statement->declaration.function.arguments[j]); }
					
					list(Statement *) funcParents = FindParents(identifiers, statement->expression, NULL);
					for (int32_t j = 0; j < ListLength(funcParents); j++) { parents = ListPush(parents, &funcParents[j]); }
					ListFree(funcParents);
				}
			}
		}
	}
	
	// free list that was cloned
	ListFree(parameters);
	
	// remove duplicates from list
	qsort(parents, ListLength(parents), ListElementSize(parents), CompareStatementIdentifier);
	for (int32_t i = 0; i < ListLength(parents); i++)
	{
		if (i > 0 && strcmp(parents[i]->declaration.variable.identifier, parents[i - 1]->declaration.variable.identifier) == 0)
		{
			parents = ListRemove(parents, i);
			i--;
		}
	}
	
	return parents;
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
			script->errorList = ListPush(script->errorList, &statement);
			continue;
		}
		switch (statement->type)
		{
			case StatementTypeVariable:
				script->identifierList = ListPush(script->identifierList, &statement);
				HashMapSet(script->identifiers, statement->declaration.variable.identifier, statement);
				list(Statement *) dependents = ListCreate(sizeof(Statement *), 1);
				HashMapSet(script->dependents, statement->declaration.variable.identifier, dependents);
				break;
			case StatementTypeFunction:
				script->identifierList = ListPush(script->identifierList, &statement);
				HashMapSet(script->identifiers, statement->declaration.function.identifier, statement);
				break;
			case StatementTypeRender: script->renderList = ListPush(script->renderList, &statement); break;
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
				list(Statement *) dependents = HashMapGet(script->dependents, parents[j]->declaration.variable.identifier);
				dependents = ListPush(dependents, &script->identifierList[i]);
				HashMapSet(script->dependents, parents[j]->declaration.variable.identifier, dependents);
			}
			ListFree(parents);
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
