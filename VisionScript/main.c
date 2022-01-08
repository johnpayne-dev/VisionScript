#include <stdio.h>
#include "Tokenizer.h"

static void PrintTokens(list(TokenStatement) tokens)
{
	for (int i = 0; i < ListLength(tokens); i++)
	{
		for (int j = 0; j < ListLength(tokens[i]); j++)
		{
			const char * tokenType = "";
			switch (tokens[i][j].type)
			{
				case TokenTypeUnknown: tokenType = "Unknown"; break;
				case TokenTypeKeyword: tokenType = "Keyword"; break;
				case TokenTypeIdentifier: tokenType = "Identifier"; break;
				case TokenTypeConstant: tokenType = "Constant"; break;
				case TokenTypeSymbol: tokenType = "Symbol"; break;
				case TokenTypeIndent: tokenType = "Indent"; break;
			}
			printf("%i,%i %s\t%s\n", i, j, tokenType, tokens[i][j].value);
		}
	}
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
		"f(x,y) = <cos(x - y), sin(x + y)>\n"
		"s = 2*pi\n"
		"\n"
		"field:\n"
		"	n = 40\n"
		"	A = (s/n)*[[2*<i, j> - 1 for j = [0 ~ n]] for i = [0 ~ n]]\n"
		"	B = A + (s/n)*f(A.x,A.y)\n"
		"	render <(B.x - A.x)*t + A.x, (B.y - A.y)*t + A.y>\n";
	
	list(TokenStatement) tokens = Tokenize(StringCreate(code));
	PrintTokens(tokens);
	ListDestroy(tokens);
	
	return 0;
}
