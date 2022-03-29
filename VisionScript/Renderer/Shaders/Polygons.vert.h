
static const char * vert_Polygons =
"#version 450\n"

"layout (location = 0) in vec2 position;"
"layout (location = 1) in vec4 color;"

"layout (binding = 0) uniform Camera"
"{"
"	mat4 matrix;"
"} camera;"

"layout (location = 0) out vec4 vertexColor;"

"void main()"
"{"
"	vertexColor = color;"
"	gl_Position = camera.matrix * vec4(position, 0.0, 1.0);"
"}";
