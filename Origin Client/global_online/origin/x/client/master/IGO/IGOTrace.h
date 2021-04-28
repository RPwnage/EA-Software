#ifndef TRACE_H
#define TRACE_H

#ifdef ORIGIN_PC
#pragma warning(disable: 4996)    // 'sprintf': name was marked as #pragma deprecated
#pragma warning(disable: 4995)    // 'wsprintfW': name was marked as #pragma deprecated
#endif

#if defined(ORIGIN_PC)
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#elif defined(ORIGIN_MAC)
#include "MacWindows.h"
#endif

#ifndef IGO_UNUSED
#define IGO_UNUSED(x) (void)x;
#endif

#ifdef ORIGIN_PC

#ifdef _DEBUG
// #define ENABLE_IGO_TRACE            // uncomment to enable IGO trace output
#endif

#define SHOW_GL_ERRORS   0
#define SHOW_IPC_MESSAGES
#define SHOW_MOUSE_MOVE_LOGIC
//#define SHOW_ANIM_ACTIVITIES

#ifdef ENABLE_IGO_TRACE
#define IGO_TRACE(x) { \
    char msg[4096]; \
    StringCbPrintfA(msg, sizeof(msg), "%s(%d): pid(%d),tid(%d) - ", __FILE__, __LINE__, GetCurrentProcessId(), GetCurrentThreadId()); \
    OutputDebugStringA(msg); \
    OutputDebugStringA(x); \
    OutputDebugStringA("\n"); \
}
#define IGO_TRACEF(...) { \
    char msg[4096]; \
    StringCbPrintfA(msg, sizeof(msg), "%s(%d): pid(%d),tid(%d) - ", __FILE__, __LINE__, GetCurrentProcessId(), GetCurrentThreadId()); \
    OutputDebugStringA(msg); \
    wsprintfA(msg, __VA_ARGS__); \
    OutputDebugStringA(msg); \
    OutputDebugStringA("\n"); \
}

#else

#define IGO_TRACE(x) (void)0
#define IGO_TRACEF(...) (void)0

#endif

#ifdef _DEBUG
#define IGO_ASSERT(x) if (!(x)) {DebugBreak();}
#else
#define IGO_ASSERT(x) (void )0
#endif

#else // MAC OSX ================================================================

#include <CoreServices/CoreServices.h>

#define FILTER_MOUSE_MOVE_EVENTS    // Filter the input events to reduce the current latency when dragging/sizing a window - this shouldn't be the final
                                    // solution. Indeed we need to be aware of the current operation (dragging/sizing/scrolling) to filter the events appropriately
#define USE_CLIENT_SIDE_FILTERING

//#define SKIP_DISPLAY_CAPTURING

#define SHOW_GL_ERRORS   0          // Show any OpenGL errors

//#define SHOW_TAP_KEYS             // Show intercepted key/mouse events + logic flow whether we're going to switch IGO state
//#define SHOW_TAP_UNUSED_KEYS      // And show the intercepted but not used key/mouse events
//#define SHOW_CURSOR_CALLS         // Show cursor-related Mac API calls
//#define SHOW_IPC_MESSAGES         // Show ipc messages back and forth from client to game
//#define SHOW_MACHPORT_SETUP       // Show machport initialization in IGOIPC
//#define SHOW_OPENGL_SETUP         // Show information about OpenGL stored states
//#define SHOW_OVERRIDE_ASM_CODES   // Show overwrite assembly codesff
//#define SHOW_MOUSE_MOVE_LOGIC     // Show the different phases of our handling of a window move/resize in IGOQtContainer
//#define SHOW_RECEIVED_EVENTS      // Show the events as immediately received by the IGOWindowManagerIPC
//#define AVOID_EA_LIBS
//#define SHOW_NOTIFICATIONS          // Log all distributed notifications as well as NSApp/NSWindow/NSWorkspaces notifications
//#define SHOW_ANIM_ACTIVITIES        // Show what's going on with the window animation states

#ifdef _DEBUG
#define ENABLE_IGO_TRACE
#endif

#ifdef ENABLE_IGO_TRACE

#if 1 // defined(INJECTED_CODE)

#define IGO_TRACE(x) { \
int threadPriority = GetCurrentThreadPriority(); \
Origin::Mac::LogMessage("%ld | %s(%d): pid(%d),tid(%d/%d/%d) - %s\n", clock(), __FUNCTION__, __LINE__, GetCurrentProcessId(), GetCurrentThreadId(), IsMainThread(), threadPriority, x); \
}

#define IGO_TRACEF(format, ...) { \
int threadPriority = GetCurrentThreadPriority(); \
Origin::Mac::LogMessage("%ld | %s(%d): pid(%d),tid(%d/%d%/%d) - " format, clock(), __FUNCTION__, __LINE__, GetCurrentProcessId(), GetCurrentThreadId(), IsMainThread(), threadPriority, __VA_ARGS__); \
}

#else

#define IGO_TRACE(x) { \
int threadPriority = GetCurrentThreadPriority(); \
char igoTraceMsg[512]; \
snprintf(igoTraceMsg, sizeof(igoTraceMsg), "%ld | %s(%d): pid(%d),tid(%d/%d/%d) - ", clock(), __FILE__, __LINE__, GetCurrentProcessId(), GetCurrentThreadId(), IsMainThread(), threadPriority); \
printf("%s", igoTraceMsg); \
printf("%s\n", x); \
}

#define IGO_TRACEF(format, ...) { \
int threadPriority = GetCurrentThreadPriority(); \
char igoTraceMsg[512]; \
snprintf(igoTraceMsg, sizeof(igoTraceMsg), "%ld | %s(%d): pid(%d),tid(%d/%d/%d) - ", clock(), __FILE__, __LINE__, GetCurrentProcessId(), GetCurrentThreadId(), IsMainThread(), threadPriority); \
printf("%s", igoTraceMsg); \
snprintf(igoTraceMsg, sizeof(igoTraceMsg), format, __VA_ARGS__); \
printf("%s\n", igoTraceMsg); \
}

#endif // NOT INJECTED_CODE

#else
#define IGO_TRACE(x) (void)0
#define IGO_TRACEF(x, ...) (void)0
#endif

#ifdef _DEBUG
#define IGO_ASSERT(x) if (!(x)) { IGO_TRACE("ASSERT"); SysBreak();}
#else
#define IGO_ASSERT(x) (void )0
#endif

#endif // MAC OSX


#endif
