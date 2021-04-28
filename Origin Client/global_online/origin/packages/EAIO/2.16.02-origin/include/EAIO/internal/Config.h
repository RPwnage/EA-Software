///////////////////////////////////////////////////////////////////////////////
// Config.h
//
// Copyright (c) 2006 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EAIO_INTERNAL_CONFIG_H
#define EAIO_INTERNAL_CONFIG_H


#include <EABase/eabase.h>


///////////////////////////////////////////////////////////////////////////////
// EAIO_VERSION
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
//     printf("EAIO version: %s", EAIO_VERSION);
//     printf("EAIO version: %d.%d.%d", EAIO_VERSION_N / 10000 % 100, EAIO_VERSION_N / 100 % 100, EAIO_VERSION_N % 100);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EAIO_VERSION
    #define EAIO_VERSION   "2.16.02"
    #define EAIO_VERSION_N  21602
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
// EAIO_INIFILE_ENABLED
//
// If defined as 1, then the IniFile class is made available. The reason it is 
// disabled by default, at least for the time being, is that it conflicts with
// the existing UTFFoundation identical class. EAIniFile is in fact in the 
// process of being moved from UTFFoundation to EAIO so that UTFFoundation can
// be formally deprecated.
//
#ifndef EAIO_INIFILE_ENABLED
    #define EAIO_INIFILE_ENABLED 1
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_WCHAR / EA_CHAR16 / EA_CHAR32
//
// EA_CHAR16 is defined in EABase 2.0.20 and later. If we are using an earlier
// version of EABase then we replicate what EABase 2.0.20 does.
//
#ifndef EA_WCHAR
     #define EA_WCHAR(s) L ## s
#endif

#ifndef EA_CHAR16
    #if !defined(EA_CHAR16_NATIVE)
        #if defined(_MSC_VER) && (_MSC_VER >= 1600) && defined(_HAS_CHAR16_T_LANGUAGE_SUPPORT) && _HAS_CHAR16_T_LANGUAGE_SUPPORT // VS2010+
            #define EA_CHAR16_NATIVE 1
        #elif defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 404) && (defined(__GXX_EXPERIMENTAL_CXX0X__) || defined(__STDC_VERSION__)) // g++ (C++ compiler) 4.4+ with -std=c++0x or gcc (C compiler) 4.4+ with -std=gnu99
            #define EA_CHAR16_NATIVE 1
        #else
            #define EA_CHAR16_NATIVE 0
        #endif
    #endif

    #if EA_CHAR16_NATIVE && !defined(_MSC_VER) // Microsoft doesn't support char16_t string literals.
        #define EA_CHAR16(s) u ## s
    #elif (EA_WCHAR_SIZE == 2)
        #define EA_CHAR16(s) L ## s
    #endif
#endif


#ifndef EA_WCHAR_UNIQUE
    #define EA_WCHAR_UNIQUE 0
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
// EA_HAVE_SYS_STAT_H
//
// Implemented in recent versions of EABase.
//
#if !defined(EA_HAVE_SYS_STAT_H) && !defined(EA_NO_HAVE_SYS_STAT_H)
    #if (defined(EA_PLATFORM_UNIX) && !(defined(EA_PLATFORM_SONY) && defined(EA_PLATFORM_CONSOLE))) || defined(__APPLE__) || defined(EA_PLATFORM_BADA) || defined(EA_PLATFORM_AIRPLAY) || defined(EA_PLATFORM_ANDROID) || defined(EA_PLATFORM_PLAYBOOK)
        #define EA_HAVE_SYS_STAT_H 1 /* declares the stat struct and function */
    #else
        #define EA_NO_HAVE_SYS_STAT_H 1
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_HAVE_UTIME_H
//
// May be implemented in recent versions of EABase.
// Not the same header path as <sys/utime.h>
//
#if !defined(EA_HAVE_UTIME_H) && !defined(EA_NO_HAVE_UTIME_H)
    #if (defined(EA_PLATFORM_UNIX) && !defined(EA_PLATFORM_CONSOLE)) || defined(EA_PLATFORM_AIRPLAY) || defined(EA_PLATFORM_PLAYBOOK) || defined(EA_PLATFORM_BADA)
        #define EA_HAVE_UTIME_H 1 /* declares the utime function */
    #else
        #define EA_NO_HAVE_UTIME_H 1
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_HAVE_STATVFS_H
//
// May be implemented in recent versions of EABase.
// Not the same header path as <sys/statvfs.h>
//
#if !defined(EA_HAVE_STATVFS_H) && !defined(EA_NO_HAVE_STATVFS_H)
    #if (defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_BSD) || defined(__APPLE__)) && !defined(EA_PLATFORM_SONY) && !defined(__CYGWIN__) && !defined(EA_PLATFORM_ANDROID) && !defined(EA_PLATFORM_BADA) && !defined(EA_PLATFORM_AIRPLAY)
        #define EA_HAVE_STATVFS_H 1 /* declares the statvfs function */
    #else
        #define EA_NO_HAVE_STATVFS_H 1
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EAIO_ALLOC_PREFIX
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
//     void* p = pCoreAllocator->Alloc(37, EAIO_ALLOC_PREFIX, 0);
//
// Example usage:
//     gMessageServer.GetMessageQueue().get_allocator().set_name(EAIO_ALLOC_PREFIX "MessageSystem/Queue");
//
#ifndef EAIO_ALLOC_PREFIX
    #define EAIO_ALLOC_PREFIX "EAIO/"
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
// EAIO_USE_CORESTREAM
//
// Defined as 0 or 1. Default is 0 until some possible day in the future.
// The CoreStream package implements an interface nearly the same as the 
// one here, but is a tiny single-header package, for simplicity.
// If EAIO_USE_CORESTREAM is enabled then our IStream interface uses the 
// ICoreStream interface from the CoreStream package. That interface is 
// nearly identical to our IStream interface.
// If you want to override EAIO_USE_CORESTREAM, you must do it globally, 
// since EAIO interfaces are affected by it.
//
#ifndef EAIO_USE_CORESTREAM
    #define EAIO_USE_CORESTREAM 0
