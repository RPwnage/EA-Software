// Copyright (c) 2006 Electronic Arts Inc. All rights reserved.

#ifndef EAASSERT_EAASSERT_H
#define EAASSERT_EAASSERT_H

// Users can #define EA_ASSERT_HAVE_OWN_HEADER to their own header file and provide their
// own definition of EA_ASSERT. Make sure to include quotes in your 
// definition, i.e. #define EA_ASSERT_HAVE_OWN_HEADER "my_game/my_game_assert.h"
//
// At minimum, this header needs to provide the following:
//
//      EA_ASSERT(expr)
//      EA_ASSERT_MSG(expr, msg)
//      EA_ASSERT_FORMATTED(expr, fmt)
//      EA_FAIL()
//      EA_FAIL_MSG(msg)
//      EA_FAIL_FORMATTED(fmt)
//      EA_COMPILETIME_ASSERT(expr)     Note: EABase (2.0.21 and later) has static_assert which should now be used in place of this.
//
//  Where:
//  
//      expr    is an expression that evaluates to a boolean
//      msg     is a string (char*)
//      fmt     is a tuple wrapped in parentheses where the first message is 
//              a formatted string (char*) and the rest is printf style parameters
//
#ifdef EA_ASSERT_HAVE_OWN_HEADER
    #include EA_ASSERT_HAVE_OWN_HEADER

#else // The user did not supply their own assert definition, so we'll use ours instead


#include "EABase/eabase.h"
#include <stdarg.h>


// The default assert handling mechanism just breaks into the debugger on an assert failure. If you wish to overwrite this 
// functionality, simply implement the following callback:
//
//      bool EAAssertFailure(
//              const char* expr, 
//              const char* filename,
//              int line,
//              const char* function,
//              const char* msg,
//              va_list args)
//
// And then use:
//
//      EA::Assert::SetCallback(&yourCallback);
//

#if defined(EA_DLL) && defined(_MSC_VER)
    #ifndef EA_ASSERT_API
        #define EA_ASSERT_API __declspec(dllimport)
    #endif
#else
    #define EA_ASSERT_API
#endif

#ifdef __cplusplus
namespace EA { 
namespace Assert {

    typedef bool (*FailureCallback)(const char*, const char*, int, const char*, const char*, va_list);

