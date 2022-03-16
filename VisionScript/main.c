#include <stdio.h>
#include "Language/Script.h"
#include "CLI.h"
#define VARIABLE_LIMIT 65536

int main(int argc, const char * argv[])
{
	if (argc < 2)
	{
		CLIRun(VARIABLE_LIMIT);
		return 0;
	}
	printf("%s %s\n", argv[0], argv[1]);
	
	char * code =
		"f(x,y) = (cos(x - y), sin(x + y))\n"
		"s = 2*pi\n"
		"n = 40\n"
		"A = (s/n)*[(i,j) for i = [-n...n] for j = [-n...n]]\n"
		"B = A + (s/n)*f(A.x,A.y)\n"
		"parametric (B - A)*t + A\n";
	
	Script * script = LoadScript(code, VARIABLE_LIMIT);
	FreeScript(script);
	
	return 0;
}
