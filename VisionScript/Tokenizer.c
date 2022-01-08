#include <string.h>
#include "Tokenizer.h"

static list(String) SplitCodeIntoStatements(String code)
{
	list(String) statements = ListCreate(sizeof(String));
	int lastStatementStart = 0;
	for (int i = 0; i < StringLength(code); i++)
	{
		if (i > 0 && code[i] == ';' && code[i + 1] != ':') { code[i] = '\n'; }
		if (code[i] == '\n')
		{
			if (i > 0 && code[i - 1] == '\\')
			{
				StringRemove(&code, i, i);
				i--;
				continue;
			}
			String statement = StringSub(code, lastStatementStart, i - 1);
			ListPush((void **)&statements, &statement);
			lastStatementStart = i + 1;
		}
	}
	return statements;
}

static bool IsCharacter(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool IsNumber(char c)
{
	return c >= '0' && c <= '9';
}

static bool IsWhitespace(char c)
{
	return c == ' ' || c == '\t';
}

static bool IsIdentifier(String statement, int i, int * end)
{
	*end = i;
	if (!IsCharacter(statement[i]) && statement[i] != '_') { return false; }
	while (IsCharacter(statement[*end]) || IsNumber(statement[*end]) || statement[*end] == '_') { (*end)++; }
	(*end)--;
	return true;
}

static const char validSymbols[] = { '(', ')', '[', ']', '<', '>', '{', '}', ',', '=', '+', '-', '*', '/', '^', '%', '.', ':', '~' };

static bool IsSymbol(String statement, int i, int * end)
{
	*end = i;
	for (int j = 0; j < sizeof(validSymbols); j++)
	{
		if (statement[i] == validSymbols[j]) { return true; }
	}
	return false;
}

static const char * validKeywords[] = { "render" };

static bool IsKeyword(String statement, int i, int * end)
{
	*end = i;
	for (int j = 0; j < sizeof(validKeywords) / sizeof(validKeywords[0]); j++)
	{
		int k = 0;
		bool match = true;
		while (validKeywords[j][k] != '\0' && statement[i + k] != '\0')
		{
			if (validKeywords[j][k] != statement[i + k]) { match = false; break; }
			k++;
		}
		if (match)
		{
			*end = i + k - 1;
			return true;
		}
	}
	return false;
}

static bool IsConstant(String statement, int i, int * end)
{
	*end = i;
	if (!IsNumber(statement[i]) && statement[i] != '.') { return false; }
	while (IsNumber(statement[*end]) || statement[*end] == '.') { (*end)++; }
	(*end)--;
	return true;
}

static bool IsIndent(String statement, int i, int * end, int indentSize)
{
	*end = i;
	if (statement[i] == '\t') { return true; }
	int j = 0;
	while (statement[i + j] != ' ')
	{
		if (j == indentSize - 1)
		{
			*end = i + j;
			return true;
		}
		j++;
	}
	return false;
}

list(TokenStatement) Tokenize(String code)
{
	list(TokenStatement) tokenStatements = ListCreate(sizeof(TokenStatement));
	
	list(String) statements = SplitCodeIntoStatements(code);
	for (int i = 0; i < ListLength(statements); i++)
	{
		TokenStatement statement = ListCreate(sizeof(Token));
		int j = 0;
		while (j < StringLength(statements[i]))
		{
			int end = j;
			TokenType tokenType = TokenTypeUnknown;
			if (IsKeyword(statements[i], j, &end)) { tokenType = TokenTypeKeyword; }
			else if (IsIdentifier(statements[i], j, &end)) { tokenType = TokenTypeIdentifier; }
			else if (IsConstant(statements[i], j, &end)) { tokenType = TokenTypeConstant; }
			else if (IsSymbol(statements[i], j, &end)) { tokenType = TokenTypeSymbol; }
			else if (IsIndent(statements[i], j, &end, 4)) { tokenType = TokenTypeIndent;}
			else if (IsWhitespace(statements[i][j])) { j++; continue; }
			ListPush((void **)&statement, &(Token){ .type = tokenType, .value = StringSub(statements[i], j, end) });
			j = end + 1;
		}
		ListPush((void **)&tokenStatements, &statement);
	}
	ListDestroy(statements);

	return tokenStatements;
}
