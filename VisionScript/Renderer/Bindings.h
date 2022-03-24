#ifndef Bindings_h
#define Bindings_h

#ifdef __APPLE__

#include <objc/runtime.h>

typedef enum NSApplicationActivationPolicy
{
	NSApplicationActivationPolicyRegular    = 0,
	NSApplicationActivationPolicyAccessory  = 1,
	NSApplicationActivationPolicyProhibited = 2,
} NSApplicationActivationPolicy;

typedef enum NSWindowStyleMask
{
	NSWindowStyleMaskBorderless             = 0,
	NSWindowStyleMaskTitled                 = 1 << 0,
	NSWindowStyleMaskClosable               = 1 << 1,
	NSWindowStyleMaskMiniaturizable         = 1 << 2,
	NSWindowStyleMaskResizable              = 1 << 3,
	NSWindowStyleMaskUnifiedTitleAndToolbar = 1 << 12,
	NSWindowStyleMaskFullScreen API_AVAILABLE(macos(10.7)) = 1 << 14,
} NSWindowStyleMask;

typedef enum NSBackingStoreType
{
	NSBackingStoreBuffered = 2,
} NSBackingStoreType;

typedef id NSRunLoopMode;

 extern NSRunLoopMode const NSDefaultRunLoopMode;
 extern NSRunLoopMode NSEventTrackingRunLoopMode;

#endif

#endif
