#ifndef StringConv_h__
#define StringConv_h__

#pragma once
#include <windows.h>

std::string  WideToMB(const std::wstring& str, UINT codePage = CP_ACP);
std::wstring MBToWide(const std::string& str,  UINT codePage = CP_ACP);

inline std::string  WideToUTF8(const std::wstring& str) { return WideToMB(str, CP_UTF8); }
inline std::wstring UTF8ToWide(const std::string& str)  { return MBToWide(str, CP_UTF8); }

std::string ANSIToUTF8(const std::string& str);
std::string UTF8ToANSI(const std::string& str);

#ifdef _UNICODE
#define TCHARToUTF8 WideToUTF8
#define UTF8ToTCHAR UTF8ToWide
#define TCHARToWide 
#define WideToTCHAR
#define TCHARToMB WideToMB 
#define MBToTCHAR MBToWide
#else
#define TCHARToUTF8 ANSIToUTF8
#define UTF8ToTCHAR UTF8ToANSI
#define TCHARToWide MBToWide
#define WideToTCHAR WideToMB
#define TCHARToMB
#define MBToTCHAR
#endif

#endif // StringConv_h__
