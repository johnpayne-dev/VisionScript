#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "REPL.h"
#include "Language/Tokenizer.h"
#include "Language/Parser.h"
#include "Language/Evaluator.h"
#include "Language/Builtin.h"

void RunREPL(void)
{
	printf("VisionScript v1.0 – REPL\n");
	
	Environment environment = CreateEmptyEnvironment();
	InitializeBuiltinVariables(&environment);
	List(String) inputs = ListCreate(sizeof(String), 1);
	while (true) {
		// wait for input
		String input = StringCreate("");
		printf("\n>>> ");
		for (int32_t c = getchar(); c != '\n'; c = getchar()) {
			if (c >= 128) { continue; }
			StringConcat(&input, (char []){ (char)c, '\0' });
		}
		if (strcmp(input, "exit") == 0) { break; }
		if (strcmp(input, "") == 0) {
			StringFree(input);
			continue;
		}
		inputs = ListPush(inputs, &input);
		
		List(Token) tokenLine = TokenizeLine(input, ListLength(inputs) - 1);
		Equation equation;
		SyntaxError error = ParseEquation(tokenLine, &equation);
		if (error.code != SyntaxErrorCodeNone) {
			PrintSyntaxError(error, inputs);
			FreeTokens(tokenLine);
			continue;
		}
		//PrintExpression(equation.expression);
		//printf("\n");
		
		if (equation.type == EquationTypeNone || equation.type == EquationTypeVariable) {
			// evaluate the expression
			clock_t timer = clock();
			VectorArray result;
			RuntimeError error = EvaluateExpression(&environment, NULL, equation.expression, &result);
			timer = clock() - timer;
			if (error.code != RuntimeErrorCodeNone) {
				PrintRuntimeError(error, inputs);
				FreeEquation(equation);
				FreeTokens(tokenLine);
				continue;
			}
			
			if (equation.type == EquationTypeVariable) {
				AddEnvironmentEquation(&environment, equation);
				SetEnvironmentCache(&environment, equation.declaration.identifier, CopyVectorArray(result));
			}
			else {
				PrintVectorArray(result);
				printf("\n");
				FreeEquation(equation);
				FreeTokens(tokenLine);
			}
			
			// print time if it takes more than .01 seconds
			if (timer / (float)CLOCKS_PER_SEC > 0.01) { printf("Done in %fs\n", timer / (float)CLOCKS_PER_SEC); }
			FreeVectorArray(result);
		}
		
		if (equation.type == EquationTypeFunction) {
			AddEnvironmentEquation(&environment, equation);
		}
	}
	FreeEnvironment(environment);
	for (int32_t i = 0; i < ListLength(inputs); i++) { StringFree(inputs[i]); }
	ListFree(inputs);
}
