#ifndef Application_h
#define Application_h

#include <stdint.h>

typedef struct WindowConfig
{
	int32_t width;
	int32_t height;
	const char * title;
} WindowConfig;

void RunApplication(WindowConfig config);

#endif
