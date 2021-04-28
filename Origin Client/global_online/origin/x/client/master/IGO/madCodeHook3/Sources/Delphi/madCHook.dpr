// ***************************************************************
//  madCHook.dpr              version: 3.0.1  ·  date: 2010-08-02
//  -------------------------------------------------------------
//  API hooking, code hooking standard library
//  -------------------------------------------------------------
//  Copyright (C) 1999 - 2010 www.madshi.net, All Rights Reserved
// ***************************************************************

// 2010-08-02 3.0.1 (1) added IsInjectionDriverInstalled/Running
//                  (2) added SetInjectionMethod API
//                  (3) added UninjectAllLibrariesA/W API
// 2010-01-07 3.0.0 added support for madCodeHook 3.0
// 2009-07-21 2.1g  RestoreCode added
// 2007-08-13 2.1f  SetMadCHookOption added
// 2006-07-11 2.1e  ThreadHandleToId added
// 2005-11-28 2.1d  (1) auto unhooking bug fixed
//                  (2) helper exports for C++ static lib added
// 2005-06-13 2.1c  (1) new functions (Un)InstallMadCHook added
//                  (2) AutoUnhook added (for internal use)
// 2004-04-27 2.1b  IsHookInUse added
// 2004-03-21 2.1a  we detect now whether we're madCHook.dll or the static lib
// 2004-03-07 2.1   CreateIpcQueueEx added with additional parameters
// 2003-10-05 2.0a  changed initialization/finalization logic to fix memory leak
// 2003-08-10 2.0   (1) HookCode parameters changed -> only one flags parameter
//                  (2) (Un)InjectLibrary: user/session/system wide injection!
//                  (3) InjectLibrary2 replaced by InjectLibrary (auto detect)
//                  (4) static lib for Microsoft C++ added
//                  (5) CreateIpcQueue + SendIpcMessage + DestroyIpcQueue added
//                  (6) AmSystemProcess + AmUsingInputDesktop added
//                  (7) GetCurrentSessionId + GetInputSessionId added
//                  (8) GetCallingModule function added
//                  (9) ProcessHandleToFileName added
//                  (a) Create/OpenGlobalMutex + Event + FileMapping added
//                  (b) WideToAnsi + AnsiToWide functions added
//                  (c) RenewHook function added
//                  (d) madCodeHook.dll -> madCHook.dll (8.3 dos name logic)
//                  (e) UnhookAPI added (= UnhookCode, added just for the look)
// 2002-10-17 1.3f  InjectLibrary2(W) was not stdcall (dumb me)
// 2002-10-03 1.3e  (1) InjectLibraryW added
//                  (2) InjectLibrary2(W) added for use in CreateProcess(W) hooks
// 2002-09-22 1.3d  CreateProcessExW added
// 2002-03-24 1.3c  CollectHooks/FlushHooks speed up mixture initialization
// 2002-02-24 1.3b  LPSTARTUPINFO -> LPSTARTUPINFOA
// 2002-01-21 1.3a  ProcessHandleToID exported
// 2001-07-08 1.3   new functions (1) AllocMemEx & FreeMemEx
//                                (2) CopyFunction
//                                (3) CreateRemoteThread and
//                                (4) InjectLibrary added
// 2001-04-20 1.2a  you can now force HookCode/API to use the mixture mode
// 2001-04-16 1.2   new function CreateProcessEx -> dll injecting

library madCHook;

// ***************************************************************

uses
  Windows,
  Messages,
  madTypes in 'madTypes.pas',
  madStrings in 'madStrings.pas',
  madDisAsm in 'madDisAsm.pas',
  madRemote in 'madRemote.pas',
  {$ifndef madRmt}
    madCodeHook in 'madCodeHook.pas',
  {$endif}
  madTools in 'madTools.pas';

{$IMAGEBASE $44400000}

{$I mad.inc}

{$R madCHook.res}

// ***************************************************************

function CopyFunction(func                 : pointer;
                      processHandle        : dword     = 0;
                      acceptUnknownTargets : boolean   = false;
                      buffer               : TPPointer = nil  ) : pointer; stdcall;
begin
  result := madRemote.CopyFunction(func, processHandle, acceptUnknownTargets, buffer, nil);
end;

// ***************************************************************

{$ifndef madRmt}
procedure AutoUnhook(module: dword); stdcall;
begin
  if module = dword(-1) then
    module := GetCallingModule;
  madCodeHook.AutoUnhook(module);
