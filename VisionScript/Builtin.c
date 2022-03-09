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
	"gamma", "gcd", "join", "lcm", "ln", "log",
	"log10", "log2", "max", "mean", "median", "min",
	"prod", "quantile", "rand", "round", "shuffle", "sign",
	"sort", "sqrt", "stdev", "sum", "unique", "var",
	
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
	BuiltinFunctionUNIQUE,
	BuiltinFunctionVAR,
	BuiltinFunctionLENGTH,
	BuiltinFunctionLENGTHSQ,
	BuiltinFunctionNORMALIZE,
};

static int compare(const void * a, const void * b)
{
	return *(scalar_t *)a - *(scalar_t *)b;
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

static RuntimeError _count(VectorArray * result)
{
	int32_t len = result->length;
	free(result->xyzw[0]);
	result->xyzw[0] = malloc(sizeof(scalar_t));
	result->xyzw[0][0] = len;
	result->length = 1;
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

static RuntimeError _gamma(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = tgammaf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _ln(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = logf(result->xyzw[d][i]); } }
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

static RuntimeError _round(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = roundf(result->xyzw[d][i]); } }
	return RuntimeErrorNone;
}

static RuntimeError _sign(VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++) { for (int32_t i = 0; i < result->length; i++) { result->xyzw[d][i] = (result->xyzw[d][i] > 0) - (result->xyzw[d][i] < 0); } }
	return RuntimeErrorNone;
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

static RuntimeError _unique(VectorArray * result)
{
	return RuntimeErrorNotImplemented;
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
		case BuiltinFunctionCOUNT: return _count(result);
		case BuiltinFunctionERF: return _erf(result);
		case BuiltinFunctionEXP: return _exp(result);
		case BuiltinFunctionFACTORIAL: return _factorial(result);
		case BuiltinFunctionGAMMA: return _gamma(result);
		case BuiltinFunctionLN: return _ln(result);
		case BuiltinFunctionLOG10: return _log10(result);
		case BuiltinFunctionLOG2: return _log2(result);
		case BuiltinFunctionMEAN: return _mean(result);
		case BuiltinFunctionMEDIAN: return _median(result);
		case BuiltinFunctionPROD: return _prod(result);
		case BuiltinFunctionROUND: return _round(result);
		case BuiltinFunctionSIGN: return _sign(result);
		case BuiltinFunctionSQRT: return _sqrt(result);
		case BuiltinFunctionSTDEV: return _stdev(result);
		case BuiltinFunctionSUM: return _sum(result);
		case BuiltinFunctionUNIQUE: return _unique(result);
		case BuiltinFunctionVAR: return _var(result);
		case BuiltinFunctionLENGTH: return _length(result);
		case BuiltinFunctionLENGTHSQ: return _lengthsq(result);
		case BuiltinFunctionNORMALIZE: return _normalize(result);
		default: return RuntimeErrorNotImplemented;
	}
}

/*static const char * builtinVariables[] =
{
	"pi", "tau", "e", "inf",
	
	"cam_x", "cam_y", "cam_scale", "cam_rot",
	"mouse_x", "mouse_y", "mouse_dx", "mouse_dy", "mouse_btn", "mouse_clk,"
	"time",
};*/
