/*! *********************************************************************************************/
/*!
    \file   config.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_INTERNAL_CONFIG_H
#define EA_TDF_INTERNAL_CONFIG_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
// EATDF_USER_CONFIG_HEADER
//
// This allows the user to define a header file to be #included before the 
// EATDF config.h contents are compiled. A primary use of this is to override
// the contents of this config.h file. Note that all the settings below in 
// this file are user-overridable.
// 
// Example usage:
//     #define EATDF_USER_CONFIG_HEADER "MyConfigOverrides.h"
//     #include <EATDF/tdfbase.h>
//
///////////////////////////////////////////////////////////////////////////////
#ifdef EATDF_USER_CONFIG_HEADER
    #include EATDF_USER_CONFIG_HEADER
#endif


#include <EABase/eabase.h>
#include <coreallocator/icoreallocatormacros.h>
#include <EATDF/internal/deprecatedconfig.h>
#include <EAAssert/eaassert.h>

// The following two methods are intentionally private and unimplemented to prevent passing
// and returning objects by value.  The compiler will give a linking error if either is used.
#define EA_TDF_NON_COPYABLE(_class_) private:\
    /* Copy constructor */ \
    _class_(const _class_& otherObj);\
    /* Assignment operator */ \
    _class_& operator=(const _class_& otherObj);


#define EA_TDF_NON_ASSIGNABLE(_class_) private:\
    /* Assignment operator */ \
    _class_& operator=(const _class_& otherObj);

///////////////////////////////////////////////////////////////////////////////
// EATDF_DLL
//
// Defined as 0 or 1. The default is dependent on the definition of EA_DLL.
// If EA_DLL is defined, then EATDF_DLL is 1, else EATDF_DLL is 0.
// EA_DLL is a define that controls DLL builds within the EAConfig build system. 
// EATDF_DLL controls whether EATDF is built and used as a DLL. 
// Normally you wouldn't do such a thing, but there are use cases for such
// a thing, particularly in the case of embedding C++ into C# applications.
//
#ifndef EATDF_DLL
    #if defined(EA_DLL)
        #define EATDF_DLL 1
    #else
        #define EATDF_DLL 0
    #endif
#endif

///////////////////////////////////////////////////////////////////////////////
// EATDF_API
//
// This is used to label functions as DLL exports under Microsoft platforms.
// If EA_DLL is defined, then the user is building EATDF as a DLL and EATDF's
// non-templated functions will be exported. EATDF template functions are not
// labelled as EATDF_API (and are thus not exported in a DLL build). This is 
// because it's not possible (or at least unsafe) to implement inline templated 
// functions in a DLL.
//
// Example usage of EATDF_API:
//    EATDF_API int someVariable = 10;         // Export someVariable in a DLL build.
//
//    struct EATDF_API SomeClass{              // Export SomeClass and its member functions in a DLL build.
//        EATDF_LOCAL void PrivateMethod();    // Not exported.
//    };
//
//    EATDF_API void SomeFunction();           // Export SomeFunction in a DLL build.
//
// For GCC, see http://gcc.gnu.org/wiki/Visibility
//
#ifndef EATDF_API // If the build file hasn't already defined this to be dllexport...
    #if EATDF_DLL 
        #if defined(_MSC_VER)
            #define EATDF_API      __declspec(dllimport)
            #define EATDF_LOCAL
        #elif defined(__CYGWIN__)
            #define EATDF_API      __attribute__((dllimport))         
            #define EATDF_LOCAL
        #elif (defined(__GNUC__) && (__GNUC__ >= 4))
            #define EATDF_API      __attribute__ ((visibility("default")))           
            #define EATDF_LOCAL    __attribute__ ((visibility("hidden")))
        #else
            #define EATDF_API
            #define EATDF_LOCAL
        #endif
    #else
        #define EATDF_API
        #define EATDF_LOCAL   
    #endif
#endif

///////////////////////////////////////////////////////////////////////////////

#ifdef EA_PLATFORM_LINUX
#include <string.h>
#define EA_TDF_STRLEN strlen
#else
#define EA_TDF_STRLEN EA::StdC::Strlen
#endif

//Allows a customize allocator type to EATDF allocations
#ifndef EA_TDF_ALLOCATOR_TYPE
#define EA_TDF_ALLOCATOR_TYPE EA::Allocator::EASTLICoreAllocator
#endif

//Allows a customized method for providing a default allocator to the TDF construction calls
#ifndef EA_TDF_GET_DEFAULT_ICOREALLOCATOR
#define EA_TDF_GET_DEFAULT_ICOREALLOCATOR *EA::Allocator::ICoreAllocator::GetDefaultAllocator()
#endif

#ifndef EA_TDF_DEFAULT_ALLOC_NAME
#define EA_TDF_DEFAULT_ALLOC_NAME ""
#endif

// If enabled, all TDF types will be registered with TdfFactory via dynamic initialization before main(), 
// so we have larger code size and are thread-safe since it happens before threads are created.
// If disabled, only TDFs that were tagged explicitly with tdfid in *.tdf file will be registered with TdfFactory 
// so we have smaller code size and makes use of EA::Thread::Mutex to ensure thread safety
#ifndef EA_TDF_REGISTER_ALL
#define EA_TDF_REGISTER_ALL 1
#endif

