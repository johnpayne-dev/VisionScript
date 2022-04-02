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
	list(list(String)) renderParents;
	list(Statement *) errorList;
	HashMap identifiers;
	HashMap cache;
	HashMap dependents;
	list(Statement *) dirtyRenders;
} Script;

Script * LoadScript(const char * code, int32_t varLimit);

void InvalidateCachedDependents(Script * script, String identifier);

void InvalidateDependentRenders(Script * script, String identifier);

void FreeScript(Script * script);

#endif
