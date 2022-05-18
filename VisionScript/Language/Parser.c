#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Parser.h"

static const char * operators[] = {
	[OperatorAdd]          = SYMBOL_PLUS,
	[OperatorSubtract]     = SYMBOL_MINUS,
	[OperatorMultiply]     = SYMBOL_ASTERICK,
	[OperatorDivide]       = SYMBOL_SLASH,
	[OperatorModulo]       = SYMBOL_PERCENT,
	[OperatorPower]        = SYMBOL_CARROT,
	[OperatorEqual]        = SYMBOL_EQUAL_EQUAL,
	[OperatorNotEqual]     = SYMBOL_BAR_EQUAL,
	[OperatorGreater]      = SYMBOL_GREATER,
	[OperatorGreaterEqual] = SYMBOL_GREATER_EQUAL,
	[OperatorLess]         = SYMBOL_LESS,
	[OperatorLessEqual]    = SYMBOL_LESS_EQUAL,
	[OperatorRange]        = SYMBOL_TILDE,
	[OperatorFor]          = KEYWORD_FOR,
	[OperatorWhen]         = KEYWORD_WHEN,
	[OperatorDimension]    = SYMBOL_DOT,
	[OperatorIndexStart]   = SYMBOL_LEFT_BRACKET,
	[OperatorIndexEnd]     = SYMBOL_RIGHT_BRACKET,
	[OperatorCallStart]    = SYMBOL_LEFT_PARENTHESIS,
	[OperatorCallEnd]      = SYMBOL_RIGHT_PARENTHESIS,
	[OperatorNegate]       = SYMBOL_MINUS,
	[OperatorFactorial]    = SYMBOL_EXCLAMATION,
	[OperatorNot]          = KEYWORD_NOT,
	[OperatorIf]           = KEYWORD_IF,
	[OperatorElse]         = KEYWORD_ELSE,
};

static Operator prefixUnaryOperators[] = {
	OperatorNegate,
	OperatorNot,
};

static Operator postfixUnaryOperators[] = {
	OperatorFactorial,
};

static Operator ternaryLeftOperators[] = {
	OperatorIf,
	OperatorFor,
};

static Operator ternaryRightOperators[] = {
	OperatorElse,
	OperatorWhen,
};

static const int32_t precedences[] = {
	[OperatorRange]        = 0,
	[OperatorFor]          = 0,
	[OperatorWhen]         = 0,
	[OperatorIf]           = 1,
	[OperatorElse]         = 1,
	[OperatorEqual]        = 4,
	[OperatorNotEqual]     = 4,
	[OperatorGreater]      = 4,
	[OperatorGreaterEqual] = 4,
	[OperatorLess]         = 4,
	[OperatorLessEqual]    = 4,
	[OperatorAdd]          = 5,
	[OperatorSubtract]     = 5,
	[OperatorMultiply]     = 6,
	[OperatorDivide]       = 6,
	[OperatorModulo]       = 6,
	[OperatorNegate]       = 7,
	[OperatorNot]          = 7,
	[OperatorPower]        = 8,
	[OperatorFactorial]    = 9,
	[OperatorDimension]    = 10,
	[OperatorIndexStart]   = 11,
	[OperatorIndexEnd]     = 11,
	[OperatorCallStart]    = 12,
	[OperatorCallEnd]      = 12,
};

static bool IsPrefix(Operator operator) {
	for (int32_t i = 0; i < sizeof(prefixUnaryOperators) / sizeof(prefixUnaryOperators[0]); i++) {
		if (operator == prefixUnaryOperators[i]) { return true; }
	}
	return false;
}

static bool IsPostfix(Operator operator) {
	for (int32_t i = 0; i < sizeof(postfixUnaryOperators) / sizeof(postfixUnaryOperators[0]); i++) {
		if (operator == postfixUnaryOperators[i]) { return true; }
	}
	return false;
}

static bool IsUnary(Operator operator) {
	return IsPrefix(operator) || IsPostfix(operator);
}

static Operator OperatorFromSymbol(const char * symbol) {
	for (int32_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++) {
		if (strcmp(symbol, operators[i]) == 0) { return i; }
	}
	return -1;
}

static Operator UnaryOperatorFromSymbol(const char * symbol) {
	for (int32_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++) {
		if (IsUnary(i) && (strcmp(symbol, operators[i]) == 0)) { return i; }
	}
	return -1;
}

