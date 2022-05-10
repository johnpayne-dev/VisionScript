
static const char * vert_Polygons =
"#version 330\n"

"layout (location = 0) in vec2 position;"
"layout (location = 1) in vec4 color;"

"uniform struct Camera"
"{"
"	mat4 matrix;"
"} camera;"

"out vec4 vertexColor;"

"void main()"
"{"
"	vertexColor = color;"
"	gl_Position = camera.matrix * vec4(position, 0.0, 1.0);"
"}";
