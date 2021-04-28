/////////////////////////////////////////////////////////////////////////////
// EATraceBase.h
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana, Maxis
//
// Primary defines provided by this module include:
//    EATRACE_TRUE / EATRACE_FALSE
//    EA_PREPROCESSOR_JOIN
//    EA_PREPROCESSOR_STRINGIFY
//    EA_PREPROCESSOR_LOCATION
//    EA_UNUSED_VAR
//    EA_UNUSED_ARG
//    EA_UNIQUELY_NAMED_VARIABLE
//    EA_CURRENT_FUNCTION
//    EA_CURRENT_FUNCTION_PARAMS
//    EA_SWALLOW_ARGS
//    EA_NOOP
//    EA_DEBUG_TEXT
//    EA_DEBUG_BREAK
//    EA_COMPILETIME_ASSERT
//    EA_COMPILETIME_ASSERT_NAMED
//    EAOutputDebugString
//    EAOutputDebugStringF
/////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_EATRACEBASE_H
#define EATRACE_EATRACEBASE_H


#ifndef INCLUDED_eabase_H
    #include <EABase/eabase.h>
#endif
#include <stdio.h>


///////////////////////////////////////////////////////////////////////////////
/// EATRACE_TRUE / EATRACE_FALSE
///
/// These expressions are replacements for 'true' and 'false' but which aren't
/// seen as so by the compiler but only at runtime. Compilers such as VC++
/// will generate warnings (such as C4127: conditional expression is constant)
/// when you try to use expressions that it sees as being constant but are 
/// intentionally so. 
///
/// Given that EA_TRUE results in code being generated instead of code being
/// compiled away, you generally only want to use these expressions in code
/// that you don't need to be optimized, such as debug code and test code.
/// For high performance code, you are better off disabling the warning via
/// compiler directives.
///
/// Example usage:
///     #define MY_TRACE(str) do{ puts(str); } while(EATRACE_FALSE)
///
#if defined(__GNUC__) && (__GNUC__ >= 4)    // GCC generates warnings that are hard to work around.
    inline int EATrueDummyFunction() { return 1; }
    #ifndef EATRACE_TRUE
        #define EATRACE_TRUE  (EATrueDummyFunction())
    #endif

    #ifndef EATRACE_FALSE
        #define EATRACE_FALSE (!EATrueDummyFunction())
    #endif
#else
    void EATrueDummyFunction();
    #ifndef EATRACE_TRUE
        #define EATRACE_TRUE  (EATrueDummyFunction != NULL)
    #endif

    #ifndef EATRACE_FALSE
        #define EATRACE_FALSE (EATrueDummyFunction == NULL)
    #endif
#endif


///////////////////////////////////////////////////////////////////////////////
/// EA_PREPROCESSOR_JOIN
///
/// This macro joins the two arguments together, even when one of  
/// the arguments is itself a macro (see 16.3.1 in C++98 standard). 
/// This is often used to create a unique name with __LINE__.
///
/// For example, this declaration:
///    char EA_PREPROCESSOR_JOIN(unique_, __LINE__);
/// expands to this:
///    char unique_73;
///
/// Note that all versions of MSVC++ up to at least version 7.1 
/// fail to properly compile macros that use __LINE__ in them
/// when the "program database for edit and continue" option
/// is enabled. The result is that __LINE__ gets converted to 
/// something like __LINE__(Var+37).
///
#ifndef EA_PREPROCESSOR_JOIN
    #define EA_PREPROCESSOR_JOIN(a, b)  EA_PREPROCESSOR_JOIN1(a, b)
    #define EA_PREPROCESSOR_JOIN1(a, b) EA_PREPROCESSOR_JOIN2(a, b)
    #define EA_PREPROCESSOR_JOIN2(a, b) a##b
#endif



///////////////////////////////////////////////////////////////////////////////
/// EA_PREPROCESSOR_STRINGIFY
///
/// This macro takes a unquoted preprocessor term and makes a string consant
/// from it. 
///
/// For example, this declaration:
///    const char* pLine = EA_PREPROCESSOR_STRINGIFY(__LINE__);
/// Expands to this:
///    const char* pLine = "96";
///
/// For example, this declaration:
///    const char* pFileAndLine = __FILE__ " (" EA_PREPROCESSOR_STRINGIFY(__LINE__) ")";
/// Expands to this:
///    const char* pFileAndLine = "/project/include/eatrace.h (101)";
///
#ifndef EA_PREPROCESSOR_STRINGIFY
    #define EA_PREPROCESSOR_STRINGIFY(x)     EA_PREPROCESSOR_STRINGIFYIMPL(x)
    #define EA_PREPROCESSOR_STRINGIFYIMPL(x) #x
