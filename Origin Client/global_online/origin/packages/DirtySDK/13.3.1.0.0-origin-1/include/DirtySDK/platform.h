/*H********************************************************************************/
/*!
    \File platform.h

    \Description
        A platform wide header that performs environment identification and replaces
        some standard lib functions with "safe" alternatives (such an snprintf that
        always terminates the buffer).

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 01/25/2005 (gschaefer) First Version
*/
/********************************************************************************H*/

#ifndef _platform_h
#define _platform_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

// identify platform if not yet known
#if defined(DIRTYCODE_PC) || defined(DIRTYCODE_LINUX)
// do nothing
#elif defined (_XBOX)
#define DIRTYCODE_XENON         1
#elif defined (_WIN32) || defined (_WIN64)
    #if defined(WINAPI_FAMILY) && WINAPI_FAMILY == WINAPI_FAMILY_APP
    #define DIRTYCODE_WINRT     1
    #elif defined(WINAPI_FAMILY) && WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
    #define DIRTYCODE_WINPRT    1
    #else
    #define DIRTYCODE_PC        1
    #endif
#elif defined (__CELLOS_LV2__)
#define DIRTYCODE_PS3           1
#elif defined(__SNC__) && defined(__psp2__)
#define DIRTYCODE_PSP2          1
#elif defined(__APPLE__) && __APPLE__
 #include <TargetConditionals.h>  // for OSX, TARGET_OS_IPHONE, and TARGET_IPHONE_SIMULATOR
 #if defined(__IPHONE__) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR)
 #define DIRTYCODE_APPLEIOS      1
 #else
 #define DIRTYCODE_APPLEOSX      1
 #endif
#elif defined(__ANDROID__)
#define DIRTYCODE_ANDROID       1
#elif defined(__QNX__)
#define DIRTYCODE_PLAYBOOK      1
#elif defined(__linux__)
#define DIRTYCODE_LINUX         1
#endif

// define platform name
#if defined(DIRTYCODE_ANDROID)
#define DIRTYCODE_PLATNAME "Android"
#define DIRTYCODE_PLATNAME_SHORT "android"
#elif defined(DIRTYCODE_APPLEIOS)
#define DIRTYCODE_PLATNAME "AppleIOS"
#define DIRTYCODE_PLATNAME_SHORT "ios"
#elif defined(DIRTYCODE_APPLEOSX)
#define DIRTYCODE_PLATNAME "AppleOSX"
#define DIRTYCODE_PLATNAME_SHORT "osx"
#elif defined(DIRTYCODE_LINUX)
#define DIRTYCODE_PLATNAME "Linux"
#define DIRTYCODE_PLATNAME_SHORT "linux"
#elif defined(DIRTYCODE_PC)
#define DIRTYCODE_PLATNAME "Windows"
#define DIRTYCODE_PLATNAME_SHORT "pc"
#elif defined(DIRTYCODE_PLAYBOOK)
#define DIRTYCODE_PLATNAME "Playbook"
#define DIRTYCODE_PLATNAME_SHORT "playbook"
#elif defined(DIRTYCODE_PS3)
#define DIRTYCODE_PLATNAME "PlayStation3"
#define DIRTYCODE_PLATNAME_SHORT "ps3"
#elif defined(DIRTYCODE_PSP2)
#define DIRTYCODE_PLATNAME "PSP2"
#define DIRTYCODE_PLATNAME_SHORT "vita"
#elif defined(DIRTYCODE_WINRT)
#define DIRTYCODE_PLATNAME "Windows8"
#define DIRTYCODE_PLATNAME_SHORT "winrt"
#elif defined(DIRTYCODE_WINPRT)
#define DIRTYCODE_PLATNAME "WindowsPhone8"
#define DIRTYCODE_PLATNAME_SHORT "winprt"
#elif defined(DIRTYCODE_XENON)
#define DIRTYCODE_PLATNAME "Xbox360"
#define DIRTYCODE_PLATNAME_SHORT "xbl2"
#else
#error The platform was not predefined and could not be auto-determined!
#endif

// error out if someone tries to define this in their header file/build script
#ifdef DIRTYCODE_PLATFORM
#error Starting with DirtySDK version 8.18.0, DIRTYCODE_PLATFORM is deprecated.
#endif
// the ';' makes sure the use of DIRTYCODE_PLATFORM causes an error
#define DIRTYCODE_PLATFORM "Starting with DirtySDK version 8.18.0, DIRTYCODE_PLATFORM is deprecated.";

// define 32-bit or 64-bit pointers
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(_M_IA64) || defined(__ia64__) || defined(__arch64__) || defined(__mips64__) || defined(__64BIT__) 
 #define DIRTYCODE_64BITPTR (1)
#elif defined(__CC_ARM) && (__sizeof_ptr == 8)
 #define DIRTYCODE_64BITPTR (1)
#else
 #define DIRTYCODE_64BITPTR (0)
#endif

// we need va_list to be a universal type
#include <stdarg.h>

