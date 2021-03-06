#include "Renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include "Language/Evaluator.h"
#include "Sampler.h"

#define SCROLL_ZOOM_FACTOR 100.0

struct Renderer renderer = { 0 };

static struct Shaders {
	GLuint polygons;
	GLuint grid;
	GLuint points;
	GLuint parametric;
} shaders;

static GLuint CreateShaderProgram(const char * vs, const char * fs) {
	GLint success;
	char info[512];
	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, 1, &vs, NULL);
	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vert, 512, NULL, info);
		printf("%s\n", info);
		abort();
	}
	
	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &fs, NULL);
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(frag, 512, NULL, info);
		printf("%s\n", info);
		abort();
	}
	
	GLuint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(program, 512, NULL, info);
		printf("%s\n", info);
		abort();
	}
	glDeleteShader(vert);
	glDeleteShader(frag);
	
	return program;
}

static void CreateShaders() {
#include "Shaders/Grid.vert.h"
#include "Shaders/Grid.frag.h"
	shaders.grid = CreateShaderProgram(vert_Grid, frag_Grid);
#include "Shaders/Points.vert.h"
#include "Shaders/Points.frag.h"
	shaders.points = CreateShaderProgram(vert_Points, frag_Points);
#include "Shaders/Parametric.vert.h"
#include "Shaders/Parametric.frag.h"
	shaders.parametric = CreateShaderProgram(vert_Parametric, frag_Parametric);
#include "Shaders/Polygons.vert.h"
#include "Shaders/Polygons.frag.h"
	shaders.polygons = CreateShaderProgram(vert_Polygons, frag_Polygons);
}

static void UpdateUniforms() {
	mat4_t matrix = CameraMatrix(renderer.camera);
	mat4_t invMatrix = CameraInverseMatrix(renderer.camera);
	
	glUseProgram(shaders.grid);
	glUniformMatrix4fv(glGetUniformLocation(shaders.grid, "camera.invMatrix"), 1, false, (float *)&invMatrix);
	glUniform2f(glGetUniformLocation(shaders.grid, "camera.scale"), renderer.camera.scale.x, renderer.camera.scale.y);
	glUniform2f(glGetUniformLocation(shaders.grid, "camera.dimensions"), renderer.width, renderer.height);
	glUniform4f(glGetUniformLocation(shaders.grid, "properties.axisColor"), 0.0, 0.0, 0.0, 2.0 / 3.0);
	glUniform4f(glGetUniformLocation(shaders.grid, "properties.majorColor"), 0.0, 0.0, 0.0, 1.0 / 3.0);
	glUniform4f(glGetUniformLocation(shaders.grid, "properties.minorColor"), 0.0, 0.0, 0.0, 1.0 / 6.0);
	glUniform1f(glGetUniformLocation(shaders.grid, "properties.axisWidth"), 0.003);
	glUniform1f(glGetUniformLocation(shaders.grid, "properties.majorWidth"), 0.002);
	glUniform1f(glGetUniformLocation(shaders.grid, "properties.minorWidth"), 0.0015);
	
	glUseProgram(shaders.points);
	glUniformMatrix4fv(glGetUniformLocation(shaders.points, "camera.matrix"), 1, false, (float *)&matrix);
	glUniform2f(glGetUniformLocation(shaders.points, "camera.dimensions"), renderer.width, renderer.height);
	
	glUseProgram(shaders.parametric);
	glUniformMatrix4fv(glGetUniformLocation(shaders.parametric, "camera.matrix"), 1, false, (float *)&matrix);
	glUniform2f(glGetUniformLocation(shaders.parametric, "camera.dimensions"), renderer.width, renderer.height);
	
	glUseProgram(shaders.polygons);
	glUniformMatrix4fv(glGetUniformLocation(shaders.polygons, "camera.matrix"), 1, false, (float *)&matrix);
}

static void BindLayout() {
	glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(vertex_t), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, false, sizeof(vertex_t), (void *)8);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 1, GL_FLOAT, false, sizeof(vertex_t), (void *)24);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, false, sizeof(vertex_t), (void *)28);
	glEnableVertexAttribArray(3);
}

