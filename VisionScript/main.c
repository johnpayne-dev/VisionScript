#include <stdio.h>
#include "Renderer/Renderer.h"
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
		"n = [1...100]\n"
		"P(t) = sum((cos(n^2*t), sin(n^2*t))/n^2)\n"
		"N = 100000\n"
		"points [P(t) for t = 2*pi*[1...N]/N]";
	
	Script * script = LoadScript(code, VARIABLE_LIMIT);
	RenderScript(script, true); // doesn't return
	
	return 0;
}
