///////////////////////////////////////////////////////////////////////////////
// Config.h
//
// Copyright (c) 2006 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EACALLSTACK_INTERNAL_CONFIG_H
#define EACALLSTACK_INTERNAL_CONFIG_H


#include <EABase/eabase.h>


///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_VERSION
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
//     printf("EACALLSTACK version: %s", EACALLSTACK_VERSION);
//     printf("EACALLSTACK version: %d.%d.%d", EACALLSTACK_VERSION_N / 10000 % 100, EACALLSTACK_VERSION_N / 100 % 100, EACALLSTACK_VERSION_N % 100);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EACALLSTACK_VERSION
	#define EACALLSTACK_VERSION   "2.01.01"
	#define EACALLSTACK_VERSION_N  20101
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

#if !defined(EA_WINAPI_PARTITION_NONE) && !defined(EA_WINAPI_PARTITION_DESKTOP)
	#if defined(EA_PLATFORM_MICROSOFT)
		#if defined(EA_PLATFORM_WINDOWS)
			#define EA_WINAPI_PARTITION_DESKTOP 1
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
// EACALLSTACK_LIBSTDCPP_DEMANGLE_AVAILABLE
//
// Defined as 0 or 1. The value depends on the compilation environment.
// Indicates if we can use system-provided abi::__cxa_demangle() at runtime.
//
#if !defined(EACALLSTACK_LIBSTDCPP_DEMANGLE_AVAILABLE)
	#if (defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_APPLE)) && defined(EA_PLATFORM_DESKTOP) // Do other variations support it too?
		#define EACALLSTACK_LIBSTDCPP_DEMANGLE_AVAILABLE 1
	#else
		#define EACALLSTACK_LIBSTDCPP_DEMANGLE_AVAILABLE 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_LLVM_DEMANGLE_AVAILABLE
//
// Defined as 0 or 1. The value depends on the compilation environment.
// Indicates if we can use our local abi::__cxa_demangle() implementation at runtime.
// Our local implementation is MIT licensed open source, but projects still need to
// get formal approval for using it.
//
#if !defined(EACALLSTACK_LLVM_DEMANGLE_AVAILABLE)
	#if EACALLSTACK_LIBSTDCPP_DEMANGLE_AVAILABLE // If the system already provides it, no need to do so ourselves.
		#define EACALLSTACK_LLVM_DEMANGLE_AVAILABLE 0
	#elif (defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)) || defined(EA_PLATFORM_DESKTOP)
		#define EACALLSTACK_LLVM_DEMANGLE_AVAILABLE 1
	#else
		#define EACALLSTACK_LLVM_DEMANGLE_AVAILABLE 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_MODULE_INFO_SUPPORTED
//
// Defined as 0 or 1. 
// Indicates if the given platform reliably supports the GetModule* functions.
//
#if !defined(EACALLSTACK_MODULE_INFO_SUPPORTED)
	#if defined(EA_PLATFORM_MICROSOFT) || defined(EA_PLATFORM_SONY) || defined(EA_PLATFORM_APPLE)
		#define EACALLSTACK_MODULE_INFO_SUPPORTED 1
	#else
		#define EACALLSTACK_MODULE_INFO_SUPPORTED 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_MS_PDB_READER_ENABLED
//
// Defined as 0 or 1, with 1 being the default under Windows.
// If enabled, causes the Microsoft-supplied PDB reader to be used whenever
// possible (on platforms where it is available). Usually you will want to
// have this enabled, but sometimes you might want to disable it, such as 
// when you want to do cross-platform code debugging. 
//
#ifndef EACALLSTACK_MS_PDB_READER_ENABLED
	#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		#define EACALLSTACK_MS_PDB_READER_ENABLED 1
	#else
		#define EACALLSTACK_MS_PDB_READER_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_MSVC_DEMANGLE_ENABLED
//
// Defined as 0 or 1, with 1 being the default on platforms that benefit from it.
// If enabled, the VC++ symbol name demangling code is enabled.
//
#ifndef EACALLSTACK_MSVC_DEMANGLE_ENABLED
	#if defined(EA_PLATFORM_MICROSOFT)
		#define EACALLSTACK_MSVC_DEMANGLE_ENABLED 1
	#else
		#define EACALLSTACK_MSVC_DEMANGLE_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_GCC_DEMANGLE_ENABLED
