#ifndef Evaluator_h
#define Evaluator_h

#include "Utilities/String.h"
#include "Utilities/List.h"
#include "Utilities/HashMap.h"
#include "Parser.h"

#define MAX_ARRAY_LENGTH (1 << 24)

typedef enum RuntimeErrorCode
{
	RuntimeErrorNone,
	RuntimeErrorUndefinedIdentifier,
	RuntimeErrorVectorInsideVector,
	RuntimeErrorNonUniformArray,
	RuntimeErrorArrayInsideArray,
	RuntimeErrorArrayTooLarge,
	RuntimeErrorInvalidArrayRange,
	RuntimeErrorIdentifierNotVariable,
	RuntimeErrorIdentifierNotFunction,
	RuntimeErrorIncorrectParameterCount,
	RuntimeErrorInvalidArgumentType,
	RuntimeErrorInvalidEllipsisOperon,
	RuntimeErrorIndexingWithVector,
	RuntimeErrorDifferingLengthVectors,
	RuntimeErrorInvalidSwizzling,
	RuntimeErrorNotImplemented,
} RuntimeErrorCode;

typedef struct RuntimeError
{
	RuntimeErrorCode code;
	int32_t tokenStart;
	int32_t tokenEnd;
	Statement * statement;
} RuntimeError;

const char * RuntimeErrorToString(RuntimeErrorCode code);

typedef struct VectorArray
{
	scalar_t * xyzw[4];
	uint8_t dimensions;
	uint32_t length;
} VectorArray;

void PrintVectorArray(VectorArray value);
void FreeVectorArray(VectorArray value);

typedef struct Parameter
{
	String identifier;
	VectorArray value;
} Parameter;

RuntimeError EvaluateExpression(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, VectorArray * result);

#endif