// c99 types for all platforms - check for defines for eabase compatibility
#if defined(DIRTYCODE_PC) || defined(DIRTYCODE_XENON) || defined(DIRTYCODE_WINRT) || defined(DIRTYCODE_WINPRT)
    #ifndef __int8_t_defined
        typedef signed char             int8_t;
        typedef signed short            int16_t;
        typedef signed int              int32_t;
        typedef signed long long        int64_t;
        #define __int8_t_defined
    #endif
    #ifndef __uint32_t_defined
        typedef unsigned char           uint8_t;
        typedef unsigned short          uint16_t;
        typedef unsigned int            uint32_t;
        typedef unsigned long long      uint64_t;
        #define __uint32_t_defined
    #endif
    #ifndef _intptr_t_defined
        #if defined(_WIN64)
            typedef signed long long    intptr_t;
        #else
            typedef signed int          intptr_t;
        #endif
        #define _intptr_t_defined
    #endif
    #ifndef _uintptr_t_defined
        #if defined(_WIN64)
            typedef unsigned long long  uintptr_t;
        #else
            typedef unsigned int        uintptr_t;
        #endif
        #define _uintptr_t_defined
    #endif

    // The following definitions of char8_t, char16_t and char32_t are needed because
    // we fail to use the ones from visual studio. They are in yvals.h but they end
    // up being ignored because nant build always add -D "_CHAR16T" to the compiler
    // command line.
    #ifndef CHAR8_T_DEFINED         // as used in EABase
        #define CHAR8_T_DEFINED
        typedef char                    char8_t;

        #ifndef _CHAR16_T           // as used in VC\INCLUDE\yvals.h
            #define _CHAR16_T
            typedef uint16_t            char16_t;
            typedef uint32_t            char32_t;
        #endif
    #endif
#else
    // other platforms supply stdint, so just include that
    #ifndef __uint32_t_defined
        #include <stdint.h>
    #endif
#endif

#include <time.h>

/*** Defines **********************************************************************/

// force our definition of TRUE/FALSE
#ifdef  TRUE
#undef  TRUE
#undef  FALSE
#endif

#define FALSE (0)
#define TRUE  (1)

// force our definitions of NULL
#ifdef  NULL
#undef  NULL
#endif

#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif

// map common debug code definitions to our debug code definition
#ifndef DIRTYCODE_DEBUG
 #if defined(EA_DEBUG)
  #define DIRTYCODE_DEBUG (1)
 #elif defined(RWDEBUG)
  #define DIRTYCODE_DEBUG (1)
 #else
  #define DIRTYCODE_DEBUG (0)
 #endif
#endif

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! time-to-string conversion type
typedef enum _TimeToSringConversionTypeE
{
    TIMETOSTRING_CONVERSION_ISO_8601,   //!< ISO 8601 standard:  yyyy-MM-ddTHH:mmZ where Z means 0 UTC offset, and no Z means local time zone
    TIMETOSTRING_CONVERSION_UNKNOWN     //!< unknown source format type; try to auto-determine if possible
} TimeToStringConversionTypeE;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// replacement time functions
uint32_t ds_timeinsecs(void);
int32_t ds_timezone(void);
struct tm *ds_localtime(struct tm *tm, uint32_t elap);
struct tm *ds_secstotime(struct tm *tm, uint32_t elap);
uint32_t ds_timetosecs(const struct tm *tm);
struct tm *ds_plattimetotime(struct tm *tm, void *pPlatTime);
char *ds_timetostr(const struct tm *tm, TimeToStringConversionTypeE eConvType, uint8_t bLocalTime, char *pStr, int32_t iBufSize);
uint32_t ds_strtotime(const char *str);
uint32_t ds_strtotime2(const char *pStr, TimeToStringConversionTypeE eConvType);

// replacement string functions
int32_t ds_vsnprintf(char *pBuffer, int32_t iLength, const char *pFormat, va_list Args);
int32_t ds_vsnzprintf(char *pBuffer, int32_t iLength, const char *pFormat, va_list Args);
int32_t ds_snzprintf(char *pBuffer, int32_t iLength, const char *pFormat, ...);
char *ds_strnzcpy(char *pDest, const char *pSource, int32_t iCount);
int32_t ds_strnzcat(char *pDst, const char *pSrc, int32_t iDstLen);
int32_t ds_stricmp(const char *pString1, const char *pString2);
int32_t ds_strnicmp(const char *pString1, const char *pString2, uint32_t uCount);
int32_t ds_strcmpwc(const char *pString1, const char *pStrWild);
int32_t ds_stricmpwc(const char *pString1, const char *pStrWild);
char *ds_stristr(const char *pHaystack, const char *pNeedle);
#if defined(_WIN32) || defined(_WIN64)
 #define ds_strtoll(_buf, _outp, _radix) _strtoi64(_buf, _outp, _radix)
#else
 #define ds_strtoll(_buf, _outp, _radix) strtoll(_buf, _outp, _radix)
#endif


// 'original' string functions
int32_t ds_strsubzcpy(char *pDst, int32_t iDstLen, const char *pSrc, int32_t iSrcLen);
int32_t ds_strsubzcat(char *pDst, int32_t iDstLen, const char *pSrc, int32_t iSrcLen);

#ifdef __cplusplus
}
#endif

#endif // _platform_h