//
// Defined as 0 or 1, with 1 being the default on platforms that benefit from it.
// If enabled, the GCC symbol name demangling code is enabled. Note that GCC 
// mangling is used by the SN compiler and EDG-based compilers.
//
#ifndef EACALLSTACK_GCC_DEMANGLE_ENABLED
	#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_UNIX) || defined(__GNUC__) || defined(__clang__)
		#define EACALLSTACK_GCC_DEMANGLE_ENABLED 1
	#else
		#define EACALLSTACK_GCC_DEMANGLE_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_MSVC_MAPFILE_ENABLED
//
// Defined as 0 or 1, with 1 being the default on platforms that benefit from it.
// If enabled, the VC++ .map file address/symbol lookup is enabled.
//
#ifndef EACALLSTACK_MSVC_MAPFILE_ENABLED
	#if defined(_MSC_VER) || defined(EA_PLATFORM_DESKTOP)
		#define EACALLSTACK_MSVC_MAPFILE_ENABLED 1
	#else
		#define EACALLSTACK_MSVC_MAPFILE_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_GCC_MAPFILE_ENABLED
//
// Defined as 0 or 1, with 1 being the default on platforms that benefit from it.
// If enabled, the GCC .map file address/symbol lookup is enabled.
// Note that this does not refer to Apple's GCC (which is entirely different),
// which is handled by EACALLSTACK_APPLE_MAPFILE_ENABLED. 
//
#ifndef EACALLSTACK_GCC_MAPFILE_ENABLED
	#if defined(__GNUC__) || defined(__clang__) || defined(EA_PLATFORM_DESKTOP)
		#define EACALLSTACK_GCC_MAPFILE_ENABLED 1
	#else
		#define EACALLSTACK_GCC_MAPFILE_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_APPLE_MAPFILE_ENABLED
//
// Defined as 0 or 1, with 1 being the default on platforms that benefit from it.
// If enabled, the Apple GCC .map file address/symbol lookup is enabled.
//
#ifndef EACALLSTACK_APPLE_MAPFILE_ENABLED
	#if defined(EA_PLATFORM_APPLE) || defined(EA_PLATFORM_DESKTOP)
		#define EACALLSTACK_APPLE_MAPFILE_ENABLED 1
	#else
		#define EACALLSTACK_APPLE_MAPFILE_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_SN_MAPFILE_ENABLED
//
// Defined as 0 or 1, with 1 being the default on platforms that benefit from it.
// If enabled, the SN .map file address/symbol lookup is enabled.
// This format also covers the clang/llvm combination under Sony platforms.
//
#ifndef EACALLSTACK_SN_MAPFILE_ENABLED
	#if (defined(__clang__) && defined(EA_PLATFORM_SONY)) || defined(EA_PLATFORM_DESKTOP)
		#define EACALLSTACK_SN_MAPFILE_ENABLED 1
	#else
		#define EACALLSTACK_SN_MAPFILE_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_PDBFILE_ENABLED
//
// Defined as 0 or 1, with 1 being the default on platforms that benefit from it.
// If enabled, the VC++ .pdb file address/symbol lookup is enabled.
//
#ifndef EACALLSTACK_PDBFILE_ENABLED
	#if defined(_MSC_VER) || defined(EA_PLATFORM_DESKTOP)
		#define EACALLSTACK_PDBFILE_ENABLED 1
	#else
		#define EACALLSTACK_PDBFILE_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_DWARFFILE_ENABLED
//
// Defined as 0 or 1, with 1 being the default on platforms that benefit from it.
// If enabled, the VC++ .pdb file address/symbol lookup is enabled.
//
#ifndef EACALLSTACK_DWARFFILE_ENABLED
	#if defined(EA_PLATFORM_WINDOWS) || defined(__GNUC__) || defined(__clang__)
		#define EACALLSTACK_DWARFFILE_ENABLED 1
	#else
		#define EACALLSTACK_DWARFFILE_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_DASM_ARM_ENABLED
