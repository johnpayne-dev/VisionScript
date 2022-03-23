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
	void (*shutdown)(void);
} AppConfig;

void RunApplication(AppConfig config);

#endif
