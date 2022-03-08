#ifndef Evaluator_h
#define Evaluator_h

#include "Utilities/String.h"
#include "Utilities/List.h"
#include "Utilities/HashMap.h"
#include "Parser.h"

typedef enum RuntimeError
{
	RuntimeErrorNone,
	RuntimeErrorUndefinedIdentifier,
	RuntimeErrorVectorInsideVector,
	RuntimeErrorNonUniformArray,
	RuntimeErrorArrayInsideArray,
	RuntimeErrorIdentifierNotVariable,
	RuntimeErrorIdentifierNotFunction,
	RuntimeErrorIncorrectParameterCount,
	RuntimeErrorInvalidEllipsisOperon,
	RuntimeErrorIndexingWithVector,
	RuntimeErrorDifferingLengthVectors,
	RuntimeErrorInvalidSwizzling,
	RuntimeErrorNotImplemented,
} RuntimeError;

typedef struct VectorArray
{
	scalar_t * xyzw[4];
	uint8_t dimensions;
	uint32_t length;
} VectorArray;

typedef struct Parameter
{
	String identifier;
	VectorArray value;
} Parameter;

RuntimeError EvaluateExpression(HashMap identifiers, list(Parameter) parameters, Expression * expression, VectorArray * result);

#endif
