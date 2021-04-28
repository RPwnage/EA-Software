#ifndef __ORIGIN_H__
#define __ORIGIN_H__

/////////////////////////////////////////////////////////////////////////////////////////

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the ORIGINSDK_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// ORIGIN_SDK_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#if !defined(ORIGIN_MAC) && !defined(ORIGIN_PC)
#ifdef __APPLE__
#define ORIGIN_MAC
#else
#define ORIGIN_PC
#endif
#endif

#ifdef ORIGIN_PC
#define ORIGIN_DEPRECATED(a) __declspec(deprecated(a))
#else
#define ORIGIN_DEPRECATED(a)
#endif

#if defined ORIGIN_MAC && defined _USRDLL
#undef _USRDLL
#endif

#ifdef _USRDLL

#ifdef ORIGIN_SDK_EXPORTS
#define ORIGIN_SDK_API __declspec(dllexport)
#else
#define ORIGIN_SDK_API __declspec(dllimport)
#endif

#else

/// \ingroup sdk
/// \brief An empty definition that allows you to import functions that are within the same DLL.
/// 
#define ORIGIN_SDK_API


#endif


#endif //__ORIGIN_H__
