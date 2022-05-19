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
	SyntaxErrorCodeMissingOpeningBrace,
	SyntaxErrorCodeNonsenseExpression,
	SyntaxErrorCodeUnreadableConstant,
	SyntaxErrorCodeInvalidUnaryPlacement,
	SyntaxErrorCodeInvalidTernaryPlacement,
} SyntaxErrorCode;

typedef struct SyntaxError {
	SyntaxErrorCode code;
	int32_t start;
	int32_t end;
	int32_t line;
} SyntaxError;

const char * SyntaxErrorCodeToString(SyntaxErrorCode code);
void PrintSyntaxError(SyntaxError error, list(String) lines);

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
		list(struct Expression) list;
		ForAssignment assignment;
		Unary unary;
		Binary binary;
		Ternary ternary;
	};
	int32_t start;
	int32_t end;
	int32_t line;
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

typedef enum EquationType {
	EquationTypeNone,
	EquationTypeVariable,
	EquationTypeFunction,
} EquationType;

typedef struct Equation {
	EquationType type;
	Declaration declaration;
	Expression expression;
	list(struct Equation *) dependents;
} Equation;

SyntaxError ParseEquation(list(Token) tokens, Equation * equation);
void FreeEquation(Equation equation);

#endif
