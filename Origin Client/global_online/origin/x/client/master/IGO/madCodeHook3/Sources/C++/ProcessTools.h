// ***************************************************************
//  ProcessTools.h            version: 1.0.0  ·  date: 2010-01-10
//  -------------------------------------------------------------
//  functions that work against processes
//  -------------------------------------------------------------
//  Copyright (C) 1999 - 2010 www.madshi.net, All Rights Reserved
// ***************************************************************

#ifndef _PROCESSTOOLS_H
#define _PROCESSTOOLS_H

#ifndef _CCOLLECTION_H
  #include "CCollection.h"
#endif

#undef EXTERN
#ifdef _PROCESSTOOLS_C
  #define EXTERN
#else
  #define EXTERN extern
#endif

SYSTEMS_API bool WINAPI Init32bitKernelAPIs(HANDLE hProcess);
SYSTEMS_API PVOID WINAPI GetKernelAPI(ULONG index);
SYSTEMS_API HMODULE WINAPI GetRemoteModuleHandle64(HANDLE ProcessHandle, LPCWSTR DllName, LPWSTR DllFullPath);
SYSTEMS_API bool WINAPI IsProcessLargeAddressAware(HANDLE hProcess);
SYSTEMS_API PVOID WINAPI LargeAddressAwareApiTo2GB(HANDLE hProcess, PVOID largeApi);

EXTERN bool IsProcessDotNet(HANDLE hProcess);
EXTERN void UnpatchCreateRemoteThread(void);
EXTERN bool CheckDllName(LPCWSTR subStr, LPCWSTR str);
EXTERN BOOL GetThreadSecurityAttributes(SECURITY_ATTRIBUTES *pSecurityAttributes);
EXTERN void GetOtherThreadHandles(CCollection<HANDLE>& threads);
// EXTERN BOOL GetProcessSid(HANDLE hProcess, SID_AND_ATTRIBUTES **ppSidAndAttributes);

#endif