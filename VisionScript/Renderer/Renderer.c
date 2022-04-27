#include <stdio.h>
#include "Backend/Graphics.h"
#include "Backend/Pipeline.h"
#include "Application.h"
#include "Renderer.h"
#include "Sampler.h"

#define SCROLL_ZOOM_FACTOR 100.0

struct Renderer renderer = { 0 };

static struct Pipelines
{
	Pipeline polygons;
	Pipeline grid;
	Pipeline points;
	Pipeline parametric;
} pipelines;

static void CreatePipelines()
{
#include "Shaders/Polygons.vert.h"
#include "Shaders/Polygons.frag.h"
	Shader vert = ShaderCompile(ShaderTypeVertex, vert_Polygons);
	Shader frag = ShaderCompile(ShaderTypeFragment, frag_Polygons);
	PipelineConfig config =
	{
		.alphaBlend = true,
		.polygonMode = PolygonModeFill,
		.primitive = VertexPrimitiveTriangleList,
		.shaderCount = 2,
		.shaders = { vert, frag },
		.vertexLayout = renderer.layout,
	};
	pipelines.polygons = PipelineCreate(config);
	
#include "Shaders/Grid.vert.h"
#include "Shaders/Grid.frag.h"
	vert = ShaderCompile(ShaderTypeVertex, vert_Grid);
	frag = ShaderCompile(ShaderTypeFragment, frag_Grid);
	config = (PipelineConfig)
	{
		.alphaBlend = true,
		.polygonMode = PolygonModeFill,
		.primitive = VertexPrimitiveTriangleList,
		.shaderCount = 2,
		.shaders = { vert, frag },
		.vertexLayout = renderer.layout,
	};
	pipelines.grid = PipelineCreate(config);
	PipelineSetUniformMember(pipelines.grid, "properties", 0, "axisColor", &(vec4_t){ 0.0, 0.0, 0.0, 2.0 / 3.0 });
	PipelineSetUniformMember(pipelines.grid, "properties", 0, "axisWidth", &(float){ 0.003 });
	PipelineSetUniformMember(pipelines.grid, "properties", 0, "majorColor", &(vec4_t){ 0.0, 0.0, 0.0, 1.0 / 3.0 });
	PipelineSetUniformMember(pipelines.grid, "properties", 0, "majorWidth", &(float){ 0.002 });
	PipelineSetUniformMember(pipelines.grid, "properties", 0, "minorColor", &(vec4_t){ 0.0, 0.0, 0.0, 1.0 / 6.0 });
	PipelineSetUniformMember(pipelines.grid, "properties", 0, "minorWidth", &(float){ 0.0015 });
	
#include "Shaders/Points.vert.h"
#include "Shaders/Points.frag.h"
	vert = ShaderCompile(ShaderTypeVertex, vert_Points);
	frag = ShaderCompile(ShaderTypeFragment, frag_Points);
	config = (PipelineConfig)
	{
		.alphaBlend = true,
		.polygonMode = PolygonModePoint,
		.primitive = VertexPrimitivePointList,
		.shaderCount = 2,
		.shaders = { vert, frag },
		.vertexLayout = renderer.layout,
	};
	pipelines.points = PipelineCreate(config);
	
#include "Shaders/Parametric.vert.h"
#include "Shaders/Parametric.frag.h"
	vert = ShaderCompile(ShaderTypeVertex, vert_Parametric);
	frag = ShaderCompile(ShaderTypeFragment, frag_Parametric);
	config = (PipelineConfig)
	{
		.alphaBlend = true,
		.polygonMode = PolygonModeFill,
		.primitive = VertexPrimitiveTriangleList,
		.shaderCount = 2,
		.shaders = { vert, frag },
		.vertexLayout = renderer.layout,
	};
	pipelines.parametric = PipelineCreate(config);
}

static void UpdatePipelines()
{
	mat4_t matrix = CameraMatrix(renderer.camera);
	PipelineSetUniformMember(pipelines.polygons, "camera", 0, "matrix", &matrix);
	
	PipelineSetUniformMember(pipelines.points, "camera", 0, "matrix", &matrix);
	PipelineSetUniformMember(pipelines.points, "camera", 0, "dimensions", &(vec2_t){ renderer.width, renderer.height });
	
	PipelineSetUniformMember(pipelines.parametric, "camera", 0, "matrix", &matrix);
	PipelineSetUniformMember(pipelines.parametric, "camera", 0, "dimensions", &(vec2_t){ renderer.width, renderer.height });
	
	matrix = CameraInverseMatrix(renderer.camera);
	PipelineSetUniformMember(pipelines.grid, "camera", 0, "invMatrix", &matrix);
	PipelineSetUniformMember(pipelines.grid, "camera", 0, "scale", &renderer.camera.scale);
	PipelineSetUniformMember(pipelines.grid, "camera", 0, "dimensions", &(vec2_t){ renderer.width, renderer.height });
}

