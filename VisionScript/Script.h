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
} Script;

Script * LoadScript(const char * code);

void DestroyScript(Script * script);

#endif
