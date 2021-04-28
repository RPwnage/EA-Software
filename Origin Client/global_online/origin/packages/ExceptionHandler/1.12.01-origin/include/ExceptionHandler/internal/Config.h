///////////////////////////////////////////////////////////////////////////////
// Config.h
//
// Copyright (c) 2006 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EXCEPTIONHANDLER_INTERNAL_CONFIG_H
#define EXCEPTIONHANDLER_INTERNAL_CONFIG_H


#include <new>
#include <stddef.h>


///////////////////////////////////////////////////////////////////////////////
// EXCEPTIONHANDLER_VERSION
//
// We more or less follow the conventional EA packaging approach to versioning 
// here. A primary distinction here is that minor versions are defined as two
// digit entities (e.g. .03") instead of minimal digit entities ".3"). The logic
// here is that the value is a counter and not a floating point fraction.
// Note that the major version doesn't have leading zeros.
//
// Example version strings:
//      "0.91.00"   // Major version 0, minor version 91, patch version 0. 
//      "1.00.00"   // Major version 1, minor and patch version 0.
//      "3.10.02"   // Major version 3, minor version 10, patch version 02.
//     "12.03.01"   // Major version 12, minor version 03, patch version 
//
// Example usage:
//     printf("EXCEPTIONHANDLER version: %s", EXCEPTIONHANDLER_VERSION);
//     printf("EXCEPTIONHANDLER version: %d.%d.%d", EXCEPTIONHANDLER_VERSION_N / 10000 % 100, EXCEPTIONHANDLER_VERSION_N / 100 % 100, EXCEPTIONHANDLER_VERSION_N % 100);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EXCEPTIONHANDLER_VERSION
    #define EXCEPTIONHANDLER_VERSION   "1.12.01"
    #define EXCEPTIONHANDLER_VERSION_N  11201
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_WINAPI_FAMILY_PARTITION / EA_WINAPI_PARTITION
// 
// Provides the EA_WINAPI_FAMILY_PARTITION macro for testing API support under
// Microsoft platforms, plus associated API identifiers for this macro.
//
// Current API identifiers:
//     EA_WINAPI_PARTITION_NONE
//     EA_WINAPI_PARTITION_DESKTOP
//     EA_WINAPI_PARTITION_XENON
//
// Example usage:
//     #if defined(EA_PLATFORM_MICROSOFT)
//         #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
//             GetEnvironmentVariableA("PATH", env, sizeof(env));
//         #else
//             // not supported.
//         #endif
//     #endif
//
#if !defined(EA_WINAPI_FAMILY_PARTITION)
    #define EA_WINAPI_FAMILY_PARTITION(partition) (partition)
#endif

#if !defined(EA_WINAPI_PARTITION_NONE) && !defined(EA_WINAPI_PARTITION_DESKTOP) && !defined(EA_WINAPI_PARTITION_XENON)
    #if defined(EA_PLATFORM_MICROSOFT)
        #if defined(EA_PLATFORM_WINDOWS)
            #define EA_WINAPI_PARTITION_DESKTOP 1
        #elif defined(EA_PLATFORM_XENON)
            #define EA_WINAPI_PARTITION_XENON 1
        #else
            // Nothing defined. Future values (e.g. Windows Phone) would go here.
        #endif
    #else
        #define EA_WINAPI_PARTITION_NONE 1
    #endif
#endif

#if !defined(EA_WINAPI_PARTITION_NONE)
    #define EA_WINAPI_PARTITION_NONE 0
#endif

#if !defined(EA_WINAPI_PARTITION_DESKTOP)
    #define EA_WINAPI_PARTITION_DESKTOP 0
#endif