//
// Defined as 0 or 1, with 1 being the default on platforms that benefit from it.
// If enabled, the ARM disassembler code is enabled.
//
#ifndef EACALLSTACK_DASM_ARM_ENABLED
	#if defined(EA_PLATFORM_DESKTOP) || defined(EA_PROCESSOR_ARM)
		#define EACALLSTACK_DASM_ARM_ENABLED 1
	#else
		#define EACALLSTACK_DASM_ARM_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_DASM_POWERPC_ENABLED
//
// Defined as 0 or 1, with 1 being the default on platforms that benefit from it.
// If enabled, the PowerPC disassembler code is enabled.
//
#ifndef EACALLSTACK_DASM_POWERPC_ENABLED
	#if defined(EA_PLATFORM_DESKTOP) || defined(EA_PROCESSOR_POWERPC)
		#define EACALLSTACK_DASM_POWERPC_ENABLED 1
	#else
		#define EACALLSTACK_DASM_POWERPC_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_DASM_X86_ENABLED
//
// Defined as 0 or 1, with 1 being the default on platforms that benefit from it.
// If enabled, the x86 disassembler code is enabled.
//
#ifndef EACALLSTACK_DASM_X86_ENABLED
	#if defined(EA_PLATFORM_DESKTOP) || defined(EA_PROCESSOR_X86)
		#define EACALLSTACK_DASM_X86_ENABLED 1
	#else
		#define EACALLSTACK_DASM_X86_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_GLIBC_BACKTRACE_AVAILABLE
//
// You generally need to be using GCC, GLIBC, and Linux for backtrace to be available.
// And even then it's available only some of the time.
//
#if (defined(__clang__) || defined(__GNUC__)) && (defined(EA_PLATFORM_LINUX) || defined(__APPLE__)) && !defined(__CYGWIN__) && !defined(EA_PLATFORM_ANDROID)
	#define EACALLSTACK_GLIBC_BACKTRACE_AVAILABLE 1
#else
	#define EACALLSTACK_GLIBC_BACKTRACE_AVAILABLE 0
#endif



///////////////////////////////////////////////////////////////////////////////
// EA_CALLSTACK_GETCALLSTACK_SUPPORTED
//
// Defined as 0 or 1.
// Identifies whether runtime callstack unwinding (i.e. GetCallstack()) is 
// supported for the given platform. In some cases it may be that unwinding 
// support code is present but it hasn't been tested for reliability and may
// have bugs preventing it from working properly. In some cases (e.g. x86) 
// it may be that optimized builds make it difficult to read the callstack 
// reliably, despite that we flag the platform as supported.
//
#if EACALLSTACK_GLIBC_BACKTRACE_AVAILABLE               // Typically this means Linux on x86.
	#define EA_CALLSTACK_GETCALLSTACK_SUPPORTED 1
#elif defined(EA_PLATFORM_IPHONE)
	#define EA_CALLSTACK_GETCALLSTACK_SUPPORTED 0       // Support for these ARM targets is currently in progress.
#elif defined(EA_PLATFORM_ANDROID)
	#define EA_CALLSTACK_GETCALLSTACK_SUPPORTED 1
#elif defined(EA_PLATFORM_IPHONE_SIMULATOR)             // This is an x64 device but support hasn't been verified.
	#define EA_CALLSTACK_GETCALLSTACK_SUPPORTED 0
#elif defined(EA_PLATFORM_MICROSOFT)
	#define EA_CALLSTACK_GETCALLSTACK_SUPPORTED 1
#elif defined(EA_PLATFORM_LINUX)
	#define EA_CALLSTACK_GETCALLSTACK_SUPPORTED 1
#elif defined(EA_PLATFORM_OSX)
	#define EA_CALLSTACK_GETCALLSTACK_SUPPORTED 1
#elif defined(EA_PLATFORM_CYGWIN)                       // Support hasn't been verified.
	#define EA_CALLSTACK_GETCALLSTACK_SUPPORTED 0
#elif defined(EA_PLATFORM_MINGW)                        // Support hasn't been verified.
	#define EA_CALLSTACK_GETCALLSTACK_SUPPORTED 0