#endif



///////////////////////////////////////////////////////////////////////////////
/// EA_PREPROCESSOR_LOCATION
///
/// This macro creates a single string that includes the file and line
/// number of the location of the macro's usage.  This becomes a clickable
/// link to the file/line in the MSVC++ debugger if first on a line with no spaces.
///
/// For example, this declaration:
///    puts(EA_PREPROCESSOR_LOCATION);
/// Expands to this:
///    puts("/project/include/eatrace.h (123)");
///
/// Note that all versions of MSVC++ up to at least version 7.1 
/// fail to properly compile macros that use __LINE__ in them
/// when the "program database for edit and continue" option
/// is enabled. The result is that __LINE__ gets converted to 
/// something like __LINE__(Var+37).
///
#ifndef EA_PREPROCESSOR_LOCATION
    #define EA_PREPROCESSOR_LOCATION       __FILE__ "(" EA_PREPROCESSOR_STRINGIFY(__LINE__) ")"
    #define EA_PREPROCESSOR_LOCATION_FUNC  __FILE__ "(" EA_PREPROCESSOR_STRINGIFY(__LINE__) "): " EA_CURRENT_FUNCTION
#endif



//////////////////////////////////////////////////////////////////////////////
/// EA_UNUSED_VAR
///
/// Used to make compiler warnings related to unused arguments and local 
/// variables go away. It is designed to be used in the function body and 
/// not in the argument list. There doesn't seem to be a universal macro that
/// can be made for both function body usage and argument usage, due to the 
/// way that different compilers generate warnings. For use in the argument 
/// list, see EA_UNUSED_ARG.
///
/// Example usage:
///     void DoNothing(int x) {
///         EA_UNUSED_VAR(x);
///     }
///
#ifndef EA_UNUSED_VAR
    #define EA_UNUSED_VAR(x) (void)(x) // This works for all current compilers that we use.
#endif



//////////////////////////////////////////////////////////////////////////////
/// EA_UNUSED_ARG
///
/// Used to make compiler warnings related to unused fucntion arguments go away.
/// If you use this for local variables, you might get compiler warnings from
/// some compilers.
///
/// Example usage:
///     void DoNothing(int EA_UNUSED_ARG(x)) {
///     }
///
#ifndef EA_UNUSED_ARG
    #define EA_UNUSED_ARG(x)
#endif

#ifndef EA_UNUSED
    #define EA_UNUSED EA_UNUSED_ARG   // EA_UNUSED is deprecated, as it's not universally usable, though the name implies so.
#endif



///////////////////////////////////////////////////////////////////////////////
/// EA_UNIQUELY_NAMED_VARIABLE
///
/// This macro creates a dynamically defined local variable. It is useful 
/// in macros that need temporary counters
///
/// For example, this declaration:
///    void DoNothing(){
///        int EA_UNIQUELY_NAMED_VARIABLE;
///    }
/// Expands to this:
///    void DoNothing(){
///        int unique_154;
///    }
///
/// Note that all versions of MSVC++ up to at least version 7.1 
/// fail to properly compile macros that use __LINE__ in them
/// when the "program database for edit and continue" option
/// is enabled. The result is that __LINE__ gets converted to 
/// something like __LINE__(Var+37). VC7 and later has a __COUNTER__
/// built-in prepcrocessor define that seems like it would be 
/// usable to create a uniquely named variable, but the problem
/// with it is that you can't access that variable name later 
/// unless you know what it expanded to. Well that somewhat 
/// defeats the purpose of using a uniquely named variable, 
/// especially in the case of making assertion/trace macros.
///
#ifndef EA_UNIQUELY_NAMED_VARIABLE
    #define EA_UNIQUELY_NAMED_VARIABLE EA_PREPROCESSOR_JOIN(unique_,__LINE__)
#endif




