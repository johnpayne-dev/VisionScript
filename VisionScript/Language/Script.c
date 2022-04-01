#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Evaluator.h"
#include "Script.h"
#include "Builtin.h"

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
	InitializeBuiltins(script->cache);
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
				break;
			case StatementTypeFunction:
				script->identifierList = ListPush(script->identifierList, &statement);
				HashMapSet(script->identifiers, statement->declaration.function.identifier, statement);
				break;
			case StatementTypeRender: script->renderList = ListPush(script->renderList, &statement); break;
			default: break;
		}
	}
	
	// go through each statement again and set dependents
	for (int32_t i = 0; i < ListLength(script->identifierList); i++)
	{
		if (script->identifierList[i]->type == StatementTypeVariable)
		{
			list(String) parents = FindExpressionParents(script->identifiers, script->identifierList[i]->expression, NULL);
			for (int32_t j = 0; j < ListLength(parents); j++)
			{
				list(String) dependents = HashMapGet(script->dependents, parents[j]);
				
				if (dependents == NULL) { dependents = ListCreate(sizeof(String), 1); }
				dependents = ListPush(dependents, &script->identifierList[i]->declaration.variable.identifier);
				HashMapSet(script->dependents, parents[j], dependents);
			}
			ListFree(parents);
		}
	}
	
	return script;
}

void UpdateScript(Script * script)
{
	((VectorArray *)HashMapGet(script->cache, "time"))->xyzw[0][0]++;
	InvalidateCachedDependents(script->cache, script->dependents, "time");
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
