#ifndef Tokenizer_h
#define Tokenizer_h

#include "Utilities/String.h"
#include "Utilities/List.h"

typedef enum TokenType
{
	TokenTypeUnknown,
	TokenTypeKeyword,
	TokenTypeIdentifier,
	TokenTypeNumber,
	TokenTypeBracket,
	TokenTypeOperator,
	TokenTypeSeparator,
} TokenType;

typedef struct Token
{
	TokenType type;
	String value;
} Token;

typedef list(Token) TokenStatement;

list(TokenStatement) Tokenize(String code);

#endif
