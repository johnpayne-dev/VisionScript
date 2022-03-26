
static const char * vert_Polygon2D =
"#version 450\n"

"layout (location = 0) in vec2 position;"

"layout (binding = 0) uniform Camera"
"{"
"	mat4 matrix;"
"} camera;"

"void main()"
"{"
"	gl_Position = camera.matrix * vec4(position, 0.0, 1.0);"
"}";
