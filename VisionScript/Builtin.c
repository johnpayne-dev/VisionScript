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
	"cov", "erf", "exp", "factorial", "floor", "gamma",
	"gcd", "join", "lcm", "len", "ln", "log",
	"log10", "log2", "max", "mean", "median", "min",
	"prod", "quantile", "rand", "round", "shuffle", "sign",
	"sort", "sqrt", "stdev", "sum", "unique", "var",
	
	"cross", "dist", "distsq", "dot", "magn", "magnsq", "normalize",
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
	BuiltinFunctionCBRT,
	BuiltinFunctionCEIL,
	BuiltinFunctionERF,
	BuiltinFunctionEXP,
	BuiltinFunctionFACTORIAL,
	BuiltinFunctionGAMMA,
	BuiltinFunctionLN,
	BuiltinFunctionLOG10,
	BuiltinFunctionLOG2,
	BuiltinFunctionROUND,
	BuiltinFunctionSIGN,
	BuiltinFunctionSQRT,
};

static RuntimeError SingleArgument(BuiltinFunction function, VectorArray * result)
{
	for (int8_t d = 0; d < result->dimensions; d++)
	{
		for (int32_t i = 0; i < result->length; i++)
		{
			switch (function)
			{
				case BuiltinFunctionSIN: result->xyzw[d][i] = sinf(result->xyzw[d][i]); break;
				case BuiltinFunctionCOS: result->xyzw[d][i] = cosf(result->xyzw[d][i]); break;
				case BuiltinFunctionTAN: result->xyzw[d][i] = tanf(result->xyzw[d][i]); break;
				case BuiltinFunctionASIN: result->xyzw[d][i] = asinf(result->xyzw[d][i]); break;
				case BuiltinFunctionACOS: result->xyzw[d][i] = acosf(result->xyzw[d][i]); break;
				case BuiltinFunctionATAN: result->xyzw[d][i] = atanf(result->xyzw[d][i]); break;
				case BuiltinFunctionSEC: result->xyzw[d][i] = 1.0 / cosf(result->xyzw[d][i]); break;
				case BuiltinFunctionCSC: result->xyzw[d][i] = 1.0 / sinf(result->xyzw[d][i]); break;
				case BuiltinFunctionCOT: result->xyzw[d][i] = 1.0 / tanf(result->xyzw[d][i]); break;
				case BuiltinFunctionASEC: result->xyzw[d][i] = acosf(1.0 / result->xyzw[d][i]); break;
				case BuiltinFunctionACSC: result->xyzw[d][i] = asinf(1.0 / result->xyzw[d][i]); break;
				case BuiltinFunctionACOT: result->xyzw[d][i] = M_PI_2 - atanf(result->xyzw[d][i]); break;
				case BuiltinFunctionSINH: result->xyzw[d][i] = sinhf(result->xyzw[d][i]); break;
				case BuiltinFunctionCOSH: result->xyzw[d][i] = coshf(result->xyzw[d][i]); break;
				case BuiltinFunctionTANH: result->xyzw[d][i] = tanhf(result->xyzw[d][i]); break;
				case BuiltinFunctionASINH: result->xyzw[d][i] = asinhf(result->xyzw[d][i]); break;
				case BuiltinFunctionACOSH: result->xyzw[d][i] = acoshf(result->xyzw[d][i]); break;
				case BuiltinFunctionATANH: result->xyzw[d][i] = atanhf(result->xyzw[d][i]); break;
				case BuiltinFunctionSECH: result->xyzw[d][i] = 1.0 / coshf(result->xyzw[d][i]); break;
				case BuiltinFunctionCSCH: result->xyzw[d][i] = 1.0 / sinhf(result->xyzw[d][i]); break;
				case BuiltinFunctionCOTH: result->xyzw[d][i] = 1.0 / tanhf(result->xyzw[d][i]); break;
				case BuiltinFunctionASECH: result->xyzw[d][i] = acoshf(1.0 / result->xyzw[d][i]); break;
				case BuiltinFunctionACSCH: result->xyzw[d][i] = asinhf(1.0 / result->xyzw[d][i]); break;
				case BuiltinFunctionACOTH: result->xyzw[d][i] = atanhf(1.0 / result->xyzw[d][i]); break;
				case BuiltinFunctionABS: result->xyzw[d][i] = fabsf(result->xyzw[d][i]); break;
				case BuiltinFunctionCBRT: result->xyzw[d][i] = cbrtf(result->xyzw[d][i]); break;
				case BuiltinFunctionCEIL: result->xyzw[d][i] = ceilf(result->xyzw[d][i]); break;
				case BuiltinFunctionERF: result->xyzw[d][i] = erff(result->xyzw[d][i]); break;
				case BuiltinFunctionEXP: result->xyzw[d][i] = expf(result->xyzw[d][i]); break;
				case BuiltinFunctionFACTORIAL: result->xyzw[d][i] = tgammaf(result->xyzw[d][i] + 1.0); break;
				case BuiltinFunctionGAMMA: result->xyzw[d][i] = tgammaf(result->xyzw[d][i]); break;
				case BuiltinFunctionLN: result->xyzw[d][i] = logf(result->xyzw[d][i]); break;
				case BuiltinFunctionLOG10: result->xyzw[d][i] = log10f(result->xyzw[d][i]); break;
				case BuiltinFunctionLOG2: result->xyzw[d][i] = log2f(result->xyzw[d][i]); break;
				case BuiltinFunctionROUND: result->xyzw[d][i] = roundf(result->xyzw[d][i]); break;
				case BuiltinFunctionSIGN: result->xyzw[d][i] = result->xyzw[d][i] > 0.0 ? 1.0 : (result->xyzw[d][i] < 0.0 ? -1.0 : 0.0); break;
				case BuiltinFunctionSQRT: result->xyzw[d][i] = sqrtf(result->xyzw[d][i]); break;
				default: break;
			}
		}
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
	if (IsFunctionSingleArgument(function)) { return SingleArgument(function, result); }
	return RuntimeErrorNotImplemented;
}

/*static const char * builtinVariables[] =
{
	"pi", "tau", "e", "inf",
	
	"cam_x", "cam_y", "cam_scale", "cam_rot",
	"mouse_x", "mouse_y", "mouse_dx", "mouse_dy", "mouse_btn", "mouse_clk,"
	"time",
};*/
