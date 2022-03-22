#include "Application.h"
#include "Bindings.h"

#ifdef _WIN32

void RunApplication(WindowConfig config)
{
}

#endif

#ifdef __APPLE__

#include <objc/message.h>
#include <Carbon/Carbon.h>
#include <OpenGL/GL.h>

static id app;
static id window;
static id view;
static WindowConfig config;
static id timer;

static Class AppDelegateClass;
static Class WindowDelegateClass;
static Class OpenGLViewClass;

static void AppDelegate_applicationDidFinishLaunching(id self, SEL method, id notification)
{
	// create window delegate
	// id delegate = [[WindowDelegate alloc] init];
	id delegate = ((id (*)(id, SEL))objc_msgSend)(((id (*)(Class, SEL))objc_msgSend)(WindowDelegateClass, sel_getUid("alloc")), sel_getUid("init"));
	
	// initialize the window
	// window = [NSWindow alloc];
	window = ((id (*)(Class, SEL))objc_msgSend)(objc_getClass("NSWindow"), sel_getUid("alloc"));
	// window = [window initWithContentRect:dimensions styleMask:style backing:NSBackingStoreBuffered defer:NO];
	CGRect dimensions = { 0, 0, config.width, config.height };
	NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
	window = ((id (*)(id, SEL, CGRect, NSWindowStyleMask, NSBackingStoreType, bool))objc_msgSend)(window, sel_getUid("initWithContentRect:styleMask:backing:defer:"), dimensions, style, NSBackingStoreBuffered, false);
	// [window setTitle:[NSString stringWithUTF8String:self->config.title]];
	((void (*)(id, SEL, id))objc_msgSend)(window, sel_getUid("setTitle:"), ((id (*)(Class, SEL, const char *))objc_msgSend)(objc_getClass("NSString"), sel_getUid("stringWithUTF8String:"), config.title));
	// [window center];
	((void (*)(id, SEL))objc_msgSend)(window, sel_getUid("center"));
	// [window makeKeyAndOrderFront:nil];
	((void (*)(id, SEL, id))objc_msgSend)(window, sel_getUid("makeKeyAndOrderFront:"), nil);
	// [window setDelegate:delegate];
	((void (*)(id, SEL, id))objc_msgSend)(window, sel_getUid("setDelegate:"), (id)delegate);
	

	// initialize the OpenGLView
	NSOpenGLPixelFormatAttribute attributes[] =
	{
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFABackingStore,
		NSOpenGLPFAClosestPolicy,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
		NSOpenGLPFAStencilSize, 8,
		0,
	};
	// id format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	id format = ((id (*)(id, SEL, NSOpenGLPixelFormatAttribute *))objc_msgSend)(((id (*)(Class, SEL))objc_msgSend)(objc_getClass("NSOpenGLPixelFormat"), sel_getUid("alloc")), sel_getUid("initWithAttributes:"), attributes);
	// view = [OpenGLView alloc];
	view = ((id (*)(Class, SEL))objc_msgSend)(OpenGLViewClass, sel_getUid("alloc"));
	// view = [view initWithFrame:dimensions pixelFormat:format];
	view = ((id (*)(id, SEL, CGRect, id))objc_msgSend)(view, sel_getUid("initWithFrame:pixelFormat:"), dimensions, format);
	// [view setWantsBestResolutionOpenGLSurface:YES];
	((void (*)(id, SEL, bool))objc_msgSend)(view, sel_getUid("setWantsBestResolutionOpenGLSurface:"), true);
	
	// set the window's view
	// [window setContentView:view];
	((void (*)(id, SEL, id))objc_msgSend)(window, sel_getUid("setContentView:"), view);
	
	// setup the timer to update every frame
	// timer = [NSTimer timerWithTimeInterval:0.001 target:view selector:@selector(timerFired) userInfo:nil repeats:YES];
	timer = ((id (*)(Class, SEL, double, id, SEL, id, bool))objc_msgSend)(objc_getClass("NSTimer"), sel_getUid("timerWithTimeInterval:target:selector:userInfo:repeats:"), 0.001, view, sel_getUid("timerFired"), nil, true);
	// id runLoop = [NSRunLoop currentRunLoop];
	id runLoop = ((id (*)(Class, SEL))objc_msgSend)(objc_getClass("NSRunLoop"), sel_getUid("currentRunLoop"));
	// [runLoop addTimer:timer forMode:NSDefaultRunLoopMode];
	((void (*)(id, SEL, id, NSRunLoopMode))objc_msgSend)(runLoop, sel_getUid("addTimer:forMode:"), timer, NSDefaultRunLoopMode);
}