static bool IsTernary(Operator left, Operator right) {
	for (int32_t i = 0; i < sizeof(ternaryLeftOperators) / sizeof(ternaryLeftOperators[0]); i++) {
		if (left == ternaryLeftOperators[i] && right == ternaryRightOperators[i]) { return true; }
	}
	return false;
}

static bool IsTokenOperable(Token token) {
	return token.type == TokenTypeIdentifier || token.type == TokenTypeNumber || StringEquals(token.value, SYMBOL_RIGHT_PARENTHESIS) || StringEquals(token.value, SYMBOL_RIGHT_BRACKET);
}

static int32_t FindCorrespondingToken(list(Token) tokens, int32_t start, int32_t end, const char * open, const char * close, int32_t step) {
	int32_t depth = 0;
	for (int32_t i = step > 0 ? start : end; i != (step > 0 ? end + 1: start - 1); i += step) {
		if (StringEquals(tokens[i].value, open)) { depth++; }
		if (StringEquals(tokens[i].value, close)) {
			depth--;
			if (depth == 0) { return i; }
		}
	}
	return -1;
}

static int32_t FindClosingParenthesis(list(Token) tokens, int32_t start, int32_t end) {
	return FindCorrespondingToken(tokens, start, end, SYMBOL_LEFT_PARENTHESIS, SYMBOL_RIGHT_PARENTHESIS, 1);
}

static int32_t FindClosingBracket(list(Token) tokens, int32_t start, int32_t end) {
	return FindCorrespondingToken(tokens, start, end, SYMBOL_LEFT_BRACKET, SYMBOL_RIGHT_BRACKET, 1);
}

static int32_t FindComma(list(Token) tokens, int32_t start, int32_t end) {
	for (int32_t i = start + 1; i <= end - 1; i++) {
		if (StringEquals(tokens[i].value, SYMBOL_LEFT_PARENTHESIS)) {
			i = FindClosingParenthesis(tokens, i, end);
			if (i == -1) { break; }
			continue;
		}
		if (StringEquals(tokens[i].value, SYMBOL_LEFT_BRACKET)) {
			i = FindClosingBracket(tokens, i, end);
			if (i == -1) { break; }
			continue;
		}
		if (StringEquals(tokens[i].value, SYMBOL_COMMA)) { return i; }
	}
	return -1;
}

static Operator FindOperator(list(Token) tokens, int32_t start, int32_t end, int32_t * operatorStart, int32_t * operatorEnd) {
	for (int32_t i = start; i <= end; i++) {
		*operatorStart = i;
		*operatorEnd = i;
		for (int32_t j = 0; j < sizeof(operators) / sizeof(operators[0]); j++) {
			if (StringEquals(tokens[i].value, operators[j])) {
				if (j == OperatorCallStart || j == OperatorIndexStart || j == OperatorSubtract) {
					if (i > 0 && IsTokenOperable(tokens[i - 1])) {
						if (j == OperatorCallStart) { *operatorEnd = FindClosingParenthesis(tokens, i, end); }
						if (j == OperatorIndexStart) { *operatorEnd = FindClosingBracket(tokens, i, end); }
						return j;
					}
				} else { return j; }
			}
		}
		if (StringEquals(tokens[i].value, SYMBOL_LEFT_PARENTHESIS)) {
			i = FindClosingParenthesis(tokens, i, end);
			if (i == -1) { break; }
			continue;
		}
		if (StringEquals(tokens[i].value, SYMBOL_LEFT_BRACKET)) {
			i = FindClosingBracket(tokens, i, end);
			if (i == -1) { break; }
			continue;
		}
	}
	return -1;
}

static list(int32_t) FindLowestPrecedenceOperators(list(Token) tokens, int32_t start, int32_t end, int32_t * lowestPrecedence) {
	*lowestPrecedence = OperatorCount;
	int32_t opStart, opEnd;
	Operator operator = FindOperator(tokens, start, end, &opStart, &opEnd);
	while (operator != -1) {
		if (precedences[operator] < *lowestPrecedence) { *lowestPrecedence = precedences[operator]; }
		operator = FindOperator(tokens, opEnd + 1, end, &opStart, &opEnd);
	}
	
	list(int32_t) indices = ListCreate(sizeof(int32_t), 1);
	operator = FindOperator(tokens, start, end, &opStart, &opEnd);
	while (operator != -1) {
		if (precedences[operator] == *lowestPrecedence) { indices = ListPush(indices, &opStart); }
		operator = FindOperator(tokens, opEnd + 1, end, &opStart, &opEnd);
	}
	return indices;
}