static void UpdateBuiltins(float dt)
{
	((VectorArray *)HashMapGet(renderer.script->cache, "time"))->xyzw[0][0] += dt;
	InvalidateCachedDependents(renderer.script, "time");
	InvalidateDependentRenders(renderer.script, "time");
	
	Statement * statement = HashMapGet(renderer.script->identifiers, "position");
	if (statement != NULL)
	{
		VectorArray result;
		if (EvaluateExpression(renderer.script->identifiers, renderer.script->cache, NULL, statement->expression, &result).code == RuntimeErrorNone)
		{
			renderer.camera.position.x = result.xyzw[0][0];
			renderer.camera.position.y = result.xyzw[1][0];
		}
	}
	else
	{
		VectorArray * position = HashMapGet(renderer.script->cache, "position");
		if (position->xyzw[0][0] != renderer.camera.position.x || position->xyzw[1][0] != renderer.camera.position.y)
		{
			position->xyzw[0][0] = renderer.camera.position.x;
			position->xyzw[1][0] = renderer.camera.position.y;
			InvalidateCachedDependents(renderer.script, "position");
			InvalidateDependentRenders(renderer.script, "position");
			InvalidateParametricRenders(renderer.script);
		}
	}
	statement = HashMapGet(renderer.script->identifiers, "scale");
	if (statement != NULL)
	{
		VectorArray result;
		if (EvaluateExpression(renderer.script->identifiers, renderer.script->cache, NULL, statement->expression, &result).code == RuntimeErrorNone)
		{
			renderer.camera.scale.x = result.xyzw[0][0];
			renderer.camera.scale.y = result.xyzw[1][0];
		}
	}
	else
	{
		VectorArray * scale = HashMapGet(renderer.script->cache, "scale");
		if (scale->xyzw[0][0] != renderer.camera.scale.x || scale->xyzw[1][0] != renderer.camera.scale.y)
		{
			scale->xyzw[0][0] = renderer.camera.scale.x;
			scale->xyzw[1][0] = renderer.camera.scale.y;
			InvalidateCachedDependents(renderer.script, "scale");
			InvalidateDependentRenders(renderer.script, "scale");
			InvalidateParametricRenders(renderer.script);
		}
	}
	statement = HashMapGet(renderer.script->identifiers, "rotation");
	if (statement != NULL)
	{
		VectorArray result;
		if (EvaluateExpression(renderer.script->identifiers, renderer.script->cache, NULL, statement->expression, &result).code == RuntimeErrorNone)
		{
			renderer.camera.angle = result.xyzw[0][0];
		}
	}
	else
	{
		VectorArray * rotation = HashMapGet(renderer.script->cache, "rotation");
		if (rotation->xyzw[0][0] != renderer.camera.angle)
		{
			rotation->xyzw[0][0] = renderer.camera.angle;
			InvalidateCachedDependents(renderer.script, "rotation");
			InvalidateDependentRenders(renderer.script, "rotation");
			InvalidateParametricRenders(renderer.script);
		}
	}
}

static void UpdateSamples()
{
	// update samples
	Camera camera = renderer.camera;
	for (int32_t i = 0; i < ListLength(renderer.objects); i++)
	{
		if (ListContains(renderer.script->dirtyRenders, &(Statement *){ renderer.objects[i].statement }))
		{
			RuntimeError error = { RuntimeErrorNone };
			if (renderer.objects[i].statement->declaration.render.type == StatementRenderTypePoints)
			{
				error = SamplePoints(renderer.script, renderer.script->renderList[i], &renderer.objects[i].buffer);
			}
			if (renderer.objects[i].statement->declaration.render.type == StatementRenderTypePolygons)
			{
				error = SamplePolygons(renderer.script, renderer.script->renderList[i], &renderer.objects[i].buffer);
			}
			if (renderer.objects[i].statement->declaration.render.type == StatementRenderTypeParametric)
			{
				error = SampleParametric(renderer.script, renderer.script->renderList[i], 0, 1, camera, &renderer.objects[i].buffer);
			}
			if (error.code != RuntimeErrorNone) { PrintRuntimeError(error, renderer.objects[i].statement); }
		}
	}
	// upload buffers
	for (int32_t i = 0; i < ListLength(renderer.objects); i++)
	{
		if (ListContains(renderer.script->dirtyRenders, &(Statement *){ renderer.objects[i].statement }))
		{
			VertexBufferUpload(renderer.objects[i].buffer);
			renderer.script->dirtyRenders = ListRemoveAll(renderer.script->dirtyRenders, &(Statement *){ renderer.objects[i].statement });
		}
	}
}

static void * UpdateThread(void * arg)
{
	clock_t start = clock();
	while (renderer.samplerRunning)
	{
		UpdateBuiltins((clock() - start) / (float)CLOCKS_PER_SEC);
		start = clock();
		UpdateSamples();
	}
	return NULL;
}

