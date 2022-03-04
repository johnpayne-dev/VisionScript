#include <stdio.h>
#include "Script.h"

static void PrintVectorArray(VectorArray array)
{
	if (array.length > 1) { printf("["); }
	for (int32_t i = 0; i < array.length; i++)
	{
		if (array.dimensions > 1) { printf("("); }
		for (int32_t j = 0; j < array.dimensions; j++)
		{
			if (array.xyzw[j][i] - (int32_t)array.xyzw[j][i] == 0) { printf("%i", (int32_t)array.xyzw[j][i]); }
			else { printf("%f", array.xyzw[j][i]); }
			if (j != array.dimensions - 1) { printf(","); }
		}
		if (array.dimensions > 1) { printf(")"); }
		if (i != array.length - 1) { printf(", "); }
	}
	if (array.length > 1) { printf("]"); }
	printf("\n");
}

int main(int argc, const char * argv[])
{
	if (argc < 2)
	{
		printf("Invalid arguments.\n");
		return 0;
	}
	printf("%s %s\n", argv[0], argv[1]);
	
	char * code =
		"f(x,y) = (cos(x - y), sin(x + y))\n"
		"s = 2*pi\n"
		"n = 40\n"
		"A = (s/n)*[2*(i, j) - 1 for i = [0...n] for j = [0...n]]\n"
		"B = A + (s/n)*f(A.x,A.y)\n"
		"parametric ((B.x - A.x)*t + A.x, (B.y - A.y)*t + A.y)\n";
	
	Script * script = LoadScript(code);
	
	list(Token) tokens = TokenizeLine(StringCreate("2^2^(-2)"));
	Expression * expression;
	SyntaxError syntax = ParseExpression(tokens, 0, ListLength(tokens) - 1, &expression);
	if (syntax != SyntaxErrorNone) { printf("SyntaxError: %i\n", syntax); return 0; }
	VectorArray result;
	RuntimeError runtime = EvaluateExpression(script->identifiers, NULL, NULL, expression, &result);
	if (runtime != RuntimeErrorNone) { printf("RuntimeError: %i\n", runtime); return 0; }
	PrintVectorArray(result);
	
	DestroyScript(script);
	
	return 0;
}