static SyntaxError CheckMissingBraces(list(Token) tokens, int32_t start, int32_t end) {
	for (int32_t i = start; i <= end; i++) {
		if (StringEquals(tokens[i].value, SYMBOL_LEFT_PARENTHESIS)) {
			int32_t next = FindClosingParenthesis(tokens, i, end);
			if (next == -1) { return (SyntaxError){ SyntaxErrorCodeMissingClosingBrace, i, end }; }
			i = next;
			continue;
		}
		if (StringEquals(tokens[i].value, SYMBOL_RIGHT_PARENTHESIS)) {
			return (SyntaxError){ SyntaxErrorCodeMismatchedBrace, start, i };
		}
		if (StringEquals(tokens[i].value, SYMBOL_LEFT_BRACKET)) {
			int32_t next = FindClosingBracket(tokens, i, end);
			if (next == -1) { return (SyntaxError){ SyntaxErrorCodeMissingClosingBrace, i, end }; }
			i = next;
			continue;
		}
		if (StringEquals(tokens[i].value, SYMBOL_RIGHT_BRACKET)) {
			return (SyntaxError){ SyntaxErrorCodeMismatchedBrace, start, i };
		}
	}
	return (SyntaxError){ SyntaxErrorCodeNone };
}

static ExpressionType DetermineExpressionType(list(Token) tokens, int32_t start, int32_t end) {
	if (start == end) {
		if (tokens[start].type == TokenTypeNumber) { return ExpressionTypeConstant; }
		if (tokens[start].type == TokenTypeIdentifier) { return ExpressionTypeIdentifier; }
	}
	if (StringEquals(tokens[start].value, SYMBOL_LEFT_PARENTHESIS) && FindClosingParenthesis(tokens, start, end) == end) {
		if (start > 0 && tokens[start - 1].type == TokenTypeIdentifier) { return ExpressionTypeArguments; }
		else { return ExpressionTypeVectorLiteral; }
	}
	if (StringEquals(tokens[start].value, SYMBOL_LEFT_BRACKET) && FindClosingBracket(tokens, start, end) == end) {
		return ExpressionTypeArrayLiteral;
	}
	if (tokens[start].type == TokenTypeIdentifier && StringEquals(tokens[start + 1].value, SYMBOL_EQUAL)) {
		return ExpressionTypeForAssignment;
	}
	
	int32_t precedence = 0;
	list(int32_t) indices = FindLowestPrecedenceOperators(tokens, start, end, &precedence);
	if (ListLength(indices) == 0) { return ExpressionTypeUnknown; }
	ExpressionType type = ExpressionTypeBinary;
	
	for (int32_t i = 0; i < ListLength(indices) - 1; i++) {
		Operator left = OperatorFromSymbol(tokens[indices[i]].value);
		Operator right = OperatorFromSymbol(tokens[indices[i + 1]].value);
		if (IsTernary(left, right)) {
			type = ExpressionTypeTernary;
			break;
		}
	}
	
	for (int32_t i = 0; i < ListLength(indices); i++) {
		if (UnaryOperatorFromSymbol(tokens[indices[i]].value) != -1 && precedence != precedences[OperatorSubtract]) {
			type = ExpressionTypeUnary;
			break;
		}
	}
	
	ListFree(indices);
	return type;
}

static SyntaxError ParseConstant(list(Token) tokens, int32_t start, int32_t end, Expression * expression) {
	char * endPtr = NULL;
	expression->constant = strtod(tokens[start].value, &endPtr);
	if (*endPtr != '\0') { return (SyntaxError){ SyntaxErrorCodeUnreadableConstant, start, end }; }
	return (SyntaxError){ SyntaxErrorCodeNone };
}

static SyntaxError ParseIdentifier(list(Token) tokens, int32_t start, int32_t end, Expression * expression) {
	expression->identifier = StringCreate(tokens[start].value);
	return (SyntaxError){ SyntaxErrorCodeNone };
}

