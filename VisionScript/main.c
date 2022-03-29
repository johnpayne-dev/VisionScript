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
		"f(x,y) = (cos(x - y), sin(x + y))\n"
		"s = 2*pi\n"
		"n = 100\n"
		"A = (s/n)*[(i,j) for i = [-n...n] for j = [-n...n]]\n"
		"B = A + (s/n)*f(A.x,A.y)\n"
		"points join(A, (A+B)/2, B)";
	
	Script * script = LoadScript(code, VARIABLE_LIMIT);
	RenderScript(script, RendererType2D, true); // doesn't return
	
	return 0;
}
