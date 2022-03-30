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
		.polygonMode = PolygonModeLine,
		.primitive = VertexPrimitiveLineStrip,
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
	
	matrix = CameraInverseMatrix(renderer.camera);
	PipelineSetUniformMember(pipelines.grid, "camera", 0, "invMatrix", &matrix);
	PipelineSetUniformMember(pipelines.grid, "camera", 0, "scale", &renderer.camera.scale);
	PipelineSetUniformMember(pipelines.grid, "camera", 0, "dimensions", &(vec2_t){ renderer.width, renderer.height });
}

static void Startup()
{
	GraphicsInitialize(renderer.width, renderer.height, 4);
	
	VertexAttribute attributes[] = { VertexAttributeVec2, VertexAttributeVec4, VertexAttributeFloat };
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
	
	renderer.buffers = ListCreate(sizeof(VertexBuffer), 1);
	renderer.renderTypes = ListCreate(sizeof(StatementRenderType), 1);
	for (int32_t i = 0; i < ListLength(renderer.script->renderList); i++)
	{
		if (renderer.script->renderList[i]->declaration.render.type == StatementRenderTypePolygons)
		{
			VertexBuffer buffer;
			RuntimeError error = SamplePolygons(renderer.script, renderer.script->renderList[i], &buffer);
			if (error.code != RuntimeErrorNone) { continue; }
			renderer.buffers = ListPush(renderer.buffers, &buffer);
			renderer.renderTypes = ListPush(renderer.renderTypes, &(StatementRenderType){ StatementRenderTypePolygons });
		}
		if (renderer.script->renderList[i]->declaration.render.type == StatementRenderTypePoints)
		{
			VertexBuffer buffer;
			RuntimeError error = SamplePoints(renderer.script, renderer.script->renderList[i], &buffer);
			if (error.code != RuntimeErrorNone) { continue; }
			renderer.buffers = ListPush(renderer.buffers, &buffer);
			renderer.renderTypes = ListPush(renderer.renderTypes, &(StatementRenderType){ StatementRenderTypePoints });
		}
		if (renderer.script->renderList[i]->declaration.render.type == StatementRenderTypeParametric)
		{
			VertexBuffer buffer;
			RuntimeError error = SampleParametric(renderer.script, renderer.script->renderList[i], 0, 1, &buffer);
			if (error.code != RuntimeErrorNone) { continue; }
			renderer.buffers = ListPush(renderer.buffers, &buffer);
			renderer.renderTypes = ListPush(renderer.renderTypes, &(StatementRenderType){ StatementRenderTypeParametric });
		}
	}
	
	CreatePipelines();
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
	for (int32_t i = 0; i < ListLength(renderer.buffers); i++)
	{
		if (renderer.renderTypes[i] == StatementRenderTypePolygons) { GraphicsBindPipeline(pipelines.polygons); }
		if (renderer.renderTypes[i] == StatementRenderTypePoints) { GraphicsBindPipeline(pipelines.points); }
		if (renderer.renderTypes[i] == StatementRenderTypeParametric) { GraphicsBindPipeline(pipelines.parametric); }
		GraphicsRenderVertexBuffer(renderer.buffers[i]);
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