#if !defined(EA_WINAPI_PARTITION_XENON)
    #define EA_WINAPI_PARTITION_XENON 0
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_EXCEPTIONHANDLER_HANDLING_SUPPORTED
//
// Defined as 0 or 1.
// Identifies whether the ExceptionHandler package supports exception handling
// on current platform. The primary reason it wouldn't be able to is that
// the platform doesn't support it or simply that support hasn't been written
// and tested yet in this package.
//
#ifndef EA_EXCEPTIONHANDLER_HANDLING_SUPPORTED
    #if defined(EA_PLATFORM_XENON)
        #define EA_EXCEPTIONHANDLER_HANDLING_SUPPORTED 1
    #elif defined(EA_PLATFORM_MICROSOFT)
        #define EA_EXCEPTIONHANDLER_HANDLING_SUPPORTED 1
    #elif defined(EA_PLATFORM_PS3)
        #define EA_EXCEPTIONHANDLER_HANDLING_SUPPORTED 1
    #elif defined(EA_PLATFORM_REVOLUTION)
        #define EA_EXCEPTIONHANDLER_HANDLING_SUPPORTED 1
    #elif defined(EA_PLATFORM_APPLE)
        #define EA_EXCEPTIONHANDLER_HANDLING_SUPPORTED 1
    #elif defined(EA_PLATFORM_PSP2) // Sony PSVita with SDK 1.8+
        #define EA_EXCEPTIONHANDLER_HANDLING_SUPPORTED 1
    #else
        #define EA_EXCEPTIONHANDLER_HANDLING_SUPPORTED 0
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_EXCEPTIONHANDLER_SIGNAL_BASED_HANDLING_SUPPORTED
//
// Defined as 0 or 1.
// Specifies whether ExceptionHandler::kModeSignalHandling can be used.
//
#ifndef EA_EXCEPTIONHANDLER_SIGNAL_BASED_HANDLING_SUPPORTED
    // Until we add support for signal-based exceptions under Unix, this will be unilaterally 0 (unsupported).
    #define EA_EXCEPTIONHANDLER_SIGNAL_BASED_HANDLING_SUPPORTED 0
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_EXCEPTIONHANDLER_VECTORED_HANDLING_SUPPORTED
//
// Defined as 0 or 1.
// Specifies whether ExceptionHandler::kModeVectored can be used.
// Vectored exception handling is a Microsoft term for an exception handler
// that's implemented in another thread via a registered callback function.
// It's as opposed to signal-based exception handling (despite signals being
// essentially callback functions) and as opposed to stack-based exception
// handling where the handling occurs in the same thread via a try/catch 
// kind of scheme.
//
#ifndef EA_EXCEPTIONHANDLER_VECTORED_HANDLING_SUPPORTED
    #if defined(EA_PLATFORM_MICROSOFT) && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
        #define EA_EXCEPTIONHANDLER_VECTORED_HANDLING_SUPPORTED 0
    #else
        #define EA_EXCEPTIONHANDLER_VECTORED_HANDLING_SUPPORTED EA_EXCEPTIONHANDLER_HANDLING_SUPPORTED
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_EXCEPTIONHANDLER_STACK_BASED_HANDLING_SUPPORTED
//
// Defined as 0 or 1.
// Specifies whether ExceptionHandler::kModeStackBased can be used.
//
#ifndef EA_EXCEPTIONHANDLER_STACK_BASED_HANDLING_SUPPORTED
    #if defined(EA_PLATFORM_MICROSOFT)
        #define EA_EXCEPTIONHANDLER_STACK_BASED_HANDLING_SUPPORTED 1 // __try/__except
    #else
        #define EA_EXCEPTIONHANDLER_STACK_BASED_HANDLING_SUPPORTED 0
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_EXCEPTIONHANDLER_RECOVERY_SUPPORTED
//
// Defined as 0 or 1.
// Indicates whether the OS lets you continue/recover from an exception under  
// any condition. Some platforms don't allow this and unilaterally kill the 
// app after an exception is encountered. Some platforms and exception handling
// modes allow you to recover, but you have to take specific actions to do so;
// you can't just say "I'd like to ignore this cpu exception and continue, please."
//
#if !defined(EA_EXCEPTIONHANDLER_RECOVERY_SUPPORTED)
    #if defined(EA_PLATFORM_SONY) || defined(EA_PLATFORM_NINTENDO)
        #define EA_EXCEPTIONHANDLER_RECOVERY_SUPPORTED 0
    #else
        #define EA_EXCEPTIONHANDLER_RECOVERY_SUPPORTED 1
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EXCEPTION_HANDLER_GET_CALLSTACK_FROM_THREAD_SUPPORTED
//
// Defined as 0 or 1.
// To do: Move the EXCEPTION_HANDLER_GET_CALLSTACK_FROM_THREAD_SUPPORTED define into EAThread.
//
#if defined(EA_PLATFORM_SONY) && defined(EA_PLATFORM_CONSOLE) && !defined(EA_PLATFORM_PS3) && !defined(EA_PLATFORM_PSP2)
    #define EXCEPTION_HANDLER_GET_CALLSTACK_FROM_THREAD_SUPPORTED 1
#else
    #define EXCEPTION_HANDLER_GET_CALLSTACK_FROM_THREAD_SUPPORTED 0
#endif



///////////////////////////////////////////////////////////////////////////////
// EA_XBDM_ENABLED
//
// Defined as 0 or 1, with 1 being the default for debug builds.
// This controls whether xbdm library usage is enabled on XBox 360. This library
// allows for runtime debug functionality. But shipping applications are not
// allowed to use xbdm. 
//
#if !defined(EA_XBDM_ENABLED)
    #if defined(EA_DEBUG)
        #define EA_XBDM_ENABLED 1
    #else
        #define EA_XBDM_ENABLED 0
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// operator new
//
// Allocate an object with a total size of 'size'.
//
// Example usage:
//    SomeClass* pObject = new("SomeClass") SomeClass(1, 2, 3);
//    SomeClass* pObject = new("SomeClass", 0, 0, __FILE__, __LINE__) SomeClass(1, 2, 3);
//    SomeClass* pObject = new("SomeClass", EA::Allocator::kFlagNormal, 1 << EA::Allocator::GeneralAllocatorDebug::kDebugDataIdCallStack) SomeClass(1, 2, 3);
//
/// Example usage:
///    SomeClass* pArray = new("SomeClass", EA::Allocator::kFlagPermanent, __FILE__, __LINE__) SomeClass(1, 2, 3)[4];
///    SomeClass* pArray = new("SomeClass", EA::Allocator::kFlagNormal, 1 << EA::Allocator::GeneralAllocatorDebug::kDebugDataIdCallStack, __FILE__, __LINE__) SomeClass(1, 2, 3)[4];
//
void* operator new     (size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line);
void* operator new[]   (size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line);
void  operator delete  (void *p,     const char* name, int flags, unsigned debugFlags, const char* file, int line);
void  operator delete[](void *p,     const char* name, int flags, unsigned debugFlags, const char* file, int line);


