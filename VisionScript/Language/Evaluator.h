#ifndef Evaluator_h
#define Evaluator_h

#include "Parser.h"
#include "Utilities/HashMap.h"

typedef enum RuntimeErrorCode {
	RuntimeErrorCodeNone,
	RuntimeErrorCodeInvalidExpression,
	RuntimeErrorCodeInvalidArgumentsPlacement,
	RuntimeErrorCodeInvalidForAssignmentPlacement,
	RuntimeErrorCodeIdentifierNotVariable,
	RuntimeErrorCodeUndefinedIdentifier,
	RuntimeErrorCodeTooManyVectorElements,
	RuntimeErrorCodeVectorInsideVector,
	RuntimeErrorCodeNonUniformArray,
	RuntimeErrorCodeArrayInsideArray,
	RuntimeErrorCodeInvalidRangeOperon,
	RuntimeErrorCodeNonUniformRange,
	RuntimeErrorCodeInvalidRangePlacement,
	RuntimeErrorCodeInvalidForPlacement,
	RuntimeErrorCodeMissingForAssignment,
	RuntimeErrorCodeInvalidDimensionOperon,
	RuntimeErrorCodeInvalidSwizzling,
	RuntimeErrorCodeInvalidIndexDimension,
	RuntimeErrorCodeUncallableExpression,
	RuntimeErrorCodeIdentifierNotFunction,
	RuntimeErrorCodeInvalidArgumentsExpression,
	RuntimeErrorCodeIncorrectArgumentCount,
	RuntimeErrorCodeDifferingOperonDimensions,
	RuntimeErrorCodeNotImplemented,
} RuntimeErrorCode;

typedef struct RuntimeError {
	RuntimeErrorCode code;
	int32_t tokenStart;
	int32_t tokenEnd;
	Equation * equation;
} RuntimeError;

const char * RuntimeErrorToString(RuntimeErrorCode code);
void PrintRuntimeError(RuntimeError error);

typedef float scalar_t;

typedef struct VectorArray {
	scalar_t * xyzw[4];
	uint32_t dimensions;
	uint32_t length;
} VectorArray;

void PrintVectorArray(VectorArray value);
VectorArray CopyVectorArray(VectorArray value);
bool TruthyVectorArray(VectorArray value);
void FreeVectorArray(VectorArray value);

typedef struct Binding {
	String identifier;
	VectorArray value;
} Binding;

Binding CreateBinding(const char * identifier, VectorArray value);
void FreeBinding(Binding binding);

typedef struct Environment {
	list(Equation) equations;
	list(Binding) cache;
} Environment;

Environment CreateEmptyEnvironment(void);
void SetEnvironmentEquation(Environment * environment, Equation equation);
void SetEnvironmentCache(Environment * environment, Binding binding);
void FreeEnvironment(Environment environment);

RuntimeError EvaluateExpression(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result);

#endif
