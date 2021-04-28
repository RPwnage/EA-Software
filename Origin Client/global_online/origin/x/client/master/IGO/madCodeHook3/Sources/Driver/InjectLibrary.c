// ***************************************************************
//  InjectLibrary.c           version: 1.0.2  ·  date: 2012-03-23
//  -------------------------------------------------------------
//  inject a dll into a specific 32bit or 64bit process
//  -------------------------------------------------------------
//  Copyright (C) 1999 - 2012 www.madshi.net, All Rights Reserved
// ***************************************************************

// 2012-03-23 1.0.2 added support for Windows 8
// 2011-03-27 1.0.1 (1) injection into wow64 processes improved (Vista+)
//                  (2) SetInjectionMethod API added
//                  (3) defaults to old injection method for all OSs now
//                  (4) added some injection tweaks to improve stability
// 2010-01-10 1.0.0 initial release

#include <ntddk.h>
#ifndef _WIN64
  #undef ExAllocatePool
  #undef ExFreePool      // NT4 doesn't support ExFreePoolWithTag
#endif

#include "ToolFuncs.h"

// ********************************************************************

// sizeof(PVOID32) is always 32bit, even in the 64bit driver
// sizeof(PVOID) is 32bit in a 32bit driver and 64bit in a 64bit driver
typedef VOID * POINTER_32 PVOID32;

// ********************************************************************

BOOLEAN UseNewInjectionMethod = FALSE;

BOOLEAN InitDone = FALSE;
HANDLE Ndh32 = NULL;     // ntdll handle
PVOID32 Ndep32 = NULL;   // ntdll entry point
PVOID32 Npvm32 = NULL;   // ntdll.NtProtectVirtualMemory
PVOID32 Lld32 = NULL;    // ntdll.LdrLoadDll
PVOID32 Nta32 = NULL;    // ntdll.NtTestAlert
#ifdef _WIN64
  PVOID Nta64 = NULL;    // ntdll.NtTestAlert
  PVOID Npvm64 = NULL;   // ntdll.NtProtectVirtualMemory
  PVOID Lld64 = NULL;    // ntdll.LdrLoadDll
#endif

BOOLEAN InitInjectLibrary(HANDLE ProcessId)
{
  if (InitDone)
    return TRUE;
  else
  {

    BOOLEAN result = FALSE;
    BOOLEAN attached = FALSE;
    PEPROCESS eprocess = NULL;
    KAPC_STATE apcstate;
    HANDLE ntdll;

    UseNewInjectionMethod = FALSE;

    // let's try to find ntdll in the dlls loaded by the system
    // this usually succeeds with all OSs until and including Windows 7
    ntdll = GetSystemModuleHandle("ntdll.dll");

    __try
    {

      if ((!ntdll) && (ProcessId))
      {
        // Windows 8 doesn't seem to have ntdll loaded in system, anymore
        // so we wait until the user application tries to inject
        // then we look for the ntdll which is loaded in the user process
        if (AttachToMemoryContext(ProcessId, &eprocess, &apcstate))
        {
          attached = TRUE;
          ntdll = FindNtdll();
        }
      }        
      if (ntdll)
      {
        #ifdef _WIN64
          Nta64  = GetProcAddress(ntdll, "NtTestAlert",            TRUE);
          Npvm64 = GetProcAddress(ntdll, "NtProtectVirtualMemory", TRUE);
          Lld64  = GetProcAddress(ntdll, "LdrLoadDll",             TRUE);
          result = ((Nta64) && (Npvm64) && (Lld64));
        #else
          Ndh32  = ntdll;
          Ndep32 = GetAddressOfEntryPoint(ntdll);
          Nta32  = GetProcAddress(ntdll, "NtTestAlert",            TRUE);
          Npvm32 = GetProcAddress(ntdll, "NtProtectVirtualMemory", TRUE);
          Lld32  = GetProcAddress(ntdll, "LdrLoadDll",             TRUE);
          result = ((Ndep32) && (Nta32) && (Npvm32) && (Lld32));
        #endif
      }
    }
    __finally
    {
      if (attached)
        DetachMemoryContext(eprocess, &apcstate);
    }

    if (result)
      InitDone = TRUE;

    return ((result) || (!ProcessId));
  }
}

#ifdef _WIN64
  void Set32bitNtdllInfo(HANDLE ModuleHandle, PVOID AddressOfEntryPoint, PVOID NtProtectVirtualMemory, PVOID LdrLoadDll, PVOID NtTestAlert)
  {
    if ((!Ndh32) && (ModuleHandle) && (AddressOfEntryPoint) && (NtProtectVirtualMemory) && (LdrLoadDll) && (NtTestAlert))
    {
      // we are being informed about the 32bit ntdll

      Ndh32 = ModuleHandle;
      Ndep32 = (PVOID32) AddressOfEntryPoint;
      Npvm32 = (PVOID32) NtProtectVirtualMemory;
      Lld32 = (PVOID32) LdrLoadDll;
      Nta32 = (PVOID32) NtTestAlert;
    }
  }
#endif

void SetInjectionMethod(BOOLEAN NewInjectionMethod)
// set the injection method used by the kernel mode driver
{
  UseNewInjectionMethod = NewInjectionMethod;
}

// ********************************************************************

#pragma pack(push, 1)

