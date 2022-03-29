
static const char * vert_Points2D =
"#version 450\n"

"layout (location = 0) in vec2 point;"
"layout (location = 1) in vec4 color;"
"layout (location = 2) in float size;"

"layout (binding = 0) uniform Camera"
"{"
"	mat4 matrix;"
"} camera;"

"layout (location = 0) out vec4 vertexColor;"

"void main()"
"{"
"	vertexColor = color;"
"	gl_Position = camera.matrix * vec4(point, 0.0, 1.0);"
"	gl_PointSize = size;"
"}";
