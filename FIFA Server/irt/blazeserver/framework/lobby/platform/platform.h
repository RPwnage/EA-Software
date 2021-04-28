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

// define the different platforms
#ifndef DIRTYCODE_UNDEF
#define DIRTYCODE_UNDEF         1   //!< Compiled on undefined platform
#define DIRTYCODE_PC            3   //!< Compiled for Windows
#define DIRTYCODE_PS2EE         4   //!< Compiled for Sony PS2 EE (EENet)
#define DIRTYCODE_PS2           5   //!< Compiled for Sony PS2 EE (Inet)
#define DIRTYCODE_NGC           6   //!< Compiled for Nintendo GameCube
#define DIRTYCODE_XBOX          7   //!< Compiled for Microsoft XBOX
#define DIRTYCODE_IOP           8   //!< Compiled for Sony PS2 IOP
#define DIRTYCODE_PSP           9   //!< Compiled for Sony PSP
#define DIRTYCODE_XENON         10  //!< Compiled for Microsoft Xenon
#define DIRTYCODE_LINUX         11  //!< Compiled for Linux
#define DIRTYCODE_PS3           12  //!< Compiled for Sony PS3 PPU
#define DIRTYCODE_DS            13  //!< Compiled for Nintendo DS
#define DIRTYCODE_REVOLUTION    14  //!< Compiled for Nintendo Revolution
#endif

// identify platform if not yet known
#ifdef DIRTYCODE_PLATFORM
#elif defined (_WIN32)
#define DIRTYCODE_PLATFORM DIRTYCODE_PC
#elif defined (_XBOX)
#define DIRTYCODE_PLATFORM DIRTYCODE_XBOX
#elif defined (_XENON)
#define DIRTYCODE_PLATFORM DIRTYCODE_XENON
#elif defined(SN_TARGET_NGC)
#define DIRTYCODE_PLATFORM DIRTYCODE_NGC
#elif defined (__CELLOS_LV2__)
#define DIRTYCODE_PLATFORM DIRTYCODE_PS3
#elif defined (__R5900__)
#define DIRTYCODE_PLATFORM DIRTYCODE_PS2
#elif defined (__R4000__)
#define DIRTYCODE_PLATFORM DIRTYCODE_PSP
#elif defined (__R3000__)
#define DIRTYCODE_PLATFORM DIRTYCODE_IOP
#elif defined(__MWERKS__) && defined(__arm)
#define DIRTYCODE_PLATFORM DIRTYCODE_DS
#elif defined(__MWERKS__) && defined(RVL_SDK)
#define DIRTYCODE_PLATFORM DIRTYCODE_REVOLUTION
#elif defined(__linux__)
#define DIRTYCODE_PLATFORM DIRTYCODE_LINUX
#else
#define DIRTYCODE_PLATFORM DIRTYCODE_UNDEF
#endif

// we need va_list to be a universal type
#include <stdarg.h>

// c99 types for all platforms - check for defines for eabase compatibility
#if ((DIRTYCODE_PLATFORM == DIRTYCODE_IOP) || (DIRTYCODE_PLATFORM == DIRTYCODE_PC) || \
    (DIRTYCODE_PLATFORM == DIRTYCODE_XBOX) || (DIRTYCODE_PLATFORM == DIRTYCODE_XENON) || \
    (DIRTYCODE_PLATFORM == DIRTYCODE_NGC))
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
    #ifdef _INTPTR_T_DEFINED
        #define _intptr_t_defined
    #endif
    #ifndef _intptr_t_defined
        typedef signed int              intptr_t;
        #define _intptr_t_defined
        #define _INTPTR_T_DEFINED
    #endif
    
    #ifdef _UINTPTR_T_DEFINED
        #define _uintptr_t_defined
    #endif
    #ifndef _uintptr_t_defined
        typedef unsigned int            uintptr_t;
        #define _uintptr_t_defined
        #define _UINTPTR_T_DEFINED
    #endif