// sizeof(PRelJump32) is always 32bit
// sizeof(PRelJump64) is 32bit in a 32bit driver and 64bit in a 64bit driver
typedef struct _RelJump {
    UCHAR jmp;     // 0xe9
    ULONG target;  // relative target
} RelJump, *PRelJump64, * POINTER_32 PRelJump32;

// sizeof(PInjectLib32New) is always 32bit
typedef struct _InjectLib32New * POINTER_32 PInjectLib32New;

// this structure has the same size in both the 32bit and 64bit driver
typedef struct _InjectLib32New
{
  PVOID32          pEntryPoint;
  ULONG            oldEntryPoint;
  PVOID32          oldEntryPointFunc;
  PVOID32          NtProtectVirtualMemory;
  PVOID32          LdrLoadDll;
  UNICODE_STRING32 dllStr;
  WCHAR            dllBuf[260];
  UCHAR            movEax;
  PVOID32          buf;
  UCHAR            jmp;
  int              target;
} InjectLib32New;

const UCHAR CInjectLib32FuncNew[155] =
  {0x55, 0x8b, 0xec, 0x83, 0xc4, 0xf4, 0x53, 0x56, 0x57, 0x8b, 0xd8, 0x8b, 0x75, 0x0c, 0x83, 0xfe,
   0x01, 0x75, 0x49, 0x8b, 0x03, 0x89, 0x45, 0xfc, 0xc7, 0x45, 0xf8, 0x04, 0x00, 0x00, 0x00, 0x8d,
   0x45, 0xf4, 0x50, 0x6a, 0x40, 0x8d, 0x45, 0xf8, 0x50, 0x8d, 0x45, 0xfc, 0x50, 0x6a, 0xff, 0xff,
   0x53, 0x0c, 0x85, 0xc0, 0x75, 0x26, 0x8b, 0x53, 0x04, 0x8b, 0x03, 0x89, 0x10, 0x89, 0x45, 0xfc,
   0xc7, 0x45, 0xf8, 0x04, 0x00, 0x00, 0x00, 0x8d, 0x45, 0xf4, 0x50, 0x8b, 0x45, 0xf4, 0x50, 0x8d,
   0x45, 0xf8, 0x50, 0x8d, 0x45, 0xfc, 0x50, 0x6a, 0xff, 0xff, 0x53, 0x0c, 0x83, 0x7b, 0x04, 0x00,
   0x74, 0x14, 0x8b, 0x45, 0x10, 0x50, 0x56, 0x8b, 0x45, 0x08, 0x50, 0xff, 0x53, 0x08, 0x85, 0xc0,
   0x75, 0x04, 0x33, 0xc0, 0xeb, 0x02, 0xb0, 0x01, 0xf6, 0xd8, 0x1b, 0xff, 0x83, 0xfe, 0x01, 0x75,
   0x0f, 0x8d, 0x45, 0xf8, 0x50, 0x8d, 0x43, 0x14, 0x50, 0x6a, 0x00, 0x6a, 0x00, 0xff, 0x53, 0x10,
   0x8b, 0xc7, 0x5f, 0x5e, 0x5b, 0x8b, 0xe5, 0x5d, 0xc2, 0x0c, 0x00};

// the CInjectLib32FuncNew data is a compilation of the following Delphi code
// this user land code is copied to newly created 32bit processes to execute the dll injection
// unfortunately the x64 editions of XP and 2003 don't like this solution

// function CInjectLib32FuncNew(var injectRec: TInjectRec; d1, d2, reserved, reason, hInstance: dword) : bool;
// var p1     : pointer;
//     c1, c2 : dword;
// begin
//   with injectRec do begin
//     if reason = DLL_PROCESS_ATTACH then begin
//       p1 := pEntryPoint;
//       c1 := 4;
//       if NtProtectVirtualMemory($ffffffff, p1, c1, PAGE_EXECUTE_READWRITE, c2) = 0 then begin
//         pEntryPoint^ := oldEntryPoint;
//         p1 := pEntryPoint;
//         c1 := 4;
//         NtProtectVirtualMemory($ffffffff, p1, c1, c2, c2);
//       end;
//     end;
//     result := (oldEntryPoint = 0) or oldEntryPointFunc(hInstance, reason, reserved);
//     if reason = DLL_PROCESS_ATTACH then
//       LdrLoadDll(0, nil, @dll, c1);
//   end;
// end;

// sizeof(PInjectLib32Old) is always 32bit
typedef struct _InjectLib32Old * POINTER_32 PInjectLib32Old;

// this structure has the same size in both the 32bit and 64bit driver
typedef struct _InjectLib32Old
{
  UCHAR            movEax;        // 0xb8
  PInjectLib32Old  param;         // pointer to "self"
  UCHAR            movEcx;        // 0xb9
  PVOID32          proc;          // pointer to "self.injectFunc"
  USHORT           callEcx;       // 0xd1ff
  ULONG            magic;         // "mcil" / 0x6c69636d
  PInjectLib32Old  next;          // next dll (if any)
  PRelJump32       pOldApi;       // which code location in user land do we patch?
  RelJump          oldApi;        // patch backup buffer, contains overwritten code
  UNICODE_STRING32 dll;           // dll path/name
  WCHAR            dllBuf [260];  // string buffer for the dll path/name
  PVOID32          npvm;          // ntdll.NtProtectVirtualMemory
  PVOID32          lld;           // ntdll.LdrLoadDll
  UCHAR            injectFunc;    // will be filled with CInjectLibFunc32 (see below)
} InjectLib32Old;