//This is the internal allocator used where we want a default allocator as an argument to a call.
//Because EATDF can be built as a DLL, we do not want any default allocator calls, or else EATDF
//itself will need to define ICoreAllocator::GetDefaultAllocator  (as the build doesn't override it).
//For that reason, we define this here for everyone building outside of the library, and within the library's 
//confines we build with this macro set to nullptr.  
#if !EA_TDF_INTERNAL_BUILD
#define EA_TDF_EXT_ONLY_DEF_ARG(_x) = _x
#else
#define EA_TDF_EXT_ONLY_DEF_ARG(_x)
#endif

#define EA_TDF_DEFAULT_ALLOCATOR_ASSIGN EA_TDF_EXT_ONLY_DEF_ARG(EA_TDF_GET_DEFAULT_ICOREALLOCATOR)

#ifdef EA_COMPILER_GNUC
#define EA_TDF_ATTRIBUTE_PRINTF_CHECK(x,y) __attribute__((format(printf,x,y)))
#else
#define EA_TDF_ATTRIBUTE_PRINTF_CHECK(x,y)
#endif

//Enable to include description and details for each class/field.
#ifndef EA_TDF_INCLUDE_EXTENDED_TAG_INFO 
#define EA_TDF_INCLUDE_EXTENDED_TAG_INFO 0
#endif

//Enable to include information about whether or not fields can be reconfigured.
#ifndef EA_TDF_INCLUDE_RECONFIGURE_TAG_INFO
#define EA_TDF_INCLUDE_RECONFIGURE_TAG_INFO 0
#endif

//Enable to include reflection info for fetching fields by tag/name only if the TDF has been marked to do so.
#ifndef EA_TDF_INCLUDE_MEMBER_MERGE
#define EA_TDF_INCLUDE_MEMGER_MERGE 0
#endif

//Enable to allow TDF map/vectors to optionally own subelements instead of always owning them.
#ifndef EA_TDF_INCLUDE_OPTIONAL_SUBELEMENT_OWNERSHIP
#define EA_TDF_INCLUDE_OPTIONAL_SUBELEMENT_OWNERSHIP 0
#endif

//Enable to have copy constructors for TDFs.
#ifndef EA_TDF_INCLUDE_COPY_CONSTRUCTOR
#define EA_TDF_INCLUDE_COPY_CONSTRUCTOR 0
#endif

//Enable to allow custom attributes to be generated and used.
#ifndef EA_TDF_INCLUDE_CUSTOM_TDF_ATTRIBUTES
#define EA_TDF_INCLUDE_CUSTOM_TDF_ATTRIBUTES 0
#endif

//Enable to allow dirty bits to be set for each field of a TDF to allow "change only" copies.
#ifndef EA_TDF_INCLUDE_CHANGE_TRACKING
#define EA_TDF_INCLUDE_CHANGE_TRACKING 1
#endif

//Adds a fixup method to maps to allow sorting the map after decoding.
#ifndef EA_TDF_MAP_ELEMENT_FIXUP_ENABLED
#define EA_TDF_MAP_ELEMENT_FIXUP_ENABLED 0
#endif

// Enable backward compatibility with EATDF 1.05
//   When EATDF 1.05 was released, it broke backward the compatibility of heat2.
//   If this constnt is defined, it will re-enable the behaviour of 1.05.  This may
//   be necessary in cases where a client is locked on 1.05 but and the server need
//   to be made compatible.
#ifndef EA_TDF_1_05_COMPATIBILITY
#define EA_TDF_1_05_COMPATIBILITY 0
#endif

//If enabled, TDFFactory Registration methods are threadsafe on the TDFFactory singleton.
#ifndef EA_TDF_THREAD_SAFE_TDF_REGISTRATION_ENABLED
#define EA_TDF_THREAD_SAFE_TDF_REGISTRATION_ENABLED 0
#endif

// If enabled, TypeDescription initialization is threadsafe. Only consider enabling if EA_TDF_REGISTER_ALL=0 
// because if EA_TDF_REGISTER_ALL=1, all TypeDescription should be initialized using static initialization before main()
#ifndef EA_TDF_THREAD_SAFE_TYPEDESC_INIT_ENABLED
#if EA_TDF_REGISTER_ALL
#define EA_TDF_THREAD_SAFE_TYPEDESC_INIT_ENABLED 0
#else
#define EA_TDF_THREAD_SAFE_TYPEDESC_INIT_ENABLED 1
#endif
#endif

//If enabled, TdfString will use a small built-in buffer for tiny strings.  This increases its size, but decreases the number of dynamic allocations the system uses.
#ifndef EA_TDF_STRING_USE_DEFAULT_BUFFER
#define EA_TDF_STRING_USE_DEFAULT_BUFFER 0 
#endif

//If enabled, TdfMemberInfo will include memberNameOverride, which is used in REST protocol.
#ifndef EA_TDF_INCLUDE_MEMBER_NAME_OVERRIDE
#define EA_TDF_INCLUDE_MEMBER_NAME_OVERRIDE 1
#endif

//Changes the tdf_ptr to properly maintain const correct logic when getting values from a list or map.
#ifndef EA_TDF_PTR_CONST_SAFETY
#define EA_TDF_PTR_CONST_SAFETY 1
#endif


#define TDFCOMMA() ,

namespace EA {
    namespace TDF{
        typedef uint32_t TdfSizeVal;
    };
};

#endif // EA_TDF_INTERNAL_CONFIG_H