end;
{$endif}

// ***************************************************************

type
  TDllMainProc = function (module, reason, reserved: dword) : bool; stdcall;

var
  CurrentModule : dword = 0;

procedure StaticLibHelper_Init(dllMain: TDllMainProc); stdcall;
var mbi : TMemoryBasicInformation;
begin
  if (CurrentModule = 0) and
     (VirtualQuery(@StaticLibHelper_Init, mbi, sizeof(mbi)) = sizeof(mbi)) and
     (mbi.State = MEM_COMMIT) and (mbi.AllocationBase <> nil) then begin
    CurrentModule := dword(mbi.AllocationBase);
    dllMain(CurrentModule, DLL_PROCESS_ATTACH, 0);
  end;
end;

procedure StaticLibHelper_Final(dllMain: TDllMainProc); stdcall;
var arrCh : array [0..MAX_PATH] of char;
begin
  GetModuleFileName(0, arrCh, MAX_PATH);
  if CurrentModule <> 0 then begin
    dllMain(CurrentModule, DLL_PROCESS_DETACH, 0);
    CurrentModule := 0;
  end;
end;

// ***************************************************************

{$ifdef evaluation}

var OriginalAPIs : TDACardinal = nil;
    NcMutex      : dword = 0;

function CallRealApi(caller: pointer) : pointer;
var i1  : integer;
    map : dword;
    buf : TPACardinal;
begin
  result := nil;
  if OriginalAPIs = nil then begin
    map := OpenGlobalFileMapping('mchEvaluation', false);
    if map <> 0 then begin
      buf := MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
      if buf <> nil then
        if buf[0] = dword(pointer(dword(GetProcAddress(HInstance, pointer(100))) + 1)^) then begin
          SetLength(OriginalAPIs, buf[1]);
          Move(buf[0], OriginalAPIs[0], buf[1] * 4);
          UnmapViewOfFile(buf);
        end else begin
          if (NcMutex = 0) and (not AmSystemProcess) and AmUsingInputDesktop then begin
            NcMutex := CreateGlobalMutex('mchNcWarning');
            if GetLastError <> ERROR_ALREADY_EXISTS then
              MessageBox(0, 'This is the evaluation version of madCodeHook.' + #$D#$A +
                            'The version of the "mchEvaluation.exe" and "madCHook.dll"' + #$D#$A +
                            'files seems to differ.',
                            'madCodeHook information...', MB_ICONINFORMATION);
          end;
        end;
      CloseHandle(map);
    end;
  end;
  if (OriginalAPIs = nil) and (NcMutex = 0) and (not AmSystemProcess) and AmUsingInputDesktop then begin
    NcMutex := CreateGlobalMutex('mchNcWarning');
    if GetLastError <> ERROR_ALREADY_EXISTS then
      MessageBox(0, 'This is the evaluation version of madCodeHook.' + #$D#$A +
                    'It works only while "mchEvaluation.exe" is running.',
                    'madCodeHook information...', MB_ICONINFORMATION);
  end;
  if OriginalAPIs <> nil then
     for i1 := 2 to high(OriginalAPIs) do
       if dword(GetProcAddress(HInstance, pchar(i1))) + 5 = dword(caller) then begin
         result := pointer(HInstance + OriginalAPIs[i1]);
         break;
       end;
end;

procedure IndexXx;
asm
  pushad
  mov eax, [esp+$20]
  call CallRealApi
  cmp eax, 0
  jz @doNothing
  mov [esp+$20], eax
  popad
  ret
 @doNothing:
  popad
  xor eax, eax
end;