#ifdef _WIN64

  const UCHAR CInjectLib32FuncOld[571] =
    {0x55, 0x8B, 0xEC, 0x83, 0xC4, 0xC0, 0x53, 0x56, 0x57, 0x8B, 0xD8, 0x64, 0x8B, 0x0D, 0x30, 0x00,
     0x00, 0x00, 0x89, 0x4D, 0xFC, 0x89, 0x6D, 0xF8, 0x8B, 0x45, 0xF8, 0x83, 0xC0, 0x04, 0x8B, 0x53,
     0x14, 0x89, 0x10, 0x8B, 0x45, 0xFC, 0x83, 0xC0, 0x0C, 0x8B, 0x00, 0x83, 0xC0, 0x14, 0x89, 0x45,
     0xEC, 0x8B, 0x45, 0xEC, 0x8B, 0xF0, 0x8D, 0x7D, 0xC0, 0xB9, 0x09, 0x00, 0x00, 0x00, 0xF3, 0xA5,
     0xE9, 0xE3, 0x01, 0x00, 0x00, 0x8B, 0xF0, 0x8D, 0x7D, 0xC0, 0xB9, 0x09, 0x00, 0x00, 0x00, 0xF3,
     0xA5, 0x83, 0x7D, 0xE0, 0x00, 0x0F, 0x84, 0xCD, 0x01, 0x00, 0x00, 0x33, 0xC9, 0xEB, 0x01, 0x41,
     0x8B, 0x45, 0xE0, 0x66, 0x83, 0x3C, 0x48, 0x00, 0x75, 0xF5, 0x8B, 0x45, 0xE0, 0x8D, 0x44, 0x48,
     0xEE, 0x81, 0x78, 0x04, 0x64, 0x00, 0x6C, 0x00, 0x0F, 0x85, 0xAA, 0x01, 0x00, 0x00, 0x81, 0x38,
     0x6E, 0x00, 0x74, 0x00, 0x0F, 0x85, 0x9E, 0x01, 0x00, 0x00, 0x8B, 0x45, 0xE0, 0x8D, 0x44, 0x48,
     0xF6, 0x81, 0x78, 0x04, 0x64, 0x00, 0x6C, 0x00, 0x0F, 0x85, 0x8A, 0x01, 0x00, 0x00, 0x81, 0x38,
     0x6C, 0x00, 0x2E, 0x00, 0x0F, 0x85, 0x7E, 0x01, 0x00, 0x00, 0x8B, 0x7D, 0xD0, 0x8D, 0x47, 0x3C,
     0x8B, 0x00, 0x03, 0xC7, 0x8B, 0x40, 0x78, 0x03, 0xC7, 0x89, 0xC6, 0x8B, 0x46, 0x18, 0x48, 0x85,
     0xC0, 0x0F, 0x8C, 0x6D, 0x01, 0x00, 0x00, 0x40, 0x89, 0x45, 0xE4, 0xC7, 0x45, 0xE8, 0x00, 0x00,
     0x00, 0x00, 0x8B, 0x46, 0x20, 0x03, 0xC7, 0x8B, 0x55, 0xE8, 0x8B, 0x0C, 0x90, 0x03, 0xCF, 0x85,
     0xC9, 0x0F, 0x84, 0x33, 0x01, 0x00, 0x00, 0x81, 0x79, 0x04, 0x6F, 0x61, 0x64, 0x44, 0x75, 0x19,
     0x81, 0x39, 0x4C, 0x64, 0x72, 0x4C, 0x75, 0x11, 0x8D, 0x41, 0x08, 0x8B, 0x00, 0x25, 0xFF, 0xFF,
     0xFF, 0x00, 0x3D, 0x6C, 0x6C, 0x00, 0x00, 0x74, 0x5A, 0x81, 0x79, 0x04, 0x6F, 0x74, 0x65, 0x63,
     0x0F, 0x85, 0x04, 0x01, 0x00, 0x00, 0x81, 0x39, 0x4E, 0x74, 0x50, 0x72, 0x0F, 0x85, 0xF8, 0x00,
     0x00, 0x00, 0x8D, 0x41, 0x08, 0x81, 0x78, 0x04, 0x74, 0x75, 0x61, 0x6C, 0x0F, 0x85, 0xE8, 0x00,
     0x00, 0x00, 0x81, 0x38, 0x74, 0x56, 0x69, 0x72, 0x0F, 0x85, 0xDC, 0x00, 0x00, 0x00, 0x8D, 0x41,
     0x10, 0x8B, 0x50, 0x04, 0x8B, 0x00, 0x81, 0xE2, 0xFF, 0xFF, 0xFF, 0x00, 0x81, 0xFA, 0x72, 0x79,
     0x00, 0x00, 0x0F, 0x85, 0xC2, 0x00, 0x00, 0x00, 0x3D, 0x4D, 0x65, 0x6D, 0x6F, 0x0F, 0x85, 0xB7,
     0x00, 0x00, 0x00, 0x8B, 0x46, 0x24, 0x03, 0xC7, 0x8B, 0x55, 0xE8, 0x0F, 0xB7, 0x04, 0x50, 0x89,
     0x45, 0xF8, 0x8B, 0x46, 0x1C, 0x03, 0xC7, 0x8B, 0x55, 0xF8, 0x8B, 0x04, 0x90, 0x89, 0x45, 0xF8,
     0x80, 0x39, 0x4C, 0x75, 0x0D, 0x8B, 0x45, 0xF8, 0x03, 0xC7, 0x89, 0x83, 0x31, 0x02, 0x00, 0x00,
     0xEB, 0x0B, 0x8B, 0x45, 0xF8, 0x03, 0xC7, 0x89, 0x83, 0x2D, 0x02, 0x00, 0x00, 0x83, 0xBB, 0x31,
     0x02, 0x00, 0x00, 0x00, 0x74, 0x74, 0x83, 0xBB, 0x2D, 0x02, 0x00, 0x00, 0x00, 0x74, 0x6B, 0x8B,
     0x43, 0x14, 0x89, 0x45, 0xF0, 0xC7, 0x45, 0xF8, 0x05, 0x00, 0x00, 0x00, 0x8D, 0x45, 0xF4, 0x50,
     0x6A, 0x40, 0x8D, 0x45, 0xF8, 0x50, 0x8D, 0x45, 0xF0, 0x50, 0x6A, 0xFF, 0xFF, 0x93, 0x2D, 0x02,
     0x00, 0x00, 0x8B, 0x43, 0x14, 0x8B, 0x53, 0x18, 0x89, 0x10, 0x8A, 0x53, 0x1C, 0x88, 0x50, 0x04,
     0xC7, 0x45, 0xF8, 0x05, 0x00, 0x00, 0x00, 0x8D, 0x45, 0xF4, 0x50, 0x8B, 0x45, 0xF4, 0x50, 0x8D,
     0x45, 0xF8, 0x50, 0x8D, 0x45, 0xF0, 0x50, 0x6A, 0xFF, 0xFF, 0x93, 0x2D, 0x02, 0x00, 0x00, 0x8D,
     0x45, 0xF8, 0x50, 0x8D, 0x43, 0x1D, 0x50, 0x6A, 0x00, 0x6A, 0x00, 0xFF, 0x93, 0x31, 0x02, 0x00,
     0x00, 0x8B, 0x5B, 0x10, 0x85, 0xDB, 0x75, 0xE7, 0xEB, 0x1A, 0xFF, 0x45, 0xE8, 0xFF, 0x4D, 0xE4,
     0x0F, 0x85, 0xAC, 0xFE, 0xFF, 0xFF, 0xEB, 0x0C, 0x8B, 0x45, 0xC0, 0x3B, 0x45, 0xEC, 0x0F, 0x85,
     0x11, 0xFE, 0xFF, 0xFF, 0x5F, 0x5E, 0x5B, 0x8B, 0xE5, 0x5D, 0xC3};

  // the CInjectLib32FuncOld data is a compilation of the following Delphi code
  // this user land code is copied to newly created wow64 processes to execute the dll injection

  // this code is used by default for all 64bit OSs

  // the 32bit injection code is rather complicated in the 64bit driver
  // the reason for that is that the 64bit driver doesn''t know some 32bit ntdll APIs
  // so the 32bit code has to find out the address of these APIs at runtime

  // procedure InjectLib32FuncOld(buf: PInjectLib32Old);
  // var c1, c2  : dword;
  //     peb     : dword;
  //     p1      : pointer;
  //     loopEnd : TPNtModuleInfo;
  //     mi      : TNtModuleInfo;
  //     len     : dword;
  //     ntdll   : dword;
  //     nh      : PImageNtHeaders;
  //     ed      : PImageExportDirectory;
  //     i1      : integer;
  //     api     : pchar;
  // begin
  //   asm
  //     mov ecx, fs:[$30]
  //     mov peb, ecx
  //     mov c1, ebp
  //   end;
  //   TPPointer(c1 + 4)^ := buf.pOldApi;
  // 
  //   // step 1: locate ntdll.dll
  //   loopEnd := pointer(dword(pointer(peb + $C)^) + $14);
  //   mi := loopEnd^;
  //   while mi.next <> loopEnd do begin
  //     mi := mi.next^;
  //     if mi.name <> nil then begin
  //       len := 0;
  //       while mi.name[len] <> #0 do
  //         inc(len);
  //       if (int64(pointer(@mi.name[len - 9])^) = $006c00640074006e) and         // ntdl
  //          (int64(pointer(@mi.name[len - 5])^) = $006c0064002e006c) then begin  // l.dl
  //         // found it!
  //         ntdll := mi.handle;
  // 
  //         // step 2: locate LdrLoadDll and NtProtectVirtualMemory
  //         nh := pointer(ntdll + dword(pointer(ntdll + $3C)^));
  //         dword(ed) := ntdll + nh^.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
  //         for i1 := 0 to ed^.NumberOfNames - 1 do begin
  //           api := pointer(ntdll + TPACardinal(ntdll + ed^.AddressOfNames)^[i1]);
  //           if (api <> nil) and
  //              ( ( (int64(pointer(@api[ 0])^)                       = $4464616f4c72644c) and                    // LdrLoadD
  //                  (dword(pointer(@api[ 8])^) and $00ffffff         = $00006c6c        )     ) or               // ll
  //                ( (int64(pointer(@api[ 0])^)                       = $6365746f7250744e) and                    // NtProtec
  //                  (int64(pointer(@api[ 8])^)                       = $6c61757472695674) and                    // tVirtual
  //                  (int64(pointer(@api[16])^) and $00ffffffffffffff = $000079726f6d654d)     )    ) then begin  // Memory
  //             c1 := TPAWord(ntdll + ed^.AddressOfNameOrdinals)^[i1];
  //             c1 := TPACardinal(ntdll + ed^.AddressOfFunctions)^[c1];
  //             if api[0] = 'L' then
  //               buf.lld := pointer(ntdll + c1)
  //             else
  //               buf.npvm := pointer(ntdll + c1);
  //             if (@buf.lld <> nil) and (@buf.npvm <> nil) then begin
  //               // found both APIs!
  // 
  //               // step 3: finally load the to-be-injected dll
  //               p1 := buf.pOldApi;
  //               c1 := 5;
  //               buf.npvm(dword(-1), p1, c1, PAGE_EXECUTE_READWRITE, c2);
  //               buf.pOldApi^ := buf.oldApi;
  //               c1 := 5;
  //               buf.npvm(dword(-1), p1, c1, c2, c2);
  //               repeat
  //                 buf.lld(0, nil, @buf.dll, c1);
  //                 buf := buf.next;
  //               until buf = nil;
  //               break;
  //             end; 
  //           end;
  //         end;
  //         break;
  //       end;
  //     end;
  //   end;
  // end;

  typedef struct _InjectLib64 *PInjectLib64;
  typedef struct _InjectLib64
  {
    USHORT         movRax;        // 0xb848
    PVOID          retAddr;       // patched API
    UCHAR          pushRax;       // 0x50
    UCHAR          pushRcx;       // 0x51
    UCHAR          pushRdx;       // 0x52
    USHORT         pushR8;        // 0x5041
    USHORT         pushR9;        // 0x5141
    ULONG          subRsp28;      // 0x28ec8348
    USHORT         movRcx;        // 0xb948
    PInjectLib64   param;         // pointer to "self"
    USHORT         movRdx;        // 0xba48
    PVOID          proc;          // pointer to "self.injectFunc"
    USHORT         jmpEdx;        // 0xe2ff
    ULONG          magic;         // "mcil" / 0x6c69636d
    PInjectLib64   next;          // next dll (if any)
    PRelJump64     pOldApi;       // which code location in user land do we patch?
    RelJump        oldApi;        // patch backup buffer, contains overwritten code
    UNICODE_STRING dll;           // dll path/name
    WCHAR          dllBuf [260];  // string buffer for the dll path/name
    PVOID          npvm;          // ntdll.NtProtectVirtualMemory
    PVOID          lld;           // ntdll.LdrLoadDll
    UCHAR          injectFunc;    // will be filled with CInjectLibFunc64 (see below)
  } InjectLib64;

  const UCHAR CInjectLib64Func[164] =
    {0x4C, 0x8B, 0xDC, 0x53, 0x48, 0x83, 0xEC, 0x30, 0x48, 0x8B, 0x41, 0x37, 0x48, 0x8B, 0xD9, 0x4D,
     0x8D, 0x43, 0x10, 0x49, 0x89, 0x43, 0x18, 0x49, 0x8D, 0x43, 0x08, 0x49, 0x8D, 0x53, 0x18, 0x41,
     0xB9, 0x40, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC9, 0xFF, 0x49, 0xC7, 0x43, 0x10, 0x05, 0x00, 0x00,
     0x00, 0x49, 0x89, 0x43, 0xE8, 0xFF, 0x93, 0x5C, 0x02, 0x00, 0x00, 0x8B, 0x43, 0x3F, 0x4C, 0x8B,
     0x5B, 0x37, 0x4C, 0x8D, 0x44, 0x24, 0x48, 0x48, 0x8D, 0x54, 0x24, 0x50, 0x41, 0x89, 0x03, 0x8A,
     0x43, 0x43, 0x48, 0x83, 0xC9, 0xFF, 0x41, 0x88, 0x43, 0x04, 0x44, 0x8B, 0x4C, 0x24, 0x40, 0x48,
     0x8D, 0x44, 0x24, 0x40, 0x48, 0x89, 0x44, 0x24, 0x20, 0x48, 0xC7, 0x44, 0x24, 0x48, 0x05, 0x00,
     0x00, 0x00, 0xFF, 0x93, 0x5C, 0x02, 0x00, 0x00, 0x4C, 0x8D, 0x43, 0x44, 0x4C, 0x8D, 0x4C, 0x24,
     0x48, 0x33, 0xD2, 0x33, 0xC9, 0xFF, 0x93, 0x64, 0x02, 0x00, 0x00, 0x48, 0x8B, 0x5B, 0x2F, 0x48,
     0x85, 0xDB, 0x75, 0xE4, 0x48, 0x83, 0xC4, 0x30, 0x5B, // 0xC3}
     0x48, 0x83, 0xC4, 0x28, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0xC3};

  // the CInjectLib64Func data is a compilation of the following C++ code
  // it got a manually created extended "tail", though (last 11 bytes)
  // this user land code is copied to newly created 64bit processes to execute the dll injection

  // static void InjectLib64Func(InjectLib64 *buf)
  // {
  //   LPVOID p1 = (LPVOID)buf->pOldApi;
  //   ULONG_PTR c1 = 5;
  //   ULONG c2;
  //
  //   buf->npvm((HANDLE) -1, &p1, &c1, PAGE_EXECUTE_READWRITE, &c2);
  //   *(buf->pOldApi) = buf->oldApi;
  //   c1 = 5;
  //   buf->npvm((HANDLE) -1, &p1, &c1, c2, &c2);
  //   do
  //   {
  //     buf->lld(0, NULL, &(buf->dll), (PHANDLE) &c1);
  //     buf = buf->next;
  //   } while (buf);
  // }

