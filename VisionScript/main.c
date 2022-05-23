#include <stdio.h>
#include <stdlib.h>

#include "Renderer/Renderer.h"
#include "REPL.h"

#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include <sokol_app.h>

sapp_desc sokol_main(int argc, char * argv[])
{
	if (argc < 2)
	{
		RunREPL();
		exit(0);
	}
	printf("%s %s\n", argv[0], argv[1]);
	
	char * code =
		"n = [1...1000]^2\n"
		"a = (time / 10) % 3 + 1\n"
		"P(t) = sum((cos(n*t), sin(n*t))/n)\n"
		"parametric P(2*pi*t)";
	
	char * code2 =
		"f(x,y) = sin(x) * cos(y)\n"
		"n = 100\n"
		"s = pi\n"
		"Ca = ((time / 10) % (2*pi),0.47)\n"
		"Cr = 6\n"
		"Cro = (Ca.x + pi/2,Ca.y)\n"
		"Cs = -sin(Cro)\n"
		"Cc = cos(Cro)\n"
		"Cp = (Cr*cos(Ca.y)*cos(Ca.x),Cr*cos(Ca.y)*sin(Ca.x),Cr*sin(Ca.y))\n"
		"parametric 2*[(0,t), (t,1), (t,0), (1,t)] - 1\n"
		"Tx(p) = dot(p.xy - Cp.xy, (Cc.x,-Cs.x))\n"
		"Ty(p) = dot(p.yx - Cp.yx, (Cc.x,Cs.x))*Cc.y + (p.z - Cp.z)*Cs.y\n"
		"Tz(p) = (p.z - Cp.z)*Cc.y - dot(p.yx - Cp.yx, (Cc.x,Cs.x))*Cs.y\n"
		"T(p) = (Tx(p),Tz(p))/Ty(p)\n"
		"S = (s/n)*[-n...n]\n"
		"t = (s/n)*[(i,j) for i = [-n...n] for j = [-n...n]]\n"
		"points T((t.x,t.y,f(t.x,t.y)))\n";
		//"parametric T(((2*t-1)*s,S,f((2*t-1)*s,S)))\n"
		//"parametric T((S,(2*t-1)*s,f(S,(2*t-1)*s)))";
	
	char * code3 =
		"f(x,y) = (cos(x - y - time), sin(x + y + time * 0.9))\n"
		"s = pi\n"
		"n = 40\n"
		"A = (s/n)*[(i,j) for i = [-n...n] for j = [-n...n]]\n"
		"B = A + (s/n)*f(A.x,A.y)\n"
		"parametric (B - A)*t + A\n";
	
	char * code4 =
		"M(a,b) = (a.x*b.x - a.y*b.y, a.x*b.y + b.x*a.y)\n"
		"f(x,y) = M((x,y), (x,y) + 1)\n"
		"n = 32\n"
		"s = 1\n"
		"S = (s/n)*[-n...n]\n"
		"w = 1\n"
		"parametric w*f((2*t - 1)*s, S) + (1 - w)*((2*t - 1)*s, S)\n"
		"parametric w*f(S, (2*t - 1)*s) + (1 - w)*(S, (2*t - 1)*s)\n";
	
	//"parametric (t*cos(1000*pi*t),t*sin(1000*pi*t))"
	Script script = LoadScript(code2);
	return RenderScript(script, true);
}

