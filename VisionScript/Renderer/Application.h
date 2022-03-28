#ifndef Application_h
#define Application_h

#include <stdint.h>

typedef struct WindowConfig
{
	int32_t width;
	int32_t height;
	const char * title;
	void (*startup)(void);
	void (*update)(void);
	void (*render)(void);
	void (*resize)(int32_t width, int32_t height);
	void (*mouseDragged)(float x, float y, float dx, float dy);
	void (*scrollWheel)(float x, float y, float ds);
	void (*shutdown)(void);
} AppConfig;

void RunApplication(AppConfig config);

float ApplicationDPIScale(void);

const void * ApplicationMacOSView(void);

#endif
