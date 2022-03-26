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

typedef struct ForAssignment
{
	String identifier;
	struct Expression * expression;
} ForAssignment;

typedef union Operon
{
	struct Expression * expression;
	list(struct Expression *) expressions;
	String identifier;
	scalar_t constant;
	ForAssignment assignment;
} Operon;

typedef struct Expression
{
	Operator operator;
	OperonType operonTypes[2];
	Operon operons[2];
	int32_t operonStart[2];
	int32_t operonEnd[2];
} Expression;

typedef enum SyntaxErrorCode
{
	SyntaxErrorNone,
	SyntaxErrorUnknownToken,
	SyntaxErrorUnknownStatementType,
	SyntaxErrorInvalidVariableDeclaration,
	SyntaxErrorInvalidFunctionDeclaration,
	SyntaxErrorMissingExpression,
	SyntaxErrorMissingBracket,
	SyntaxErrorNonsenseExpression,
	SyntaxErrorMissingOperon,
	SyntaxErrorTooManyVectorElements,
	SyntaxErrorInvalidCommaPlacement,
	SyntaxErrorInvalidForPlacement,
	SyntaxErrorInvalidForAssignment,
	SyntaxErrorInvalidForAssignmentPlacement,
	SyntaxErrorInvalidEllipsisPlacement,
	SyntaxErrorInvalidEllipsisOperon,
	SyntaxErrorIndexingWithVector,
	SyntaxErrorInvalidFunctionCall,
} SyntaxErrorCode;

typedef struct SyntaxError
{
	SyntaxErrorCode code;
	int32_t tokenStart;
	int32_t tokenEnd;
} SyntaxError;

const char * SyntaxErrorToString(SyntaxErrorCode code);

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
	list(Token) tokens;
	SyntaxError error;
	StatementType type;
	StatementDeclaration declaration;
	Expression * expression;
} Statement;

Statement * ParseTokenLine(list(Token) tokens);
void FreeStatement(Statement * statement);

#endif
