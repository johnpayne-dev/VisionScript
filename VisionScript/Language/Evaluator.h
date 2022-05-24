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
	RuntimeErrorCodeInvalidArgumentType,
	RuntimeErrorCodeInvalidRenderDimensionality,
	RuntimeErrorCodeInvalidParametricEquation,
	RuntimeErrorCodeInvalidParametricDomain,
	RuntimeErrorCodeNotImplemented,
} RuntimeErrorCode;

typedef struct RuntimeError {
	RuntimeErrorCode code;
	int32_t start;
	int32_t end;
	int32_t line;
} RuntimeError;

const char * RuntimeErrorToString(RuntimeErrorCode code);
void PrintRuntimeError(RuntimeError error, List(String) lines);

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
	HashMap(Equation) equations;
	HashMap(VectorArray) cache;
	HashMap(List(Equation)) dependents;
} Environment;

Environment CreateEmptyEnvironment(void);
void AddEnvironmentEquation(Environment * environment, Equation equation);
void SetEnvironmentCache(Environment * environment, const char * identifier, VectorArray value);
Equation * GetEnvironmentEquation(Environment * environment, const char * identifier);
VectorArray * GetEnvironmentCache(Environment * environment, const char * identifier);
void InitializeEnvironmentDependents(Environment * environment);
void FreeEnvironment(Environment environment);

RuntimeError EvaluateExpression(Environment * environment, List(Binding) parameters, Expression expression, VectorArray * result);
void FindExpressionParents(Environment environment, Expression expression, List(String) parameters, List(String) * identifiers);

#endif