static void UpdateBuiltins(float dt) {
	GetEnvironmentCache(&renderer.script.environment, "time")->xyzw[0][0] += dt;
	InvalidateDependents(&renderer.script, "time");
	
	Equation * equation = GetEnvironmentEquation(&renderer.script.environment, "position");
	if (equation != NULL) {
		VectorArray result;
		if (EvaluateExpression(&renderer.script.environment, NULL, equation->expression, &result).code == RuntimeErrorCodeNone) {
			renderer.camera.position.x = result.xyzw[0][0];
			renderer.camera.position.y = result.xyzw[1][0];
		}
	} else {
		VectorArray * position = GetEnvironmentCache(&renderer.script.environment, "position");
		if (position->xyzw[0][0] != renderer.camera.position.x || position->xyzw[1][0] != renderer.camera.position.y) {
			position->xyzw[0][0] = renderer.camera.position.x;
			position->xyzw[1][0] = renderer.camera.position.y;
			InvalidateDependents(&renderer.script, "position");
			InvalidateParametrics(&renderer.script);
		}
	}
	equation = GetEnvironmentEquation(&renderer.script.environment, "scale");
	if (equation != NULL) {
		VectorArray result;
		if (EvaluateExpression(&renderer.script.environment, NULL, equation->expression, &result).code == RuntimeErrorCodeNone) {
			renderer.camera.scale.x = result.xyzw[0][0];
			renderer.camera.scale.y = result.xyzw[1][0];
		}
	} else {
		VectorArray * scale = GetEnvironmentCache(&renderer.script.environment, "scale");
		if (scale->xyzw[0][0] != renderer.camera.scale.x || scale->xyzw[1][0] != renderer.camera.scale.y) {
			scale->xyzw[0][0] = renderer.camera.scale.x;
			scale->xyzw[1][0] = renderer.camera.scale.y;
			InvalidateDependents(&renderer.script, "scale");
			InvalidateParametrics(&renderer.script);
		}
	}
	equation = GetEnvironmentEquation(&renderer.script.environment, "rotation");
	if (equation != NULL) {
		VectorArray result;
		if (EvaluateExpression(&renderer.script.environment, NULL, equation->expression, &result).code == RuntimeErrorCodeNone) {
			renderer.camera.angle = result.xyzw[0][0];
		}
	} else {
		VectorArray * rotation = GetEnvironmentCache(&renderer.script.environment, "position");
		if (rotation->xyzw[0][0] != renderer.camera.angle) {
			rotation->xyzw[0][0] = renderer.camera.angle;
			InvalidateDependents(&renderer.script, "rotation");
		}
	}
}

static void UpdateSamples() {
	Camera camera = renderer.camera;
	for (int32_t i = 0; i < ListLength(renderer.objects); i++) {
		RenderObject object = renderer.objects[i];
		if (ListContains(renderer.script.needsRender, &object.equation)) {
			RuntimeError error = { RuntimeErrorCodeNone };
			if (object.equation.declaration.attribute == DeclarationAttributePoints) {
				error = SamplePoints(&renderer.script, object.equation, &object);
			}
			if (object.equation.declaration.attribute == DeclarationAttributePolygons) {
				error = SamplePolygons(&renderer.script, object.equation, &object);
			}
			if (object.equation.declaration.attribute == DeclarationAttributeParametric) {
				error = SampleParametric(&renderer.script, object.equation, camera, &object);
			}
			RemoveFromRenderList(&renderer.script, object.equation);
			if (error.code != RuntimeErrorCodeNone) { PrintRuntimeError(error, renderer.script.lines); }
			renderer.objects[i] = object;
		}
	}
}

static void * UpdateThread(void * arg) {
	clock_t start = clock();
	while (renderer.samplerRunning) {
		UpdateBuiltins((clock() - start) / (float)CLOCKS_PER_SEC);
		start = clock();
		UpdateSamples();
	}
	return NULL;
}

