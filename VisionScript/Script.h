#ifndef Script_h
#define Script_h

#include "Tokenizer.h"
#include "Parser.h"
#include "Utilities/HashMap.h"

typedef struct Script
{
	const char * code;
	list(TokenStatement) tokenStatements;
	list(Statement *) identifiers;
	list(Statement *) renders;
	list(Statement *) errors;
	HashMap identifierValues;
} Script;

Script * LoadScript(const char * code);
void DestroyScript(Script * script);

#endif
