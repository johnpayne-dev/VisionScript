#include "Application.h"

#ifdef _WIN32

void RunApplication(int32_t width, int32_t height, const char * title)
{
}

#endif

#ifdef __APPLE__

#include <objc/runtime.h>
#include <objc/message.h>
#include <Carbon/Carbon.h>

typedef enum NSApplicationActivationPolicy
{
	NSApplicationActivationPolicyRegular    = 0,
	NSApplicationActivationPolicyAccessory  = 1,
	NSApplicationActivationPolicyProhibited = 2,
} NSApplicationActivationPolicy;

typedef enum NSWindowStyleMask
{
	NSWindowStyleMaskBorderless = 0,
	NSWindowStyleMaskTitled = 1 << 0,
	NSWindowStyleMaskClosable = 1 << 1,
	NSWindowStyleMaskMiniaturizable = 1 << 2,
	NSWindowStyleMaskResizable = 1 << 3,
	NSWindowStyleMaskUnifiedTitleAndToolbar = 1 << 12,
	NSWindowStyleMaskFullScreen API_AVAILABLE(macos(10.7)) = 1 << 14,
} NSWindowStyleMask;

typedef enum NSBackingStoreType
{
	NSBackingStoreBuffered = 2,
} NSBackingStoreType;

static Class AppDelegateClass;
typedef struct AppDelegate
{
	Class isa;
	id app;
	id window;
} AppDelegate;

static Class WindowDelegateClass;
typedef struct WindowDelegate
{
	Class isa;
	AppDelegate * appDelegate;
} WindowDelegate;

static void AppDelegate_applicationDidFinishLaunching(AppDelegate * self, SEL method, id notification)
{
	// WindowDelegate * windowDelegate = [[WindowDelegate alloc] init];
	WindowDelegate * delegate = ((WindowDelegate * (*)(id, SEL))objc_msgSend)(((id (*)(Class, SEL))objc_msgSend)(WindowDelegateClass, sel_getUid("alloc")), sel_getUid("init"));
	delegate->appDelegate = self;
	// [self->window setDelegate:windowDelegate];
	((void (*)(id, SEL, id))objc_msgSend)(self->window, sel_getUid("setDelegate:"), (id)delegate);
}

static void AppDelegate_applicationWillTerminate(AppDelegate * self, SEL method, id notification)
{
}

static void CreateAppDelegateClass()
{
	AppDelegateClass = objc_allocateClassPair(objc_getClass("NSObject"), "AppDelegate", 0);
	class_addMethod(AppDelegateClass, sel_registerName("applicationDidFinishLaunching:"), (IMP)AppDelegate_applicationDidFinishLaunching, "v@:@");
	class_addMethod(AppDelegateClass, sel_registerName("applicationWillTerminate:"), (IMP)AppDelegate_applicationWillTerminate, "v@:@");
	objc_registerClassPair(AppDelegateClass);
}

static void WindowDelegate_windowWillClose(WindowDelegate * self, SEL method, id notification)
{
	// [self->appDelegate->app terminate];
	((void (*)(id, SEL, id))objc_msgSend)(self->appDelegate->app, sel_getUid("terminate:"), nil);
}

static void CreateWindowDelegateClass()
{
	WindowDelegateClass = objc_allocateClassPair(objc_getClass("NSObject"), "WindowDelegate", 0);
	class_addMethod(WindowDelegateClass, sel_registerName("windowWillClose:"), (IMP)WindowDelegate_windowWillClose, "v@:@");
	objc_registerClassPair(WindowDelegateClass);
}

void RunApplication(int32_t width, int32_t height, const char * title)
{
	CreateAppDelegateClass();
	CreateWindowDelegateClass();
	
	// AppDelegate * delegate = [[AppDelegate alloc] init];
	AppDelegate * delegate = ((AppDelegate * (*)(id, SEL))objc_msgSend)(((id (*)(Class, SEL))objc_msgSend)(AppDelegateClass, sel_getUid("alloc")), sel_getUid("init"));
	
	// delegate->app = [NSApplication sharedApplication];
	// [delegate->app setActivationPolicy:NSActivationPolicyRegular];
	delegate->app = ((id (*)(Class, SEL))objc_msgSend)(objc_getClass("NSApplication"), sel_getUid("sharedApplication"));
	((void (*)(id, SEL, NSApplicationActivationPolicy))objc_msgSend)(delegate->app, sel_getUid("setActivationPolicy:"), NSApplicationActivationPolicyRegular);
	
	// CGRect screenFrame = [[NSScreen mainScreen] frame];
	CGRect screenFrame = ((CGRect (*)(id, SEL))objc_msgSend)(((id (*)(Class, SEL))objc_msgSend)(objc_getClass("NSScreen"), sel_getUid("mainScreen")), sel_getUid("frame"));
	CGRect dimensions = { screenFrame.size.width / 2 - width / 2, screenFrame.size.height / 2 - height / 2, width, height };
	NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
	
	// delegate->window = [NSWindow alloc];
	// delegate->window = [delegate->window initWithContentRect:dimensions styleMask:style backing:NSBackingStoreBuffered defer:NO];
	// [delegate->window setTitle:[NSString stringWithUTF8String:"VisionScript"]];
	// [delegate->window makeKeyAndOrderFront:nil];
	delegate->window = ((id (*)(Class, SEL))objc_msgSend)(objc_getClass("NSWindow"), sel_getUid("alloc"));
	delegate->window = ((id (*)(id, SEL, CGRect, NSWindowStyleMask, NSBackingStoreType, bool))objc_msgSend)(delegate->window, sel_getUid("initWithContentRect:styleMask:backing:defer:"), dimensions, style, NSBackingStoreBuffered, false);
	((void (*)(id, SEL, id))objc_msgSend)(delegate->window, sel_getUid("setTitle:"), ((id (*)(Class, SEL, const char *))objc_msgSend)(objc_getClass("NSString"), sel_getUid("stringWithUTF8String:"), title));
	((void (*)(id, SEL, id))objc_msgSend)(delegate->window, sel_getUid("makeKeyAndOrderFront:"), nil);
	
	// [app activateIgnoringOtherApps:YES];
	// [app setDelegate:delegate];
	// [app run];
	((void (*)(id, SEL, bool))objc_msgSend)(delegate->app, sel_getUid("activateIgnoringOtherApps:"), true);
	((void (*)(id, SEL, id))objc_msgSend)(delegate->app, sel_getUid("setDelegate:"), (id)delegate);
	((void (*)(id, SEL))objc_msgSend)(delegate->app, sel_getUid("run"));
}

#endif

#ifdef __linux__

void RunApplication(int32_t width, int32_t height, const char * title)
{
}

#endif