static void AppDelegate_applicationWillTerminate(id self, SEL method, id notification)
{
}

static void CreateAppDelegateClass()
{
	AppDelegateClass = objc_allocateClassPair(objc_getClass("NSObject"), "AppDelegate", 0);
	class_addMethod(AppDelegateClass, sel_registerName("applicationDidFinishLaunching:"), (IMP)AppDelegate_applicationDidFinishLaunching, "v@:@");
	class_addMethod(AppDelegateClass, sel_registerName("applicationWillTerminate:"), (IMP)AppDelegate_applicationWillTerminate, "v@:@");
	objc_registerClassPair(AppDelegateClass);
}

static void WindowDelegate_windowWillClose(id self, SEL method, id notification)
{
	// terminate the app upon window being closed
	// [app terminate];
	((void (*)(id, SEL, id))objc_msgSend)(app, sel_getUid("terminate:"), nil);
}

static void CreateWindowDelegateClass()
{
	WindowDelegateClass = objc_allocateClassPair(objc_getClass("NSObject"), "WindowDelegate", 0);
	class_addMethod(WindowDelegateClass, sel_registerName("windowWillClose:"), (IMP)WindowDelegate_windowWillClose, "v@:@");
	objc_registerClassPair(WindowDelegateClass);
}

static void OpenGLView_drawRect(id self, SEL method, CGRect bounds)
{
	// flush buffer
	// [[view openGLContext] flushBuffer];
	((void (*)(id, SEL))objc_msgSend)(((id (*)(id, SEL))objc_msgSend)(view, sel_getUid("openGLContext")), sel_getUid("flushBuffer"));
}

static void OpenGLView_timerFired(id self, SEL method)
{
	// [self setNeedsDisplay:YES];
	((void (*)(id, SEL, bool))objc_msgSend)(self, sel_getUid("setNeedsDisplay:"), true);
}

static void CreateOpenGLViewClass()
{
	OpenGLViewClass = objc_allocateClassPair(objc_getClass("NSOpenGLView"), "OpenGLView", 0);
	class_addMethod(OpenGLViewClass, sel_registerName("drawRect:"), (IMP)OpenGLView_drawRect, "v@:{size={width=f,height=f},origin={x=f,y=f}}");
	class_addMethod(OpenGLViewClass, sel_registerName("timerFired"), (IMP)OpenGLView_timerFired, "v@:");
	objc_registerClassPair(OpenGLViewClass);
}

void RunApplication(WindowConfig windowConfig)
{
	// initialize the classes
	CreateAppDelegateClass();
	CreateWindowDelegateClass();
	CreateOpenGLViewClass();
	
	// create the AppDelegate and set global window config
	// id delegate = [[AppDelegate alloc] init];
	id delegate = ((id (*)(id, SEL))objc_msgSend)(((id (*)(Class, SEL))objc_msgSend)(AppDelegateClass, sel_getUid("alloc")), sel_getUid("init"));
	config = windowConfig;
	
	// initialize the app and run it
	// app = [NSApplication sharedApplication];
	app = ((id (*)(Class, SEL))objc_msgSend)(objc_getClass("NSApplication"), sel_getUid("sharedApplication"));
	// [app setActivationPolicy:NSActivationPolicyRegular];
	((void (*)(id, SEL, NSApplicationActivationPolicy))objc_msgSend)(app, sel_getUid("setActivationPolicy:"), NSApplicationActivationPolicyRegular);
	// [app activateIgnoringOtherApps:YES];
	((void (*)(id, SEL, bool))objc_msgSend)(app, sel_getUid("activateIgnoringOtherApps:"), true);
	// [app setDelegate:delegate];
	((void (*)(id, SEL, id))objc_msgSend)(app, sel_getUid("setDelegate:"), (id)delegate);
	// [app run];
	((void (*)(id, SEL))objc_msgSend)(app, sel_getUid("run"));
}

#endif

#ifdef __linux__

void RunApplication(WindowConfig config)
{
}

#endif
