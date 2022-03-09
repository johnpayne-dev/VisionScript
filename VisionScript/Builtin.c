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
	"gamma", "join", "ln", "log", "log10", "log2",
	"max", "mean", "median", "min", "prod", "quantile",
	"rand", "round", "shuffle", "sign", "sort", "sqrt",
	"stdev", "sum", "var",
	
	"cross", "dist", "distsq", "dot", "length", "lengthsq", "normalize",
};

static BuiltinFunction singleArgumentBuiltins[] =
{
	BuiltinFunctionSIN,
	BuiltinFunctionCOS,
	BuiltinFunctionTAN,
	BuiltinFunctionASIN,
	BuiltinFunctionACOS,
	BuiltinFunctionATAN,
	BuiltinFunctionSEC,
	BuiltinFunctionCSC,
	BuiltinFunctionCOT,
	BuiltinFunctionASEC,
	BuiltinFunctionACSC,
	BuiltinFunctionACOT,
	BuiltinFunctionSINH,
	BuiltinFunctionCOSH,
	BuiltinFunctionTANH,
	BuiltinFunctionASINH,
	BuiltinFunctionACOSH,
	BuiltinFunctionATANH,
	BuiltinFunctionSECH,
	BuiltinFunctionCSCH,
	BuiltinFunctionCOTH,
	BuiltinFunctionASECH,
	BuiltinFunctionACSCH,
	BuiltinFunctionACOTH,
	BuiltinFunctionABS,
	BuiltinFunctionARGMAX,
	BuiltinFunctionARGMIN,
	BuiltinFunctionCBRT,
	BuiltinFunctionCEIL,
	BuiltinFunctionCOUNT,
	BuiltinFunctionERF,
	BuiltinFunctionEXP,
	BuiltinFunctionFACTORIAL,
	BuiltinFunctionFLOOR,
	BuiltinFunctionGAMMA,
	BuiltinFunctionLN,
	BuiltinFunctionLOG10,
	BuiltinFunctionLOG2,
	BuiltinFunctionMEAN,
	BuiltinFunctionMEDIAN,
	BuiltinFunctionPROD,
	BuiltinFunctionROUND,
	BuiltinFunctionSIGN,
	BuiltinFunctionSQRT,
	BuiltinFunctionSTDEV,
	BuiltinFunctionSUM,
	BuiltinFunctionVAR,
	BuiltinFunctionLENGTH,
	BuiltinFunctionLENGTHSQ,
	BuiltinFunctionNORMALIZE,
};

static int compare(const void * a, const void * b)
{
	return (*(scalar_t *)a - *(scalar_t *)b > 0) - (*(scalar_t *)a - *(scalar_t *)b < 0);
}

static int coupled_compare(const void * a, const void * b)
{
	struct { int32_t i; scalar_t s; } * x = (void *)a;
	struct { int32_t i; scalar_t s; } * y = (void *)b;
	return (x->s - y->s > 0) - (x->s - y->s < 0);
}

