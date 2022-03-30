
static const char * frag_Points =
"#version 450\n"

"layout (location = 0) in vec4 vertexColor;"
"layout (location = 1) in float vertexSize;"

"layout (location = 0) out vec4 fragColor;"

"void main()"
"{"
"	vec2 uv = 2.0 * gl_PointCoord.xy - 1.0;"
"	fragColor = vertexColor;"
"	fragColor.a = smoothstep(1.0, 1.0 - 4.0 / vertexSize, length(uv));"
"}";
