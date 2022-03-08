#ifndef Parser_h
#define Parser_h

#include "Utilities/List.h"
#include "Tokenizer.h"

typedef float scalar_t;

typedef enum Operator
{
	OperatorNone,
	OperatorFor, OperatorEllipsis,
	OperatorAdd, OperatorSubtract,
	OperatorMultiply, OperatorDivide, OperatorModulo,
	OperatorPositive, OperatorNegative,
	OperatorPower,
	OperatorIndex,
	OperatorFunctionCall,
} Operator;

typedef enum OperonType
{
	OperonTypeNone,
	OperonTypeExpression,
	OperonTypeIdentifier,
	OperonTypeArguments,
	OperonTypeConstant,
	OperonTypeArrayLiteral,
	OperonTypeVectorLiteral,
	OperonTypeForAssignment
} OperonType;

typedef union Operon
{
	struct Expression * expression;
	list(struct Expression *) expressions;
	String identifier;
	scalar_t constant;
	struct ForAssignment
	{
		String identifier;
		struct Expression * expression;
	} forAssignment;
} Operon;

typedef struct Expression
{
	Operator operator;
	OperonType operonTypes[2];
	Operon operons[2];
} Expression;

typedef enum SyntaxError
{
	SyntaxErrorNone,
	SyntaxErrorUnknownToken,
	SyntaxErrorUnknownStatementType,
	SyntaxErrorInvalidVariableDeclaration,
	SyntaxErrorInvalidFunctionDeclaration,
	SyntaxErrorMissingExpression,
	SyntaxErrorMissingBracket,
	SyntaxErrorMissingOperator,
	SyntaxErrorTooManyVectorElements,
	SyntaxErrorInvalidCommaPlacement,
	SyntaxErrorInvalidForPlacement,
	SyntaxErrorInvalidForAssignment,
	SyntaxErrorInvalidForAssignmentPlacement,
	SyntaxErrorInvalidEllipsisPlacement,
	SyntaxErrorInvalidEllipsisOperon,
	SyntaxErrorIndexingConstant,
	SyntaxErrorIndexingWithConstant,
	SyntaxErrorIndexingWithVector,
	SyntaxErrorInvalidFunctionCall,
} SyntaxError;

typedef enum StatementType
{
	StatementTypeUnknown,
	StatementTypeFunction,
	StatementTypeVariable,
	StatementTypeRender,
} StatementType;

typedef enum StatementRenderType
{
	StatementRenderTypePoint,
	StatementRenderTypeParametric,
	StatementRenderTypePolygon,
} StatementRenderType;

typedef union StatementDeclaration
{
	struct { String identifier; } variable;
	struct { String identifier; list(String) arguments; } function;
	struct { StatementRenderType type; } render;
} StatementDeclaration;

typedef struct Statement
{
	SyntaxError error;
	StatementType type;
	StatementDeclaration declaration;
	Expression * expression;
} Statement;

Statement * ParseTokenLine(list(Token) tokens);
SyntaxError ParseExpression(list(Token) tokens, int32_t start, int32_t end, Expression ** expression);
void DestroyStatement(Statement * statement);

#endif
