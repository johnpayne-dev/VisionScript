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
	TokenTypeSymbol,
} TokenType;

typedef struct Token
{
	TokenType type;
	String value;
	String line;
	int32_t lineIndexStart;
	int32_t lineIndexEnd;
} Token;

list(Token) TokenizeLine(String line);
list(list(Token)) TokenizeCode(String code);

void FreeTokens(list(Token) token);

#endif
