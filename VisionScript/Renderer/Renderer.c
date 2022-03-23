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
} renderer = { 0 };

static void Startup()
{
	InitializeGraphicsBackend();
}

static void Update()
{
}

static void Render()
{
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
	
	AppConfig config =
	{
		.width = 1080,
		.height = 720,
		.title = "VisionScript",
		.startup = Startup,
		.update = Update,
		.render = Render,
		.shutdown = Shutdown,
	};
	RunApplication(config);
}