#endif



///////////////////////////////////////////////////////////////////////////////
// EA_CALLSTACK_OTHER_THREAD_CONTEXT_SUPPORTED
//
// Defined as 0 or 1.
// Identifies whether the GetCallstackContext(CallstackContext& context, intptr_t threadId)
// function is supported. This is significant because it identifies whether one
// thread can read the callstack of another thread.
//
#if defined(EA_PLATFORM_MICROSOFT)
	#define EA_CALLSTACK_OTHER_THREAD_CONTEXT_SUPPORTED 1
#elif defined(EA_PLATFORM_APPLE)
	#define EA_CALLSTACK_OTHER_THREAD_CONTEXT_SUPPORTED 1
#else
	#define EA_CALLSTACK_OTHER_THREAD_CONTEXT_SUPPORTED 0
#endif



///////////////////////////////////////////////////////////////////////////////
// _GNU_SOURCE
//
// Defined or not defined.
// If this is defined then GlibC extension functionality is enabled during 
// calls to glibc header files.
//
#if !defined(_GNU_SOURCE)
	#define _GNU_SOURCE
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_OFFLINE
//
// Defined as 0 or 1. 
// Online means that EACallstack is being used to read the current process.
// Offline means that EACallstack is being used to read data from another 
// process, in a way that supports multiple platforms, as with a tool.
//
#ifndef EACALLSTACK_OFFLINE
	#if defined(EA_PLATFORM_DESKTOP)
		#define EACALLSTACK_OFFLINE 1 // Actually it may or may not be offline. For runtime apps it is, for tools it isn't.
	#else
		#define EACALLSTACK_OFFLINE 0 // Unless somebody is writing a user tool to run on a console/phone, this is 0.
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_DEBUG_DETAIL_ENABLED
//
// Defined as 0 or 1. 
// If true then detailed debug info is displayed. Can be enabled in opt builds.
//
#ifndef EACALLSTACK_DEBUG_DETAIL_ENABLED
	#define EACALLSTACK_DEBUG_DETAIL_ENABLED 0
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_ALLOC_PREFIX
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
//     void* p = pCoreAllocator->Alloc(37, EACALLSTACK_ALLOC_PREFIX, 0);
//
// Example usage:
//     gMessageServer.GetMessageQueue().get_allocator().set_name(EACALLSTACK_ALLOC_PREFIX "MessageSystem/Queue");
//
#ifndef EACALLSTACK_ALLOC_PREFIX
	#define EACALLSTACK_ALLOC_PREFIX "EACallstack/"
#endif



///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_DEFAULT_ALLOCATOR_ENABLED
//
// Defined as 0 or 1. Default is 1 (for backward compatibility).
// Enables the use of ICoreAllocator::GetDefaultAllocator.
// If disabled then the user is required to explicitly specify the 
// allocator used by EACALLSTACK with EA::Callstack::SetAllocator() or 
// on a class by class basis within EACALLSTACK.
//
#ifndef EACALLSTACK_DEFAULT_ALLOCATOR_ENABLED
	#define EACALLSTACK_DEFAULT_ALLOCATOR_ENABLED 1
#endif


///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_ENABLE_ELF_SPAWN
//
// Defined as 0 or 1. Default is 1.
// Enables use of a spawned elf reading utility (addr2line.exe) to deal with 
// elf files that somehow can't be read by our native code. On Windows platforms
// only (not supported on others).
// This can be enabled for shipping products only if you get legal clearance
// to distribute addr2line.exe (part of gnu binutils). This is not the same 
// issue as the issue of building code with gnu licensed code, as this is 
// the distribution of a pre-existing binary, which is much less of a concern.
// 
#ifndef EACALLSTACK_ENABLE_ELF_SPAWN
	#define EACALLSTACK_ENABLE_ELF_SPAWN 1
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
// EACALLSTACK_DLL
//
// Defined as 0 or 1. The default is dependent on the definition of EA_DLL.
// If EA_DLL is defined, then EACALLSTACK_DLL is 1, else EACALLSTACK_DLL is 0.
// EA_DLL is a define that controls DLL builds within the EAConfig build system. 
// EACALLSTACK_DLL controls whether EACallstack is built and used as a DLL. 
// Normally you wouldn't do such a thing, but there are use cases for such
// a thing, particularly in the case of embedding C++ into C# applications.
//
#ifndef EACALLSTACK_DLL
	#if defined(EA_DLL)
		#define EACALLSTACK_DLL 1
	#else
		#define EACALLSTACK_DLL 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_API
