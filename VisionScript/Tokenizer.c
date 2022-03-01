#include <string.h>
#include "Tokenizer.h"

static list(String) SplitCodeIntoStatements(String code)
{
	list(String) statements = ListCreate(sizeof(String), 16);
	int lastStatementStart = 0;
	for (int i = 0; i < StringLength(code); i++)
	{
		if (code[i] == ';') { code[i] = '\n'; } // semicolons are treated as newlines
		if (code[i] == '\n')
		{
			if (i > 0 && code[i - 1] == '\\') // backslashes ignore newlines
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

static bool IsLetter(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool IsDigit(char c)
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
	if (!IsLetter(statement[i]) && statement[i] != '_') { return false; } // if first character isn't letter or underscore
	while (IsLetter(statement[*end]) || IsDigit(statement[*end]) || statement[*end] == '_') { (*end)++; } // finds end of identifier
	(*end)--;
	return true;
}

static const char brackets[] = { '(', ')', '[', ']' };
// '{', '}'

static bool IsBracket(String statement, int i, int * end)
{
	*end = i;
	for (int j = 0; j < sizeof(brackets); j++)
	{
		if (statement[i] == brackets[j]) { return true; }
	}
	return false;
}

static const char * operators[] = { "+", "-", "*", "/", "%", "^", "for", "...", "." };
// "==", "~=", "<=", ">=", "<", ">"

static bool IsOperator(String statement, int i, int * end)
{
	*end = i;
	for (int j = 0; j < sizeof(operators) / sizeof(operators[0]); j++)
	{
		int k = 0;
		bool match = true;
		for (k = 0; operators[j][k] != '\0'; k++) // compares each character of the operator with the statement
		{
			if (operators[j][k] != statement[i + k]) { match = false; break; }
		}
		if (match)
		{
			*end = i + k - 1;
			return true;
		}
	}
	return false;
}

static const char symbols[] = { ',', '=' };

static bool IsSymbol(String statement, int i, int * end)
{
	*end = i;
	for (int j = 0; j < sizeof(symbols); j++)
	{
		if (statement[i] == symbols[j]) { return true; }
	}
	return false;
}

static const char * keywords[] = { "point", "parametric", "polygon" };

static bool IsKeyword(String statement, int i, int * end)
{
	*end = i;
	for (int j = 0; j < sizeof(keywords) / sizeof(keywords[0]); j++)
	{
		int k = 0;
		bool match = true;
		for (k = 0; keywords[j][k] != '\0'; k++) // compares each character of the keyword with the statement
		{
			if (keywords[j][k] != statement[i + k]) { match = false; break; }
		}
		if (match)
		{
			*end = i + k - 1;
			return true;
		}
	}
	return false;
}

static bool IsNumber(String statement, int i, int * end)
{
	*end = i;
	if (!IsDigit(statement[i]) && statement[i] != '.') { return false; } // check if first character isn't number or '.'
	while (IsDigit(statement[*end]) || statement[*end] == '.')
	{
		(*end)++;
		if (statement[*end] == '.' && statement[*end + 1] == '.') { break; } // if there's two '.' in a row then assume it's the ... operator
	}
	(*end)--;
	if (*end == i && statement[i] == '.') { return false; } // if the only character is '.' then it's not a number
	return true;
}

list(TokenStatement) Tokenize(String code)
{
	list(TokenStatement) tokenStatements = ListCreate(sizeof(TokenStatement), 16);
	
	list(String) statements = SplitCodeIntoStatements(code);
	for (int i = 0; i < ListLength(statements); i++)
	{
		TokenStatement statement = ListCreate(sizeof(Token), 32);
		int j = 0;
		while (j < StringLength(statements[i]))
		{
			int end = j;
			TokenType tokenType = TokenTypeUnknown;
			if (IsKeyword(statements[i], j, &end)) { tokenType = TokenTypeKeyword; }
			else if (IsNumber(statements[i], j, &end)) { tokenType = TokenTypeNumber; }
			else if (IsOperator(statements[i], j, &end)) { tokenType = TokenTypeOperator; }
			else if (IsIdentifier(statements[i], j, &end)) { tokenType = TokenTypeIdentifier; }
			else if (IsBracket(statements[i], j, &end)) { tokenType = TokenTypeBracket; }
			else if (IsSymbol(statements[i], j, &end)) { tokenType = TokenTypeSymbol; }
			else if (IsWhitespace(statements[i][j])) { j++; continue; }
			ListPush((void **)&statement, &(Token){ .type = tokenType, .value = StringSub(statements[i], j, end) });
			j = end + 1;
		}
		ListPush((void **)&tokenStatements, &statement);
	}
	ListDestroy(statements);

	return tokenStatements;
}
