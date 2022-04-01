#ifndef Script_h
#define Script_h

#include "Tokenizer.h"
#include "Parser.h"
#include "Utilities/HashMap.h"

typedef struct Script
{
	const char * code;
	list(list(Token)) tokenLines;
	list(Statement *) identifierList;
	list(Statement *) renderList;
	list(Statement *) errorList;
	HashMap identifiers;
	HashMap cache;
	HashMap dependents;
} Script;

Script * LoadScript(const char * code, int32_t varLimit);

void UpdateScript(Script * script);

void FreeScript(Script * script);

#endif
