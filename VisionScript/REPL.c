#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "REPL.h"
#include "Language/Tokenizer.h"
#include "Language/Parser.h"
#include "Language/Evaluator.h"

void RunREPL(void)
{
	printf("VisionScript v1.0 – REPL\n");
	
	Environment environment = CreateEmptyEnvironment();
	list(String) inputs = ListCreate(sizeof(String), 1);
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
		
		list(Token) tokenLine = TokenizeLine(input, ListLength(inputs) - 1);
		Equation equation;
		SyntaxError error = ParseEquation(tokenLine, &equation);
		if (error.code != SyntaxErrorCodeNone) {
			PrintSyntaxError(error, tokenLine, &input);
			FreeEquation(equation);
			FreeTokens(tokenLine);
			continue;
		}
		
		if (equation.type == EquationTypeNone || equation.type == EquationTypeVariable) {
			// evaluate the expression
			clock_t timer = clock();
			VectorArray result;
			RuntimeError error = EvaluateExpression(&environment, NULL, equation.expression, &result);
			timer = clock() - timer;
			if (error.code != RuntimeErrorCodeNone) {
				printf("error %i\n", error.code);//PrintRuntimeError(error, statement);
				FreeEquation(equation);
				FreeTokens(tokenLine);
				continue;
			}
			
			if (equation.type == EquationTypeVariable) {
				SetEnvironmentEquation(&environment, equation);
				SetEnvironmentCache(&environment, CreateBinding(equation.declaration.identifier, result));
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
			SetEnvironmentEquation(&environment, equation);
		}
	}
	FreeEnvironment(environment);
	for (int32_t i = 0; i < ListLength(inputs); i++) { StringFree(inputs[i]); }
	ListFree(inputs);
}
