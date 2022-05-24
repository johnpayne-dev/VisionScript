#include <stdio.h>
#include <stdlib.h>

#include "Renderer/Renderer.h"
#include "REPL.h"

#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include <sokol_app.h>

sapp_desc sokol_main(int argc, char * argv[]) {
	if (argc < 2) {
		RunREPL();
		exit(0);
	}
	printf("%s %s\n", argv[0], argv[1]);
	
	char * code =
		"n = [1 ~ 1000]^2\n"
		"parametric P(t) = sum((cos(n*t), sin(n*t))/n)\n"
		"P:domain = [0,2*pi]";
	
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
		"parametric border(t) = [(-1,t), (t,1), (t,-1), (1,t)]\n"
		"border:domain = [-1,1]\n"
		"Tx(p) = dot(p.xy - Cp.xy, (Cc.x,-Cs.x))\n"
		"Ty(p) = dot(p.yx - Cp.yx, (Cc.x,Cs.x))*Cc.y + (p.z - Cp.z)*Cs.y\n"
		"Tz(p) = (p.z - Cp.z)*Cc.y - dot(p.yx - Cp.yx, (Cc.x,Cs.x))*Cs.y\n"
		"T(p) = (Tx(p),Tz(p))/Ty(p)\n"
		"S = (s/n)*[-n ~ n]\n"
		"t = (s/n)*[(-n,-n) ~ (n,n)]\n"
		//"points graph = T((t.x,t.y,f(t.x,t.y)))";
		"parametric L1(t) = T((s*t,S,f(s*t,S)))\n"
		"parametric L2(t) = T((S,s*t,f(S,s*t)))\n"
		"L1:domain = [-1,1]\n"
		"L2:domain = [-1,1]";
	
	char * code3 =
		"f(x,y) = (cos(x - y - time), sin(x + y + time * 0.9))\n"
		"s = pi\n"
		"n = 40\n"
		"A = (s/n)*[(-n,-n) ~ (n,n)]\n"
		"B = A + (s/n)*f(A.x,A.y)\n"
		"parametric P(t) = (B - A)*t + A";
	
	char * code4 =
		"M(a,b) = (a.x*b.x - a.y*b.y, a.x*b.y + b.x*a.y)\n"
		"f(x,y) = M((x,y), (x,y))\n"
		"n = 32\n"
		"s = 1\n"
		"S = (s/n)*[-n ~ n]\n"
		"w = 1\n"
		"parametric L1(t) = w*f((2*t - 1)*s, S) + (1 - w)*((2*t - 1)*s, S)\n"
		"parametric L2(t) = w*f(S, (2*t - 1)*s) + (1 - w)*(S, (2*t - 1)*s)\n";
	
	//"parametric p(t) = (t*cos(1000*pi*t),t*sin(1000*pi*t))"
	Script script = LoadScript(code2);
	return RenderScript(script, true);
}

