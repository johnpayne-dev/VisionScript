
static const char * frag_Polygon2D =
"#version 450\n"

"layout (location = 0) out vec4 fragColor;"

"layout (binding = 1) uniform Properties"
"{"
"	vec3 color;"
"} properties;"

"void main()"
"{"
"	fragColor = vec4(properties.color, 1.0);"
"}";
