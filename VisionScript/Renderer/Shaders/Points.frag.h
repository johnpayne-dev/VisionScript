
static const char * frag_Points =
"#version 450\n"

"layout (location = 0) in vec4 vertexColor;"

"layout (location = 0) out vec4 fragColor;"

"void main()"
"{"
"	vec2 uv = 2.0 * gl_PointCoord.xy - 1.0;"
"	fragColor = vertexColor;"
"	if (length(uv) > 1.0) { fragColor.a = 0.0; }"
"}";
