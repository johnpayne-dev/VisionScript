
static const char * vert_Grid2D =
"#version 450\n"

"layout (location = 0) in vec2 position;"

"layout (location = 0) out vec2 uv;"

"void main()"
"{"
"	uv = position;"
"	gl_Position = vec4(position, 0.0, 1.0);"
"}";
