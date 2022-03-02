#include <stdlib.h>
#include "Script.h"

Script * LoadScript(const char * code)
{
	Script * script = malloc(sizeof(Script));
	script->code = code;
	
	String codeStr = StringCreate(code);
	script->tokenStatements = Tokenize(codeStr);
	StringDestroy(codeStr);
	
	script->identifiers = ListCreate(sizeof(Statement *), 65536);
	script->renders = ListCreate(sizeof(Statement *), 1024);
	script->errors = ListCreate(sizeof(Statement *), 1024);
	script->identifierValues = HashMapCreate(65536);
	
	for (int32_t i = 0; i < ListLength(script->tokenStatements); i++)
	{
		Statement * statement = ParseTokenStatement(script->tokenStatements[i]);
		if (statement->error != SyntaxErrorNone || statement->type == StatementTypeUnknown)
		{
			ListPush((void **)&script->errors, &statement);
			continue;
		}
		switch (statement->type)
		{
			case StatementTypeVariable:
				ListPush((void **)&script->identifiers, &statement);
				HashMapSet(script->identifierValues, statement->declaration.variable.identifier, statement);
				break;
			case StatementTypeFunction:
				ListPush((void **)&script->identifiers, &statement);
				HashMapSet(script->identifierValues, statement->declaration.function.identifier, statement);
				break;
			case StatementTypeRender: ListPush((void **)&script->renders, &statement); break;
			default: break;
		}
	}
	
	return script;
}

void DestroyScript(Script * script)
{
	for (int32_t i = 0; i < ListLength(script->errors); i++) { DestroyStatement(script->errors[i]); }
	for (int32_t i = 0; i < ListLength(script->renders); i++) { DestroyStatement(script->renders[i]); }
	for (int32_t i = 0; i < ListLength(script->identifiers); i++) { DestroyStatement(script->identifiers[i]); }
	ListDestroy(script->errors);
	ListDestroy(script->renders);
	ListDestroy(script->identifiers);
	HashMapDestroy(script->identifierValues);
}
