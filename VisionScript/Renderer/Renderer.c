#include "Application.h"
#include "Camera.h"
#include "Renderer.h"
#include "Backend.h"

static struct Renderer
{
	Script * script;
	RendererType type;
	union { Camera2D _2d; Camera3D _3d; } camera;
	bool debug;
	int32_t width, height;
} renderer = { 0 };

static void Startup()
{
	InitializeGraphicsBackend(renderer.width, renderer.height);
}

static void Update()
{
	UpdateBackend();
}

static void Render()
{
	AquireNextSwapchainImage();
	PresentSwapchainImage();
}

static void Shutdown()
{
	
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
		.shutdown = Shutdown,
	};
	RunApplication(config);
}