static SyntaxError ParseCommas(list(Token) tokens, int32_t start, int32_t end, list(Expression) * expressions) {
	int32_t index = FindComma(tokens, start, end);
	int32_t prevIndex = start;
	while (index > -1) {
		*expressions = ListPush(*expressions, &(Expression){ 0 });
		SyntaxError error = ParseExpression(tokens, prevIndex + 1, index - 1, &(*expressions)[ListLength(*expressions) - 1]);
		if (error.code != SyntaxErrorCodeNone) { return error; }
		prevIndex = index;
		index = FindComma(tokens, index + 1, end);
	}
	*expressions = ListPush(*expressions, &(Expression){ 0 });
	SyntaxError error = ParseExpression(tokens, prevIndex + 1, end - 1, &(*expressions)[ListLength(*expressions) - 1]);
	if (error.code != SyntaxErrorCodeNone) { return error; }
	
	return (SyntaxError){ SyntaxErrorCodeNone };
}

static SyntaxError ParseVectorLiteral(list(Token) tokens, int32_t start, int32_t end, Expression * expression) {
	expression->vector = ListCreate(sizeof(Expression), 1);
	SyntaxError error = ParseCommas(tokens, start, end, &expression->vector);
	if (error.code != SyntaxErrorCodeNone) { return error; }
	if (ListLength(expression->vector) > 4) { return (SyntaxError){ SyntaxErrorCodeTooManyVectorElements, start, end }; }
	return (SyntaxError){ SyntaxErrorCodeNone };
}

static SyntaxError ParseArrayLiteral(list(Token) tokens, int32_t start, int32_t end, Expression * expression) {
	expression->array = ListCreate(sizeof(Expression), 1);
	return ParseCommas(tokens, start, end, &expression->array);
}

static SyntaxError ParseArguments(list(Token) tokens, int32_t start, int32_t end, Expression * expression) {
	expression->arguments = ListCreate(sizeof(Expression), 1);
	return ParseCommas(tokens, start, end, &expression->arguments);
}

static SyntaxError ParseForAssignment(list(Token) tokens, int32_t start, int32_t end, Expression * expression) {
	expression->assignment.identifier = StringCreate(tokens[start].value);
	expression->assignment.expression = calloc(1, sizeof(Expression));
	return ParseExpression(tokens, start + 2, end, expression->assignment.expression);
}

static SyntaxError ParseUnary(list(Token) tokens, int32_t start, int32_t end, Expression * expression) {
	list(int32_t) indices = FindLowestPrecedenceOperators(tokens, start, end, &(int32_t){ 0 });
	expression->unary.expression = calloc(1, sizeof(Expression));
	expression->unary.operator = UnaryOperatorFromSymbol(tokens[indices[0]].value);
	
	SyntaxError error = (SyntaxError){ SyntaxErrorCodeNone };
	if (IsPostfix(expression->unary.operator)) {
		if (indices[ListLength(indices) - 1] != end) { return (SyntaxError){ SyntaxErrorCodeInvalidUnaryPlacement, start, end }; }
		error = ParseExpression(tokens, start, end - 1, expression->unary.expression);
	} else {
		if (indices[0] != start) { return (SyntaxError){ SyntaxErrorCodeInvalidUnaryPlacement, start, end }; }
		error = ParseExpression(tokens, start + 1, end, expression->unary.expression);
	}
	ListFree(indices);
	return error;
}

static SyntaxError ParseBinary(list(Token) tokens, int32_t start, int32_t end, Expression * expression) {
	list(int32_t) indices = FindLowestPrecedenceOperators(tokens, start, end, &(int32_t){ 0 });
	int32_t opIndex = indices[ListLength(indices) - 1];
	ListFree(indices);
	expression->binary.left = calloc(1, sizeof(Expression));
	expression->binary.right = calloc(1, sizeof(Expression));
	expression->binary.operator = OperatorFromSymbol(tokens[opIndex].value);
	SyntaxError error = { SyntaxErrorCodeNone };
	if (expression->binary.operator == OperatorCallStart || expression->binary.operator == OperatorIndexStart) {
		error = ParseExpression(tokens, start, opIndex - 1, expression->binary.left);
		if (error.code != SyntaxErrorCodeNone) { return error; }
		error = ParseExpression(tokens, opIndex, end, expression->binary.right);
	} else {
		error = ParseExpression(tokens, start, opIndex - 1, expression->binary.left);
		if (error.code != SyntaxErrorCodeNone) { return error; }
		error = ParseExpression(tokens, opIndex + 1, end, expression->binary.right);
	}
	return error;
}