#endif

#if EAIO_USE_CORESTREAM                     // The ICoreStream interface always works
    #ifdef EAIO_64_BIT_SIZE_ENABLED         // in 64 bit mode, so we unilaterally 
        #undef EAIO_64_BIT_SIZE_ENABLED     // enable it.
    #endif
    #define EAIO_64_BIT_SIZE_ENABLED 1
#endif



///////////////////////////////////////////////////////////////////////////////
// EAIO_64_BIT_SIZE_ENABLED
//
// Defined as 0 or 1. Default is 0.
// Determines if size_type and off_type are 64 bits when the platform is otherwise
// only 32 bits. 
//
#ifndef EAIO_64_BIT_SIZE_ENABLED
    #if (EA_PLATFORM_PTR_SIZE >= 8)
        #define EAIO_64_BIT_SIZE_ENABLED 1
    #else
        #define EAIO_64_BIT_SIZE_ENABLED 0  // To consider: change this to 1 some day.
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EAIO_DEFAULT_ALLOCATOR_IMPL_ENABLED
//
// Defined as 0 or 1. Default is 1 (for backward compatibility).
// Enables the implementation of ICoreAllocator::GetDefaultAllocator
// in DLL builds. If you build EAIO as a standalone DLL then you typically
// need provide a local implementation of the ICoreAllocator::GetDefaultAllocator 
// function. However, some user usage patterns are such that they would prefer
// to disable this implementation, because they link in a way that it causes 
// problems.
//
#ifndef EAIO_DEFAULT_ALLOCATOR_IMPL_ENABLED
    #define EAIO_DEFAULT_ALLOCATOR_IMPL_ENABLED 1
#endif


///////////////////////////////////////////////////////////////////////////////
// EAIO_DEFAULT_ALLOCATOR_ENABLED
//
// Defined as 0 or 1. Default is 1 (for backward compatibility).
// Enables the use of ICoreAllocator::GetDefaultAllocator.
// If disabled then the user is required to explicitly specify the 
// allocator used by EAIO with EA::IO::SetAllocator() or on a class by
// class basis within EAIO. Note that this option is different from the 
// EAIO_DEFAULT_ALLOCATOR_IMPL_ENABLED option. This option enables the 
// calling of GetDefaultAllocator by this package, the former enables the 
// actual implementation of GetDefaultAllocator within this package.
//
#ifndef EAIO_DEFAULT_ALLOCATOR_ENABLED
    #define EAIO_DEFAULT_ALLOCATOR_ENABLED 1
#endif


///////////////////////////////////////////////////////////////////////////////
// EAIO_DIRECTORY_ITERATOR_USE_PATHSTRING
//
// Defined as 0 or 1. 1 means to use EAIO PathString.
// This controls whether the DirectoryIterator class in EAFileDirectory.h 
// uses EAIO PathString or straight EASTL string16. The latter is the old 
// setting, and the former is the new setting. The problem with the old
// use of EASTL string16 is that it doesn't use the EAIO memory allocator
// and instead uses whatever global EASTL allocator is defined.
//
#ifndef EAIO_DIRECTORY_ITERATOR_USE_PATHSTRING
    #define EAIO_DIRECTORY_ITERATOR_USE_PATHSTRING 0  // Defaults to zero until old package versions which depend on it as zero are obsolete.
#endif


