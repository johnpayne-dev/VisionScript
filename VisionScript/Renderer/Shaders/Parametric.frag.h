
static const char * frag_Parametric =
"#version 330\n"

"in vec4 vertexColor;"
"in vec2 fragPosition;"
"in struct Segment"
"{"
"	vec2 center;"
"	vec2 size;"
"	vec2 dir;"
"} segment;"

"uniform struct Camera"
"{"
"	mat4 matrix;"
"	vec2 dimensions;"
"} camera;"

"out vec4 fragColor;"

"void main()"
"{"
"	fragColor = vertexColor;"
"	float projy = dot(fragPosition - segment.center, segment.dir);"
"	float projx = dot(fragPosition - segment.center, vec2(segment.dir.y, -segment.dir.x));"
"	fragColor.a *= 1.0 - smoothstep(-2.0 / (segment.size.x * camera.dimensions.y), 0.0, (abs(projx) - segment.size.x) / segment.size.x);"
"	if (abs(projy) > segment.size.y)"
"	{"
"		vec2 endPoint = segment.center + segment.dir * segment.size.y * sign(projy);"
"		fragColor.a *= 1.0 - smoothstep(1.0 - 2.0 / (segment.size.x * camera.dimensions.y), 1.0, length(fragPosition - endPoint) / segment.size.x);"
"	}"
"}";
