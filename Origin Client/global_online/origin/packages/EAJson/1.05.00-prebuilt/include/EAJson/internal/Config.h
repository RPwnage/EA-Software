///////////////////////////////////////////////////////////////////////////////
// Config.h
//
// Copyright (c) 2006 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_INTERNAL_CONFIG_H
#define EAJSON_INTERNAL_CONFIG_H


#include <EABase/eabase.h>

#if defined(EA_PLATFORM_AIRPLAY)
#include <sys/types.h>      // for ssize_t typedef
#endif

///////////////////////////////////////////////////////////////////////////////
// EAJSON_VERSION
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
//     printf("EAJSON version: %s", EAJSON_VERSION);
//     printf("EAJSON version: %d.%d.%d", EAJSON_VERSION_N / 10000 % 100, EAJSON_VERSION_N / 100 % 100, EAJSON_VERSION_N % 100);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EAJSON_VERSION
    #define EAJSON_VERSION   "1.05.00"
    #define EAJSON_VERSION_N  10500
#endif


///////////////////////////////////////////////////////////////////////////////
// EAJSON_ALLOC_PREFIX
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
//     void* p = pCoreAllocator->Alloc(37, EAJSON_ALLOC_PREFIX, 0);
//
// Example usage:
//     gMessageServer.GetMessageQueue().get_allocator().set_name(EAJSON_ALLOC_PREFIX "MessageSystem/Queue");
//
#ifndef EAJSON_ALLOC_PREFIX
    #define EAJSON_ALLOC_PREFIX "EAJSON/"
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
// EAJSON_DEV_ASSERT
//
// Defined as EA_ASSERT or defined away.
//
#if !defined(EAJSON_DEV_ASSERT)
    #if defined(AUTHOR_PPEDRIANA)
        #define EAJSON_DEV_ASSERT(x) EA_ASSERT(x)
        #define EAJSON_DEV_ASSERT_ENABLED 1
    #else
        #define EAJSON_DEV_ASSERT(x)
        #define EAJSON_DEV_ASSERT_ENABLED 0
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EAJSON_SIZE_TYPE / EAJSON_SSIZE_TYPE
//
// Specifies the type of EA::Json::size_type and EA::Json::ssize_type, 
// which are used to denote stream data sizes and offsets. size_type must 
// be unsigned and ssize_type must be signed. 
//
// The user can define EAJSON_SIZE_TYPE as uint64_t and EAJSON_SSIZE_TYPE
// as int64_t, for example. 
//
#ifdef EAJSON_SIZE_TYPE // If the user has already externally defined this to something...
    namespace EA { namespace Json {
        typedef EAJSON_SIZE_TYPE   size_type;
        typedef EAJSON_SSIZE_TYPE ssize_type;
    } } 
#else
    namespace EA { namespace Json {
        #if defined(EAJSON_64_BIT_SIZE_ENABLED) && EAJSON_64_BIT_SIZE_ENABLED
            typedef uint64_t  size_type;
            typedef  int64_t ssize_type;
            typedef  int64_t   off_type;
        #else
            typedef  size_t   size_type;
            typedef ssize_t  ssize_type;
            typedef ssize_t    off_type;
        #endif
    } } 
#endif


///////////////////////////////////////////////////////////////////////////////
// EAJSON_DLL
//
// Defined as 0 or 1. The default is dependent on the definition of EA_DLL.
// If EA_DLL is defined, then EAJSON_DLL is 1, else EAJSON_DLL is 0.
// EA_DLL is a define that controls DLL builds within the EAConfig build system. 
// EAJSON_DLL controls whether EAJSON is built and used as a DLL. 
// Normally you wouldn't do such a thing, but there are use cases for such
// a thing, particularly in the case of embedding C++ into C# applications.
//
#ifndef EAJSON_DLL
    #if defined(EA_DLL)
        #define EAJSON_DLL 1
    #else
        #define EAJSON_DLL 0
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EAJSON_API
//
// This is used to label functions as DLL exports under Microsoft platforms.
// If EA_DLL is defined, then the user is building EAJSON as a DLL and EAJSON's
// non-templated functions will be exported. EAJSON template functions are not
// labelled as EAJSON_API (and are thus not exported in a DLL build). This is 
// because it's not possible (or at least unsafe) to implement inline templated 
// functions in a DLL.
//
// Example usage of EAJSON_API:
//    EAJSON_API int someVariable = 10;  // Export someVariable in a DLL build.
//
//    struct EAJSON_API SomeClass{       // Export SomeClass and its member functions in a DLL build.
//    };
//
//    EAJSON_API void SomeFunction();    // Export SomeFunction in a DLL build.
//
#ifndef EAJSON_API // If the build file hasn't already defined this to be dllexport...
    #if EAJSON_DLL && defined(_MSC_VER)
        #define EAJSON_API           __declspec(dllimport)
        #define EAJSON_TEMPLATE_API  // Not sure if there is anything we can do here.
    #else
        #define EAJSON_API
        #define EAJSON_TEMPLATE_API
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EAJSON_ENABLE_DEBUG_HELP
//
// Defined as 0 or 1. Defaults to 1 when EA_DEBUG is defined; else 0.
// This option enables functionality that aids in debugging JSON files.
// For example, the JsonReader will be able to print a string indicating
// the character location and string of a syntax error, along with a 
// brief description of the problem.
//
#ifndef EAJSON_ENABLE_DEBUG_HELP
    #if defined(EA_DEBUG)
        #define EAJSON_ENABLE_DEBUG_HELP 1
    #else
        #define EAJSON_ENABLE_DEBUG_HELP 0
    #endif
#endif



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
#ifndef EA_UNUSED // Recent versions of EABase now define this.
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



///////////////////////////////////////////////////////////////////////////////
// EAArrayCount
//
// This is defined by recent versions of EABase, but for the time being we 
// provide backward compatibility here.
//
#ifndef EAArrayCount
    #define EAArrayCount(x) (sizeof(x) / sizeof(x[0]))
#endif



#endif // Header include guard

















