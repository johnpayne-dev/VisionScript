
static const char * frag_Points =
"#version 330\n"

"in vec4 vertexColor;"
"in float vertexSize;"

"out vec4 fragColor;"

"void main()"
"{"
"	vec2 uv = 2.0 * gl_PointCoord.xy - 1.0;"
"	fragColor = vertexColor;"
"	fragColor.a = smoothstep(1.0, 1.0 - 4.0 / vertexSize, length(uv));"
"}";