static void Startup()
{
	GraphicsInitialize(renderer.width, renderer.height, 4);
	
	VertexAttribute attributes[] = { VertexAttributeVec2, VertexAttributeVec4, VertexAttributeFloat, VertexAttributeVec2 };
	renderer.layout = VertexLayoutCreate(sizeof(attributes) / sizeof(attributes[0]), attributes);
	
	renderer.quad = VertexBufferCreate(renderer.layout, 6);
	vertex_t * vertices = VertexBufferMapVertices(renderer.quad);
	vertices[0].position = (vec2_t){ -1.0, -1.0 };
	vertices[1].position = (vec2_t){ 1.0, -1.0 };
	vertices[2].position = (vec2_t){ 1.0, 1.0 };
	vertices[3].position = (vec2_t){ -1.0, -1.0 };
	vertices[4].position = (vec2_t){ 1.0, 1.0 };
	vertices[5].position = (vec2_t){ -1.0, 1.0 };
	VertexBufferUnmapVertices(renderer.quad);
	VertexBufferUpload(renderer.quad);
	
	renderer.objects = ListCreate(sizeof(RenderObject), 1);
	for (int32_t i = 0; i < ListLength(renderer.script->renderList); i++)
	{
		renderer.objects = ListPush(renderer.objects, &(RenderObject){ .statement = renderer.script->renderList[i] });
	}
	
	CreatePipelines();
	
	renderer.samplerRunning = true;
	pthread_create(&renderer.samplerThread, NULL, UpdateThread, NULL);
}

static void Update()
{
	GraphicsUpdate();
	UpdatePipelines();
}

static void Render()
{
	GraphicsBegin();
	GraphicsClearColor(1.0, 1.0, 1.0, 1.0);
	if (renderer.testMode)
	{
		GraphicsBindPipeline(pipelines.grid);
		GraphicsRenderVertexBuffer(renderer.quad);
	}
	for (int32_t i = 0; i < ListLength(renderer.objects); i++)
	{
		if (renderer.objects[i].statement->declaration.render.type == StatementRenderTypePolygons) { GraphicsBindPipeline(pipelines.polygons); }
		if (renderer.objects[i].statement->declaration.render.type == StatementRenderTypePoints) { GraphicsBindPipeline(pipelines.points); }
		if (renderer.objects[i].statement->declaration.render.type == StatementRenderTypeParametric) { GraphicsBindPipeline(pipelines.parametric); }
		if (renderer.objects[i].buffer.vertexCount > 0) { GraphicsRenderVertexBuffer(renderer.objects[i].buffer); }
	}
	GraphicsEnd();
}

static void Resize(int32_t width, int32_t height)
{
	renderer.width = width;
	renderer.height = height;
	renderer.camera.aspectRatio = (float)width / height;
	GraphicsRecreateSwapchain(width, height);
}

static void MouseDragged(float x, float y, float dx, float dy)
{
	vec4_t dir = mat4_mulv(mat4_rotate_z(mat4_identity, -renderer.camera.angle), (vec4_t){ dx, dy, 0.0, 1.0 });
	renderer.camera.position.x -= dir.x / (renderer.width * renderer.camera.scale.x) * renderer.camera.aspectRatio;
	renderer.camera.position.y -= dir.y / (renderer.height * renderer.camera.scale.y);
}

static void ScrollWheel(float x, float y, float ds)
{
	renderer.camera.scale.x *= 1.0 + ds / SCROLL_ZOOM_FACTOR;
	renderer.camera.scale.y *= 1.0 + ds / SCROLL_ZOOM_FACTOR;
	vec4_t mousePos = { 2.0 * x / renderer.width - 1.0, -(2.0 * y / renderer.height - 1.0), 0.0, 1.0 };
	vec4_t camMousePos = mat4_mulv(CameraInverseMatrix(renderer.camera), mousePos);
	vec2_t diff = vec2_sub(vec4_to_vec2(camMousePos), renderer.camera.position);
	renderer.camera.position = vec2_add(renderer.camera.position, vec2_mulf(diff, ds / SCROLL_ZOOM_FACTOR));
}

static void Shutdown()
{
	renderer.samplerRunning = false;
	pthread_join(renderer.samplerThread, NULL);
	GraphicsShutdown();
}

void RenderScript(Script * script, bool testMode)
{
	renderer.script = script;
	renderer.testMode = testMode;
	renderer.width = 1080;
	renderer.height = 720;
	renderer.camera = (Camera){ .scale = vec2_one, .aspectRatio = (float)renderer.width / renderer.height };
	
	AppConfig config =
	{
		.width = renderer.width,
		.height = renderer.height,
		.title = "VisionScript",
		.startup = Startup,
		.update = Update,
		.render = Render,
		.resize = Resize,
		.mouseDragged = MouseDragged,
		.scrollWheel = ScrollWheel,
		.shutdown = Shutdown,
	};
	RunApplication(config);
}
