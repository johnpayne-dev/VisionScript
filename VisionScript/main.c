#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "Script.h"
#include "Evaluator.h"

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
		if (i == 9 && array.length > 100)
		{
			printf("..., ");
			i = array.length - 5;
		}
	}
	if (array.length > 1) { printf("]"); }
	printf("\n");
}

int main(int argc, const char * argv[])
{
	if (argc < 2)
	{
		printf("VisionScript v0 - Console Interpretter\n");
		HashMap identifiers = HashMapCreate(65536);
		HashMap cache = HashMapCreate(65536);
		while (true)
		{
			String input = StringCreate("");
			printf("\n>>> ");
			for (int32_t c = getchar(); c != '\n'; c = getchar())
			{
				if (c >= 128) { continue; }
				StringConcat(&input, (char []){ (char)c, '\0' });
			}
			if (strcmp(input, "exit") == 0) { break; }
			if (strcmp(input, "") == 0) { continue; }
			
			Statement * statement = ParseTokenLine(TokenizeLine(input));
			if (statement->error.code != SyntaxErrorNone) { printf("SyntaxError: %i\n", statement->error.code); continue; }
			
			if (statement->type == StatementTypeUnknown)
			{
				VectorArray result;
				clock_t timer = clock();
				RuntimeError error = EvaluateExpression(identifiers, cache, NULL, statement->expression, &result);
				timer = clock() - timer;
				if (error.code != RuntimeErrorNone) { printf("RuntimeError: %i\n", error.code); continue; }
				PrintVectorArray(result);
				if (timer / (float)CLOCKS_PER_SEC > 0.01) { printf("Done in %fs\n", timer / (float)CLOCKS_PER_SEC); } 
				for (int8_t d = 0; d < result.dimensions; d++) { free(result.xyzw[d]); }
			}
			if (statement->type == StatementTypeVariable)
			{
				VectorArray * result = malloc(sizeof(VectorArray));
				clock_t timer = clock();
				RuntimeError error = EvaluateExpression(identifiers, cache, NULL, statement->expression, result);
				timer = clock() - timer;
				if (error.code != RuntimeErrorNone) { printf("RuntimeError: %i\n", error.code); continue; }
				if (timer / (float)CLOCKS_PER_SEC > 0.01) { printf("Done in %fs\n", timer / (float)CLOCKS_PER_SEC); }
				HashMapSet(identifiers, statement->declaration.variable.identifier, statement);
				HashMapSet(cache, statement->declaration.variable.identifier, result);
			}
			if (statement->type == StatementTypeFunction) { HashMapSet(identifiers, statement->declaration.function.identifier, statement); }
		}
		return 0;
	}
	printf("%s %s\n", argv[0], argv[1]);
	
	char * code =
		"f(x,y) = (cos(x - y), sin(x + y))\n"
		"s = 2*pi\n"
		"n = 40\n"
		"A = (s/n)*[(i,j) for i = [-n...n] for j = [-n...n]]\n"
		"B = A + (s/n)*f(A.x,A.y)\n"
		"parametric (B - A)*t + A\n";
	
	Script * script = LoadScript(code);
	DestroyScript(script);
	
	return 0;
}
