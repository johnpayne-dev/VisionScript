#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Evaluator.h"
#include "Script.h"
#include "Builtin.h"

#define VARIABLE_LIMIT 65536

Script * LoadScript(const char * code)
{
	Script * script = malloc(sizeof(Script));
	script->code = code;
	
	String codeStr = StringCreate(code);
	script->tokenLines = TokenizeCode(codeStr);
	StringFree(codeStr);
	
	script->identifierList = ListCreate(sizeof(Statement *), VARIABLE_LIMIT);
	script->renderList = ListCreate(sizeof(Statement *), 1024);
	script->renderParents = ListCreate(sizeof(list(String)), 1);
	script->errorList = ListCreate(sizeof(Statement *), 1024);
	script->identifiers = HashMapCreate(VARIABLE_LIMIT);
	script->cache = HashMapCreate(VARIABLE_LIMIT);
	script->dependents = HashMapCreate(VARIABLE_LIMIT);
	script->dirtyRenders = ListCreate(sizeof(Statement *), 1);
	
	// go through each statement, parse it and add it to the internal data structures
	InitializeBuiltins(script->cache);
	for (int32_t i = 0; i < ListLength(script->tokenLines); i++)
	{
		Statement * statement = ParseTokenLine(script->tokenLines[i]);
		if (statement->error.code != SyntaxErrorNone || statement->type == StatementTypeUnknown)
		{
			script->errorList = ListPush(script->errorList, &statement);
			PrintSyntaxError(statement);
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
	
	// go through each render statement, set its parents and add it to dirty list
	for (int32_t i = 0; i < ListLength(script->renderList); i++)
	{
		list(String) parameters = ListCreate(sizeof(String), 1);
		if (script->renderList[i]->declaration.render.type == StatementRenderTypeParametric) { parameters = ListPush(parameters, &(String){ StringCreate("t") }); }
		
		list(String) parents = FindExpressionParents(script->identifiers, script->renderList[i]->expression, parameters);
		script->renderParents = ListPush(script->renderParents, &parents);
		
		if (script->renderList[i]->declaration.render.type == StatementRenderTypeParametric) { StringFree(parameters[0]); }
		ListFree(parameters);
		
		script->dirtyRenders = ListPush(script->dirtyRenders, &(Statement *){ script->renderList[i] });
	}

	return script;
}

void InvalidateCachedDependents(Script * script, String identifier)
{
	list(String) dependentIdentifiers = HashMapGet(script->dependents, identifier);
	if (dependentIdentifiers != NULL)
	{
		for (int32_t i = 0; i < ListLength(dependentIdentifiers); i++)
		{
			VectorArray * cached = HashMapGet(script->cache, dependentIdentifiers[i]);
			if (cached != NULL)
			{
				FreeVectorArray(*cached);
				free(cached);
			}
			HashMapSet(script->cache, dependentIdentifiers[i], NULL);
		}
	}
}

void InvalidateDependentRenders(Script * script, String identifier)
{
	for (int32_t i = 0; i < ListLength(script->renderParents); i++)
	{
		list(String) parents = script->renderParents[i];
		for (int32_t j = 0; j < ListLength(parents); j++)
		{
			if (strcmp(parents[j], identifier) == 0 && !ListContains(script->dirtyRenders, &(Statement *){ script->renderList[i] }))
			{
				script->dirtyRenders = ListPush(script->dirtyRenders, &(Statement *){ script->renderList[i] });
				break;
			}
		}
	}
}

void InvalidateParametricRenders(Script * script)
{
	for (int32_t i = 0; i < ListLength(script->renderList); i++)
	{
		if (script->renderList[i]->declaration.render.type == StatementRenderTypeParametric && !ListContains(script->dirtyRenders, &(Statement *){ script->renderList[i] }))
		{
			script->dirtyRenders = ListPush(script->dirtyRenders, &(Statement *){ script->renderList[i] });
		}
	}
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
