#ifndef Parser_h
#define Parser_h

#include "Utilities/List.h"
#include "Utilities/String.h"
#include "Tokenizer.h"

typedef enum SyntaxErrorCode {
	SyntaxErrorCodeNone,
	SyntaxErrorCodeUnknownToken,
	SyntaxErrorCodeInvalidFunctionDeclaration,
	SyntaxErrorCodeMissingExpression,
	SyntaxErrorCodeMissingClosingBrace,
	SyntaxErrorCodeMismatchedBrace,
	SyntaxErrorCodeNonsenseExpression,
	SyntaxErrorCodeUnreadableConstant,
	SyntaxErrorCodeTooManyVectorElements,
	SyntaxErrorCodeInvalidUnaryPlacement,
	SyntaxErrorCodeInvalidTernaryPlacement,
} SyntaxErrorCode;

typedef struct SyntaxError {
	SyntaxErrorCode code;
	int32_t tokenStart;
	int32_t tokenEnd;
} SyntaxError;

const char * SyntaxErrorCodeToString(SyntaxErrorCode code);

typedef struct ForAssignment {
	String identifier;
	struct Expression * expression;
} ForAssignment;

typedef enum Operator {
	OperatorAdd,
	OperatorSubtract,
	OperatorMultiply,
	OperatorDivide,
	OperatorModulo,
	OperatorPower,
	OperatorEqual,
	OperatorNotEqual,
	OperatorGreater,
	OperatorGreaterEqual,
	OperatorLess,
	OperatorLessEqual,
	OperatorRange,
	OperatorFor,
	OperatorWhen,
	OperatorDimension,
	OperatorIndexStart,
	OperatorIndexEnd,
	OperatorCallStart,
	OperatorCallEnd,
	OperatorNegate,
	OperatorFactorial,
	OperatorNot,
	OperatorIf,
	OperatorElse,
	OperatorCount,
} Operator;

typedef struct Unary {
	Operator operator;
	struct Expression * expression;
} Unary;

typedef struct Binary {
	Operator operator;
	struct Expression * left;
	struct Expression * right;
} Binary;

typedef struct Ternary {
	Operator leftOperator;
	Operator rightOperator;
	struct Expression * left;
	struct Expression * middle;
	struct Expression * right;
} Ternary;

typedef enum ExpressionType {
	ExpressionTypeUnknown,
	ExpressionTypeConstant,
	ExpressionTypeIdentifier,
	ExpressionTypeVectorLiteral,
	ExpressionTypeArrayLiteral,
	ExpressionTypeArguments,
	ExpressionTypeForAssignment,
	ExpressionTypeUnary,
	ExpressionTypeBinary,
	ExpressionTypeTernary,
} ExpressionType;

typedef struct Expression {
	ExpressionType type;
	union {
		double constant;
		String identifier;
		list(struct Expression) vector;
		list(struct Expression) array;
		list(struct Expression) arguments;
		ForAssignment assignment;
		Unary unary;
		Binary binary;
		Ternary ternary;
	};
} Expression;

SyntaxError ParseExpression(list(Token) tokens, int32_t start, int32_t end, Expression * expression);
void PrintExpression(Expression expression);
void FreeExpression(Expression expression);

typedef enum DeclarationAttribute {
	DeclarationAttributeNone,
	DeclarationAttributePoints,
	DeclarationAttributeParametric,
	DeclarationAttributePolygons,
} DeclarationAttribute;

typedef struct Declaration {
	String identifier;
	list(String) parameters;
	DeclarationAttribute attribute;
} Declaration;

typedef enum StatementType {
	StatementTypeNone,
	StatementTypeVariable,
	StatementTypeFunction,
} StatementType;

typedef struct Statement {
	StatementType type;
	Declaration declaration;
	Expression expression;
} Statement;

SyntaxError ParseStatement(list(Token) tokens, Statement * statement);
void FreeStatement(Statement statement);

#endif
