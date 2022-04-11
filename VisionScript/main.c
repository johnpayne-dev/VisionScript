#include <stdio.h>
#include "Renderer/Renderer.h"
#include "REPL.h"

#define VARIABLE_LIMIT 65536

int main(int argc, const char * argv[])
{
	if (argc < 2)
	{
		RunREPL();
		return 0;
	}
	printf("%s %s\n", argv[0], argv[1]);
	
	char * code =
		"n = [1...100]\n"
		"P(t) = sum((cos(n^2*t), sin(n^2*t))/n^2)\n"
		"parametric P(2*pi*t)";
	
	Script * script = LoadScript(code, VARIABLE_LIMIT);
	RenderScript(script, true); // doesn't return
	
	return 0;
}
