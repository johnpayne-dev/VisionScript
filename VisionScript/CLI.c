#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "Utilities/HashMap.h"
#include "Utilities/String.h"
#include "Language/Parser.h"
#include "Language/Evaluator.h"
#include "CLI.h"

void CLIRun(int32_t varLimit)
{
	printf("VisionScript v1.0 – Command-Line Interpretter\n");
	HashMap identifiers = HashMapCreate(varLimit);
	HashMap cache = HashMapCreate(varLimit);
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
		if (statement->error.code != SyntaxErrorNone)
		{
			printf("SyntaxError: %i\n", statement->error.code);
			continue;
		}
		
		if (statement->type == StatementTypeUnknown || statement->type == StatementTypeVariable)
		{
			VectorArray * result = malloc(sizeof(VectorArray));;
			clock_t timer = clock();
			RuntimeError error = EvaluateExpression(identifiers, cache, NULL, statement->expression, result);
			timer = clock() - timer;
			if (error.code != RuntimeErrorNone)
			{
				printf("RuntimeError: %i\n", error.code);
				continue;
			}
			
			if (statement->type == StatementTypeVariable)
			{
				HashMapSet(identifiers, statement->declaration.variable.identifier, statement);
				HashMapSet(cache, statement->declaration.variable.identifier, result);
			}
			else
			{
				VectorArrayPrint(*result);
				for (int8_t d = 0; d < result->dimensions; d++) { free(result->xyzw[d]); }
			}
			
			if (timer / (float)CLOCKS_PER_SEC > 0.01) { printf("Done in %fs\n", timer / (float)CLOCKS_PER_SEC); }
		}
	
		if (statement->type == StatementTypeFunction) { HashMapSet(identifiers, statement->declaration.function.identifier, statement); }
	}
}
