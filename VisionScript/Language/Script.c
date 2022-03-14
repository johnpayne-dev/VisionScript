#include <stdlib.h>
#include "Script.h"

Script * LoadScript(const char * code)
{
	Script * script = malloc(sizeof(Script));
	script->code = code;
	
	String codeStr = StringCreate(code);
	script->tokenLines = TokenizeCode(codeStr);
	StringDestroy(codeStr);
	
	script->identifierList = ListCreate(sizeof(Statement *), 65536);
	script->renderList = ListCreate(sizeof(Statement *), 1024);
	script->errorList = ListCreate(sizeof(Statement *), 1024);
	script->identifiers = HashMapCreate(65536);
	
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
