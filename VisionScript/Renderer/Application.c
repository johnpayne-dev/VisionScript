#include "Application.h"
#include "Bindings.h"

#ifdef _WIN32

void RunApplication(AppConfig config)
{
}

float ApplicationDPIScale()
{
	return 1.0;
}

#endif

#ifdef __APPLE__

#include <objc/runtime.h>
#include <objc/message.h>
#include <Carbon/Carbon.h>
#include <CoreVideo/CoreVideo.h>

static id app;
static id window;
static id view;
static AppConfig config;
static id timer;

static Class AppDelegateClass;
static Class WindowDelegateClass;
static Class ViewClass;

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
	// [window setAcceptsMouseMovedEvents:YES];
	((void (*)(id, SEL, bool))objc_msgSend)(window, sel_getUid("setAcceptsMouseMovedEvents:"), true);
	
	// initialize the view assign it to the window
	// view = [[View alloc] initWithFrame:dimensions];
	view = ((id (*)(id, SEL, CGRect))objc_msgSend)(((id (*)(Class, SEL))objc_msgSend)(ViewClass, sel_getUid("alloc")), sel_getUid("initWithFrame:"), dimensions);
	// [view setWantsLayer:YES];
	((void (*)(id, SEL, bool))objc_msgSend)(view, sel_getUid("setWantsLayer:"), true);
	// [window setContentView:view];
	((void (*)(id, SEL, id))objc_msgSend)(window, sel_getUid("setContentView:"), view);
	// [window makeFirstResponder:view];
	((bool (*)(id, SEL, id))objc_msgSend)(window, sel_getUid("makeFirstResponder:"), view);
	
	if (config.startup != NULL) { config.startup(); }
	
	// setup the timer to update every frame
	// timer = [NSTimer timerWithTimeInterval:0.001 target:view selector:@selector(timerFired) userInfo:nil repeats:YES];
	timer = ((id (*)(Class, SEL, double, id, SEL, id, bool))objc_msgSend)(objc_getClass("NSTimer"), sel_getUid("timerWithTimeInterval:target:selector:userInfo:repeats:"), 0.001, view, sel_getUid("timerFired"), nil, true);
		// id runLoop = [NSRunLoop currentRunLoop];
	id runLoop = ((id (*)(Class, SEL))objc_msgSend)(objc_getClass("NSRunLoop"), sel_getUid("currentRunLoop"));
	// [runLoop addTimer:timer forMode:NSDefaultRunLoopMode];
	((void (*)(id, SEL, id, NSRunLoopMode))objc_msgSend)(runLoop, sel_getUid("addTimer:forMode:"), timer, NSDefaultRunLoopMode);
	// [runLoop addTimer:timer forMode:NSEventTrackingRunLoopMode]; //Ensure timer fires during resize
	((void (*)(id, SEL, id, NSRunLoopMode))objc_msgSend)(runLoop, sel_getUid("addTimer:forMode:"), timer, NSEventTrackingRunLoopMode);
}

static bool AppDelegate_applicationShouldTerminateAfterLastWindowClosed(id self, SEL method, id sender)
{
	return true;
}

static void AppDelegate_applicationWillTerminate(id self, SEL method, id notification)
{
	if (config.shutdown != NULL) { config.shutdown(); }
}

static void CreateAppDelegateClass()
{
	AppDelegateClass = objc_allocateClassPair(objc_getClass("NSObject"), "AppDelegate", 0);
	class_addMethod(AppDelegateClass, sel_registerName("applicationWillTerminate:"), (IMP)AppDelegate_applicationWillTerminate, "v@:@");
	class_addMethod(AppDelegateClass, sel_registerName("applicationDidFinishLaunching:"), (IMP)AppDelegate_applicationDidFinishLaunching, "v@:@");
	class_addMethod(AppDelegateClass, sel_registerName("applicationShouldTerminateAfterLastWindowClosed:"), (IMP)AppDelegate_applicationShouldTerminateAfterLastWindowClosed, "i@:@");
	objc_registerClassPair(AppDelegateClass);
}

static void WindowDelegate_windowDidResize(id self, SEL method, id notification)
{
	// CGRect rect = [view frame];
	CGRect rect = ((CGRect (*)(id, SEL))objc_msgSend)(view, sel_getUid("frame"));
	if (config.resize != NULL) { config.resize(rect.size.width, rect.size.height); }
}

static void CreateWindowDelegateClass()
{
	WindowDelegateClass = objc_allocateClassPair(objc_getClass("NSObject"), "WindowDelegate", 0);
	class_addMethod(WindowDelegateClass, sel_registerName("windowDidResize:"), (IMP)WindowDelegate_windowDidResize, "v@:@");
	objc_registerClassPair(WindowDelegateClass);
}

static void View_timerFired(id self, SEL method)
{
	if (config.update != NULL) { config.update(); }
	if (config.render != NULL) { config.render(); }
}