procedure Index02; asm call IndexXx ret 16 end;
procedure Index03; asm call IndexXx ret 20 end;
procedure Index04; asm call IndexXx ret  4 end;
procedure Index05; asm call IndexXx ret 44 end;
procedure Index06; asm call IndexXx ret  8 end;
procedure Index07; asm call IndexXx ret  8 end;
procedure Index08; asm call IndexXx ret 16 end;
procedure Index09; asm call IndexXx ret 28 end;
procedure Index10; asm call IndexXx ret 12 end;
procedure Index11; asm call IndexXx ret  4 end;
procedure Index12; asm call IndexXx ret  0 end;
procedure Index13; asm call IndexXx ret  0 end;
procedure Index14; asm call IndexXx ret 44 end;
procedure Index15; asm call IndexXx ret 12 end;
procedure Index18; asm call IndexXx ret 12 end;
procedure Index19; asm call IndexXx ret 12 end;
procedure Index20; asm call IndexXx ret  0 end;
procedure Index21; asm call IndexXx ret  0 end;
procedure Index22; asm call IndexXx ret 20 end;
procedure Index23; asm call IndexXx ret  4 end;
procedure Index24; asm call IndexXx ret  0 end;
procedure Index27; asm call IndexXx ret  4 end;
procedure Index28; asm call IndexXx ret  4 end;
procedure Index29; asm call IndexXx ret  8 end;
procedure Index30; asm call IndexXx ret  8 end;
procedure Index31; asm call IndexXx ret  8 end;
procedure Index32; asm call IndexXx ret  8 end;
procedure Index33; asm call IndexXx ret  4 end;
procedure Index34; asm call IndexXx ret 12 end;
procedure Index35; asm call IndexXx ret  4 end;
procedure Index36; asm call IndexXx ret  8 end;
procedure Index37; asm call IndexXx ret 28 end;
procedure Index38; asm call IndexXx ret  4 end;
procedure Index40; asm call IndexXx ret  0 end;
procedure Index41; asm call IndexXx ret  0 end;
procedure Index42; asm call IndexXx ret 32 end;
procedure Index43; asm call IndexXx ret 32 end;
procedure Index44; asm call IndexXx ret 32 end;
procedure Index45; asm call IndexXx ret 32 end;
procedure Index46; asm call IndexXx ret  8 end;
procedure Index47; asm call IndexXx ret 16 end;
procedure Index48; asm call IndexXx ret  4 end;
procedure Index49; asm call IndexXx ret  4 end;
procedure Index52; asm call IndexXx ret  4 end;
procedure Index53; asm call IndexXx ret  4 end;
procedure Index54; asm call IndexXx ret  4 end;
procedure Index55; asm call IndexXx ret  8 end;
procedure Index56; asm call IndexXx ret  4 end;
procedure Index57; asm call IndexXx ret 12 end;
procedure Index58; asm call IndexXx ret 12 end;
procedure Index59; asm call IndexXx ret 16 end;
procedure Index60; asm call IndexXx ret  4 end;
procedure Index61; asm call IndexXx ret 12 end;
procedure Index62; asm call IndexXx ret  4 end;
procedure Index63; asm call IndexXx ret  4 end;
procedure Index64; asm call IndexXx ret  0 end;
procedure Index65; asm call IndexXx ret  4 end;
procedure Index66; asm call IndexXx ret  4 end;
procedure Index67; asm call IndexXx ret  4 end;
procedure Index68; asm call IndexXx ret  4 end;
procedure Index69; asm call IndexXx ret 12 end;
procedure Index70; asm call IndexXx ret 12 end;
procedure Index71; asm call IndexXx ret  8 end;

procedure DateTime;
asm
  ret
  dd $77777777
end;

exports
  DateTime index 100,

  Index02 index 102,
  Index03 index 103,
  Index04 index 104,
  Index05 index 105,
  Index06 index 106,
  Index07 index 107,
  Index08 index 108,
  Index09 index 109,
  Index10 index 110,
  Index11 index 111,
  Index12 index 112,
  Index13 index 113,
  Index14 index 114,
  Index15 index 115,
  Index18 index 118,
  Index19 index 119,
  Index20 index 120,
  Index21 index 121,
  Index22 index 122,
  Index23 index 123,
  Index24 index 124,
  Index27 index 127,
  Index28 index 128,
  Index29 index 129,
  Index30 index 130,
  Index31 index 131,
  Index32 index 132,
  Index33 index 133,
  Index34 index 134,
  Index35 index 135,
  Index36 index 136,
  Index37 index 137,
  Index38 index 138,
  Index40 index 140,
  Index41 index 141,
  Index42 index 142,
  Index43 index 143,
  Index44 index 144,
  Index45 index 145,
  Index46 index 146,
  Index47 index 147,
  Index48 index 148,
  Index49 index 149,
  Index52 index 152,
  Index53 index 153,
  Index54 index 154,
  Index55 index 155,
  Index56 index 156,
  Index57 index 157,
  Index58 index 158,
  Index59 index 159,
  Index60 index 160,
  Index61 index 161,
  Index62 index 162,
  Index63 index 163,
  Index64 index 164,
  Index65 index 165,
  Index66 index 166;
  Index67 index 167;
  Index68 index 168;
  Index69 index 169;
  Index70 index 170;
  Index71 index 171;

