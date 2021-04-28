///////////////////////////////////////////////////////////////////////////////
// Config.h
//
// Copyright (c) 2006 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_INTERNAL_CONFIG_H
#define EATRACE_INTERNAL_CONFIG_H


///////////////////////////////////////////////////////////////////////////////
// ReadMe
//
// This is the EATrace configuration file. All configurable parameters of EATrace
// are controlled through this file. However, all the settings here can be 
// manually overridden by the user. There are three ways for a user to override
// the settings in this file:
//
//     - Simply edit this file.
//     - Define EATRACE_USER_CONFIG_HEADER.
//     - Predefine individual defines (e.g. EA_TRACE_ENABLED).
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// EATRACE_USER_CONFIG_HEADER
//
// This allows the user to define a header file to be #included before the 
// EATrace config.h contents are compiled. A primary use of this is to override
// the contents of this config.h file. Note that all the settings below in 
// this file are user-overridable.
// 
// Example usage:
//     #define EATRACE_USER_CONFIG_HEADER "MyConfigOverrides.h" // Normally this would be globally defined instead of defined in code.
//     #include <EATrace/EATrace.h>
//
///////////////////////////////////////////////////////////////////////////////

#ifdef EATRACE_USER_CONFIG_HEADER
    #include EATRACE_USER_CONFIG_HEADER
#endif



#include <EABase/eabase.h>


///////////////////////////////////////////////////////////////////////////////
// EATRACE_VERSION
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
//     printf("EATRACE version: %s", EATRACE_VERSION);
//     printf("EATRACE version: %d.%d.%d", EATRACE_VERSION_N / 10000 % 100, EATRACE_VERSION_N / 100 % 100, EATRACE_VERSION_N % 100);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EATRACE_VERSION
    #define EATRACE_VERSION   "2.09.05"
    #define EATRACE_VERSION_N  20905
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_PLATFORM_MICROSOFT
//
// Defined as 1 or undefined.
// Implements support for the definition of EA_PLATFORM_MICROSOFT for the case
// of using EABase versions prior to the addition of its EA_PLATFORM_MICROSOFT support.
//
#if (EABASE_VERSION_N < 20022) && !defined(EA_PLATFORM_MICROSOFT)
    #if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XENON)
        #define EA_PLATFORM_MICROSOFT 1
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_PLATFORM_MOBILE
//
// Defined as 1 or undefined.
// Implements support for the definition of EA_PLATFORM_MOBILE for the case
// of using EABase versions prior to the addition of its EA_PLATFORM_MOBILE support.
//
#if (EABASE_VERSION_N < 20022) && !defined(EA_PLATFORM_MOBILE)
    #if defined(EA_PLATFORM_IPHONE) || defined(EA_PLATFORM_ANDROID) || defined(EA_PLATFORM_PALM) || \
        defined(EA_PLATFORM_AIRPLAY) || defined(EA_PLATFORM_BADA) || defined(EA_PLATFORM_WINCE)
        #define EA_PLATFORM_MOBILE 1
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATRACE_ALLOC_PREFIX
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
//     void* p = pCoreAllocator->Alloc(37, EATRACE_ALLOC_PREFIX, 0);
//
// Example usage:
//     gMessageServer.GetMessageQueue().get_allocator().set_name(EATRACE_ALLOC_PREFIX "MessageSystem/Queue");
//
#ifndef EATRACE_ALLOC_PREFIX
    #define EATRACE_ALLOC_PREFIX "EATrace/"
#endif


///////////////////////////////////////////////////////////////////////////////
/// EA_TRACE_ENABLED
///
/// Defined as 0 or 1, default is 1 if EA_DEBUG is defined.
///
/// This is a master switch controlling the enabling of debug or release build
/// behaviour for trace-related macros. By default trace statements are compiled 
/// into debug builds and removed from release builds.  This behaviour can be 
/// overridden by setting EA_TRACE_ENABLED manually.
///
#if !defined(EA_TRACE_ENABLED)
    #if defined(EA_DEBUG)
        #define EA_TRACE_ENABLED 1
    #else
        #define EA_TRACE_ENABLED 0
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
/// EA_LOG_ENABLED
///
#if !defined(EA_LOG_ENABLED)
    #define EA_LOG_ENABLED EA_TRACE_ENABLED
