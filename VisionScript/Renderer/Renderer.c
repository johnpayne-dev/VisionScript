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
	bool debug;
	int32_t width, height;
} renderer = { 0 };

Pipeline pipeline;
VertexBuffer buffer;

static void Startup()
{
	GraphicsInitialize(renderer.width, renderer.height);
	
	VertexAttribute attributes[] = { VertexAttributeVec2 };
	VertexLayout layout = VertexLayoutCreate(sizeof(attributes) / sizeof(attributes[0]), attributes);
	
	buffer = VertexBufferCreate(layout, 3);
	vec2_t * vertices = VertexBufferMapVertices(buffer);
	vertices[0] = (vec2_t){ cosf(0.0), sinf(0.0) };
	vertices[1] = (vec2_t){ cosf(2 * M_PI / 3), sinf(2 * M_PI / 3) };
	vertices[2] = (vec2_t){ cosf(4 * M_PI / 3), sinf(4 * M_PI / 3) };
	VertexBufferUnmapVertices(buffer);
	VertexBufferUpload(buffer);
	
#include "Shaders/Polygon2D.vert.h"
#include "Shaders/Polygon2D.frag.h"
	Shader vert = ShaderCompile(ShaderTypeVertex, vert_Polygon2D);
	Shader frag = ShaderCompile(ShaderTypeFragment, frag_Polygon2D);
	
	PipelineConfig config =
	{
		.polygonMode = PolygonModeFill,
		.primitive = VertexPrimitiveTriangleList,
		.shaderCount = 2,
		.shaders = { vert, frag },
		.vertexLayout = layout,
	};
	pipeline = PipelineCreate(config);
	PipelineSetUniformMember(pipeline, "properties", 0, "color", &(vec3_t){ 0.0, 0.0, 1.0 });
}

static void Update()
{
	GraphicsUpdate();
	PipelineSetUniformMember(pipeline, "camera", 0, "aspectRatio", &(float){ (float)renderer.width / renderer.height });
	PipelineSetUniformMember(pipeline, "camera", 0, "matrix", &mat4_identity);
}

static void Render()
{
	GraphicsBegin();
	GraphicsClearColor(1.0, 1.0, 1.0, 1.0);
	GraphicsBindPipeline(pipeline);
	GraphicsRenderVertexBuffer(buffer);
	GraphicsEnd();
}

static void Resize(int32_t width, int32_t height)
{
	renderer.width = width;
	renderer.height = height;
	GraphicsRecreateSwapchain(width, height);
}

static void Shutdown()
{
	GraphicsShutdown();
}

void RenderScript(Script * script, RendererType type, bool debug)
{
	renderer.script = script;
	renderer.type = type;
	renderer.debug = debug;
	renderer.camera._2d = (Camera2D){ .scale = vec2_one };
	renderer.width = 1080;
	renderer.height = 720;
	
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