static SyntaxError ParseTernary(list(Token) tokens, int32_t start, int32_t end, Expression * expression) {
	list(int32_t) indices = FindLowestPrecedenceOperators(tokens, start, end, &(int32_t){ 0 });
	if (ListLength(indices) != 2) {
		ListFree(indices);
		return (SyntaxError){ SyntaxErrorCodeInvalidTernaryPlacement, start, end };
	}
	int32_t leftOpIndex = indices[0];
	int32_t rightOpIndex = indices[1];
	ListFree(indices);
	expression->ternary.left = calloc(1, sizeof(Expression));
	expression->ternary.middle = calloc(1, sizeof(Expression));
	expression->ternary.right = calloc(1, sizeof(Expression));
	expression->ternary.leftOperator = OperatorFromSymbol(tokens[leftOpIndex].value);
	expression->ternary.rightOperator = OperatorFromSymbol(tokens[rightOpIndex].value);
	
	SyntaxError error = ParseExpression(tokens, start, leftOpIndex - 1, expression->ternary.left);
	if (error.code != SyntaxErrorCodeNone) { return error; }
	error = ParseExpression(tokens, leftOpIndex + 1, rightOpIndex - 1, expression->ternary.middle);
	if (error.code != SyntaxErrorCodeNone) { return error; }
	error = ParseExpression(tokens, rightOpIndex + 1, end, expression->ternary.right);
	return error;
}

SyntaxError ParseExpression(list(Token) tokens, int32_t start, int32_t end, Expression * expression) {
	if (end < start) { return (SyntaxError){ SyntaxErrorCodeMissingExpression, end, start }; }
	
	SyntaxError error = CheckMissingBraces(tokens, start, end);
	if (error.code != SyntaxErrorCodeNone) { return error; }
	
	if (StringEquals(tokens[start].value, SYMBOL_LEFT_PARENTHESIS) && FindClosingParenthesis(tokens, start, end) == end && FindComma(tokens, start, end) == -1) {
		return ParseExpression(tokens, start + 1, end - 1, expression);
	}
	
	expression->type = DetermineExpressionType(tokens, start, end);
	switch (expression->type) {
		case ExpressionTypeUnknown: return (SyntaxError){ SyntaxErrorCodeNonsenseExpression, start, end };
		case ExpressionTypeConstant: return ParseConstant(tokens, start, end, expression);
		case ExpressionTypeIdentifier: return ParseIdentifier(tokens, start, end, expression);
		case ExpressionTypeVectorLiteral: return ParseVectorLiteral(tokens, start, end, expression);
		case ExpressionTypeArrayLiteral: return ParseArrayLiteral(tokens, start, end, expression);
		case ExpressionTypeArguments: return ParseArguments(tokens, start, end, expression);
		case ExpressionTypeForAssignment: return ParseForAssignment(tokens, start, end, expression);
		case ExpressionTypeUnary: return ParseUnary(tokens, start, end, expression);
		case ExpressionTypeBinary: return ParseBinary(tokens, start, end, expression);
		case ExpressionTypeTernary: return ParseTernary(tokens, start, end, expression);
	}
	
	return (SyntaxError){ SyntaxErrorCodeNone };
}