#else

  const UCHAR CInjectLib32FuncOld[133] =
    {0x55, 0x8B, 0xEC, 0x83, 0xC4, 0xF4, 0x53, 0x8B, 0xD8, 0x89, 0x6D, 0xFC, 0x8B, 0x45, 0xFC, 0x83,
     0xC0, 0x04, 0x8B, 0x53, 0x14, 0x89, 0x10, 0x8B, 0x43, 0x14, 0x89, 0x45, 0xF4, 0xC7, 0x45, 0xFC,
     0x05, 0x00, 0x00, 0x00, 0x8D, 0x45, 0xF8, 0x50, 0x6A, 0x40, 0x8D, 0x45, 0xFC, 0x50, 0x8D, 0x45,
     0xF4, 0x50, 0x6A, 0xFF, 0xFF, 0x93, 0x2D, 0x02, 0x00, 0x00, 0x8B, 0x43, 0x14, 0x8B, 0x53, 0x18,
     0x89, 0x10, 0x8A, 0x53, 0x1C, 0x88, 0x50, 0x04, 0xC7, 0x45, 0xFC, 0x05, 0x00, 0x00, 0x00, 0x8D,
     0x45, 0xF8, 0x50, 0x8B, 0x45, 0xF8, 0x50, 0x8D, 0x45, 0xFC, 0x50, 0x8D, 0x45, 0xF4, 0x50, 0x6A,
     0xFF, 0xFF, 0x93, 0x2D, 0x02, 0x00, 0x00, 0x8D, 0x45, 0xFC, 0x50, 0x8D, 0x43, 0x1D, 0x50, 0x6A,
     0x00, 0x6A, 0x00, 0xFF, 0x93, 0x31, 0x02, 0x00, 0x00, 0x8B, 0x5B, 0x10, 0x85, 0xDB, 0x75, 0xE7,
     0x5B, 0x8B, 0xE5, 0x5D, 0xC3};

  // the CInjectLib32FuncOld data is a compilation of the following Delphi code
  // this user land code is copied to newly created 32bit processes to execute the dll injection
  // this solution is used by default in all 32bit OSs

  // procedure InjectLib32FuncOld(buf: PInjectLib32Old);
  // var c1, c2 : dword;
  //     p1     : pointer;
  // begin
  //   asm
  //     mov c1, ebp
  //   end;
  //   TPPointer(c1 + 4)^ := buf.pOldApi;
  //   p1 := buf.pOldApi;
  //   c1 := 5;
  //   buf.npvm(dword(-1), p1, c1, PAGE_EXECUTE_READWRITE, c2);
  //   buf.pOldApi^ := buf.oldApi;
  //   c1 := 5;
  //   buf.npvm(dword(-1), p1, c1, c2, c2);
  //   repeat
  //     buf.lld(0, nil, @buf.dll, c1);
  //     buf := buf.next;
  //   until buf = nil;
  // end;

