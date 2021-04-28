///////////////////////////////////////////////////////////////////////////////
// eastl_rw.h
//
// This is an EASTL configuration header that can be used in place of 
// EABase and which uses RenderWare 4.5 definitions. 
//
// To use this file, you can either copy and paste its contents right below 
// the EASTL_USER_CONFIG_HEADER section of EASTL's config.h or you can leave 
// config.h unmodified and instead #define EASTL_USER_CONFIG_HEADER be 
// config_rw.h and config.h will #include this file automatically.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_RW_H
#define EASTL_RW_H


// Include RenderWare core headers
#include <rw/core/base/ostypes.h>
#include <rw/core/corelite.h>


// Disable the use of eabase.h
#define EASTL_EABASE_DISABLED


// From here on, we replicate functionality from EABase.
#ifndef EA_WCHAR_T_NON_NATIVE
    #if defined(__ICL) || defined(__ICC)
        #if (EA_COMPILER_VERSION < 700)
            #define EA_WCHAR_T_NON_NATIVE 1
        #else
            #if (!defined(_WCHAR_T_DEFINED) && !defined(_WCHAR_T))
                #define EA_WCHAR_T_NON_NATIVE 1
            #endif
        #endif
    #elif defined(_MSC_VER) || defined(EA_COMPILER_BORLAND)
        #ifndef _NATIVE_WCHAR_T_DEFINED
            #define EA_WCHAR_T_NON_NATIVE 1
        #endif
    #elif defined(__MWERKS__)
        #if !__option(wchar_type)
            #define EA_WCHAR_T_NON_NATIVE 1
        #endif
    #endif
#endif


#ifndef EA_PLATFORM_WORD_SIZE
    #if defined (rwHOST_XBOX2) || defined (rwHOST_PS3) || defined (rwHOST_PS2)
        #define EA_PLATFORM_WORD_SIZE 8
    #else
        #define EA_PLATFORM_WORD_SIZE rwHOST_POINTERSIZE
    #endif
#endif


#if defined(__GNUC__) && (defined(__ICL) || defined(__ICC))
    #define EA_COMPILER_NO_EXCEPTIONS
#elif (defined(__GNUC__) || (defined(__ICL) || defined(__ICC)) || defined(__SN__)) && !defined(__EXCEPTIONS)
    #define EA_COMPILER_NO_EXCEPTIONS
#elif defined(__MWERKS__)
    #if !__option(exceptions)
        #define EA_COMPILER_NO_EXCEPTIONS
    #endif
#elif (defined(__BORLANDC__) || defined(_MSC_VER)) && !defined(_CPPUNWIND)
    #define EA_COMPILER_NO_UNWIND
#endif


#ifndef EASTL_DEBUG
    #if defined(RWDEBUG) || defined(_DEBUG)
        #define EASTL_DEBUG 1
    #else
        #define EASTL_DEBUG 0
    #endif
#endif



#endif // Header include guard







