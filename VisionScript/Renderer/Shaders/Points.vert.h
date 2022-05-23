
static const char * vert_Points =
"#version 330\n"

"layout (location = 0) in vec2 point;"
"layout (location = 1) in vec4 color;"
"layout (location = 2) in float size;"

"uniform struct Camera {"
"	mat4 matrix;"
"	vec2 dimensions;"
"} camera;"

"out vec4 vertexColor;"
"out float vertexSize;"

"void main() {"
"	vertexColor = color;"
"	gl_Position = camera.matrix * vec4(point, 0.0, 1.0);"
"	gl_PointSize = size;"
"	vertexSize = size;"
"}";