#endif

#pragma pack(pop)

// ********************************************************************

static BOOLEAN InjectLibraryNew32(HANDLE ProcessHandle, PWSTR LibraryFileName)
// inject a dll into a newly started 32bit process
// injection is done by providing/patching the ntdll.dll entry point
// unfortunately NT4 and Windows 2000 simply ignore an ntdll.dll entry point
// this solution is currently not used in any OSs by default
// the dll is injected directly after ntdll.dll is initialized
// it performs like calling LoadLibrary at the end of ntdll's init source code
{
  BOOLEAN result = FALSE;
  InjectLib32New il;
  PInjectLib32New buf = (PInjectLib32New) AllocMemEx(ProcessHandle, sizeof(InjectLib32New) + sizeof(CInjectLib32FuncNew), NULL);

  il.pEntryPoint = Ndep32;

  if ( (buf) &&
       (ReadProcessMemory(ProcessHandle, il.pEntryPoint, &il.oldEntryPoint, 4)) )
  {
    // we were able to allocate a buffer in the newly created process

    ULONG newEntryPoint;
    ULONG op;

    if (!il.oldEntryPoint)
      il.oldEntryPointFunc = NULL;
    else
      il.oldEntryPointFunc = (PVOID32) ((ULONG_PTR) Ndh32 + il.oldEntryPoint);
    il.NtProtectVirtualMemory = Npvm32;
    il.LdrLoadDll             = Lld32;
    il.dllStr.Length          = (USHORT) (wcslen(LibraryFileName) * 2);
    il.dllStr.MaximumLength   = il.dllStr.Length + 2;
    il.dllStr.Buffer          = (ULONG) ((ULONG_PTR) buf + sizeof(CInjectLib32FuncNew) + ((ULONG_PTR) (&il.dllBuf[0]) - (ULONG_PTR) (&il)));
    memcpy(il.dllBuf, LibraryFileName, il.dllStr.MaximumLength);
    il.movEax = 0xb8;  // mov eax, dw
    il.buf    = (PVOID32) ((ULONG_PTR) buf + sizeof(CInjectLib32FuncNew));
    il.jmp    = 0xe9;  // jmp dw
    il.target = - (int) sizeof(CInjectLib32FuncNew) - (int) sizeof(il);
    newEntryPoint = (ULONG) ((ULONG_PTR) buf + sizeof(CInjectLib32FuncNew) + sizeof(il) - 10 - (ULONG_PTR) Ndh32);
    if ( WriteProcessMemory(ProcessHandle, buf, (PVOID) CInjectLib32FuncNew, sizeof(CInjectLib32FuncNew)) &&
         WriteProcessMemory(ProcessHandle, (PVOID32) ((ULONG_PTR) buf + sizeof(CInjectLib32FuncNew)), &il, sizeof(il)) &&
         VirtualProtectEx(ProcessHandle, il.pEntryPoint, 4, PAGE_EXECUTE_READWRITE, &op) &&
         WriteProcessMemory(ProcessHandle, il.pEntryPoint, &newEntryPoint, 4) )
    {
      VirtualProtectEx(ProcessHandle, il.pEntryPoint, 4, op, &op);
      result = TRUE;
    }
  }

  return result;
}