///////////////////////////////////////////////////////////////////////////////
/// EA_CURRENT_FUNCTION
///
/// Provides a consistent way to get the current function name as a macro
/// like the __FILE__ and __LINE__ macros work. The C99 standard specifies
/// that __func__ be provided by the compiler, but most compilers don't yet
/// follow that convention. However, many compilers have an alternative.
///
/// We also define EA_CURRENT_FUNCTION_SUPPORTED for when it is not possible
/// to have EA_CURRENT_FUNCTION work as expected.
///
/// Defined inside a function because otherwise the macro might not be 
/// defined and code below might not compile. This happens with some 
/// compilers.
///
#if !defined(EA_CURRENT_FUNCTION_SUPPORTED) && !defined(EA_CURRENT_FUNCTION)
    inline void EA_CURRENT_FUNCTION_HOLDER()
    {   
        #define EA_CURRENT_FUNCTION_SUPPORTED

        #if defined(__CWCC__) && (__CWCC__ < 0x4200)                        // Older CodeWarrior has a bug with __FUNCTION__ or __PRETTY_FUNCTION__ in templated functions.
            #define EA_CURRENT_FUNCTION     0 // NULL

        #elif (EA_TRACE_CURRENT_FUNCTION_LEVEL == 0)
            #define EA_CURRENT_FUNCTION     0 // NULL

        #elif (EA_TRACE_CURRENT_FUNCTION_LEVEL == 1)
            #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)   // C99-compatible compilers.
                #define EA_CURRENT_FUNCTION __func__
            #elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)          // Borland
                #define EA_CURRENT_FUNCTION __FUNC__
            #else
                #define EA_CURRENT_FUNCTION __FUNCTION__
            #endif

        #else
            #if defined(__GNUC__) || (defined(__ICC) && (__ICC >= 600))     // GCC (and thus also SN) and some Intel variations.
                #define EA_CURRENT_FUNCTION __PRETTY_FUNCTION__
            #elif defined(__FUNCSIG__)                                      // VC++
                #define EA_CURRENT_FUNCTION __FUNCSIG__
            #elif defined(__CWCC__)                                         // CodeWarrior has bugs that prevent __PRETTY_FUNCTION__ from being used in all cases.
                #define EA_CURRENT_FUNCTION __FUNCTION__
            #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901) // C99-compatible compilers.
                #define EA_CURRENT_FUNCTION __func__
            #elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)          // Borland
                #define EA_CURRENT_FUNCTION __FUNC__
            #else                                                           // Fallback
                #define EA_CURRENT_FUNCTION __FUNCTION__
            #endif
        #endif
    }
#endif


///////////////////////////////////////////////////////////////////////////////
/// EA_CURRENT_FUNCTION_PARAMS
///
/// VC++ has a three versions of the function signature.  This is a version containing the return value and params. 
///  This can be used on that platform to determine the arguments and is unique for overloaded functions.
///
/// ex.  EA_CURRENT_FUNCTION               N1::Class::Function
/// ex.  EA_CURRENT_FUNCTION_PARAMS   void _cdecl N1::Class::Function(int var)
///
/// Defined inside a function because otherwise the macro might not be defined and code below might not compile.
///
#if !defined(EA_CURRENT_FUNCTION_PARAMS_SUPPORTED) && !defined(EA_CURRENT_FUNCTION_PARAMS)
    inline void EA_CURRENT_FUNCTION_PARAMS_HOLDER()
    { 
        #define EA_CURRENT_FUNCTION_PARAMS_SUPPORTED

        #if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600))  // GCC (and thus SN), Metrowerks, and some Intel variations.
            #define EA_CURRENT_FUNCTION_PARAMS __FUNCTION__
        #elif defined(__FUNCSIG__) // VC++
            #define EA_CURRENT_FUNCTION_PARAMS __FUNCSIG__
        #elif defined(__FUNCTION__)  // gcc equivalent of __FUNCSIG__, possibly others.
            #define EA_CURRENT_FUNCTION_PARAMS __FUNCTION__
        #else
            #undef EA_CURRENT_FUNCTION_PARAMS_SUPPORTED
            #define EA_CURRENT_FUNCTION_PARAMS EA_CURRENT_FUNCTION
        #endif
    }
#endif



