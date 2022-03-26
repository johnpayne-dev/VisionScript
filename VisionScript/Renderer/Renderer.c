#include "Application.h"
#include "Camera.h"
#include "Renderer.h"
#include "Backend/Graphics.h"
#include "Backend/Pipeline.h"

static struct Renderer
{
	Script * script;
	RendererType type;
	union { Camera2D _2d; Camera3D _3d; } camera;
	bool testMode;
	int32_t width, height;
	VertexLayout layout2d;
	VertexBuffer quad;
} renderer = { 0 };

static struct Pipelines
{
	Pipeline polygon2d;
	Pipeline grid;
} pipelines;

static void CreatePipelines()
{
#include "Shaders/Polygon2D.vert.h"
#include "Shaders/Polygon2D.frag.h"
	Shader vert = ShaderCompile(ShaderTypeVertex, vert_Polygon2D);
	Shader frag = ShaderCompile(ShaderTypeFragment, frag_Polygon2D);
	PipelineConfig config =
	{
		.alphaBlend = true,
		.polygonMode = PolygonModeFill,
		.primitive = VertexPrimitiveTriangleList,
		.shaderCount = 2,
		.shaders = { vert, frag },
		.vertexLayout = renderer.layout2d,
	};
	pipelines.polygon2d = PipelineCreate(config);
	PipelineSetUniformMember(pipelines.polygon2d, "properties", 0, "color", &(vec3_t){ 0.0, 0.0, 1.0 });
	
#include "Shaders/Grid2D.vert.h"
#include "Shaders/Grid2D.frag.h"
	vert = ShaderCompile(ShaderTypeVertex, vert_Grid2D);
	frag = ShaderCompile(ShaderTypeFragment, frag_Grid2D);
	config = (PipelineConfig)
	{
		.alphaBlend = true,
		.polygonMode = PolygonModeFill,
		.primitive = VertexPrimitiveTriangleList,
		.shaderCount = 2,
		.shaders = { vert, frag },
		.vertexLayout = renderer.layout2d,
	};
	pipelines.grid = PipelineCreate(config);
	PipelineSetUniformMember(pipelines.grid, "properties", 0, "axisColor", &(vec4_t){ 0.0, 0.0, 0.0, 2.0 / 3.0 });
	PipelineSetUniformMember(pipelines.grid, "properties", 0, "axisWidth", &(float){ 0.003 });
	PipelineSetUniformMember(pipelines.grid, "properties", 0, "majorColor", &(vec4_t){ 0.0, 0.0, 0.0, 1.0 / 3.0 });
	PipelineSetUniformMember(pipelines.grid, "properties", 0, "majorWidth", &(float){ 0.002 });
	PipelineSetUniformMember(pipelines.grid, "properties", 0, "minorColor", &(vec4_t){ 0.0, 0.0, 0.0, 1.0 / 6.0 });
	PipelineSetUniformMember(pipelines.grid, "properties", 0, "minorWidth", &(float){ 0.0015 });
}

static void Startup()
{
	GraphicsInitialize(renderer.width, renderer.height);
	
	VertexAttribute attributes[] = { VertexAttributeVec2 };
	renderer.layout2d = VertexLayoutCreate(sizeof(attributes) / sizeof(attributes[0]), attributes);
	
	renderer.quad = VertexBufferCreate(renderer.layout2d, 6);
	vec2_t * vertices = VertexBufferMapVertices(renderer.quad);
	vertices[0] = (vec2_t){ -1.0, -1.0 };
	vertices[1] = (vec2_t){ 1.0, -1.0 };
	vertices[2] = (vec2_t){ 1.0, 1.0 };
	vertices[3] = (vec2_t){ -1.0, -1.0 };
	vertices[4] = (vec2_t){ 1.0, 1.0 };
	vertices[5] = (vec2_t){ -1.0, 1.0 };
	VertexBufferUnmapVertices(renderer.quad);
	VertexBufferUpload(renderer.quad);
	
	CreatePipelines();
}

static void Update()
{
	GraphicsUpdate();
	mat4_t matrix = Camera2DTransform(renderer.camera._2d);
	PipelineSetUniformMember(pipelines.polygon2d, "camera", 0, "matrix", &matrix);
	matrix = Camera2DInverseTransform(renderer.camera._2d);
	PipelineSetUniformMember(pipelines.grid, "camera", 0, "invMatrix", &matrix);
	PipelineSetUniformMember(pipelines.grid, "camera", 0, "scale", &renderer.camera._2d.scale);
	PipelineSetUniformMember(pipelines.grid, "camera", 0, "dimensions", &(vec2_t){ renderer.width, renderer.height });
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
	GraphicsEnd();
}

static void Resize(int32_t width, int32_t height)
{
	renderer.width = width;
	renderer.height = height;
	renderer.camera._2d.aspectRatio = (float)width / height;
	GraphicsRecreateSwapchain(width, height);
}

static void Shutdown()
{
	GraphicsShutdown();
}

void RenderScript(Script * script, RendererType type, bool testMode)
{
	renderer.script = script;
	renderer.type = type;
	renderer.testMode = testMode;
	renderer.width = 1080;
	renderer.height = 720;
	renderer.camera._2d = (Camera2D){ .scale = vec2_one, .aspectRatio = (float)renderer.width / renderer.height };
	
	AppConfig config =
	{
		.width = renderer.width,
		.height = renderer.height,
		.title = "VisionScript",
		.startup = Startup,
		.update = Update,
		.render = Render,
		.resize = Resize,
		.shutdown = Shutdown,
	};
	RunApplication(config);
}
