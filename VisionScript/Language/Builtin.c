#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Builtin.h"

static const char * builtinFunctions[] =
{
	"sin", "cos", "tan", "asin", "acos", "atan", "atan2",
	"sec", "csc", "cot", "asec", "acsc", "acot",
	"sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
	"sech", "csch", "coth", "asech", "acsch", "acoth",
	
	"abs", "argmax", "argmin", "cbrt", "ceil", "corr",
	"count", "cov", "erf", "exp", "factorial", "floor",
	"gamma", "interleave", "join", "ln", "log", "log10", "log2",
	"max", "mean", "median", "min", "prod", "quantile",
	"rand", "round", "shuffle", "sign", "sort", "sqrt",
	"stdev", "sum", "var",
	
	"cross", "dist", "distsq", "dot", "length", "lengthsq", "normalize",
};

static BuiltinFunction multiArgumentBuiltins[] =
{
	BuiltinFunctionATAN2,
	BuiltinFunctionCORR,
	BuiltinFunctionCOUNT,
	BuiltinFunctionCOV,
	BuiltinFunctionINTERLEAVE,
	BuiltinFunctionJOIN,
	BuiltinFunctionLOG,
	BuiltinFunctionMAX,
	BuiltinFunctionMIN,
	BuiltinFunctionQUANTILE,
	BuiltinFunctionRAND,
	BuiltinFunctionSHUFFLE,
	BuiltinFunctionSORT,
	BuiltinFunctionCROSS,
	BuiltinFunctionDIST,
	BuiltinFunctionDISTSQ,
	BuiltinFunctionDOT,
};

static BuiltinFunction unindexableBuiltins[] =
{
	BuiltinFunctionARGMAX,
	BuiltinFunctionARGMIN,
	BuiltinFunctionCORR,
	BuiltinFunctionCOUNT,
	BuiltinFunctionCOV,
	BuiltinFunctionINTERLEAVE,
	BuiltinFunctionJOIN,
	BuiltinFunctionMAX,
	BuiltinFunctionMEAN,
	BuiltinFunctionMEDIAN,
	BuiltinFunctionMIN,
	BuiltinFunctionPROD,
	BuiltinFunctionQUANTILE,
	BuiltinFunctionRAND,
	BuiltinFunctionSHUFFLE,
	BuiltinFunctionSORT,
	BuiltinFunctionSTDEV,
	BuiltinFunctionSUM,
	BuiltinFunctionVAR,
};

static int compare(const void * a, const void * b)
{
	// used for list sorting
	return (*(scalar_t *)a - *(scalar_t *)b > 0) - (*(scalar_t *)a - *(scalar_t *)b < 0);
}

static int coupled_compare(const void * a, const void * b)
{
	// used for list sorting relative to another list
	struct { int32_t i; scalar_t s; } * x = (void *)a;
	struct { int32_t i; scalar_t s; } * y = (void *)b;
	return (x->s - y->s > 0) - (x->s - y->s < 0);
}