///////////////////////////////////////////////////////////////////////////////
// EAIO_FILEPATH_ENABLED
//
// Defined as 0 or 1. Default is 0.
// Defines if the old EAFilePath.h/cpp functionality in the compat directory
// is enabled and available. This #define doesn't enable compiling of that
// module; the build system has to do that.
//
#ifndef EAIO_FILEPATH_ENABLED
    #define EAIO_FILEPATH_ENABLED 0
#endif


///////////////////////////////////////////////////////////////////////////////
// EAIO_BACKWARDS_COMPATIBILITY
//
// Defined as 0 or 1. Default is 1 (for backward compatibility).
// Enables some deprecated features.
// To do: Set EAIO_BACKWARDS_COMPATIBILITY to default to zero (disabled) 
// by about Spring of 2009.
//
#ifndef EAIO_BACKWARDS_COMPATIBILITY
    #define EAIO_BACKWARDS_COMPATIBILITY 1
#endif


///////////////////////////////////////////////////////////////////////////////
// EAIO_DLL
//
// Defined as 0 or 1. The default is dependent on the definition of EA_DLL.
// If EA_DLL is defined, then EAIO_DLL is 1, else EAIO_DLL is 0.
// EA_DLL is a define that controls DLL builds within the EAConfig build system. 
// EAIO_DLL controls whether EAIO is built and used as a DLL. 
// Normally you wouldn't do such a thing, but there are use cases for such
// a thing, particularly in the case of embedding C++ into C# applications.
//
#ifndef EAIO_DLL
    #if defined(EA_DLL)
        #define EAIO_DLL 1
    #else
        #define EAIO_DLL 0
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EAIO_API
//
// This is used to label functions as DLL exports under Microsoft platforms.
// If EA_DLL is defined, then the user is building EAIO as a DLL and EAIO's
// non-templated functions will be exported. EAIO template functions are not
// labelled as EAIO_API (and are thus not exported in a DLL build). This is 
// because it's not possible (or at least unsafe) to implement inline templated 
// functions in a DLL.
//
// Example usage of EAIO_API:
//    EAIO_API int someVariable = 10;         // Export someVariable in a DLL build.
//
//    struct EAIO_API SomeClass{              // Export SomeClass and its member functions in a DLL build.
//        EAIO_LOCAL void PrivateMethod();    // Not exported.
//    };
//
//    EAIO_API void SomeFunction();           // Export SomeFunction in a DLL build.
//
// For GCC, see http://gcc.gnu.org/wiki/Visibility
//
#ifndef EAIO_API // If the build file hasn't already defined this to be dllexport...
    #if EAIO_DLL 
        #if defined(_MSC_VER)
            #define EAIO_API      __declspec(dllimport)
            #define EAIO_LOCAL
        #elif defined(__CYGWIN__)
            #define EAIO_API      __attribute__((dllimport))
            #define EAIO_LOCAL
        #elif (defined(__GNUC__) && (__GNUC__ >= 4))
            #define EAIO_API      __attribute__ ((visibility("default")))
            #define EAIO_LOCAL    __attribute__ ((visibility("hidden")))
        #else
            #define EAIO_API
            #define EAIO_LOCAL
        #endif
    #else
        #define EAIO_API
        #define EAIO_LOCAL
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EAIO_USE_UNIX_IO
//
// Defined as 0 or 1.
// If defined as 1 then Unix IO API calls will be used, even if EA_PLATFORM_UNIX
// isn't defined.
//
#ifndef EAIO_USE_UNIX_IO
    #if defined(EA_PLATFORM_UNIX) || defined(__APPLE__) || defined(EA_PLATFORM_BADA) || defined(EA_PLATFORM_AIRPLAY) || defined(EA_PLATFORM_ANDROID) || defined(EA_PLATFORM_PLAYBOOK)
        #define EAIO_USE_UNIX_IO 1
    #else
        #define EAIO_USE_UNIX_IO 0
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EAIO_APPLE_USE_UNIX_IO
//
// Defined as 0 or 1.
// If defined as 1 then we use Unix IO calls instead of Apple Cocoa framework
// system calls. Normally you'd want to use the Framework calls but command line
// console applications might not want to or be able to drag in a UI framework.
//
#ifndef EAIO_APPLE_USE_UNIX_IO
    #define EAIO_APPLE_USE_UNIX_IO 1
#endif



///////////////////////////////////////////////////////////////////////////////
// EAIO_THREAD_SAFETY_ENABLED
//
// Defined as 0 or 1.
// If defined as 1 then thread safety is enabled where appropriate. See the documentation
// for individual classes and functions or information about how thread safety impacts them.
//
#ifndef EAIO_THREAD_SAFETY_ENABLED
    #define EAIO_THREAD_SAFETY_ENABLED 1
#endif