///////////////////////////////////////////////////////////////////////////////
// EXCEPTIONHANDLER_ALLOC_PREFIX
//
// Defined as a string literal. Defaults to this package's name.
// Can be overridden by the user by predefining it or by editing this file.
// This define is used as the default name used by this package for naming
// memory allocations and memory allocators.
//
// All allocations names follow the same naming pattern:
//     <package>/<module>[/<specific usage>]
// 
// Example usage:
//     void* p = pCoreAllocator->Alloc(37, EXCEPTIONHANDLER_ALLOC_PREFIX, 0);
//
// Example usage:
//     gMessageServer.GetMessageQueue().get_allocator().set_name(EXCEPTIONHANDLER_ALLOC_PREFIX "MessageSystem/Queue");
//
#ifndef EXCEPTIONHANDLER_ALLOC_PREFIX
    #define EXCEPTIONHANDLER_ALLOC_PREFIX "ExceptionHandler/"
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_EXCEPTIONHANDLER_DEBUG
//
// Defined as 0 or 1, with the default being 1 if EA_DEBUG and 0 otherwise.
//
#ifndef EA_EXCEPTIONHANDLER_DEBUG
    #if defined(EA_DEBUG) || defined(_DEBUG)
        #define EA_EXCEPTIONHANDLER_DEBUG 1
    #else
        #define EA_EXCEPTIONHANDLER_DEBUG 0
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EA_EH_NEW
//
// This is merely a wrapper for operator new which can be overridden and 
// which has debug/release forms.
//
// Example usage:
//    SomeClass* pObject = EA_EH_NEW("SomeClass") SomeClass(1, 2, 3);
//
#ifndef EA_EH_NEW
    #if EA_EXCEPTIONHANDLER_DEBUG
        #define EA_EH_NEW(name)   new(name, 0, 0, __FILE__, __LINE__)
        #define EA_EH_DELETE      delete
    #else
        #define EA_EH_NEW(name)   new(name, 0, 0, 0, 0)
        #define EA_EH_DELETE      delete
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// UTF_USE_EATRACE    Defined as 0 or 1. Default is 1 but may be overridden by your project file.
// UTF_USE_EAASSERT   Defined as 0 or 1. Default is 0.
//
// Defines whether this package uses the EATrace or EAAssert package to do assertions.
// EAAssert is a basic lightweight package for portable asserting.
// EATrace is a larger "industrial strength" package for assert/trace/log.
// If both UTF_USE_EATRACE and UTF_USE_EAASSERT are defined as 1, then UTF_USE_EATRACE
// overrides UTF_USE_EAASSERT.
//
#if defined(UTF_USE_EATRACE) && UTF_USE_EATRACE
    #undef  UTF_USE_EAASSERT
    #define UTF_USE_EAASSERT 0
#elif !defined(UTF_USE_EAASSERT)
    #define UTF_USE_EAASSERT 0
#endif

#undef UTF_USE_EATRACE
#if defined(UTF_USE_EAASSERT) && UTF_USE_EAASSERT
    #define UTF_USE_EATRACE 0
#else
    #define UTF_USE_EATRACE 1
#endif

#if !defined(EA_ASSERT_HEADER)
    #if defined(UTF_USE_EAASSERT) && UTF_USE_EAASSERT
        #define EA_ASSERT_HEADER <EAAssert/eaassert.h>
    #else
        #define EA_ASSERT_HEADER <EATrace/EATrace.h>
    #endif
#endif
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// EA_UNUSED
// 
// Makes compiler warnings about unused variables go away.
//
// Example usage:
//    void Function(int x)
//    {
//        int y;
//        EA_UNUSED(x);
//        EA_UNUSED(y);
//    }
//
#ifndef EA_UNUSED
    // The EDG solution below is pretty weak and needs to be augmented or replaced.
    // It can't handle the C language, is limited to places where template declarations
    // can be used, and requires the type x to be usable as a functions reference argument. 
    #if defined(__cplusplus) && defined(__EDG__) && !defined(__PPU__) && !defined(__SPU__) // All EDG variants except the SN PS3 variant which allows usage of (void)x;
        template <typename T>
        inline void EABaseUnused(T const volatile & x) { (void)x; }
        #define EA_UNUSED(x) EABaseUnused(x)
    #else
        #define EA_UNUSED(x) (void)x
    #endif
#endif


#endif // Header include guard