static RuntimeErrorCode _sin(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = sinf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _cos(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = cosf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _tan(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tanf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _asin(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = asinf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _acos(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = acosf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _atan(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = atanf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _atan2_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	// atan2 takes two non-vector arguments
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions > 1 || args[1].dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
	*length = args[0].length < args[1].length ? args[0].length : args[1].length;
	if (args[0].length == 1 && args[1].length > 1) { *length = args[1].length; }
	if (args[1].length == 1 && args[0].length > 1) { *length = args[0].length; }
	*dimensions = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _atan2(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _atan2_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	// length 1 argument will be extended to length of other argument
	VectorArray y = args[0], x = args[1];
	bool yi = y.length == 1 && x.length > 1;
	bool xi = x.length == 1 && y.length > 1;
	
	result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
	for (int32_t i = 0; i < result->length; i++) { result->xyzw[0][i] = atan2f(y.xyzw[0][yi ? 0 : i], x.xyzw[0][xi ? 0 : i]); }
	
	return RuntimeErrorNone;
}

static RuntimeErrorCode _sec(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / cosf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _csc(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / sinf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _cot(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / tanf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _asec(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = acosf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _acsc(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = asinf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _acot(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = M_PI_2 - atanf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _sinh(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = sinhf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _cosh(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = coshf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _tanh(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tanhf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _asinh(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = asinhf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _acosh(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = acoshf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _atanh(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = atanhf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _sech(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / coshf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _csch(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / sinhf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _coth(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / tanhf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _asech(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = acoshf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _acsch(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = asinhf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _acoth(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = atanhf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _abs(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = fabsf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _argmax_size(uint32_t * length, uint32_t * dimensions)
{
	// argmax takes 1 non-vector argument
	if (*dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
	*length = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _argmax(VectorArray * result)
{
	RuntimeErrorCode error = _argmax_size(&result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	scalar_t max = result->xyzw[0][0];
	int32_t index = 0;
	for (int32_t i = 1; i < result->length; i++)
	{
		if (result->xyzw[0][i] > max) { max = result->xyzw[0][i]; index = i; }
	}
	free(result->xyzw[0]);
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = (scalar_t)index;
	return RuntimeErrorNone;
}

static RuntimeErrorCode _argmin_size(uint32_t * length, uint32_t * dimensions)
{
	// argmax takes 1 non-vector argument
	if (*dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
	*length = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _argmin(VectorArray * result)
{
	RuntimeErrorCode error = _argmin_size(&result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	scalar_t min = result->xyzw[0][0];
	int32_t index = 0;
	for (int32_t i = 1; i < result->length; i++)
	{
		if (result->xyzw[0][i] < min) { min = result->xyzw[0][i]; index = i; }
	}
	free(result->xyzw[0]);
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = index;
	return RuntimeErrorNone;
}

static RuntimeErrorCode _cbrt(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = cbrtf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _ceil(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = ceilf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _corr_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	// corr takes two arguments of same dimensionality
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorInvalidArgumentType; }
	*length = 1;
	*dimensions = args[0].dimensions;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _corr(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _corr_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	VectorArray a = args[0], b = args[1];
	int32_t length = a.length < b.length ? a.length : b.length;
	for (int8_t d = 0; d < a.dimensions; d++)
	{
		// calculate the means for both
		scalar_t avgA = 0.0, avgB = 0.0;
		for (int32_t i = 0; i < length; i++) { avgA += a.xyzw[d][i]; avgB += b.xyzw[d][i]; }
		avgA /= length;
		avgB /= length;
		
		// calculate the variance for both
		scalar_t varA = 0.0, varB = 0.0;
		for (int32_t i = 0; i < length; i++) { varA += (a.xyzw[d][i] - avgA) * (a.xyzw[d][i] - avgA); varB += (b.xyzw[d][i] - avgB) * (b.xyzw[d][i] - avgB); }
		
		// calculate final normalized sum
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < length; i++) { sum += (a.xyzw[d][i] - avgA) * (b.xyzw[d][i] - avgB); }
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sum / sqrtf(varA * varB);
	}
	return RuntimeErrorNone;
}

static RuntimeErrorCode _count_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	*length = 1;
	*dimensions = 1;
	if (ListLength(args) == 2)
	{
		if (args[1].dimensions != args[0].dimensions) { return RuntimeErrorInvalidArgumentType; }
		if (args[1].length > 1) { return RuntimeErrorInvalidArgumentType; }
	}
	return ListLength(args) != 1 && ListLength(args) != 2 ? RuntimeErrorIncorrectParameterCount : RuntimeErrorNone;
}
static RuntimeErrorCode _count(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _count_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	if (ListLength(args) == 1)
	{
		// returns length of the vector array passed
		int32_t len = args[0].length;
		result->xyzw[0] = malloc(sizeof(scalar_t));
		result->xyzw[0][0] = len;
		return RuntimeErrorNone;
	}
	if (ListLength(args) == 2)
	{
		// counts how many elements equal the second argument
		int32_t count = 0;
		for (int32_t i = 0; i < args[0].length; i++)
		{
			bool equal = true;
			for (int32_t d = 0; d < args[0].dimensions; d++)
			{
				if (args[0].xyzw[d][i] != args[1].xyzw[d][0]) { equal = false; break; }
			}
			if (equal) { count++; }
		}
		result->xyzw[0] = malloc(sizeof(scalar_t));
		result->xyzw[0][0] = count;
		return RuntimeErrorNone;
	}
	return RuntimeErrorIncorrectParameterCount;
}

static RuntimeErrorCode _cov_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	// cov takes two arguments of same dimensionality
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorInvalidArgumentType; }
	*length = 1;
	*dimensions = args[0].dimensions;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _cov(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _cov_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	VectorArray a = args[0], b = args[1];
	int32_t length = a.length < b.length ? a.length : b.length;
	for (int8_t d = 0; d < a.dimensions; d++)
	{
		// calculate means
		scalar_t avgA = 0.0, avgB = 0.0;
		for (int32_t i = 0; i < length; i++) { avgA += a.xyzw[d][i]; avgB += b.xyzw[d][i]; }
		avgA /= length;
		avgB /= length;
		
		// then calculate the covariance
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < length; i++) { sum += (a.xyzw[d][i] - avgA) * (b.xyzw[d][i] - avgB); }
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sum / (length - 1);
	}
	return RuntimeErrorNone;
}

static RuntimeErrorCode _erf(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = erff(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _exp(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = expf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _factorial(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tgammaf(result->xyzw[d][i] + 1.0); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _floor(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = floorf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _gamma(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tgammaf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _interleave_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	// takes n arguments of same dimensionality
	*dimensions = 0;
	*length = 0;
	for (int32_t i = 0; i < ListLength(args); i++)
	{
		// first argument determines dimensionality of whole array
		if (*dimensions == 0) { *dimensions = args[i].dimensions; }
		if (args[i].dimensions != *dimensions) { return RuntimeErrorInvalidArgumentType; }
		*length += args[i].length;
	}
	return RuntimeErrorNone;
}
static RuntimeErrorCode _interleave(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _interleave_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	// interleave the contents of each argument into a single array
	for (int32_t d = 0; d < result->dimensions; d++)
	{
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		int32_t * counters = calloc(ListLength(args), sizeof(int32_t));
		int32_t c = 0;
		while (c < result->length)
		{
			for (int32_t i = 0; i < ListLength(args); i++)
			{
				if (counters[i] < args[i].length) { result->xyzw[d][c++] = args[i].xyzw[d][counters[i]++]; }
			}
		}
		free(counters);
	}
	return RuntimeErrorNone;
}

static RuntimeErrorCode _join_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	// takes n arguments of same dimensionality
	*dimensions = 0;
	*length = 0;
	for (int32_t i = 0; i < ListLength(args); i++)
	{
		// first argument determines dimensionality of whole array
		if (*dimensions == 0) { *dimensions = args[i].dimensions; }
		if (args[i].dimensions != *dimensions) { return RuntimeErrorInvalidArgumentType; }
		*length += args[i].length;
	}
	return RuntimeErrorNone;
}
static RuntimeErrorCode _join(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _join_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	// copy the contents of each argument into a single array
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0, p = 0; i < ListLength(args); i++)
		{
			memcpy(result->xyzw[d] + p, args[i].xyzw[d], args[i].length * sizeof(scalar_t));
			p += args[i].length;
		}
	}
	return RuntimeErrorNone;
}

static RuntimeErrorCode _ln(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = logf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _log_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	// takes two arguments of same vector, (with exception to dimension 1)
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != args[1].dimensions && args[0].dimensions != 1 && args[1].dimensions != 1) { return RuntimeErrorInvalidArgumentType; }
	*length = args[1].length < args[0].length ? args[1].length : args[0].length;
	if (args[1].length == 1 && args[0].length > 1) { *length = args[0].length; }
	if (args[0].length == 1 && args[1].length > 1) { *length = args[1].length; }
	*dimensions = args[1].dimensions > args[0].dimensions ? args[1].dimensions : args[0].dimensions;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _log(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _log_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	// determine which arguments are scalars
	VectorArray b = args[0], a = args[1];
	bool ai = a.length == 1 && b.length > 1;
	bool bi = b.length == 1 && a.length > 1;
	bool ad = a.dimensions == 1 && b.dimensions > 1;
	bool bd = b.dimensions == 1 && a.dimensions > 1;
	
	// calculate the log
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0; i < result->length; i++)
		{
			result->xyzw[d][i] = logf(a.xyzw[ad ? 0 : d][ai ? 0 : i]) / logf(b.xyzw[bd ? 0 : d][bi ? 0 : i]);
		}
	}
	
	return RuntimeErrorNone;
}

static RuntimeErrorCode _log10(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = log10f(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _log2(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = log2f(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _max_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	*length = 1;
	*dimensions = 1;
	for (int32_t i = 0; i < ListLength(args); i++)
	{
		if (args[i].dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
	}
	return RuntimeErrorNone;
}
static RuntimeErrorCode _max(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _max_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	// takes n arguments all of dimension 1
	scalar_t max = args[0].xyzw[0][0];
	for (int32_t i = 0; i < ListLength(args); i++)
	{
		for (int32_t j = 0; j < args[i].length; j++) { if (args[i].xyzw[0][j] > max) { max = args[i].xyzw[0][j]; } }
	}
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = max;
	return RuntimeErrorNone;
}

static RuntimeErrorCode _mean_size(uint32_t * length, uint32_t * dimensions)
{
	*length = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _mean(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < result->length; i++) { sum += result->xyzw[d][i]; }
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sum / result->length;
	}
	result->length = 1;
	return RuntimeErrorNone;
}

static RuntimeErrorCode _median_size(uint32_t * length, uint32_t * dimensions)
{
	*length = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _median(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		// sorts the list then gets the element in the middle
		qsort(result->xyzw[d], result->length, sizeof(scalar_t), compare);
		scalar_t median = result->length % 2 == 1 ? result->xyzw[d][result->length / 2] : (result->xyzw[d][result->length / 2] + result->xyzw[d][result->length / 2 - 1]) / 2.0;
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = median;
	}
	result->length = 1;
	return RuntimeErrorNone;
}

static RuntimeErrorCode _min_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	*length = 1;
	*dimensions = 1;
	for (int32_t i = 0; i < ListLength(args); i++)
	{
		if (args[i].dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
	}
	return RuntimeErrorNone;
}
static RuntimeErrorCode _min(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _min_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	// takes n arguments all of dimension 1
	scalar_t min = args[0].xyzw[0][0];
	for (int32_t i = 0; i < ListLength(args); i++)
	{
		for (int32_t j = 0; j < args[i].length; j++) { if (args[i].xyzw[0][j] < min) { min = args[i].xyzw[0][j]; } }
	}
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = min;
	return RuntimeErrorNone;
}

static RuntimeErrorCode _prod_size(uint32_t * length, uint32_t * dimensions)
{
	*length = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _prod(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		scalar_t prod = 1.0;
		for (int32_t i = 0; i < result->length; i++) { prod *= result->xyzw[d][i]; }
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = prod;
	}
	result->length = 1;
	return RuntimeErrorNone;
}

static RuntimeErrorCode _quantile_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	// takes two arguments, second argument must not be a vector
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[1].dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
	*length = args[1].length;
	*dimensions = args[0].dimensions;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _quantile(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _quantile_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		// sort list and get each element at each given quantile
		qsort(args[0].xyzw[d], args[0].length, sizeof(scalar_t), compare);
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0; i < result->length; i++)
		{
			if (args[1].xyzw[0][i] < 0.0 || args[1].xyzw[0][i] > 1.0) { result->xyzw[d][i] = NAN; continue; }
			scalar_t index = args[1].xyzw[0][i] * (args[0].length - 1);
			int32_t a = floorf(index), b = ceilf(index);
			result->xyzw[d][i] = args[0].xyzw[d][a] * (1.0 - (index - a)) + args[0].xyzw[d][b] * (index - a);
		}
	}
	return RuntimeErrorNone;
}

static RuntimeErrorCode _rand_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	return RuntimeErrorNotImplemented;
}
static RuntimeErrorCode _rand(list(VectorArray) args, VectorArray * result)
{
	return RuntimeErrorNotImplemented;
}

static RuntimeErrorCode _round(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = roundf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _shuffle_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	return RuntimeErrorNotImplemented;
}
static RuntimeErrorCode _shuffle(list(VectorArray) args, VectorArray * result)
{
	return RuntimeErrorNotImplemented;
}

static RuntimeErrorCode _sign(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = (result->xyzw[d][i] > 0) - (result->xyzw[d][i] < 0); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _sort_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	if (ListLength(args) == 1)
	{
		*length = args[0].length;
		*dimensions = 1;
	}
	if (ListLength(args) == 2)
	{
		if (args[1].dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
		*length = args[0].length < args[1].length ? args[0].length : args[1].length;
		*dimensions = args[0].dimensions;
	}
	return RuntimeErrorIncorrectParameterCount;
}
static RuntimeErrorCode _sort(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _sort_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	// if one argument is passed then sort in ascending order
	if (ListLength(args) == 1)
	{
		for (int8_t d = 0; d < result->dimensions; d++)
		{
			result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
			memcpy(result->xyzw[d], args[0].xyzw[d], result->length * sizeof(scalar_t));
			qsort(result->xyzw[d], result->length, sizeof(scalar_t), compare);
		}
		return RuntimeErrorNone;
	}
	// if two arguments are passed then sort relative the second list
	if (ListLength(args) == 2)
	{
		// each index of the first list is coupled with the corresponding element of the second list
		struct { int32_t i; scalar_t s; } * coupled = malloc(result->length * (sizeof(int32_t) + sizeof(scalar_t)));
		for (int32_t i = 0; i < result->length; i++) { coupled[i].i = i; coupled[i].s = args[1].xyzw[0][i]; }
		qsort(coupled, result->length, sizeof(int32_t) + sizeof(scalar_t), coupled_compare);
		
		// rearrange the first list to be in the order of how the second list was sorted
		for (int8_t d = 0; d < result->dimensions; d++)
		{
			result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
			for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = args[0].xyzw[d][coupled[i].i]; }
		}
		return RuntimeErrorNone;
	}
	return RuntimeErrorIncorrectParameterCount;
}

static RuntimeErrorCode _sqrt(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = sqrtf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeErrorCode _stdev_size(uint32_t * length, uint32_t * dimensions)
{
	*length = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _stdev(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		// calculate the mean
		scalar_t avg = 0.0;
		for (int32_t i = 0; i < result->length; i++) { avg += result->xyzw[d][i]; }
		avg /= result->length;
		
		// sum the deviations
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < result->length; i++) { sum += (result->xyzw[d][i] - avg) * (result->xyzw[d][i] - avg); }
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sqrtf(sum / result->length);
	}
	result->length = 1;
	return RuntimeErrorNone;
}

static RuntimeErrorCode _sum_size(uint32_t * length, uint32_t * dimensions)
{
	*length = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _sum(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < result->length; i++) { sum += result->xyzw[d][i]; }
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sum;
	}
	result->length = 1;
	return RuntimeErrorNone;
}

static RuntimeErrorCode _var_size(uint32_t * length, uint32_t * dimensions)
{
	*length = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _var(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		// calculate the mean
		scalar_t avg = 0.0;
		for (int32_t i = 0; i < result->length; i++) { avg += result->xyzw[d][i]; }
		avg /= result->length;
		
		// sum the variances
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < result->length; i++) { sum += (result->xyzw[d][i] - avg) * (result->xyzw[d][i] - avg); }
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sum / result->length;
	}
	result->length = 1;
	return RuntimeErrorNone;
}

static RuntimeErrorCode _cross_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	// cross product works on two 3d vectors
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != 3 || args[1].dimensions != 3) { return RuntimeErrorInvalidArgumentType; }
	*length = args[0].length < args[1].length ? args[0].length : args[1].length;
	if (args[0].length == 1 && args[1].length > 1) { *length = args[1].length; }
	if (args[1].length == 1 && args[0].length > 1) { *length = args[0].length; }
	*dimensions = 3;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _cross(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _cross_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	bool ai = args[0].length == 1 && args[1].length > 1;
	bool bi = args[1].length == 1 && args[0].length > 1;
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0; i < result->length; i++)
		{
			if (d == 0) { result->xyzw[0][i] = args[0].xyzw[1][ai ? 0 : i] * args[1].xyzw[2][bi ? 0 : i] - args[0].xyzw[2][ai ? 0 : i] * args[1].xyzw[1][bi ? 0 : i]; }
			if (d == 1) { result->xyzw[1][i] = args[0].xyzw[2][ai ? 0 : i] * args[1].xyzw[0][bi ? 0 : i] - args[0].xyzw[0][ai ? 0 : i] * args[1].xyzw[2][bi ? 0 : i]; }
			if (d == 2) { result->xyzw[2][i] = args[0].xyzw[0][ai ? 0 : i] * args[1].xyzw[1][bi ? 0 : i] - args[0].xyzw[1][ai ? 0 : i] * args[1].xyzw[0][bi ? 0 : i]; }
		}
	}
	return RuntimeErrorNone;
}

static RuntimeErrorCode _dist_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	// takes two arguments of the same dimension
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorInvalidArgumentType; }
	*length = args[0].length < args[1].length ? args[0].length : args[1].length;
	if (args[0].length == 1 && args[1].length > 1) { *length = args[1].length; }
	if (args[1].length == 1 && args[0].length > 1) { *length = args[0].length; }
	*dimensions = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _dist(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _dist_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	bool ai = args[0].length == 1 && args[1].length > 1;
	bool bi = args[1].length == 1 && args[0].length > 1;
	result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
	for (int32_t i = 0; i < result->length; i++)
	{
		result->xyzw[0][i] = 0.0;
		for (int8_t d = 0; d < args[0].dimensions; d++)
		{
			scalar_t l = args[0].xyzw[d][ai ? 0 : i] - args[1].xyzw[d][bi ? 0 : i];
			result->xyzw[0][i] += l * l;
		}
		result->xyzw[0][i] = sqrtf(result->xyzw[0][i]);
	}
	
	return RuntimeErrorNone;
}

static RuntimeErrorCode _distsq_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	// takes two arguments of the same dimension
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorInvalidArgumentType; }
	*length = args[0].length < args[1].length ? args[0].length : args[1].length;
	if (args[0].length == 1 && args[1].length > 1) { *length = args[1].length; }
	if (args[1].length == 1 && args[0].length > 1) { *length = args[0].length; }
	*dimensions = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _distsq(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _distsq_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	bool ai = args[0].length == 1 && args[1].length > 1;
	bool bi = args[1].length == 1 && args[0].length > 1;
	result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
	for (int32_t i = 0; i < result->length; i++)
	{
		result->xyzw[0][i] = 0.0;
		for (int8_t d = 0; d < args[0].dimensions; d++)
		{
			scalar_t l = args[0].xyzw[d][ai ? 0 : i] - args[1].xyzw[d][bi ? 0 : i];
			result->xyzw[0][i] += l * l;
		}
	}
	
	return RuntimeErrorNone;
}

static RuntimeErrorCode _dot_size(list(VectorArray) args, uint32_t * length, uint32_t * dimensions)
{
	// takes two arguments of the same dimension
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorInvalidArgumentType; }
	*length = args[0].length < args[1].length ? args[0].length : args[1].length;
	if (args[0].length == 1 && args[1].length > 1) { *length = args[1].length; }
	if (args[1].length == 1 && args[0].length > 1) { *length = args[0].length; }
	*dimensions = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _dot(list(VectorArray) args, VectorArray * result)
{
	RuntimeErrorCode error = _dot_size(args, &result->length, &result->dimensions);
	if (error != RuntimeErrorNone) { return error; }
	
	bool ai = args[0].length == 1 && args[1].length > 1;
	bool bi = args[1].length == 1 && args[0].length > 1;
	result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
	for (int32_t i = 0; i < result->length; i++)
	{
		result->xyzw[0][i] = 0.0;
		for (int8_t d = 0; d < args[0].dimensions; d++) { result->xyzw[0][i] += args[0].xyzw[d][ai ? 0 : i] * args[1].xyzw[d][bi ? 0 : i]; }
	}
	
	return RuntimeErrorNone;
}

static RuntimeErrorCode _length_size(uint32_t * length, uint32_t * dimensions)
{
	*dimensions = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _length(VectorArray * result)
{
	// takes a single vector argument
	if (result->dimensions == 1) { return RuntimeErrorNone; } // if it's 1 dimension then return itself
	for (int32_t i = 0; i < result->length; i++)
	{
		result->xyzw[0][i] = result->xyzw[0][i] * result->xyzw[0][i];
		for (int8_t d = 1; d < result->dimensions; d++) { result->xyzw[0][i] += result->xyzw[d][i] * result->xyzw[d][i]; }
		result->xyzw[0][i] = sqrtf(result->xyzw[0][i]);
	}
	for (int8_t d = 1; d < result->dimensions; d++) { free(result->xyzw[d]); }
	result->dimensions = 1;
	return RuntimeErrorNone;
}

static RuntimeErrorCode _lengthsq_size(uint32_t * length, uint32_t * dimensions)
{
	*dimensions = 1;
	return RuntimeErrorNone;
}
static RuntimeErrorCode _lengthsq(VectorArray * result)
{
	// takes a single vector argument
	for (int32_t i = 0; i < result->length; i++)
	{
		result->xyzw[0][i] = result->xyzw[0][i] * result->xyzw[0][i];
		for (int8_t d = 1; d < result->dimensions; d++) { result->xyzw[0][i] += result->xyzw[d][i] * result->xyzw[d][i]; }
	}
	for (int8_t d = 1; d < result->dimensions; d++) { free(result->xyzw[d]); }
	result->dimensions = 1;
	return RuntimeErrorNone;
}

static RuntimeErrorCode _normalize(VectorArray * result)
{
	// takes a single vector argument
	if (result->dimensions == 1) { return RuntimeErrorNone; } // if it's 1 dimension then return itself
	for (int32_t i = 0; i < result->length; i++)
	{
		scalar_t len = 0.0;
		for (int8_t d = 0; d < result->dimensions; d++) { len += result->xyzw[d][i] * result->xyzw[d][i]; }
		len = sqrtf(len);
		for (int8_t d = 0; d < result->dimensions; d++) { result->xyzw[d][i] /= len; }
	}
	return RuntimeErrorNone;
}

BuiltinFunction DetermineBuiltinFunction(const char * identifier)
{
	for (int32_t i = 0; i < sizeof(builtinFunctions) / sizeof(builtinFunctions[0]); i++)
	{
		if (strcmp(identifier, builtinFunctions[i]) == 0) { return i; }
	}
	return BuiltinFunctionNone;
}

bool IsFunctionSingleArgument(BuiltinFunction function)
{
	for (int32_t i = 0; i < sizeof(multiArgumentBuiltins) / sizeof(multiArgumentBuiltins[0]); i++)
	{
		if (function == multiArgumentBuiltins[i]) { return false; }
	}
	return true;
}

bool IsFunctionIndexable(BuiltinFunction function)
{
	for (int32_t i = 0; i < sizeof(unindexableBuiltins) / sizeof(unindexableBuiltins[0]); i++)
	{
		if (function == unindexableBuiltins[i]) { return false; }
	}
	return true;
}

RuntimeErrorCode EvaluateBuiltinFunctionSize(BuiltinFunction function, list(VectorArray) arguments, uint32_t * length, uint32_t * dimensions)
{
	switch (function)
	{
		case BuiltinFunctionATAN2: return _atan2_size(arguments, length, dimensions);
		case BuiltinFunctionARGMAX: return _argmax_size(length, dimensions);
		case BuiltinFunctionARGMIN: return _argmin_size(length, dimensions);
		case BuiltinFunctionCORR: return _corr_size(arguments, length, dimensions);
		case BuiltinFunctionCOUNT: return _count_size(arguments, length, dimensions);
		case BuiltinFunctionCOV: return _cov_size(arguments, length, dimensions);
		case BuiltinFunctionINTERLEAVE: return _interleave_size(arguments, length, dimensions);
		case BuiltinFunctionJOIN: return _join_size(arguments, length, dimensions);
		case BuiltinFunctionLOG: return _log_size(arguments, length, dimensions);
		case BuiltinFunctionMAX: return _max_size(arguments, length, dimensions);
		case BuiltinFunctionMEAN: return _mean_size(length, dimensions);
		case BuiltinFunctionMEDIAN: return _median_size(length, dimensions);
		case BuiltinFunctionMIN: return _min_size(arguments, length, dimensions);
		case BuiltinFunctionPROD: return _prod_size(length, dimensions);
		case BuiltinFunctionQUANTILE: return _quantile_size(arguments, length, dimensions);
		case BuiltinFunctionRAND: return _rand_size(arguments, length, dimensions);
		case BuiltinFunctionSHUFFLE: return _shuffle_size(arguments, length, dimensions);
		case BuiltinFunctionSORT: return _sort_size(arguments, length, dimensions);
		case BuiltinFunctionSTDEV: return _stdev_size(length, dimensions);
		case BuiltinFunctionSUM: return _sum_size(length, dimensions);
		case BuiltinFunctionVAR: return _var_size(length, dimensions);
		case BuiltinFunctionCROSS: return _cross_size(arguments, length, dimensions);
		case BuiltinFunctionDIST: return _dist_size(arguments, length, dimensions);
		case BuiltinFunctionDISTSQ: return _distsq_size(arguments, length, dimensions);
		case BuiltinFunctionDOT: return _dot_size(arguments, length, dimensions);
		case BuiltinFunctionLENGTH: return _length_size(length, dimensions);
		case BuiltinFunctionLENGTHSQ: return _lengthsq_size(length, dimensions);
		default: break;
	}
	return RuntimeErrorNone;
}

RuntimeErrorCode EvaluateBuiltinFunction(BuiltinFunction function, list(VectorArray) arguments, VectorArray * result)
{
	switch (function)
	{
		case BuiltinFunctionSIN: return _sin(result);
		case BuiltinFunctionCOS: return _cos(result);
		case BuiltinFunctionTAN: return _tan(result);
		case BuiltinFunctionASIN: return _asin(result);
		case BuiltinFunctionACOS: return _acos(result);
		case BuiltinFunctionATAN: return _atan(result);
		case BuiltinFunctionATAN2: return _atan2(arguments, result);
		case BuiltinFunctionSEC: return _sec(result);
		case BuiltinFunctionCSC: return _csc(result);
		case BuiltinFunctionCOT: return _cot(result);
		case BuiltinFunctionASEC: return _asec(result);
		case BuiltinFunctionACSC: return _acsc(result);
		case BuiltinFunctionACOT: return _acot(result);
		case BuiltinFunctionSINH: return _sinh(result);
		case BuiltinFunctionCOSH: return _cosh(result);
		case BuiltinFunctionTANH: return _tanh(result);
		case BuiltinFunctionASINH: return _asinh(result);
		case BuiltinFunctionACOSH: return _acosh(result);
		case BuiltinFunctionATANH: return _atanh(result);
		case BuiltinFunctionSECH: return _sech(result);
		case BuiltinFunctionCSCH: return _csch(result);
		case BuiltinFunctionCOTH: return _coth(result);
		case BuiltinFunctionASECH: return _asech(result);
		case BuiltinFunctionACSCH: return _acsch(result);
		case BuiltinFunctionACOTH: return _acoth(result);
		case BuiltinFunctionABS: return _abs(result);
		case BuiltinFunctionARGMAX: return _argmax(result);
		case BuiltinFunctionARGMIN: return _argmin(result);
		case BuiltinFunctionCBRT: return _cbrt(result);
		case BuiltinFunctionCEIL: return _ceil(result);
		case BuiltinFunctionCORR: return _corr(arguments, result);
		case BuiltinFunctionCOUNT: return _count(arguments, result);
		case BuiltinFunctionCOV: return _cov(arguments, result);
		case BuiltinFunctionERF: return _erf(result);
		case BuiltinFunctionEXP: return _exp(result);
		case BuiltinFunctionFACTORIAL: return _factorial(result);
		case BuiltinFunctionFLOOR: return _floor(result);
		case BuiltinFunctionGAMMA: return _gamma(result);
		case BuiltinFunctionINTERLEAVE: return _interleave(arguments, result);
		case BuiltinFunctionJOIN: return _join(arguments, result);
		case BuiltinFunctionLN: return _ln(result);
		case BuiltinFunctionLOG: return _log(arguments, result);
		case BuiltinFunctionLOG10: return _log10(result);
		case BuiltinFunctionLOG2: return _log2(result);
		case BuiltinFunctionMAX: return _max(arguments, result);
		case BuiltinFunctionMEAN: return _mean(result);
		case BuiltinFunctionMEDIAN: return _median(result);
		case BuiltinFunctionMIN: return _min(arguments, result);
		case BuiltinFunctionPROD: return _prod(result);
		case BuiltinFunctionQUANTILE: return _quantile(arguments, result);
		case BuiltinFunctionRAND: return _rand(arguments, result);
		case BuiltinFunctionROUND: return _round(result);
		case BuiltinFunctionSHUFFLE: return _shuffle(arguments, result);
		case BuiltinFunctionSIGN: return _sign(result);
		case BuiltinFunctionSORT: return _sort(arguments, result);
		case BuiltinFunctionSQRT: return _sqrt(result);
		case BuiltinFunctionSTDEV: return _stdev(result);
		case BuiltinFunctionSUM: return _sum(result);
		case BuiltinFunctionVAR: return _var(result);
		case BuiltinFunctionCROSS: return _cross(arguments, result);
		case BuiltinFunctionDIST: return _dist(arguments, result);
		case BuiltinFunctionDISTSQ: return _distsq(arguments, result);
		case BuiltinFunctionDOT: return _dot(arguments, result);
		case BuiltinFunctionLENGTH: return _length(result);
		case BuiltinFunctionLENGTHSQ: return _lengthsq(result);
		case BuiltinFunctionNORMALIZE: return _normalize(result);
		default: return RuntimeErrorNotImplemented;
	}
}

static const char * builtinVariables[] =
{
	"pi", "tau", "e", "inf",
	"position", "scale", "rotation",
	"time",
};

BuiltinVariable DetermineBuiltinVariable(const char * identifier)
{
	for (int32_t i = 0; i < sizeof(builtinVariables) / sizeof(builtinVariables[0]); i++)
	{
		if (strcmp(identifier, builtinVariables[i]) == 0) { return i; }
	}
	return BuiltinVariableNone;
}

void InitializeBuiltins(HashMap cache)
{
	VectorArray * pi = malloc(sizeof(VectorArray));
	*pi = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = malloc(sizeof(scalar_t)) };
	pi->xyzw[0][0] = M_PI;
	
	VectorArray * tau = malloc(sizeof(VectorArray));
	*tau = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = malloc(sizeof(scalar_t)) };
	tau->xyzw[0][0] = 2 * M_PI;
	
	VectorArray * e = malloc(sizeof(VectorArray));
	*e = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = malloc(sizeof(scalar_t)) };
	e->xyzw[0][0] = M_E;
	
	VectorArray * inf = malloc(sizeof(VectorArray));
	*inf = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = malloc(sizeof(scalar_t)) };
	inf->xyzw[0][0] = INFINITY;
	
	VectorArray * position = malloc(sizeof(VectorArray));
	*position = (VectorArray){ .length = 1, .dimensions = 2, .xyzw = { malloc(sizeof(scalar_t)), malloc(sizeof(scalar_t)) } };
	position->xyzw[0][0] = 0;
	position->xyzw[1][0] = 0;
	
	VectorArray * scale = malloc(sizeof(VectorArray));
	*scale = (VectorArray){ .length = 1, .dimensions = 2, .xyzw = { malloc(sizeof(scalar_t)), malloc(sizeof(scalar_t)) } };
	scale->xyzw[0][0] = 1;
	scale->xyzw[1][0] = 1;
	
	VectorArray * rotation = malloc(sizeof(VectorArray));
	*rotation = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = malloc(sizeof(scalar_t)) };
	rotation->xyzw[0][0] = 0.0;
	
	VectorArray * time = malloc(sizeof(VectorArray));
	*time = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = malloc(sizeof(scalar_t)) };
	time->xyzw[0][0] = 0.0;
	
	HashMapSet(cache, "pi", pi);
	HashMapSet(cache, "tau", tau);
	HashMapSet(cache, "e", e);
	HashMapSet(cache, "inf", inf);
	HashMapSet(cache, "position", position);
	HashMapSet(cache, "scale", scale);
	HashMapSet(cache, "rotation", rotation);
	HashMapSet(cache, "time", time);
}

RuntimeErrorCode EvaluateBuiltinVariableSize(HashMap cache, BuiltinVariable variable, uint32_t * length, uint32_t * dimensions)
{
	VectorArray * cached = HashMapGet(cache, builtinVariables[variable]);
	if (cached == NULL) { return RuntimeErrorNotImplemented; }
	*length = cached->length;
	*dimensions = cached->dimensions;
	return RuntimeErrorNone;
}

RuntimeErrorCode EvaluateBuiltinVariable(HashMap cache, BuiltinVariable variable, VectorArray * result)
{
	VectorArray * cached = HashMapGet(cache, builtinVariables[variable]);
	if (cached == NULL) { return RuntimeErrorNotImplemented; }
	*result = CopyVectorArray(*cached, NULL, 0);
	return RuntimeErrorNone;
}
