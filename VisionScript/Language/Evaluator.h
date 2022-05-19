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
	RuntimeErrorCodeInvalidIfPlacement,
	RuntimeErrorCodeInvalidElsePlacement,
	RuntimeErrorCodeInvalidWhenPlacement,
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
	int32_t start;
	int32_t end;
	int32_t line;
} RuntimeError;

const char * RuntimeErrorToString(RuntimeErrorCode code);
void PrintRuntimeError(RuntimeError error, list(String) lines);

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
Equation * GetEnvironmentEquation(Environment * environment, const char * identifier);
VectorArray * GetEnvironmentCache(Environment * environment, const char * identifier);
void InitializeEnvironmentDependents(Environment * environment);
void FreeEnvironment(Environment environment);

RuntimeError EvaluateExpression(Environment * environment, list(Binding) parameters, Expression expression, VectorArray * result);
void FindExpressionParents(Environment environment, Expression expression, list(String) parameters, list(String) * identifiers);

#endif
