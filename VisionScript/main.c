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
	
	/*char * code =
		"f(x,y) = (cos(x - y), sin(x + y))\n"
		"s = 2*pi\n"
		"n = 40\n"
		"A = (s/n)*[(i,j) for i = [-n...n] for j = [-n...n]]\n"
		"B = A + (s/n)*f(A.x,A.y)\n"
		"parametric (B - A)*t + A\n";*/
	
	// char * code = "polygon [(sin(0),cos(0)), (sin(tau/3),cos(tau/3)), (sin(2*tau/3),cos(2*tau/3))]";
	
	char * code =
		"n = 10\n"
		"v1 = [(0,0) for i = [1...n]]\n"
		"v2 = [(cos(i), sin(i)) for i = 2*pi*[0...n-1]/n]\n"
		"v3 = [(cos(i), sin(i)) for i = 2*pi*[1...n]/n]\n"
		"polygon join(v1, v2, v3)";
	
	Script * script = LoadScript(code, VARIABLE_LIMIT);
	RenderScript(script, RendererType2D, true); // doesn't return
	
	return 0;
}