///////////////////////////////////////////////////////////////////////////////
/// EA_SWALLOW_ARGS
///
/// This is useful for making variable count unused function arguments go 
/// away in non-debug builds.
///
/// Example usage:
///    #ifdef EA_DEBUG
///       inline void VPrint(char* pFormat, ...);
///    #else
///       #define VPrint EA_SWALLOW_ARGS
///    #endif
///
/// Example usage:
///    #ifdef EA_DEBUG
///       #define LOCAL_TRACE TRACE
///    #else
///       #define LOCAL_TRACE EA_SWALLOW_ARGS
///    #endif
///
/// We use the following implementation:
///    #define EA_SWALLOW_ARGS 1?0:
/// Other possibilites for implementation include:
///    #define EA_SWALLOW_ARGS 0&&
///    #define EA_SWALLOW_ARGS sizeof
/// However, there is some question as to whether the sizeof version would
/// generate legal C++ for the case of:
///    EA_SWALLOW_ARGS(GetSomeString());
///
#ifndef EA_SWALLOW_ARGS0
    #if defined(__GNUC__) || defined(__MWERKS__) || (defined(_MSC_VER) && (_MSC_VER >= 1400))
        #define EA_SWALLOW_ARGS0(...)
        #define EA_SWALLOW_ARGS1(a, ...)
        #define EA_SWALLOW_ARGS2(a, b, ...)
        #define EA_SWALLOW_ARGS3(a, b, c, ...)
        #define EA_SWALLOW_ARGS4(a, b, c, d, ...)
        #define EA_SWALLOW_ARGS5(a, b, c, d, e, ...)
    #elif defined(_MSC_VER)
        #define EA_SWALLOW_ARGS0 1?0:
        #define EA_SWALLOW_ARGS1 1?0:
        #define EA_SWALLOW_ARGS2 1?0:
        #define EA_SWALLOW_ARGS3 1?0:
        #define EA_SWALLOW_ARGS4 1?0:
        #define EA_SWALLOW_ARGS4 1?0:
    #else
        inline void EA_SWALLOW_ARGS0(...){}
        inline void EA_SWALLOW_ARGS1(...){}
        inline void EA_SWALLOW_ARGS2(...){}
        inline void EA_SWALLOW_ARGS3(...){}
        inline void EA_SWALLOW_ARGS4(...){}
        inline void EA_SWALLOW_ARGS5(...){}
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
/// EA_NOOP
///
/// Implements a no-op (no operation). This is useful for having intentionally
/// empty statements without the compiler warning about them. A typical usage
/// is shown below in the example whereby a no-op is used to implement an 
/// empty statement in a release build. The result is that the expression 
/// is not seen as a standalone ';' by the compiler, but it is an empty 
/// statement nonetheless.
///
/// Example usage:
///    #ifdef DEBUG_BUILD
///       #define Trace printf
///    #else
///       #define Trace EA_NOOP
///    #endif
///
#ifndef EA_NOOP
    #if defined(_MSC_VER) && (_MSC_VER >= 1300)
        #define EA_NOOP() __noop()
    #else
        #define EA_NOOP() ((void)0)
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EA_DEBUG_STRING_ENABLED / EA_DEBUG_STRING / EA_DEBUG_STRING_VAL
//
// Used to wrap debug string names. In a release build, the definition 
// goes away. These are present to avoid release build compiler warnings 
// and to make code simpler.
//
// Example usage of EA_DEBUG_STRING:
//    // pName will defined away in a release build and thus prevent 
//    // compiler warnings and possible string linking.
//    void SomeClass::SetName(const char* EA_DEBUG_STRING(pName))
//    {
//        #if EA_DEBUG_STRING_ENABLED
//            mpName = pName;
//        #endif
//    }
//
// Example usage of EA_DEBUG_STRING_VAL:
//    // "blah" is defined to NULL in a release build.
//    void SomeClass::SomeFunction()
//    {
//        SetName(EA_DEBUG_STRING_VAL("blah"));
//    }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EA_DEBUG_STRING_ENABLED
    #ifdef EA_DEBUG
        #define EA_DEBUG_STRING_ENABLED 1
    #else
        #define EA_DEBUG_STRING_ENABLED 0
    #endif
#endif

#ifndef EA_DEBUG_STRING
    #if EA_DEBUG_STRING_ENABLED
        #define EA_DEBUG_STRING(x)     x
        #define EA_DEBUG_STRING_VAL(x) x
    #else
        #define EA_DEBUG_STRING(x)
        #define EA_DEBUG_STRING_VAL(x) NULL
    #endif
#endif