#endif


///////////////////////////////////////////////////////////////////////////////
// EATRACE_DEFAULT_ALLOCATOR_ENABLED
//
// Defined as 0 or 1. Default is 1.
// Enables the use of ICoreAllocator::GetDefaultAllocator in the case that the
// user never called EA::Trace::SetAllocator. If disabled then the user is 
// required to explicitly specify the allocator used by EATrace with 
// EA::Trace::SetAllocator or on a class-by-class basis within EATrace. 
//
#ifndef EATRACE_DEFAULT_ALLOCATOR_ENABLED
    #define EATRACE_DEFAULT_ALLOCATOR_ENABLED 1
#endif


///////////////////////////////////////////////////////////////////////////////
// EATRACE_FLUSH_ON_WRITE_ENABLED_BY_DEFAULT
//
// Defined as 0 or 1. 
// Defines whether ILogReporter::SetFlushOnWrite and ITracer::SetFlushOnWrite 
// defaults to true. 
// This also specifies whether the global EAOutputDebugString function flushes
// buffered output upon write.
// ILogReporter::SetFlushOnWrite whether buffered IO writes are followed up 
// with a stream flush (e.g. stdio's fflush function). Mobile platforms need 
// this because otherwise you can easily lose output if the device crashes, 
// because the device writes buffered output over a write to your debugging PC.
//
#ifndef EATRACE_FLUSH_ON_WRITE_ENABLED_BY_DEFAULT
    #if defined(EA_PLATFORM_MOBILE)
        #define EATRACE_FLUSH_ON_WRITE_ENABLED_BY_DEFAULT 1
    #else
        #define EATRACE_FLUSH_ON_WRITE_ENABLED_BY_DEFAULT 0
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_TRACE_CURRENT_FUNCTION_LEVEL
//
// Defined as 0, 1, or 2
// Specifies how verbose the EA_CURRENT_FUNCTION macro is.
//     0: Expands to NULL
//     1: Expands to __FUNCTION__ (e.g. "MyNamespace::MyClass::MyFunction")
//     2: Expands to GCC's __PRETTY_FUNCTION__ or MSVC's __FUNCSIG__ (e.g. "int MyNamespace::MyClass::MyFunction(int, char*)")
//
#ifndef EA_TRACE_CURRENT_FUNCTION_LEVEL
    #define EA_TRACE_CURRENT_FUNCTION_LEVEL 2
#endif


