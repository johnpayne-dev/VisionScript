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
	RuntimeErrorInvalidRenderDimensionality,
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
void PrintRuntimeError(RuntimeError error, Statement * statement);

typedef struct VectorArray
{
	scalar_t * xyzw[4];
	uint32_t dimensions;
	uint32_t length;
} VectorArray;

void PrintVectorArray(VectorArray value);
VectorArray CopyVectorArray(VectorArray value, int32_t * indices, int32_t indexCount);
void FreeVectorArray(VectorArray value);

typedef struct Parameter
{
	String identifier;
	Expression * expression;
	list(struct Parameter) parameters;
	bool cached;
	VectorArray cache;
} Parameter;

RuntimeError CreateParameter(HashMap identifiers, HashMap cache, String identifier, Expression * expression, list(Parameter) parameters, bool shouldCache, Parameter * parameter);
RuntimeError EvaluateParameterSize(HashMap identifiers, HashMap cache, Parameter parameter, uint32_t * length, uint32_t * dimensions);
RuntimeError EvaluateParameter(HashMap identifiers, HashMap cache, Parameter parameter, int32_t * indices, int32_t indexCount, VectorArray * result);
void FreeParameter(Parameter parameter);

RuntimeError EvaluateExpressionSize(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, uint32_t * length, uint32_t * dimensions);
RuntimeError EvaluateExpressionIndices(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, int32_t * indices, int32_t indexCount, VectorArray * result);
RuntimeError EvaluateExpression(HashMap identifiers, HashMap cache, list(Parameter) parameters, Expression * expression, VectorArray * result);

list(String) FindExpressionParents(HashMap identifiers, Expression * expression, list(String) parameters);

#endif