///////////////////////////////////////////////////////////////////////////////
/// EA_DEBUG_BREAK_ENABLED
///
/// Defined as 0 or 1. Default is 1 if EA_DEBUG is defined.
/// This controls whether debugger breaks are enabled. Caller must explicitly
/// enable breaks in DebugRelease and Release builds to have asserts drop
/// into the debugger.  Breaks cause an exception to occur when a debugger is not 
/// present so only use this to debug DebugRelease and Release builds, 
/// and remove them from shipping code.
///
#if !defined(EA_DEBUG_BREAK_ENABLED)
    #if defined(EA_DEBUG)
        #define EA_DEBUG_BREAK_ENABLED 1
    #else
        #define EA_DEBUG_BREAK_ENABLED 0
    #endif
#endif

///////////////////////////////////////////////////////////////////////////////
/// EA_DEBUG_BREAK
///
/// This function causes an app to immediately stop under the debugger.
/// It is implemented as a macro in order to all stopping at the site 
/// of the call.
///
/// Example usage:
///    EA_DEBUG_BREAK();
///
/// The EA_DEBUG_BREAK function must be implemented on a per-platform basis. 
/// On a PC, you would normally define this to function to be the inline
/// assembly: "asm int 3", which tells the debugger to stop here immediately.
/// A very basic platform-independent implementation of EA_DEBUG_BREAK could 
/// be the following:
///   void EA_DEBUG_BREAK()
///   {
///      atoi(""); // Place a breakpoint here if you want to catch breaks.
///   }
/// 
/// The EA_DEBUG_BREAK default behaviour here can be disabled or changed by 
/// globally defining EA_DEBUG_BREAK_DEFINED and implementing an alternative
/// implementation of it. Our implementation here doesn't simply always have
/// it be defined externally because a major convenience of EA_DEBUG_BREAK
/// being inline is that it stops right on the troublesome line of code and
/// not in another function.
///
#if !defined(EA_DEBUG_BREAK)
    #if EA_DEBUG_BREAK_ENABLED
        #ifndef EA_DEBUG_BREAK_DEFINED
            #define EA_DEBUG_BREAK_DEFINED

            #if defined(EA_COMPILER_MSVC) && (_MSC_VER >= 1300)
                #define EA_DEBUG_BREAK() __debugbreak() // This is a compiler intrinsic which will map to appropriate inlined asm for the platform.
            #elif defined(EA_PROCESSOR_MIPS)    // Covers Playstation2-related platforms.
                #define EA_DEBUG_BREAK() asm("break")
            #elif defined(EA_PLATFORM_PSP2)
                #include <libdbg.h>
                #define EA_DEBUG_BREAK() SCE_BREAK()
            #elif defined(EA_PLATFORM_SONY) && defined(EA_PROCESSOR_X86_64)
                #define EA_DEBUG_BREAK() do { { __asm volatile ("int $0x41"); } } while(0)
            #elif defined(__SNC__)
                #define EA_DEBUG_BREAK() __builtin_snpause()
            #elif defined(EA_PLATFORM_PS3)
                #define EA_DEBUG_BREAK() asm volatile("tw 31,1,1")
            #elif defined(EA_PLATFORM_PS3_SPU)
                #include <sys/spu_stop_and_signal.h>
                #define EA_DEBUG_BREAK() spu_stop(STOP_BREAK)
            #elif defined(EA_PLATFORM_REVOLUTION)
                #include <revolution/os.h>
                #define EA_DEBUG_BREAK() OSHalt("EA_DEBUG_BREAK")
            #elif defined(EA_PLATFORM_GAMECUBE)
                #include <dolphin/os.h>
                #define EA_DEBUG_BREAK() OSHalt("EA_DEBUG_BREAK")
            #elif defined(EA_PLATFORM_CTR)
                #include <nn/dbg.h>
                #define EA_DEBUG_BREAK() ::nn::dbg::Break(::nn::dbg::BREAK_REASON_ASSERT)
            #elif defined(EA_PROCESSOR_ARM) && (defined(__APPLE__) || defined(EA_PLATFORM_PALM))
                #include <signal.h>
                #include <unistd.h>
                #define EA_DEBUG_BREAK() kill(getpid(), SIGINT)   // This lets you continue execution.
            //#elif defined(__APPLE__)
            //  #include <CoreServices/CoreServices.h>  // This works, but int3 works better on x86. Should we use this on Apple+ARM?
            //  #define EA_DEBUG_BREAK() Debugger() // Need to link with -framework CoreServices for this to be usable.
            #elif defined(EA_PROCESSOR_ARM) && defined(__APPLE__)
                #define EA_DEBUG_BREAK() asm("trap") // Apparently __builtin_trap() doesn't let you continue execution, so we don't use it.
            #elif defined(EA_PROCESSOR_ARM) && defined(EA_PLATFORM_AIRPLAY)
                #define EA_DEBUG_BREAK() *(int*)(0) = 0
            #elif defined(EA_PROCESSOR_ARM) && defined(__GNUC__)
                #define EA_DEBUG_BREAK() asm("BKPT 10")     // The 10 is arbitrary. It's just a unique id.
            #elif defined(EA_PROCESSOR_ARM) && defined(__ARMCC_VERSION)
                #define EA_DEBUG_BREAK() __breakpoint(10)
            #elif defined(EA_PROCESSOR_POWERPC) // Generic PowerPC. This triggers an exception by executing opcode 0x00000000.
                #define EA_DEBUG_BREAK() asm(".long 0")
            #elif (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)) && defined(EA_ASM_STYLE_INTEL)
                #define EA_DEBUG_BREAK() { __asm int 3 }
            #elif (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)) && (defined(EA_ASM_STYLE_ATT) || defined(__GNUC__))
                #define EA_DEBUG_BREAK() asm("int3") 
            #else
                void EA_DEBUG_BREAK(); // User must define this externally.
            #endif
        #else
            void EA_DEBUG_BREAK(); // User must define this externally.
        #endif
    #else
        #define EA_DEBUG_BREAK()
    #endif
