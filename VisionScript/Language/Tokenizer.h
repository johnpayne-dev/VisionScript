#ifndef Tokenizer_h
#define Tokenizer_h

#include "Utilities/String.h"
#include "Utilities/List.h"

#define KEYWORD_POINTS     "points"
#define KEYWORD_PARAMETRIC "parametric"
#define KEYWORD_POLYGONS   "polygons"
#define KEYWORD_FOR        "for"
#define KEYWORD_WHEN       "when"
#define KEYWORD_IF         "if"
#define KEYWORD_ELSE       "else"
#define KEYWORD_NOT        "not"

#define SYMBOL_COMMA             ","
#define SYMBOL_EQUAL             "="
#define SYMBOL_LEFT_PARENTHESIS  "("
#define SYMBOL_RIGHT_PARENTHESIS ")"
#define SYMBOL_LEFT_BRACKET      "["
#define SYMBOL_RIGHT_BRACKET     "]"
#define SYMBOL_PLUS              "+"
#define SYMBOL_MINUS             "-"
#define SYMBOL_ASTERICK          "*"
#define SYMBOL_SLASH             "/"
#define SYMBOL_PERCENT           "%"
#define SYMBOL_CARROT            "^"
#define SYMBOL_LESS              "<"
#define SYMBOL_GREATER           ">"
#define SYMBOL_TILDE             "~"
#define SYMBOL_DOT               "."
#define SYMBOL_EXCLAMATION       "!"
#define SYMBOL_EQUAL_EQUAL       "=="
#define SYMBOL_BAR_EQUAL         "|="
#define SYMBOL_GREATER_EQUAL     ">="
#define SYMBOL_LESS_EQUAL        "<="

typedef enum TokenType {
	TokenTypeUnknown,
	TokenTypeKeyword,
	TokenTypeIdentifier,
	TokenTypeNumber,
	TokenTypeSymbol,
} TokenType;

typedef struct Token {
	TokenType type;
	String value;
	int32_t line;
	int32_t start;
	int32_t end;
} Token;

List(Token) TokenizeLine(String line, int32_t lineNumber);
void FreeTokens(List(Token) token);

List(String) SplitCodeIntoLines(char * code);

#endif