BOOLEAN InjectLibraryOld32(HANDLE ProcessHandle, PWCHAR LibraryFileName)
// inject a dll into a newly started 32bit process
// this solution is used by default for all 32bit and 64bit OSs
// the dll is injected after all statically linked dlls are initialized
// it performs like calling LoadLibrary in the 1st source code line of the exe
{
  BOOLEAN result = FALSE;
  PInjectLib32Old buf = (PInjectLib32Old) AllocMemEx(ProcessHandle, sizeof(InjectLib32Old) + sizeof(CInjectLib32FuncOld), NULL);

  if (buf)
  {
    // we were able to allocate a buffer in the newly created process

    InjectLib32Old il;
    RelJump rj;

    il.movEax            = 0xb8;
    il.param             = buf;
    il.movEcx            = 0xb9;
    il.proc              = (PVOID32) &buf->injectFunc;
    il.callEcx           = 0xd1ff;
    il.magic             = 0x6c69636d;  // "mcil"
    il.next              = NULL;
    if (Nta32)
      il.pOldApi           = Nta32;
    else
      il.pOldApi           = (PVOID32) GetProcessEntryPoint(ProcessHandle);
    il.dll.Length        = (USHORT) wcslen(LibraryFileName) * 2;
    il.dll.MaximumLength = il.dll.Length + 2;
    il.dll.Buffer        = (ULONG) (&buf->dllBuf[0]);
    wcscpy(il.dllBuf, LibraryFileName);
    il.npvm              = Npvm32;
    il.lld               = Lld32;

    if ( (il.pOldApi) &&
         WriteProcessMemory(ProcessHandle, buf, &il, sizeof(InjectLib32Old)) &&
         WriteProcessMemory(ProcessHandle, &(buf->injectFunc), (PVOID) CInjectLib32FuncOld, sizeof(CInjectLib32FuncOld)) &&
         ReadProcessMemory (ProcessHandle, il.pOldApi, &rj, 5) &&
         WriteProcessMemory(ProcessHandle, &(buf->oldApi), &rj, 5) )
    {
      // we've successfully initialized the buffer

      if (rj.jmp == 0xe9)
      {
        // there's already a jump instruction
        // maybe another dll injection request is already installed?

        InjectLib32Old buf2;
        PInjectLib32Old bufAddr = (PVOID32) ((ULONG) il.pOldApi + rj.target + 5);
        if ( ReadProcessMemory(ProcessHandle, bufAddr, &buf2, sizeof(buf2)) &&
             (buf2.magic == 0x6c69636d) )
        {
          // oh yes, there is and it's compatible to us
          // so instead of installing another patch we just append our dll
          // injectioh request to the already existing one

          while (buf2.next)
          {
            // there's already more than one dll injection request
            // let's loop through them all to find the end of the queue
            bufAddr = buf2.next;
            if ( (!ReadProcessMemory(ProcessHandle, bufAddr, &buf2, sizeof(buf2))) ||
                 (buf2.magic != 0x6c69636d) )
            {
              // the queue is broken somewhere, so we abort
              bufAddr = NULL;
              break;
            }
          }

          if (bufAddr)
          {
            // we found the end of the queue
            buf2.next = buf;
            result = WriteProcessMemory(ProcessHandle, bufAddr, &buf2, sizeof(buf2));
          }
        }
      }

      if (!result)
      {
        // there's no compatible patch installed yet, so let's install one

        ULONG op;
        if ( VirtualProtectEx(ProcessHandle, il.pOldApi, 5, PAGE_EXECUTE_READWRITE, &op) )
        {
          // we successfully unprotected the to-be-patched code, so now we can patch it

          rj.jmp    = 0xe9;
          rj.target = (ULONG) buf - (ULONG) il.pOldApi - 5;
          WriteProcessMemory(ProcessHandle, il.pOldApi, &rj, 5);

          // now restore the original page protection

          VirtualProtectEx(ProcessHandle, il.pOldApi, 5, op, &op);

          result = TRUE;
        }
      }
    }
  }

  return result;
}