///////////////////////////////////////////////////////////////////////////////
// EATRACE_DEBUG_BREAK_ABORT_ENABLED
//
// Defined as 0 or 1. 
// If enabled then the app is aborted instead of using a debug break instruction.
// On some platforms there isn't sufficient debug break functionality to allow
// it to work and there is little option other than to abort or call some other
// user function.
//
#ifndef EATRACE_DEBUG_BREAK_ABORT_ENABLED
    #if defined(EA_PLATFORM_ANDROID)
        // There isn't currently a way to 'debug break' on Android, and so we have little choice 
        // but to exit the application. Otherwise the user would have to manually force the debug
        // monitor to kill the application.
        #define EATRACE_DEBUG_BREAK_ABORT_ENABLED 1
    #else
        #define EATRACE_DEBUG_BREAK_ABORT_ENABLED 0
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EATRACE_GLOBAL_ENABLED
//
// Defined as 0 or 1, with 1 (enabled) being the default.
// If enabled then the Trace system is shared between the application and its 
// DLLs. If disabled then each compilation unit gets its own copy of EATrace.
//
#ifndef EATRACE_GLOBAL_ENABLED
    #define EATRACE_GLOBAL_ENABLED 1
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
// EATRACE_VA_COPY_ENABLED
//
// Defined as 0 or 1. Default is 1 for compilers that need it, 0 for others.
// Some compilers on some platforms implement va_list whereby its contents  
// are destroyed upon usage, even if passed by value to another function. 
// With these compilers you can use va_copy to restore the a va_list.
// Known compiler/platforms that destroy va_list contents upon usage include:
//     CodeWarrior on PowerPC
//     GCC on x86-64
// However, va_copy is part of the C99 standard and not part of earlier C and
// C++ standards. So not all compilers support it. VC++ doesn't support va_copy,
// but it turns out that VC++ doesn't need it on the platforms it supports.
// For example usage, see the EASTL string.h file.
//
#ifndef EATRACE_VA_COPY_ENABLED
    #if defined(__MWERKS__) || (defined(__GNUC__) && (__GNUC__ >= 3) && (!defined(__i386__) || defined(__x86_64__)) && !defined(__ppc__) && !defined(__PPC__) && !defined(__PPC64__))
        #define EATRACE_VA_COPY_ENABLED 1
    #else
        #define EATRACE_VA_COPY_ENABLED 0
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATRACE_DLL
//
// Defined as 0 or 1. The default is dependent on the definition of EA_DLL.
// If EA_DLL is defined, then EATRACE_DLL is 1, else EATRACE_DLL is 0.
// EA_DLL is a define that controls DLL builds within the EAConfig build system. 
// EATRACE_DLL controls whether EATrace is built and used as a DLL. 
// Normally you wouldn't do such a thing, but there are use cases for such
// a thing, particularly in the case of embedding C++ into C# applications.
//
#ifndef EATRACE_DLL
    #if defined(EA_DLL)
        #define EATRACE_DLL 1
    #else
        #define EATRACE_DLL 0
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATRACE_API
//
// This is used to label functions as DLL exports under Microsoft platforms.
// If EA_DLL is defined, then the user is building EATrace as a DLL and EATrace's
// non-templated functions will be exported. EATrace template functions are not
// labelled as EATRACE_API (and are thus not exported in a DLL build). This is 
// because it's not possible (or at least unsafe) to implement inline templated 
// functions in a DLL.
//
// Example usage of EATRACE_API:
//    EATRACE_API int someVariable = 10;         // Export someVariable in a DLL build.
//
//    struct EATRACE_API SomeClass{              // Export SomeClass and its member functions in a DLL build.
//        EATRACE_LOCAL void PrivateMethod();    // Not exported.
//    };
//
//    EATRACE_API void SomeFunction();           // Export SomeFunction in a DLL build.
//
// For GCC, see http://gcc.gnu.org/wiki/Visibility
//
#ifndef EATRACE_API // If the build file hasn't already defined this to be dllexport...
    #if EATRACE_DLL 
        #if defined(_MSC_VER)
            #define EATRACE_API      __declspec(dllimport)
            #define EATRACE_LOCAL
        #elif defined(__CYGWIN__)
            #define EATRACE_API      __attribute__((dllimport))
            #define EATRACE_LOCAL
        #elif (defined(__GNUC__) && (__GNUC__ >= 4))
            #define EATRACE_API      __attribute__ ((visibility("default")))
            #define EATRACE_LOCAL    __attribute__ ((visibility("hidden")))
        #else
            #define EATRACE_API
            #define EATRACE_LOCAL
        #endif
    #else
        #define EATRACE_API
        #define EATRACE_LOCAL
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
// EA_CUSTOM_TRACER_ENABLED
//
// Defined as 0 or 1.
// This is something the user might override.
// To consider: should this be renamed to EATRACE_CUSTOM_TRACER_ENABLED?
//
#if !defined(EA_CUSTOM_TRACER_ENABLED)
    #define EA_CUSTOM_TRACER_ENABLED 0
#endif
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// EA_HAVE_SYS_PTRACE_H
//
// Defined or not defined, as per <EABase/eahave.h>
//
#if !defined(EA_HAVE_SYS_PTRACE_H) && !defined(EA_NO_HAVE_SYS_PTRACE_H)
    #if defined(EA_PLATFORM_UNIX) && !defined(__CYGWIN__) && (defined(EA_PLATFORM_DESKTOP) || defined(EA_PLATFORM_SERVER))
        #define EA_HAVE_SYS_PTRACE_H 1 /* declares the ptrace function */
    #else
        #define EA_NO_HAVE_SYS_PTRACE_H 1
    #endif
#endif


#endif // Header include guard

