static void Startup() {
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glGenVertexArrays(1, &renderer.layout);
	glBindVertexArray(renderer.layout);
	
	glGenBuffers(1, &renderer.quad);
	glBindBuffer(GL_ARRAY_BUFFER, renderer.quad);
	vertex_t vertices[6] = {
		{ .position = { -1.0, -1.0 } },
		{ .position = { 1.0, 1.0 } },
		{ .position = { -1.0, 1.0 } },
		{ .position = { -1.0, -1.0 } },
		{ .position = { 1.0, 1.0 } },
		{ .position = { 1.0, -1.0 } },
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	renderer.objects = ListCreate(sizeof(RenderObject), 1);
	for (int32_t i = 0; i < ListLength(renderer.script.needsRender); i++) {
		renderer.objects = ListPush(renderer.objects, &(RenderObject){ .equation = renderer.script.needsRender[i] });
		glGenBuffers(1, &renderer.objects[i].buffer);
	}
	
	CreateShaders();
	
	renderer.samplerRunning = true;
	pthread_create(&renderer.samplerThread, NULL, UpdateThread, NULL);
}

static void Frame() {
	UpdateUniforms();
	
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	if (renderer.testMode) {
		glUseProgram(shaders.grid);
		glBindBuffer(GL_ARRAY_BUFFER, renderer.quad);
		BindLayout();
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	
	for (int32_t i = 0; i < ListLength(renderer.objects); i++) {
		GLenum mode;
		switch (renderer.objects[i].equation.declaration.attribute) {
			case DeclarationAttributePoints:
				glUseProgram(shaders.points);
				mode = GL_POINTS;
				break;
			case DeclarationAttributeParametric:
				glUseProgram(shaders.parametric);
				mode = GL_TRIANGLES;
				break;
			case DeclarationAttributePolygons:
				mode = GL_TRIANGLES;
				glUseProgram(shaders.polygons);
				break;
			default:
				mode = GL_TRIANGLES;
				break;
		}
		if (renderer.objects[i].vertexCount > 0) {
			glBindBuffer(GL_ARRAY_BUFFER, renderer.objects[i].buffer);
			if (renderer.objects[i].needsUpload) {
				glBufferData(GL_ARRAY_BUFFER, renderer.objects[i].vertexCount * sizeof(vertex_t), renderer.objects[i].vertices, GL_DYNAMIC_DRAW);
				renderer.objects[i].needsUpload = false;
			}
			BindLayout();
			glDrawArrays(mode, 0, (GLsizei)renderer.objects[i].vertexCount);
		}
	}
}

static void Resize(int32_t width, int32_t height) {
	renderer.width = width;
	renderer.height = height;
	renderer.camera.aspectRatio = (float)width / height;
	glViewport(0, 0, width, height);
}

static void MouseDragged(float x, float y, float dx, float dy) {
	vec4_t dir = mat4_mulv(mat4_rotate_z(mat4_identity, -renderer.camera.angle), (vec4_t){ dx, dy, 0.0, 1.0 });
	renderer.camera.position.x -= dir.x / (renderer.width * renderer.camera.scale.x) * renderer.camera.aspectRatio;
	renderer.camera.position.y -= -dir.y / (renderer.height * renderer.camera.scale.y);
}

static void ScrollWheel(float x, float y, float ds) {
	x /= sapp_dpi_scale();
	y /= sapp_dpi_scale();
	renderer.camera.scale.x *= 1.0 + ds / SCROLL_ZOOM_FACTOR;
	renderer.camera.scale.y *= 1.0 + ds / SCROLL_ZOOM_FACTOR;
	vec4_t mousePos = { 2.0 * x / renderer.width - 1.0, -(2.0 * y / renderer.height - 1.0), 0.0, 1.0 };
	vec4_t camMousePos = mat4_mulv(CameraInverseMatrix(renderer.camera), mousePos);
	vec2_t diff = vec2_sub(vec4_to_vec2(camMousePos), renderer.camera.position);
	renderer.camera.position = vec2_add(renderer.camera.position, vec2_mulf(diff, ds / SCROLL_ZOOM_FACTOR));
}

static bool mouseDown = false;

static void Event(const sapp_event * event) {
	if (event->type == SAPP_EVENTTYPE_RESIZED) { Resize(event->window_width, event->window_height); }
	if (event->type == SAPP_EVENTTYPE_MOUSE_DOWN) { mouseDown = true; }
	if (event->type == SAPP_EVENTTYPE_MOUSE_UP) { mouseDown = false; }
	if (event->type == SAPP_EVENTTYPE_MOUSE_MOVE && mouseDown) { MouseDragged(event->mouse_x, event->mouse_y, event->mouse_dx, event->mouse_dy); }
	if (event->type == SAPP_EVENTTYPE_MOUSE_SCROLL) { ScrollWheel(event->mouse_x, event->mouse_y, event->scroll_y); }
}

static void Shutdown() {
	renderer.samplerRunning = false;
	pthread_join(renderer.samplerThread, NULL);
}

sapp_desc RenderScript(Script script, bool testMode) {
	renderer.script = script;
	renderer.testMode = testMode;
	renderer.width = 1080;
	renderer.height = 720;
	renderer.camera = (Camera){ .scale = vec2_one, .aspectRatio = (float)renderer.width / renderer.height };
	
	return (sapp_desc) {
		.width = renderer.width,
		.height = renderer.height,
		.window_title = "VisionScript",
		.high_dpi = true,
		.init_cb = Startup,
		.frame_cb = Frame,
		.event_cb = Event,
		.cleanup_cb = Shutdown,
		.sample_count = 4,
	};
}
