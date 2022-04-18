
static const char * frag_Parametric =
"#version 450\n"

"layout (location = 0) in vec4 vertexColor;"
"layout (location = 1) in vec2 fragPosition;"
"layout (location = 2) in struct Segment"
"{"
"	vec2 center;"
"	vec2 size;"
"	vec2 dir;"
"} segment;"

"layout (binding = 0) uniform Camera"
"{"
"	mat4 matrix;"
"	vec2 dimensions;"
"} camera;"

"layout (location = 0) out vec4 fragColor;"

"void main()"
"{"
"	fragColor = vertexColor;"
"	float proj = dot(fragPosition - segment.center, segment.dir);"
"	if (abs(proj) > segment.size.y)"
"	{"
"		vec2 endPoint = segment.center + segment.dir * segment.size.y * sign(proj);"
"		fragColor.a = smoothstep(1.0, 1.0 - 0.002 / segment.size.x, length(fragPosition - endPoint) / segment.size.x);"
"	}"
"}";
