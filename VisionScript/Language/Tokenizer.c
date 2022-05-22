#include "Tokenizer.h"

static bool IsLetter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

static bool IsDigit(char c) { return c >= '0' && c <= '9'; }

static bool IsWhitespace(char c) { return c == ' ' || c == '\t'; }

static bool IsValidInIdentifier(char c) { return IsLetter(c) || IsDigit(c) || c == '_' || c == ':'; }

static bool MatchesWord(String line, int32_t start, int32_t * end, const char * word) {
	int32_t j = 0;
	while (true) {
		if (word[j] == '\0' && !IsValidInIdentifier(line[start + j])) {
			*end = start + j - 1;
			return true;
		}
		if (word[j] != line[start + j] || line[start + j] == '\0') { return false; }
		j++;
	}
}

static const char * keywords[] = {
	KEYWORD_POINTS,
	KEYWORD_PARAMETRIC,
	KEYWORD_POLYGONS,
	KEYWORD_FOR,
	KEYWORD_WHEN,
	KEYWORD_IF,
	KEYWORD_ELSE,
	KEYWORD_NOT,
};

static bool IsKeyword(String line, int32_t start, int32_t * end) {
	for (int32_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
		if (MatchesWord(line, start, end, keywords[i])) { return true; }
	}
	return false;
}

static bool IsIdentifier(String line, int32_t start, int32_t * end) {
	*end = start;
	if (!IsLetter(line[start]) && line[start] != '_') { return false; } // if first character isn't letter or underscore
	while (IsValidInIdentifier(line[*end])) { (*end)++; } // finds end of identifier
	(*end)--;
	return true;
}

static const char * singleSymbols[] = {
	SYMBOL_COMMA,
	SYMBOL_EQUAL,
	SYMBOL_LEFT_PARENTHESIS,
	SYMBOL_RIGHT_PARENTHESIS,
	SYMBOL_LEFT_BRACKET,
	SYMBOL_RIGHT_BRACKET,
	SYMBOL_PLUS,
	SYMBOL_MINUS,
	SYMBOL_ASTERICK,
	SYMBOL_SLASH,
	SYMBOL_PERCENT,
	SYMBOL_CARROT,
	SYMBOL_LESS,
	SYMBOL_GREATER,
	SYMBOL_TILDE,
	SYMBOL_DOT,
	SYMBOL_EXCLAMATION,
};

static const char * doubleSymbols[] = {
	SYMBOL_EQUAL_EQUAL,
	SYMBOL_BAR_EQUAL,
	SYMBOL_GREATER_EQUAL,
	SYMBOL_LESS_EQUAL,
};

static bool IsSymbol(String line, int32_t start, int32_t * end) {
	for (int32_t i = 0; i < sizeof(doubleSymbols) / sizeof(doubleSymbols[0]); i++) {
		if (line[start] == doubleSymbols[i][0] && line[start + 1] == doubleSymbols[i][1]) {
			*end = start + 1;
			return true;
		}
	}
	if (line[start] == '.') {
		if (IsDigit(line[start + 1]) || (start > 0 && IsDigit(line[start - 1]))) { return false; }
	}
	for (int32_t i = 0; i < sizeof(singleSymbols) / sizeof(singleSymbols[0]); i++) {
		if (line[start] == singleSymbols[i][0]) {
			*end = start;
			return true;
		}
	}
	return false;
}


static bool IsNumber(String line, int32_t start, int32_t * end) {
	int32_t i = start;
	while (IsDigit(line[i]) || line[i] == '.') { i++; }
	if (i > start) {
		*end = i - 1;
		return true;
	} else { return false; }
}

List(Token) TokenizeLine(String line, int32_t lineNumber) {
	List(Token) tokens = ListCreate(sizeof(Token), 32);
	int32_t i = 0;
	while (i < StringLength(line)) {
		int32_t end = i;
		TokenType tokenType = TokenTypeUnknown;
		if (IsKeyword(line, i, &end)) { tokenType = TokenTypeKeyword; }
		else if (IsIdentifier(line, i, &end)) { tokenType = TokenTypeIdentifier; }
		else if (IsSymbol(line, i, &end)) { tokenType = TokenTypeSymbol; }
		else if (IsNumber(line, i, &end)) { tokenType = TokenTypeNumber; }
		else if (IsWhitespace(line[i])) {
			i++;
			continue;
		}
		tokens = ListPush(tokens, &(Token){ .type = tokenType, .value = StringSub(line, i, end), .lineNumber = lineNumber, .start = i, .end = end });
		i = end + 1;
	}
	return tokens;
}

void FreeTokens(List(Token) tokens) {
	for (int32_t i = 0; i < ListLength(tokens); i++) { StringFree(tokens[i].value); }
	ListFree(tokens);
}

List(String) SplitCodeIntoLines(char * code) {
	code = StringCreate(code);
	List(String) statements = ListCreate(sizeof(String), 16);
	int32_t lastStatementStart = 0;
	for (int32_t i = 0; i < StringLength(code); i++) {
		if (code[i] == ';') { code[i] = '\n'; } // semicolons are treated as newlines
		if (code[i] == '\n') {
			if (i > 0 && code[i - 1] == '\\') { // backslashes ignore newlines
				StringRemove(&code, i, i);
				i--;
				continue;
			}
			String statement = StringSub(code, lastStatementStart, i - 1);
			statements = ListPush(statements, &statement);
			lastStatementStart = i + 1;
		}
	}
	String statement = StringSub(code, lastStatementStart, StringLength(code) - 1);
	statements = ListPush(statements, &statement); // add last line if it doesn't end with '\n'
	
	StringFree(code);
	return statements;
}
