#ifndef Script_h
#define Script_h

#include "Tokenizer.h"
#include "Parser.h"
#include "Evaluator.h"

typedef struct Script {
	list(String) lines;
	Environment environment;
	list(Equation *) needsRender;
} Script;

Script LoadScript(const char * code);

void AddToScriptRenderList(Script * script, Equation equation);

void RemoveFromRenderList(Script * script, Equation equation);

void InvalidateDependents(String * script, String identifer);

void FreeScript(Script script);

#endif