static RuntimeError _sin(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = sinf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _cos(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = cosf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _tan(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tanf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _asin(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = asinf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _acos(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = acosf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _atan(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = atanf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _atan2(list(VectorArray) args, VectorArray * result)
{
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions > 1 || args[1].dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
	
	VectorArray y = args[0];
	VectorArray x = args[1];
	result->length = y.length < x.length ? y.length : x.length;
	bool yi = false, xi = false;
	if (y.length == 1 && x.length > 1) { result->length = x.length; yi = true; }
	if (x.length == 1 && y.length > 1) { result->length = y.length; xi = true; }
	
	result->dimensions = 1;
	result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
	for (int32_t i = 0; i < result->length; i++) { result->xyzw[0][i] = atan2f(y.xyzw[0][yi ? 0 : i], x.xyzw[0][xi ? 0 : i]); }
	
	return RuntimeErrorNone;
}

static RuntimeError _sec(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / cosf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _csc(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / sinf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _cot(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / tanf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _asec(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = acosf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _acsc(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = asinf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _acot(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = M_PI_2 - atanf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _sinh(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = sinhf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _cosh(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = coshf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _tanh(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tanhf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _asinh(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = asinhf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _acosh(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = acoshf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _atanh(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = atanhf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _sech(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / coshf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _csch(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / sinhf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _coth(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = 1.0 / tanhf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _asech(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = acoshf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _acsch(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = asinhf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _acoth(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = atanhf(1.0 / result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _abs(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = fabsf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _argmax(VectorArray * result)
{
	if (result->dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
	scalar_t max = result->xyzw[0][0];
	int32_t index = 0;
	for (int32_t i = 1; i < result->length; i++)
	{
		if (result->xyzw[0][i] > max) { max = result->xyzw[0][i]; index = i; }
	}
	free(result->xyzw[0]);
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = (scalar_t)index;
	result->length = 1;
	return RuntimeErrorNone;
}

static RuntimeError _argmin(VectorArray * result)
{
	if (result->dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
	scalar_t min = result->xyzw[0][0];
	int32_t index = 0;
	for (int32_t i = 1; i < result->length; i++)
	{
		if (result->xyzw[0][i] < min) { min = result->xyzw[0][i]; index = i; }
	}
	free(result->xyzw[0]);
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = index;
	result->length = 1;
	return RuntimeErrorNone;
}

static RuntimeError _cbrt(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = cbrtf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _ceil(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = ceilf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _corr(list(VectorArray) args, VectorArray * result)
{
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorInvalidArgumentType; }
	VectorArray a = args[0], b = args[1];
	int32_t length = a.length < b.length ? a.length : b.length;
	for (int8_t d = 0; d < a.dimensions; d++)
	{
		scalar_t avgA = 0.0, avgB = 0.0;
		for (int32_t i = 0; i < length; i++) { avgA += a.xyzw[d][i]; avgB += b.xyzw[d][i]; }
		avgA /= length;
		avgB /= length;
		scalar_t varA = 0.0, varB = 0.0;
		for (int32_t i = 0; i < length; i++) { varA += (a.xyzw[d][i] - avgA) * (a.xyzw[d][i] - avgA); varB += (b.xyzw[d][i] - avgB) * (b.xyzw[d][i] - avgB); }
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < length; i++) { sum += (a.xyzw[d][i] - avgA) * (b.xyzw[d][i] - avgB); }
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sum / sqrtf(varA * varB);
	}
	result->length = 1;
	result->dimensions = a.dimensions;
	return RuntimeErrorNone;
}

static RuntimeError _count(VectorArray * result)
{
	int32_t len = result->length;
	free(result->xyzw[0]);
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = len;
	result->length = 1;
	result->dimensions = 1;
	return RuntimeErrorNone;
}

static RuntimeError _cov(list(VectorArray) args, VectorArray * result)
{
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorInvalidArgumentType; }
	VectorArray a = args[0], b = args[1];
	int32_t length = a.length < b.length ? a.length : b.length;
	for (int8_t d = 0; d < a.dimensions; d++)
	{
		scalar_t avgA = 0.0, avgB = 0.0;
		for (int32_t i = 0; i < length; i++) { avgA += a.xyzw[d][i]; avgB += b.xyzw[d][i]; }
		avgA /= length;
		avgB /= length;
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < length; i++) { sum += (a.xyzw[d][i] - avgA) * (b.xyzw[d][i] - avgB); }
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sum / (length - 1);
	}
	result->length = 1;
	result->dimensions = a.dimensions;
	return RuntimeErrorNone;
}

static RuntimeError _erf(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = erff(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _exp(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = expf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _factorial(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tgammaf(result->xyzw[d][i] + 1.0); } }
	return RuntimeErrorNone;
}

static RuntimeError _floor(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = floorf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _gamma(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tgammaf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _join(list(VectorArray) args, VectorArray * result)
{
	result->dimensions = 0;
	result->length = 0;
	for (int32_t i = 0; i < ListLength(args); i++)
	{
		if (result->dimensions == 0) { result->dimensions = args[i].dimensions; }
		if (args[i].dimensions != result->dimensions) { return RuntimeErrorInvalidArgumentType; }
		result->length += args[i].length;
	}
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

static RuntimeError _ln(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = logf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _log(list(VectorArray) args, VectorArray * result)
{
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != args[1].dimensions && args[0].dimensions != 1 && args[1].dimensions != 1) { return RuntimeErrorInvalidArgumentType; }
	
	VectorArray b = args[0];
	VectorArray a = args[1];
	result->dimensions = a.dimensions > b.dimensions ? a.dimensions : b.dimensions;
	result->length = a.length < b.length ? a.length : b.length;
	bool ai = false, bi = false, ad = false, bd = false;
	if (a.length == 1 && b.length > 1) { result->length = b.length; ai = true; }
	if (b.length == 1 && a.length > 1) { result->length = a.length; bi = true; }
	if (a.dimensions == 1 && b.dimensions > 1) { ad = true; }
	if (b.dimensions == 1 && a.dimensions > 1) { bd = true; }
	
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

static RuntimeError _log10(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = log10f(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _log2(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = log2f(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _max(list(VectorArray) args, VectorArray * result)
{
	scalar_t max = args[0].xyzw[0][0];
	for (int32_t i = 0; i < ListLength(args); i++)
	{
		if (args[i].dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
		for (int32_t j = 0; j < args[i].length; j++) { if (args[i].xyzw[0][j] > max) { max = args[i].xyzw[0][j]; } }
	}
	result->length = 1;
	result->dimensions = 1;
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = max;
	return RuntimeErrorNone;
}

static RuntimeError _mean(VectorArray * result)
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

static RuntimeError _median(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		qsort(result->xyzw[d], result->length, sizeof(scalar_t), compare);
		scalar_t median = result->length % 2 == 1 ? result->xyzw[d][result->length / 2] : (result->xyzw[d][result->length / 2] + result->xyzw[d][result->length / 2 - 1]) / 2.0;
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = median;
	}
	result->length = 1;
	return RuntimeErrorNone;
}

static RuntimeError _min(list(VectorArray) args, VectorArray * result)
{
	scalar_t min = args[0].xyzw[0][0];
	for (int32_t i = 0; i < ListLength(args); i++)
	{
		if (args[i].dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
		for (int32_t j = 0; j < args[i].length; j++) { if (args[i].xyzw[0][j] < min) { min = args[i].xyzw[0][j]; } }
	}
	result->length = 1;
	result->dimensions = 1;
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = min;
	return RuntimeErrorNone;
}

static RuntimeError _prod(VectorArray * result)
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

static RuntimeError _quantile(list(VectorArray) args, VectorArray * result)
{
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[1].dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
	result->length = args[1].length;
	result->dimensions = args[0].dimensions;
	for (int8_t d = 0; d < result->dimensions; d++)
	{
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

static RuntimeError _rand(list(VectorArray) args, VectorArray * result)
{
	if (ListLength(args) == 0)
	{
		result->length = 1;
		result->dimensions = 1;
		result->xyzw[0] = malloc(sizeof(scalar_t));
		result->xyzw[0][0] = (scalar_t)rand() / RAND_MAX;
		return RuntimeErrorNone;
	}
	if (ListLength(args) == 1 || ListLength(args) == 2)
	{
		if (ListLength(args) == 2)
		{
			if (args[1].dimensions > 1 || args[1].length > 1) { return RuntimeErrorInvalidArgumentType; }
			srand(args[1].xyzw[0][0]);
		}
		if (args[0].dimensions > 1 || args[0].length > 1) { return RuntimeErrorInvalidArgumentType; }
		result->length = args[0].xyzw[0][0];
		result->dimensions = 1;
		result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0; i < result->length; i++) { result->xyzw[0][i] = (scalar_t)rand() / RAND_MAX; }
		return RuntimeErrorNone;
	}
	
	return RuntimeErrorIncorrectParameterCount;
}

static RuntimeError _round(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = roundf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _shuffle(list(VectorArray) args, VectorArray * result)
{
	if (ListLength(args) == 1 || ListLength(args) == 2)
	{
		if (ListLength(args) == 2)
		{
			if (args[1].dimensions > 1 || args[1].length > 1) { return RuntimeErrorInvalidArgumentType; }
			srand(args[1].xyzw[0][0]);
		}
		
		result->length = args[0].length;
		result->dimensions = args[0].dimensions;
		result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
		for (int32_t i = 0; i < result->length; i++)
		{
			int32_t swapIndex = rand() % result->length;
			for (int8_t d = 0; d < result->dimensions; d++)
			{
				result->xyzw[d][swapIndex] = args[0].xyzw[d][i];
				result->xyzw[d][i] = args[0].xyzw[d][swapIndex];
				args[0].xyzw[d][swapIndex] = result->xyzw[d][swapIndex];
				args[0].xyzw[d][i] = result->xyzw[d][i];
			}
		}
		return RuntimeErrorNone;
	}
	return RuntimeErrorIncorrectParameterCount;
}

static RuntimeError _sign(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = (result->xyzw[d][i] > 0) - (result->xyzw[d][i] < 0); } }
	return RuntimeErrorNone;
}

static RuntimeError _sort(list(VectorArray) args, VectorArray * result)
{
	if (ListLength(args) == 1)
	{
		result->length = args[0].length;
		result->dimensions = 1;
		for (int8_t d = 0; d < result->dimensions; d++)
		{
			result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
			memcpy(result->xyzw[d], args[0].xyzw[d], result->length * sizeof(scalar_t));
			qsort(result->xyzw[d], result->length, sizeof(scalar_t), compare);
		}
		return RuntimeErrorNone;
	}
	if (ListLength(args) == 2)
	{
		if (args[1].dimensions > 1) { return RuntimeErrorInvalidArgumentType; }
		int32_t length = args[0].length < args[1].length ? args[0].length : args[1].length;
		struct { int32_t i; scalar_t s; } * coupled = malloc(length * (sizeof(int32_t) + sizeof(scalar_t)));
		for (int32_t i = 0; i < length; i++) { coupled[i].i = i; coupled[i].s = args[1].xyzw[0][i]; }
		qsort(coupled, length, sizeof(int32_t) + sizeof(scalar_t), coupled_compare);
		
		result->length = length;
		result->dimensions = args[0].dimensions;
		for (int8_t d = 0; d < result->dimensions; d++)
		{
			result->xyzw[d] = malloc(result->length * sizeof(scalar_t));
			for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = args[0].xyzw[d][coupled[i].i]; }
		}
		return RuntimeErrorNone;
	}
	return RuntimeErrorIncorrectParameterCount;
}

static RuntimeError _sqrt(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = sqrtf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _stdev(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		scalar_t avg = 0.0;
		for (int32_t i = 0; i < result->length; i++) { avg += result->xyzw[d][i]; }
		avg /= result->length;
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < result->length; i++) { sum += (result->xyzw[d][i] - avg) * (result->xyzw[d][i] - avg); }
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sqrtf(sum / result->length);
	}
	result->length = 1;
	return RuntimeErrorNone;
}

static RuntimeError _sum(VectorArray * result)
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

static RuntimeError _var(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		scalar_t avg = 0.0;
		for (int32_t i = 0; i < result->length; i++) { avg += result->xyzw[d][i]; }
		avg /= result->length;
		scalar_t sum = 0.0;
		for (int32_t i = 0; i < result->length; i++) { sum += (result->xyzw[d][i] - avg) * (result->xyzw[d][i] - avg); }
		free(result->xyzw[d]);
		result->xyzw[d] = malloc(sizeof(scalar_t));
		result->xyzw[d][0] = sum / result->length;
	}
	result->length = 1;
	return RuntimeErrorNone;
}

static RuntimeError _cross(list(VectorArray) args, VectorArray * result)
{
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != 3 || args[1].dimensions != 3) { return RuntimeErrorInvalidArgumentType; }
	
	result->dimensions = 3;
	result->length = args[0].length < args[1].length ? args[0].length : args[1].length;
	bool ai = false, bi = false;
	if (args[0].length == 1 && args[1].length > 1) { result->length = args[1].length; ai = true; }
	if (args[1].length == 1 && args[0].length > 1) { result->length = args[0].length; bi = true; }
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

static RuntimeError _dist(list(VectorArray) args, VectorArray * result)
{
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorInvalidArgumentType; }
	
	result->dimensions = 1;
	result->length = args[0].length < args[1].length ? args[0].length : args[1].length;
	bool ai = false, bi = false;
	if (args[0].length == 1 && args[1].length > 1) { result->length = args[1].length; ai = true; }
	if (args[1].length == 1 && args[0].length > 1) { result->length = args[0].length; bi = true; }
	
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

static RuntimeError _distsq(list(VectorArray) args, VectorArray * result)
{
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorInvalidArgumentType; }
	
	result->dimensions = 1;
	result->length = args[0].length < args[1].length ? args[0].length : args[1].length;
	bool ai = false, bi = false;
	if (args[0].length == 1 && args[1].length > 1) { result->length = args[1].length; ai = true; }
	if (args[1].length == 1 && args[0].length > 1) { result->length = args[0].length; bi = true; }
	
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

static RuntimeError _dot(list(VectorArray) args, VectorArray * result)
{
	if (ListLength(args) != 2) { return RuntimeErrorIncorrectParameterCount; }
	if (args[0].dimensions != args[1].dimensions) { return RuntimeErrorInvalidArgumentType; }
	
	result->dimensions = 1;
	result->length = args[0].length < args[1].length ? args[0].length : args[1].length;
	bool ai = false, bi = false;
	if (args[0].length == 1 && args[1].length > 1) { result->length = args[1].length; ai = true; }
	if (args[1].length == 1 && args[0].length > 1) { result->length = args[0].length; bi = true; }
	
	result->xyzw[0] = malloc(result->length * sizeof(scalar_t));
	for (int32_t i = 0; i < result->length; i++)
	{
		result->xyzw[0][i] = 0.0;
		for (int8_t d = 0; d < args[0].dimensions; d++) { result->xyzw[0][i] += args[0].xyzw[d][ai ? 0 : i] * args[1].xyzw[d][bi ? 0 : i]; }
	}
	
	return RuntimeErrorNone;
}

static RuntimeError _length(VectorArray * result)
{
	if (result->dimensions == 1) { return RuntimeErrorNone; }
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

static RuntimeError _lengthsq(VectorArray * result)
{
	for (int32_t i = 0; i < result->length; i++)
	{
		result->xyzw[0][i] = result->xyzw[0][i] * result->xyzw[0][i];
		for (int8_t d = 1; d < result->dimensions; d++) { result->xyzw[0][i] += result->xyzw[d][i] * result->xyzw[d][i]; }
	}
	for (int8_t d = 1; d < result->dimensions; d++) { free(result->xyzw[d]); }
	result->dimensions = 1;
	return RuntimeErrorNone;
}

static RuntimeError _normalize(VectorArray * result)
{
	if (result->dimensions == 1) { return RuntimeErrorNone; }
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
	for (int32_t i = 0; i < sizeof(singleArgumentBuiltins) / sizeof(singleArgumentBuiltins[0]); i++)
	{
		if (function == singleArgumentBuiltins[i]) { return true; }
	}
	return false;
}

RuntimeError EvaluateBuiltinFunction(BuiltinFunction function, list(VectorArray) arguments, VectorArray * result)
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
		case BuiltinFunctionCOUNT: return _count(result);
		case BuiltinFunctionCOV: return _cov(arguments, result);
		case BuiltinFunctionERF: return _erf(result);
		case BuiltinFunctionEXP: return _exp(result);
		case BuiltinFunctionFACTORIAL: return _factorial(result);
		case BuiltinFunctionFLOOR: return _floor(result);
		case BuiltinFunctionGAMMA: return _gamma(result);
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
	/*"cam_x", "cam_y", "cam_scale", "cam_rot",
	"mouse_x", "mouse_y", "mouse_dx", "mouse_dy", "mouse_btn", "mouse_clk,"
	"time",*/
};

BuiltinVariable DetermineBuiltinVariable(const char * identifier)
{
	for (int32_t i = 0; i < sizeof(builtinVariables) / sizeof(builtinVariables[0]); i++)
	{
		if (strcmp(identifier, builtinVariables[i]) == 0) { return i; }
	}
	return BuiltinVariableNone;
}

RuntimeError EvaluateBuiltinVariable(BuiltinVariable variable, VectorArray * result)
{
	result->length = 1;
	result->dimensions = 1;
	result->xyzw[0] = malloc(sizeof(scalar_t));
	switch (variable)
	{
		case BuiltinVariablePI: result->xyzw[0][0] = M_PI; break;
		case BuiltinVariableTAU: result->xyzw[0][0] = 2 * M_PI; break;
		case BuiltinVariableE: result->xyzw[0][0] = M_E; break;
		case BuiltinVariableINF: result->xyzw[0][0] = INFINITY; break;
		default: return RuntimeErrorNotImplemented;
	}
	return RuntimeErrorNone;
}
