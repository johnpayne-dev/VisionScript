#ifndef Script_h
#define Script_h

#include "Tokenizer.h"
#include "Parser.h"
#include "Evaluator.h"

typedef struct Script {
	List(String) lines;
	Environment environment;
	List(Equation) needsRender;
} Script;

Script LoadScript(const char * code);

void AddToScriptRenderList(Script * script, Equation equation);

void RemoveFromRenderList(Script * script, Equation equation);

void InvalidateDependents(Script * script, const char * identifer);

void InvalidateParametrics(Script * script);

void FreeScript(Script script);

#endif
