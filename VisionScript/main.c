#include <stdio.h>
#include "Script.h"

int main(int argc, const char * argv[])
{
	if (argc < 2)
	{
		printf("Invalid arguments.\n");
		return 0;
	}
	printf("%s %s\n", argv[0], argv[1]);
	
	char * code =
		"f(x,y) = (cos(x - y), sin(x + y))\n"
		"s = 2*pi\n"
		"n = 40\n"
		"A = (s/n)*[2*(i, j) - 1 for i = [0...n] for j = [0...n]]\n"
		"B = A + (s/n)*f(A.x,A.y)\n"
		"parametric ((B.x - A.x)*t + A.x, (B.y - A.y)*t + A.y)\n";
	
	Script * script = LoadScript(code);
	DestroyScript(script);
	
	return 0;
}
