#ifndef Script_h
#define Script_h

#include "Tokenizer.h"
#include "Parser.h"
#include "Utilities/HashMap.h"

typedef enum RuntimeError
{
	RuntimeErrorNone,
	RuntimeErrorUndefinedIdentifier,
	RuntimeErrorVectorInsideVector,
	RuntimeErrorNonUniformArray,
	RuntimeErrorArrayInsideArray,
	RuntimeErrorIdentifierNotVariable,
	RuntimeErrorIdentifierNotFunction,
	RuntimeErrorInvalidEllipsisOperon,
	RuntimeErrorIndexingWithVector,
	RuntimeErrorDifferingLengthVectors,
	RuntimeErrorNotImplemented,
} RuntimeError;

typedef struct VectorArray
{
	float * xyzw[4];
	uint8_t dimensions;
	uint32_t length;
} VectorArray;

typedef struct Script
{
	const char * code;
	list(TokenStatement) tokenStatements;
	list(Statement *) identifierList;
	list(Statement *) renderList;
	list(Statement *) errorList;
	HashMap identifiers;
} Script;

Script * LoadScript(const char * code);

RuntimeError EvaluateExpression(Script * script, Statement * statement, list(VectorArray) arguments, Expression * expression, VectorArray * result);

void DestroyScript(Script * script);

#endif