#else
    #ifndef EA_DEBUG_BREAK_DEFINED
        #define EA_DEBUG_BREAK_DEFINED
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
/// EA_CRASH
///
/// Executes an invalid memory write, which should result in an exception 
/// on most platforms.
///
#if !defined(EA_CRASH) && !defined(EA_CRASH_DEFINED)
    #define EA_CRASH() *(int*)(0) = 0

    #define EA_CRASH_DEFINED
#endif



///////////////////////////////////////////////////////////////////////////////
/// EA_COMPILETIME_ASSERT / EA_COMPILETIME_ASSERT_NAMED
///
/// *** Note: These macros are deprecated, and you should instead use static_assert
/// ***       from EABase (which itself maps to the C++11 static_assert when possible).
///
/// EA_COMPILETIME_ASSERT is a macro for compile time assertion checks, useful for 
/// validating *constant* expressions. The advantage over using the ASSERT, 
/// VERIFY, etc. macros is that errors are caught at compile time instead 
/// of runtime.
///
/// This EA_COMPILETIME_ASSERT has a weakness in that when used at global scope (outside 
/// functions) there can be two such statements on the same line of a given 
/// file and some compilers might complain about this. If this is the case, 
/// you can either put the EA_COMPILETIME_ASSERT in a namespace or you can use EA_COMPILETIME_ASSERT_NAMED.
///
/// EA_COMPILETIME_ASSERT_NAMED requires that you use a name that's compatible 
/// with C++ symbol names. 
///
/// Example: 
///    EA_COMPILETIME_ASSERT(sizeof(int) == 4);
///    EA_COMPILETIME_ASSERT_NAMED(sizeof(int) == 4, UnexpectedSize);
///
#ifndef EA_COMPILETIME_ASSERT
    template <bool> struct EA_COMPILETIME_ASSERTION_FAILURE;
    template <>     struct EA_COMPILETIME_ASSERTION_FAILURE<true>{ enum { value = 1 }; }; // We create a specialization for true, but not for false.
    template<int x> struct EA_COMPILETIME_ASSERTION_TEST{};

    #if defined(EA_COMPILER_MSVC)
        #define EA_COMPILETIME_ASSERT(expression)          typedef EA_COMPILETIME_ASSERTION_TEST< sizeof(EA_COMPILETIME_ASSERTION_FAILURE< (bool)(expression) >)> EA_CT_ASSERT_FAILURE
    #elif defined(EA_COMPILER_INTEL)
        #define EA_COMPILETIME_ASSERT(expression)          typedef char EA_PREPROCESSOR_JOIN(EA_COMPILETIME_ASSERTION_FAILURE_, __LINE__) [EA_COMPILETIME_ASSERTION_FAILURE< (bool)(expression) >::value]
    #elif defined(EA_COMPILER_METROWERKS)
        #define EA_COMPILETIME_ASSERT(expression)          enum { EA_PREPROCESSOR_JOIN(EA_COMPILETIME_ASSERTION_FAILURE_, __LINE__) = sizeof(EA_COMPILETIME_ASSERTION_FAILURE< (bool)(expression) >) }
    #else // GCC, etc.
        #define EA_COMPILETIME_ASSERT(expression)          typedef EA_COMPILETIME_ASSERTION_TEST< sizeof(EA_COMPILETIME_ASSERTION_FAILURE< (bool)(expression) >)> EA_PREPROCESSOR_JOIN1(EA_COMPILETIME_ASSERTION_FAILURE_, __LINE__)
    #endif

    #define EA_COMPILETIME_ASSERT_NAMED(expression, name)  typedef EA_COMPILETIME_ASSERTION_TEST< sizeof(EA_COMPILETIME_ASSERTION_FAILURE< (bool)(expression) >)> name