BOOLEAN InjectLibrary32(HANDLE ProcessHandle, PWCHAR LibraryFileName)
// inject a dll into a newly started 32bit process
// chooses between 2 different methods, depending on the OS
{
  if (UseNewInjectionMethod)
    return InjectLibraryNew32(ProcessHandle, LibraryFileName);
  else
    return InjectLibraryOld32(ProcessHandle, LibraryFileName);
}

// ********************************************************************

BOOLEAN InjectLibrary64(HANDLE ProcessHandle, PWCHAR LibraryFileName)
// inject a dll into a newly started 64bit process
// injection is done by patching "NtTestAlert"
// the dll is injected after all statically linked dlls are initialized
// it performs like calling LoadLibrary in the 1st source code line of the exe
{
  BOOLEAN result = FALSE;

  #ifdef _WIN64

    PInjectLib64 buf = AllocMemEx(ProcessHandle, sizeof(InjectLib64) + sizeof(CInjectLib64Func), Nta64);

    if (buf)
    {
      // we were able to allocate a buffer in the newly created process

      InjectLib64 il;
      RelJump rj;

      il.movRax            = 0xb848;
      il.retAddr           = Nta64;
      il.pushRax           = 0x50;
      il.pushRcx           = 0x51;
      il.pushRdx           = 0x52;
      il.pushR8            = 0x5041;
      il.pushR9            = 0x5141;
      il.subRsp28          = 0x28ec8348;
      il.movRcx            = 0xb948;
      il.param             = buf;
      il.movRdx            = 0xba48;
      il.proc              = &buf->injectFunc;
      il.jmpEdx            = 0xe2ff;
      il.magic             = 0x6c69636d;  // "mcil"
      il.next              = NULL;
      il.pOldApi           = il.retAddr;
      il.dll.Length        = (USHORT) wcslen(LibraryFileName) * 2;
      il.dll.MaximumLength = il.dll.Length + 2;
      il.dll.Buffer        = buf->dllBuf;
      wcscpy(il.dllBuf, LibraryFileName);
      il.npvm              = Npvm64;
      il.lld               = Lld64;

      if ( WriteProcessMemory(ProcessHandle, buf, &il, sizeof(InjectLib64)) &&
           WriteProcessMemory(ProcessHandle, &(buf->injectFunc), (PVOID) CInjectLib64Func, sizeof(CInjectLib64Func)) &&
           ReadProcessMemory (ProcessHandle, il.pOldApi, &rj, 5) &&
           WriteProcessMemory(ProcessHandle, &(buf->oldApi), &rj, 5) )
      {
        // we've successfully initialized the buffer

        if (rj.jmp == 0xe9)
        {
          // there's already a jump instruction
          // maybe another dll injection request is already installed?

          InjectLib64 buf2;
          PInjectLib64 bufAddr = (PVOID) ((ULONG_PTR) il.pOldApi + rj.target + 5);
          if ( ReadProcessMemory(ProcessHandle, bufAddr, &buf2, sizeof(buf2)) &&
               (buf2.magic == 0x6c69636d) )
          {
            // oh yes, there is and it's compatible to us
            // so instead of installing another patch we just append our dll
            // injectioh request to the already existing one

            while (buf2.next)
            {
              // there's already more than one dll injection request
              // let's loop through them all to find the end of the queue
              bufAddr = buf2.next;
              if ( (!ReadProcessMemory(ProcessHandle, bufAddr, &buf2, sizeof(buf2))) ||
                   (buf2.magic != 0x6c69636d) )
              {
                // the queue is broken somewhere, so we abort
                bufAddr = NULL;
                break;
              }
            }

            if (bufAddr)
            {
              // we found the end of the queue
              buf2.next = buf;
              result = WriteProcessMemory(ProcessHandle, bufAddr, &buf2, sizeof(buf2));
            }
          }
        }

        if (!result)
        {
          // there's no compatible patch installed yet, so let's install one

          ULONG op;
          if ( VirtualProtectEx(ProcessHandle, il.pOldApi, 5, PAGE_EXECUTE_READWRITE, &op) )
          {
            // we successfully unprotected the to-be-patched code, so now we can patch it

            rj.jmp    = 0xe9;
            rj.target = (ULONG) ((ULONG_PTR) buf - (ULONG_PTR) il.pOldApi - 5);
            WriteProcessMemory(ProcessHandle, il.pOldApi, &rj, 5);

            // now restore the original page protection

            VirtualProtectEx(ProcessHandle, il.pOldApi, 5, op, &op);

            result = TRUE;
          }
        }
      }
    }

  #endif

  return result;
}

// ********************************************************************

BOOLEAN InjectLibrary(HANDLE ProcessHandle, PWCHAR LibraryFileName)
// inject a dll into a newly started 32bit or 64bit process
{
  if (Is64bitProcess(ProcessHandle))
    return InjectLibrary64(ProcessHandle, LibraryFileName);
  else
    return InjectLibrary32(ProcessHandle, LibraryFileName);
}

// ********************************************************************