function InjectLibraryX(inject, session: bool; sessionNo, processHandle: dword; systemProcesses: bool; libFileNameA: pchar; libFileNameW: PWideChar; timeOut: dword) : bool;
type TInjectLibraryIpc = record
                           inject          : bool;
                           session         : bool;
                           sessionNo       : dword;
                           processHandle   : dword;
                           systemProcesses : bool;
                           timeOut         : dword;
                           name            : word;
                         end;
var ili  : ^TInjectLibraryIpc;
    size : integer;
    s1   : string;
    ws1  : string;
    res  : record
             result    : bool;
             lastError : dword;
           end;
begin
  if CheckLibFilePath(libFileNameA, libFileNameW, s1, ws1) then begin
    if (processHandle or CURRENT_PROCESS) and (not SYSTEM_PROCESSES) = CURRENT_SESSION then begin
      session := true;
      sessionNo := GetCurrentSessionId;
      systemProcesses := processHandle and SYSTEM_PROCESSES <> 0;
    end;
    if GetVersion and $80000000 = 0 then
      size := sizeOf(ili^) + lstrlenW(libFileNameW) * 2 + 2
    else
      size := sizeOf(ili^) + lstrlenA(libFileNameA) + 1;
    ili := pointer(LocalAlloc(LPTR, size));
    ili.inject          := inject;
    ili.session         := session;
    ili.sessionNo       := sessionNo;
    ili.processHandle   := processHandle;
    ili.systemProcesses := systemProcesses;
    ili.timeOut         := timeOut;
    if ((processHandle or CURRENT_PROCESS) and (not SYSTEM_PROCESSES) <> ALL_SESSIONS   ) and
       ((processHandle or CURRENT_PROCESS) and (not SYSTEM_PROCESSES) <> CURRENT_SESSION) and
       ((processHandle or CURRENT_PROCESS) and (not SYSTEM_PROCESSES) <> CURRENT_USER   ) then
      ili.processHandle := ProcessHandleToId(processHandle);
    if GetVersion and $80000000 = 0 then
      Move(libFileNameW^, ili.name, lstrlenW(libFileNameW) * 2 + 2)
    else
      Move(libFileNameA^, ili.name, lstrlenA(libFileNameA) + 1);
    if SendIpcMessage('mchEvaluation', ili, size, @res, sizeOf(res), INFINITE, false) then begin
      result := res.result;
      SetLastError(res.lastError);
    end else
      result := false;
    LocalFree(dword(ili));
  end else
    result := false;
end;

function InjectLibraryA(processHandle: dword; libFileName: pchar; timeOut: dword = 7000) : bool; stdcall;
begin
  result := InjectLibraryX(true, false, 0, processHandle, false, libFileName, nil, timeOut);
end;

function InjectLibraryW(processHandle: dword; libFileName: pwidechar; timeOut: dword = 7000) : bool; stdcall;
begin
  result := InjectLibraryX(true, false, 0, processHandle, false, nil, libFileName, timeOut);
end;

function InjectLibrarySessionA(session: dword; systemProcesses: bool; libFileName: pchar; timeOut: dword = 7000) : bool; stdcall;
begin
  result := InjectLibraryX(true, true, session, 0, systemProcesses, libFileName, nil, timeOut);
end;

function InjectLibrarySessionW(session: dword; systemProcesses: bool; libFileName: pwidechar; timeOut: dword = 7000) : bool; stdcall;
begin
  result := InjectLibraryX(true, true, session, 0, systemProcesses, nil, libFileName, timeOut);
end;

function UninjectLibraryA(processHandle: dword; libFileName: pchar; timeOut: dword = 7000) : bool; stdcall;
begin
  result := InjectLibraryX(false, false, 0, processHandle, false, libFileName, nil, timeOut);
end;

function UninjectLibraryW(processHandle: dword; libFileName: pwidechar; timeOut: dword = 7000) : bool; stdcall;
begin
  result := InjectLibraryX(false, false, 0, processHandle, false, nil, libFileName, timeOut);
end;

function UninjectLibrarySessionA(session: dword; systemProcesses: bool; libFileName: pchar; timeOut: dword = 7000) : bool; stdcall;
begin
  result := InjectLibraryX(false, true, session, 0, systemProcesses, libFileName, nil, timeOut);
