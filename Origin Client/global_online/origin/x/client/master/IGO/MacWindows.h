//
//  Windows.h
//  IGOUnitTests
//
//  Created by Frederic Meraud on 5/24/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#ifndef IGOUnitTests_Windows_h
#define IGOUnitTests_Windows_h

#if defined(ORIGIN_MAC)

#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include <libkern/OSAtomic.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <MacTypes.h>
//#include <CoreServices/CoreServices.h>

typedef signed char BOOL;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned long* PDWORD;
typedef int HANDLE;
typedef void* PVOID;
typedef void* LPVOID;
typedef void VOID;
typedef const char* LPCSTR;
typedef char* LPWSTR;
typedef wchar_t WCHAR;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef float FLOAT;
typedef long LONG;
typedef UInt32 ULONG;
typedef long long LONG64;
typedef unsigned long long ULONG64;
typedef unsigned char BYTE;
typedef unsigned long ULONG_PTR; // FIXME: NOT REALLY THAT!
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
typedef intptr_t HKL;


typedef struct tagPOINT
{
    LONG  x;
    LONG  y;

} POINT;

typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;

} RECT;

#define MAX_PATH                            255
#define WINAPI
#define APIENTRY
#define INVALID_HANDLE_VALUE                -1

#ifndef FALSE
#define FALSE               0
#endif
#ifndef TRUE
#define TRUE                1
#endif


#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)           ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))


#if !defined(_countof)
#if !defined(__cplusplus)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#else
extern "C++"
{
    template <typename _CountofType, size_t _SizeOfArray>
    char (*__countof_helper(_CountofType (&_Array)[_SizeOfArray]))[_SizeOfArray];

#define _countof(_Array) (sizeof(*__countof_helper(_Array)) + 0)
}
#endif
#endif



//#define INVALID_HANDLE_VALUE                ((HANDLE)(LONG_PTR)-1)
#define WM_USER 0x0400


#define CRITICAL_SECTION                    ::pthread_mutex_t
#define InitializeCriticalSection(x)        ::pthread_mutex_init(x, NULL)
#define EnterCriticalSection(x)             ::pthread_mutex_lock(x)
#define LeaveCriticalSection(x)             ::pthread_mutex_unlock(x)
#define DeleteCriticalSection(x)            ::pthread_mutex_destroy(x)


#define OutputDebugString(...)
#define OutputDebugStringA(...)

#define _aligned_malloc(size, alignment)    ({void* m; int success = posix_memalign(&m, alignment, size); success ? m : NULL;})
#define ZeroMemory(dst, size)               memset(dst, 0, size)
#define CopyMemory(dst, src, size)          memcpy(dst, src, size)


#define GetTickCount()                      (unsigned long)(UnsignedWideToUInt64(AbsoluteToNanoseconds(UpTime())) / 1000000)
#define Sleep(x)                            ::usleep(x * 1000)
#define GetLastError()                      errno
#define GetCurrentProcessId()               ::getpid()
#define GetCurrentThreadId()                ::pthread_mach_thread_np(::pthread_self())
#define IsMainThread()                      ::pthread_main_np() != 0
#define GetCurrentThreadPriority()          ({int policy; sched_param params; int success = pthread_getschedparam(::pthread_self(), &policy, &params); !success ? params.sched_priority : -1111111;})


#define EALocalStorage "~/EA/Origin/"
#define EAPipesStorage "~/EA/Origin/Pipes/"
#define EAPipeFileName "IGO_Pipe_"

#define IGO_INJECTED_MODULE_NAME                "InjectedCode"
#define IGOIPC_REGISTRATION_SERVICE             "ea.origin.igo"
#define IGOIPC_SHARED_MEMORY_BASE_NAME          "BUF_"
#define IGOIPC_SHARED_MEMORY_BUFFER_SIZE        (100 * 1024 * 1024)
#define IGOIPC_TELEMETRY_REGISTRATION_SERVICE   "ea.origin.tel"
#define IGOIPC_TELEMETRY_MEMORY_BUFFER_SIZE     (256) // just for the buffer to be valid

#define USE_SEPARATE_KBD_LAYOUT_FOR_IGO

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
namespace Origin
{
namespace Mac
{

// Log messages to the system Console.
void LogError(const char* format, ...);
void LogMessage(const char* format, ...);


// Returns 0 if successful; otherwise returns -1 (buffer not defined, buffer size too small, unable to retrieve home directory)
int GetHomeDirectory(char* buffer, size_t bufferSize);

void SaveRawRGBAToTGA(const char* fileName, size_t width, size_t height, const unsigned char* data, bool overwrite = false);


// Extract the module name that contains the specific address. If addr = NULL, returns the full path to the current executable.
bool GetModuleFileName(void* addr, char* buffer, size_t bufferSize);

  
int32_t GetHashCode(const char* value);
  
int32_t CurrentInputSource();
bool    SetInputSource(int32_t id);
int32_t UseNextInputSource();
int32_t UsePreviousInputSource();

class Semaphore
{
    public:
        Semaphore(int count = 1);
        ~Semaphore();

        bool IsValid();
        void Signal();
        void Wait();
        bool Wait(unsigned int timeoutInMS);

    private:
        int _count;
        int _isValid;

        pthread_mutex_t _mutex;
        pthread_cond_t _cond;

        bool InnerWait(const timespec* abstime);
};

class AutoLock
{
    public:
        explicit AutoLock(Semaphore* lock, unsigned int timeoutInMS = 100)
            : _lock(lock)
        {
            lock->Wait(timeoutInMS);
        }
    
        ~AutoLock()
        {
            _lock->Signal();
        }

    private:
        Semaphore* _lock;
};
    
}
}
#endif // __cplusplus

#endif
#endif