static CGPoint lastMousePosition = { 0, 0 };

static void View_mouseDown(id self, SEL method, id event)
{
	// lastMousePosition = [NSEvent mouseLocation];
	lastMousePosition = ((CGPoint (*)(Class, SEL))objc_msgSend)(objc_getClass("NSEvent"), sel_getUid("mouseLocation"));
	// CGRect rect = [window frame];
	CGRect rect = ((CGRect (*)(id, SEL))objc_msgSend)(window, sel_getUid("frame"));
	lastMousePosition.x -= rect.origin.x;
	lastMousePosition.y -= rect.origin.y;
}

static void View_mouseDragged(id self, SEL method, id event)
{
	// CGPoint pos = [NSEvent mouseLocation];
	CGPoint pos = ((CGPoint (*)(Class, SEL))objc_msgSend)(objc_getClass("NSEvent"), sel_getUid("mouseLocation"));
	// CGRect rect = [window frame];
	CGRect rect = ((CGRect (*)(id, SEL))objc_msgSend)(window, sel_getUid("frame"));
	pos.x -= rect.origin.x;
	pos.y -= rect.origin.y;
	if (config.mouseDragged != NULL) { config.mouseDragged(pos.x, pos.y, (pos.x - lastMousePosition.x) * ApplicationDPIScale(), (pos.y - lastMousePosition.y) * ApplicationDPIScale()); }
	lastMousePosition = pos;
}

static void View_scrollWheel(id self, SEL method, id event)
{
	// CGPoint pos = [NSEvent mouseLocation];
	CGPoint pos = ((CGPoint (*)(Class, SEL))objc_msgSend)(objc_getClass("NSEvent"), sel_getUid("mouseLocation"));
	// CGRect rect = [window frame];
	CGRect rect = ((CGRect (*)(id, SEL))objc_msgSend)(window, sel_getUid("frame"));
	// float ds = [event deltaY];
	float ds = ((CGFloat (*)(id, SEL))objc_msgSend)(event, sel_getUid("deltaY"));
	if (config.scrollWheel != NULL) { config.scrollWheel(pos.x - rect.origin.x, pos.y - rect.origin.y, ds); }
}

static bool View_wantsUpdateLayer(id self, SEL method)
{
	return true;
}

static Class View_layerClass(id self, SEL method)
{
	//return [CAMetalLayer class];
	return objc_getClass("CAMetalLayer");
}

static id View_makeBackingLayer(id self, SEL mthod)
{
	// id layer = [CAMetalLayer layer];
	return ((id (*)(Class, SEL))objc_msgSend)(objc_getClass("CAMetalLayer"), sel_getUid("layer"));
}

static void CreateViewClass()
{
	ViewClass = objc_allocateClassPair(objc_getClass("NSView"), "View", 0);
	class_addMethod(ViewClass, sel_registerName("timerFired"), (IMP)View_timerFired, "v@:");
	class_addMethod(ViewClass, sel_registerName("wantsUpdateLayer"), (IMP)View_wantsUpdateLayer, "i@:");
	class_addMethod(ViewClass, sel_registerName("layerClass"), (IMP)View_layerClass, "#@:");
	class_addMethod(ViewClass, sel_registerName("makeBackingLayer"), (IMP)View_makeBackingLayer, "@@:");
	class_addMethod(ViewClass, sel_registerName("mouseDown:"), (IMP)View_mouseDown, "v@:@");
	class_addMethod(ViewClass, sel_registerName("mouseDragged:"), (IMP)View_mouseDragged, "v@:@");
	class_addMethod(ViewClass, sel_registerName("scrollWheel:"), (IMP)View_scrollWheel, "v@:@");
	objc_registerClassPair(ViewClass);
}

void RunApplication(AppConfig windowConfig)
{
	// initialize the classes
	CreateAppDelegateClass();
	CreateWindowDelegateClass();
	CreateViewClass();
	
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
	((void (*)(id, SEL, id))objc_msgSend)(app, sel_getUid("setDelegate:"), delegate);
	// [app run];
	((void (*)(id, SEL))objc_msgSend)(app, sel_getUid("run"));
}

float ApplicationDPIScale()
{
	// CGSize viewScale = [self convertSizeToBacking: CGSizeMake(1.0, 1.0)];
	CGSize viewScale = ((CGSize (*)(id, SEL, CGSize))objc_msgSend)(view, sel_getUid("convertSizeToBacking:"), CGSizeMake(1.0, 1.0));
	return fminf(viewScale.width, viewScale.height);
}

#endif

#ifdef __linux__

void RunApplication(AppConfig config)
{
}

float ApplicationDPIScale()
{
	return 1.0;
}

#endif

const void * ApplicationMacOSView()
{
#ifdef __APPLE__
	return view;
#endif
	return NULL;
}