#else
    // other platforms supply stdint, so just include that
    #ifndef __uint32_t_defined
        #include <stdint.h>
    #endif
#endif

/* since the IOP toolchain does not include time support, we need to
   define it here for IOP and include <time.h> for non-IOP platforms,
   to make the time support transparent for all platforms */
#if DIRTYCODE_PLATFORM == DIRTYCODE_IOP
struct tm
{
    int32_t tm_sec;
    int32_t tm_min;
    int32_t tm_hour;
    int32_t tm_mday;
    int32_t tm_mon;
    int32_t tm_year;
    int32_t tm_wday;
    int32_t tm_yday;
    int32_t tm_isdst;
};
typedef int32_t time_t;
#else
#include <time.h>
#endif

/*** Defines **********************************************************************/

// force our definition of TRUE/FALSE
#ifdef  TRUE
#undef  TRUE
#undef  FALSE
#endif

#define FALSE (0)
#define TRUE  (1)

// force our definitions of nullptr
#ifdef  nullptr
#undef  nullptr
#endif

#ifdef __cplusplus
#define nullptr 0
#else
#define nullptr ((void *)0)
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

/*! macros to redefine common function names to their dirtysock functional
    equivalents.  these macros do not compile in by default; if they are
    desired, DS_PLATFORM must be defined in the project/makefile. */
#ifdef DS_PLATFORM

//! platform debug print function
#if DIRTYCODE_DEBUG
 #define logprintf(_x) _Platform_pLogPrintf _x
#else
 #define logprintf(_x) { }
#endif

// make sure that replacement functions use our versions
#define vsnzprintf ds_vsnzprintf
#define snzprintf ds_snzprintf
#define strnzcpy ds_strnzcpy
#define strcasecmp ds_stricmp
#define strncasecmp ds_strnicmp
#ifndef stricmp
#define stricmp ds_stricmp
#endif
#ifndef strnicmp
#define strnicmp ds_strnicmp
#endif
#define stristr ds_stristr

// custom functions, named similarly to standard string functions
#define strsubzcpy ds_strsubzcpy
#define strnzcat ds_strnzcat
#define strsubzcat ds_strsubzcat

// these are special time related functions -- new names avoid issues
#define timezone ds_timezone
#define timetosecs ds_timetosecs
#define secstotime ds_secstotime
#define timeinsecs ds_timeinsecs
#define strtotime ds_strtotime
#endif // DS_PLATFORM

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

//! this variable is not implemented in platform, it must be implemented in client code
#if DIRTYCODE_DEBUG
extern int (*_Platform_pLogPrintf)(const char *pFmt, ...);
#endif

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// replacement time functions
uint32_t ds_timeinsecs(void);
uint32_t ds_timezone(void);
struct tm *ds_secstotime(struct tm *tm, uint32_t elap);
uint32_t ds_timetosecs(const struct tm *tm);
uint32_t ds_strtotime(const char *str);

// replacement string functions
int32_t ds_vsnzprintf(char *pBuffer, int32_t iLength, const char *pFormat, va_list Args);
int32_t ds_snzprintf(char *pBuffer, int32_t iLength, const char *pFormat, ...);
char *ds_strnzcpy(char *pDest, const char *pSource, int32_t iCount);
int32_t ds_strnzcat(char *pDst, const char *pSrc, int32_t iDstLen);
int32_t ds_stricmp(const char *pString1, const char *pString2);
int32_t ds_strnicmp(const char *pString1, const char *pString2, uint32_t uCount);
char *ds_stristr(const char *pHaystack, const char *pNeedle);

// 'original' string functions
int32_t ds_strsubzcpy(char *pDst, int32_t iDstLen, const char *pSrc, int32_t iSrcLen);
int32_t ds_strsubzcat(char *pDst, int32_t iDstLen, const char *pSrc, int32_t iSrcLen);

#ifdef __cplusplus
}
#endif

#endif // _platform_h