end;

function UninjectLibrarySessionW(session: dword; systemProcesses: bool; libFileName: pwidechar; timeOut: dword = 7000) : bool; stdcall;
begin
  result := InjectLibraryX(false, true, session, 0, systemProcesses, nil, libFileName, timeOut);
end;

function CPInject(owner: dword; pi: TProcessInformation; flags: dword; libA: pchar; libW: pwidechar) : boolean;
begin
  result := InjectLibraryX(true, false, 0, pi.hProcess, false, libA, libW, INFINITE);
  if result then begin
    if flags and CREATE_SUSPENDED = 0 then
      ResumeThread(pi.hThread);
  end else
    TerminateProcess(pi.hProcess, 0);
end;

function CreateProcessExA(applicationName, commandLine : pchar;
                          processAttr, threadAttr      : PSecurityAttributes;
                          inheritHandles               : BOOL;
                          creationFlags                : DWORD;
                          environment                  : pointer;
                          currentDirectory             : pchar;
                          const startupInfo            : TStartupInfo;
                          var processInfo              : TProcessInformation;
                          loadLibrary                  : pchar              ) : bool; stdcall;
begin
  result := CreateProcess(applicationName, commandLine, processAttr, threadAttr,
                          inheritHandles, creationFlags or CREATE_SUSPENDED,
                          environment, currentDirectory, startupInfo, processInfo) and
            CPInject(GetCallingModule, processInfo, creationFlags, loadLibrary, nil);
end;

function CreateProcessExW(applicationName, commandLine : pwidechar;
                          processAttr, threadAttr      : PSecurityAttributes;
                          inheritHandles               : bool;
                          creationFlags                : dword;
                          environment                  : pointer;
                          currentDirectory             : pwidechar;
                          const startupInfo            : TStartupInfo;
                          var processInfo              : TProcessInformation;
                          loadLibrary                  : pwidechar          ) : bool; stdcall;
const CUnicows        : AnsiString = (* unicows.dll    *) #$20#$3B#$3C#$36#$3A#$22#$26#$7B#$31#$39#$39;
      CCreateProcessW : AnsiString = (* CreateProcessW *) #$16#$27#$30#$34#$21#$30#$05#$27#$3A#$36#$30#$26#$26#$02;
var unicows : dword;
    cpw     : function (app, cmd, pattr, tattr: pointer; inherit: bool; flags: dword;
                        env, dir: pointer; const si: TStartupInfo; var pi: TProcessInformation) : bool; stdcall;
begin
  cpw := nil;
  if GetVersion and $80000000 <> 0 then begin
    unicows := GetModuleHandle(pchar(DecryptStr(CUnicows)));
    if unicows = 0 then
      unicows := LoadLibraryA(pchar(DecryptStr(CUnicows)));
    if unicows <> 0 then
      cpw := GetProcAddress(unicows, pchar(DecryptStr(CCreateProcessW)));
  end;
  if @cpw = nil then
    cpw := @CreateProcessW;
  result := cpw(applicationName, commandLine, processAttr, threadAttr,
                inheritHandles, creationFlags or CREATE_SUSPENDED,
                environment, currentDirectory, startupInfo, processInfo) and
            CPInject(GetCallingModule, processInfo, creationFlags, nil, loadLibrary);
end;

{$endif}

// ***************************************************************