    EA_ASSERT_API   FailureCallback GetFailureCallback();
    EA_ASSERT_API   void            SetFailureCallback(FailureCallback failureCallback);
}}
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
#if ! defined EA_DEBUG_BREAK && ! defined EA_DEBUG_BREAK_DEFINED
    #ifdef EA_PLATFORM_PS3_SPU
        #include <spu_printf.h>
        #define EA_DEBUG_BREAK() __asm__ volatile ("stop 0x3FFF") 
    #elif defined EA_PLATFORM_PS3
        #if defined(__SN_VER__)
            #define EA_DEBUG_BREAK() __builtin_snpause()
        #else
            #define EA_DEBUG_BREAK() __asm__ volatile ("tw 31,1,1")
        #endif
    #elif defined _MSC_VER
        #define EA_DEBUG_BREAK() __debugbreak()
    #elif defined EA_PLATFORM_GAMECUBE
        #include <dolphin/os.h>
        #define EA_DEBUG_BREAK() OSHalt("EA_DEBUG_BREAK")
    #elif defined EA_PLATFORM_REVOLUTION
        #include <revolution/os.h>
        #define EA_DEBUG_BREAK() asm("trap")
    #elif defined EA_PLATFORM_PLAYSTATION2
        #define EA_DEBUG_BREAK() __asm__ volatile (".word 0x4E00DEAD")
    #elif defined(EA_PROCESSOR_ARM) && defined(EA_PLATFORM_AIRPLAY)
        #define EA_DEBUG_BREAK() *(int*)(0) = 0
    #elif (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)) && defined(__GNUC__)
        #define EA_DEBUG_BREAK() asm("int3") 
    #elif defined(EA_PROCESSOR_ARM) && (defined(__APPLE__) || defined(EA_PLATFORM_PALM))
        #include <signal.h>
        #include <unistd.h>
        #define EA_DEBUG_BREAK() kill( getpid(), SIGINT )
    #elif defined(EA_PROCESSOR_ARM) && defined(__GNUC__)
        #define EA_DEBUG_BREAK() asm("BKPT 10")     // The 10 is arbitrary. It's just a unique id.
    #elif defined(EA_PROCESSOR_ARM) && defined(__ARMCC_VERSION)
        #define EA_DEBUG_BREAK() __breakpoint(10)
    #elif defined(EA_PROCESSOR_MIPS)    // Covers Playstation2-related platforms.
        #define EA_DEBUG_BREAK() asm("break")
    #elif defined(__SNC__)
        #define EATEST_DEBUG_BREAK() __builtin_snpause()
    #else
        #define EA_DEBUG_BREAK() *(int*)(0) = 0
    #endif

    #define EA_DEBUG_BREAK_DEFINED
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
#ifndef EA_CURRENT_FUNCTION
    #if defined __GNUC__ || (defined __ICC && __ICC >= 600)
        #define EA_CURRENT_FUNCTION __PRETTY_FUNCTION__
    #elif defined(__FUNCSIG__)
        #define EA_CURRENT_FUNCTION __FUNCSIG__
    #elif (defined __INTEL_COMPILER && __INTEL_COMPILER >= 600) || (defined __IBMCPP__ && __IBMCPP__ >= 500) || (defined __CWCC__ && __CWCC__ >= 0x4200)
        #define EA_CURRENT_FUNCTION __FUNCTION__
    #elif defined __BORLANDC__ && __BORLANDC__ >= 0x550
        #define EA_CURRENT_FUNCTION __FUNC__
    #elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901
        #define EA_CURRENT_FUNCTION __func__
    #elif defined __CWCC__ && __CWCC__ < 0x4200
        // Old CodeWarrior compiler has a bug with __FUNCTION__ or __PRETTY_FUNCTION__ in templated functions.  There are no plans to fix it.
        #define EA_CURRENT_FUNCTION 0
    #else
        #define EA_CURRENT_FUNCTION "(unknown function)"
    #endif
#endif



#ifdef __cplusplus
namespace EA { 
namespace Assert { 
namespace Detail 
{
    #ifdef EA_PLATFORM_PS3_SPU
        // Not needed on the SPU
    #else
        // I'm not happy about this, but it seems the least-ugly way to pass the formatted msg
        // to the user-supplied callback. Too bad it can't take a ... (ellipsis) instead of a va_list though. I would
        // love to see a less complicated design here, and am open for suggestions.
        struct Forwarder
        { 
            const char* mExpr;
            const char* mFunction;
            const char* mFilename;
            int mLine;
            Forwarder(const char* expr, const char* filename, int line,  const char* function) : mExpr(expr), mFunction(function), mFilename(filename), mLine(line) {}
            bool Call(const char* msg, ...) const
            {
                va_list args;
                va_start(args, msg);
                bool ret = (*GetFailureCallback())(mExpr, mFilename, mLine, mFunction, msg, args);
                va_end(args);
                return ret;
            }
        };
    #endif
}}}
#endif