void PrintExpression(Expression expression) {
	if (expression.type == ExpressionTypeUnknown) { printf("#error#"); }
	if (expression.type == ExpressionTypeConstant) { printf("%f", expression.constant); }
	if (expression.type == ExpressionTypeIdentifier) { printf("%s", expression.identifier); }
	if (expression.type == ExpressionTypeVectorLiteral) {
		printf("(");
		for (int32_t i = 0; i < ListLength(expression.vector); i++) {
			PrintExpression(expression.vector[i]);
			if (i < ListLength(expression.vector) - 1) { printf(", "); }
		}
		printf(")");
	}
	if (expression.type == ExpressionTypeArrayLiteral) {
		printf("[");
		for (int32_t i = 0; i < ListLength(expression.array); i++) {
			PrintExpression(expression.array[i]);
			if (i < ListLength(expression.array) - 1) { printf(", "); }
		}
		printf("]");
	}
	if (expression.type == ExpressionTypeArguments) {
		for (int32_t i = 0; i < ListLength(expression.arguments); i++) {
			PrintExpression(expression.arguments[i]);
			if (i < ListLength(expression.array) - 1) { printf(", "); }
		}
	}
	if (expression.type == ExpressionTypeForAssignment) {
		printf("%s = ", expression.assignment.identifier);
		PrintExpression(*expression.assignment.expression);
	}
	if (expression.type == ExpressionTypeUnary) {
		if (IsPrefix(expression.unary.operator)) {
			printf("%s(", operators[expression.unary.operator]);
			PrintExpression(*expression.unary.expression);
			printf(")");
		} else {
			printf("(");
			PrintExpression(*expression.unary.expression);
			printf(")%s", operators[expression.unary.operator]);
		}
	}
	if (expression.type == ExpressionTypeBinary) {
		if (expression.binary.operator == OperatorCallStart) {
			PrintExpression(*expression.binary.left);
			printf("(");
			PrintExpression(*expression.binary.right);
			printf(")");
		} else if (expression.binary.operator == OperatorIndexStart) {
			PrintExpression(*expression.binary.left);
			PrintExpression(*expression.binary.right);
		} else {
			printf("(");
			PrintExpression(*expression.binary.left);
			printf(") %s (", operators[expression.binary.operator]);
			PrintExpression(*expression.binary.right);
			printf(")");
		}
	}
	if (expression.type == ExpressionTypeTernary) {
		printf("(");
		PrintExpression(*expression.ternary.left);
		printf(") %s (", operators[expression.ternary.leftOperator]);
		PrintExpression(*expression.ternary.middle);
		printf(") %s (", operators[expression.ternary.rightOperator]);
		PrintExpression(*expression.ternary.right);
		printf(")");
	}
}

void FreeExpression(Expression expression) {
	if (expression.type == ExpressionTypeIdentifier) { StringFree(expression.identifier); }
	if (expression.type == ExpressionTypeVectorLiteral || expression.type == ExpressionTypeArguments || expression.type == ExpressionTypeArrayLiteral) {
		for (int32_t i = 0; i < ListLength(expression.array); i++) { FreeExpression(expression.array[i]); }
		ListFree(expression.array);
	}
	if (expression.type == ExpressionTypeForAssignment) {
		StringFree(expression.assignment.identifier);
		if (expression.assignment.expression != NULL) { FreeExpression(*expression.assignment.expression); }
		free(expression.assignment.expression);
	}
	if (expression.type == ExpressionTypeUnary) {
		if (expression.unary.expression != NULL) { FreeExpression(*expression.unary.expression); }
		free(expression.unary.expression);
	}
	if (expression.type == ExpressionTypeBinary) {
		if (expression.binary.left != NULL) { FreeExpression(*expression.binary.left); }
		if (expression.binary.right != NULL) { FreeExpression(*expression.binary.right); }
		free(expression.binary.left);
		free(expression.binary.right);
	}
	if (expression.type == ExpressionTypeTernary) {
		if (expression.ternary.left != NULL) { FreeExpression(*expression.ternary.left); }
		if (expression.ternary.middle != NULL) { FreeExpression(*expression.ternary.middle); }
		if (expression.ternary.right != NULL) { FreeExpression(*expression.ternary.right); }
		free(expression.ternary.left);
		free(expression.ternary.middle);
		free(expression.ternary.right);
	}
}

static const char * declarationAttributes[] = { KEYWORD_POINTS, KEYWORD_PARAMETRIC, KEYWORD_POLYGONS };

static bool IsDeclarationttribute(Token token) {
	if (token.type == TokenTypeKeyword) {
		for (int32_t i = 0; i < sizeof(declarationAttributes) / sizeof(declarationAttributes[0]); i++) {
			if (StringEquals(token.value, declarationAttributes[i])) { return true; }
		}
	}
	return false;
}

static StatementType DetermineStatementType(list(Token) tokens) {
	if (ListLength(tokens) == 0) { return StatementTypeNone; }
	
	// check if first token is a declaration attribute
	int32_t start = 0;
	if (IsDeclarationttribute(tokens[start])) { start++; }
	if (start >= ListLength(tokens) - 1) { return StatementTypeNone; }
	
	// check if it's a variable or function declaration
	if (tokens[start].type == TokenTypeIdentifier) {
		// if second token is a '(' then it must be a function
		if (start + 1 < ListLength(tokens) && StringEquals(tokens[start + 1].value, SYMBOL_LEFT_PARENTHESIS)) {
			int32_t index = FindClosingParenthesis(tokens, start + 1, ListLength(tokens) - 1);
			if (index > -1 && index != ListLength(tokens) - 1 && tokens[index + 1].value[0] == '=') { return StatementTypeFunction; }
		}
		// if second token is a '=' then it must be a variable
		if (StringEquals(tokens[start + 1].value, SYMBOL_EQUAL)) { return StatementTypeVariable; }
		return StatementTypeNone;
	}
	
	return StatementTypeNone;
}

