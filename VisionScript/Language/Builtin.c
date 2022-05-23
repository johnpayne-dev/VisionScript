#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Builtin.h"

static const char * builtinFunctions[] = {
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

static BuiltinFunction multiArgumentBuiltins[] = {
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

static int compare(const void * a, const void * b) {
	// used for list sorting
	return (*(scalar_t *)a - *(scalar_t *)b > 0) - (*(scalar_t *)a - *(scalar_t *)b < 0);
}

static int coupled_compare(const void * a, const void * b) {
	// used for list sorting relative to another list
	struct { int32_t i; scalar_t s; } * x = (void *)a;
	struct { int32_t i; scalar_t s; } * y = (void *)b;
	return (x->s - y->s > 0) - (x->s - y->s < 0);
}

static RuntimeErrorCode _sin(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = sinf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _cos(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = cosf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _tan(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tanf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _asin(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = asinf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _acos(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = acosf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _atan(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = atanf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _atan2(List(VectorArray) args, VectorArray * result) {
	// atan2 takes two non-vector arguments
	if (ListLength(args) != 2) { return RuntimeErrorCodeIncorrectArgumentCount; }
	if (args[0].dimensions > 1 || args[1].dimensions > 1) { return RuntimeErrorCodeInvalidArgumentType; }
	result->length = args[0].length < args[1].length ? args[0].length : args[1].length;
	if (args[0].length == 1 && args[1].length > 1) { result->length = args[1].length; }
	if (args[1].length == 1 && args[0].length > 1) { result->length = args[0].length; }
	result->dimensions = 1;
	
	// length 1 argument will be extended to length of other argument
	VectorArray y = args[0], x = args[1];
	bool yi = y.length == 1 && x.length > 1;
	bool xi = x.length == 1 && y.length > 1;
	
	result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
	for (int32_t i = 0; i < result->length; i++) { result->xyzw[0][i] = atan2f(y.xyzw[0][yi ? 0 : i], x.xyzw[0][xi ? 0 : i]); }
	
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _sec(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / cosf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _csc(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / sinf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _cot(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / tanf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _asec(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = acosf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _acsc(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = asinf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _acot(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = M_PI_2 - atanf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _sinh(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = sinhf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _cosh(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = coshf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _tanh(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tanhf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _asinh(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = asinhf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _acosh(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = acoshf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _atanh(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = atanhf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _sech(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / coshf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _csch(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / sinhf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _coth(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / tanhf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _asech(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = acoshf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _acsch(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = asinhf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _acoth(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = atanhf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _abs(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = fabsf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _argmax(VectorArray * result) {
	// argmax takes 1 non-vector argument
	if (result->dimensions > 1) { return RuntimeErrorCodeInvalidArgumentType; }
	result->length = 1;
	
	scalar_t max = result->xyzw[0][0];
	int32_t index = 0;
	for (int32_t i = 1; i < result->length; i++) {
		if (result->xyzw[0][i] > max) {
			max = result->xyzw[0][i];
			index = i;
		}
	}
	free(result->xyzw[0]);
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = (scalar_t)index;
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _argmin(VectorArray * result) {
	// argmin takes 1 non-vector argument
	if (result->dimensions > 1) { return RuntimeErrorCodeInvalidArgumentType; }
	result->length = 1;
	
	scalar_t min = result->xyzw[0][0];
	int32_t index = 0;
	for (int32_t i = 1; i < result->length; i++) {
		if (result->xyzw[0][i] < min) {
			min = result->xyzw[0][i];
			index = i;
		}
	}
	free(result->xyzw[0]);
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = index;
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _cbrt(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = cbrtf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _ceil(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = ceilf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _corr(List(VectorArray) args, VectorArray * result) {
	// corr takes two arguments of same dimensionality
	if (ListLength(args) != 2) { return RuntimeErrorCodeIncorrectArgumentCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorCodeInvalidArgumentType; }
	result->length = 1;
	result->dimensions = args[0].dimensions;
	
	VectorArray a = args[0], b = args[1];
	int32_t length = a.length < b.length ? a.length : b.length;
	for (int32_t d = 0; d < a.dimensions; d++) {
		// calculate the means for both
		scalar_t avgA = 0.0, avgB = 0.0;
		for (int32_t i = 0; i < length; i++) {
			avgA += a.xyzw[d][i];
			avgB += b.xyzw[d][i];
		}
		avgA /= length;
		avgB /= length;
		
		// calculate the variance for both
		scalar_t varA = 0.0, varB = 0.0;
		for (int32_t i = 0; i < length; i++) {
			varA += (a.xyzw[d][i] - avgA) * (a.xyzw[d][i] - avgA);
			varB += (b.xyzw[d][i] - avgB) * (b.xyzw[d][i] - avgB);
		}
		
		// calculate final normalized sum
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < length; i++) { sum += (a.xyzw[d][i] - avgA) * (b.xyzw[d][i] - avgB); }
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sum / sqrtf(varA * varB);
	}
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _count(List(VectorArray) args, VectorArray * result) {
	result->length = 1;
	result->dimensions = 1;
	if (ListLength(args) == 2) {
		if (args[1].dimensions != args[0].dimensions) { return RuntimeErrorCodeInvalidArgumentType; }
		if (args[1].length > 1) { return RuntimeErrorCodeInvalidArgumentType; }
	}
	
	if (ListLength(args) == 1) {
		// returns length of the vector array passed
		int32_t len = args[0].length;
		result->xyzw[0] = malloc(sizeof(scalar_t));
		result->xyzw[0][0] = len;
		return RuntimeErrorCodeNone;
	}
	if (ListLength(args) == 2) {
		// counts how many elements equal the second argument
		int32_t count = 0;
		for (int32_t i = 0; i < args[0].length; i++) {
			bool equal = true;
			for (int32_t d = 0; d < args[0].dimensions; d++) {
				if (args[0].xyzw[d][i] != args[1].xyzw[d][0]) {
					equal = false;
					break;
				}
			}
			if (equal) { count++; }
		}
		result->xyzw[0] = malloc(sizeof(scalar_t));
		result->xyzw[0][0] = count;
		return RuntimeErrorCodeNone;
	}
	return RuntimeErrorCodeIncorrectArgumentCount;
}

static RuntimeErrorCode _cov(List(VectorArray) args, VectorArray * result) {
	// cov takes two arguments of same dimensionality
	if (ListLength(args) != 2) { return RuntimeErrorCodeIncorrectArgumentCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorCodeInvalidArgumentType; }
	result->length = 1;
	result->dimensions = args[0].dimensions;
	
	VectorArray a = args[0], b = args[1];
	int32_t length = a.length < b.length ? a.length : b.length;
	for (int32_t d = 0; d < a.dimensions; d++) {
		// calculate means
		scalar_t avgA = 0.0, avgB = 0.0;
		for (int32_t i = 0; i < length; i++) {
			avgA += a.xyzw[d][i];
			avgB += b.xyzw[d][i];
		}
		avgA /= length;
		avgB /= length;
		
		// then calculate the covariance
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < length; i++) { sum += (a.xyzw[d][i] - avgA) * (b.xyzw[d][i] - avgB); }
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sum / (length - 1);
	}
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _erf(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = erff(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _exp(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = expf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _factorial(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tgammaf(result->xyzw[d][i] + 1.0); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _floor(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = floorf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _gamma(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tgammaf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _interleave(List(VectorArray) args, VectorArray * result) {
	// takes n arguments of same dimensionality
	result->dimensions = 0;
	result->length = 0;
	for (int32_t i = 0; i < ListLength(args); i++) {
		// first argument determines dimensionality of whole array
		if (result->dimensions == 0) { result->dimensions = args[i].dimensions; }
		if (args[i].dimensions != result->dimensions) { return RuntimeErrorCodeInvalidArgumentType; }
		result->length += args[i].length;
	}
	
	// interleave the contents of each argument into a single array
	for (int32_t d = 0; d < result->dimensions; d++) {
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		int32_t * counters = calloc(ListLength(args), sizeof(int32_t));
		int32_t c = 0;
		while (c < result->length) {
			for (int32_t i = 0; i < ListLength(args); i++) {
				if (counters[i] < args[i].length) { result->xyzw[d][c++] = args[i].xyzw[d][counters[i]++]; }
			}
		}
		free(counters);
	}
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _join(List(VectorArray) args, VectorArray * result)
{
	// takes n arguments of same dimensionality
	result->dimensions = 0;
	result->length = 0;
	for (int32_t i = 0; i < ListLength(args); i++) {
		// first argument determines dimensionality of whole array
		if (result->dimensions == 0) { result->dimensions = args[i].dimensions; }
		if (args[i].dimensions != result->dimensions) { return RuntimeErrorCodeInvalidArgumentType; }
		result->length += args[i].length;
	}
	
	// copy the contents of each argument into a single array
	for (int32_t d = 0; d < result->dimensions; d++) {
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0, p = 0; i < ListLength(args); i++) {
			memcpy(result->xyzw[d] + p, args[i].xyzw[d], args[i].length * sizeof(scalar_t));
			p += args[i].length;
		}
	}
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _ln(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = logf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _log(List(VectorArray) args, VectorArray * result) {
	// takes two arguments of same vector, (with exception to dimension 1)
	if (ListLength(args) != 2) { return RuntimeErrorCodeIncorrectArgumentCount; }
	if (args[0].dimensions != args[1].dimensions && args[0].dimensions != 1 && args[1].dimensions != 1) { return RuntimeErrorCodeInvalidArgumentType; }
	result->length = args[1].length < args[0].length ? args[1].length : args[0].length;
	if (args[1].length == 1 && args[0].length > 1) { result->length = args[0].length; }
	if (args[0].length == 1 && args[1].length > 1) { result->length = args[1].length; }
	result->dimensions = args[1].dimensions > args[0].dimensions ? args[1].dimensions : args[0].dimensions;
	
	// determine which arguments are scalars
	VectorArray b = args[0], a = args[1];
	bool ai = a.length == 1 && b.length > 1;
	bool bi = b.length == 1 && a.length > 1;
	bool ad = a.dimensions == 1 && b.dimensions > 1;
	bool bd = b.dimensions == 1 && a.dimensions > 1;
	
	// calculate the log
	for (int32_t d = 0; d < result->dimensions; d++) {
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0; i < result->length; i++) {
			result->xyzw[d][i] = logf(a.xyzw[ad ? 0 : d][ai ? 0 : i]) / logf(b.xyzw[bd ? 0 : d][bi ? 0 : i]);
		}
	}
	
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _log10(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = log10f(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _log2(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = log2f(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _max(List(VectorArray) args, VectorArray * result) {
	result->length = 1;
	result->dimensions = 1;
	for (int32_t i = 0; i < ListLength(args); i++) {
		if (args[i].dimensions > 1) { return RuntimeErrorCodeInvalidArgumentType; }
	}
	
	// takes n arguments all of dimension 1
	scalar_t max = args[0].xyzw[0][0];
	for (int32_t i = 0; i < ListLength(args); i++) {
		for (int32_t j = 0; j < args[i].length; j++) {
			if (args[i].xyzw[0][j] > max) { max = args[i].xyzw[0][j]; }
		}
	}
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = max;
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _mean(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) {
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < result->length; i++) { sum += result->xyzw[d][i]; }
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sum / result->length;
	}
	result->length = 1;
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _median(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) {
		// sorts the list then gets the element in the middle
		qsort(result->xyzw[d], result->length, sizeof(scalar_t), compare);
		scalar_t median = result->length % 2 == 1 ? result->xyzw[d][result->length / 2] : (result->xyzw[d][result->length / 2] + result->xyzw[d][result->length / 2 - 1]) / 2.0;
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = median;
	}
	result->length = 1;
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _min(List(VectorArray) args, VectorArray * result) {
	result->length = 1;
	result->dimensions = 1;
	for (int32_t i = 0; i < ListLength(args); i++) {
		if (args[i].dimensions > 1) { return RuntimeErrorCodeInvalidArgumentType; }
	}
	
	// takes n arguments all of dimension 1
	scalar_t min = args[0].xyzw[0][0];
	for (int32_t i = 0; i < ListLength(args); i++) {
		for (int32_t j = 0; j < args[i].length; j++) {
			if (args[i].xyzw[0][j] < min) { min = args[i].xyzw[0][j]; }
		}
	}
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = min;
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _prod(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) {
		scalar_t prod = 1.0;
		for (int32_t i = 0; i < result->length; i++) { prod *= result->xyzw[d][i]; }
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = prod;
	}
	result->length = 1;
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _quantile(List(VectorArray) args, VectorArray * result) {
	// takes two arguments, second argument must not be a vector
	if (ListLength(args) != 2) { return RuntimeErrorCodeIncorrectArgumentCount; }
	if (args[1].dimensions > 1) { return RuntimeErrorCodeInvalidArgumentType; }
	result->length = args[1].length;
	result->dimensions = args[0].dimensions;
	
	for (int32_t d = 0; d < result->dimensions; d++) {
		// sort list and get each element at each given quantile
		qsort(args[0].xyzw[d], args[0].length, sizeof(scalar_t), compare);
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0; i < result->length; i++) {
			if (args[1].xyzw[0][i] < 0.0 || args[1].xyzw[0][i] > 1.0) { result->xyzw[d][i] = NAN; continue; }
			scalar_t index = args[1].xyzw[0][i] * (args[0].length - 1);
			int32_t a = floorf(index), b = ceilf(index);
			result->xyzw[d][i] = args[0].xyzw[d][a] * (1.0 - (index - a)) + args[0].xyzw[d][b] * (index - a);
		}
	}
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _rand(List(VectorArray) args, VectorArray * result) {
	return RuntimeErrorCodeNotImplemented;
}

static RuntimeErrorCode _round(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = roundf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _shuffle(List(VectorArray) args, VectorArray * result) {
	return RuntimeErrorCodeNotImplemented;
}

static RuntimeErrorCode _sign(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = (result->xyzw[d][i] > 0) - (result->xyzw[d][i] < 0); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _sort(List(VectorArray) args, VectorArray * result) {
	// if one argument is passed then sort in ascending order
	if (ListLength(args) == 1) {
		result->length = args[0].length;
		result->dimensions = 1;
		for (int32_t d = 0; d < result->dimensions; d++) {
			result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
			memcpy(result->xyzw[d], args[0].xyzw[d], result->length * sizeof(scalar_t));
			qsort(result->xyzw[d], result->length, sizeof(scalar_t), compare);
		}
		return RuntimeErrorCodeNone;
	}
	// if two arguments are passed then sort relative the second list
	if (ListLength(args) == 2) {
		if (args[1].dimensions > 1) { return RuntimeErrorCodeInvalidArgumentType; }
		result->length = args[0].length < args[1].length ? args[0].length : args[1].length;
		result->dimensions = args[0].dimensions;
		// each index of the first list is coupled with the corresponding element of the second list
		struct { int32_t i; scalar_t s; } * coupled = malloc(result->length * (sizeof(int32_t) + sizeof(scalar_t)));
		for (int32_t i = 0; i < result->length; i++) { coupled[i].i = i; coupled[i].s = args[1].xyzw[0][i]; }
		qsort(coupled, result->length, sizeof(int32_t) + sizeof(scalar_t), coupled_compare);
		
		// rearrange the first list to be in the order of how the second list was sorted
		for (int32_t d = 0; d < result->dimensions; d++) {
			result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
			for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = args[0].xyzw[d][coupled[i].i]; }
		}
		return RuntimeErrorCodeNone;
	}
	return RuntimeErrorCodeIncorrectArgumentCount;
}

static RuntimeErrorCode _sqrt(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = sqrtf(result->xyzw[d][i]); } }
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _stdev(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) {
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
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _sum(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) {
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < result->length; i++) { sum += result->xyzw[d][i]; }
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sum;
	}
	result->length = 1;
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _var(VectorArray * result) {
	for (int32_t d = 0; d < result->dimensions; d++) {
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
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _cross(List(VectorArray) args, VectorArray * result) {
	// cross product works on two 3d vectors
	if (ListLength(args) != 2) { return RuntimeErrorCodeIncorrectArgumentCount; }
	if (args[0].dimensions != 3 || args[1].dimensions != 3) { return RuntimeErrorCodeInvalidArgumentType; }
	result->length = args[0].length < args[1].length ? args[0].length : args[1].length;
	if (args[0].length == 1 && args[1].length > 1) { result->length = args[1].length; }
	if (args[1].length == 1 && args[0].length > 1) { result->length = args[0].length; }
	result->dimensions = 3;
	
	bool ai = args[0].length == 1 && args[1].length > 1;
	bool bi = args[1].length == 1 && args[0].length > 1;
	for (int32_t d = 0; d < result->dimensions; d++) {
		result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0; i < result->length; i++) {
			if (d == 0) { result->xyzw[0][i] = args[0].xyzw[1][ai ? 0 : i] * args[1].xyzw[2][bi ? 0 : i] - args[0].xyzw[2][ai ? 0 : i] * args[1].xyzw[1][bi ? 0 : i]; }
			if (d == 1) { result->xyzw[1][i] = args[0].xyzw[2][ai ? 0 : i] * args[1].xyzw[0][bi ? 0 : i] - args[0].xyzw[0][ai ? 0 : i] * args[1].xyzw[2][bi ? 0 : i]; }
			if (d == 2) { result->xyzw[2][i] = args[0].xyzw[0][ai ? 0 : i] * args[1].xyzw[1][bi ? 0 : i] - args[0].xyzw[1][ai ? 0 : i] * args[1].xyzw[0][bi ? 0 : i]; }
		}
	}
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _dist(List(VectorArray) args, VectorArray * result) {
	// takes two arguments of the same dimension
	if (ListLength(args) != 2) { return RuntimeErrorCodeIncorrectArgumentCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorCodeInvalidArgumentType; }
	result->length = args[0].length < args[1].length ? args[0].length : args[1].length;
	if (args[0].length == 1 && args[1].length > 1) { result->length = args[1].length; }
	if (args[1].length == 1 && args[0].length > 1) { result->length = args[0].length; }
	result->dimensions = 1;
	
	bool ai = args[0].length == 1 && args[1].length > 1;
	bool bi = args[1].length == 1 && args[0].length > 1;
	result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
	for (int32_t i = 0; i < result->length; i++) {
		result->xyzw[0][i] = 0.0;
		for (int32_t d = 0; d < args[0].dimensions; d++) {
			scalar_t l = args[0].xyzw[d][ai ? 0 : i] - args[1].xyzw[d][bi ? 0 : i];
			result->xyzw[0][i] += l * l;
		}
		result->xyzw[0][i] = sqrtf(result->xyzw[0][i]);
	}
	
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _distsq(List(VectorArray) args, VectorArray * result) {
	// takes two arguments of the same dimension
	if (ListLength(args) != 2) { return RuntimeErrorCodeIncorrectArgumentCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorCodeInvalidArgumentType; }
	result->length = args[0].length < args[1].length ? args[0].length : args[1].length;
	if (args[0].length == 1 && args[1].length > 1) { result->length = args[1].length; }
	if (args[1].length == 1 && args[0].length > 1) { result->length = args[0].length; }
	result->dimensions = 1;
	
	bool ai = args[0].length == 1 && args[1].length > 1;
	bool bi = args[1].length == 1 && args[0].length > 1;
	result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
	for (int32_t i = 0; i < result->length; i++) {
		result->xyzw[0][i] = 0.0;
		for (int32_t d = 0; d < args[0].dimensions; d++) {
			scalar_t l = args[0].xyzw[d][ai ? 0 : i] - args[1].xyzw[d][bi ? 0 : i];
			result->xyzw[0][i] += l * l;
		}
	}
	
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _dot(List(VectorArray) args, VectorArray * result) {
	// takes two arguments of the same dimension
	if (ListLength(args) != 2) { return RuntimeErrorCodeIncorrectArgumentCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorCodeInvalidArgumentType; }
	result->length = args[0].length < args[1].length ? args[0].length : args[1].length;
	if (args[0].length == 1 && args[1].length > 1) { result->length = args[1].length; }
	if (args[1].length == 1 && args[0].length > 1) { result->length = args[0].length; }
	result->dimensions = 1;
	
	bool ai = args[0].length == 1 && args[1].length > 1;
	bool bi = args[1].length == 1 && args[0].length > 1;
	result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
	for (int32_t i = 0; i < result->length; i++) {
		result->xyzw[0][i] = 0.0;
		for (int32_t d = 0; d < args[0].dimensions; d++) { result->xyzw[0][i] += args[0].xyzw[d][ai ? 0 : i] * args[1].xyzw[d][bi ? 0 : i]; }
	}
	
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _length(VectorArray * result) {
	// takes a single vector argument
	if (result->dimensions == 1) { return RuntimeErrorCodeNone; } // if it's 1 dimension then return itself
	for (int32_t i = 0; i < result->length; i++) {
		result->xyzw[0][i] = result->xyzw[0][i] * result->xyzw[0][i];
		for (int32_t d = 1; d < result->dimensions; d++) { result->xyzw[0][i] += result->xyzw[d][i] * result->xyzw[d][i]; }
		result->xyzw[0][i] = sqrtf(result->xyzw[0][i]);
	}
	for (int32_t d = 1; d < result->dimensions; d++) { free(result->xyzw[d]); }
	result->dimensions = 1;
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _lengthsq(VectorArray * result) {
	// takes a single vector argument
	for (int32_t i = 0; i < result->length; i++) {
		result->xyzw[0][i] = result->xyzw[0][i] * result->xyzw[0][i];
		for (int32_t d = 1; d < result->dimensions; d++) { result->xyzw[0][i] += result->xyzw[d][i] * result->xyzw[d][i]; }
	}
	for (int32_t d = 1; d < result->dimensions; d++) { free(result->xyzw[d]); }
	result->dimensions = 1;
	return RuntimeErrorCodeNone;
}

static RuntimeErrorCode _normalize(VectorArray * result) {
	// takes a single vector argument
	if (result->dimensions == 1) { return RuntimeErrorCodeNone; } // if it's 1 dimension then return itself
	for (int32_t i = 0; i < result->length; i++) {
		scalar_t len = 0.0;
		for (int32_t d = 0; d < result->dimensions; d++) { len += result->xyzw[d][i] * result->xyzw[d][i]; }
		len = sqrtf(len);
		for (int32_t d = 0; d < result->dimensions; d++) { result->xyzw[d][i] /= len; }
	}
	return RuntimeErrorCodeNone;
}

BuiltinFunction DetermineBuiltinFunction(const char * identifier) {
	for (int32_t i = 0; i < sizeof(builtinFunctions) / sizeof(builtinFunctions[0]); i++) {
		if (strcmp(identifier, builtinFunctions[i]) == 0) { return i; }
	}
	return BuiltinFunctionNone;
}

bool IsFunctionSingleArgument(BuiltinFunction function) {
	for (int32_t i = 0; i < sizeof(multiArgumentBuiltins) / sizeof(multiArgumentBuiltins[0]); i++) {
		if (function == multiArgumentBuiltins[i]) { return false; }
	}
	return true;
}

RuntimeErrorCode EvaluateBuiltinFunction(BuiltinFunction function, List(VectorArray) arguments, VectorArray * result) {
	switch (function) {
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
		default: return RuntimeErrorCodeNotImplemented;
	}
}

static const char * builtinVariables[] = {
	[BuiltinVariablePI]       = "pi",
	[BuiltinVariableTAU]      = "tau",
	[BuiltinVariableE]        = "e",
	[BuiltinVariableINF]      = "inf",
	[BuiltinVariablePOSITION] = "position",
	[BuiltinVariableSCALE]    = "scale",
	[BuiltinVariableROTATION] = "rotation",
	[BuiltinVariableTIME]     = "time",
};

BuiltinVariable DetermineBuiltinVariable(const char * identifier) {
	for (int32_t i = 0; i < sizeof(builtinVariables) / sizeof(builtinVariables[0]); i++) {
		if (strcmp(identifier, builtinVariables[i]) == 0) { return i; }
	}
	return BuiltinVariableNone;
}

void InitializeBuiltinVariables(Environment * environment) {
	VectorArray pi = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = malloc(sizeof(scalar_t)) };
	pi.xyzw[0][0] = M_PI;
	SetEnvironmentCache(environment, builtinVariables[BuiltinVariablePI], pi);
	
	VectorArray tau = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = malloc(sizeof(scalar_t)) };
	tau.xyzw[0][0] = 2 * M_PI;
	SetEnvironmentCache(environment, builtinVariables[BuiltinVariableTAU], tau);
	
	VectorArray e = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = malloc(sizeof(scalar_t)) };
	e.xyzw[0][0] = M_E;
	SetEnvironmentCache(environment, builtinVariables[BuiltinVariableE], e);
	
	VectorArray inf = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = malloc(sizeof(scalar_t)) };
	inf.xyzw[0][0] = INFINITY;
	SetEnvironmentCache(environment, builtinVariables[BuiltinVariableINF], inf);
	
	VectorArray position = (VectorArray){ .length = 1, .dimensions = 2, .xyzw = { malloc(sizeof(scalar_t)), malloc(sizeof(scalar_t)) } };
	position.xyzw[0][0] = 0;
	position.xyzw[1][0] = 0;
	SetEnvironmentCache(environment, builtinVariables[BuiltinVariablePOSITION], position);
	
	VectorArray scale = (VectorArray){ .length = 1, .dimensions = 2, .xyzw = { malloc(sizeof(scalar_t)), malloc(sizeof(scalar_t)) } };
	scale.xyzw[0][0] = 1;
	scale.xyzw[1][0] = 1;
	SetEnvironmentCache(environment, builtinVariables[BuiltinVariableSCALE], scale);
	
	VectorArray rotation = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = malloc(sizeof(scalar_t)) };
	rotation.xyzw[0][0] = 0.0;
	SetEnvironmentCache(environment, builtinVariables[BuiltinVariableROTATION], rotation);
	
	VectorArray time = (VectorArray){ .length = 1, .dimensions = 1, .xyzw[0] = malloc(sizeof(scalar_t)) };
	time.xyzw[0][0] = 0.0;
	SetEnvironmentCache(environment, builtinVariables[BuiltinVariableTIME], time);
}

RuntimeErrorCode EvaluateBuiltinVariable(Environment environment, BuiltinVariable variable, VectorArray * result) {
	VectorArray * cached = GetEnvironmentCache(&environment, builtinVariables[variable]);
	if (cached == NULL) { return RuntimeErrorCodeNotImplemented; }
	*result = CopyVectorArray(*cached);
	return RuntimeErrorCodeNone;
}