function InjectLibraryA(libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
begin
  result := madCodeHook.InjectLibraryA(libFileName, processHandle, timeOut);
end;

function InjectLibraryW(libFileName: PWideChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
begin
  result := madCodeHook.InjectLibraryW(libFileName, processHandle, timeOut);
end;

function UninjectLibraryA(libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
begin
  result := madCodeHook.UninjectLibraryA(libFileName, processHandle, timeOut);
end;

function UninjectLibraryW(libFileName: PWideChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
begin
  result := madCodeHook.UninjectLibraryW(libFileName, processHandle, timeOut);
end;

function InjectLibrarySystemWideA(driverName, libFileName: PAnsiChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PAnsiChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall;
begin
  result := madCodeHook.InjectLibraryA(driverName, libFileName, session, systemProcesses, includeMask, excludeMask, excludePIDs, timeOut);
end;

function InjectLibrarySystemWideW(driverName, libFileName: PWideChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PWideChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall;
begin
  result := madCodeHook.InjectLibraryW(driverName, libFileName, session, systemProcesses, includeMask, excludeMask, excludePIDs, timeOut);
end;

function UninjectLibrarySystemWideA(driverName, libFileName: PAnsiChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PAnsiChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall;
begin
  result := madCodeHook.UninjectLibraryA(driverName, libFileName, session, systemProcesses, includeMask, excludeMask, excludePIDs, timeOut);
end;

function UninjectLibrarySystemWideW(driverName, libFileName: PWideChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PWideChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall;
begin
  result := madCodeHook.UninjectLibraryW(driverName, libFileName, session, systemProcesses, includeMask, excludeMask, excludePIDs, timeOut);
end;

// ***************************************************************

exports
  {$ifndef madRmt}
  HookCode                   index  2,
  HookAPI                    index  3,
  UnhookCode                 index  4,
  CreateProcessExA           index  5,
  {$endif}
  AllocMemEx                 index  6,
  FreeMemEx                  index  7,
  CopyFunction               index  8,
  madCreateRemoteThread      index  9,
  {$ifndef madRmt}
  InjectLibraryA             index 10,
  {$endif}
  ProcessHandleToId          index 11,
  {$ifndef madRmt}
  CollectHooks               index 12,
  FlushHooks                 index 13,
  CreateProcessExW           index 14,
  InjectLibraryW             index 15,
  UninjectLibraryA           index 18,
  UninjectLibraryW           index 19,
  AmUsingInputDesktop        index 20,
  AmSystemProcess            index 21,
  {$endif}
  RemoteExecute              index 22,
  {$ifndef madRmt}
  RenewHook                  index 23,
  GetCallingModule           index 24,
  CreateGlobalMutex          index 27,
  OpenGlobalMutex            index 28,
  CreateGlobalFileMapping    index 29,
  OpenGlobalFileMapping      index 30,
  AnsiToWide                 index 31,
  WideToAnsi                 index 32,
  UnhookAPI                  index 33,
  CreateGlobalEvent          index 34,
  OpenGlobalEvent            index 35,
  CreateIpcQueue             index 36,
  SendIpcMessage             index 37,
  DestroyIpcQueue            index 38,
  GetCurrentSessionId        index 40,
  GetInputSessionId          index 41,
  InjectLibrarySystemWideA   index 42,
  InjectLibrarySystemWideW   index 43,
  UninjectLibrarySystemWideA index 44,
  UninjectLibrarySystemWideW index 45,
  AddAccessForEveryone       index 46,
  CreateIpcQueueEx           index 47,
  IsHookInUse                index 48,
  AutoUnhook                 index 49,
  {$endif}
  StaticLibHelper_Init       index 52,
  StaticLibHelper_Final      index 53,
  ThreadHandleToId           index 54
  {$ifndef madRmt},
  SetMadCHookOption          index 55,
  RestoreCode                index 56,
  {$endif}
  ProcessIdToFileNameA       index 57,
  ProcessIdToFileNameW       index 58,
  InstallInjectionDriver     index 59,
  UninstallInjectionDriver   index 60,
  LoadInjectionDriver        index 61,
  StopInjectionDriver        index 62,
  StartInjectionDriver       index 63,
  Is64bitOS                  index 64,
  Is64bitProcess             index 65,
  Is64bitModule              index 66,
  IsInjectionDriverInstalled index 67,
  IsInjectionDriverRunning   index 68,
  UninjectAllLibrariesA      index 69,
  UninjectAllLibrariesW      index 70,
  SetInjectionMethod         index 71;

// ***************************************************************

function GetCurrentEip : dword;
// where is the current thread running?
asm
  mov eax, [esp]
end;

{$ifndef madRmt}
var nh : PImageNtHeaders;
begin  // <- this is the entry point of the madCHook.dll
  {$ifdef protect}
    InitSpecialProcs;
  {$endif}

  nh := GetImageNtHeaders(HInstance);
  // we can either be madCHook.dll or we can be the static lib file for C++
  // that's important to know, because we must behave differently in each case
  // we can find out what we are by checking the entry point of our own dll
  // if we are madCHook.dll, the entry point must be the "begin" marked above
  amMchDll := (nh <> nil) and
              (HInstance + nh^.OptionalHeader.AddressOfEntryPoint > dword(@GetCurrentEip)) and
              (HInstance + nh^.OptionalHeader.AddressOfEntryPoint < GetCurrentEip        );
{$endif}
end.
