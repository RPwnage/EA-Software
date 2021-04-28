///////////////////////////////////////////////////////////////////////////////
// Config.h
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_INTERNAL_CONFIG_H
#define EAPATCHCLIENT_INTERNAL_CONFIG_H


#include <EABase/eabase.h>
#include <EAPatchClient/Version.h>


///////////////////////////////////////////////////////////////////////////////
// EAPATCH_DEBUG
//
// Defined as 0, 1, or 2, with 1 being the default for debug builds.
// 2 means to enable EAPatch developer functionality.
//
#if !defined(EAPATCH_DEBUG)
    #if defined(EA_DEBUG)
        #define EAPATCH_DEBUG 1
    #else
        #define EAPATCH_DEBUG 0
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EAPATCH_ENABLE_ERROR_ASSERTIONS
//
// If true then the Error class asserts that it's been checked. See the Error class.
//
#if !defined(EAPATCH_ENABLE_ERROR_ASSERTIONS)
    #define EAPATCH_ENABLE_ERROR_ASSERTIONS 0
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
// EAPATCHCLIENT_64_BIT_FILES_SUPPORTED
//
// Defined as 0 or 1. Depends on the version of DirtySDK and how EAIO is configured on 32 bit systems.
// A 64 bit file is defined here as a file whose size is >= 2^32 bytes (4 GiB).
// If EAPATCHCLIENT_64_BIT_FILES_SUPPORTED is 0 then it's not possible to 
// have files in a patch which are > 4 GiB in size.
// 
#include <DirtySDK/dirtyvers.h>
#include <EAIO/internal/Config.h>

#if !defined(EAPATCHCLIENT_64_BIT_FILES_SUPPORTED)
    // Need to have an updated version of DirtySDK to support 64 bit files.
    // As of this writing (July 2012) no version of DirtySDK supports it, but a future version will.
    #if (DIRTYVERS == 0xffffffff)
        // Need to be on a 64 bit platform or have EAIO configured to use 64 bit size types on a 32 bit platform.
        #if (EA_PLATFORM_PTR_SIZE >= 8) || EAIO_64_BIT_SIZE_ENABLED
            #define EAPATCHCLIENT_64_BIT_FILES_SUPPORTED 1
        #else
            #define EAPATCHCLIENT_64_BIT_FILES_SUPPORTED 0
        #endif
    #else
        #define EAPATCHCLIENT_64_BIT_FILES_SUPPORTED 0
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EAPATCHCLIENT_DLL
//
// Defined as 0 or 1. The default is dependent on the definition of EA_DLL.
// If EA_DLL is defined, then EAPATCHCLIENT_DLL is 1, else EAPATCHCLIENT_DLL is 0.
// EA_DLL is a define that controls DLL builds within the eaconfig build system. 
// EAPATCHCLIENT_DLL controls whether EAPatchClient is built and used as a DLL. 
// Normally you wouldn't do such a thing, but there are use cases for such
// a thing, particularly in the case of embedding C++ into C# applications.
//
#ifndef EAPATCHCLIENT_DLL
    #if defined(EA_DLL)
        #define EAPATCHCLIENT_DLL 1
    #else
        #define EAPATCHCLIENT_DLL 0
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EAPATCHCLIENT_API
//
// This is used to label functions as DLL exports under Microsoft platforms.
// If EA_DLL is defined, then the user is building EAPATCHCLIENT as a DLL and EAPATCHCLIENT's
// non-templated functions will be exported. EAPATCHCLIENT template functions are not
// labelled as EAPATCHCLIENT_API (and are thus not exported in a DLL build). This is 
// because it's not possible (or at least unsafe) to implement inline templated 
// functions in a DLL.
//
// Example usage of EAPATCHCLIENT_API:
//    EAPATCHCLIENT_API int someVariable = 10;         // Export someVariable in a DLL build.
//
//    struct EAPATCHCLIENT_API SomeClass{              // Export SomeClass and its member functions in a DLL build.
//        EAPATCHCLIENT_LOCAL void PrivateMethod();    // Not exported.
//    };
//
//    EAPATCHCLIENT_API void SomeFunction();           // Export SomeFunction in a DLL build.
//
// For GCC, see http://gcc.gnu.org/wiki/Visibility
//
#ifndef EAPATCHCLIENT_API // If the build file hasn't already defined this to be dllexport...
    #if EAPATCHCLIENT_DLL 
        #if defined(_MSC_VER)
            #define EAPATCHCLIENT_API      __declspec(dllimport)
            #define EAPATCHCLIENT_LOCAL
        #elif defined(__CYGWIN__)
            #define EAPATCHCLIENT_API      __attribute__((dllimport))
            #define EAPATCHCLIENT_LOCAL
        #elif (defined(__GNUC__) && (__GNUC__ >= 4))
            #define EAPATCHCLIENT_API      __attribute__ ((visibility("default")))
            #define EAPATCHCLIENT_LOCAL    __attribute__ ((visibility("hidden")))
        #else
            #define EAPATCHCLIENT_API
            #define EAPATCHCLIENT_LOCAL
        #endif
    #else
        #define EAPATCHCLIENT_API
        #define EAPATCHCLIENT_LOCAL
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EAPATCHCLIENT_ALLOC_PREFIX
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
//     void* p = pCoreAllocator->Alloc(37, EAPATCHCLIENT_ALLOC_PREFIX, 0);
//
// Example usage:
//     gMessageServer.GetMessageQueue().get_allocator().set_name(EAPATCHCLIENT_ALLOC_PREFIX "MessageSystem/Queue");
//
#ifndef EAPATCHCLIENT_ALLOC_PREFIX
    #define EAPATCHCLIENT_ALLOC_PREFIX "EAPatchClient/"
#endif



#endif // Header include guard



