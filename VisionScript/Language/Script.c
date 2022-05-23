#include "Script.h"

Script LoadScript(const char * code) {
	Script script = {
		.lines = SplitCodeIntoLines((char *)code),
		.environment = CreateEmptyEnvironment(),
		.needsRender = ListCreate(sizeof(Equation), 1),
	};
	for (int32_t i = 0; i < ListLength(script.lines); i++) {
		List(Token) tokens = TokenizeLine(script.lines[i], i);
		Equation equation;
		SyntaxError error = ParseEquation(tokens, &equation);
		if (error.code != SyntaxErrorCodeNone) {
			PrintSyntaxError(error, script.lines);
			continue;
		}
		if (equation.type == EquationTypeNone) { continue; }
		AddEnvironmentEquation(&script.environment, equation);
		AddToScriptRenderList(&script, equation);
	}
	InitializeEnvironmentDependents(&script.environment);
	return script;
}

void AddToScriptRenderList(Script * script, Equation equation) {
	if (equation.type != EquationTypeNone && equation.declaration.attribute != DeclarationAttributeNone) {
		if (!ListContains(script->needsRender, &equation)) { script->needsRender = ListPush(script->needsRender, &equation); }
	}
}

void RemoveFromRenderList(Script * script, Equation equation) {
	script->needsRender = ListRemoveAll(script->needsRender, &equation);
}

void InvalidateDependents(Script * script, const char * identifer) {
	List(String) * dependents = HashMapGet(script->environment.dependents, identifer);
	for (int32_t i = 0; i < ListLength(*dependents); i++) {
		Equation * dependent = HashMapGet(script->environment.equations, (*dependents)[i]);
		if (dependent->type == EquationTypeVariable) {
			VectorArray * cache = HashMapGet(script->environment.cache, (*dependents)[i]);
			if (cache != NULL) { FreeVectorArray(*cache); }
			HashMapSet(script->environment.cache, (*dependents)[i], NULL);
		}
		if (dependent->declaration.attribute != DeclarationAttributeNone) { AddToScriptRenderList(script, *dependent); }
	}
}

void FreeScript(Script script) {
	for (int32_t i = 0; i < ListLength(script.lines); i++) { StringFree(script.lines[i]); }
	ListFree(script.lines);
	ListFree(script.needsRender);
	FreeEnvironment(script.environment);
}
