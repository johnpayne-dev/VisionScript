
static const char * frag_Grid =
"#version 450\n"

"layout (location = 0) in vec2 uv;"

"layout (binding = 0) uniform Camera"
"{"
"	mat4 invMatrix;"
"	vec2 scale;"
"	vec2 dimensions;"
"} camera;"

"layout (binding = 1) uniform Properties"
"{"
"	vec4 axisColor;"
"	vec4 majorColor;"
"	vec4 minorColor;"
"	float axisWidth;"
"	float majorWidth;"
"	float minorWidth;"
"} properties;"

"layout (location = 0) out vec4 fragColor;"

"void main()"
"{"
"	vec2 pos = (camera.invMatrix * vec4(uv, 0.0, 1.0)).xy;"
"	vec2 spacing = pow(vec2(2.0), floor(log2(camera.scale)));"
"	vec2 majorLine = round(pos * 4.0 * spacing) / (4.0 * spacing);"
"	vec2 minorLine = round(pos * 16.0 * spacing) / (16.0 * spacing);"
"	float heightScale = 720.0 / camera.dimensions.y;"
"	vec2 axisWidth = heightScale * properties.axisWidth / camera.scale;"
"	vec2 majorWidth = heightScale * properties.majorWidth / camera.scale;"
"	vec2 minorWidth = heightScale * properties.minorWidth / camera.scale;"
"	fragColor = vec4(0.0);"
"	if (abs(pos.x - minorLine.x) < minorWidth.x || abs(pos.y - minorLine.y) < minorWidth.x) { fragColor = properties.minorColor; }"
"	if (abs(pos.x - majorLine.x) < majorWidth.x || abs(pos.y - majorLine.y) < majorWidth.y) { fragColor = properties.majorColor; }"
"	if (abs(pos.x) < axisWidth.x || abs(pos.y) < axisWidth.y) { fragColor = properties.axisColor; }"
"}";