///////////////////////////////////////////////////////////////////////////////
/// EA_COMPILETIME_ASSERT
///
/// EA_COMPILETIME_ASSERT is a macro for compile time assertion checks, useful for 
/// validating *constant* expressions. The advantage over using the ASSERT, 
/// VERIFY, etc. macros is that errors are caught at compile time instead 
/// of runtime.
///
/// This EA_COMPILETIME_ASSERT has a weakness in that when used at global scope (outside 
/// functions) there can be two such statements on the same line of a given 
/// file and some compilers might complain about this. 
///
/// Example: 
///    EA_COMPILETIME_ASSERT(sizeof(int) == 4);
///
#ifndef EA_COMPILETIME_ASSERT
    #define EAASSERT_TOKEN_PASTE(a,b) a ## b
    #define EAASSERT_CONCATENATE_HELPER(a,b) EAASSERT_TOKEN_PASTE(a,b)
    #define EA_COMPILETIME_ASSERT(expr) typedef char EAASSERT_CONCATENATE_HELPER(compileTimeAssert,__LINE__) [((expr) != 0) ? 1 : -1]
#endif

#ifndef EA_COMPILETIME_ASSERT
    #define EA_CT_ASSERT EA_COMPILETIME_ASSERT
#endif



#ifdef EA_DEBUG
    #ifndef EA_ASSERT_ENABLED
        #define EA_ASSERT_ENABLED
    #endif
#endif

#ifdef EA_ASSERT_ENABLED    
    #ifdef __cplusplus
        #ifndef EA_PLATFORM_PS3_SPU
            #ifndef EA_ASSERT
				#define EA_ASSERT(expr)                if (!(expr) && EA::Assert::Detail::Forwarder(#expr,     __FILE__, __LINE__, EA_CURRENT_FUNCTION).Call("none"))   EA_DEBUG_BREAK(); else ((void)0)
            #endif
            #ifndef EA_ASSERT_MSG
                #define EA_ASSERT_MSG(expr, msg)       if (!(expr) && EA::Assert::Detail::Forwarder(#expr,     __FILE__, __LINE__, EA_CURRENT_FUNCTION).Call(msg))      EA_DEBUG_BREAK(); else ((void)0)
            #endif
            #ifndef EA_ASSERT_FORMATTED
                #define EA_ASSERT_FORMATTED(expr, fmt) if (!(expr) && EA::Assert::Detail::Forwarder(#expr,     __FILE__, __LINE__, EA_CURRENT_FUNCTION).Call fmt)       EA_DEBUG_BREAK(); else ((void)0)
            #endif
            #ifndef EA_FAIL
                #define EA_FAIL()                      if (           EA::Assert::Detail::Forwarder("EA_FAIL", __FILE__, __LINE__, EA_CURRENT_FUNCTION).Call("none"))   EA_DEBUG_BREAK(); else ((void)0)
            #endif
            #ifndef EA_FAIL_MSG
                #define EA_FAIL_MSG(msg)               if (           EA::Assert::Detail::Forwarder("EA_FAIL", __FILE__, __LINE__, EA_CURRENT_FUNCTION).Call(msg))      EA_DEBUG_BREAK(); else ((void)0)
            #endif
            #ifndef EA_FAIL_FORMATTED
                #define EA_FAIL_FORMATTED(fmt)         if (           EA::Assert::Detail::Forwarder("EA_FAIL", __FILE__, __LINE__, EA_CURRENT_FUNCTION).Call fmt)       EA_DEBUG_BREAK(); else ((void)0)
            #endif
        #else
            #ifndef EA_ASSERT
                #define EA_ASSERT(expr)                if (!(expr)) { spu_printf("\nEA_ASSERT(%s) failed on SPU in %s(%d)\n", #expr, __FILE__, __LINE__); EA_DEBUG_BREAK(); } else ((void)0)
            #endif
            #ifndef EA_ASSERT_MSG
                #define EA_ASSERT_MSG(expr, msg)       if (!(expr)) { spu_printf("\nEA_ASSERT(%s) failed on SPU in %s(%d): %s\n", #expr, __FILE__, __LINE__, msg); EA_DEBUG_BREAK(); } else ((void)0)
            #endif
            #ifndef EA_ASSERT_FORMATTED
                #define EA_ASSERT_FORMATTED(expr, fmt) if (!(expr)) { spu_printf fmt; EA_DEBUG_BREAK(); } else ((void)0)
            #endif
            #ifndef EA_FAIL
                #define EA_FAIL()                      if(true) { spu_printf("\nEA_FAIL on SPU: %s(%d)\n", __FILE__, __LINE__); EA_DEBUG_BREAK(); } else ((void)0)
            #endif
            #ifndef EA_FAIL_MSG
                #define EA_FAIL_MSG(msg)               if(true) { spu_printf("\nEA_FAIL on SPU: %s(%d): %s\n", __FILE__, __LINE__, msg); EA_DEBUG_BREAK(); } else ((void)0)
            #endif
            #ifndef EA_FAIL_FORMATTED
				#define EA_FAIL_FORMATTED(fmt)         if(true) { spu_printf fmt; EA_DEBUG_BREAK(); } else ((void)0)
            #endif
        #endif
    #else
            #ifndef EA_ASSERT
                #define EA_ASSERT(expr)                if (!(expr)) { printf("\nEA_ASSERT(%s) failed in %s(%d)\n", #expr, __FILE__, __LINE__); EA_DEBUG_BREAK(); } else ((void)0)
            #endif
            #ifndef EA_ASSERT_MSG
                #define EA_ASSERT_MSG(expr, msg)       if (!(expr)) { printf("\nEA_ASSERT(%s) failed in %s(%d): %s\n", #expr, __FILE__, __LINE__, msg); EA_DEBUG_BREAK(); } else ((void)0)
            #endif
            #ifndef EA_ASSERT_FORMATTED
                #define EA_ASSERT_FORMATTED(expr, fmt) if (!(expr)) { printf fmt; EA_DEBUG_BREAK(); } else ((void)0)
            #endif
            #ifndef EA_FAIL
				#define EA_FAIL()                      if(true) { printf("\nEA_FAIL: %s(%d)\n", __FILE__, __LINE__); EA_DEBUG_BREAK(); } else ((void)0)
            #endif
            #ifndef EA_FAIL_MSG
                #define EA_FAIL_MSG(msg)               if(true) { printf("\nEA_FAIL: %s(%d): %s\n", __FILE__, __LINE__, msg); EA_DEBUG_BREAK(); } else ((void)0)
            #endif
            #ifndef EA_FAIL_FORMATTED
				#define EA_FAIL_FORMATTED(fmt)         if(true) { printf fmt; EA_DEBUG_BREAK(); } else ((void)0)
            #endif
    #endif
