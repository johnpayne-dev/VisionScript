#ifndef Builtin_h
#define Builtin_h

#include "Evaluator.h"

typedef enum BuiltinFunction
{
	BuiltinFunctionSIN,
	BuiltinFunctionCOS,
	BuiltinFunctionTAN,
	BuiltinFunctionASIN,
	BuiltinFunctionACOS,
	BuiltinFunctionATAN,
	BuiltinFunctionATAN2,
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
	BuiltinFunctionCORR,
	BuiltinFunctionCOUNT,
	BuiltinFunctionCOV,
	BuiltinFunctionERF,
	BuiltinFunctionEXP,
	BuiltinFunctionFACTORIAL,
	BuiltinFunctionFLOOR,
	BuiltinFunctionGAMMA,
	BuiltinFunctionJOIN,
	BuiltinFunctionLN,
	BuiltinFunctionLOG,
	BuiltinFunctionLOG10,
	BuiltinFunctionLOG2,
	BuiltinFunctionMAX,
	BuiltinFunctionMEAN,
	BuiltinFunctionMEDIAN,
	BuiltinFunctionMIN,
	BuiltinFunctionPROD,
	BuiltinFunctionQUANTILE,
	BuiltinFunctionRAND,
	BuiltinFunctionROUND,
	BuiltinFunctionSHUFFLE,
	BuiltinFunctionSIGN,
	BuiltinFunctionSORT,
	BuiltinFunctionSQRT,
	BuiltinFunctionSTDEV,
	BuiltinFunctionSUM,
	BuiltinFunctionVAR,
	BuiltinFunctionCROSS,
	BuiltinFunctionDIST,
	BuiltinFunctionDISTSQ,
	BuiltinFunctionDOT,
	BuiltinFunctionLENGTH,
	BuiltinFunctionLENGTHSQ,
	BuiltinFunctionNORMALIZE,
	BuiltinFunctionNone,
} BuiltinFunction;

BuiltinFunction DetermineBuiltinFunction(const char * identifier);

bool IsFunctionSingleArgument(BuiltinFunction function);

RuntimeErrorCode EvaluateBuiltinFunction(BuiltinFunction function, list(VectorArray) arguments, VectorArray * result);

typedef enum BuiltinVariable
{
	BuiltinVariablePI,
	BuiltinVariableTAU,
	BuiltinVariableE,
	BuiltinVariableINF,
	BuiltinVariableNone
} BuiltinVariable;

BuiltinVariable DetermineBuiltinVariable(const char * identifier);

RuntimeErrorCode EvaluateBuiltinVariable(BuiltinVariable variable, VectorArray * result);

#endif
