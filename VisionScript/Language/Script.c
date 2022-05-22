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
		SetEnvironmentEquation(&script.environment, equation);
		AddToScriptRenderList(&script, equation);
	}
	InitializeEnvironmentDependents(&script.environment);
	return script;
}

void AddToScriptRenderList(Script * script, Equation equation) {
	if (equation.declaration.attribute != DeclarationAttributeNone) {
		Equation * envEquation = GetEnvironmentEquation(&script->environment, equation.declaration.identifier);
		if (envEquation != NULL && !ListContains(script->needsRender, &envEquation)) { script->needsRender = ListPush(script->needsRender, &envEquation); }
	}
}

void RemoveFromRenderList(Script * script, Equation equation) {
	Equation * envEquation = GetEnvironmentEquation(&script->environment, equation.declaration.identifier);
	script->needsRender = ListRemoveAll(script->needsRender, &envEquation);
}

void FreeScript(Script script) {
	for (int32_t i = 0; i < ListLength(script.lines); i++) { StringFree(script.lines[i]); }
	ListFree(script.lines);
	ListFree(script.needsRender);
	FreeEnvironment(script.environment);
}