///////////////////////////////////////////////////////////////////////////////
// EAIO_FTRUNCATE_PRESENT
//
// Defined as 0 or 1.
// Defines if the ftruncate function is present. If it is not present then
// some platform-specific implementation needs to be implemented. Usually this
// function is present on Unix systems, but not always.
//
#ifndef EAIO_FTRUNCATE_PRESENT
    #if defined(EA_PLATFORM_BADA) && !defined(__MINGW32__) // Add other systems as needed.
        #define EAIO_FTRUNCATE_PRESENT 0
    #else
        #define EAIO_FTRUNCATE_PRESENT 1
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EAIO_MKDIR_PRESENT
//
// Defined as 0 or 1.
//
#ifndef EAIO_MKDIR_PRESENT
    #if defined(EA_PLATFORM_SONY) || defined(EA_PLATFORM_NINTENDO)
        #define EAIO_MKDIR_PRESENT 0
    #else
        #define EAIO_MKDIR_PRESENT 1
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EAIO_UTIME_PRESENT
//
// Defined as 0 or 1.
// Defines if the utime (or _utime) function is present. If it is not present 
// then some platform-specific implementation needs to be implemented. Usually this
// function is present on Unix systems, but not always. It's usually found in the 
// <utime.h> or <sys/utime.h> header file.
//
#ifndef EAIO_UTIME_PRESENT
    #if defined(EA_PLATFORM_BADA) || defined(EA_PLATFORM_AIRPLAY) || (defined(EA_PLATFORM_UNIX) && defined(EA_PLATFORM_CONSOLE)) // Add other systems as needed.
        #define EAIO_UTIME_PRESENT 0
    #else
        #define EAIO_UTIME_PRESENT 1
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EAIO_DIRENT_PRESENT
//
// Defined as 0 or 1.
// Defines if the dirent (directory entry iteration) function is present. If it is 
// not present then some platform-specific implementation needs to be implemented.
// Usually this function is present on Unix systems, but not always.
// To consider: Eventually switch this to use EABase's EA_HAVE_DIRENT_H.
//
#ifndef EAIO_DIRENT_PRESENT
    #if defined(EA_PLATFORM_BADA) || (defined(EA_PLATFORM_SONY) && defined(EA_PLATFORM_CONSOLE))
        #define EAIO_DIRENT_PRESENT 0
    #else
        #define EAIO_DIRENT_PRESENT 1
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EAIO_READDIR_R_PRESENT
//
// Defined as 0 or 1.
// Defines if the readdir_r function is present in dirent.h. Else the readdir function.
//
#if !defined(EAIO_READDIR_R_PRESENT)
    #if EAIO_USE_UNIX_IO && !defined(EA_PLATFORM_AIRPLAY)
        #define EAIO_READDIR_R_PRESENT 1
    #else
        #define EAIO_READDIR_R_PRESENT 0
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EAIO_CPP_STREAM_ENABLED
//
// Defined as 0 or 1.
// Some platforms don't support the C++ standard library.
//
#ifndef EAIO_CPP_STREAM_ENABLED
    #if defined(EA_PLATFORM_ANDROID)        // Android doesn't support the C++ standard library; at least not very well.
        #define EAIO_CPP_STREAM_ENABLED 0
    #elif defined(EA_PLATFORM_DESKTOP)
        #define EAIO_CPP_STREAM_ENABLED 1
    #elif defined(EA_PLATFORM_REVOLUTION) || defined(EA_PLATFORM_PS3) || defined(EA_PLATFORM_XENON)   // Enabled for backward compatibility.
        #define EAIO_CPP_STREAM_ENABLED 1
    #else
        #define EAIO_CPP_STREAM_ENABLED 0   // Most gaming platform games don't need nor use this functionality.
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// FCN_POLL_ENABLED
//
// Defined as 0 or 1, with the default being 1 only for platforms that have 
// no other option (i.e. typically console platforms).
// Enables file change notification via manual polling instead of via an 
// automatic (possibly threaded) background operation. Polling is the only 
// option on some platforms, and for these platforms it doesn't matter what
// value FCN_POLL_ENABLED is set to.
//
#ifndef FCN_POLL_ENABLED
    #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
        #define FCN_POLL_ENABLED 0
    #else
        #define FCN_POLL_ENABLED 1
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EA_STREAM_SYSTEM_ENABLED
//
// This define is deprecated; it is provided for backwards compatibility.
//
#define EA_STREAM_SYSTEM_ENABLED 1



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
// EAIO_WINDOWS8_EMULATION_ENABLED
//
// Defined as 0 or 1.
// Identifies if Windows8 file functions are force-emulated when not present.
//
#if !defined(EAIO_WINDOWS8_EMULATION_ENABLED)
    #define EAIO_WINDOWS8_EMULATION_ENABLED 1
#endif




#endif // Header include guard

