//
// This is used to label functions as DLL exports under Microsoft platforms.
// If EA_DLL is defined, then the user is building EACallstack as a DLL and EACallstack's
// non-templated functions will be exported. EACallstack template functions are not
// labelled as EACALLSTACK_API (and are thus not exported in a DLL build). This is 
// because it's not possible (or at least unsafe) to implement inline templated 
// functions in a DLL.
//
// Example usage of EACALLSTACK_API:
//    EACALLSTACK_API int someVariable = 10;  // Export someVariable in a DLL build.
//
//    struct EACALLSTACK_API SomeClass{       // Export SomeClass and its member functions in a DLL build.
//    };
//
//    EACALLSTACK_API void SomeFunction();    // Export SomeFunction in a DLL build.
//
//
#ifndef EACALLSTACK_API // If the build file hasn't already defined this to be dllexport...
	#if EACALLSTACK_DLL && defined(_MSC_VER)
		#define EACALLSTACK_API           __declspec(dllimport)
		#define EACALLSTACK_TEMPLATE_API  // Not sure if there is anything we can do here.
	#else
		#define EACALLSTACK_API
		#define EACALLSTACK_TEMPLATE_API
	#endif
#endif
///////////////////////////////////////////////////////////////////////////////
// EACALLSTACK_API
//
// This is used to label functions as DLL exports under Microsoft platforms.
// If EA_DLL is defined, then the user is building EACallstack as a DLL and EACallstack's
// non-templated functions will be exported. EACallstack template functions are not
// labelled as EACALLSTACK_API (and are thus not exported in a DLL build). This is 
// because it's not possible (or at least unsafe) to implement inline templated 
// functions in a DLL.
//
// Example usage of EACALLSTACK_API:
//    EACALLSTACK_API int someVariable = 10;         // Export someVariable in a DLL build.
//
//    struct EACALLSTACK_API SomeClass{              // Export SomeClass and its member functions in a DLL build.
//        EACALLSTACK_LOCAL void PrivateMethod();    // Not exported.
//    };
//
//    EACALLSTACK_API void SomeFunction();           // Export SomeFunction in a DLL build.
//
// For GCC, see http://gcc.gnu.org/wiki/Visibility
//
#ifndef EACALLSTACK_API // If the build file hasn't already defined this to be dllexport...
	#if EACALLSTACK_DLL 
		#if defined(_MSC_VER)
			#define EACALLSTACK_API      __declspec(dllimport)
			#define EACALLSTACK_LOCAL
		#elif defined(__CYGWIN__)
			#define EACALLSTACK_API      __attribute__((dllimport))
			#define EACALLSTACK_LOCAL
		#elif (defined(__GNUC__) && (__GNUC__ >= 4))
			#define EACALLSTACK_API      __attribute__ ((visibility("default")))
			#define EACALLSTACK_LOCAL    __attribute__ ((visibility("hidden")))
		#else
			#define EACALLSTACK_API
			#define EACALLSTACK_LOCAL
		#endif
	#else
		#define EACALLSTACK_API
		#define EACALLSTACK_LOCAL
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
#ifndef EA_UNUSED
	// The EDG solution below is pretty weak and needs to be augmented or replaced.
	// It can't handle the C language, is limited to places where template declarations
	// can be used, and requires the type x to be usable as a functions reference argument. 
	#if defined(__cplusplus) && defined(__EDG__)
		template <typename T>
		inline void EABaseUnused(T const volatile & x) { (void)x; }
		#define EA_UNUSED(x) EABaseUnused(x)
	#else
		#define EA_UNUSED(x) (void)x
	#endif
#endif



#endif // Header include guard

