#else
    #ifndef EA_ASSERT
        #define EA_ASSERT(expr)                 ((void)0)
    #endif
    #ifndef EA_ASSERT_MSG
        #define EA_ASSERT_MSG(expr, msg)        ((void)0)
    #endif
    #ifndef EA_ASSERT_FORMATTED
        #define EA_ASSERT_FORMATTED(expr, fmt)  ((void)0)
    #endif
    #ifndef EA_FAIL
        #define EA_FAIL()                       ((void)0)
    #endif
    #ifndef EA_FAIL_MSG
        #define EA_FAIL_MSG(msg)                ((void)0)
    #endif
    #ifndef EA_FAIL_FORMATTED
        #define EA_FAIL_FORMATTED(fmt)          ((void)0)
    #endif
#endif

#endif

// We provide some synonyms for the message style macros, because we couldn't reach common ground
// and there was too much existing code using different versions. It's not pretty, but relatively harmless
#ifndef EA_ASSERT_M
    #define EA_ASSERT_M EA_ASSERT_MSG
#endif
#ifndef EA_ASSERT_MESSAGE 
    #define EA_ASSERT_MESSAGE EA_ASSERT_MSG
#endif
#ifndef EA_FAIL_M
    #define EA_FAIL_M EA_FAIL_MSG
#endif
#ifndef EA_FAIL_MESSAGE
    #define EA_FAIL_MESSAGE EA_FAIL_MSG
#endif

#endif