static int32_t ParseDeclarationAttribute(list(Token) tokens, Declaration * declaration) {
	if (IsDeclarationttribute(tokens[0])) {
		if (StringEquals(tokens[0].value, KEYWORD_POINTS)) { declaration->attribute = DeclarationAttributePoints; }
		if (StringEquals(tokens[0].value, KEYWORD_PARAMETRIC)) { declaration->attribute = DeclarationAttributeParametric; }
		if (StringEquals(tokens[0].value, KEYWORD_POLYGONS)) { declaration->attribute = DeclarationAttributePolygons; }
		return 1;
	}
	return 0;
}

static SyntaxError ParseVariableDeclaration(list(Token) tokens, Declaration * declaration, int32_t * end) {
	int32_t start = ParseDeclarationAttribute(tokens, declaration);
	declaration->identifier = StringCreate(tokens[start].value);
	*end = start + 1;
	return (SyntaxError){ SyntaxErrorCodeNone };
}

static SyntaxError ParseFunctionDeclaration(list(Token) tokens, Declaration * declaration, int32_t * end) {
	int32_t start = ParseDeclarationAttribute(tokens, declaration);
	int32_t argumentsEnd = FindClosingParenthesis(tokens, start + 1, ListLength(tokens) - 1);
	
	// go through each identifier and add it to the argument list
	declaration->parameters = ListCreate(sizeof(String), 4);
	for (int32_t i = start + 2; i < argumentsEnd; i++) {
		if (tokens[i].type != TokenTypeIdentifier) { return (SyntaxError){ SyntaxErrorCodeInvalidFunctionDeclaration, i, i }; }
		declaration->parameters = ListPush(declaration->parameters, &(String){ StringCreate(tokens[i].value) });
		i++;
		if (i == argumentsEnd) { break; }
		if (!StringEquals(tokens[i].value, SYMBOL_COMMA)) { return (SyntaxError){ SyntaxErrorCodeInvalidFunctionDeclaration, i, i }; }
	}
	
	declaration->identifier = StringCreate(tokens[start].value);
	*end = argumentsEnd + 1;
	return (SyntaxError){ SyntaxErrorCodeNone };
}

SyntaxError ParseStatement(list(Token) tokens, Statement * statement) {
	*statement = (Statement){ 0 };
	
	// check for any unknown tokens
	for (int32_t i = 0; i < ListLength(tokens); i++) {
		if (tokens[i].type == TokenTypeUnknown) { return (SyntaxError){ SyntaxErrorCodeUnknownToken, i, i }; }
	}
	
	// parse statement declaration
	statement->type = DetermineStatementType(tokens);
	int32_t declarationEnd = -1;
	SyntaxError error = (SyntaxError){ SyntaxErrorCodeNone };
	switch (statement->type) {
		case StatementTypeVariable:
			error = ParseVariableDeclaration(tokens, &statement->declaration, &declarationEnd);
			break;
		case StatementTypeFunction:
			error = ParseFunctionDeclaration(tokens, &statement->declaration, &declarationEnd);
			break;
		default: break;
	}
	if (error.code != SyntaxErrorCodeNone) {
		FreeStatement(*statement);
		return error;
	}
	
	// parse the expression
	int32_t start = declarationEnd + 1;
	int32_t end = ListLength(tokens) - 1;
	error = ParseExpression(tokens, start, end, &statement->expression);
	if (error.code != SyntaxErrorCodeNone) { FreeStatement(*statement); }
	return error;
}

void FreeStatement(Statement statement) {
	FreeExpression(statement.expression);
	if (statement.type == StatementTypeVariable) {
		StringFree(statement.declaration.identifier);
	}
	if (statement.type == StatementTypeFunction) {
		StringFree(statement.declaration.identifier);
		for (int32_t i = 0; i < ListLength(statement.declaration.parameters); i++) { StringFree(statement.declaration.parameters[i]); }
		ListFree(statement.declaration.parameters);
	}
}
