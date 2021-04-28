#ifndef __TRACE_H
#define __TRACE_H
#include <stdio.h>
#include "windows.h"
#include <assert.h>

#define ENABLE_TRACE

void Log(char *str);	// in Timing.cpp

#ifdef ENABLE_TRACE
#ifdef _DEBUG
#define ENC_TRACE(...) { \
	char msg[4096]; \
	sprintf_s(msg,4096, __VA_ARGS__); \
	Log(msg); \
	OutputDebugString(msg); \
}
#else
#define ENC_TRACE(...) { \
	char msg[4096]; \
	sprintf_s(msg,4096, __VA_ARGS__); \
	Log(msg); \
}
#endif
#else
#define ENC_TRACE(...)
#endif

#ifdef _DEBUG
#define ENC_ASSERT(x) assert((x))
#else
#define ENC_ASSERT(x)
#endif


#endif