#endif

// Extended name for compatibility with other libraries.
#ifndef EA_CT_ASSERT
    #define EA_CT_ASSERT EA_COMPILETIME_ASSERT  
#endif
#ifndef EA_CT_ASSERT_NAMED
    #define EA_CT_ASSERT_NAMED EA_COMPILETIME_ASSERT_NAMED  
#endif



///////////////////////////////////////////////////////////////////////////////
/// EAOUTPUTDEBUGSTRING_ENABLED
///
/// Defined as 0 or 1, with the default being 1 if EA_DEBUG is enabled.
/// This define enables EAOutputDebugString execution or makes it act
/// like a no-op. You can override this value in your local configuration
/// to make it act differently than the default.
///
#ifndef EAOUTPUTDEBUGSTRING_ENABLED
    #if defined(EA_DEBUG) || defined(EA_DEBUGRELEASE) || defined(_DEBUG)
        #define EAOUTPUTDEBUGSTRING_ENABLED 1
    #else
        #define EAOUTPUTDEBUGSTRING_ENABLED 0
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
/// EAIsDebuggerPresent
///
/// Returns true if the app appears to be running under a debugger.
///
bool EAIsDebuggerPresent();



///////////////////////////////////////////////////////////////////////////////
/// EAOutputDebugString / EAOutputDebugStringF
///
/// These functions provide portable debug trace functions that do nothing
/// more than write directly to the system-provided output mechanism.
/// This is useful for when you neeed to bypass the higher level system 
/// tracing/logging system and just write text.
///
/// These functions have an effect only if EAOUTPUTDEBUGSTRING_ENABLED is 
/// enabled, which is so by default in a debug build but not in a release
/// build.
/// 
/// Caution: These functions are something of a backdoor behind the trace and
/// log system and should only be used when you are sure you want or need to
/// bypass that system.
///
/// Function prototype:
///     void EAOutputDebugString(const char* text);
///     void EAOutputDebugStringF(const char* format, ...);
///
/// Example usage:
///     EAOutputDebugString("Hello world");
///     EAOutputDebugStringF("%s", "Hello world");
///
#ifndef EAOUTPUTDEBUGSTRING_DEFINED      // If somebody hasn't already defined this..
    #define EAOUTPUTDEBUGSTRING_DEFINED 1

    #if EAOUTPUTDEBUGSTRING_ENABLED
        #if defined(EA_PLATFORM_MOBILE) && (defined(__GNUC__) || defined(__ARMCC_VERSION))
            void    EAOutputDebugString(const char* pStr);
            void    EAOutputDebugStringF(const char* pFormat, ...);
        #elif defined(EA_PLATFORM_MICROSOFT) || defined(EA_PLATFORM_UNIX)
            void    EAOutputDebugString(const char* pStr);
            void    EAOutputDebugStringF(const char* pFormat, ...);
        #else
            #define EAOutputDebugString(str) printf("%s", (str))
            #define EAOutputDebugStringF     printf
        #endif
    #else
        #if defined(__GNUC__) || defined(__MWERKS__) || defined(__ARMCC_VERSION) || (defined(_MSC_VER) && (_MSC_VER >= 1400))
            #define EAOutputDebugString(str)
            #define EAOutputDebugStringF(fmt, ...)
        #else
            #define EAOutputDebugString(str)
            #define EAOutputDebugStringF 1?0: // Old trick: this causes the arguments to never get executed.
        #endif
    #endif
#endif


#endif // Header include guard










