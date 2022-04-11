#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "Utilities/HashMap.h"
#include "Utilities/String.h"
#include "Language/Parser.h"
#include "Language/Evaluator.h"
#include "Language/Builtin.h"
#include "REPL.h"

#define VARIABLE_LIMIT 65536

void RunREPL(void)
{
	printf("VisionScript v1.0 – REPL\n");
	HashMap identifiers = HashMapCreate(VARIABLE_LIMIT);
	HashMap cache = HashMapCreate(VARIABLE_LIMIT);
	InitializeBuiltins(cache);
	while (true)
	{
		// wait for input
		String input = StringCreate("");
		printf("\n>>> ");
		for (int32_t c = getchar(); c != '\n'; c = getchar())
		{
			if (c >= 128) { continue; }
			StringConcat(&input, (char []){ (char)c, '\0' });
		}
		
		// break on exit or do nothing on empty line
		if (strcmp(input, "exit") == 0) { break; }
		if (strcmp(input, "") == 0) { continue; }
		
		list(Token) tokenLine = TokenizeLine(input);
		Statement * statement = ParseTokenLine(tokenLine);
		StringFree(input);
		
		// skip if it's a render command
		if (statement->type == StatementTypeRender)
		{
			printf("REPL doens't support render commands\n");
			FreeStatement(statement);
			FreeTokens(tokenLine);
			continue;
		}
		
		// assert error
		if (statement->error.code != SyntaxErrorNone)
		{
			printf("SyntaxError: %s", SyntaxErrorToString(statement->error.code));
			// retrieve the code snippet that the error occured at
			int32_t start = statement->tokens[statement->error.tokenStart].lineIndexStart;
			int32_t end = statement->tokens[statement->error.tokenEnd].lineIndexEnd;
			String snippet = StringSub(statement->tokens[0].line, start, end);
			printf(" \"%s\"\n", snippet);
			StringFree(snippet);
			
			FreeStatement(statement);
			FreeTokens(tokenLine);
			continue;
		}
		
		if (statement->type == StatementTypeUnknown || statement->type == StatementTypeVariable)
		{
			VectorArray * result = malloc(sizeof(VectorArray));;
			
			// evaluate the expression
			clock_t timer = clock();
			RuntimeError error = EvaluateExpression(identifiers, cache, NULL, statement->expression, result);
			timer = clock() - timer;
			if (error.code != RuntimeErrorNone)
			{
				printf("RuntimeError: %s", RuntimeErrorToString(error.code));
				// retrieve code snippet that the error occured at
				list(Token) tokens = (error.statement == NULL ? statement : error.statement)->tokens;
				String snippet = StringSub(tokens[0].line, tokens[error.tokenStart].lineIndexStart, tokens[error.tokenEnd].lineIndexEnd);
				printf(" \"%s\"\n", snippet);
				StringFree(snippet);
				
				FreeStatement(statement);
				FreeTokens(tokenLine);
				continue;
			}
			
			if (statement->type == StatementTypeVariable)
			{
				Statement * oldStatement = HashMapGet(identifiers, statement->declaration.variable.identifier);
				VectorArray * oldValue = HashMapGet(cache, statement->declaration.variable.identifier);
				
				HashMapSet(identifiers, statement->declaration.variable.identifier, statement);
				HashMapSet(cache, statement->declaration.variable.identifier, result);
				
				// free old statement and corresponding cache if it exists
				if (oldStatement != NULL)
				{
					FreeTokens(oldStatement->tokens);
					FreeStatement(oldStatement);
				}
				if (oldValue != NULL)
				{
					for (int8_t d = 0; d < oldValue->dimensions; d++) { free(oldValue->xyzw[d]); }
					free(oldValue);
				}
			}
			else
			{
				// otherwise print it and free result
				PrintVectorArray(*result);
				printf("\n");
				for (int8_t d = 0; d < result->dimensions; d++) { free(result->xyzw[d]); }
				FreeStatement(statement);
				FreeTokens(tokenLine);
			}
			
			// print time if it takes more than .01 seconds
			if (timer / (float)CLOCKS_PER_SEC > 0.01) { printf("Done in %fs\n", timer / (float)CLOCKS_PER_SEC); }
		}
	
		if (statement->type == StatementTypeFunction)
		{
			Statement * oldStatement = HashMapGet(identifiers, statement->declaration.function.identifier);
			HashMapSet(identifiers, statement->declaration.function.identifier, statement);
			// free old statement if it exists
			if (oldStatement != NULL)
			{
				FreeTokens(oldStatement->tokens);
				FreeStatement(oldStatement);
			}
		}
	}
}
