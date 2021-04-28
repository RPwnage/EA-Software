// ***************************************************************
//  madCodeHook.pas           version: 3.0.1  ·  date: 2011-03-27
//  -------------------------------------------------------------
//  API/code hooking, DLL injection
//  -------------------------------------------------------------
//  Copyright (C) 1999 - 2011 www.madshi.net, All Rights Reserved
// ***************************************************************

// 2011-03-27 3.0.1 (1) CreateGlobalFileMapping: LastError value is now restored
//                  (2) (Un)InjectLibraryA: improved ansi string conversion
//                  (3) added IsInjectionDriverInstalled/Running
//                  (4) added UninjectAllLibraries
//                  (5) added SetInjectionMethod
//                  (6) improved injection driver APIs' GetLastError handling
//                  (7) small change to make (un)injection slightly more stable
//                  (8) fixed win9x injection crash
//                  (9) sped up first HookAPI call
//                  (a) modified internal LoadLibrary hook (win7) once again
//                  (b) added SetInjectionMethod API
//                  (c) added UninjectAllLibrariesA/W API
//                  (d) added special handling for "wine"
//                  (e) increased default internal IPC timeout from 2s to 7s
// 2010-01-10 3.0.0 (1) added full support for 64bit OSs
//                  (2) DLL injection APIs changed
//                  (3) injection driver control APIs added
//                  (4) process black & white lists for DLL injection added
//                  (5) (Un)InstallMadCHook replaced by driver injection APIs
//                  (6) Is64bitOS added + Is64bitProcess exported
//                  (7) CreateProcessEx now works for 64bit processes
//                  (8) ProcessIdToFileName modified and unicode version added
//                  (9) added "INJECT_INTO/UNINJECT_FROM_RUNNING_PROCESSES"
// 2010-01-07 2.2l  (1) privileges stay untouched in hook dlls (win 7 stability)
//                  (2) fixed: some allocation was still done at 0x5f040000
//                  (3) fixed: internal bug in CollectCache
//                  (4) modified LoadLibrary hook for Windows 7
// 2009-09-15 2.2k  (1) fixed: DLL inj failed when some native APIs were hooked
//                  (2) fixed: SendIpcMessage sometimes failed in Windows 7
//                  (3) moved allocation from $71700000 to $71b00000 (msys bug)
//                  (4) fixed: LoadLibrary hook eventually crashed in x64 OSs
//                  (5) DONT_TOUCH_RUNNING_PROCESSES now only affects injection
// 2009-07-22 2.2j  (1) other libs' hooks are not overwritten by default, anymore
//                  (2) DONT_RENEW_OVERWRITTEN_HOOKS -> RENEW_OVERWRITTEN_HOOKS
//                  (3) RestoreCode added
//                  (4) DLL injection thread now always uses 1MB stack size
// 2009-02-09 2.2i  (1) Delphi 2009 support
//                  (2) added new option "DONT_TOUCH_RUNNING_PROCESSES"
//                  (3) added new option "DONT_RENEW_OVERWRITTEN_HOOKS"
//                  (4) fixed "USE_EXTERNAL_DRIVER_FILE": driver got deleted
// 2008-02-18 2.2h  fixed DEP issue introduced with version 2.2f <sigh>
// 2008-01-19 2.2g  fixed exception introduced with version 2.2f
// 2008-01-14 2.2f  (1) new function "SetMadCHookOption" added
//                  (2) stability check was too careful with UPXed module
//                  (3) SeTakeOwnershipPrivilege is not enabled, anymore
//                  (4) improved cooperation with other hooking libraries
//                      - internal LoadLibrary hook modified to avoid conflicts
//                      - when unhooking fails, stability is still maintained
//                  (5) hooking unprotected (writable) code sometimes failed
// 2007-05-28 2.2e  IPC works in Vista64 now, too
// 2007-03-28 2.2d  (1) incompatability with ZoneAlarm AV / Kaspersky fixed
//                  (2) ALLOW_WINSOCK2_MIXTURE_MODE flag added
//                  (3) Vista ASLR incorrectly triggered mixture mode
// 2006-11-28 2.2c  (1) endless recursion protection for tool functions
//                  (2) some first changes for 64bit OS support
//                  (3) Get/SetLastError value for LoadLibrary was destroyed
//                  (4) "MixtureCache" renamed to "CollectCache"
//                  (5) CollectHooks improves performance even more now
//                  (6) improved performance in internal LoadLibrary hook
//                  (7) little bug in dll injection fixed
//                  (8) official Vista (32bit) support added
// 2006-01-30 2.2b  (1) Windows Vista (32bit) support added
//                  (2) Injection driver allocates at $71700000 now (winNT)
// 2005-11-28 2.2a  (1) fixed deletion of invalid crit. section in finalization
//                  (2) auto unhooking bug fixed
//                  (3) GetCallingModule crash in non-com. Delphi edition fixed
// 2005-07-23 2.2   (1) fixed a little resource leak (1 critical section per dll)
//                  (2) NextHook variable stays usable after unhooking now
//                  (3) flag "DONT_COUNT" renamed to "NO_SAFE_UNHOOKING"
//                  (4) safe unhooking slightly improved (winNT)
//                  (5) "SAFE_HOOKING" -> thread safe hook installation (winNT)
//                  (6) turning "compiler -> optimization off" made problems
//                  (7) reduced double hooking problems with other hook libaries
//                  (8) mchInjDrv doesn't get listed in device manager, anymore
//                  (9) you can now permanently install the injection driver
// 2005-01-02 2.1e  patch 6 bytes before page boundary: hook installation failed
// 2004-09-26 2.1d  (1) memory leak in CreateIpcQueue fixed
//                  (2) win9x: injection skips newly created 16bit processes now
// 2004-08-05 2.1c  (1) CURRENT_PROCESS is not ignored by UninjectLibrary anymore
//                  (2) enabling Backup/Restore privileges broke Samba support
//                  (3) some tweaks for win98, when IPC functions are stressed
//                  (4) xp sp2: IPC sending from under privileged account failed
//                  (5) IsHookInUse added
// 2004-04-25 2.1b  (1) VirtualAlloc now allocates >= $5f040000 (winNT)
//                  (2) new injection driver version embedded
// 2004-04-12 2.1a  (1) hook finalization: "nextHook" may be zeroed too early
//                  (2) safe unhooking improved (16 -> 280 thread capacity)
//                  (3) AMD64 NX: LocalAlloc -> VirtualAlloc (TCodeHook.Create)
//                  (4) GetTickCount doesn't overrun now, anymore
// 2004-03-07 2.1   (1) dll inject into new processes done by kernel driver (nt)
//                  (2) w2k3 support improved
//                  (3) AutoCAD debugger detection doesn't trigger anymore
//                  (4) ProtectMemory added where UnprotectMemory was used
//                  (5) PatchExportTable sometimes froze IE in win2k
//                  (6) special fix for a "Process Explorer" bug (see CheckMap)
//                  (7) InjectLibrary wide string handling bug fixed
//                  (8) bug in library path checking functions fixed
//                  (9) NO_MIXTURE_MODE flag added
//                  (a) the mixture mode can't be used for ws_32.dll
//                  (b) workaround for 2k/XP DuplicateHandle bug in AcLayers.dll
//                  (c) initialization ends with SetLastError(0), just in case
//                  (d) HookCode/API calls SetLastError(0) when it succeeds
//                  (e) CheckHooks doesn't modify the LastError value anymore
//                  (f) dummy win9x API detection improved
//                  (g) virtual memory allocation now >= $5f000000 (winNT)
//                  (h) CreateIpcQueueEx added with additional parameters
// 2003-11-10 2.0a  (1) memory leak in API hooking code fixed
//                  (2) some bugs in Ipc functionality fixed
//                  (3) memory leak in madCHook.dll/lib fixed
//                  (4) call CloseHandle/ReleaseMutex/etc only for valid handles
//                  (5) error handling improved a bit
// 2003-08-10 2.0   (1) most code is totally rewritten/changed
//                  (2) HookCode parameters changed -> only one flags parameter
//                  (3) automatic mixture mode detection mightily improved
//                  (4) process wide hooking of system APIs in win9x more stable
//                  (5) shared PE sections are fully supported now (win9x)
//                  (6) dummy 9x wide APIs (e.g. CreateFileW) are not hooked
//                  (7) (Un)InjectLibrary: user/session/system wide injection!
//                  (8) InjectLibrary2 replaced by InjectLibrary (auto detect)
//                  (9) hook dlls protected from unauthorized FreeLibrary calls
//                  (a) fully automatic unhooking before dll unloading
//                  (b) safe unhooking implemented
//                  (c) static lib for Microsoft C++ added
//                  (d) CreateIpcQueue + SendIpcMessage + DestroyIpcQueue added
//                  (e) AmSystemProcess + AmUsingInputDesktop added
//                  (f) GetCurrentSessionId + GetInputSessionId added
//                  (g) GetCallingModule function added
//                  (h) ProcessIdToFileName added
//                  (i) Create/OpenGlobalMutex + Event + FileMapping added
//                  (j) WideToAnsi + AnsiToWide functions added
//                  (k) IAT patching doesn't stumble about compressed modules
//                  (l) try..except/try..finally works without SysUtils now
//                  (m) RenewHook function added
//                  (n) madCodeHook.dll -> madCHook.dll (8.3 dos name logic)
//                  (o) madIsBadModule tool added
//                  (p) several nice new demos added
//                  (q) UnhookAPI added (= UnhookCode, added just for the look)
//                  (r) AddAccessForEveryone added
// 2002-11-13 1.3k  (1) using GetMem was a bad idea for dlls, now LocalAlloc
//                  (2) mixture mode hooks get uninstalled now, too
//                  (3) InjectLibraryW didn't allocate enough memory for dll path
//                  (4) InjectLibrary2(W) doesn't ask GetProcessVersion, anymore
// 2002-10-17 1.3j  (1) InjectLibrary2(W) was not stdcall (ouch)
//                  (2) AtomicMove improves stability when (un)installing Hook
// 2002-10-06 1.3i  improved reliability of CreateProcessEx in 9x (hopefully)
// 2002-10-03 1.3h  (1) 1.3g introduced a bug in CreateProcessEx
//                  (2) InjectLibraryW added
//                  (3) InjectLibrary2(W) added for use in CreateProcess(W) hooks
// 2002-09-22 1.3g  (1) added a FreeMem to TCodeHook.Destroy to fix a memory leak
//                  (2) CreateProcessExW added
//                  (3) GetProcAddressEx avoids IAT patching problems
// 2002-04-06 1.3f  InstallMixturePatcher only did a part of what it should
// 2002-03-24 1.3e  (1) mixture mode initialization changed again
//                  (2) CollectHooks/FlushHooks speed up mixture initialization
//                  (3) bug in PatchMyImportTables made HookCode crash sometimes
// 2002-03-15 1.3d  CreateProcessEx now works with .net programs in NT family
// 2002-03-10 1.3c  when using system wide hooking with mixture mode in win9x
//                  each process now patches its own import tables
//                  this gets rid of any (noticable) initialization delays
// 2002-02-23 1.3b  mutexes were not entered, if they already existed
// 2001-07-28 1.3a  serious bug (-> crash!) in CreateProcessEx fixed
// 2001-07-08 1.3   InjectLibrary added
// 2001-05-25 1.2b  mixture: automatic IAT patching of new processes/modules (9x)
//                  normally not needed, because we also patch the export table
//                  but in rare cases the import table is hard coded
// 2001-04-20 1.2a  you can now force HookCode to use the mixture mode
// 2000-12-23 1.2   new function CreateProcessEx -> dll injecting
// 2000-11-28 1.1c  bug in PatchImportTable fixed (2nd missing VirtualProtectEx)
// 2000-11-26 1.1b  (1) bug in PatchImportTable fixed (missing VirtualProtectEx)
//                  (2) support for w9x debug mode
// 2000-11-25 1.1a  two bugs in the new hooking mode fixed
// 2000-11-23 1.1   (1) integrated enhanced import/export table patching added
//                      so now we should be able to hook really *every* API
//                  (2) no interfaces anymore to get rid of madBasic
// 2000-07-25 1.0d  minor changes in order to get rid of SysUtils

unit madCodeHook;

{$I mad.inc} {$W-}

// use ntdll entry point patching in XP and higher?
// advantage: dll gets initialized even before the statically linked dlls are
// disadvantage: dll can't be uninjected anymore
{ $define InjectLibraryPatchXp}

// enable logging?
{ $define log}

// enable automatic infinite recursion protection?
{$define CheckRecursion}

interface

uses Windows, madTypes, madDisAsm;

// ***************************************************************
// 64bit specific functions

// is the current OS a native 64bit OS?
function Is64bitOS : bool; stdcall;

// is the specified process a native 64bit process?
function Is64bitProcess(processHandle: dword) : bool; stdcall;

// ***************************************************************

const
  // don't use the driver file embedded in madCodeHook
  // instead use an external driver file
  // to avoid virus false alarms you can rename the driver
  // you can also sign it with your own Verisign certificate
  // param: e.g. "C:\Program Files\yourSoft\yourInjDrv.sys"
  USE_EXTERNAL_DRIVER_FILE = $00000001;

  // make all memory maps available only to current user + admins + system
  // without this option all memory maps are open for Everyone
  // this option is not fully tested yet
  // so use it only after extensive testing and on your own danger
  // param: unsused
  SECURE_MEMORY_MAPS = $00000002;

  // before installing an API hook madCodeHook does some security checks
  // one check is verifying whether the to be hooked code was already modified
  // in this case madCodeHook does not tempt to modify the code another time
  // otherwise there would be a danger to run into stability issues
  // with protected/compressed modules there may be false alarms, though
  // so you can turn this check off
  // param: unsused
  DISABLE_CHANGED_CODE_CHECK = $00000003;

  // madCodeHook has two different IPC solutions built in
  // in Vista and in all 64 bit OSs the "old" IPC solution doesn't work
  // so in these OSs the new IPC solution is always used
  // in all other OSs the old IPC solution is used by default
  // the new solution is based on undocumented internal Windows IPC APIs
  // the old solution is based on pipes and memory mapped files
  // you can optionally enable the new IPC solution for the older OSs, too
  // the new IPC solution doesn't work in win9x and so cannot be enabled there
  // param: unused
  USE_NEW_IPC_LOGIC = $00000004;

  // when calling SendIpcMessage you can specify a timeout value
  // this value only applies to how long madCodeHook waits for the reply
  // there's an additional internal timeout value which specifies how long
  // madCodeHook waits for the IPC message to be accepted by the queue owner
  // the default value is 7000ms
  // param: internal timeout value in ms
  // example: SetMadCHookOption(SET_INTERNAL_IPC_TIMEOUT, PWideChar(5000));
  SET_INTERNAL_IPC_TIMEOUT = $00000005;

  // VMware: when disabling acceleration dll injection sometimes is delayed
  // to work around this issue you can activate this special option
  // it will result in a slightly modified dll injection logic
  // as a side effect injection into DotNet applications may not work properly
  // param: unused
  VMWARE_INJECTION_MODE = $00000006;

  // system wide dll injection normally injects the hook dll into both:
  // (1) currently running processes
  // (2) newly created processes
  // this flag disables injection into already running processes
  // param: unused
  DONT_TOUCH_RUNNING_PROCESSES = $00000007;

  // normally madCodeHook renews hooks only when they were removed
  // hooks that were overwritten with some other code aren't renewed by default
  // this behaviour allows other hooking libraries to co-exist with madCodeHook
  // use this flag to force madCodeHook to always renew hooks
  // this may result in other hooking libraries stopping to work correctly
  // param: unused
  RENEW_OVERWRITTEN_HOOKS = $00000009;

  // system wide dll injection normally injects the hook dll into both:
  // (1) currently running processes
  // (2) newly created processes
  // this option enabled/disables injection into already running processes
  // param: bool
  // default: true
  INJECT_INTO_RUNNING_PROCESSES = $0000000a;

  // system wide dll uninjection normally does two things:
  // (1) uninjecting from all running processes
  // (2) stop automatic injection into newly created processes
  // this option controls if uninjection from running processes is performed
  // param: bool
  // default: true
  UNINJECT_FROM_RUNNING_PROCESSES = $0000000b;

// with this API you can configure some aspects of madCodeHook
// available options see constants above
function SetMadCHookOption (option: dword; param: PWideChar) : bool; stdcall;

// ***************************************************************

const
  // by default madCodeHook counts how many times any thread is currently
  // running inside of your callback function
  // this way unhooking can be safely synchronized to that counter
  // sometimes you don't need/want this counting to happen, e.g.
  // (1) if you don't plan to ever unhook, anyway
  // (2) if the counting performance drop is too high for your taste
  // (3) if you want to unhook from inside the hook callback function
  // in those cases you can set the flag "NO_SAFE_UNHOOKING"
  DONT_COUNT = $00000001;         // old name - kept for compatability
  NO_SAFE_UNHOOKING = $00000001;  // new name

  // with 2.1f the "safe unhooking" functionality (see above) was improved
  // most probably there's no problem with the improvement
  // but to be sure you can disable the improvement
  // the improved safe unhooking is currently only available in the NT family
  NO_IMPROVED_SAFE_UNHOOKING = $00000040;

  // optionally madCodeHook can use a special technique to make sure that
  // hooking in multi threaded situations won't result in crashing threads
  // this technique is not tested too well right now, so it's optional for now
  // you can turn this feature on by setting the flag "SAFE_HOOKING"
  // without this technique crashes can happen, if a thread is calling the API
  // which we want to hook in exactly the moment when the hook is installed
  // safe hooking is currently only available in the NT family
  SAFE_HOOKING = $00000020;

  // madCodeHook implements two different API hooking methods
  // the mixture mode is the second best method, it's only used if the main
  // hooking method doesn't work for whatever reason (e.g. API code structure)
  // normally madCodeHook chooses automatically which mode to use
  // you can force madCodeHook to use the mixture mode by specifying the flag:
  MIXTURE_MODE = $00000002;

  // if you don't want madCodeHook to use the mixture mode, you can say so
  // however, if the main hooking mode can't be used, hooking then simply fails
  NO_MIXTURE_MODE = $00000010;

  // winsock2 normally doesn't like the mixture mode
  // however, I've found a way to convince winsock2 to accept mixture hooks
  // this is a somewhat experimental feature, though
  // so it must be turned on explicitly
  ALLOW_WINSOCK2_MIXTURE_MODE = $00000080;

  // under win9x you can hook code system wide, if it begins > $80000000
  // or if the code section of the to-be-hooked dll is shared
  // the callback function is in this case automatically copied to shared memory
  // use only kernel32 APIs in such a system wide hook callback function (!!)
  // if you want an easier way and/or a NT family compatible way to hook code
  // system wide, please use InjectLibrary(ALL_SESSIONS) instead of these flags:
  SYSTEM_WIDE_9X            = $00000004;
  ACCEPT_UNKNOWN_TARGETS_9X = $00000008;

// hook any code or a specific API
function HookCode (code                 : pointer;
                   callbackFunc         : pointer;
                   out nextHook         : pointer;
                   flags                : dword = 0) : bool; stdcall;
function HookAPI  (module, api          : PAnsiChar;
                   callbackFunc         : pointer;
                   out nextHook         : pointer;
                   flags                : dword = 0) : bool; stdcall;

// some firewall/antivirus programs kill our hooks, so we need to renew them
function RenewHook (var nextHook: pointer) : bool; stdcall;

// is the hook callback function of the specified hook currently in use?
// 0: the hook callback function is not in use
// x: the hook callback function is in use x times
function IsHookInUse (var nextHook: pointer) : integer; stdcall;

// unhook again
function UnhookCode (var nextHook: pointer) : bool; stdcall;
function UnhookAPI  (var nextHook: pointer) : bool; stdcall;

// putting all your "HookCode" calls into a "CollectHooks".."FlushHooks" frame
// can eventually speed up the installation of the hooks
procedure CollectHooks;
procedure   FlushHooks;

// restores the original code of the API/function (only first 6 bytes)
// the original code is read from the dll file on harddisk
// you can use this function e.g. to remove the hook of another hook library
// don't use this to uninstall your own hooks, use UnhookCode for that purpose
function RestoreCode (code: pointer) : bool; stdcall;

// ***************************************************************

// same as CreateProcess
// additionally the dll "loadLibrary" is injected into the newly created process
// the dll gets loaded before the entry point of the exe module is called
function CreateProcessExA (applicationName, commandLine : PAnsiChar;
                           processAttr, threadAttr      : PSecurityAttributes;
                           inheritHandles               : bool;
                           creationFlags                : dword;
                           environment                  : pointer;
                           currentDirectory             : PAnsiChar;
                           const startupInfo            : {$ifdef UNICODE} TStartupInfoA; {$else} TStartupInfo; {$endif}
                           var processInfo              : TProcessInformation;
                           loadLibrary                  : PAnsiChar          ) : bool; stdcall;
function CreateProcessExW (applicationName, commandLine : PWideChar;
                           processAttr, threadAttr      : PSecurityAttributes;
                           inheritHandles               : bool;
                           creationFlags                : dword;
                           environment                  : pointer;
                           currentDirectory             : PWideChar;
                           const startupInfo            : {$ifdef UNICODE} TStartupInfoW; {$else} TStartupInfo; {$endif}
                           var processInfo              : TProcessInformation;
                           loadLibrary                  : PWideChar          ) : bool; stdcall;
function CreateProcessEx  (applicationName, commandLine : PAnsiChar;
                           processAttr, threadAttr      : PSecurityAttributes;
                           inheritHandles               : bool;
                           creationFlags                : dword;
                           environment                  : pointer;
                           currentDirectory             : PAnsiChar;
                           const startupInfo            : {$ifdef UNICODE} TStartupInfoA; {$else} TStartupInfo; {$endif}
                           var processInfo              : TProcessInformation;
                           loadLibrary                  : AnsiString         ) : boolean;

// ***************************************************************

const
  // flags for injection/uninjection "session" parameter
  ALL_SESSIONS    : dword = dword(-1);
  CURRENT_SESSION : dword = dword(-2);

// same as Load/FreeLibrary, but is able to load/free the library in any process
function   InjectLibraryA (libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall; overload;
function   InjectLibraryW (libFileName: PWideChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall; overload;
function   InjectLibrary  (libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall; overload;
function UninjectLibraryA (libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall; overload;
function UninjectLibraryW (libFileName: PWideChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall; overload;
function UninjectLibrary  (libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall; overload;

// (un)injects a library to/from all processes of the specified session(s)
// driverName:      name of the driver to use
// libFileName:     full file path/name of the hook dll
// session:         session id into which you want the dll to be injected
//                  "-1" or "ALL_SESSIONS" means all sessions
//                  "-2" or "CURRENT_SESSION" means the current session
// systemProcesses: shall the dll be injected into system processes?
// includeMask:     list of exe file name/path masks into which the hook dll shall be injected
//                  you can use multiple masks separated by a "|" char
//                  you can either use a full path, or a file name, only
//                  leaving this parameter empty means that all processes are "included"
//                  Example: "c:\program files\*.exe|calc.exe|*.scr"
// excludeMask:     list of exe file name/path masks which shall be excluded from dll injection
//                  the excludeMask has priority over the includeMask
//                  leaving this parameter empty means that no processes are "excluded"
// excludePIDs:     list of process IDs which shall not be touched
//                  when used, the list must be terminated with a "0" PID item
function   InjectLibraryA (driverName      : PAnsiChar;
                           libFileName     : PAnsiChar;
                           session         : dword;
                           systemProcesses : bool;
                           includeMask     : PAnsiChar  = nil;
                           excludeMask     : PAnsiChar  = nil;
                           excludePIDs     : TPCardinal = nil;
                           timeOut         : dword      = 7000) : bool; stdcall; overload;
function   InjectLibraryW (driverName      : PWideChar;
                           libFileName     : PWideChar;
                           session         : dword;
                           systemProcesses : bool;
                           includeMask     : PWideChar  = nil;
                           excludeMask     : PWideChar  = nil;
                           excludePIDs     : TPCardinal = nil;
                           timeOut         : dword      = 7000) : bool; stdcall; overload;
function   InjectLibrary  (driverName      : PAnsiChar;
                           libFileName     : PAnsiChar;
                           session         : dword;
                           systemProcesses : bool;
                           includeMask     : PAnsiChar  = nil;
                           excludeMask     : PAnsiChar  = nil;
                           excludePIDs     : TPCardinal = nil;
                           timeOut         : dword      = 7000) : bool; stdcall; overload;
function UninjectLibraryA (driverName      : PAnsiChar;
                           libFileName     : PAnsiChar;
                           session         : dword;
                           systemProcesses : bool;
                           includeMask     : PAnsiChar  = nil;
                           excludeMask     : PAnsiChar  = nil;
                           excludePIDs     : TPCardinal = nil;
                           timeOut         : dword      = 7000) : bool; stdcall; overload;
function UninjectLibraryW (driverName      : PWideChar;
                           libFileName     : PWideChar;
                           session         : dword;
                           systemProcesses : bool;
                           includeMask     : PWideChar  = nil;
                           excludeMask     : PWideChar  = nil;
                           excludePIDs     : TPCardinal = nil;
                           timeOut         : dword      = 7000) : bool; stdcall; overload;
function UninjectLibrary  (driverName      : PAnsiChar;
                           libFileName     : PAnsiChar;
                           session         : dword;
                           systemProcesses : bool;
                           includeMask     : PAnsiChar  = nil;
                           excludeMask     : PAnsiChar  = nil;
                           excludePIDs     : TPCardinal = nil;
                           timeOut         : dword      = 7000) : bool; stdcall; overload;

// uninjects every currently active system/session wide injection
function UninjectAllLibrariesW (driverName: PWideChar; excludePIDs: TPCardinal = nil; timeOutPerUninject: dword = 7000) : bool; stdcall;
function UninjectAllLibrariesA (driverName: PAnsiChar; excludePIDs: TPCardinal = nil; timeOutPerUninject: dword = 7000) : bool; stdcall;
function UninjectAllLibraries  (driverName: PAnsiChar; excludePIDs: TPCardinal = nil; timeOutPerUninject: dword = 7000) : bool; stdcall;

// set the injection method used by the kernel mode driver
function SetInjectionMethod (driverName: PWideChar; newInjectionMethod: bool) : bool; stdcall;

// ***************************************************************

// you can use these APIs to permanently (un)install your injection driver
// it will survive reboots, it will stay installed until you uninstall it
// installing the same driver twice fails succeeds and updates all parameters
function   InstallInjectionDriver (driverName, fileName32bit, fileName64bit, description: PWideChar) : bool; stdcall;
function UninstallInjectionDriver (driverName: PWideChar) : bool; stdcall;

// this will load and activate your injection driver
// it will stay active only until the next reboot
// you may prefer this technique if you want your product to not require any
// installation and uninstallation procedures
// loading the same driver twice fails with ERROR_SERVICE_ALREADY_RUNNING
function LoadInjectionDriver (driverName, fileName32bit, fileName64bit: PWideChar) : bool; stdcall;

// stopping the injection driver will only work:
// (1) if the driver was configured at build time to be stoppable
// (2) if you call StopInjectionDriver
// (3) if there are no dll injections currently running
// stopping the injection driver with the device manager or "sc" is blocked
// call Stop + Start to update an installed driver to a newer version
// call Stop + Load  to update a  loaded    driver to a newer version
function  StopInjectionDriver (driverName: PWideChar) : bool; stdcall;
function StartInjectionDriver (driverName: PWideChar) : bool; stdcall;

// checks whether the specific injection driver is installed
// returns true  if you used InstallInjectionDriver
// returns false if you used LoadInjectionDriver
function IsInjectionDriverInstalled (driverName: PWideChar) : bool; stdcall;

// checks whether the specific injection driver is running
// returns true if you used InstallInjectionDriver and
//              if the driver is running (not paused / stopped)
// returns true if you used LoadInjectionDriver
function IsInjectionDriverRunning (driverName: PWideChar) : bool; stdcall;

// ***************************************************************

// is the current process a service/system process?  (win9x -> always false)
function AmSystemProcess : bool; stdcall;

// is the current thread's desktop the input desktop?  (win9x -> always true)
// only in that case you should show messages boxes or other GUI stuff
// but please note that in XP fast user switching AmUsingInputDesktop may
// return true, although the current session is currently not visible
// XP fast user switching is implemented by using terminal server logic
// so each fast user session has its own window station and input desktop
function AmUsingInputDesktop : bool; stdcall;

// the following two functions can be used to get the session id of the
// current session and of the input session
// each terminal server (or XP fast user switching) session has its own id
// the "input session" is the one currently shown on the physical screen
function GetCurrentSessionId : dword; stdcall;
function GetInputSessionId   : dword; stdcall;

// ***************************************************************

// which module called me? works only if your function has a stack frame
function GetCallingModule : dword; stdcall;

// ***************************************************************

// find out what file the specified process was executed from
function ProcessIdToFileNameA (processId: dword; fileName: PAnsiChar; bufLenInChars: word) : bool; stdcall;
function ProcessIdToFileNameW (processId: dword; fileName: PWideChar; bufLenInChars: word) : bool; stdcall;
function ProcessIdToFileName  (processId: dword; var fileName: AnsiString) : boolean;

// ***************************************************************
// global  =  normal  +  "access for everyone"  +  "non session specific"

function CreateGlobalMutex (name: PAnsiChar) : dword; stdcall;
function   OpenGlobalMutex (name: PAnsiChar) : dword; stdcall;

function CreateGlobalEvent (name: PAnsiChar; manual, initialState: bool) : dword; stdcall;
function   OpenGlobalEvent (name: PAnsiChar                            ) : dword; stdcall;

function CreateGlobalFileMapping (name: PAnsiChar; size: dword) : dword; stdcall;
function   OpenGlobalFileMapping (name: PAnsiChar; write: bool) : dword; stdcall;

// ***************************************************************

// convert strings ansi <-> wide
// the result buffer must have a size of MAX_PATH characters (or more)
// please use these functions in nt wide API hook callback functions
// because the OS' own functions seem to confuse nt in hook callback functions
procedure AnsiToWide (ansi: PAnsiChar; wide: PWideChar); stdcall;
procedure WideToAnsi (wide: PWideChar; ansi: PAnsiChar); stdcall;

// ***************************************************************
// ipc (inter process communication) message services
// in contrast to SendMessage the following functions don't crash NT services

type
  // this is how you get notified about incoming ipc messages
  // you have to write a function which fits to this type definition
  // and then you give it into "CreateIpcQueue"
  // your callback function will then be called for each incoming message
  // CAUTION: each ipc message is handled by a seperate thread, as a result
  //          your callback will be called by a different thread each time
  TIpcCallback = procedure (name       : PAnsiChar;
                            messageBuf : pointer;
                            messageLen : dword;
                            answerBuf  : pointer;
                            answerLen  : dword); stdcall;

// create an ipc queue
// please choose a unique ipc name to avoid conflicts with other programs
// only one ipc queue with the same name can be open at the same time
// so if 2 programs try to create the same ipc queue, the second call will fail
// you can specify how many threads may be created to handle incoming messages
// if the order of the messages is crucial for you, set "maxThreadCount" to "1"
// in its current implementation "maxThreadCount" only supports "1" or unlimited
// the parameter "maxQueueLen" is not yet implemented at all
function CreateIpcQueueEx (ipc            : PAnsiChar;
                           callback       : TIpcCallback;
                           maxThreadCount : dword = 16;
                           maxQueueLen    : dword = $1000) : bool; stdcall;
function CreateIpcQueue   (ipc            : PAnsiChar;
                           callback       : TIpcCallback ) : bool; stdcall;

// send an ipc message to whomever has created the ipc queue (doesn't matter)
// if you only fill the first 3 parameters, SendIpcMessage returns at once
// if you fill the next two parameters, too, SendIpcMessage will
// wait for an answer of the ipc queue owner
// you can further specify how long you're willing to wait for the answer
// and whether you want SendIpcMessage to handle messages while waiting
function SendIpcMessage(ipc            : PAnsiChar;
                        messageBuf     : pointer;
                        messageLen     : dword;
                        answerBuf      : pointer = nil;
                        answerLen      : dword   = 0;
                        answerTimeOut  : dword   = INFINITE;
                        handleMessages : bool    = true    ) : bool; stdcall;

// destroy the ipc queue again
// when the queue owning process quits, the ipc queue is automatically deleted
// only the queue owning process can destroy the queue
function DestroyIpcQueue (ipc: PAnsiChar) : bool; stdcall;

// ***************************************************************

// this function adds some access rights to the specified target
// the target can either be a process handle or a service handle
function AddAccessForEveryone (processOrService, access: dword) : bool; stdcall;

// ***************************************************************

{$ifndef dynamic}
// internal stuff, please do not use
function FindRealCode (code: pointer) : pointer;
function CheckLibFilePath (var libA: PAnsiChar;  var libW: PWideChar;
                           var strA: AnsiString; var strW: AnsiString) : boolean;
procedure InitializeMadCHook;
procedure FinalizeMadCHook;
procedure AutoUnhook (moduleHandle: dword);
procedure EnableAllPrivileges;
var amMchDll : boolean = false;
{$endif}

{$ifdef log}
  procedure log(str: AnsiString; paramA: PAnsiChar = nil; paramW: PWideChar = nil; ph: dword = 0);
{$endif}

{$ifdef protect}
  procedure InitSpecialProcs;
{$endif}

implementation

uses madStrings, madTools, madRemote, madRipeMD;

// ***************************************************************

{$ifdef log}
  function ExtractFileName(path: AnsiString) : AnsiString;
  var i1 : integer;
  begin
    result := path;
    for i1 := Length(result) downto 1 do
      if result[i1] = '\' then begin
        Delete(result, 1, i1);
        break;
      end;
  end;

  var firstLog : boolean = true;
      doLog    : boolean = false;
  procedure log(str: AnsiString; paramA: PAnsiChar = nil; paramW: PWideChar = nil; ph: dword = 0);
  var c1, c2 : dword;
      st     : TSystemTime;
      time   : AnsiString;
      mutex  : dword;
      s1     : AnsiString;
      ws1    : array [0..MAX_PATH] of WideChar;
  begin
    if firstLog then begin
      if GetVersion and $80000000 = 0 then
           doLog := GetFileAttributesW('c:\madCodeHook.txt') <> maxCard
      else doLog := GetFileAttributesA('c:\madCodeHook.txt') <> maxCard;
      firstLog := false;
    end;
    if doLog then begin
      GetLocalTime(st);
      time := IntToStrEx(dword(st.wHour),         2) + ':' +
              IntToStrEx(dword(st.wMinute),       2) + ':' +
              IntToStrEx(dword(st.wSecond),       2) + '-' +
              IntToStrEx(dword(st.wMilliseconds), 3) + ' ';
      if GetVersion and $80000000 = 0 then begin
        GetModuleFileNameW(0, ws1, MAX_PATH);
        s1 := WideToAnsiEx(ws1);
      end else begin
        SetLength(s1, MAX_PATH + 1);
        GetModuleFileNameA(0, PAnsiChar(s1), MAX_PATH);
        s1 := PAnsiChar(s1);
      end;
      s1 := ExtractFileName(s1);
      s1 := time + IntToHexEx(GetCurrentProcessID, 8) + ' ' + s1 + ' ';
      ReplaceStr(str, #$D#$A, #$D#$A + s1);
      str := s1 + str + #$D#$A;
      if paramW <> nil then
           s1 := WideToAnsiEx(paramW)
      else s1 := paramA;
      ReplaceStr(str, '%1', s1);
      if ph <> 0 then begin
        SetLength(s1, MAX_PATH + 1);
        if not ProcessIdToFileNameA(ProcessHandleToId(ph), PAnsiChar(s1)) then
          s1 := '';
        ReplaceStr(str, '%p', 'ph:' + IntToHexEx(ph) + ';pid:' + PAnsiChar(s1));
      end;
      mutex := CreateMutexW(nil, false, 'mAHLogMutex');
      if mutex <> 0 then begin
        WaitForSingleObject(mutex, INFINITE);
        try
          if GetVersion and $80000000 = 0 then
               c1 := CreateFileW('c:\madCodeHook.txt', GENERIC_READ or GENERIC_WRITE, 0, nil, OPEN_EXISTING, 0, 0)
          else c1 := CreateFileA('c:\madCodeHook.txt', GENERIC_READ or GENERIC_WRITE, 0, nil, OPEN_EXISTING, 0, 0);
          if c1 <> INVALID_HANDLE_VALUE then begin
            SetFilePointer(c1, 0, nil, FILE_END);
            WriteFile(c1, pointer(str)^, length(str), c2, nil);
            CloseHandle(c1);
          end;
        finally ReleaseMutex(mutex) end;
        CloseHandle(mutex);
      end;
    end;
  end;
{$endif}

// ***************************************************************

{$ifdef protect}
  {$include protect.inc}
{$endif}

// ***************************************************************

{$ifdef dynamic}

var
  Is64bitOSProc                  : function : bool; stdcall;
  Is64bitProcessProc             : function (processHandle: dword) : bool; stdcall;
  SetMadCHookOptionProc          : function (option: dword; param: PWideChar) : bool; stdcall;
  HookCodeProc                   : function (code         : pointer;
                                             callbackFunc : pointer;
                                             out nextHook : pointer;
                                             flags        : dword = 0) : bool; stdcall;
  HookAPIProc                    : function (module, api  : PAnsiChar;
                                             callbackFunc : pointer;
                                             out nextHook : pointer;
                                             flags        : dword = 0) : bool; stdcall;
  RenewHookProc                  : function (var nextHook: pointer) : bool; stdcall;
  IsHookInUseProc                : function (var nextHook: pointer) : integer; stdcall;
  UnhookCodeProc                 : function (var nextHook: pointer) : bool; stdcall;
  UnhookAPIProc                  : function (var nextHook: pointer) : bool; stdcall;
  CollectHooksProc               : procedure;
  FlushHooksProc                 : procedure;
  RestoreCodeProc                : function (code: pointer) : bool; stdcall;
  CreateProcessExAProc           : function (applicationName, commandLine : PAnsiChar;
                                             processAttr, threadAttr      : PSecurityAttributes;
                                             inheritHandles               : bool;
                                             creationFlags                : dword;
                                             environment                  : pointer;
                                             currentDirectory             : PAnsiChar;
                                             const startupInfo            : {$ifdef UNICODE} TStartupInfoA; {$else} TStartupInfo; {$endif}
                                             var processInfo              : TProcessInformation;
                                             loadLibrary                  : PAnsiChar          ) : bool; stdcall;
  CreateProcessExWProc           : function (applicationName, commandLine : PWideChar;
                                             processAttr, threadAttr      : PSecurityAttributes;
                                             inheritHandles               : bool;
                                             creationFlags                : dword;
                                             environment                  : pointer;
                                             currentDirectory             : PWideChar;
                                             const startupInfo            : {$ifdef UNICODE} TStartupInfoW; {$else} TStartupInfo; {$endif}
                                             var processInfo              : TProcessInformation;
                                             loadLibrary                  : PWideChar          ) : bool; stdcall;
  InjectLibraryAProc             : function (libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
  InjectLibraryWProc             : function (libFileName: PWideChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
  UninjectLibraryAProc           : function (libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
  UninjectLibraryWProc           : function (libFileName: PWideChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
  InjectLibrarySystemWideAProc   : function (driverName, libFileName: PAnsiChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PAnsiChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall;
  InjectLibrarySystemWideWProc   : function (driverName, libFileName: PWideChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PWideChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall;
  UninjectLibrarySystemWideAProc : function (driverName, libFileName: PAnsiChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PAnsiChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall;
  UninjectLibrarySystemWideWProc : function (driverName, libFileName: PWideChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PWideChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall;
  UninjectAllLibrariesWProc      : function (driverName: PWideChar; excludePIDs: TPCardinal = nil; timeOutPerUninject: dword = 7000) : bool; stdcall;
  UninjectAllLibrariesAProc      : function (driverName: PAnsiChar; excludePIDs: TPCardinal = nil; timeOutPerUninject: dword = 7000) : bool; stdcall;
  InstallInjectionDriverProc     : function (driverName, fileName32bit, fileName64bit, description: PWideChar) : bool; stdcall;
  UninstallInjectionDriverProc   : function (driverName: PWideChar) : bool; stdcall;
  LoadInjectionDriverProc        : function (driverName, fileName32bit, fileName64bit: PWideChar) : bool; stdcall;
  StopInjectionDriverProc        : function (driverName: PWideChar) : bool; stdcall;
  StartInjectionDriverProc       : function (driverName: PWideChar) : bool; stdcall;
  IsInjectionDriverInstalledProc : function (driverName: PWideChar) : bool; stdcall;
  IsInjectionDriverRunningProc   : function (driverName: PWideChar) : bool; stdcall;
  SetInjectionMethodProc         : function (driverName: PWideChar; newInjectionMethod: bool) : bool; stdcall;
  ProcessIdToFileNameAProc       : function (processId: dword; fileName: PAnsiChar; bufLenInChars: word) : bool; stdcall;
  ProcessIdToFileNameWProc       : function (processId: dword; fileName: PWideChar; bufLenInChars: word) : bool; stdcall;
  GetCurrentSessionIdProc        : function : dword; stdcall;
  GetInputSessionIdProc          : function : dword; stdcall;
  GetCallingModuleProc           : function : dword; stdcall;
  CreateGlobalMutexProc          : function (name: PAnsiChar) : dword; stdcall;
  OpenGlobalMutexProc            : function (name: PAnsiChar) : dword; stdcall;
  CreateGlobalEventProc          : function (name: PAnsiChar; manual, initialState: bool) : dword; stdcall;
  OpenGlobalEventProc            : function (name: PAnsiChar) : dword; stdcall;
  CreateGlobalFileMappingProc    : function (name: PAnsiChar; size: dword) : dword; stdcall;
  OpenGlobalFileMappingProc      : function (name: PAnsiChar; write: bool) : dword; stdcall;
  AnsiToWideProc                 : procedure (ansi: PAnsiChar; wide: PWideChar); stdcall;
  WideToAnsiProc                 : procedure (wide: PWideChar; ansi: PAnsiChar); stdcall;
  CreateIpcQueueExProc           : function (ipc            : PAnsiChar;
                                             callback       : TIpcCallback;
                                             maxThreadCount : dword = 16;
                                             maxQueueLen    : dword = $1000) : bool; stdcall;
  CreateIpcQueueProc             : function (ipc      : PAnsiChar;
                                             callback : TIpcCallback) : bool; stdcall;
  SendIpcMessageProc             : function (ipc            : PAnsiChar;
                                             messageBuf     : pointer;
                                             messageLen     : dword;
                                             answerBuf      : pointer = nil;
                                             answerLen      : dword   = 0;
                                             answerTimeOut  : dword   = INFINITE;
                                             handleMessages : bool    = true    ) : bool; stdcall;
  DestroyIpcQueueProc            : function (ipc: PAnsiChar) : bool; stdcall;
  AddAccessForEveryoneProc       : function (processOrService, access: dword) : bool; stdcall;
  AutoUnhook                     : procedure (moduleHandle: dword); stdcall;

function Is64bitOS : bool; stdcall;
begin
  result := GetMadCHookApi('Is64bitOS', @Is64bitOSProc) and
            Is64bitOSProc;
end;

function Is64bitProcess(processHandle: dword) : bool; stdcall;
begin
  result := GetMadCHookApi('Is64bitProcess', @Is64bitProcessProc) and
            Is64bitProcessProc(processHandle);
end;

function SetMadCHookOption(option: dword; param: PWideChar) : bool; stdcall;
begin
  result := GetMadCHookApi('SetMadCHookOption', @SetMadCHookOptionProc) and
            SetMadCHookOptionProc(option, param);
end;

function HookCode(code         : pointer;
                  callbackFunc : pointer;
                  out nextHook : pointer;
                  flags        : dword = 0) : bool; stdcall;
begin
  result := GetMadCHookApi('HookCode', @HookCodeProc) and
            HookCodeProc(code, callbackFunc, nextHook, flags);
end;

function HookAPI(module, api  : PAnsiChar;
                 callbackFunc : pointer;
                 out nextHook : pointer;
                 flags        : dword = 0) : bool; stdcall;
begin
  result := GetMadCHookApi('HookAPI', @HookAPIProc) and
            HookAPIProc(module, api, callbackFunc, nextHook, flags);
end;

function RenewHook(var nextHook: pointer) : bool; stdcall;
begin
  result := GetMadCHookApi('RenewHook', @RenewHookProc) and
            RenewHookProc(nextHook);
end;

function IsHookInUse(var nextHook: pointer) : integer; stdcall;
begin
  if GetMadCHookApi('IsHookInUse', @IsHookInUseProc) then
       result := IsHookInUseProc(nextHook)
  else result := 0;
end;

function UnhookCode(var nextHook: pointer) : bool; stdcall;
begin
  result := GetMadCHookApi('UnhookCode', @UnhookCodeProc) and
            UnhookCodeProc(nextHook);
end;

function UnhookAPI(var nextHook: pointer) : bool; stdcall;
begin
  result := GetMadCHookApi('UnhookAPI', @UnhookAPIProc) and
            UnhookAPIProc(nextHook);
end;

procedure CollectHooks;
begin
  if GetMadCHookApi('CollectHooks', @CollectHooksProc) then
    CollectHooksProc;
end;

procedure FlushHooks;
begin
  if GetMadCHookApi('FlushHooks', @FlushHooksProc) then
    FlushHooksProc;
end;

function RestoreCode(code: pointer) : bool; stdcall;
begin
  result := GetMadCHookApi('RestoreCode', @RestoreCodeProc) and
            RestoreCodeProc(code);
end;

function CreateProcessExA(applicationName, commandLine : PAnsiChar;
                          processAttr, threadAttr      : PSecurityAttributes;
                          inheritHandles               : bool;
                          creationFlags                : dword;
                          environment                  : pointer;
                          currentDirectory             : PAnsiChar;
                          const startupInfo            : {$ifdef UNICODE} TStartupInfoA; {$else} TStartupInfo; {$endif}
                          var processInfo              : TProcessInformation;
                          loadLibrary                  : PAnsiChar          ) : bool; stdcall;
begin
  result := GetMadCHookApi('CreateProcessExA', @CreateProcessExAProc) and
            CreateProcessExAProc(applicationName, commandLine, processAttr, threadAttr,
                                 inheritHandles, creationFlags, environment, currentDirectory,
                                 startupInfo, processInfo, loadLibrary);
end;

function CreateProcessExW(applicationName, commandLine : PWideChar;
                          processAttr, threadAttr      : PSecurityAttributes;
                          inheritHandles               : bool;
                          creationFlags                : dword;
                          environment                  : pointer;
                          currentDirectory             : PWideChar;
                          const startupInfo            : {$ifdef UNICODE} TStartupInfoW; {$else} TStartupInfo; {$endif}
                          var processInfo              : TProcessInformation;
                          loadLibrary                  : PWideChar          ) : bool; stdcall;
begin
  result := GetMadCHookApi('CreateProcessExW', @CreateProcessExWProc) and
            CreateProcessExWProc(applicationName, commandLine, processAttr, threadAttr,
                                 inheritHandles, creationFlags, environment, currentDirectory,
                                 startupInfo, processInfo, loadLibrary);
end;

function CreateProcessEx(applicationName, commandLine : PAnsiChar;
                         processAttr, threadAttr      : PSecurityAttributes;
                         inheritHandles               : bool;
                         creationFlags                : dword;
                         environment                  : pointer;
                         currentDirectory             : PAnsiChar;
                         const startupInfo            : {$ifdef UNICODE} TStartupInfoA; {$else} TStartupInfo; {$endif}
                         var processInfo              : TProcessInformation;
                         loadLibrary                  : AnsiString         ) : boolean;
begin
  result := GetMadCHookApi('CreateProcessExA', @CreateProcessExAProc) and
            CreateProcessExAProc(applicationName, commandLine, processAttr, threadAttr,
                                 inheritHandles, creationFlags, environment, currentDirectory,
                                 startupInfo, processInfo, PAnsiChar(loadLibrary));
end;

function InjectLibraryA(libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall; overload;
begin
  result := GetMadCHookApi('InjectLibraryA', @InjectLibraryAProc) and
            InjectLibraryAProc(libFileName, processHandle, timeOut);
end;

function InjectLibraryW(libFileName: PWideChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall; overload;
begin
  result := GetMadCHookApi('InjectLibraryW', @InjectLibraryWProc) and
            InjectLibraryWProc(libFileName, processHandle, timeOut);
end;

function InjectLibrary(libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall; overload;
begin
  result := GetMadCHookApi('InjectLibraryA', @InjectLibraryAProc) and
            InjectLibraryAProc(libFileName, processHandle, timeOut);
end;

function UninjectLibraryA(libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall; overload;
begin
  result := GetMadCHookApi('UninjectLibraryA', @UninjectLibraryAProc) and
            UninjectLibraryAProc(libFileName, processHandle, timeOut);
end;

function UninjectLibraryW(libFileName: PWideChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall; overload;
begin
  result := GetMadCHookApi('UninjectLibraryW', @UninjectLibraryWProc) and
            UninjectLibraryWProc(libFileName, processHandle, timeOut);
end;

function UninjectLibrary(libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall; overload;
begin
  result := GetMadCHookApi('UninjectLibraryA', @UninjectLibraryAProc) and
            UninjectLibraryAProc(libFileName, processHandle, timeOut);
end;

function InjectLibraryA(driverName, libFileName: PAnsiChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PAnsiChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall; overload;
begin
  result := GetMadCHookApi('InjectLibrarySystemWideA', @InjectLibrarySystemWideAProc) and
            InjectLibrarySystemWideAProc(driverName, libFileName, session, systemProcesses, includeMask, excludeMask, excludePIDs, timeOut);
end;

function InjectLibraryW(driverName, libFileName: PWideChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PWideChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall; overload;
begin
  result := GetMadCHookApi('InjectLibrarySystemWideW', @InjectLibrarySystemWideWProc) and
            InjectLibrarySystemWideWProc(driverName, libFileName, session, systemProcesses, includeMask, excludeMask, excludePIDs, timeOut);
end;

function InjectLibrary(driverName, libFileName: PAnsiChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PAnsiChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall; overload;
begin
  result := GetMadCHookApi('InjectLibrarySystemWideA', @InjectLibrarySystemWideAProc) and
            InjectLibrarySystemWideAProc(driverName, libFileName, session, systemProcesses, includeMask, excludeMask, excludePIDs, timeOut);
end;

function UninjectLibraryA(driverName, libFileName: PAnsiChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PAnsiChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall; overload;
begin
  result := GetMadCHookApi('UninjectLibrarySystemWideA', @UninjectLibrarySystemWideAProc) and
            UninjectLibrarySystemWideAProc(driverName, libFileName, session, systemProcesses, includeMask, excludeMask, excludePIDs, timeOut);
end;

function UninjectLibraryW(driverName, libFileName: PWideChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PWideChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall; overload;
begin
  result := GetMadCHookApi('UninjectLibrarySystemWideW', @UninjectLibrarySystemWideWProc) and
            UninjectLibrarySystemWideWProc(driverName, libFileName, session, systemProcesses, includeMask, excludeMask, excludePIDs, timeOut);
end;

function UninjectLibrary(driverName, libFileName: PAnsiChar; session: dword; systemProcesses: bool; includeMask, excludeMask: PAnsiChar; excludePIDs: TPCardinal; timeOut: dword) : bool; stdcall; overload;
begin
  result := GetMadCHookApi('UninjectLibrarySystemWideA', @UninjectLibrarySystemWideAProc) and
            UninjectLibrarySystemWideAProc(driverName, libFileName, session, systemProcesses, includeMask, excludeMask, excludePIDs, timeOut);
end;

function UninjectAllLibrariesW(driverName: PWideChar; excludePIDs: TPCardinal = nil; timeOutPerUninject: dword = 7000) : bool; stdcall;
begin
  result := GetMadCHookApi('UninjectAllLibrariesW', @UninjectAllLibrariesWProc) and
            UninjectAllLibrariesWProc(driverName, excludePIDs, timeOutPerUninject);
end;

function UninjectAllLibrariesA(driverName: PAnsiChar; excludePIDs: TPCardinal = nil; timeOutPerUninject: dword = 7000) : bool; stdcall;
begin
  result := GetMadCHookApi('UninjectAllLibrariesA', @UninjectAllLibrariesAProc) and
            UninjectAllLibrariesAProc(driverName, excludePIDs, timeOutPerUninject);
end;

function UninjectAllLibraries(driverName: PAnsiChar; excludePIDs: TPCardinal = nil; timeOutPerUninject: dword = 7000) : bool; stdcall;
begin
  result := GetMadCHookApi('UninjectAllLibrariesA', @UninjectAllLibrariesAProc) and
            UninjectAllLibrariesAProc(driverName, excludePIDs, timeOutPerUninject);
end;

function InstallInjectionDriver(driverName, fileName32bit, fileName64bit, description: PWideChar) : bool; stdcall;
begin
  result := GetMadCHookApi('InstallInjectionDriver', @InstallInjectionDriverProc) and
            InstallInjectionDriverProc(driverName, fileName32bit, fileName64bit, description);
end;

function UninstallInjectionDriver(driverName: PWideChar) : bool; stdcall;
begin
  result := GetMadCHookApi('UninstallInjectionDriver', @UninstallInjectionDriverProc) and
            UninstallInjectionDriverProc(driverName);
end;

function LoadInjectionDriver(driverName, fileName32bit, fileName64bit: PWideChar) : bool; stdcall;
begin
  result := GetMadCHookApi('LoadInjectionDriver', @LoadInjectionDriverProc) and
            LoadInjectionDriverProc(driverName, fileName32bit, fileName64bit);
end;

function StopInjectionDriver(driverName: PWideChar) : bool; stdcall;
begin
  result := GetMadCHookApi('StopInjectionDriver', @StopInjectionDriverProc) and
            StopInjectionDriverProc(driverName);
end;

function StartInjectionDriver(driverName: PWideChar) : bool; stdcall;
begin
  result := GetMadCHookApi('StartInjectionDriver', @StartInjectionDriverProc) and
            StartInjectionDriverProc(driverName);
end;

function IsInjectionDriverInstalled(driverName: PWideChar) : bool; stdcall;
begin
  result := GetMadCHookApi('IsInjectionDriverInstalled', @IsInjectionDriverInstalledProc) and
            IsInjectionDriverInstalledProc(driverName);
end;

function IsInjectionDriverRunning(driverName: PWideChar) : bool; stdcall;
begin
  result := GetMadCHookApi('IsInjectionDriverRunning', @IsInjectionDriverRunningProc) and
            IsInjectionDriverRunningProc(driverName);
end;

function SetInjectionMethod(driverName: PWideChar; newInjectionMethod: bool) : bool; stdcall;
begin
  result := GetMadCHookApi('SetInjectionMethod', @SetInjectionMethodProc) and
            SetInjectionMethod(driverName, newInjectionMethod);
end;

function ProcessIdToFileNameA(processId: dword; fileName: PAnsiChar; bufLenInChars: word) : bool; stdcall;
begin
  result := GetMadCHookApi('ProcessIdToFileNameA', @ProcessIdToFileNameAProc) and
            ProcessIdToFileNameAProc(processId, fileName, bufLenInChars);
end;

function ProcessIdToFileNameW(processId: dword; fileName: PWideChar; bufLenInChars: word) : bool; stdcall;
begin
  result := GetMadCHookApi('ProcessIdToFileNameW', @ProcessIdToFileNameWProc) and
            ProcessIdToFileNameWProc(processId, fileName, bufLenInChars);
end;

function ProcessIdToFileName(processId: dword; var fileName: AnsiString) : boolean;
var buf : PWideChar;
begin
  buf := pointer(LocalAlloc(LPTR, 64 * 1024));
  result := GetMadCHookApi('ProcessIdToFileNameW', @ProcessIdToFileNameWProc) and
            ProcessIdToFileNameWProc(processId, buf, 32 * 1024);
  if result and (buf[0] <> #0) then begin
    SetLength(fileName, lstrlenW(buf));
    WideToAnsi(buf, PAnsiChar(fileName));
  end else
    fileName := '';
  LocalFree(dword(buf));
end;

function GetProcessSid(processHandle: dword; var saa: PSidAndAttributes) : boolean;
var token, size : dword;
begin
  result := false;
  if OpenProcessToken(processHandle, TOKEN_QUERY, token) then begin
    size := 0;
    GetTokenInformation(token, TokenUser, nil, 0, size);
    dword(saa) := LocalAlloc(LPTR, size * 2);
    if GetTokenInformation(token, TokenUser, saa, size * 2, size) then
         result := true
    else LocalFree(dword(saa));
    CloseHandle(token);
  end;
end;

function IsSystemProcess(processHandle: dword; sid: AnsiString) : boolean;
var saa   : PSidAndAttributes;
    error : dword;
begin
  error := GetLastError;
  result := GetVersion and $80000000 = 0;
  saa := nil;
  if result and ((sid <> '') or GetProcessSid(processHandle, saa)) then begin
    if ( (Length(sid) > 1) and (ord(sid[2]) > 1) ) or
       ( (saa <> nil) and (saa^.Sid <> nil) and (TPAByte(saa^.Sid)^[1] > 1) ) then
      result := false;
    if saa <> nil then
      LocalFree(dword(saa));
  end;
  SetLastError(error);
end;

function AmSystemProcess : bool; stdcall;
begin
  {$ifdef log}
    log('AmSystemProcess');
  {$endif}
  result := IsSystemProcess(GetCurrentProcess, '');
  {$ifdef log}
    log('AmSystemProcess -> ' + booleanToChar(result));
  {$endif}
end;

function AmUsingInputDesktop : bool; stdcall;
var s1, s2 : AnsiString;
    c1, c2 : dword;
    error  : dword;
begin
  error := GetLastError;
  {$ifdef log}
    log('AmUsingInputDesktop');
  {$endif}
  if GetVersion and $80000000 = 0 then begin
    SetLength(s1, MAX_PATH + 1);
    s1[1] := #0;
    c1 := OpenInputDesktop(0, false, READ_CONTROL or GENERIC_READ);
    GetUserObjectInformationA(c1, UOI_NAME, PAnsiChar(s1), MAX_PATH, c2);
    CloseDesktop(c1);
    s1 := PAnsiChar(s1);
    SetLength(s2, MAX_PATH + 1);
    s2[1] := #0;
    c1 := GetThreadDesktop(GetCurrentThreadId);
    GetUserObjectInformationA(c1, UOI_NAME, PAnsiChar(s2), MAX_PATH, c2);
    CloseDesktop(c1);
    s2 := PAnsiChar(s2);
    result := (s1 <> '') and (s1 = s2);
  end else
    result := true;
  {$ifdef log}
    log('AmUsingInputDesktop -> ' + booleanToChar(result));
  {$endif}
  SetLastError(error);
end;

function GetCurrentSessionId : dword; stdcall;
begin
  if GetMadCHookApi('GetCurrentSessionId', @GetCurrentSessionIdProc) then
       result := GetCurrentSessionIdProc
  else result := 0;
end;

function GetInputSessionId : dword; stdcall;
begin
  if GetMadCHookApi('GetInputSessionId', @GetInputSessionIdProc) then
       result := GetInputSessionIdProc
  else result := 0;
end;

function GetCallingModule : dword; stdcall;
begin
  if GetMadCHookApi('GetCallingModule', @GetCallingModuleProc) then
    asm
      jmp GetCallingModuleProc
    end;
  result := 0;
end;

function CreateGlobalMutex(name: PAnsiChar) : dword; stdcall;
begin
  if GetMadCHookApi('CreateGlobalMutex', @CreateGlobalMutexProc) then
       result := CreateGlobalMutexProc(name)
  else result := 0;
end;

function OpenGlobalMutex(name: PAnsiChar) : dword; stdcall;
begin
  if GetMadCHookApi('OpenGlobalMutex', @OpenGlobalMutexProc) then
       result := OpenGlobalMutexProc(name)
  else result := 0;
end;

function CreateGlobalEvent(name: PAnsiChar; manual, initialState: bool) : dword; stdcall;
begin
  if GetMadCHookApi('CreateGlobalEvent', @CreateGlobalEventProc) then
       result := CreateGlobalEventProc(name, manual, initialState)
  else result := 0;
end;

function OpenGlobalEvent(name: PAnsiChar) : dword; stdcall;
begin
  if GetMadCHookApi('OpenGlobalEvent', @OpenGlobalEventProc) then
       result := OpenGlobalEventProc(name)
  else result := 0;
end;

function CreateGlobalFileMapping(name: PAnsiChar; size: dword) : dword; stdcall;
begin
  if GetMadCHookApi('CreateGlobalFileMapping', @CreateGlobalFileMappingProc) then
       result := CreateGlobalFileMappingProc(name, size)
  else result := 0;
end;

function OpenGlobalFileMapping(name: PAnsiChar; write: bool) : dword; stdcall;
begin
  if GetMadCHookApi('OpenGlobalFileMapping', @OpenGlobalFileMappingProc) then
       result := OpenGlobalFileMappingProc(name, write)
  else result := 0;
end;

procedure AnsiToWide(ansi: PAnsiChar; wide: PWideChar); stdcall;
begin
  if GetMadCHookApi('AnsiToWide', @AnsiToWideProc) then
    AnsiToWideProc(ansi, wide);
end;

procedure WideToAnsi(wide: PWideChar; ansi: PAnsiChar); stdcall;
begin
  if GetMadCHookApi('WideToAnsi', @WideToAnsiProc) then
    WideToAnsiProc(wide, ansi);
end;

function CreateIpcQueueEx(ipc            : PAnsiChar;
                          callback       : TIpcCallback;
                          maxThreadCount : dword = 16;
                          maxQueueLen    : dword = $1000) : bool; stdcall;
begin
  result := GetMadCHookApi('CreateIpcQueueEx', @CreateIpcQueueExProc) and
            CreateIpcQueueExProc(ipc, callback, maxThreadCount, maxQueueLen);
end;

function CreateIpcQueue(ipc      : PAnsiChar;
                        callback : TIpcCallback) : bool; stdcall;
begin
  result := GetMadCHookApi('CreateIpcQueue', @CreateIpcQueueProc) and
            CreateIpcQueueProc(ipc, callback);
end;

function SendIpcMessage(ipc            : PAnsiChar;
                        messageBuf     : pointer;
                        messageLen     : dword;
                        answerBuf      : pointer = nil;
                        answerLen      : dword   = 0;
                        answerTimeOut  : dword   = INFINITE;
                        handleMessages : bool    = true    ) : bool; stdcall;
begin
  result := GetMadCHookApi('SendIpcMessage', @SendIpcMessageProc) and
            SendIpcMessageProc(ipc, messageBuf, messageLen, answerBuf, answerLen,
                               answerTimeOut, handleMessages);
end;

function DestroyIpcQueue(ipc: PAnsiChar) : bool; stdcall;
begin
  result := GetMadCHookApi('DestroyIpcQueue', @DestroyIpcQueueProc) and
            DestroyIpcQueueProc(ipc);
end;

function AddAccessForEveryone(processOrService, access: dword) : bool; stdcall;
begin
  result := GetMadCHookApi('AddAccessForEveryone', @AddAccessForEveryoneProc) and
            AddAccessForEveryoneProc(processOrService, access);
end;

initialization
  if (not GetMadCHookApi('GetCurrentSessionId', @GetCurrentSessionIdProc)) and
     (not AmSystemProcess) and AmUsingInputDesktop then begin
    CreateMutex(nil, false, 'mchDynamicWarning');
    if GetLastError <> ERROR_ALREADY_EXISTS then
      MessageBoxA(0, 'This is the evaluation version of madCodeHook.' + #$D#$A +
                     'You need to copy "madCHook.dll" to the system32 folder.',
                     'madCodeHook information...', MB_ICONINFORMATION);
  end;
finalization
  if GetMadCHookApi('AutoUnhook', @AutoUnhook) then
    AutoUnhook(HInstance);

{$else}

// ***************************************************************
// SetMadCHookOption options

var
  CustomInjDrvFile             : array [0..MAX_PATH] of WideChar;
  SecureMemoryMaps             : boolean = false;
  DisableChangedCodeCheck      : boolean = false;
  UseNewIpcLogic               : boolean = false;
  InternalIpcTimeout           : dword = 7000;
  VmWareInjectionMode          : boolean = false;
  RenewOverwrittenHooks        : boolean = false;
  InjectIntoRunningProcesses   : boolean = true;
  UninjectFromRunningProcesses : boolean = true;

// ***************************************************************
// encrypted strings

const
  // ntdll.dll
  CNtQueryInformationToken   : AnsiString = (* NtQueryInformationToken        *)  #$1B#$21#$04#$20#$30#$27#$2C#$1C#$3B#$33#$3A#$27#$38#$34#$21#$3C#$3A#$3B#$01#$3A#$3E#$30#$3B;
  CNtSetSystemInformation    : AnsiString = (* NtSetSystemInformation         *)  #$1B#$21#$06#$30#$21#$06#$2C#$26#$21#$30#$38#$1C#$3B#$33#$3A#$27#$38#$34#$21#$3C#$3A#$3B;
  CLdrInitializeThunk        : AnsiString = (* LdrInitializeThunk             *)  #$19#$31#$27#$1C#$3B#$3C#$21#$3C#$34#$39#$3C#$2F#$30#$01#$3D#$20#$3B#$3E;
  CLdrLoadDll                : AnsiString = (* LdrLoadDll                     *)  #$19#$31#$27#$19#$3A#$34#$31#$11#$39#$39;
  CLdrUnloadDll              : AnsiString = (* LdrUnloadDll                   *)  #$19#$31#$27#$00#$3B#$39#$3A#$34#$31#$11#$39#$39;
  CLdrGetDllHandle           : AnsiString = (* LdrGetDllHandle                *)  #$19#$31#$27#$12#$30#$21#$11#$39#$39#$1D#$34#$3B#$31#$39#$30;
  CNtLoadDriver              : AnsiString = (* NtLoadDriver                   *)  #$1B#$21#$19#$3A#$34#$31#$11#$27#$3C#$23#$30#$27;
  CNtUnloadDriver            : AnsiString = (* NtUnloadDriver                 *)  #$1B#$21#$00#$3B#$39#$3A#$34#$31#$11#$27#$3C#$23#$30#$27;
  CNtClose                   : AnsiString = (* NtClose                        *)  #$1B#$21#$16#$39#$3A#$26#$30;
  CNtCreateProcess           : AnsiString = (* NtCreateProcess                *)  #$1B#$21#$16#$27#$30#$34#$21#$30#$05#$27#$3A#$36#$30#$26#$26;
  CNtCreateProcessEx         : AnsiString = (* NtCreateProcessEx              *)  #$1B#$21#$16#$27#$30#$34#$21#$30#$05#$27#$3A#$36#$30#$26#$26#$10#$2D;
  CNtTestAlert               : AnsiString = (* NtTestAlert                    *)  #$1B#$21#$01#$30#$26#$21#$14#$39#$30#$27#$21;
  CRtlCreateUserProcess      : AnsiString = (* RtlCreateUserProcess           *)  #$07#$21#$39#$16#$27#$30#$34#$21#$30#$00#$26#$30#$27#$05#$27#$3A#$36#$30#$26#$26;
  CNtAllocateVirtualMemory   : AnsiString = (* NtAllocateVirtualMemory        *)  #$1B#$21#$14#$39#$39#$3A#$36#$34#$21#$30#$03#$3C#$27#$21#$20#$34#$39#$18#$30#$38#$3A#$27#$2C;
  CNtFreeVirtualMemory       : AnsiString = (* NtFreeVirtualMemory            *)  #$1B#$21#$13#$27#$30#$30#$03#$3C#$27#$21#$20#$34#$39#$18#$30#$38#$3A#$27#$2C;
  CNtQueryVirtualMemory      : AnsiString = (* NtQueryVirtualMemory           *)  #$1B#$21#$04#$20#$30#$27#$2C#$03#$3C#$27#$21#$20#$34#$39#$18#$30#$38#$3A#$27#$2C;
  CNtProtectVirtualMemory    : AnsiString = (* NtProtectVirtualMemory         *)  #$1B#$21#$05#$27#$3A#$21#$30#$36#$21#$03#$3C#$27#$21#$20#$34#$39#$18#$30#$38#$3A#$27#$2C;
  CNtReadVirtualMemory       : AnsiString = (* NtReadVirtualMemory            *)  #$1B#$21#$07#$30#$34#$31#$03#$3C#$27#$21#$20#$34#$39#$18#$30#$38#$3A#$27#$2C;
  CNtWriteVirtualMemory      : AnsiString = (* NtWriteVirtualMemory           *)  #$1B#$21#$02#$27#$3C#$21#$30#$03#$3C#$27#$21#$20#$34#$39#$18#$30#$38#$3A#$27#$2C;
  CNtOpenProcessToken        : AnsiString = (* NtOpenProcessToken             *)  #$1B#$21#$1A#$25#$30#$3B#$05#$27#$3A#$36#$30#$26#$26#$01#$3A#$3E#$30#$3B;
  CNtQueryInfoProcess        : AnsiString = (* NtQueryInformationProcess      *)  #$1B#$21#$04#$20#$30#$27#$2C#$1C#$3B#$33#$3A#$27#$38#$34#$21#$3C#$3A#$3B#$05#$27#$3A#$36#$30#$26#$26;
  CNtOpenSection             : AnsiString = (* NtOpenSection                  *)  #$1B#$21#$1A#$25#$30#$3B#$06#$30#$36#$21#$3C#$3A#$3B;
  CNtMapViewOfSection        : AnsiString = (* NtMapViewOfSection             *)  #$1B#$21#$18#$34#$25#$03#$3C#$30#$22#$1A#$33#$06#$30#$36#$21#$3C#$3A#$3B;
  CNtUnmapViewOfSection      : AnsiString = (* NtUnmapViewOfSection           *)  #$1B#$21#$00#$3B#$38#$34#$25#$03#$3C#$30#$22#$1A#$33#$06#$30#$36#$21#$3C#$3A#$3B;
  CRtlValidSid               : AnsiString = (* RtlValidSid                    *)  #$07#$21#$39#$03#$34#$39#$3C#$31#$06#$3C#$31;
  CRtlEqualSid               : AnsiString = (* RtlEqualSid                    *)  #$07#$21#$39#$10#$24#$20#$34#$39#$06#$3C#$31;
  CKiUserExceptionDispatcher : AnsiString = (* KiUserExceptionDispatcher      *)  #$1E#$3C#$00#$26#$30#$27#$10#$2D#$36#$30#$25#$21#$3C#$3A#$3B#$11#$3C#$26#$25#$34#$21#$36#$3D#$30#$27;
  CNtOpenThread              : AnsiString = (* NtOpenThread                   *)  #$1B#$21#$1A#$25#$30#$3B#$01#$3D#$27#$30#$34#$31;
  CNtCreatePort              : AnsiString = (* NtCreatePort                   *)  #$1B#$21#$16#$27#$30#$34#$21#$30#$05#$3A#$27#$21;
  CNtConnectPort             : AnsiString = (* NtConnectPort                  *)  #$1B#$21#$16#$3A#$3B#$3B#$30#$36#$21#$05#$3A#$27#$21;
  CNtReplyWaitReceivePort    : AnsiString = (* NtReplyWaitReceivePort         *)  #$1B#$21#$07#$30#$25#$39#$2C#$02#$34#$3C#$21#$07#$30#$36#$30#$3C#$23#$30#$05#$3A#$27#$21;
  CNtAcceptConnectPort       : AnsiString = (* NtAcceptConnectPort            *)  #$1B#$21#$14#$36#$36#$30#$25#$21#$16#$3A#$3B#$3B#$30#$36#$21#$05#$3A#$27#$21;
  CNtCompleteConnectPort     : AnsiString = (* NtCompleteConnectPort          *)  #$1B#$21#$16#$3A#$38#$25#$39#$30#$21#$30#$16#$3A#$3B#$3B#$30#$36#$21#$05#$3A#$27#$21;
  CRtlEnterCriticalSection   : AnsiString = (* RtlEnterCriticalSection        *)  #$07#$21#$39#$10#$3B#$21#$30#$27#$16#$27#$3C#$21#$3C#$36#$34#$39#$06#$30#$36#$21#$3C#$3A#$3B;
  CRtlLeaveCriticalSection   : AnsiString = (* RtlLeaveCriticalSection        *)  #$07#$21#$39#$19#$30#$34#$23#$30#$16#$27#$3C#$21#$3C#$36#$34#$39#$06#$30#$36#$21#$3C#$3A#$3B;
  CWineGetVersion            : AnsiString = (* wine_get_version               *)  #$22#$3C#$3B#$30#$0A#$32#$30#$21#$0A#$23#$30#$27#$26#$3C#$3A#$3B;
  
  // kernel32.dll
  CGetModuleHandleA          : AnsiString = (* GetModuleHandleA               *)  #$12#$30#$21#$18#$3A#$31#$20#$39#$30#$1D#$34#$3B#$31#$39#$30#$14;
  CLoadLibraryA              : AnsiString = (* LoadLibraryA                   *)  #$19#$3A#$34#$31#$19#$3C#$37#$27#$34#$27#$2C#$14;
  CLoadLibraryExA            : AnsiString = (* LoadLibraryExA                 *)  #$19#$3A#$34#$31#$19#$3C#$37#$27#$34#$27#$2C#$10#$2D#$14;
  CLoadLibraryExW            : AnsiString = (* LoadLibraryExW                 *)  #$19#$3A#$34#$31#$19#$3C#$37#$27#$34#$27#$2C#$10#$2D#$02;
  CFreeLibrary               : AnsiString = (* FreeLibrary                    *)  #$13#$27#$30#$30#$19#$3C#$37#$27#$34#$27#$2C;
  CGetLastError              : AnsiString = (* GetLastError                   *)  #$12#$30#$21#$19#$34#$26#$21#$10#$27#$27#$3A#$27;
  CVirtualFree               : AnsiString = (* VirtualFree                    *)  #$03#$3C#$27#$21#$20#$34#$39#$13#$27#$30#$30;
  CCreateMutexA              : AnsiString = (* CreateMutexA                   *)  #$16#$27#$30#$34#$21#$30#$18#$20#$21#$30#$2D#$14;
  CGetModuleFileNameA        : AnsiString = (* GetModuleFileNameA             *)  #$12#$30#$21#$18#$3A#$31#$20#$39#$30#$13#$3C#$39#$30#$1B#$34#$38#$30#$14;
  COpenFileMappingA          : AnsiString = (* OpenFileMappingA               *)  #$1A#$25#$30#$3B#$13#$3C#$39#$30#$18#$34#$25#$25#$3C#$3B#$32#$14;
  COpenFileMappingW          : AnsiString = (* OpenFileMappingW               *)  #$1A#$25#$30#$3B#$13#$3C#$39#$30#$18#$34#$25#$25#$3C#$3B#$32#$02;
  CMapViewOfFile             : AnsiString = (* MapViewOfFile                  *)  #$18#$34#$25#$03#$3C#$30#$22#$1A#$33#$13#$3C#$39#$30;
  CUnmapViewOfFile           : AnsiString = (* UnmapViewOfFile                *)  #$00#$3B#$38#$34#$25#$03#$3C#$30#$22#$1A#$33#$13#$3C#$39#$30;
  CReleaseMutex              : AnsiString = (* ReleaseMutex                   *)  #$07#$30#$39#$30#$34#$26#$30#$18#$20#$21#$30#$2D;
  CSetLastError              : AnsiString = (* SetLastError                   *)  #$06#$30#$21#$19#$34#$26#$21#$10#$27#$27#$3A#$27;
  CLocalAlloc                : AnsiString = (* LocalAlloc                     *)  #$19#$3A#$36#$34#$39#$14#$39#$39#$3A#$36;
  CLocalFree                 : AnsiString = (* LocalFree                      *)  #$19#$3A#$36#$34#$39#$13#$27#$30#$30;
  CVirtualQuery              : AnsiString = (* VirtualQuery                   *)  #$03#$3C#$27#$21#$20#$34#$39#$04#$20#$30#$27#$2C;
  CWaitForMultipleObjects    : AnsiString = (* WaitForMultipleObjects         *)  #$02#$34#$3C#$21#$13#$3A#$27#$18#$20#$39#$21#$3C#$25#$39#$30#$1A#$37#$3F#$30#$36#$21#$26;
  CCreateProcessInternalW    : AnsiString = (* CreateProcessInternalW         *)  #$16#$27#$30#$34#$21#$30#$05#$27#$3A#$36#$30#$26#$26#$1C#$3B#$21#$30#$27#$3B#$34#$39#$02;
  CCreateProcessW            : AnsiString = (* CreateProcessW                 *)  #$16#$27#$30#$34#$21#$30#$05#$27#$3A#$36#$30#$26#$26#$02;
  CLoadLibraryW              : AnsiString = (* LoadLibraryW                   *)  #$19#$3A#$34#$31#$19#$3C#$37#$27#$34#$27#$2C#$02;
  CGetModuleHandleW          : AnsiString = (* GetModuleHandleW               *)  #$12#$30#$21#$18#$3A#$31#$20#$39#$30#$1D#$34#$3B#$31#$39#$30#$02;
  CGetInputSessionId         : AnsiString = (* WTSGetActiveConsoleSessionId   *)  #$02#$01#$06#$12#$30#$21#$14#$36#$21#$3C#$23#$30#$16#$3A#$3B#$26#$3A#$39#$30#$06#$30#$26#$26#$3C#$3A#$3B#$1C#$31;
  CFlushInstructionCache     : AnsiString = (* FlushInstructionCache          *)  #$13#$39#$20#$26#$3D#$1C#$3B#$26#$21#$27#$20#$36#$21#$3C#$3A#$3B#$16#$34#$36#$3D#$30;
  CAddVecExceptHandler       : AnsiString = (* AddVectoredExceptionHandler    *)  #$14#$31#$31#$03#$30#$36#$21#$3A#$27#$30#$31#$10#$2D#$36#$30#$25#$21#$3C#$3A#$3B#$1D#$34#$3B#$31#$39#$30#$27;
  CRemoveVecExceptHandler    : AnsiString = (* RemoveVectoredExceptionHandler *)  #$07#$30#$38#$3A#$23#$30#$03#$30#$36#$21#$3A#$27#$30#$31#$10#$2D#$36#$30#$25#$21#$3C#$3A#$3B#$1D#$34#$3B#$31#$39#$30#$27;
  CIsWow64Process            : AnsiString = (* IsWow64Process                 *)  #$1C#$26#$02#$3A#$22#$63#$61#$05#$27#$3A#$36#$30#$26#$26;
  CGetNativeSystemInfo       : AnsiString = (* GetNativeSystemInfo            *)  #$12#$30#$21#$1B#$34#$21#$3C#$23#$30#$06#$2C#$26#$21#$30#$38#$1C#$3B#$33#$3A;
  CQueryFullProcessImageName : AnsiString = (* QueryFullProcessImageName      *)  #$04#$20#$30#$27#$2C#$13#$20#$39#$39#$05#$27#$3A#$36#$30#$26#$26#$1C#$38#$34#$32#$30#$1B#$34#$38#$30#$02;
  CNtWow64QueryInfoProcess64 : AnsiString = (* NtWow64QueryInformationProcess64 *)  #$1B#$21#$02#$3A#$22#$63#$61#$04#$20#$30#$27#$2C#$1C#$3B#$33#$3A#$27#$38#$34#$21#$3C#$3A#$3B#$05#$27#$3A#$36#$30#$26#$26#$63#$61;
  CNtWow64ReadVirtualMemory64: AnsiString = (* NtWow64ReadVirtualMemory64     *)  #$1B#$21#$02#$3A#$22#$63#$61#$07#$30#$34#$31#$03#$3C#$27#$21#$20#$34#$39#$18#$30#$38#$3A#$27#$2C#$63#$61;

  // advapi32.dll
  CAdvApi32                  : AnsiString = (* advapi32.dll                   *)  #$34#$31#$23#$34#$25#$3C#$66#$67#$7B#$31#$39#$39;
  CGetSecurityInfo           : AnsiString = (* GetSecurityInfo                *)  #$12#$30#$21#$06#$30#$36#$20#$27#$3C#$21#$2C#$1C#$3B#$33#$3A;
  CSetSecurityInfo           : AnsiString = (* SetSecurityInfo                *)  #$06#$30#$21#$06#$30#$36#$20#$27#$3C#$21#$2C#$1C#$3B#$33#$3A;
  CSetEntriesInAclA          : AnsiString = (* SetEntriesInAclA               *)  #$06#$30#$21#$10#$3B#$21#$27#$3C#$30#$26#$1C#$3B#$14#$36#$39#$14;
  CCloseServiceHandle        : AnsiString = (* CloseServiceHandle             *)  #$16#$39#$3A#$26#$30#$06#$30#$27#$23#$3C#$36#$30#$1D#$34#$3B#$31#$39#$30;
  CControlService            : AnsiString = (* ControlService                 *)  #$16#$3A#$3B#$21#$27#$3A#$39#$06#$30#$27#$23#$3C#$36#$30;
  CQueryServiceStatus        : AnsiString = (* QueryServiceStatus             *)  #$04#$20#$30#$27#$2C#$06#$30#$27#$23#$3C#$36#$30#$06#$21#$34#$21#$20#$26;
  CDeleteService             : AnsiString = (* DeleteService                  *)  #$11#$30#$39#$30#$21#$30#$06#$30#$27#$23#$3C#$36#$30;
  COpenSCManagerW            : AnsiString = (* OpenSCManagerW                 *)  #$1A#$25#$30#$3B#$06#$16#$18#$34#$3B#$34#$32#$30#$27#$02;
  COpenServiceW              : AnsiString = (* OpenServiceW                   *)  #$1A#$25#$30#$3B#$06#$30#$27#$23#$3C#$36#$30#$02;
  CStartServiceW             : AnsiString = (* StartServiceW                  *)  #$06#$21#$34#$27#$21#$06#$30#$27#$23#$3C#$36#$30#$02;
  CCreateServiceW            : AnsiString = (* CreateServiceW                 *)  #$16#$27#$30#$34#$21#$30#$06#$30#$27#$23#$3C#$36#$30#$02;
  CChangeServiceConfigW      : AnsiString = (* ChangeServiceConfigW           *)  #$16#$3D#$34#$3B#$32#$30#$06#$30#$27#$23#$3C#$36#$30#$16#$3A#$3B#$33#$3C#$32#$02;
  CQueryServiceConfigW       : AnsiString = (* QueryServiceConfigW            *)  #$04#$20#$30#$27#$2C#$06#$30#$27#$23#$3C#$36#$30#$16#$3A#$3B#$33#$3C#$32#$02;

  // user32.dll
  CCharLowerBuffW            : AnsiString = (* CharLowerBuffW                 *)  #$16#$3D#$34#$27#$19#$3A#$22#$30#$27#$17#$20#$33#$33#$02;

  // unicows.dll
  CUnicows                   : AnsiString = (* unicows.dll                    *)  #$20#$3B#$3C#$36#$3A#$22#$26#$7B#$31#$39#$39;

  // dll names
  CGdi32                     : AnsiString = (* gdi32.dll                      *)  #$32#$31#$3C#$66#$67#$7B#$31#$39#$39;
  CUser32                    : AnsiString = (* user32.dll                     *)  #$20#$26#$30#$27#$66#$67#$7B#$31#$39#$39;
  CTapi32                    : AnsiString = (* tapi32.dll                     *)  #$21#$34#$25#$3C#$66#$67#$7B#$31#$39#$39;
  CWs2_32                    : AnsiString = (* ws2_32.dll                     *)  #$22#$26#$67#$0A#$66#$67#$7B#$31#$39#$39;

  // other strings
  CSLSvc                     : AnsiString = (* slsvc.exe                      *)  #$26#$39#$26#$23#$36#$7B#$30#$2D#$30;
  CSeBackupPrivilege         : AnsiString = (* SeBackupPrivilege              *)  #$06#$30#$17#$34#$36#$3E#$20#$25#$05#$27#$3C#$23#$3C#$39#$30#$32#$30;
  CSeRestorePrivilege        : AnsiString = (* SeRestorePrivilege             *)  #$06#$30#$07#$30#$26#$21#$3A#$27#$30#$05#$27#$3C#$23#$3C#$39#$30#$32#$30;
  CSeTakeOwnershipPrivilege  : AnsiString = (* SeTakeOwnershipPrivilege       *)  #$06#$30#$01#$34#$3E#$30#$1A#$22#$3B#$30#$27#$26#$3D#$3C#$25#$05#$27#$3C#$23#$3C#$39#$30#$32#$30;
  CGlobal                    : AnsiString = (* Global\                        *)  #$12#$39#$3A#$37#$34#$39#$09;
  CSession                   : AnsiString = (* Session\                       *)  #$06#$30#$26#$26#$3C#$3A#$3B#$09;
  CBaseNamedObjects          : AnsiString = (* \BaseNamedObjects\             *)  #$09#$17#$34#$26#$30#$1B#$34#$38#$30#$31#$1A#$37#$3F#$30#$36#$21#$26#$09;
  CMutex                     : AnsiString = (* Mutex                          *)  #$18#$20#$21#$30#$2D;
  CEvent                     : AnsiString = (* Event                          *)  #$10#$23#$30#$3B#$21;
  CMap                       : AnsiString = (* Map                            *)  #$18#$34#$25;
  CShared                    : AnsiString = (* Shared                         *)  #$06#$3D#$34#$27#$30#$31;
  CProcess                   : AnsiString = (* Process                        *)  #$05#$27#$3A#$36#$30#$26#$26;
  CApi                       : AnsiString = (* API                            *)  #$14#$05#$1C;
  CNamedBuffer               : AnsiString = (* NamedBuffer                    *)  #$1B#$34#$38#$30#$31#$17#$20#$33#$33#$30#$27;
  CMixPrefix                 : AnsiString = (* mix                            *)  #$38#$3C#$2D;
  CMAHPrefix                 : AnsiString = (* mAH                            *)  #$38#$14#$1D;
  CMAHWatcherThread          : AnsiString = (* mAHWatcherThread               *)  #$38#$14#$1D#$02#$34#$21#$36#$3D#$30#$27#$01#$3D#$27#$30#$34#$31;
  CMAHStubs                  : AnsiString = (* mAHStubs                       *)  #$38#$14#$1D#$06#$21#$20#$37#$26;
  CMchPIT9x                  : AnsiString = (* mchPIT9x                       *)  #$38#$36#$3D#$05#$1C#$01#$6C#$2D;
  CMchMixCache               : AnsiString = (* mchMixCache                    *)  #$38#$36#$3D#$18#$3C#$2D#$16#$34#$36#$3D#$30;
  CMchPitRT                  : AnsiString = (* mchPitRT                       *)  #$38#$36#$3D#$05#$3C#$21#$07#$01;
  CMchLLEW2                  : AnsiString = (* mchLLEW2                       *)  #$38#$36#$3D#$19#$19#$10#$02#$67;
  CMchI9xMA                  : AnsiString = (* mchI9xMA                       *)  #$38#$36#$3D#$1C#$6C#$2D#$18#$14;
  CMchIInjT                  : AnsiString = (* mc4IInjT                       *)  #$38#$36#$61#$1C#$1C#$3B#$3F#$01;
  CMchInjDrvMap              : AnsiString = (* mc2InjDrvMap                   *)  #$38#$36#$67#$1C#$3B#$3F#$11#$27#$23#$18#$34#$25;
  CAutoUnhookMap             : AnsiString = (* AutoUnhookMap                  *)  #$14#$20#$21#$3A#$00#$3B#$3D#$3A#$3A#$3E#$18#$34#$25;
  CDllInjectName             : AnsiString = (* mc2SWDIJ                       *)  #$38#$36#$67#$06#$02#$11#$1C#$1F;
  CInjDrvName                : AnsiString = (* mchInjDrv                      *)  #$38#$36#$3D#$1C#$3B#$3F#$11#$27#$23;
  CInjDrvDescr               : AnsiString = (* madCodeHook DLL injection driver *)  #$38#$34#$31#$16#$3A#$31#$30#$1D#$3A#$3A#$3E#$75#$11#$19#$19#$75#$3C#$3B#$3F#$30#$36#$21#$3C#$3A#$3B#$75#$31#$27#$3C#$23#$30#$27;
  CDrivers                   : AnsiString = (* Drivers                        *)  #$11#$27#$3C#$23#$30#$27#$26;
  CSys                       : AnsiString = (* sys                            *)  #$26#$2C#$26;
  CVersion                   : AnsiString = (* Version                        *)  #$03#$30#$27#$26#$3C#$3A#$3B;
  CChecksum                  : AnsiString = (* Checksum                       *)  #$16#$3D#$30#$36#$3E#$26#$20#$38;
  CInstallCount              : AnsiString = (* InstallCount                   *)  #$1C#$3B#$26#$21#$34#$39#$39#$16#$3A#$20#$3B#$21;
  CType                      : AnsiString = (* Type                           *)  #$01#$2C#$25#$30;
  CErrorControl              : AnsiString = (* ErrorControl                   *)  #$10#$27#$27#$3A#$27#$16#$3A#$3B#$21#$27#$3A#$39;
  CStart                     : AnsiString = (* Start                          *)  #$06#$21#$34#$27#$21;
  CImagePath                 : AnsiString = (* ImagePath                      *)  #$1C#$38#$34#$32#$30#$05#$34#$21#$3D;
  CDeleteFlag                : AnsiString = (* DeleteFlag                     *)  #$11#$30#$39#$30#$21#$30#$13#$39#$34#$32;
  CSystemCcsServices         : AnsiString = (* SYSTEM\CurrentControlSet\Services *)  #$06#$0C#$06#$01#$10#$18#$09#$16#$20#$27#$27#$30#$3B#$21#$16#$3A#$3B#$21#$27#$3A#$39#$06#$30#$21#$09#$06#$30#$27#$23#$3C#$36#$30#$26;
  CRegistryMachine           : AnsiString = (* registry\machine               *)  #$27#$30#$32#$3C#$26#$21#$27#$2C#$09#$38#$34#$36#$3D#$3C#$3B#$30;
  CIpc                       : AnsiString = (* Ipc2                           *)  #$1C#$25#$36#$67;
  CAnswerBuf                 : AnsiString = (* AnswerBuf2                     *)  #$14#$3B#$26#$22#$30#$27#$17#$20#$33#$67;
  CCounter                   : AnsiString = (* Cnt                            *)  #$16#$3B#$21;
  CRpcControlIpc             : AnsiString = (* \RPC Control\mchIpc            *)  #$09#$07#$05#$16#$75#$16#$3A#$3B#$21#$27#$3A#$39#$09#$38#$36#$3D#$1C#$25#$36;
  {$ifndef unlocked}
    CEnumServicesStatusA       : AnsiString = (* EnumServicesStatusA          *)  #$10#$3B#$20#$38#$06#$30#$27#$23#$3C#$36#$30#$26#$06#$21#$34#$21#$20#$26#$14;
    CEnumServicesStatusW       : AnsiString = (* EnumServicesStatusW          *)  #$10#$3B#$20#$38#$06#$30#$27#$23#$3C#$36#$30#$26#$06#$21#$34#$21#$20#$26#$02;
    CNtQuerySystemInfo2        : AnsiString = (* NtQuerySystemInformation     *)  #$1B#$21#$04#$20#$30#$27#$2C#$06#$2C#$26#$21#$30#$38#$1C#$3B#$33#$3A#$27#$38#$34#$21#$3C#$3A#$3B;
    CProcess32Next2            : AnsiString = (* Process32Next                *)  #$05#$27#$3A#$36#$30#$26#$26#$66#$67#$1B#$30#$2D#$21;
    CEnumServicesStatusA2      : AnsiString = (* EnumServicesStatusA          *)  #$10#$3B#$20#$38#$06#$30#$27#$23#$3C#$36#$30#$26#$06#$21#$34#$21#$20#$26#$14;
    CEnumServicesStatusW2      : AnsiString = (* EnumServicesStatusW          *)  #$10#$3B#$20#$38#$06#$30#$27#$23#$3C#$36#$30#$26#$06#$21#$34#$21#$20#$26#$02;
  {$endif}

// ***************************************************************

{$ifdef CheckRecursion}
  function NotRecursedYet(caller, buf: pointer) : boolean;

    function GetStackAddr : dword;
    // ask the current thread's current stack pointer
    asm
      mov eax, ebp
    end;

    function GetStackTop : dword;
    // ask the current thread's stack starting address
    asm
      mov eax, fs:[4]
    end;

  var stackAddr : dword;
      stackTop  : dword;
      count     : integer;
  begin
    stackAddr := GetStackAddr;
    stackTop := GetStackTop;
    count := 0;
    result := true;
    if (stackAddr + 16 < stackTop) and (not IsBadReadPtr(pointer(stackAddr), stackTop - stackAddr)) then
      while stackAddr + 16 < stackTop do begin
        if (TPACardinal(stackAddr)[0] = $77777777) and
           (TPACardinal(stackAddr)[1] = stackAddr) and
           (TPAPointer (stackAddr)[2] = caller   ) and
           (TPACardinal(stackAddr)[3] = stackAddr xor dword(caller)) then begin
          if stackAddr = stackTop then
            break;
          inc(count);
          if count = 7 then begin
            result := false;
            break;
          end;
        end;
        inc(stackAddr, 4);
      end;
    if result then begin
      TPACardinal(buf)[0] := $77777777;
      TPACardinal(buf)[1] := dword(buf);
      TPACardinal(buf)[2] := dword(caller);
      TPACardinal(buf)[3] := TPACardinal(buf)[1] xor TPACardinal(buf)[2];
    end;
  end;
{$endif}

// ***************************************************************

type
  TUnicodeStr = packed record
    len, maxLen : word;
    str         : PWideChar;
  end;
  TUnicodeStr64 = packed record
    len, maxLen : word;
    dummy       : dword;
    str         : int64;
  end;
  TObjectAttributes = packed record
    length  : dword;
    rootDir : dword;
    name    : ^TUnicodeStr;
    attr    : dword;
    sd      : pointer;
    sqos    : pointer;
  end;

// ***************************************************************

procedure AnsiToWide(ansi: PAnsiChar; wide: PWideChar); stdcall;
var {$ifdef CheckRecursion}
      recTestBuf : array [0..3] of dword;
    {$endif}
    s1         : AnsiString;
    error      : dword;
begin
  error := GetLastError;
  {$ifdef CheckRecursion}
    if NotRecursedYet(@AnsiToWide, @recTestBuf) then begin
  {$endif}
    s1 := AnsiToWideEx(ansi);
    Move(PAnsiChar(s1)^, wide^, Length(s1));
    {$ifdef CheckRecursion}
      recTestBuf[3] := 0;
    end;
  {$endif}
  SetLastError(error);
end;

procedure WideToAnsi(wide: PWideChar; ansi: PAnsiChar); stdcall;
var {$ifdef CheckRecursion}
      recTestBuf : array [0..3] of dword;
    {$endif}
    s1         : AnsiString;
    error      : dword;
begin
  error := GetLastError;
  {$ifdef CheckRecursion}
    if NotRecursedYet(@WideToAnsi, @recTestBuf) then begin
  {$endif}
    s1 := WideToAnsiEx(wide);
    Move(PAnsiChar(s1)^, ansi^, Length(s1) + 1);
  {$ifdef CheckRecursion}
      recTestBuf[3] := 0;
    end;
  {$endif}
  SetLastError(error);
end;

function WideToWideEx(wide: PWideChar; addTerminatingNull: boolean = true) : AnsiString;
begin
  SetLength(result, lstrlenW(wide) * 2);
  if result <> '' then
    Move(wide^, pointer(result)^, Length(result));
  if addTerminatingNull then
    result := result + #0#0;
end;

function AnsiToWideEx2(const ansi: AnsiString; addTerminatingZero: boolean = true) : AnsiString;
var ws : UnicodeString;
begin
  if GetVersion and $80000000 = 0 then begin
    ws := UnicodeString(ansi);
    SetLength(result, Length(ws) * 2);
    if ws <> '' then
      Move(pointer(ws)^, pointer(result)^, Length(result));
    if addTerminatingZero then
      result := result + #0#0;
  end else
    result := AnsiToWideEx(ansi, addTerminatingZero);
end;

procedure AnsiToWide2(ansi: PAnsiChar; wide: PWideChar);
var s1 : AnsiString;
begin
  s1 := AnsiToWideEx2(ansi);
  Move(PAnsiChar(s1)^, wide^, Length(s1));
end;

function WideToAnsiEx2(wide: PWideChar) : AnsiString;
begin
  if GetVersion and $80000000 = 0 then
    result := AnsiString(UnicodeString(wide))
  else
    result := WideToAnsiEx(wide);
end;

// ***************************************************************

var SetErrorModeFunc           : function (mode: dword) : dword stdcall = nil;
    LoadLibraryAFunc           : function (lib: PAnsiChar) : dword stdcall = nil;
    LoadLibraryWFunc           : function (lib: PWideChar) : dword stdcall = nil;
    FreeLibraryFunc            : function (module: dword) : bool  stdcall = nil;
    GetModuleHandleAFunc       : function (lib: PAnsiChar) : dword stdcall = nil;
    GetModuleHandleWFunc       : function (lib: PWideChar) : dword stdcall = nil;
    GetLastErrorFunc           : function : integer stdcall = nil;
    VirtualFreeFunc            : function (address: pointer; size, flags: dword) : bool stdcall = nil;
    GetVersionFunc             : function : dword stdcall = nil;
    CreateMutexAFunc           : function (attr: PSecurityAttributes; initialOwner: bool; name: PAnsiChar) : dword stdcall = nil;
    GetModuleFileNameAFunc     : function (module: dword; fileName: PAnsiChar; size: dword) : dword stdcall = nil;
    WaitForSingleObjectFunc    : function (handle, timeOut: dword) : dword stdcall = nil;
    GetCurrentProcessIdFunc    : function : dword stdcall = nil;
    OpenFileMappingAFunc       : function (access: dword; inheritHandle: bool; name: PAnsiChar) : dword stdcall = nil;
    MapViewOfFileFunc          : function (map, access, ofsHi, ofsLo, size: dword) : pointer stdcall = nil;
    UnmapViewOfFileFunc        : function (addr: pointer) : bool stdcall = nil;
    CloseHandleFunc            : function (obj: dword) : bool stdcall = nil;
    ReleaseMutexFunc           : function (mutex: dword) : bool stdcall = nil;
    SetLastErrorProc           : procedure (error: dword) stdcall = nil;
    LocalAllocFunc             : function (flags, size: dword) : pointer stdcall = nil;
    LocalFreeFunc              : function (addr: pointer) : dword stdcall = nil;
    VirtualQueryFunc           : function (addr: pointer; var mbi: TMemoryBasicInformation; size: dword) : dword stdcall = nil;
    SleepProc                  : procedure (time: dword) stdcall = nil;
    WaitForMultipleObjectsFunc : function (count: dword; handles: PWOHandleArray; waitAll: bool; timeOut: dword) : dword stdcall = nil;
    SharedMem9x_Alloc          : function (size : dword  ) : pointer stdcall = nil;
    SharedMem9x_Free           : function (ptr  : pointer) : bool    stdcall = nil;
    EnterCriticalSectionFunc   : procedure (var criticalSection: TRTLCriticalSection); stdcall;
    LeaveCriticalSectionFunc   : procedure (var criticalSection: TRTLCriticalSection); stdcall;

procedure InitProcs(hookOnly: boolean);
begin
  if @GetModuleHandleAFunc = nil then
    if hookOnly and (GetVersion and $80000000 = 0) then begin
      if @GetModuleFileNameAFunc = nil then
        GetModuleFileNameAFunc   := KernelProc(CGetModuleFileNameA,      true);
      if @VirtualQueryFunc = nil then
        VirtualQueryFunc         := KernelProc(CVirtualQuery,            true);
    end else begin
      GetModuleHandleAFunc       := KernelProc(CGetModuleHandleA,        true);
      GetModuleHandleWFunc       := KernelProc(CGetModuleHandleW,        true);
      SetErrorModeFunc           := KernelProc(CSetErrorMode,            true);
      LoadLibraryAFunc           := KernelProc(CLoadLibraryA,            true);
      LoadLibraryWFunc           := KernelProc(CLoadLibraryW,            true);
      FreeLibraryFunc            := KernelProc(CFreeLibrary,             true);
      GetLastErrorFunc           := KernelProc(CGetLastError,            true);
      VirtualFreeFunc            := KernelProc(CVirtualFree,             true);
      GetVersionFunc             := KernelProc(CGetVersion,              true);
      CreateMutexAFunc           := KernelProc(CCreateMutexA,            true);
      GetModuleFileNameAFunc     := KernelProc(CGetModuleFileNameA,      true);
      WaitForSingleObjectFunc    := KernelProc(CWaitForSingleObject,     true);
      GetCurrentProcessIdFunc    := KernelProc(CGetCurrentProcessId,     true);
      OpenFileMappingAFunc       := KernelProc(COpenFileMappingA,        true);
      MapViewOfFileFunc          := KernelProc(CMapViewOfFile,           true);
      UnmapViewOfFileFunc        := KernelProc(CUnmapViewOfFile,         true);
      CloseHandleFunc            := KernelProc(CCloseHandle,             true);
      ReleaseMutexFunc           := KernelProc(CReleaseMutex,            true);
      SetLastErrorProc           := KernelProc(CSetLastError,            true);
      LocalAllocFunc             := KernelProc(CLocalAlloc,              true);
      LocalFreeFunc              := KernelProc(CLocalFree,               true);
      VirtualQueryFunc           := KernelProc(CVirtualQuery,            true);
      SleepProc                  := KernelProc(CSleep,                   true);
      WaitForMultipleObjectsFunc := KernelProc(CWaitForMultipleObjects,  true);
      EnterCriticalSectionFunc   :=     NtProc(CRtlEnterCriticalSection, true);
      LeaveCriticalSectionFunc   :=     NtProc(CRtlLeaveCriticalSection, true);
    end;
end;

// ***************************************************************

function AtomicMove(src, dst: pointer; len: integer) : boolean; // len = 1..8

  function IsCmpxchg8bSupported : boolean;
  asm
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 00200000h
    push eax
    popfd
    pushfd
    pop eax
    cmp eax, ecx
    mov eax, 0
    jz @quit
    mov eax, 1
    push ebx
    dw $A20F  // D4/D5 doesn't know the assembler instruction "cpuid"
    pop ebx
    test edx, $100
    mov eax, 0
    jz @quit
    mov eax, 1
    @quit:
  end;

  procedure cmpxchg8b;//(src, dst: pointer; len: integer); stdcall;
  asm
    add esp, -8

    push esi
    push edi
    push ebx
    push ebp

    mov ebp, [esp+$20]

    mov eax, ebp
    mov edx, [eax+4]
    mov eax, [eax]

   @tryAgain:
    mov [esp+$10], eax
    mov [esp+$14], edx

    mov esi, [esp+$1c]
    lea edi, [esp+$10]
    mov ecx, [esp+$24]
    cld
    rep movsb

    mov ebx, [esp+$10]
    mov ecx, [esp+$14]

    db $F0        // lock ...
    dd $004DC70F  // ...  cmpxchg8b qword ptr [ebp+$00]

    jnz @tryAgain

    pop ebp
    pop ebx
    pop edi
    pop esi

    add esp, 8
    ret $c
  end;

type TCmpxchg8b = procedure (src, dst: pointer; len: integer); stdcall;
begin
  result := (len > 0) and (len <= 8) and IsCmpxchg8bSupported;
  if result then
    TCmpxchg8b(@cmpxchg8b)(src, dst, len);
end;

var FVista : integer = -1;
function IsVista : boolean;
var viW : TOsVersionInfoW;
begin
  if FVista = -1 then
    if GetVersion and $80000000 = 0 then begin
      ZeroMemory(@viW, sizeOf(viW));
      viW.dwOSVersionInfoSize := sizeOf(viW);
      GetVersionExW(POsVersionInfo(@viW)^);
      FVista := ord(viW.dwMajorVersion >= 6);
    end else
      FVista := 0;
  result := FVista = 1;
end;

function GetProcessSid(processHandle: dword; var saa: PSidAndAttributes) : boolean;
var token, size : dword;
begin
  result := false;
  if OpenProcessToken(processHandle, TOKEN_QUERY, token) then begin
    size := 0;
    GetTokenInformation(token, TokenUser, nil, 0, size);
    dword(saa) := LocalAlloc(LPTR, size * 2);
    if GetTokenInformation(token, TokenUser, saa, size * 2, size) then
         result := true
    else LocalFree(dword(saa));
    CloseHandle(token);
  end;
end;

procedure InitSecAttr(var sa: TSecurityAttributes; var sd: TSecurityDescriptor; limited: boolean);
const
  CEveryoneSia : TSidIdentifierAuthority = (value: (0, 0, 0, 0, 0, 1));
  CSystemSia   : TSidIdentifierAuthority = (value: (0, 0, 0, 0, 0, 5));
type
  TTrustee = packed record
    multiple : pointer;
    multop   : dword;
    form     : dword;
    type_    : dword;
    sid      : PSid;
  end;
  TExplicitAccess = packed record
    access  : dword;
    mode    : dword;
    inherit : dword;
    trustee : TTrustee;
  end;
var seia       : function (count: dword; var ees: TExplicitAccess; oldAcl: PAcl; var newAcl: PAcl) : dword; stdcall;
    sid1, sid2 : PSid;
    dacl       : PAcl;
    saa        : PSidAndAttributes;
    ea         : array [0..2] of TExplicitAccess;
begin
  sid1 := nil;
  sid2 := nil;
  saa  := nil;
  dacl := nil;
  if limited and SecureMemoryMaps then begin
    seia := GetProcAddress(GetModuleHandleA(PAnsiChar(DecryptStr(CAdvApi32))), PAnsiChar(DecryptStr(CSetEntriesInAclA)));
    if @seia <> nil then begin
      if AllocateAndInitializeSid(CEveryoneSia, 1, 0,  0, 0, 0, 0, 0, 0, 0, sid1) and
         AllocateAndInitializeSid(CSystemSia,   1, 18, 0, 0, 0, 0, 0, 0, 0, sid2) and
         GetProcessSid(GetCurrentProcess, saa) and (saa <> nil) then begin
        ZeroMemory(@ea, sizeOf(ea));
        ea[0].mode          := 1; // GRANT_ACCESS
        ea[0].trustee.form  := 0; // TRUSTEE_IS_SID
        ea[0].trustee.type_ := 1; // TRUSTEE_IS_USER
        ea[1] := ea[0];
        ea[2] := ea[1];
        ea[0].access        := SECTION_MAP_READ;
        ea[1].access        := STANDARD_RIGHTS_ALL or SPECIFIC_RIGHTS_ALL;
        ea[2].access        := STANDARD_RIGHTS_ALL or SPECIFIC_RIGHTS_ALL;
        ea[0].trustee.sid   := sid1;
        ea[1].trustee.sid   := sid2;
        ea[2].trustee.sid   := saa.Sid;
        if seia(3, ea[0], nil, dacl) <> 0 then
          dacl := nil;
      end;
    end;
  end;
  sa.nLength := sizeOf(sa);
  sa.lpSecurityDescriptor := @sd;
  sa.bInheritHandle := false;
  InitializeSecurityDescriptor(@sd, SECURITY_DESCRIPTOR_REVISION);
  SetSecurityDescriptorDacl(@sd, true, dacl, false);
  if sid1 <> nil then
    FreeSid(sid1);
  if sid2 <> nil then
    FreeSid(sid2);
  if saa <> nil then
    LocalFree(dword(saa));
end;

function InternalCreateMutex(name: PAnsiChar; global: boolean) : dword;
var sa : TSecurityAttributes;
    sd : TSecurityDescriptor;
begin
  if GetVersion and $80000000 = 0 then begin
    InitSecAttr(sa, sd, false);
    if global then
      result := CreateMutexW(@sa, false, pointer(AnsiToWideEx(DecryptStr(CGlobal) + name)))
    else
      result := 0;
    if result = 0 then
      result := CreateMutexW(@sa, false, pointer(AnsiToWideEx(name)));
  end else
    result := CreateMutexA(nil, false, name);
end;

function CreateGlobalMutex(name: PAnsiChar) : dword; stdcall;
begin
  result := InternalCreateMutex(name, true);
end;

function CreateLocalMutex(name: PAnsiChar) : dword; stdcall;
begin
  result := InternalCreateMutex(name, false);
end;

function OpenGlobalMutex(name: PAnsiChar) : dword; stdcall;
begin
  if GetVersion and $80000000 = 0 then begin
    result := OpenMutexW(SYNCHRONIZE, false, pointer(AnsiToWideEx(DecryptStr(CGlobal) + name)));
    if result = 0 then
      result := OpenMutexW(SYNCHRONIZE, false, pointer(AnsiToWideEx(name)));
  end else
    result := OpenMutexA(SYNCHRONIZE, false, name);
end;

function InternalCreateFileMapping(name: PAnsiChar; size: dword; global: boolean) : dword;
var sa : TSecurityAttributes;
    sd : TSecurityDescriptor;
    le : dword;
begin
  if GetVersion and $80000000 = 0 then begin
    sd.Dacl := nil;
    InitSecAttr(sa, sd, true);
    if global then
      result := CreateFileMappingW(dword(-1), @sa, PAGE_READWRITE, 0, size, pointer(AnsiToWideEx(DecryptStr(CGlobal) + name)))
    else
      result := 0;
    if result = 0 then
      result := CreateFileMappingW(dword(-1), @sa, PAGE_READWRITE, 0, size, pointer(AnsiToWideEx(name)));
    if sd.Dacl <> nil then begin
      le := GetLastError;
      LocalFree(dword(sd.Dacl));
      SetLastError(le);
    end;
  end else
    result := CreateFileMappingA(dword(-1), nil, PAGE_READWRITE, 0, size, name);
end;

function CreateGlobalFileMapping(name: PAnsiChar; size: dword) : dword; stdcall;
begin
  EnableAllPrivileges;
  result := InternalCreateFileMapping(name, size, true);
end;

function CreateLocalFileMapping(name: PAnsiChar; size: dword) : dword; stdcall;
begin
  result := InternalCreateFileMapping(name, size, false);
end;

function InternalOpenFileMapping(name: PAnsiChar; write: bool; global: boolean) : dword;
var access : dword;
begin
  if write then
       access := FILE_MAP_ALL_ACCESS
  else access := FILE_MAP_READ;
  if GetVersion and $80000000 = 0 then begin
    if global then
      result := OpenFileMappingW(access, false, pointer(AnsiToWideEx(DecryptStr(CGlobal) + name)))
    else
      result := 0;
    if result = 0 then
      result := OpenFileMappingW(access, false, pointer(AnsiToWideEx(name)));
  end else
    result := OpenFileMappingA(access, false, name);
end;

function OpenGlobalFileMapping(name: PAnsiChar; write: bool) : dword; stdcall;
begin
  result := InternalOpenFileMapping(name, write, true);
end;

(*function OpenLocalFileMapping(name: PAnsiChar; write: bool) : dword; stdcall;
begin
  result := InternalOpenFileMapping(name, write, false);
end;*)

function CreateGlobalEvent(name: PAnsiChar; manual, initialState: bool) : dword; stdcall;
var sa : TSecurityAttributes;
    sd : TSecurityDescriptor;
begin
  if GetVersion and $80000000 = 0 then begin
    InitSecAttr(sa, sd, false);
    result := CreateEventW(@sa, manual, initialState, pointer(AnsiToWideEx(DecryptStr(CGlobal) + name)));
    if result = 0 then
      result := CreateEventW(@sa, manual, initialState, pointer(AnsiToWideEx(name)));
  end else
    result := CreateEventA(nil, manual, initialState, name);
end;

function OpenGlobalEvent(name: PAnsiChar) : dword; stdcall;
begin
  if GetVersion and $80000000 = 0 then begin
    result := OpenEventW(EVENT_ALL_ACCESS, false, pointer(AnsiToWideEx(DecryptStr(CGlobal) + name)));
    if result = 0 then
      result := OpenEventW(EVENT_ALL_ACCESS, false, pointer(AnsiToWideEx(name)));
  end else
    result := OpenEventA(EVENT_ALL_ACCESS, false, name);
end;

function ApiSpecialName(prefix: AnsiString; api: pointer; shared: boolean) : AnsiString;
begin
  if shared then
       result := DecryptStr(CShared)
  else result := DecryptStr(CProcess) + ' ' + IntToHexEx(GetCurrentProcessID, 8);
  result := DecryptStr(prefix) + ', ' + result + ', ' + DecryptStr(CApi) + ' ' + IntToHexEx(dword(api), 8);
end;

function ResolveMixtureMode(var code: pointer) : boolean;

  function DoIt(shared: boolean) : boolean;
  var map : cardinal;
      buf : pointer;
      s1  : AnsiString;
  begin
    result := false;
    s1 := DecryptStr(CNamedBuffer) + ', ' + ApiSpecialName(CMixPrefix, code, shared);
    map := OpenGlobalFileMapping(PAnsiChar(s1), true);
    if map <> 0 then begin
      buf := MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
      if buf <> nil then begin
        code := pointer(buf^);
        UnmapViewOfFile(buf);
        result := true;
      end;
      CloseHandle(map);
    end;
  end;

begin
  if GetVersion and $80000000 <> 0 then
       result := ((dword(code) < $80000000) and DoIt(false)) or DoIt(true)
  else result := DoIt(false);
end;

function FindRealCode(code: pointer) : pointer;

  function IsAlreadyKnown : boolean;

    function DoIt(shared: boolean) : boolean;
    var s1 : AnsiString;
        c1 : cardinal;
    begin
      s1 := DecryptStr(CNamedBuffer) + ', ' + ApiSpecialName(CMAHPrefix, code, shared);
      c1 := OpenGlobalFileMapping(PAnsiChar(s1), false);
      result := c1 <> 0;
      if result then CloseHandle(c1);
    end;

  begin
    if GetVersion and $80000000 <> 0 then
         result := ((dword(code) < $80000000) and DoIt(false)) or DoIt(true)
    else result := DoIt(false);
  end;

  function CheckCode : boolean;
  var module : cardinal;
      pexp   : PImageExportDirectory;
      i1     : integer;
      c1     : cardinal;
  begin
    result := ResolveMixtureMode(code) or IsAlreadyKnown;
    if (not result) and GetExportDirectory(code, module, pexp) then
      with pexp^ do
        for i1 := 0 to NumberOfFunctions - 1 do begin
          c1 := TPACardinal(module + AddressOfFunctions)^[i1];
          if module + c1 = cardinal(code) then begin
            result := true;
            break;
          end;
        end;
  end;

begin
  {$ifdef log}
    log('FindRealCode (code: ' + IntToHexEx(dword(code)) + ')');
  {$endif}
  result := code;
  if code <> nil then
    if CheckCode then
      result := code
    else
      if word(code^) = $25FF then begin
        code := pointer(TPPointer(dword(code) + 2)^^);
        if CheckCode then
          result := code
        else
          if byte(code^) = $68 then begin  // w9x debug mode?
            code := TPPointer(cardinal(code) + 1)^;
            if CheckCode then
              result := code;
          end;
      end;
  {$ifdef log}
    log('FindRealCode -> ' + IntToHexEx(dword(result)));
  {$endif}
end;

function VirtualAlloc2(size: integer) : pointer;
var p1  : pointer;
    mbi : TMemoryBasicInformation;
begin
  result := nil;
  if GetVersion and $80000000 = 0 then begin
    p1 := pointer($71af0000);
    while VirtualQuery(p1, mbi, sizeOf(mbi)) = sizeOf(mbi) do begin
      if mbi.State = MEM_FREE then begin
        result := VirtualAlloc(mbi.BaseAddress, size, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        break;
      end;
      if p1 = mbi.AllocationBase then
        dec(dword(p1), $10000)
      else
        p1 := mbi.AllocationBase;
    end;
  end;
  result := VirtualAlloc(result, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
end;

function FindWs2InternalProcList(module: dword) : pointer;
// Winsock2 internally has an additional list of some exported APIs
// we need to find this list and modify it, too
// or else Winsock2 will refuse to initialize, when we change the export table
var nh     : PImageNtHeaders;
    pexp   : PImageExportDirectory;
    ish    : PImageSectionHeader;
    i1, i2 : integer;
    buf    : array [0..9] of dword;
begin
  result := nil;
  nh := GetImageNtHeaders(module);
  pexp := GetImageExportDirectory(module);
  if (nh <> nil) and (pexp <> nil) then begin
    dword(ish) := dword(nh) + sizeOf(TImageNtHeaders);
    for i1 := 0 to integer(nh^.FileHeader.NumberOfSections) - 1 do begin
      if (ish.Name[0] = ord('.')) and
         ((ish.Name[1] = ord('d')) or (ish.Name[1] = ord('D'))) and
         ((ish.Name[2] = ord('a')) or (ish.Name[1] = ord('A'))) and
         ((ish.Name[3] = ord('t')) or (ish.Name[1] = ord('T'))) and
         ((ish.Name[4] = ord('a')) or (ish.Name[1] = ord('A'))) and
         (ish.Name[5] = 0) then begin
        for i2 := 0 to high(buf) do
          buf[i2] := module + TPACardinal(module + pexp^.AddressOfFunctions)[i2];
        i2 := PosPChar(@buf, PAnsiChar(module + ish.VirtualAddress), sizeOf(buf), ish.SizeOfRawData);
        if i2 >= 0 then
          dword(result) := module + ish.VirtualAddress + dword(i2);
        break;
      end;
      inc(ish);
    end;
  end;
end;

function PatchExportTable(module: dword; old, new: pointer; ws2procList: pointer) : boolean;

  function HackWinsock2IfNecessary(ws2procList: TPAPointer; old, new: pointer) : boolean;
  var i1 : integer;
      b1 : boolean;
  begin
    result := ws2procList = nil;
    if not result then begin
      i1 := 0;
      while ws2procList[i1] <> nil do
        if ws2procList[i1] = old then begin
          b1 := IsMemoryProtected(ws2procList[i1]);
          if UnprotectMemory(ws2procList[i1], 4) then begin
            ws2procList[i1] := new;
            result := true;
            if b1 then
              ProtectMemory(ws2procList[i1], 4);
          end;
          break;
        end else
          inc(i1);
    end;
  end;

var pexp : PImageExportDirectory;
    i1   : integer;
    c1   : cardinal;
    p1   : TPCardinal;
    b1   : boolean;
begin
  {$ifdef log}
    log('PatchExportTable (old: ' + IntToHexEx(dword(old)) +
                        '; new: ' + IntToHexEx(dword(new)) + ')');
  {$endif}
  result := false;
  try
    pexp := GetImageExportDirectory(module);
    if pexp <> nil then
      with pexp^ do
        for i1 := 0 to NumberOfFunctions - 1 do begin
          c1 := TPACardinal(module + AddressOfFunctions)^[i1];
          if module + c1 = cardinal(old) then begin
            if HackWinsock2IfNecessary(ws2procList, old, new) then begin
              p1 := @TPACardinal(module + AddressOfFunctions)^[i1];
              b1 := IsMemoryProtected(p1);
              if UnprotectMemory(p1, 4) then begin
                p1^ := cardinal(new) - module;
                result := true;
                if b1 then
                  ProtectMemory(p1, 4);
              end else
                HackWinsock2IfNecessary(ws2procList, new, old);
            end;
            break;
          end;
        end;
  except end;
  {$ifdef log}
    log('PatchExportTable (old: ' + IntToHexEx(dword(old)) +
                        '; new: ' + IntToHexEx(dword(new)) + ') -> ' + booleanToChar(result));
  {$endif}
end;

var UnprotectMemoryAsmJmp : procedure = UnprotectMemoryAsm;

{function TryMove(const Source; var Dest; count: integer) : boolean;
asm
  push ebx
  push esi
  push edi

  lea ebx, @except
  push ebx
  mov ebx, fs:[0]
  push ebx
  mov fs:[0], esp

  call Move

  mov eax, 1
  jmp @cleanup

 @except:
  mov eax, [esp+$c]
  lea ecx, @failure
  mov [eax+$b8], ecx
  mov ecx, [esp+$8]
  mov [eax+$c4], ecx
  mov eax, 0
  ret

 @failure:
  mov eax, 0

 @cleanup:
  mov ecx, [esp]
  mov fs:[0], ecx
  add esp, 8

  pop edi
  pop esi
  pop ebx
end;}

procedure PatchImportTableAsm; // (module: cardinal; old, new: pointer); stdcall;
asm
  push ebp
  mov ebp, esp
  push esi
  push edi
  push ebx
  mov eax, [ebp+$08]               // eax := module;
  cmp word ptr [eax], CEMAGIC
  jnz @quit
  mov ecx, [eax+CENEWHDR]
  add ecx, eax                     // ecx := PImageNtHeaders;
  cmp word ptr [ecx], CPEMAGIC
  jnz @quit
  mov edi, [ecx+$80]               // edi := PImageNtHeaders.OptionalFileHeader.DataDirectory[IMPORT].VirtualAddress
  cmp edi, 0                       // if edi = 0 then
  je @quit                         //   exit;
  mov ebx, [ecx+$50]               // ebx := PImageNtHeaders.OptionalFileHeader.SizeOfImage
  cmp edi, ebx                     // if edi >= ebx then
  jae @quit                        //   exit;
  add edi, eax                     // edi := PImportDirectory;
  cmp edi, 0                       // if PImportDirectory <> nil then begin
  je @quit
 @loop1Begin:
  cmp dword ptr [edi+$0c], 0       //   while PImportDirectory^.Name_ <> 0 do begin
  je @quit
  mov esi, [edi+$10]               //     esi := Thunk;
  cmp esi, 0                       //     if esi = 0 then
  je @quit                         //       exit;
  cmp esi, ebx                     //     if esi >= ebx then
  jae @quit                        //       exit;
  add esi, [ebp+$08]               //     esi := esi + module;
  cmp esi, 0                       //     if esi <> nil then
  je @quit
 @loop2Begin:
  cmp dword ptr [esi], 0           //       while esi^ <> nil do begin   
  je @nextLoop1
  mov edx, [ebp+$0c]
  cmp dword ptr [esi], edx
  jne @nextLoop2
  push 4
  push esi
  call UnprotectMemoryAsmJmp
  cmp eax, 0
  jz @nextLoop2
  mov edx, [ebp+$10]
  mov [esi], edx
 @nextLoop2:
  add esi, $04
  jmp @loop2Begin
 @nextLoop1:
  add edi, $14
  jmp @loop1Begin
 @quit:
  pop ebx
  pop edi
  pop esi
  pop ebp
  ret $0c
end;

var PatchImportTableAsmJmp : procedure = PatchImportTableAsm;

procedure PatchAllImportTablesAsm; // (old, new: pointer; shared: bool); stdcall;
asm
  push ebp
  mov ebp, esp
  sub esp, $20
  pushad
  pushfd
  xor ebx, ebx
  mov esi, [ebp+$08]
  mov edi, [ebp+$0c]
  mov dword [ebp-$20], 0
 @nextLoop:
  push $1c
  lea eax, [ebp-$1c]
  push eax
  push ebx
  call VirtualQueryFunc
  cmp eax, $1c
  jne @quit
  cmp dword [ebp-$0c], MEM_COMMIT
  jnz @noModule
  cmp dword [ebp-$18], 0
  jz @noModule
  mov eax, [ebp-$18]
  cmp [ebp-$20], eax
  jz @noModule
  mov [ebp-$20], eax
  mov eax, [ebp+$10]
  cmp eax, 0
  jnz @notShared
  mov eax, [ebp-$18]
  cmp eax, $80000000
  jae @quit
 @notShared:
  push 2
  lea eax, [ebp-$1c]
  push eax
  mov eax, [ebp-$18]
  push eax
  call GetModuleFileNameAFunc
  cmp eax, 0
  jz @noModule
  push edi
  push esi
  mov eax, [ebp-$18]
  push eax
  call PatchImportTableAsmJmp
 @noModule:
  add ebx, [ebp-$10]
  jmp @nextLoop
 @quit:
  popfd
  popad
  add esp, $20
  pop ebp
  ret $0c
end;

var PatchAllImportTablesAsmJmp : procedure = PatchAllImportTablesAsm;

procedure PatchMyImportTables(old, new: pointer);
asm
  push 1
  push new
  push old
  call PatchAllImportTablesAsm
end;

function PatchImportTableRemoteThread(delay: dword) : integer; stdcall;
asm
  push delay
  call SleepProc
  push 0
  push esp
  call LoadLibraryAFunc
  pop eax
  xor eax, eax
end;

(* Caution: currently only 32bit support

function HookReloc(module: dword; old, new: pointer; var next: pointer) : boolean;
type TImageBaseRelocation = record
       VirtualAddress : dword;
       SizeOfBlock    : dword;
     end;
var
  nh     : PImageNtHeaders;
  rel1   : ^TImageBaseRelocation;
  rel2   : dword;
  i1     : integer;
  item   : TPWord;
  data   : ^TPPointer;
  opcode : TPWord;
  cb, ce : dword;
  p1     : pointer;
  c1     : dword;
begin
  result := false;
  nh := GetImageNtHeaders(module);
  if nh <> nil then
    with nh^.OptionalHeader do begin
      cb := module + BaseOfCode + 2;
      ce := module + BaseOfCode + SizeOfCode;
      with DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] do begin
        dword(rel1) := module + VirtualAddress;
        rel2 := dword(rel1) + Size;
      end;
      while (dword(rel1) < rel2) and (rel1^.VirtualAddress <> 0) do begin
        dword(item) := dword(rel1) + sizeOf(rel1^);
        for i1 := 1 to (rel1.SizeOfBlock - sizeOf(rel1^)) div 2 do begin
          if item^ and $f000 = $3000 then begin
            dword(data) := module + rel1.VirtualAddress + item^ and $0fff;
            dword(opcode) := dword(data) - 2;
            if (dword(data) >= cb) and (dword(data) < ce) and
               (data^ <> @next) and
               ( (opcode^ = $25ff) or (opcode^ = $15ff) ) and
               TryRead(data^, @p1, 4) and
               (p1 = old) and
               VirtualProtect(data^, 4, PAGE_EXECUTE_READWRITE, c1) then begin
              result := true;
              data^^ := new;
              VirtualProtect(data^, 4, c1, c1);
            end;
          end;
          inc(item);
        end;
        dword(rel1) := dword(rel1) + rel1^.SizeOfBlock;
      end;
    end;
end; *)

function GetKernelEntryPoint : pointer;
var module : dword;
    nh     : PImageNtHeaders;
begin
  {$ifdef log}
    log('GetKernelEntryPoint');
  {$endif}
  module := kernel32handle;
  nh     := GetImageNtHeaders(module);
  if nh <> nil then
       result := FindRealCode(pointer(nh^.OptionalHeader.AddressOfEntryPoint + module))
  else result := nil;
  {$ifdef log}
    log('GetKernelEntryPoint -> ' + IntToHexEx(dword(result)));
  {$endif}
end;

function PatchKernelEntryPoint(new: pointer) : boolean;
var module : dword;
    nh     : PImageNtHeaders;
    p1     : TPCardinal;
    b1     : boolean;
begin
  {$ifdef log}
    log('PatchKernelEntryPoint (new: ' + IntToHexEx(dword(new)) + ')');
  {$endif}
  module := kernel32handle;
  nh     := GetImageNtHeaders(module);
  result := false;
  if nh <> nil then begin
    p1 := @nh^.OptionalHeader.AddressOfEntryPoint;
    b1 := IsMemoryProtected(p1);
    if UnprotectMemory(p1, 4) then begin
      p1^ := cardinal(new) - module;
      if b1 then
        ProtectMemory(p1, 4);
      result := true;
    end;
  end;
  {$ifdef log}
    log('PatchKernelEntryPoint (new: ' + IntToHexEx(dword(new)) + ') -> ' + booleanToChar(result));
  {$endif}
end;

var
  OldKernelEntryPoint : function (module, reason: dword; reserved: pointer) : bool; stdcall;
  NewKernelEntryPoint : packed record
    cmpReason  : packed record
                   opcode  : dword;
                   cmp     : byte;
                 end;
    jnzEnd     : packed record
                   opcode  : byte;
                   target  : byte;
                 end;
    pushShared : packed record
                   opcode  : byte;
                   value   : cardinal;
                 end;
    pushNew    : packed record
                   opcode  : byte;
                   value   : pointer;
                 end;
    pushOld    : packed record
                   opcode  : byte;
                   value   : pointer;
                 end;
    callPatch  : packed record
                   opcode  : word;
                   ptarget : pointer;
                 end;
    pushParam3 : packed record
                   opcode  : dword;
                 end;
    pushParam2 : packed record
                   opcode  : dword;
                 end;
    pushParam1 : packed record
                   opcode  : dword;
                 end;
    callOld    : packed record
                   opcode  : word;
                   ptarget : pointer;
                 end;
    ret0c      : packed record
                   opcode  : byte;
                   value   : word;
                 end;
  end =
   (cmpReason  : (opcode: $08247C83; cmp    : $01);
    jnzEnd     : (opcode: $75;       target : $15);
    pushShared : (opcode: $68;       value  : 0  );
    pushNew    : (opcode: $68;       value  : nil);
    pushOld    : (opcode: $68;       value  : nil);
    callPatch  : (opcode: $15FF;     ptarget: @@PatchAllImportTablesAsmJmp);
    pushParam3 : (opcode: $0C2474FF              );
    pushParam2 : (opcode: $0C2474FF              );
    pushParam1 : (opcode: $0C2474FF              );
    callOld    : (opcode: $15FF;     ptarget: @@OldKernelEntryPoint       );
    ret0c      : (opcode: $C2;       value  : $0C);
   );

  OldLoadLibrary : function (lib: PAnsiChar) : dword; stdcall;
  NewLoadLibrary : packed record
    pushEbx             : packed record
                            opcode    : byte;
                          end;
    movEaxEsp1          : packed record
                            opcode    : dword;
                          end;
    cmpEaxNil           : packed record
                            opcode    : byte;
                            value     : dword;
                          end;
    jnzNormalCall       : packed record
                            opcode    : byte;
                            target    : byte;
                          end;
    pushEax1            : packed record
                            opcode    : byte;
                          end;
    callOldLoadLibrary1 : packed record
                            opcode    : word;
                            absTarget : pointer;
                          end;
    jmpPatch            : packed record
                            opcode    : byte;
                            target    : byte;
                          end;
    pushEax2            : packed record
                            opcode    : byte;
                          end;
    callGetModuleHandle : packed record
                            opcode    : byte;
                            relTaget  : dword;
                          end;
    movEbxEax           : packed record
                            opcode    : word;
                          end;
    movEaxEsp2          : packed record
                            opcode    : dword;
                          end;
    pushEax3            : packed record
                            opcode    : byte;
                          end;
    callOldLoadLibrary2 : packed record
                            opcode    : word;
                            absTarget : pointer;
                          end;
    cmpEax0             : packed record
                            opcode    : word;
                            value     : byte;
                          end;
    jzNoPatch           : packed record
                            opcode    : byte;
                            target    : byte;
                          end;
    cmpEbx0             : packed record
                            opcode    : word;
                            value     : byte;
                          end;
    jnzNoPatch          : packed record
                            opcode    : byte;
                            target    : byte;
                          end;
    pushShared          : packed record
                            opcode    : byte;
                            value     : cardinal;
                          end;
    pushNew             : packed record
                            opcode    : byte;
                            value     : pointer;
                          end;
    pushOld             : packed record
                            opcode    : byte;
                            value     : pointer;
                          end;
    callPatch           : packed record
                            opcode    : word;
                            ptarget   : pointer;
                          end;
    popEbx              : packed record
                            opcode    : byte;
                          end;
    ret04               : packed record
                            opcode    : byte;
                            value     : word;
                          end;
  end =
   (pushEbx             : (opcode: $53      );
    movEaxEsp1          : (opcode: $0824448B);
    cmpEaxNil           : (opcode: $3D;       value     : 0);
    jnzNormalCall       : (opcode: $75;       target    : $09);
    pushEax1            : (opcode: $50      );
    callOldLoadLibrary1 : (opcode: $15FF;     absTarget : @@OldLoadLibrary);
    jmpPatch            : (opcode: $EB;       target    : $1D);
    pushEax2            : (opcode: $50;     );
    callGetModuleHandle : (opcode: $E8;       relTaget  : $00);
    movEbxEax           : (opcode: $C389    );
    movEaxEsp2          : (opcode: $0824448B);
    pushEax3            : (opcode: $50      );
    callOldLoadLibrary2 : (opcode: $15FF;     absTarget : @@OldLoadLibrary);
    cmpEax0             : (opcode: $F883;     value     : 0  );
    jzNoPatch           : (opcode: $74;       target    : $1A);
    cmpEbx0             : (opcode: $FB83;     value     : 0  );
    jnzNoPatch          : (opcode: $75;       target    : $15);
    pushShared          : (opcode: $68;       value     : 0  );
    pushNew             : (opcode: $68;       value     : nil);
    pushOld             : (opcode: $68;       value     : nil);
    callPatch           : (opcode: $15FF;     ptarget   : @@PatchAllImportTablesAsmJmp);
    popEbx              : (opcode: $5B      );
    ret04               : (opcode: $C2;       value     : $04);
   );

function HookCodeX(owner        : dword;
                   moduleH      : dword;
                   module       : AnsiString;
                   api          : AnsiString;
                   code         : pointer;
                   callbackFunc : pointer;
                   out nextHook : pointer;
                   flags        : dword = 0) : boolean; forward;

procedure InstallMixturePatcher(old, new: pointer);
type TMchPIT9x = packed record
                   name       : array [0..7] of AnsiChar;
                   unprotect  : pointer;
                   patchIt    : pointer;
                   patchAllIt : pointer;
                 end;
var map    : dword;
    newMap : boolean;
    buf    : ^TMchPIT9x;
    s1     : AnsiString;
    i1     : integer;
begin
  {$ifdef log}
    log('InstallMixturePatcher (old: ' + IntToHexEx(dword(old)) +
                             '; new: ' + IntToHexEx(dword(new)) + ')');
  {$endif}
  if dword(@PatchAllImportTablesAsmJmp) < $80000000 then begin
    s1 := DecryptStr(CMchPIT9x);
    map := CreateFileMappingA(dword(-1), nil, PAGE_READWRITE, 0, sizeOf(buf^), PAnsiChar(s1));
    if map <> 0 then begin
      newMap := GetLastError <> ERROR_ALREADY_EXISTS;
      buf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
      if buf <> nil then begin
        for i1 := 1 to 50 do
          if newMap or (TPInt64(@buf^.name)^ = TPInt64(s1)^) then
               break
          else Sleep(50);
        if newMap or (TPInt64(@buf^.name)^ <> TPInt64(s1)^) then begin
          UnprotectMemoryAsmJmp      := CopyFunction(@UnprotectMemoryAsm);
          PatchImportTableAsmJmp     := CopyFunction(@PatchImportTableAsm);
          PatchAllImportTablesAsmJmp := CopyFunction(@PatchAllImportTablesAsm);
          buf^.unprotect  := @UnprotectMemoryAsmJmp;
          buf^.patchIt    := @PatchImportTableAsmJmp;
          buf^.patchAllIt := @PatchAllImportTablesAsmJmp;
          AtomicMove(pointer(s1), @buf^.name, 8);
          HandleLiveForever(map);
        end else begin
          UnprotectMemoryAsmJmp      := buf^.unprotect;
          PatchImportTableAsmJmp     := buf^.patchIt;
          PatchAllImportTablesAsmJmp := buf^.patchAllIt;
        end;
        UnmapViewOfFile(buf);
      end;
      CloseHandle(map);
    end;
  end;
  if dword(@PatchAllImportTablesAsmJmp) >= $80000000 then begin
    NewKernelEntryPoint.pushNew.value := new;
    NewKernelEntryPoint.pushOld.value := old;
    {$ifdef log}
      log('MixturePatcher -> hook kernel entry point (' + IntToHexEx(dword(GetKernelEntryPoint)) + ')');
    {$endif}
    HookCodeX(0, 0, '', '', GetKernelEntryPoint, @NewKernelEntryPoint, @OldKernelEntryPoint, SYSTEM_WIDE_9X);
    NewLoadLibrary.callGetModuleHandle.relTaget := dword(FindRealCode(KernelProc(CGetModuleHandleA, true))) - dword(@NewLoadLibrary.movEbxEax);
    NewLoadLibrary.pushNew.value := new;
    NewLoadLibrary.pushOld.value := old;
    {$ifdef log}
      log('MixturePatcher -> hook LoadLibraryA (' + IntToHexEx(dword(KernelProc(CLoadLibraryA, true))) + ')');
    {$endif}
    HookCodeX(0, 0, '', '', FindRealCode(KernelProc(CLoadLibraryA, true)), @NewLoadLibrary, @OldLoadLibrary, SYSTEM_WIDE_9X);
  end;
  {$ifdef log}
    log('InstallMixturePatcher (old: ' + IntToHexEx(dword(old)) +
                             '; new: ' + IntToHexEx(dword(new)) + ') -> done');
  {$endif}
end;

// ***************************************************************

var IsWine : boolean = false;

var
  CollectCache : array of packed record
    thread     : dword;
    refCount   : integer;
    active     : boolean;
    wait       : boolean;

    // in RAM a lot of patching can happen
    // sometimes madCodeHook has to make sure that it can get original unpatched information
    // for such purposes it opens the original exe/dll files on harddisk
    // in order to speed things up, we're caching the last opened file here
    moduleOrg  : dword;    // which loaded exe/dll module is currently open?
    moduleFile : pointer;  // where is the file mapped into memory?
  end;

function CollectCache_Open(var mutex: dword; force: boolean) : integer;
var i1 : integer;
begin
  mutex := CreateLocalMutex(PAnsiChar(DecryptStr(CMchMixCache) + IntToHexEx(dword(@CollectCache_Open)) + IntToHexEx(GetCurrentProcessID)));
  if mutex <> 0 then
    WaitForSingleObject(mutex, INFINITE);
  result := -1;
  for i1 := 0 to high(CollectCache) do
    if CollectCache[i1].thread = GetCurrentThreadID then begin
      result := i1;
      break;
    end;
  if (result = -1) and force then begin
    result := Length(CollectCache);
    SetLength(CollectCache, result + 1);
    CollectCache[result].thread := GetCurrentThreadID;
  end;
end;

procedure CollectCache_Close(mutex: dword);
begin
  if mutex <> 0 then begin
    ReleaseMutex(mutex);
    CloseHandle (mutex);
  end;
end;

procedure CollectCache_AddRef;
var mutex : dword;
    i1    : integer;
begin
  i1 := CollectCache_Open(mutex, true);
  try
    inc(CollectCache[i1].refCount);
  finally CollectCache_Close(mutex) end;
end;

procedure CollectCache_Activate(wait: boolean);
var mutex : dword;
    i1    : integer;
begin
  i1 := CollectCache_Open(mutex, true);
  try
    CollectCache[i1].active := true;
    CollectCache[i1].wait   := CollectCache[i1].wait or wait;
  finally CollectCache_Close(mutex) end;
end;

function CollectCache_NeedModuleFileMap(module: dword) : pointer;
var mutex : dword;
    i1    : integer;
    buf   : pointer;
begin
  result := nil;
  if (not IsWine) and (CollectCache <> nil) then begin
    buf := nil;
    i1 := CollectCache_Open(mutex, false);
    try
      if i1 <> -1 then begin
        if CollectCache[i1].moduleOrg <> module then begin
          if CollectCache[i1].moduleOrg <> 0 then begin
            buf := CollectCache[i1].moduleFile;
            CollectCache[i1].moduleOrg  := 0;
            CollectCache[i1].moduleFile := nil;
          end;
        end else
          result := CollectCache[i1].moduleFile;
      end;
    finally CollectCache_Close(mutex) end;
    if i1 <> -1 then begin
      if buf <> nil then
        UnmapViewOfFile(buf);
      if result = nil then begin
        buf := NeedModuleFileMap(module);
        if buf <> nil then begin
          i1 := CollectCache_Open(mutex, false);
          try
            if i1 <> -1 then begin
              CollectCache[i1].moduleOrg  := module;
              CollectCache[i1].moduleFile := buf;
              result := buf;
            end else
              UnmapViewOfFile(buf);
          finally CollectCache_Close(mutex) end;
        end;
      end;
    end;
  end;
end;

procedure CollectCache_Release;
type TMchPitRT = packed record
                   name   : array [0..7] of AnsiChar;
                   thread : pointer;
                 end;
var mutex      : dword;
    patch      : boolean;
    wait       : boolean;
    c1, c2, c3 : cardinal;
    pe         : TProcessEntry32;
    buf        : ^TMchPitRT;
    thread     : pointer;
    threads    : TDACardinal;
    i1, i2     : integer;
    delay      : dword;
    map        : dword;
    newMap     : boolean;
    s1         : AnsiString;
begin
  patch := false;
  wait  := false;
  buf   := nil;
  i1 := CollectCache_Open(mutex, false);
  try
    if i1 <> -1 then begin
      dec(CollectCache[i1].refCount);
      if CollectCache[i1].refCount = 0 then begin
        patch := CollectCache[i1].active;
        wait  := CollectCache[i1].wait;
        buf   := CollectCache[i1].moduleFile;
        CollectCache[i1] := CollectCache[high(CollectCache)];
        SetLength(CollectCache, high(CollectCache));
      end;
    end;
  finally CollectCache_Close(mutex) end;
  if buf <> nil then
    UnmapViewOfFile(buf);
  if patch then begin
    thread := nil;
    s1 := DecryptStr(CMchPitRT);
    map := CreateFileMappingA(dword(-1), nil, PAGE_READWRITE, 0, sizeOf(buf^), PAnsiChar(s1));
    if map <> 0 then begin
      newMap := GetLastError <> ERROR_ALREADY_EXISTS;
      buf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
      if buf <> nil then begin
        for i1 := 1 to 50 do
          if newMap or (TPInt64(@buf^.name)^ = TPInt64(s1)^) then
               break
          else Sleep(50);
        if newMap or (TPInt64(@buf^.name)^ <> TPInt64(s1)^) then begin
          buf^.thread := CopyFunction(@PatchImportTableRemoteThread);
          AtomicMove(pointer(s1), @buf^.name, 8);
          HandleLiveForever(map);
        end;
        thread := buf^.thread;
        UnmapViewOfFile(buf);
      end;
      CloseHandle(map);
    end;
    if thread <> nil then begin
      InitToolhelp;
      i1 := 0;
      c1 := CreateToolHelp32Snapshot(TH32CS_SnapProcess, 0);
      if c1 <> INVALID_HANDLE_VALUE then
        try
          if wait then delay := 0
          else         delay := 500;
          pe.size := sizeOf(TProcessEntry32);
          if Process32First(c1, pe) then
            repeat
              if pe.process <> GetCurrentProcessID then begin
                c2 := OpenProcess(PROCESS_ALL_ACCESS, false, pe.process);
                if c2 <> 0 then
                  try
                    c3 := CreateRemoteThreadEx(c2, nil, 0, thread, pointer(delay), 0, c3);
                    if c3 <> 0 then
                      if wait then begin
                        SetLength(threads, i1 + 1);
                        threads[i1] := c3;
                        inc(i1);
                      end else begin
                        SetThreadPriority(c3, THREAD_PRIORITY_IDLE);
                        CloseHandle(c3);
                        inc(delay, 500);
                      end;
                  finally CloseHandle(c2) end;
              end;
            until not Process32Next(c1, pe);
        finally CloseHandle(c1) end;
      while i1 > 0 do
        if i1 > 32 then begin
          WaitForMultipleObjects(32, @threads[0], true, INFINITE);
          for i2 := 0 to 31 do
            CloseHandle(threads[i2]);
          Move(threads[32], threads[0], (i1 - 32) * 4);
          dec(i1, 32);
        end else begin
          WaitForMultipleObjects(i1, @threads[0], true, INFINITE);
          for i2 := 0 to i1 - 1 do
            CloseHandle(threads[i2]);
          break;
        end;
    end;
  end;
end;

// ***************************************************************

type
  // type for absolute jmp calls
  TAbsoluteJmp = packed record  // jmp [target]
    opcode : byte;       // $FF
    modRm  : byte;       // $25
    target : TPPointer;  // @(absolute target)
  end;

  // type for absolute jumps
  THookStub = packed record
    jmpNextHook : TAbsoluteJmp;  // $FF / $25 / @absTarget
    absTarget   : pointer;       // absolute target
  end;

  // type for TICodeHook.FShareStub
  TSharedMemoryStub = packed record
    pushEax                 : byte;                        // $50
    callGetCurrentProcessID : packed record
                                opcode    : byte;          // $E8
                                relTarget : integer;       // @GetCurrentProcessID
                              end;
    cmpEax                  : packed record
                                opcode    : byte;          // $3D
                                cmpValue  : cardinal;      // GetCurrentProcessID value
                              end;
    popEax                  : byte;                        // $58
    jnz5                    : packed record
                                opcode    : byte;          // $75
                                relTarget : shortInt;      // 10
                              end;
    jmpCallbackFunc         : packed record
                                absJump   : TAbsoluteJmp;  // $FF / $25 / @absTarget
                                absTarget : pointer;       // CallbackFunc
                              end;
    jmpNextHook             : THookStub;
  end;

  THookEntry = record
    HookProc  : pointer;
    PNextHook : TPPointer;
  end;
  THookQueueItems = array [0..maxInt shr 4 - 1] of THookEntry;
  THookQueue = record
    ItemCount   : integer;
    Capacity    : integer;
    MapHandle   : cardinal;
    PatchAddr   : ^TAbsoluteJmp;
    OldCode     : TAbsoluteJmp;
    NewCode     : TAbsoluteJmp;
    Items       : THookQueueItems;
  end;

const
  CHookEntrySize = sizeOf(THookEntry);
  CHookQueueSize = sizeOf(THookQueue) - sizeOf(THookQueueItems);

// ***************************************************************

type
  T9xProcesses = packed array [0..maxInt shr 4] of packed record
    pid    : dword;
    ph     : dword;
    hook   : pointer;
  end;
  TP9xProcesses = ^T9xProcesses;
  T9xSharedItems = packed array [0..maxInt shr 5] of packed record
    api    : pointer;
    apiHex : int64;
    hook   : pointer;
    count2 : integer;
    items2 : TP9xProcesses;
  end;
  TP9xSharedItems = ^T9xSharedItems;
  T9xSharedStubs = packed record
    count1 : integer;
    items1 : TP9xSharedItems;
  end;
  TP9xSharedStubs = ^T9xSharedStubs;

  TWaitArray = array [0..MAXIMUM_WAIT_OBJECTS - 1] of dword;
  T9xWatcherThread = packed record
    mutex1 : dword;
    mutex2 : dword;
    ss     : TP9xSharedStubs;
    wait   : TWaitArray;
    pids   : array [1..MAXIMUM_WAIT_OBJECTS - 1] of dword;
    count2 : integer;
  end;
  TP9xWatcherThread = ^T9xWatcherThread;
  T9xWatcherThreads = packed array [0..maxInt shr 10] of TP9xWatcherThread;
  TP9xWatcherThreads = ^T9xWatcherThreads;
  T9xWatcher = packed record
    count1 : integer;
    items1 : TP9xWatcherThreads;
    func   : pointer;
  end;

function Del9xSharedStubThread(var wt: T9xWatcherThread) : integer; stdcall;
var c1         : dword;
    wait       : TWaitArray;
    pid        : dword;
    ph         : dword;
    i1, i2, i3 : integer;
    si         : TP9xSharedItems;
    b1, b2     : boolean;
    mutex      : cardinal;
    map        : cardinal;
    buf        : pointer;
    str1       : array [0..39] of AnsiChar;
    str2       : array [0..33] of AnsiChar;
begin
  str1[00] := 'N';
  str1[01] := 'a';
  str1[02] := 'm';
  str1[03] := 'e'; 
  str1[04] := 'd';
  str1[05] := 'B';
  str1[06] := 'u'; str2[00] := 'M';
  str1[07] := 'f'; str2[01] := 'u';
  str1[08] := 'f'; str2[02] := 't';
  str1[09] := 'e'; str2[03] := 'e';
  str1[10] := 'r'; str2[04] := 'x';
  str1[11] := ','; str2[05] := ',';
  str1[12] := ' '; str2[06] := ' ';
  str1[13] := 'm'; str2[07] := 'm';
  str1[14] := 'A'; str2[08] := 'A';
  str1[15] := 'H'; str2[09] := 'H';
  str1[16] := ','; str2[10] := ',';
  str1[17] := ' '; str2[11] := ' ';
  str1[18] := 'S'; str2[12] := 'S';
  str1[19] := 'h'; str2[13] := 'h';
  str1[20] := 'a'; str2[14] := 'a';
  str1[21] := 'r'; str2[15] := 'r';
  str1[22] := 'e'; str2[16] := 'e';
  str1[23] := 'd'; str2[17] := 'd';
  str1[24] := ','; str2[18] := ',';
  str1[25] := ' '; str2[19] := ' ';
  str1[26] := 'A'; str2[20] := 'A';
  str1[27] := 'P'; str2[21] := 'P';
  str1[28] := 'I'; str2[22] := 'I';
  str1[29] := ' '; str2[23] := ' ';
  str1[30] := '$'; str2[24] := '$';
  str1[39] := #0;  str2[33] := #0;
  c1 := 0;
  while c1 < MAXIMUM_WAIT_OBJECTS do begin
    if c1 = 0 then begin
      WaitForSingleObjectFunc(wt.mutex1, INFINITE);
      wait := wt.wait;
      c1 := wt.count2;
      ReleaseMutexFunc(wt.mutex1);
    end else begin
      WaitForSingleObjectFunc(wt.mutex1, INFINITE);
      pid := wt.pids[c1];
      ph  := wt.wait[c1];
      wt.pids[c1] := wt.pids[wt.count2]; wt.pids[wt.count2] := 0;
      wt.wait[c1] := wt.wait[wt.count2]; wt.wait[wt.count2] := 0;
         wait[c1] :=    wait[wt.count2];    wait[wt.count2] := 0;
      dec(wt.count2);
      c1 := wt.count2;
      ReleaseMutexFunc(wt.mutex1);
      CloseHandleFunc(ph);
      WaitForSingleObjectFunc(wt.mutex2, INFINITE);
      i3 := 0;
      with wt.ss^ do
        for i1 := 0 to count1 - 1 do
          with items1^[i1] do
            for i2 := 0 to count2 - 1 do
              if items2^[i2].pid = pid then
                inc(i3);
      si := LocalAllocFunc(LPTR, i3 * 24);
      i3 := 0;
      with wt.ss^ do begin
        b1 := true;
        for i1 := 0 to count1 - 1 do
          with items1^[i1] do begin
            b2 := true;
            for i2 := 0 to count2 - 1 do
              if items2^[i2].pid = pid then begin
                si^[i3] := items1^[i1];
                si^[i3].hook := items2^[i2].hook;
                inc(i3);
                items2^[i2].pid := 0;
              end else
                if items2^[i2].pid <> 0 then
                  b2 := false;
            if b2 then begin
              if items2 <> nil then
                SharedMem9x_Free(items2);
              count2 := 0;
              items2 := nil;
            end else
              if count2 > 0 then
                b1 := false;
          end;
        if b1 then begin
          if items1 <> nil then
            SharedMem9x_Free(items1);
          count1 := 0;
          items1 := nil;
        end;
      end;
      ReleaseMutexFunc(wt.mutex2);
      for i1 := 0 to i3 - 1 do
        with si^[i1] do begin
          TPInt64(@str1[31])^ := apiHex;
          TPInt64(@str2[25])^ := apiHex;
          mutex := CreateMutexAFunc(nil, false, str2);
          if mutex <> 0 then begin
            WaitForSingleObjectFunc(mutex, INFINITE);
            map := OpenFileMappingAFunc(FILE_MAP_ALL_ACCESS, false, str1);
            if map <> 0 then begin
              buf := MapViewOfFileFunc(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
              if buf <> nil then begin
                with THookQueue(TPPointer(buf)^^) do
                  for i2 := 1 to ItemCount - 2 do
                    if Items[i2].HookProc = hook then begin
                      for i3 := i2 to ItemCount - 2 do
                        Items[i3] := Items[i3 + 1];
                      Items[i2 - 1].PNextHook^ := Items[i2].HookProc;
                      dec(ItemCount);
                      if hook <> nil then
                        SharedMem9x_Free(hook);
                      break;
                    end;
                UnmapViewOfFileFunc(buf);
              end;
              CloseHandleFunc(map);
            end;
            ReleaseMutexFunc(mutex);
            CloseHandleFunc(mutex);
          end;
        end;
      LocalFreeFunc(si);
    end;
    c1 := WaitForMultipleObjectsFunc(c1 + 1, @wait, false, INFINITE);
  end;
  result := 0;
end;

procedure Add9xSharedStub(ph_, pid_: dword; api_, hook_: pointer);
var ppss : ^TP9xSharedStubs;
    pss  : TP9xSharedStubs;

  procedure AddProcessToIndex(index: integer);
  var i1, i2 : integer;
      prc    : TP9xProcesses;
  begin
    with pss^.items1^[index] do begin
      for i1 := 0 to count2 - 1 do
        if items2^[i1].pid = 0 then begin
          items2^[i1].pid  := pid_;
          items2^[i1].ph   := ph_;
          items2^[i1].hook := hook_;
          exit;
        end;
      i2 := count2;
      if count2 = 0 then
           count2 := 16
      else count2 := count2 * 2;
      prc := AllocMemEx(count2 * 12);
      for i1 := 0 to i2 - 1 do
        prc^[i1] := items2^[i1];
      FreeMemEx(items2);
      items2 := prc;
      items2^[i2].pid  := pid_;
      items2^[i2].ph   := ph_;
      items2^[i2].hook := hook_;
      for i1 := i2 + 1 to count2 - 1 do
        items2^[i1].pid := 0;
    end;
  end;

  procedure AddApiToIndex(index: integer);
  begin
    with pss^.items1^[index] do begin
      api  := api_;
      hook := nil;
      Move(IntToHexEx(dword(api_), 8)[2], apiHex, 8);
      AddProcessToIndex(index);
    end;
  end;

  procedure AddApi;
  var i1, i2 : integer;
      si     : TP9xSharedItems;
  begin
    with pss^ do begin
      for i1 := 0 to count1 - 1 do
        with items1^[i1] do
          if (count2 > 0) and (api = api_) then begin
            AddProcessToIndex(i1);
            exit;
          end;
      for i1 := 0 to count1 - 1 do
        with items1^[i1] do
          if count2 = 0 then begin
            AddApiToIndex(i1);
            exit;
          end;
      i2 := count1;
      if count1 = 0 then
           count1 := 32
      else count1 := count1 * 2;
      si := AllocMemEx(count1 * 24);
      for i1 := 0 to i2 - 1 do
        si^[i1] := items1^[i1];
      FreeMemEx(items1);
      items1 := si;
      AddApiToIndex(i2);
      for i1 := i2 + 1 to count1 - 1 do begin
        items1^[i1].count2 := 0;
        items1^[i1].items2 := nil;
      end;
    end;
  end;

  procedure AddProcess(mutex_: dword);
  var i1, i2 : integer;
      kh     : dword;
      wbuf   : ^T9xWatcher;
      mutex  : dword;
      map    : dword;
      newMap : boolean;
      b1     : boolean;
      event  : dword;
      wt     : TP9xWatcherThreads;
      c1     : dword;
  begin
    with pss^ do
      for i1 := 0 to count1 - 1 do
        with items1^[i1] do
          for i2 := 0 to count2 - 1 do
            if items2^[i2].pid = pid_ then begin
              ph_ := items2^[i2].ph;
              exit;
            end;
    kh := GetKernel32ProcessHandle;
    if kh <> 0 then
      try
        mutex := CreateMutexA(nil, false, PAnsiChar(DecryptStr(CMAHWatcherThread) + DecryptStr(CMutex)));
        if mutex <> 0 then begin
          WaitForSingleObject(mutex, INFINITE);
          try
            map := CreateFileMappingA(dword(-1), nil, PAGE_READWRITE, 0, 12, PAnsiChar(DecryptStr(CMAHWatcherThread) + DecryptStr(CMap)));
            if map <> 0 then begin
              newMap := GetLastError <> ERROR_ALREADY_EXISTS;
              wbuf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
              if wbuf <> nil then begin
                if newMap then begin
                  wbuf^.count1 := 0;
                  wbuf^.items1 := nil;
                  wbuf^.func   := CopyFunction(@Del9xSharedStubThread);
                  if wbuf^.func <> nil then
                    DuplicateHandle(GetCurrentProcess, map, kh, @c1, 0, false, DUPLICATE_SAME_ACCESS);
                end;
                if wbuf^.func <> nil then begin
                  b1 := true;
                  for i1 := 0 to wbuf^.count1 - 1 do
                    with wbuf^.items1^[i1]^ do
                      for i2 := 0 to count2 - 1 do
                        if pids[count2] = pid_ then begin
                          ph_ := wait[count2];
                          b1 := false;
                          break;
                        end;
                  if b1 then begin
                    DuplicateHandle(GetCurrentProcess, ph_, kh, @ph_, 0, false, DUPLICATE_SAME_ACCESS);
                    for i1 := 0 to wbuf^.count1 - 1 do
                      with wbuf^.items1^[i1]^ do
                        if count2 < MAXIMUM_WAIT_OBJECTS - 1 then begin
                          inc(count2);
                          pids[count2] := pid_;
                          wait[count2] := ph_;
                          event := OpenEventA(EVENT_ALL_ACCESS, false, PAnsiChar(DecryptStr(CMAHWatcherThread) + DecryptStr(CEvent) + IntToStrEx(i1)));
                          if event <> 0 then begin
                            SetEvent(event);
                            CloseHandle(event);
                          end;
                          b1 := false;
                          break;
                        end;
                    if b1 then
                      with wbuf^ do begin
                        event := CreateEventA(nil, false, false, PAnsiChar(DecryptStr(CMAHWatcherThread) + DecryptStr(CEvent) + IntToStrEx(count1)));
                        if event <> 0 then begin
                          inc(count1);
                          wt := AllocMemEx(count1 * 4);
                          for i1 := 0 to count1 - 2 do
                            wt^[i1] := items1^[i1];
                          FreeMemEx(items1);
                          items1 := wt;
                          items1^[count1 - 1] := AllocMemEx(524);
                          with items1^[count1 - 1]^ do begin
                            count2 := 1;
                            pids[1] := pid_;
                            DuplicateHandle(GetCurrentProcess, event, kh, @wait[0], 0, false, DUPLICATE_SAME_ACCESS);
                            CloseHandle(event);
                            wait[1] := ph_;
                            ss := pss;
                            if count1 = 1 then begin
                              DuplicateHandle(GetCurrentProcess, mutex,  kh, @mutex1, 0, false, DUPLICATE_SAME_ACCESS);
                              DuplicateHandle(GetCurrentProcess, mutex_, kh, @mutex2, 0, false, DUPLICATE_SAME_ACCESS);
                            end else begin
                              mutex1 := items1^[0].mutex1;
                              mutex2 := items1^[0].mutex2;
                            end;
                            CreateRemoteThreadEx(kh, nil, 0, func, items1^[count1 - 1], 0, c1);
                          end;
                        end;
                      end;
                  end;
                end;
                UnmapViewOfFile(wbuf);
              end;
              CloseHandle(map);
            end;
          finally ReleaseMutex(mutex) end;
          CloseHandle(mutex);
        end;
      finally CloseHandle(kh) end;
  end;

var mutex  : dword;
    newBuf : boolean;
    map    : dword;
begin
  {$ifdef log}
    log('Add9xSharedStub (ph: '   + IntToHexEx(ph_) +
                       '; pid: '  + IntToHexEx(pid_) +
                       '; api: '  + IntToHexEx(dword(api_)) +
                       '; hook: ' + IntToHexEx(dword(hook_)) +
                         ')');
  {$endif}
  mutex := CreateMutexA(nil, false, PAnsiChar(DecryptStr(CMAHStubs) + DecryptStr(CMutex)));
  if mutex <> 0 then begin
    WaitForSingleObject(mutex, INFINITE);
    try
      map := CreateFileMappingA(dword(-1), nil, PAGE_READWRITE, 0, 4, PAnsiChar(DecryptStr(CMAHStubs) + DecryptStr(CMap)));
      if map <> 0 then begin
        newBuf := GetLastError <> ERROR_ALREADY_EXISTS;
        ppss := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if ppss <> nil then begin
          if newBuf then begin
            pss := AllocMemEx(8);
            pss^.count1 := 0;
            pss^.items1 := nil;
            ppss^ := pss;
            HandleLiveForever(map);
          end else
            pss := ppss^;
          AddProcess(mutex);
          AddApi;
          UnmapViewOfFile(ppss);
        end;
        CloseHandle(map);
      end;
    finally ReleaseMutex(mutex) end;
    CloseHandle(mutex);
  end;
end;

procedure Del9xSharedStub(pid_: dword; api_, hook_: pointer);
var mutex  : dword;
    map    : dword;
    ppss   : ^TP9xSharedStubs;
    b1, b2 : boolean;
    i1, i2 : integer;
begin
  {$ifdef log}
    log('Del9xSharedStub (pid: '  + IntToHexEx(pid_) +
                       '; api: '  + IntToHexEx(dword(api_)) +
                       '; hook: ' + IntToHexEx(dword(hook_)) +
                         ')');
  {$endif}
  mutex := OpenMutexA(SYNCHRONIZE, false, PAnsiChar(DecryptStr(CMAHStubs) + DecryptStr(CMutex)));
  if mutex <> 0 then begin
    WaitForSingleObject(mutex, INFINITE);
    try
      map := OpenFileMappingA(FILE_MAP_ALL_ACCESS, false, PAnsiChar(DecryptStr(CMAHStubs) + DecryptStr(CMap)));
      if map <> 0 then begin
        ppss := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if ppss <> nil then begin
          with ppss^^ do begin
            b1 := true;
            for i1 := 0 to count1 - 1 do
              with items1^[i1] do begin
                b2 := true;
                for i2 := 0 to count2 - 1 do
                  with items2^[i2] do
                    if (api = api_) and (pid = pid_) and (hook = hook_) then
                      items2^[i2].pid := 0
                    else
                      if items2^[i2].pid <> 0 then
                        b2 := false;
                if b2 then begin
                  if items2 <> nil then
                    SharedMem9x_Free(items2);
                  count2 := 0;
                  items2 := nil;
                end else
                  if count2 > 0 then
                    b1 := false;
              end;
            if b1 then begin
              if items1 <> nil then
                SharedMem9x_Free(items1);
              count1 := 0;
              items1 := nil;
            end;
          end;
          UnmapViewOfFile(ppss);
        end;
        CloseHandle(map);
      end;
    finally ReleaseMutex(mutex) end;
    CloseHandle(mutex);
  end;
end;

// ***************************************************************

const CInUseCount = 280;

type
  TInUseArr = packed array [0..CInUseCount - 1] of packed record
    push    : byte;     // $68
    retaddr : pointer;
    lock    : byte;     // $f0
    and_    : word;     // $2583
    andaddr : pointer;
    zero    : byte;     // $00
    ret     : byte;     // $c3
  end;

  TCodeHook = class
  public
    FValid        : boolean;
    FNewHook      : boolean;
    FHookedFunc   : pointer;
    FSharedHook   : boolean;
    FCallbackFunc : pointer;
    FPNextHook    : TPPointer;
    FShareCode    : pointer;
    FShareStub    : ^TSharedMemoryStub;
    FPHookStub    : ^THookStub;
    FSystemWide   : boolean;
    FInUseArr     : ^TInUseArr;
    FDestroying   : boolean;
    FLeakUnhook   : boolean;
    FModuleH      : dword;
    FSafeHooking  : boolean;
    FNoImprUnhook : boolean;
    FIsWinsock2   : boolean;
    FTramp        : pointer;
    FPatchAddr    : ^TAbsoluteJmp;
    FNewCode      : TAbsoluteJmp;
    FOldCode      : TAbsoluteJmp;

    constructor Create (moduleH: dword; api: AnsiString; hookThisFunc, callbackFunc: pointer; out nextHook: pointer; flags: dword);
    destructor Destroy; override;
    procedure WritePatch;
    function IsHookInUse : integer;
  end;

var
  SharedMemoryStub : TSharedMemoryStub =
    (pushEax                 :          $50;
     callGetCurrentProcessID : (opcode: $E8; relTarget:  0);
     cmpEax                  : (opcode: $3D; cmpValue :  0);
     popEax                  :          $58;
     jnz5                    : (opcode: $75; relTarget: 10);
     jmpCallbackFunc         : (absJump:     (opcode: $FF; modRm: $25; target: nil); absTarget: nil);
     jmpNextHook             : (jmpNextHook: (opcode: $FF; modRm: $25; target: nil); absTarget: nil));

procedure InUseStub;
asm
  push edi
  push edx
  push ecx
  push eax
  mov edi, $11111111
  mov edx, [esp+$10]
  mov ecx, CInUseCount
  xor eax, eax
 @loop:
  lock cmpxchg [edi+1], edx
  jz @success
  add edi, 14
  xor eax, eax
  loop @loop
  jmp @quit
 @success:
  mov [esp+$10], edi
 @quit:
  pop eax
  pop ecx
  pop edx
  pop edi
  db $e9
  dd $22222222
end;

var
  gdi32addr  : TDAPointer;
  user32addr : TDAPointer;
  tapi32addr : TDAPointer;

function Is9xMixtureApi(module: dword; addr: pointer) : boolean;

  function CheckModule(moduleName: AnsiString; var addrs: TDAPointer) : boolean;
  type TMchI9xMA = packed record
                     name  : array [0..7] of AnsiChar;
                     count : integer;
                     apis  : TPACardinal;
                   end;
  var ed             : PImageExportDirectory;
      i1, i2, i3, i4 : integer;
      fis            : array of TFunctionInfo;
      b1             : boolean;
      c1             : dword;
      map            : dword;
      newMap         : boolean;
      buf            : ^TMchI9xMA;
      s1             : AnsiString;
  begin
    result := false;
    if GetModuleHandleA(PAnsiChar(moduleName)) = module then begin
      if addrs = nil then begin
        s1 := DecryptStr(CMchI9xMA);
        map := CreateFileMappingA(dword(-1), nil, PAGE_READWRITE, 0, sizeOf(buf^), PAnsiChar(s1 + '.' + moduleName));
        if map <> 0 then begin
          newMap := GetLastError <> ERROR_ALREADY_EXISTS;
          buf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
          if buf <> nil then begin
            for i1 := 1 to 50 do
              if newMap or (TPInt64(@buf^.name)^ = TPInt64(s1)^) then
                   break
              else Sleep(50);
            if newMap or (TPInt64(@buf^.name)^ <> TPInt64(s1)^) then begin
              i4 := 0;
              ed := GetImageExportDirectory(module);
              if ed <> nil then
                with ed^ do begin
                  SetLength(fis,   NumberOfFunctions);
                  SetLength(addrs, NumberOfFunctions);
                  for i1 := 0 to NumberOfFunctions - 1 do
                    fis[i1] := ParseFunction(pointer(module + TPACardinal(module + AddressOfFunctions)^[i1]));
                  for i1 := 0 to high(fis) do
                    if fis[i1].IsValid and fis[i1].Interceptable then begin
                      b1 := false;
                      for i2 := 0 to high(fis) do
                        if i1 <> i2 then begin
                          for i3 := 0 to high(fis[i2].CodeAreas) do begin
                            c1 := dword(fis[i2].CodeAreas[i3].AreaBegin);
                            if (c1 > dword(fis[i1].EntryPoint)    ) and
                               (c1 < dword(fis[i1].EntryPoint) + 6) then begin
                              addrs[i4] := fis[i1].EntryPoint;
                              inc(i4);
                              break;
                            end;
                          end;
                          if b1 then
                            break;
                        end;
                    end;
                end;
              SetLength(addrs, i4);
              buf^.count := i4;
              buf^.apis  := AllocMemEx(i4 * 4);
              Move(addrs[0], buf^.apis^[0], i4 * 4);
              AtomicMove(pointer(s1), @buf^.name, 8);
              HandleLiveForever(map);
            end else
              if buf^.count > 0 then begin
                SetLength(addrs, buf^.count);
                Move(buf^.apis^[0], addrs[0], buf^.count * 4);
              end;
            UnmapViewOfFile(buf);
          end;
          CloseHandle(map);
        end;
      end;
      for i1 := 0 to high(addrs) do
        if addrs[i1] = addr then begin
          result := true;
          break;
        end;
    end;
  end;

begin
  {$ifdef log}
    log('Is9xMixtureApi (module: ' + IntToHexEx(module) + '; addr: ' + IntToHexEx(dword(addr)) + ')');
  {$endif}
  result := (GetVersion and $80000000 <> 0) and (module <> 0) and
            ( CheckModule(DecryptStr( CGdi32),  gdi32addr) or
              CheckModule(DecryptStr(CUser32), user32addr) or
              CheckModule(DecryptStr(CTapi32), tapi32addr)    );
  {$ifdef log}
    log('Is9xMixtureApi (module: ' + IntToHexEx(module) + '; addr: ' + IntToHexEx(dword(addr)) + ') -> ' + booleanToChar(result));
  {$endif}
end;

function Is9xDummyApi(module: dword; api: AnsiString; addr: pointer) : boolean;
var ed     : PImageExportDirectory;
    i1, i2 : integer;
    ci     : TCodeInfo;
begin
  {$ifdef log}
    log('IsDummyApi (module: ' + IntToHexEx(module) + '; api: "' + api + '"; addr: ' + IntToHexEx(dword(addr)) + ')');
  {$endif}
  result := false;
  if (GetVersion and $80000000 <> 0) and (module <> 0) and (api <> '') and
     ((module = kernel32handle) or
      (module = GetModuleHandleA(PAnsiChar(DecryptStr(CUser32))))) then begin
    ed := GetImageExportDirectory(module);
    if ed <> nil then
      with ed^ do begin
        i2 := 0;
        for i1 := 0 to NumberOfFunctions - 1 do
          if module + TPACardinal(module + AddressOfFunctions)^[i1] = dword(addr) then begin
            inc(i2);
            if i2 > 3 then begin
              result := true;
              break;
            end;
          end;
      end;
    if not result then begin
      ci := ParseCode(addr);
      ci := ParseCode(ci.Next);
      if ci.Opcode = $b1 then begin
        ci := ParseCode(ci.Next);
        try
          // kernel ordinal export 17 is for dummy APIs in win9x
          result := ci.Jmp and (ci.Target <> nil) and (word(ci.Target^) = $25ff) and
                    (pointer(TPPointer(dword(ci.Target) + 2)^^) = GetImageProcAddress(kernel32handle, 17));
        except end;
      end;            
    end;
  end;
  {$ifdef log}
    log('IsDummyApi (module: ' + IntToHexEx(module) + '; api: "' + api + '"; addr: ' + IntToHexEx(dword(addr)) + ') -> ' + booleanToChar(result));
  {$endif}
end;

function IsSharedPtr(module: dword; addr: pointer) : boolean;
// is the specified address shared between all processes?  (winNT always false)
var nh  : PImageNtHeaders;
    ish : PImageSectionHeader;
    i1  : integer;
begin
  {$ifdef log}
    log('IsSharedPtr (module: ' + IntToHexEx(module) + '; addr: ' + IntToHexEx(dword(addr)) + ')');
  {$endif}
  if GetVersion and $80000000 <> 0 then begin
    result := dword(addr) >= $80000000;
    if (not result) and (module <> 0) then begin
      nh := GetImageNtHeaders(module);
      if nh <> nil then begin
        dword(ish) := dword(nh) + sizeOf(TImageNtHeaders);
        for i1 := 0 to integer(nh^.FileHeader.NumberOfSections) - 1 do begin
          if (dword(addr) >= (module + ish^.VirtualAddress                     )) and
             (dword(addr) <  (module + ish^.VirtualAddress + ish^.SizeOfRawData)) then begin
            result := ish^.Characteristics and IMAGE_SCN_MEM_SHARED <> 0;
            break;
          end;
          inc(ish);
        end;
      end;
    end;
  end else
    result := false;
  {$ifdef log}
    log('IsSharedPtr (module: ' + IntToHexEx(module) + '; addr: ' + IntToHexEx(dword(addr)) + ') -> ' + booleanToChar(result));
  {$endif}
end;

function WasCodeChanged(module: dword; code: pointer; out orgCode: TAbsoluteJmp) : boolean;

  function ApplyRelocation(nh: PImageNtHeaders; module: dword; code, arr: pointer; size: dword) : boolean;
  type TImageBaseRelocation = record
         VirtualAddress : dword;
         SizeOfBlock    : dword;
       end;
  var
    rel1 : ^TImageBaseRelocation;
    rel2 : dword;
    i1   : integer;
    item : TPWord;
    data : ^TPPointer;
  begin
    result := false;
    with nh^.OptionalHeader do begin
      dword(rel1) := module + DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
      rel2 := dword(rel1) + DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
      while (dword(rel1) < rel2) and (rel1^.VirtualAddress <> 0) do begin
        dword(item) := dword(rel1) + sizeOf(rel1^);
        for i1 := 1 to (rel1.SizeOfBlock - sizeOf(rel1^)) div 2 do begin
          if item^ and $f000 = $3000 then begin
            dword(data) := module + rel1.VirtualAddress + item^ and $0fff;
            if (dword(data) >= dword(code)) and (dword(data) < dword(code) + size) then begin
              data := pointer(dword(arr) + (dword(data) - dword(code)));
              dword(data^) := dword(data^) - ImageBase + module;
            end;
          end;
          inc(item);
        end;
        dword(rel1) := dword(rel1) + rel1^.SizeOfBlock;
      end;
    end;
  end;

  function IsCompressedModule(nh1, nh2: PImageNtHeaders) : boolean;
  var sh1 : PImageSectionHeader;
  begin
    dword(sh1) := dword(@nh1^.OptionalHeader) + sizeOf(TImageOptionalHeader);
    result := (nh1^.OptionalHeader.BaseOfCode   <> nh2^.OptionalHeader.BaseOfCode  ) or
              (nh1^.OptionalHeader.BaseOfData   <> nh2^.OptionalHeader.BaseOfData  ) or
              (nh1^.FileHeader.NumberOfSections <> nh2^.FileHeader.NumberOfSections) or
              ((sh1^.Name[0] = ord('U')) and (sh1^.Name[1] = ord('P')) and (sh1^.Name[2] = ord('X')));
  end;

var buf      : pointer;
    nh1, nh2 : PImageNtHeaders;
    c1       : dword;
    arr      : array [0..13] of byte;
begin
  result := false;
  ZeroMemory(@orgCode, sizeOf(orgCode));
  if module <> 0 then begin
    nh1 := GetImageNtHeaders(module);
    if (nh1 <> nil) and (dword(code) > module) and (dword(code) < module + GetSizeOfImage(nh1)) then begin
      buf := CollectCache_NeedModuleFileMap(module);
      if buf <> nil then begin
        c1 := VirtualToRaw(nh1, dword(code) - module);
        nh2 := GetImageNtHeaders(dword(buf));
        if (nh2 <> nil) and (c1 < GetSizeOfImage(nh2)) then
          try
            result := (not IsCompressedModule(nh1, nh2)) and
                      ( (dword(pointer(dword(code) + 0)^) <> dword(pointer(dword(buf) + c1 + 0)^)) or
                        ( word(pointer(dword(code) + 4)^) <>  word(pointer(dword(buf) + c1 + 4)^))    );
            if result then begin
              Move(pointer(dword(buf) + c1 - 4)^, arr[0], sizeOf(arr));
              ApplyRelocation(nh2, module, pointer(dword(code) - 4), @arr, sizeOf(arr) - 4);
              Move(arr[4], orgCode, sizeOf(orgCode));
              result := (dword(pointer(dword(@arr[4]) + 0)^) <> dword(pointer(dword(code) + 0)^)) or
                        ( word(pointer(dword(@arr[4]) + 4)^) <>  word(pointer(dword(code) + 4)^));
            end;
          except end;
      end;
    end;
  end;
end;

constructor TCodeHook.Create(moduleH: dword; api: AnsiString; hookThisFunc, callbackFunc: pointer; out nextHook: pointer; flags: dword);

  function CheckMap(var map: dword) : boolean;
  var buf : ^pointer;
  begin
    if map <> 0 then begin
      // there is a map, but is it filled?
      // in win2k the Process Explorer duplicates one of our map handles  :-(
      buf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
      if buf <> nil then begin
        if buf^ = nil then begin
          CloseHandle(map);
          map := 0;
        end;
        UnmapViewOfFile(buf);
      end else begin
        CloseHandle(map);
        map := 0;
      end;
    end; 
    result := map <> 0;
  end;

var cc1, cc2      : pointer;
    c1            : cardinal;
    ci            : TCodeInfo;
    mutex         : dword;
    mutex2        : dword;
    map           : cardinal;
    buf           : pointer;
    s1, s2, s3    : AnsiString;
    p1, p2        : pointer;
    fi            : TFunctionInfo;
    i1            : integer;
    tableStub     : ^THookStub;
    pa            : pointer;
    pp            : TPPointer;
    error         : dword;
    inUseStubSize : integer;
    keepMapFile   : boolean;
    orgCode       : TAbsoluteJmp;
begin
  error := 0;
  FPHookStub := nil;
  CollectCache_AddRef;
  try
    if GetVersion and $80000000 <> 0 then
      InitSharedMem9x(@@SharedMem9x_Alloc, @@SharedMem9x_Free);
    nextHook := hookThisFunc;
    FModuleH := moduleH;
    FSafeHooking := flags and SAFE_HOOKING <> 0;
    FNoImprUnhook := flags and NO_IMPROVED_SAFE_UNHOOKING <> 0;
    FInUseArr := nil;
    inUseStubSize := ParseFunction(@InUseStub).CodeLen;
    FPHookStub := VirtualAlloc2(sizeOf(FPHookStub^) + sizeOf(TInUseArr) + inUseStubSize);
    FPHookStub^.jmpNextHook.opcode := $FF;
    FPHookStub^.jmpNextHook.modRm  := $25;
    FPHookStub^.jmpNextHook.target := @FPHookStub^.absTarget;
    FHookedFunc   := hookThisFunc;
    pa            := FHookedFunc;
    FCallbackFunc := callbackFunc;
    FPNextHook    := @nextHook;
    keepMapFile   := false;
    {$ifdef log}
      log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)));
    {$endif}
    s1 := ApiSpecialName(CMAHPrefix, FHookedFunc, false);
    s2 := ApiSpecialName(CMAHPrefix, FHookedFunc, true);
    if GetVersion and $80000000 <> 0 then begin
      if dword(FHookedFunc) >= $80000000 then begin
        s1 := s2;
        s2 := '';
      end;
    end else
      s2 := '';
    mutex := CreateLocalMutex(PAnsiChar(DecryptStr(CMutex) + ', ' + s1));
    if s2 <> '' then
         mutex2 := CreateLocalMutex(PAnsiChar(DecryptStr(CMutex) + ', ' + s2))
    else mutex2 := 0;
    try
      if mutex  <> 0 then WaitForSingleObject(mutex,  INFINITE);
      if mutex2 <> 0 then WaitForSingleObject(mutex2, INFINITE);
      try
        map := OpenGlobalFileMapping(PAnsiChar(DecryptStr(CNamedBuffer) + ', ' + s1), true);
        FNewHook := not CheckMap(map);
        if FNewHook and (s2 <> '') then begin
          map := OpenGlobalFileMapping(PAnsiChar(DecryptStr(CNamedBuffer) + ', ' + s2), true);
          FNewHook := not CheckMap(map);
          if not FNewHook then
            s1 := s2;
        end;
        try
          if FNewHook then begin
            if Is9xDummyApi(moduleH, api, FHookedFunc) then begin
              {$ifdef log}
                log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) + ' -> dummy api');
              {$endif}
              error := CErrorNo_InvalidCode;
              exit;
            end;
            if flags and MIXTURE_MODE = 0 then
              if Is9xMixtureApi(moduleH, FHookedFunc) then
                flags := flags or MIXTURE_MODE
              else begin
                fi := ParseFunction(FHookedFunc);
                if (not fi.IsValid) or (not fi.Interceptable) then begin
                  {$ifdef log}
                    log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) +
                        ' -> not interceptable -> MIXTURE');
                    ParseFunction(FHookedFunc, s3);
                    log(s3);
                  {$endif}
                  flags := flags or MIXTURE_MODE;
                end else
                  if (not DisableChangedCodeCheck) and WasCodeChanged(moduleH, FHookedFunc, orgCode) then begin
                    {$ifdef log}
                      log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) +
                          ' -> already hooked by another hooking library? -> MIXTURE');
                      ParseFunction(FHookedFunc, s3);
                      log(s3);
                    {$endif}
                    flags := flags or MIXTURE_MODE;
                    error := CErrorNo_DoubleHook;
                  end;
              end;
            if flags and MIXTURE_MODE = 0 then begin
              FSharedHook := IsSharedPtr(moduleH, FHookedFunc);
              if FSharedHook and (s2 <> '') then
                s1 := s2;
            end else
              if flags and NO_MIXTURE_MODE = 0 then begin
                if moduleH <> 0 then
                  if GetVersion and $80000000 = 0 then
                       FIsWinsock2 := GetModuleHandleW(pointer(AnsiToWideEx(DecryptStr(CWs2_32)))) = moduleH
                  else FIsWinsock2 := GetModuleHandleA(pointer             (DecryptStr(CWs2_32)) ) = moduleH;
                if FIsWinsock2 and (flags and ALLOW_WINSOCK2_MIXTURE_MODE <> 0) then
                     p1 := FindWs2InternalProcList(moduleH)
                else p1 := nil;
                if (not FIsWinsock2) or (p1 <> nil) then begin
                  FSharedHook := (GetVersion and $80000000 <> 0) and
                                 ( (dword(FHookedFunc) >= $80000000) or
                                   ( (moduleH <> 0) and
                                     IsSharedPtr(moduleH, GetImageExportDirectory(moduleH)) ) );
                  if FSharedHook then begin
                    if s2 <> '' then
                      s1 := s2;
                    tableStub := AllocMemEx(sizeOf(THookStub));
                  end else
                    tableStub := VirtualAlloc2(sizeOf(THookStub));
                  {$ifdef log}
                    log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) +
                        ' -> MIXTURE -> ' + IntToHexEx(dword(tableStub)));
                  {$endif}
                  tableStub^.jmpNextHook.opcode := $FF;
                  tableStub^.jmpNextHook.modRm  := $25;
                  tableStub^.jmpNextHook.target := @tableStub^.absTarget;
                  tableStub^.absTarget          := FHookedFunc;
                  CheckProcAddress := ResolveMixtureMode;
                  s3 := ApiSpecialName(CMixPrefix, tableStub, FSharedHook);
                  map := CreateLocalFileMapping(PAnsiChar(DecryptStr(CNamedBuffer) + ', ' + s3), 4);
                  if map <> 0 then begin
                    buf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
                    if buf <> nil then begin
                      pointer(buf^) := FHookedFunc;
                      UnmapViewOfFile(buf);
                    end else begin
                      CloseHandle(map);
                      map := 0;
                    end;
                  end;
                  if (map <> 0) and
                     ( PatchExportTable(moduleH, FHookedFunc, tableStub, p1) or
                       ((FHookedFunc = GetKernelEntryPoint) and PatchKernelEntryPoint(tableStub)) ) then begin
                    if FSharedHook then begin
                      HandleLiveForever(map);
                      CloseHandle(map);
                      if FHookedFunc <> GetKernelEntryPoint then begin
                        InstallMixturePatcher(FHookedFunc, tableStub);
                        CollectCache_Activate(flags and SYSTEM_WIDE_9X <> 0);
                      end;
                    end;
                    if FIsWinsock2 then
                      // we've hooked Winsock2 by using the mixture mode
                      // in order to improve the efficieny of our manipulations
                      // we're increasing the load count of the Winsock2 dll
                      // this way we won't have to redo the hook all the time
                      // if the application unloads and reloads the dll
                      if GetVersion and $80000000 = 0 then
                           LoadLibraryW(pointer(AnsiToWideEx(DecryptStr(CWs2_32))))
                      else LoadLibraryA(pointer             (DecryptStr(CWs2_32)) );
                    try
                      PatchMyImportTables(FHookedFunc, tableStub);
                    except end;
                    pa := tableStub;
                    keepMapFile := true;
                    error := 0;
                    {$ifdef log}
                      log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) +
                          ' -> MIXTURE -> ' + IntToHexEx(dword(tableStub)) + ' -> done');
                    {$endif}
                  end else begin
                    {$ifdef log}
                      log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) +
                          ' -> MIXTURE -> ' + IntToHexEx(dword(tableStub)) + ' -> error!');
                    {$endif}
                    if map <> 0 then
                      CloseHandle(map);
                    if FSharedHook then
                         FreeMemEx(tableStub)
                    else VirtualFree(tableStub, 0, MEM_RELEASE);
                    if error = 0 then
                      error := CErrorNo_CodeNotInterceptable;
                    exit;
                  end;
                end else begin
                  (*ci := ParseCode(FHookedFunc);
                  if ci.Jmp and
                     ( (ci.Opcode in [$e9..$eb]) or
                       ((ci.Opcode = $ff) and (ci.ModRm and $30 = $20)) ) and
                     (ci.Target <> nil) then begin
                    fi := ParseFunction(ci.Target);
                    if fi.IsValid and fi.Interceptable then begin
                      {$ifdef log}
                        log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) +
                            ' -> ws2_32.dll was already hooked -> we hooked the other hook''s callback function!');
                      {$endif}
                      pa := ci.Target;
                      FSharedHook := false;
                    end else begin
                      {$ifdef log}
                        log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) +
                            ' -> MIXTURE -> ws2_32.dll can''t be hooked in mixture mode!');
                      {$endif}
                      error := CErrorNo_CodeNotInterceptable;
                      exit;
                    end;
                  end else begin  *)
                    {$ifdef log}
                      log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) +
                          ' -> MIXTURE -> ws2_32.dll can''t be hooked in mixture mode!');
                    {$endif}
                    error := CErrorNo_CodeNotInterceptable;
                    exit;
//                  end;
                end;
              end else begin
                {$ifdef log}
                  log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) +
                      ' -> MIXTURE -> NO_MIXTURE_MODE flag specified!');
                {$endif}
                error := CErrorNo_CodeNotInterceptable;
                exit;
              end;
          end else
            FSharedHook := s1[6] = 'S';
          FSystemWide := FSharedHook and (flags and SYSTEM_WIDE_9X <> 0);
          {$ifdef log}
            log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) +
                '; FSharedHook: ' + booleanToChar(FSharedHook) +
                '; FSystemWide: ' + booleanToChar(FSystemWide));
          {$endif}
          if FSystemWide then
            if dword(FCallbackFunc) < $80000000 then begin
              FCallbackFunc := CopyFunction(FCallbackFunc, 0, flags and ACCEPT_UNKNOWN_TARGETS_9X <> 0, @FShareCode, @fi);
              if FCallbackFunc = nil then begin
                error := CErrorNo_BadFunction;
                exit;
              end;
              for i1 := 0 to high(fi.FarCalls) do
                with fi.FarCalls[i1] do
                  if (not RelTarget) and (PPTarget <> nil) and (PPTarget^ = FPNextHook) then begin
                    pp := pointer(int64(cardinal(PPTarget)) - (int64(cardinal(fi.CodeBegin)) - int64(cardinal(FShareCode))));
                    if FSystemWide or UnprotectMemory(pp, 4) then
                      pp^ := PAnsiChar(FShareCode) + fi.CodeLen;
                  end;
              FPHookStub^.jmpNextHook.target := pointer(PAnsiChar(FShareCode) + fi.CodeLen);
            end else begin
              fi := ParseFunction(FCallbackFunc);
              if not fi.IsValid then begin
                error := CErrorNo_BadFunction;
                exit;
//                raise MadException.Create(fi.LastErrorStr) at fi.LastErrorAddr;
              end;
              for i1 := 0 to high(fi.FarCalls) do
                with fi.FarCalls[i1] do
                  if (not RelTarget) and (PPTarget <> nil) and (PPTarget^ = FPNextHook) then begin
                    if FShareCode = nil then begin
                      FShareCode := AllocMemEx(4);
                      FPHookStub^.jmpNextHook.target := FShareCode;
                    end;
                    PPTarget^ := FShareCode;
                  end;
            end;
          buf := nil;
          try
            if FNewHook then begin
              {$ifdef log}
                log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) + ' -> new hook');
              {$endif}
              if FSharedHook then
                FTramp := AllocMemEx(30 + 4)
              else
                if GetVersion and $80000000 <> 0 then
                  FTramp := pointer(LocalAlloc(LPTR, 30 + 4))
                else
                  FTramp := VirtualAlloc2(30 + 4);
              cc1 := pa;
              cc2 := FTramp;
              repeat
                ci := ParseCode(cc1);
                if ci.RelTarget then begin
                  if ci.TargetSize = 1 then begin
                    if ci.Opcode <> $EB then begin
                      // ($70 - $7F)  1 byte Jcc calls -> 4 byte Jcc calls
                      TPAByte(cc2)^[0] := $0F;
                      TPAByte(cc2)^[1] := $80 + ci.Opcode and $F;
                      cardinal(cc2) := cardinal(cc2) + 6;
                    end else begin
                      TPByte(cc2)^ := $E9;
                      cardinal(cc2) := cardinal(cc2) + 5;
                    end;
                  end else if ci.TargetSize = 2 then begin
                    // kill 16 bit prefix
                    c1 := cardinal(ci.Next) - cardinal(cc1);
                    Move(pointer(cardinal(cc1) + 1)^, cc2^, c1 - 3);
                    cardinal(cc2) := cardinal(cc2) + c1 + 1;
                  end else begin
                    c1 := cardinal(ci.Next) - cardinal(cc1);
                    Move(cc1^, cc2^, c1);
                    cardinal(cc2) := cardinal(cc2) + c1;
                  end;
                  TPInteger(cardinal(cc2) - 4)^ := int64(cardinal(ci.Target)) - int64(cardinal(cc2));
                end else begin
                  c1 := cardinal(ci.Next) - cardinal(cc1);
                  Move(cc1^, cc2^, c1);
                  cardinal(cc2) := cardinal(cc2) + c1;
                end;
                cc1 := ci.Next;
              until cardinal(cc1) - cardinal(pa) >= 6;
              TPByte(cc2)^ := $E9;
              cardinal(cc2) := cardinal(cc2) + 5;
              TPInteger(cardinal(cc2) - 4)^ := int64(cardinal(cc1)) - int64(cardinal(cc2));
              map := CreateLocalFileMapping(PAnsiChar(DecryptStr(CNamedBuffer) + ', ' + s1), 4);
              if map <> 0 then begin
                if FSharedHook then
                  HandleLiveForever(map);
                buf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
                if buf <> nil then begin
                  if FSharedHook then
                       TPPointer(buf)^ := AllocMemEx   (CHookQueueSize + 8 * CHookEntrySize)
                  else TPPointer(buf)^ := VirtualAlloc2(CHookQueueSize + 8 * CHookEntrySize);
                  with THookQueue(TPPointer(buf)^^) do begin
                    ItemCount := 2;
                    Capacity  := 8;
                    PatchAddr := pa;
                    OldCode   := PatchAddr^;
                    NewCode.opcode  := $FF;
                    NewCode.modRm   := $25;
                    NewCode.target  := pointer(cardinal(FTramp) + 30);
                    NewCode.target^ := FTramp;
                    if (not FSharedHook) and (not keepMapFile) then
                         MapHandle := map
                    else MapHandle := 0;
                    Items[0].HookProc  := nil;
                    Items[0].PNextHook := pointer(cardinal(FTramp) + 30);
                    Items[1].HookProc  := FTramp;
                    Items[1].PNextHook := nil;
                  end;
                end;
              end;
            end else begin
              buf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
              with THookQueue(TPPointer(buf)^^) do begin
                {$ifdef log}
                  log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) + ' -> hook no ' + IntToStrEx(ItemCount - 1));
                {$endif}
                FTramp := Items[ItemCount - 1].HookProc;
                if ItemCount = Capacity then begin
                  if FSharedHook then
                       p1 := AllocMemEx   (CHookQueueSize + Capacity * 2 * CHookEntrySize)
                  else p1 := VirtualAlloc2(CHookQueueSize + Capacity * 2 * CHookEntrySize);
                  Move(TPPointer(buf)^^, p1^, CHookQueueSize + Capacity * CHookEntrySize);
                  THookQueue(p1^).Capacity := Capacity * 2;
                  p2 := TPPointer(buf)^;
                  TPPointer(buf)^ := p1;
                  if FSharedHook then FreeMemEx(p2)
                  else                VirtualFree(p2, 0, MEM_RELEASE);
                end;
              end;
            end;
            if not FSystemWide then begin
              if flags and NO_SAFE_UNHOOKING = 0 then begin
                dword(FInUseArr) := dword(FPHookStub) + sizeOf(FPHookStub^);
                for i1 := 0 to high(FInUseArr^) do
                  with FInUseArr^[i1] do begin
                    push    := $68;
                    retAddr := nil;
                    lock    := $f0;
                    and_    := $2583;
                    andAddr := @retAddr;
                    zero    := 0;
                    ret     := $c3;
                  end;
                dword(p1) := dword(FInUseArr) + sizeOf(TInUseArr);
                Move(InUseStub, p1^, inUseStubSize);
                TPCardinal(dword(p1) + dword(PosPChar(PAnsiChar(#$11#$11#$11#$11), PAnsiChar(p1), 4, inUseStubSize)))^ := dword(FInUseArr); // AH: Added type casting
                TPCardinal(dword(p1) + dword(PosPChar(PAnsiChar(#$22#$22#$22#$22), PAnsiChar(p1), 4, inUseStubSize)))^ := dword(FCallbackFunc) - dword(p1) - dword(inUseStubSize); // AH: Added type casting
                FCallbackFunc := p1;
              end;
              if FSharedHook then begin
                FShareStub := AllocMemEx(sizeOf(TSharedMemoryStub));
                FShareStub^ := SharedMemoryStub;
                FShareStub^.callGetCurrentProcessID.relTarget :=
                  int64(cardinal(KernelProc(CGetCurrentProcessId, true))) -
                  int64(cardinal(@FShareStub^.callGetCurrentProcessID) + 5);
                FShareStub^.cmpEax.cmpValue := GetCurrentProcessID;
                FShareStub^.jmpCallbackFunc.absJump.target := @FShareStub^.jmpCallbackFunc.absTarget;
                FShareStub^.jmpCallbackFunc.absTarget := FCallbackFunc;
                FShareStub^.jmpNextHook.jmpNextHook.target := @FShareStub^.jmpNextHook.absTarget;
                FPHookStub^.jmpNextHook.target := @FShareStub^.jmpNextHook.absTarget;
                Add9xSharedStub(GetCurrentProcess, GetCurrentProcessID, FHookedFunc, FShareStub);
              end;
            end;
            FPNextHook^ := FPHookStub;
            with THookQueue(TPPointer(buf)^^) do begin
              FPatchAddr := pointer(PatchAddr);
              FNewCode   := NewCode;
              FOldCode   := OldCode;
              Items[ItemCount] := Items[ItemCount - 1];
              if FSharedHook and (not FSystemWide) then
                   Items[ItemCount - 1].HookProc := FShareStub
              else Items[ItemCount - 1].HookProc := FCallbackFunc;
              Items[ItemCount - 1].PNextHook  := FPHookStub^.jmpNextHook.target;
              Items[ItemCount - 1].PNextHook^ := Items[ItemCount    ].HookProc;
              Items[ItemCount - 2].PNextHook^ := Items[ItemCount - 1].HookProc;
              inc(ItemCount);
            end;
            FValid := true;
            WritePatch;
            {$ifdef log}
              log('  FHookedFunc: ' + IntToHexEx(dword(FHookedFunc)) + ' -> done');
            {$endif}
          finally
            if buf <> nil then
              UnmapViewOfFile(buf);
          end;
        finally
          if (map <> 0) and ((not FNewHook) or FSharedHook) then
            CloseHandle(map);
        end;
      finally
        if mutex  <> 0 then ReleaseMutex(mutex );
        if mutex2 <> 0 then ReleaseMutex(mutex2);
      end;
    finally
      if mutex  <> 0 then CloseHandle(mutex );
      if mutex2 <> 0 then CloseHandle(mutex2);
    end;
  finally
    CollectCache_Release;
    SetLastError(error);
  end;
end;

var
  NtQuerySystemInformation  : function (infoClass: cardinal; buffer: pointer; bufSize: cardinal;
                                        returnSize: TPCardinal) : cardinal; stdcall = nil;
  FlushInstructionCacheProc : function (process: dword; baseAddress: pointer; size: dword) : bool; stdcall = nil;
  NtOpenThreadProc          : function (var hThread: cardinal; access: cardinal;
                                        objAttr, clientId: pointer) : cardinal; stdcall = nil;
  QueryFullProcessImageName : function (process, flags: dword; exeName: PWideChar; var size: dword) : bool; stdcall = nil;

function GetOtherThreadHandles : TDACardinal;
// get a list of thread ids (undocumented)
const
  THREAD_ALL_ACCESS = STANDARD_RIGHTS_REQUIRED or SYNCHRONIZE or $3FF;
  CNtObjAttr : array [0..5] of cardinal = ($18, 0, 0, 0, 0, 0);
type
  TNtProcessInfo = record
    offset     : cardinal;
    numThreads : cardinal;
    d1         : array [2..16] of cardinal;
    pid        : cardinal;
    d3         : array [18..42] of cardinal;
    threads    : array [0..maxInt shr 7 - 1] of record
                   tidNt4 : cardinal;
                   d5     : array [44..54] of cardinal;
                   tidNt5 : cardinal;
                   d6     : array [56..58] of cardinal;
                 end;
  end;
var
  c1, c2 : cardinal;
  p1     : pointer;
  npi    : ^TNtProcessInfo;
  i1     : integer;
  cid    : array [0..1] of cardinal;
begin
  result := nil;
  if @NtQuerySystemInformation = nil then
    NtQuerySystemInformation := NtProc(CNtQuerySystemInformation);
  if @NtOpenThreadProc = nil then
    NtOpenThreadProc := NtProc(CNtOpenThread);
  if (@NtQuerySystemInformation <> nil) and (@NtOpenThreadProc <> nil) then begin
    c1 := 0;
    NtQuerySystemInformation(5, nil, 0, @c1);
    p1 := nil;
    try
      if c1 = 0 then begin
        c1 := $10000;
        repeat
          c1 := c1 * 2;
          LocalFree(dword(p1));
          dword(p1) := LocalAlloc(LPTR, c1);
          c2 := NtQuerySystemInformation(5, p1, c1, nil);
        until (c2 = 0) or (c1 = $400000);
      end else begin
        c1 := c1 * 2;
        dword(p1) := LocalAlloc(LPTR, c1);
        c2 := NtQuerySystemInformation(5, p1, c1, nil);
      end;
      if c2 = 0 then begin
        npi := p1;
        while true do begin
          if npi^.pid = GetCurrentProcessId then
            for i1 := 0 to npi^.numThreads - 1 do begin
              if OS.winNtEnum > osWinNt4 then
                   c1 := npi^.threads[i1].tidNt5
              else c1 := npi^.threads[i1].tidNt4;
              if c1 <> GetCurrentThreadId then begin
                cid[0] := 0;
                cid[1] := c1;
                if NtOpenThreadProc(c1, THREAD_ALL_ACCESS, @CNtObjAttr, @cid) = 0 then begin
                  SetLength(result, Length(result) + 1);
                  result[high(result)] := c1;
                end;
              end;
            end;
          if npi^.offset = 0 then break;
          npi := pointer(cardinal(npi) + npi^.offset);
        end;
      end;
    finally LocalFree(dword(p1)) end;
  end;
end;

procedure FlushInstructionCache(code: pointer; size: dword);
begin
  if GetVersion and $80000000 = 0 then begin
    if @FlushInstructionCacheProc = nil then
      FlushInstructionCacheProc := KernelProc(CFlushInstructionCache);
    if @FlushInstructionCacheProc <> nil then
      FlushInstructionCacheProc(GetCurrentProcess, code, size);
  end;
end;

var OldEip, NewEip : dword;
    RtlDispatchExceptionNext : function (excRec: PExceptionRecord; excCtxt: PContext) : integer; stdcall;
    AddVectoredExceptionHandler : function (firstHandler: dword; handler: pointer) : pointer; stdcall;
    RemoveVectoredExceptionHandler : function (handler: pointer) : dword; stdcall;

procedure TCodeHook.WritePatch;
var rdecMutex : dword;

  function VectoredExceptionHandler(var exceptInfo: TExceptionPointers) : integer; stdcall;
  begin
    if (exceptInfo.ContextRecord.eip = OldEip) and
       ( (exceptInfo.ExceptionRecord.ExceptionCode = STATUS_PRIVILEGED_INSTRUCTION  ) or
         ( (exceptInfo.ExceptionRecord.ExceptionCode = STATUS_ACCESS_VIOLATION) and
           (exceptInfo.ExceptionRecord.ExceptionInformation[0] = 0            ) and
           (exceptInfo.ExceptionRecord.ExceptionInformation[1] = $ffffffff    )     )    ) then begin
      exceptInfo.ContextRecord.eip := NewEip;
      result := -1;
    end else
      result := 0;
  end;

  function InitRtlDispatchException(oeip, neip: pointer) : boolean;

    function RtlDispatchExceptionCallback(var excRec: TExceptionRecord; var excCtxt: TContext) : integer; stdcall;
    begin
      if (excCtxt.eip = OldEip) and
         ( (excRec.ExceptionCode = STATUS_PRIVILEGED_INSTRUCTION  ) or
           ( (excRec.ExceptionCode = STATUS_ACCESS_VIOLATION) and
             (excRec.ExceptionInformation[0] = 0            ) and
             (excRec.ExceptionInformation[1] = $ffffffff    )     )    ) then begin
        excCtxt.eip := NewEip;
        result := -1;
      end else
        result := RtlDispatchExceptionNext(@excRec, @excCtxt);
    end;

  var c1, c2 : dword;
      s1     : AnsiString;
  begin
    result := false;
    AddVectoredExceptionHandler := KernelProc(CAddVecExceptHandler);
    RemoveVectoredExceptionHandler := KernelProc(CRemoveVecExceptHandler);
    SetLength(s1, 5);
    s1[1] := 'r';  // Rtl
    s1[2] := 'd';  // Dispatch
    s1[3] := 'e';  // Exception
    s1[4] := 'c';  // Callback
    s1[5] := ' ';
    s1 := DecryptStr(CMutex) + ', ' + s1 + IntToHexEx(GetCurrentProcessId);
    rdecMutex := CreateMutexA(nil, false, PAnsiChar(s1));
    if rdecMutex <> 0 then begin
      WaitForSingleObject(rdecMutex, INFINITE);
      OldEip := dword(oeip);
      NewEip := dword(neip);
      if (@AddVectoredExceptionHandler = nil) or (@RemoveVectoredExceptionHandler = nil) then begin
        c1 := dword(NtProc(CKiUserExceptionDispatcher, true));
        if (dword(pointer(c1    )^)               = $04244c8b) and  // mov  ecx, [esp+4] ; PContext
           (dword(pointer(c1 + 4)^) and $00ffffff =   $241c8b) and  // mov  ebx, [esp+0] ; PExceptionRecord
           (byte (pointer(c1 + 7)^)               =       $51) and  // push ecx
           (byte (pointer(c1 + 8)^)               =       $53) and  // push ebx
           (byte (pointer(c1 + 9)^)               =       $e8) and  // call RtlDispatchException
           VirtualProtect(pointer(c1 + 10), 4, PAGE_EXECUTE_READWRITE, @c2) then begin
          RtlDispatchExceptionNext := pointer(c1 + 14 + dword(pointer(c1 + 10)^));
          dword(pointer(c1 + 10)^) := dword(@RtlDispatchExceptionCallback) - c1 - 14;
          VirtualProtect(pointer(c1 + 10), 4, c2, @c2);
          result := true;
        end;
      end else
        result := AddVectoredExceptionHandler(1, @VectoredExceptionHandler) <> nil;
      if not result then begin
        ReleaseMutex(rdecMutex);
        CloseHandle(rdecMutex);
      end;
    end;
  end;

  procedure DoneRtlDispatchException;
  var c1, c2 : dword;
  begin
    if (@AddVectoredExceptionHandler = nil) or (@RemoveVectoredExceptionHandler = nil) then begin
      c1 := dword(NtProc(CKiUserExceptionDispatcher, true));
      if VirtualProtect(pointer(c1 + 10), 4, PAGE_EXECUTE_READWRITE, @c2) then begin
        dword(pointer(c1 + 10)^) := dword(@RtlDispatchExceptionNext) - c1 - 14;
        VirtualProtect(pointer(c1 + 10), 4, c2, @c2);
      end;
      ReleaseMutex(rdecMutex);
      CloseHandle(rdecMutex);
    end else
      RemoveVectoredExceptionHandler(@VectoredExceptionHandler);
  end;

var
  s1     : AnsiString;
  mutex  : dword;
  map    : dword;
  buf    : pointer;
  pa     : pointer;
  b1, b2 : boolean;
  hlt    : byte;
  tl     : TDACardinal;
  i1     : integer;
  ctxt   : TContext;
  c1, c2 : dword;
  ci     : TCodeInfo;
  p1     : pointer;
begin
  tl := nil;
  if FValid and (not FDestroying) then begin
    pa := nil;
    s1 := ApiSpecialName(CMAHPrefix, FHookedFunc, FSharedHook);
    mutex := CreateLocalMutex(PAnsiChar(DecryptStr(CMutex) + ', ' + s1));
    if mutex <> 0 then
      try
        WaitForSingleObject(mutex, INFINITE);
        try
          map := OpenGlobalFileMapping(PAnsiChar(DecryptStr(CNamedBuffer) + ', ' + s1), true);
          if map <> 0 then
            try
              buf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
              if buf <> nil then
                try
                  with THookQueue(TPPointer(buf)^^) do
                    if (TPCardinal(dword(PatchAddr)    )^ <> TPCardinal(dword(@NewCode)    )^) or
                       (TPWord    (dword(PatchAddr) + 4)^ <> TPWord    (dword(@NewCode) + 4)^) then begin
                      pa := PatchAddr;
                      b1 := IsMemoryProtected(PatchAddr);
                      if (not b1) or UnprotectMemory(PatchAddr, 8) then begin
                        ci := ParseCode(PatchAddr);
                        b2 := FSafeHooking and (GetVersion and $80000000 = 0) and
                              (dword(ci.Next) - dword(ci.This) < 6) and
                              InitRtlDispatchException(PatchAddr, FTramp);
                        if b2 then begin
                          hlt := $f4;  // asm HLT instruction
                          if not AtomicMove(@hlt, PatchAddr, 1) then
                            byte(pointer(PatchAddr)^) := hlt;
                          FlushInstructionCache(PatchAddr, 1);
                          tl := GetOtherThreadHandles;
                          for i1 := 0 to high(tl) do begin
                            while true do begin
                              ctxt.ContextFlags := CONTEXT_CONTROL;
                              if (not GetThreadContext(tl[i1], ctxt)) or
                                 (ctxt.Eip <= dword(PatchAddr)) or
                                 (ctxt.Eip >= dword(PatchAddr) + 6) then
                                break;
                              Sleep(10);
                            end;
                            CloseHandle(tl[i1]);
                          end;
                        end;
                        if not AtomicMove(@NewCode, PatchAddr, sizeOf(NewCode)) then
                          PatchAddr^ := NewCode;
                        FlushInstructionCache(PatchAddr, sizeOf(NewCode));
                        if b2 then begin
                          c1 := dword(NtProc(CKiUserExceptionDispatcher, true));
                          c2 := c1 + 14 + dword(pointer(c1 + 10)^);
                          tl := GetOtherThreadHandles;
                          for i1 := 0 to high(tl) do begin
                            while true do begin
                              ctxt.ContextFlags := CONTEXT_CONTROL;
                              if (not GetThreadContext(tl[i1], ctxt)) or
                                 ( (ctxt.Eip <> dword(PatchAddr)) and
                                   ( (ctxt.Eip < c1) or (ctxt.Eip > c1 + 10) ) and
                                   (ctxt.Eip and $ffff0000 <> c2 and $ffff0000)    ) then
                                break;
                              Sleep(10);
                            end;
                            CloseHandle(tl[i1]);
                          end;
                        end;
                        if b1 then
                          ProtectMemory(PatchAddr, 8);
                        if b2 then
                          DoneRtlDispatchException;
                      end;
                    end;
                finally UnmapViewOfFile(buf) end;
            finally CloseHandle(map) end;
          if (pa <> nil) and (FHookedFunc <> pa) then begin
            if FIsWinsock2 then
                 p1 := FindWs2InternalProcList(FModuleH)
            else p1 := nil;
            if (not FIsWinsock2) or (p1 <> nil) then
              PatchExportTable(FModuleH, FHookedFunc, pa, p1);
          end;
        finally ReleaseMutex(mutex) end;
      finally CloseHandle(mutex) end;
  end;
end;

function TCodeHook.IsHookInUse : integer;
var i1 : integer;
begin
  result := 0;
  if FValid and (FInUseArr <> nil) then
    for i1 := 0 to high(FInUseArr^) do
      if FInUseArr^[i1].retaddr <> nil then
        inc(result);
end;

destructor TCodeHook.Destroy;
var mutex     : cardinal;
    map       : cardinal;
    buf       : pointer;
    s1        : AnsiString;
    i1, i2    : integer;
    b1, b2    : boolean;
    tramp     : pointer;
    queue     : pointer;
    mbi       : TMemoryBasicInformation;
    tl        : TDACardinal;
    ctxt      : TContext;
    inUseSize : dword;
begin
  FDestroying := true;
  tl := nil;
  if FValid then begin
    tramp := nil;
    queue := nil;
    if FSharedHook and (not FSystemWide) then
      Del9xSharedStub(GetCurrentProcessID, FHookedFunc, FShareStub);
    s1 := ApiSpecialName(CMAHPrefix, FHookedFunc, FSharedHook);
    mutex := CreateLocalMutex(PAnsiChar(DecryptStr(CMutex) + ', ' + s1));
    if mutex <> 0 then
      try
        WaitForSingleObject(mutex, INFINITE);
        try
          map := OpenGlobalFileMapping(PAnsiChar(DecryptStr(CNamedBuffer) + ', ' + s1), true);
          if map <> 0 then
            try
              buf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
              if buf <> nil then
                try
                  with THookQueue(TPPointer(buf)^^) do
                    for i1 := 1 to ItemCount - 2 do
                      if ((      FSharedHook and (not FSystemWide)   and (Items[i1].HookProc = FShareStub   )) or
                          ((not (FSharedHook and (not FSystemWide))) and (Items[i1].HookProc = FCallbackFunc))    ) and
                         (Items[i1].PNextHook = FPHookStub^.jmpNextHook.target) then begin
                        for i2 := i1 to ItemCount - 2 do
                          Items[i2] := Items[i2 + 1];
                        Items[i1 - 1].PNextHook^ := Items[i1].HookProc;
                        dec(ItemCount);
                        if (ItemCount = 2) and (MapHandle <> 0) then begin
                          if (not IsBadReadPtr(PatchAddr, sizeOf(OldCode))) and
                             (PatchAddr.opcode = NewCode.opcode) and
                             (PatchAddr.modRm  = NewCode.modRm ) and
                             (PatchAddr.target = NewCode.target) then begin
                            b1 := IsMemoryProtected(PatchAddr);
                            if UnprotectMemory(PatchAddr, 8) then begin
                              if not AtomicMove(@OldCode, PatchAddr, sizeOf(OldCode)) then
                                PatchAddr^ := OldCode;
                              FlushInstructionCache(PatchAddr, sizeOf(OldCode));
                              if b1 then
                                ProtectMemory(PatchAddr, 8);
                              tramp := Items[1].HookProc;
                            end;
                          end;
                          queue := TPPointer(buf)^;
                          TPPointer(buf)^ := nil;
                          CloseHandle(MapHandle);
                        end;
                        break;
                      end;
                finally UnmapViewOfFile(buf) end;
            finally CloseHandle(map) end;
        finally ReleaseMutex(mutex) end;
      finally CloseHandle(mutex) end;
    if FPNextHook <> nil then
      FPNextHook^ := FHookedFunc;
    if FInUseArr <> nil then begin
      if (not FNoImprUnhook) and (GetVersion and $80000000 = 0) then begin
        tl := GetOtherThreadHandles;
        inUseSize := sizeOf(TInUseArr) + ParseFunction(@InUseStub).CodeLen;
      end else
        inUseSize := 0;
      try
        b1 := false;
        while true do begin
          b2 := true;
          for i2 := 0 to high(tl) do begin
            ctxt.ContextFlags := CONTEXT_CONTROL;
            if GetThreadContext(tl[i2], ctxt) and
               (ctxt.Eip >= dword(FInUseArr)) and
               (ctxt.Eip < dword(FInUseArr) + inUseSize) then begin
              b2 := false;
              break;
            end;
          end;
          for i2 := 0 to high(FInUseArr^) do
            if FInUseArr^[i2].retaddr <> nil then begin
              b2 := false;
              break;
            end;
          if b2 then break;
          b1 := true;
          if FLeakUnhook then
            exit;
          Sleep(500);
        end;
      finally
        for i2 := 0 to high(tl) do
          CloseHandle(tl[i2]);
      end;
      if b1 then
        Sleep(500);
    end;
    if tramp <> nil then
      if (GetVersion and $80000000 <> 0) or
         (VirtualQuery(tramp, mbi, sizeOf(mbi)) <> sizeOf(mbi)) or
         (mbi.Protect <> PAGE_EXECUTE_READWRITE) then
        LocalFree(dword(tramp))
      else
        VirtualFree(tramp, 0, MEM_RELEASE);
    if queue <> nil then
      if FSharedHook then FreeMemEx(queue)
      else                VirtualFree(queue, 0, MEM_RELEASE);
    if FShareStub <> nil then begin
      FreeMemEx(FShareStub);
      FShareStub := nil;
    end;
    if FShareCode <> nil then begin
      FreeMemEx(FShareCode);
      FShareCode := nil;
    end;
  end;
  VirtualFree(FPHookStub, 0, MEM_RELEASE);
  FInUseArr := nil;
  FPHookStub := nil;
end;

type
  THookItem = record
                owner    : dword;
                moduleH  : dword;
                module   : AnsiString;
                moduleW  : AnsiString;
                api      : AnsiString;
                code     : pointer;
                ch       : TCodeHook;
                nextHook : TPPointer;
                callback : pointer;
                flags    : dword;
              end;

var
  HookSection : TRTLCriticalSection;
  HookReady   : boolean = false;
  HookList    : array of THookItem;

var LoadLibraryANextHook   : function (lib: PAnsiChar) : dword stdcall = nil;
    LoadLibraryExANextHook : function (lib: PAnsiChar; file_, flags: dword) : dword stdcall = nil;
    LoadLibraryExWNextHook : function (lib: PWideChar; file_, flags: dword) : dword stdcall = nil;
    LdrLoadDllNextHook     : function (path: dword; flags: TPCardinal; var name: TUnicodeStr; var handle: dword) : dword stdcall = nil;
    LoadLibraryHooked      : boolean = false;
    LoadLibraryExWDone     : boolean = false;
    LoadLibraryCallsNtDll  : pointer = nil;

procedure CheckHooks(dll: dword); forward;

function LoadLibraryACallbackProc(lib: PAnsiChar) : dword; stdcall;
var module : dword;
    le     : dword;
begin
  le := GetLastError;
  try
    module := GetModuleHandleA(lib);
  except module := 0 end;
  SetLastError(le);
  result := LoadLibraryANextHook(lib);
  if result <> module then
    CheckHooks(result);
end;

function LoadLibraryExACallbackProc(lib: PAnsiChar; file_, flags: dword) : dword; stdcall;
var module : dword;
    le     : dword;
begin
  le := GetLastError;
  try
    module := GetModuleHandleA(lib);
  except module := 0 end;
  SetLastError(le);
  result := LoadLibraryExANextHook(lib, file_, flags);
  if (result <> 0) and (result <> module) and (flags and LOAD_LIBRARY_AS_DATAFILE = 0) then
    CheckHooks(result);
end;

function LoadLibraryExWCallbackProc(lib: PWideChar; file_, flags: dword) : dword; stdcall;
var module : dword;
    le     : dword;
begin
  le := GetLastError;
  try
    module := GetModuleHandleW(lib);
  except module := 0 end;
  SetLastError(le);
  result := LoadLibraryExWNextHook(lib, file_, flags);
  if (result <> 0) and (result <> module) and (flags and LOAD_LIBRARY_AS_DATAFILE = 0) then
    CheckHooks(result);
end;

function LdrLoadDllCallbackProc(path: dword; flags: TPCardinal; var name: TUnicodeStr; var handle: dword) : dword stdcall;
var module : dword;
    le     : dword;
begin
  le := GetLastError;
  try
    module := GetModuleHandleW(name.str);
  except module := 0 end;
  SetLastError(le);
  result := LdrLoadDllNextHook(path, flags, name, handle);
  if (handle <> 0) and (handle <> module) then
    CheckHooks(handle);
end;

procedure HookLoadLibrary;
type TMchLLEW = packed record
                  name : array [0..7] of AnsiChar;
                  jmp  : ^THookStub;
                end;
type THookStubEx = packed record
                     jmpNextHook : TAbsoluteJmp;  // $FF / $25 / @absTarget
                     absTarget   : pointer;       // absolute target
                     self        : pointer;
                   end;
var map    : dword;
    newMap : boolean;
    buf1   : ^TMchLLEW;
    buf2   : ^THookStubEx;
    s1     : AnsiString;
    i1     : integer;
    c1     : dword;
    fi     : TFunctionInfo;
    p1     : pointer;
    b1     : boolean;
begin
  if not LoadLibraryHooked then begin
    if GetVersion and $80000000 <> 0 then begin
      HookCodeX(0, 0, '', '', FindRealCode(KernelProc(CLoadLibraryA,   true)), @LoadLibraryACallbackProc,   @LoadLibraryANextHook  );
      HookCodeX(0, 0, '', '', FindRealCode(KernelProc(CLoadLibraryExA, true)), @LoadLibraryExACallbackProc, @LoadLibraryExANextHook);
    end else begin
      if not LoadLibraryExWDone then begin
        LoadLibraryExWDone := true;
        s1 := DecryptStr(CMchLLEW2);
        map := CreateFileMappingA(dword(-1), nil, PAGE_READWRITE, 0, sizeOf(TMchLLEW), PAnsiChar(s1 + IntToHexEx(GetCurrentProcessId)));
        if map <> 0 then begin
          newMap := GetLastError <> ERROR_ALREADY_EXISTS;
          buf1 := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
          if buf1 <> nil then begin
            for i1 := 1 to 50 do
              if newMap or (TPInt64(@buf1^.name)^ = TPInt64(s1)^) then
                   break
              else Sleep(50);
            if newMap or (TPInt64(@buf1^.name)^ <> TPInt64(s1)^) then begin
              buf2 := VirtualAlloc(nil, sizeOf(THookStubEx), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
              p1 := NtProc(CLdrLoadDll, true);
              fi := ParseFunction(KernelProc(CLoadLibraryExW, true));
              for i1 := 0 to high(fi.FarCalls) do
                if fi.FarCalls[i1].Target = p1 then begin
                  if (fi.FarCalls[i1].PTarget <> nil) or (fi.FarCalls[i1].PPTarget <> nil) then begin
                    buf1^.jmp := pointer(buf2);
                    buf2^.jmpNextHook.opcode := $ff;
                    buf2^.jmpNextHook.modRm  := $25;
                    buf2^.jmpNextHook.target := @buf2^.absTarget;
                    buf2^.absTarget := p1;
                    buf2^.self := buf2;
                    if fi.FarCalls[i1].PTarget <> nil then
                      p1 := fi.FarCalls[i1].PTarget
                    else
                      p1 := fi.FarCalls[i1].PPTarget;
                    b1 := IsMemoryProtected(p1);
                    if (not b1) or UnprotectMemory(p1, 4) then begin
                      if fi.FarCalls[i1].RelTarget then
                        c1 := dword(buf2) - dword(fi.FarCalls[i1].CodeAddr1) - 5
                      else
                        c1 := dword(@buf2^.self);
                      if not AtomicMove(@c1, p1, 4) then
                        dword(p1^) := c1;
                      if b1 then
                        ProtectMemory(p1, 4);
                      LoadLibraryCallsNtDll := buf2;
                      map := 0;
                    end;
                  end;
                  break;
                end;
              if LoadLibraryCallsNtDll = nil then begin
                UnmapViewOfFile(buf1);
                VirtualFree(buf2, 0, MEM_RELEASE);
              end else
                TPInt64(@buf1^.name)^ := TPInt64(s1)^;
            end else begin
              LoadLibraryCallsNtDll := buf1^.jmp;
              UnmapViewOfFile(buf1);
            end;
          end;
          if map <> 0 then
            CloseHandle(map);
        end;
      end;
      if LoadLibraryCallsNtDll <> nil then
        HookCodeX(0, 0, '', '', LoadLibraryCallsNtDll, @LdrLoadDllCallbackProc, @LdrLoadDllNextHook)
      else
        HookCodeX(0, 0, '', '', FindRealCode(KernelProc(CLoadLibraryExW, true)), @LoadLibraryExWCallbackProc, @LoadLibraryExWNextHook);
    end;
    LoadLibraryHooked := true;
  end;
end;

function UnhookCodeEx (var nextHook: pointer; dontUnhookHelperHooks, wait: boolean) : boolean; forward;

procedure UnhookLoadLibrary(wait: boolean);
begin
  if LoadLibraryHooked then begin
    if @LoadLibraryANextHook   <> nil then UnhookCodeEx(@LoadLibraryANextHook,   true, wait);
    if @LoadLibraryExANextHook <> nil then UnhookCodeEx(@LoadLibraryExANextHook, true, wait);
    if @LoadLibraryExWNextHook <> nil then UnhookCodeEx(@LoadLibraryExWNextHook, true, wait);
    if @LdrLoadDllNextHook     <> nil then UnhookCodeEx(@LdrLoadDllNextHook,     true, wait);
    LoadLibraryHooked := false;
  end;
end;

var AutoUnhookMap : dword = 0;

{$ifdef unlocked}
function HookCodeX(owner        : dword;
                   moduleH      : dword;
                   module       : AnsiString;
                   api          : AnsiString;
                   code         : pointer;
                   callbackFunc : pointer;
                   out nextHook : pointer;
                   flags        : dword = 0) : boolean;

{$else}
var forbiddenMutex : dword = 0;
function HookCodeX_(owner        : dword;
                    moduleH      : dword;
                    module       : AnsiString;
                    api          : AnsiString;
                    code         : pointer;
                    callbackFunc : pointer;
                    out nextHook : pointer;
                    flags        : dword = 0) : boolean;
{$endif}
var ch1 : TCodeHook;
    c1  : dword;
    b1  : boolean;
    s1  : AnsiString;
    i1  : integer;
    map : dword;
    buf : TPPointer;
begin
  result := false;
  {$ifdef log}
    SetLength(s1, MAX_PATH + 1);
    GetModuleFileNameA(owner, PAnsiChar(s1), MAX_PATH);
    s1 := PAnsiChar(s1);
    log('HookCodeX (owner: '    + IntToHexEx(owner) + ' (' + s1 + ')' +
                 '; moduleH: '  + IntToHexEx(moduleH) +
                 '; module: '   + module +
                 '; api: '      + api +
                 '; code: '     + IntToHexEx(dword(code)) +
                 '; callback: ' + IntToHexEx(dword(callbackFunc)) +
                 '; nextHook: ' + IntToHexEx(dword(@nextHook)) +
                 '; flags: '    + IntToHexEx(flags) +
                   ')');
  {$endif}
  InitProcs(true);
  SetLastError(0);
  InitUnprotectMemory;
  if code <> nil then begin
    try
      if moduleH = 0 then begin
        FindModule(code, c1, s1);
        ch1 := TCodeHook.Create(c1, api, code, callbackFunc, nextHook, flags);
      end else
        ch1 := TCodeHook.Create(moduleH, api, code, callbackFunc, nextHook, flags);
      if not ch1.FValid then begin
        c1 := GetLastError;
        ch1.Free;
        SetLastError(c1);
        ch1 := nil;
        moduleH := 0;
      end;
    except ch1 := nil end;
  end else
    ch1 := nil;
  if (ch1 <> nil) or ((code = nil) and (module <> '')) then begin
    result := true;
    if not HookReady then begin
      InitializeCriticalSection(HookSection);
      HookReady := true;
    end;
    EnterCriticalSection(HookSection);
    try
      c1 := Length(HookList);
      SetLength(HookList, c1 + 1);
      HookList[c1].owner    := owner;
      HookList[c1].moduleH  := moduleH;
      HookList[c1].module   := module;
      HookList[c1].moduleW  := AnsiToWideEx(module);
      HookList[c1].api      := api;
      HookList[c1].code     := code;
      HookList[c1].ch       := ch1;
      HookList[c1].nextHook := @nextHook;
      HookList[c1].callback := callbackFunc;
      HookList[c1].flags    := flags;
      b1 := (owner <> 0) and (not LoadLibraryHooked);
    finally LeaveCriticalSection(HookSection) end;
    if b1 then begin
      HookLoadLibrary;
      s1 := DecryptStr(CAutoUnhookMap) + IntToHexEx(GetCurrentProcessID, 8);
      if amMchDll then begin
        i1 := Length(s1);
        SetLength(s1, i1 + 4);
        s1[i1 + 1] := '$';
        s1[i1 + 2] := 'm';
        s1[i1 + 3] := 'c';
        s1[i1 + 4] := 'h';
      end else
        s1 := s1 + IntToHexEx(HInstance, 8);
      map := CreateLocalFileMapping(PAnsiChar(s1), 4);
      if map <> 0 then begin
        buf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if buf <> nil then begin
          buf^ := @AutoUnhook;
          UnmapViewOfFile(buf);
          AutoUnhookMap := map;
        end else
          CloseHandle(map);
      end;
    end;
  end;
  if result then
    SetLastError(0);
  {$ifdef log}
    SetLength(s1, MAX_PATH + 1);
    GetModuleFileNameA(owner, PAnsiChar(s1), MAX_PATH);
    s1 := PAnsiChar(s1);
    log('HookCodeX (owner: '    + IntToHexEx(owner) + ' (' + s1 + ')' +
                 '; moduleH: '  + IntToHexEx(moduleH) +
                 '; module: '   + module +
                 '; api: '      + api +
                 '; code: '     + IntToHexEx(dword(code)) +
                 '; callback: ' + IntToHexEx(dword(callbackFunc)) +
                 '; nextHook: ' + IntToHexEx(dword(@nextHook)) +
                 '; flags: '    + IntToHexEx(flags) +
                   ') -> ' + booleanToChar(result));
  {$endif}
end;

{$ifndef unlocked}
  function HookCodeX(owner        : dword;
                     moduleH      : dword;
                     module       : AnsiString;
                     api          : AnsiString;
                     code         : pointer;
                     callbackFunc : pointer;
                     out nextHook : pointer;
                     flags        : dword = 0) : boolean;
  var FindHookCodeX3Addr : dword;

    procedure JmpRel;
    asm
      push dword ptr [esp]
      add [esp+4], eax
    end;

    function HookCodeX3(owner        : dword;
                        moduleH      : dword;
                        module       : AnsiString;
                        api          : AnsiString;
                        code         : pointer;
                        callbackFunc : pointer;
                        out nextHook : pointer;
                        flags        : dword = 0) : boolean;
    begin
      result := false;
    end;

    procedure HookCodeX2;
    asm
      db $82
      db $0d
      jmp HookCodeX_
    end;

    function FindHookCodeX5(api: AnsiString; ofs: integer) : pointer;
    begin
      result := pointer(dword(@HookCodeX3) - 2);
    end;

    function FindHookCodeX4(api: AnsiString; ofs: integer) : pointer;

      function CmpHookThis(api: AnsiString) : bool;
      begin
        result := (api <> DecryptStr(CProcess32Next           )) and
                  (api <> DecryptStr(CEnumServicesStatusA     )) and
                  (api <> DecryptStr(CEnumServicesStatusW     )) and
                  (api <> DecryptStr(CNtQuerySystemInformation));
      end;

    asm
      cmp ofs, 0
      jz @Data
      push ofs
      call CmpHookThis
      cmp eax, 0
      jnz @Ok
      pop eax
      mov eax, offset HookCodeX3
      sub eax, 2
      ret
      @Ok:
      pop ecx
      mov eax, offset @Data
      add eax, ecx
      mov eax, [eax]
      ret
      @Data:
      db $82
      db $1d
      db $00
      dd offset HookCodeX2
    end;

    procedure FindHookCodeX3;
    asm
      cmp eax, 0
      jnz @zeroParam
      mov eax, 8
      call JmpRel
      @zeroParam:
      mov eax, offset FindHookCodeX5
      ret
      db $82
      db $05
      mov eax, offset FindHookCodeX4
    end;

    procedure FindHookCodeX2;
    asm
      mov [esp], eax
      mov eax, [eax]
      xor [esp], eax
      xor eax, eax
    end;

    function FindHookCodeX1(param: pointer) : dword;
    asm
      call FindHookCodeX2
    end;

  var FindHookCode1 : function (api: AnsiString; offset: integer) : dword;
      FindHookCode2 : function (owner        : dword;
                                moduleH      : dword;
                                module       : AnsiString;
                                api          : AnsiString;
                                code         : pointer;
                                callbackFunc : pointer;
                                out nextHook : pointer;
                                flags        : dword = 0) : boolean;
  begin
    FindHookCodeX3Addr := dword(@FindHookCodeX3Addr) xor dword(@FindHookCodeX3);
    FindHookCode1 := pointer(FindHookCodeX1(@FindHookCodeX3Addr));
    FindHookCode2 := pointer(FindHookCode1(api, 3) + 2);
    result := FindHookCode2(owner, moduleH, module, api, code, callbackFunc, nextHook, flags);
  end;

  procedure CheckForbiddenAPIs(api: AnsiString);
  begin
    if (api = DecryptStr(CProcess32Next2      )) or
       (api = DecryptStr(CEnumServicesStatusA2)) or
       (api = DecryptStr(CEnumServicesStatusW2)) or
       (api = DecryptStr(CNtQuerySystemInfo2  )) then
      if (forbiddenMutex = 0) and (not AmSystemProcess) and AmUsingInputDesktop then begin
        forbiddenMutex := CreateMutex(nil, false, 'forbiddenAPIsMutex');
        if GetLastError <> ERROR_ALREADY_EXISTS then
          MessageBoxA(0, PAnsiChar('You''ve tried to hook one of the following APIs: ' + #$D#$A + #$D#$A +
                                   '- ' + DecryptStr(CProcess32Next2) + #$D#$A +
                                   '- ' + DecryptStr(CEnumServicesStatusA2) + #$D#$A +
                                   '- ' + DecryptStr(CEnumServicesStatusW2) + #$D#$A +
                                   '- ' + DecryptStr(CNtQuerySystemInfo2) + #$D#$A + #$D#$A +
                                   'These APIs are usually hooked in order to hide a ' + #$D#$A +
                                   'process. Of course madCodeHook can do that just ' + #$D#$A +
                                   'fine. But I don''t want virus/trojan writers to misuse ' + #$D#$A +
                                   'madCodeHook for illegal purposes. So I''ve decided ' + #$D#$A +
                                   'to not allow these APIs to be hooked.' + #$D#$A + #$D#$A +
                                   'If you absolutely have to hook these APIs, and if ' + #$D#$A +
                                   'you have a commercial madCodeHook license, you ' + #$D#$A +
                                   'may contact me.'),
                     'madCodeHook warning...', 0);
      end;
  end;
{$endif}

function HookCode(code         : pointer;
                  callbackFunc : pointer;
                  out nextHook : pointer;
                  flags        : dword = 0) : bool; stdcall;
var s1, s2 : AnsiString;
    module : dword;
begin
  result := false;
  {$ifdef log}
    log('HookCode (code: '          + IntToHexEx(dword(code)) +
                '; callback: '      + IntToHexEx(dword(callbackFunc)) +
                '; nextHook: '      + IntToHexEx(dword(@nextHook)) +
                '; flags: '         + IntToHexEx(flags) +
                  ')');
  {$endif}
  if code <> nil then begin
    code := FindRealCode(code);
    s1 := '';
    if FindModule(code, module, s1) then begin
      s2 := GetImageProcName(module, code, false);
      if s2 = '' then begin
        module := 0;
        s1 := '';
      end;
    end else
      s2 := '';
    result := HookCodeX(GetCallingModule, module, s1, s2, code, callbackFunc, nextHook, flags);
    {$ifndef unlocked}
      CheckForbiddenAPIs(s2);
    {$endif}
  end else
    SetLastError(ERROR_INVALID_PARAMETER);
  {$ifdef log}
    log('HookCode (code: '          + IntToHexEx(dword(code)) +
                '; callback: '      + IntToHexEx(dword(callbackFunc)) +
                '; nextHook: '      + IntToHexEx(dword(@nextHook)) +
                '; flags: '         + IntToHexEx(flags) +
                  ') -> ' + booleanToChar(result));
  {$endif}
end;

function HookAPI(module, api  : PAnsiChar;
                 callbackFunc : pointer;
                 out nextHook : pointer;
                 flags        : dword = 0) : bool; stdcall;
var dll  : dword;
    code : pointer;
    s2   : AnsiString;
begin
  result := false;
  {$ifdef log}
    log('HookCode (module: '        + module +
                '; api: '           + api +
                '; callback: '      + IntToHexEx(dword(callbackFunc)) +
                '; nextHook: '      + IntToHexEx(dword(@nextHook)) +
                '; flags: '         + IntToHexEx(flags) +
                  ')');
  {$endif}
  if (module <> '') and (api <> '') then begin
    if GetVersion and $80000000 = 0 then
         dll := GetModuleHandleW(pointer(AnsiToWideEx(module)))
    else dll := GetModuleHandleA(                     module  );
    if dll <> 0 then begin
      code := GetImageProcAddress(dll, api, true);
      if code <> nil then begin
        code := FindRealCode(code);
        s2 := GetImageProcName(dll, code, false);
        if s2 <> '' then
          api := PAnsiChar(s2);
      end;
    end else
      code := nil;
    result := HookCodeX(GetCallingModule, dll, module, api, code, callbackFunc, nextHook, flags);
    {$ifndef unlocked}
      CheckForbiddenAPIs(s2);
    {$endif}
  end else
    SetLastError(ERROR_INVALID_PARAMETER);
  {$ifdef log}
    log('HookCode (module: '        + module +
                '; api: '           + api +
                '; callback: '      + IntToHexEx(dword(callbackFunc)) +
                '; nextHook: '      + IntToHexEx(dword(@nextHook)) +
                '; flags: '         + IntToHexEx(flags) +
                  ') -> ' + booleanToChar(result));
  {$endif}
end;

function RenewHook(var nextHook: pointer) : bool; stdcall;
var {$ifdef CheckRecursion}
      recTestBuf : array [0..3] of dword;
    {$endif}
    ch         : TCodeHook;
    i1         : integer;
    le         : dword;
begin
  result := false;
  le := GetLastError;
  {$ifdef CheckRecursion}
    if NotRecursedYet(@RenewHook, @recTestBuf) then begin
  {$endif}
    {$ifdef log}
      log('RenewHook (nextHook: ' + IntToHexEx(dword(@nextHook)) + ')');
    {$endif}
    if HookReady then begin
      ch := nil;
      EnterCriticalSection(HookSection);
      try
        for i1 := high(HookList) downto 0 do
          if HookList[i1].nextHook = @nextHook then begin
            result := true;
            ch := HookList[i1].ch;
            break;
          end;
      finally LeaveCriticalSection(HookSection) end;
      if ch <> nil then
        ch.WritePatch;
    end;
    {$ifdef log}
      log('RenewHook (nextHook: ' + IntToHexEx(dword(@nextHook)) + ') -> ' + booleanToChar(result));
    {$endif}
    {$ifdef CheckRecursion}
      recTestBuf[3] := 0;
    end;
  {$endif}
  SetLastError(le);
end;

function IsHookInUse(var nextHook: pointer) : integer; stdcall;
var {$ifdef CheckRecursion}
      recTestBuf : array [0..3] of dword;
    {$endif}
    ch         : TCodeHook;
    i1         : integer;
    le         : dword;
begin
  result := 0;
  le := GetLastError;
  {$ifdef CheckRecursion}
    if NotRecursedYet(@IsHookInUse, @recTestBuf) then begin
  {$endif}
    {$ifdef log}
      log('IsHookInUse (nextHook: ' + IntToHexEx(dword(@nextHook)) + ')');
    {$endif}
    if HookReady then begin
      ch := nil;
      EnterCriticalSection(HookSection);
      try
        for i1 := high(HookList) downto 0 do
          if HookList[i1].nextHook = @nextHook then begin
            ch := HookList[i1].ch;
            break;
          end;
      finally LeaveCriticalSection(HookSection) end;
      if ch <> nil then
        result := ch.IsHookInUse;
    end;
    {$ifdef log}
      log('IsHookInUse (nextHook: ' + IntToHexEx(dword(@nextHook)) + ') -> ' + IntToStrEx(result));
    {$endif}
    {$ifdef CheckRecursion}
      recTestBuf[3] := 0;
    end;
  {$endif}
  SetLastError(le);
end;

function UnhookCodeEx(var nextHook: pointer; dontUnhookHelperHooks, wait: boolean) : boolean;
var ch     : TCodeHook;
    i1, i2 : integer;
    b1     : boolean;
begin
  result := false;
  {$ifdef log}
    log('UnhookCode (nextHook: '              + IntToHexEx(dword(@nextHook)) +
                  ', dontUnhookHelperHooks: ' + booleanToChar(dontUnhookHelperHooks) +
                  ', wait: '                  + booleanToChar(wait) + ')');
  {$endif}
  if HookReady then begin
    EnterCriticalSection(HookSection);
    try
      ch := nil;
      for i1 := high(HookList) downto 0 do
        if HookList[i1].nextHook = @nextHook then begin
          result := true;
          ch := HookList[i1].ch;
          i2 := high(HookList);
          HookList[i1] := HookList[i2];
          SetLength(HookList, i2);
          break;
        end;
      b1 := result and (not dontUnhookHelperHooks);
      if b1 then
        for i1 := 0 to high(HookList) do
          if HookList[i1].owner <> 0 then begin
            b1 := false;
            break;
          end;
    finally LeaveCriticalSection(HookSection) end;
    if result then begin
      if ch <> nil then begin
        ch.FLeakUnhook := not wait;
        ch.Free;
      end;
      if b1 then
        UnhookLoadLibrary(wait);
    end;
  end;
  {$ifdef log}
    log('UnhookCode (nextHook: '              + IntToHexEx(dword(@nextHook)) +
                  ', dontUnhookHelperHooks: ' + booleanToChar(dontUnhookHelperHooks) +
                  ', wait: '                  + booleanToChar(wait) + ') -> ' + booleanToChar(result));
  {$endif}
end;

function UnhookCode(var nextHook: pointer) : bool; stdcall;
begin
  result := UnhookCodeEx(nextHook, false, true);
end;

function UnhookAPI(var nextHook: pointer) : bool; stdcall;
begin
  result := UnhookCodeEx(nextHook, false, true);
end;

procedure CheckHooks(dll: dword);
var {$ifdef CheckRecursion}
      recTestBuf : array [0..3] of dword;
    {$endif}
    i1, i2     : integer;
    ach        : array of THookItem;
    c1         : dword;
    p1         : pointer;
    error      : dword;
    b1         : boolean;
begin
  error := GetLastError;
  {$ifdef CheckRecursion}
    if NotRecursedYet(@CheckHooks, @recTestBuf) then begin
  {$endif}
    {$ifdef log}
      log('CheckHooks (dll: ' + IntToHexEx(dll) + ')');
    {$endif}
    try
      if (dll <> 0) and HookReady then begin
        i2 := 0;
        EnterCriticalSection(HookSection);
        try
          SetLength(ach, Length(HookList));
          for i1 := 0 to high(HookList) do
            if HookList[i1].module <> '' then begin
              ach[i2] := HookList[i1];
              inc(i2);
            end;
        finally LeaveCriticalSection(HookSection) end;
        b1 := false;
        try
          for i1 := 0 to i2 - 1 do
            with ach[i1] do
              if (module <> '') and
                 ( (ch = nil) or
                   IsBadReadPtr(ch.FPatchAddr, sizeOf(ch.FNewCode)) or
                   (TPCardinal(dword(ch.FPatchAddr)    )^ <> TPCardinal(dword(@ch.FNewCode)    )^) or
                   (TPWord    (dword(ch.FPatchAddr) + 4)^ <> TPWord    (dword(@ch.FNewCode) + 4)^)    ) then begin
                if not b1 then begin
                  b1 := true;
                  CollectCache_AddRef;
                end;
                if GetVersion and $80000000 <> 0 then
                     c1 := GetModuleHandleA(PAnsiChar(module ))
                else c1 := GetModuleHandleW(pointer  (moduleW));
                p1 := FindRealCode(GetImageProcAddress(c1, PAnsiChar(api), true));
                if (ch = nil) or (code <> p1) then begin
                  UnhookCodeEx(nextHook^, true, true);
                  HookCodeX(owner, c1, module, api, p1, callback, nextHook^, flags);
                end else
                  if RenewOverwrittenHooks or
                     ( (not IsBadReadPtr(ch.FPatchAddr, sizeOf(ch.FOldCode))) and
                       (TPCardinal(dword(ch.FPatchAddr)    )^ = TPCardinal(dword(@ch.FOldCode)    )^) and
                       (TPWord    (dword(ch.FPatchAddr) + 4)^ = TPWord    (dword(@ch.FOldCode) + 4)^)     ) then
                    RenewHook(nextHook^);
              end;
        finally
          if b1 then
            CollectCache_Release;
        end;
      end;
    except end;
    {$ifdef log}
      log('CheckHooks (dll: ' + IntToHexEx(dll) + ') -> +');
    {$endif}
    {$ifdef CheckRecursion}
      recTestBuf[3] := 0;
    end;
  {$endif}
  SetLastError(error);
end;

procedure CollectHooks;
begin
  {$ifdef log}
    log('CollectHooks');
  {$endif}
  CollectCache_AddRef;
end;

procedure FlushHooks;
begin
  {$ifdef log}
    log('FlushHooks');
  {$endif}
  CollectCache_Release;
end;

function RestoreCode(code: pointer) : bool; stdcall;
var module  : dword;
    orgCode : TAbsoluteJmp;
    b1      : boolean;
    s1      : string;
begin
  {$ifdef log}
    log('RestoreCode (code: ' + IntToHexEx(dword(code)) + ')');
  {$endif}
  result := false;
  CollectCache_AddRef;
  try
    if FindModule(code, module, s1) and WasCodeChanged(module, code, orgCode) then begin
      InitUnprotectMemory;
      b1 := IsMemoryProtected(code);
      if (not b1) or UnprotectMemory(code, 8) then begin
        result := true;
        if not AtomicMove(@orgCode, code, sizeOf(orgCode)) then
          TAbsoluteJmp(code^) := orgCode;
        FlushInstructionCache(code, sizeOf(orgCode));
        if b1 then
          ProtectMemory(code, 8);
      end;
    end;
  finally CollectCache_Release end;
  {$ifdef log}
    log('RestoreCode (code: ' + IntToHexEx(dword(code)) + ') -> ' + booleanToChar(result));
  {$endif}
end;

// ***************************************************************

function AmUsingInputDesktop : bool; stdcall;
var {$ifdef CheckRecursion}
      recTestBuf : array [0..3] of dword;
    {$endif}
    s1, s2     : AnsiString;
    c1, c2     : dword;
    error      : dword;
begin
  error := GetLastError;
  {$ifdef CheckRecursion}
    result := false;
    if NotRecursedYet(@AmUsingInputDesktop, @recTestBuf) then begin
  {$endif}
    {$ifdef log}
      log('AmUsingInputDesktop');
    {$endif}
    if GetVersion and $80000000 = 0 then begin
      SetLength(s1, MAX_PATH + 1);
      s1[1] := #0;
      c1 := OpenInputDesktop(0, false, READ_CONTROL or GENERIC_READ);
      GetUserObjectInformationA(c1, UOI_NAME, PAnsiChar(s1), MAX_PATH, c2);
      CloseDesktop(c1);
      s1 := PAnsiChar(s1);
      SetLength(s2, MAX_PATH + 1);
      s2[1] := #0;
      c1 := GetThreadDesktop(GetCurrentThreadId);
      GetUserObjectInformationA(c1, UOI_NAME, PAnsiChar(s2), MAX_PATH, c2);
      CloseDesktop(c1);
      s2 := PAnsiChar(s2);
      result := (s1 <> '') and (s1 = s2);
    end else
      result := true;
    {$ifdef log}
      log('AmUsingInputDesktop -> ' + booleanToChar(result));
    {$endif}
    {$ifdef CheckRecursion}
      recTestBuf[3] := 0;
    end;
  {$endif}
  SetLastError(error);
end;

{$W+}

function GetCallingModule : dword; stdcall;
var {$ifdef CheckRecursion}
      recTestBuf : array [0..3] of dword;
    {$endif}
    ebp_       : dword;
    c1         : dword;
    mbi        : TMemoryBasicInformation;
    w1         : word;
    i1         : integer;
    error      : dword;
begin
  asm
    mov ebp_, ebp
  end;
  result := 0;
  error := GetLastError;
  {$ifdef CheckRecursion}
    if NotRecursedYet(@GetCallingModule, @recTestBuf) then begin
  {$endif}
    {$ifdef log}
      log('GetCallingModule');
    {$endif}
    if TryRead(pointer(ebp_), @ebp_, 4) and TryRead(pointer(ebp_ + 4), @c1, 4) then
      if (VirtualQuery(pointer(c1), mbi, sizeOf(mbi)) <> sizeOf(mbi)) or
         (not TryRead(mbi.AllocationBase, @w1, 2)) or (w1 <> CEMAGIC) then begin
        if HookReady then begin
          EnterCriticalSection(HookSection);
          try
            for i1 := high(HookList) downto 0 do
              if (HookList[i1].ch <> nil) and
                 (HookList[i1].ch.FInUseArr <> nil) and
                 (c1 >= dword(HookList[i1].ch.FInUseArr)) and
                 (c1 < dword(HookList[i1].ch.FInUseArr) + sizeOf(TInUseArr)) then begin
                if (VirtualQuery(pointer(dword(pointer(c1 + 1)^)), mbi, sizeOf(mbi)) = sizeOf(mbi)) and
                   TryRead(mbi.AllocationBase, @w1, 2) and (w1 = CEMAGIC) then
                  result := dword(mbi.AllocationBase);
                break;
              end;
          finally LeaveCriticalSection(HookSection) end;
        end;
      end else
        result := dword(mbi.AllocationBase);
    {$ifdef log}
      log('GetCallingModule -> ' + IntToHexEx(result));
    {$endif}
    {$ifdef CheckRecursion}
      recTestBuf[3] := 0;
    end;
  {$endif}
  SetLastError(error);
end;

{$W-}

function IsSystemProcess(processHandle: dword; sid: AnsiString) : boolean;
var {$ifdef CheckRecursion}
      recTestBuf : array [0..3] of dword;
    {$endif}
    saa        : PSidAndAttributes;
    error      : dword;
begin
  error := GetLastError;
  {$ifdef CheckRecursion}
    result := false;
    if NotRecursedYet(@IsSystemProcess, @recTestBuf) then begin
  {$endif}
    result := GetVersion and $80000000 = 0;
    saa := nil;
    if result and ((sid <> '') or GetProcessSid(processHandle, saa)) then begin
      if ( (Length(sid) > 1) and (ord(sid[2]) > 1) ) or
         ( (saa <> nil) and (saa^.Sid <> nil) and (TPAByte(saa^.Sid)^[1] > 1) ) then
        result := false;
      if saa <> nil then
        LocalFree(dword(saa));
    end;
    {$ifdef CheckRecursion}
      recTestBuf[3] := 0;
    end;
  {$endif}
  SetLastError(error);
end;

var NtQueryInformationProcess : function (ph, level: dword; buf: pointer; len, retLen: dword) : dword; stdcall = nil;
function GetPeb(process: dword) : dword;
// get the process environment block ("peb")
var pbi : array [0..5] of dword;  // TProcessBasicInformation
begin
  if @NtQueryInformationProcess = nil then
    NtQueryInformationProcess := NtProc(CNtQueryInformationProcess);
  if (@NtQueryInformationProcess <> nil) and (NtQueryInformationProcess(process, 0, @pbi, sizeOf(pbi), 0) = 0) then
       result := pbi[1]
  else result := 0;
end;

var NtWow64QueryInformationProcess64 : function (ph, level: dword; buf: pointer; len, retLen: dword) : dword; stdcall = nil;
function GetPeb64(process: dword) : int64;
// get the 64bit process environment block ("peb")
var pbi : record
            ExitStatus      : dword;
            PebBaseAddress  : int64;
            AffinityMask    : int64;
            BasePriority    : dword;
            ProcessId       : int64;
            ParentProcessId : int64;
          end;
begin
  if @NtWow64QueryInformationProcess64 = nil then
    NtWow64QueryInformationProcess64 := NtProc(CNtWow64QueryInfoProcess64);
  if (@NtWow64QueryInformationProcess64 <> nil) and (NtWow64QueryInformationProcess64(process, 0, @pbi, sizeOf(pbi), 0) = 0) then
       result := pbi.PebBaseAddress
  else result := 0;
end;

var NtWow64ReadVirtualMemory64 : function (ph: dword; baseAddress: int64; buf: pointer; bufSize: int64; var read: int64) : dword; stdcall = nil;
function ReadProcessMemory64(process: dword; baseAddress: int64; buf: pointer; bufSize: dword) : boolean;
var read64 : int64;
begin
  if @NtWow64ReadVirtualMemory64 = nil then
    NtWow64ReadVirtualMemory64 := NtProc(CNtWow64ReadVirtualMemory64);
  result := (@NtWow64ReadVirtualMemory64 <> nil) and
            (NtWow64ReadVirtualMemory64(process, baseAddress, buf, bufSize, read64) = 0) and
            (read64 = bufSize);
end;

(*function IsNativeProcess(processHandle: dword) : boolean;
var peb, subSystem, c1 : dword;
begin
  peb := GetPeb(processHandle);
  result := (peb <> 0) and
            ReadProcessMemory(processHandle, pointer(peb + $b4), @subSystem, 4, c1) and
            (subSystem = IMAGE_SUBSYSTEM_NATIVE);
end;*)

var
  IsWow64Process : function (processHandle: dword; var wow64Process: bool) : bool; stdcall = nil;
  IsWow64Ready   : boolean = false;
  Am64OS         : boolean = false;

function Is64bitProcess(processHandle: dword) : bool; stdcall;
var b1 : bool;
begin
  if Am64OS then begin
    if not IsWow64Ready then begin
      IsWow64Process := KernelProc(CIsWow64Process);
      IsWow64Ready := true;
    end;
    result := (@IsWow64Process <> nil) and ((not IsWow64Process(processHandle, b1)) or (not b1));
  end else
    result := false;
end;

function Is64bitOS : bool; stdcall;
begin
  result := Am64OS;
end;

function AmSystemProcess : bool; stdcall;
begin
  {$ifdef log}
    log('AmSystemProcess');
  {$endif}
  result := IsSystemProcess(GetCurrentProcess, '');
  {$ifdef log}
    log('AmSystemProcess -> ' + booleanToChar(result));
  {$endif}
end;

function GetProcessSessionId(pid: dword) : dword;
var {$ifdef CheckRecursion}
      recTestBuf : array [0..3] of dword;
    {$endif}
    pid2sid    : function (processId: dword; var sessionId: dword) : bool; stdcall;
    error      : dword;
begin
  error := GetLastError;
  {$ifdef CheckRecursion}
    result := 0;
    if NotRecursedYet(@GetProcessSessionId, @recTestBuf) then begin
  {$endif}
    {$ifdef log}
      log('GetProcessSessionId (pid: ' + IntToHexEx(pid) + ')');
    {$endif}
    pid2sid := KernelProc(CProcessIdToSessionId);
    if (@pid2sid = nil) or (not pid2sid(pid, result)) then
      result := 0;
    {$ifdef log}
      log('GetProcessSessionId (pid: ' + IntToHexEx(pid) + ') -> ' + IntToStrEx(result));
    {$endif}
    {$ifdef CheckRecursion}
      recTestBuf[3] := 0;
    end;
  {$endif}
  SetLastError(error);
end;

function GetCurrentSessionId : dword; stdcall;
begin
  {$ifdef log}
    log('GetCurrentSessionId');
  {$endif}
  result := GetProcessSessionId(GetCurrentProcessId);
  {$ifdef log}
    log('GetCurrentSessionId -> ' + IntToHexEx(result));
  {$endif}
end;

function GetInputSessionId : dword; stdcall;
var {$ifdef CheckRecursion}
      recTestBuf : array [0..3] of dword;
    {$endif}
    gacsid     : function : dword; stdcall;
    error      : dword;
begin
  error := GetLastError;
  {$ifdef CheckRecursion}
    result := 0;
    if NotRecursedYet(@GetInputSessionId, @recTestBuf) then begin
  {$endif}
    {$ifdef log}
      log('GetInputSessionId');
    {$endif}
    gacsid := KernelProc(CGetInputSessionId);
    if @gacsid <> nil then
         result := gacsid
    else result := 0;
    {$ifdef log}
      log('GetInputSessionId -> ' + IntToHexEx(result));
    {$endif}
    {$ifdef CheckRecursion}
      recTestBuf[3] := 0;
    end;
  {$endif}
  SetLastError(error);
end;

function ProcessIdToFileNameEx(processId: dword; fileNameA: PAnsiChar; fileNameW: PWideChar; bufLenInChars: dword; enumIfNecessary: boolean) : boolean;
// find out what file the specified process was executed from
const PROCESS_QUERY_LIMITED_INFORMATION = $1000;
var {$ifdef CheckRecursion}
      recTestBuf : array [0..3] of dword;
    {$endif}
    prcs       : TDAProcess;
    i1         : integer;
    error      : dword;
    ph         : dword;
    len        : dword;
    peb32      : dword;
    peb64      : int64;
    pp32       : dword;
    pp64       : int64;
    ifn32      : TUnicodeStr;
    ifn64      : TUnicodeStr64;
    ifnLen     : dword;
    flags      : dword;
    readOk     : boolean;
    arrChW     : array [0..MAX_PATH - 1] of wideChar;
begin
  result := false;
  error := GetLastError;
  prcs := nil;
  {$ifdef CheckRecursion}
    if NotRecursedYet(@ProcessIdToFileName, @recTestBuf) then begin
  {$endif}
    if fileNameA <> nil then
      fileNameW := pointer(LocalAlloc(LPTR, bufLenInChars * 2));
    fileNameW[0] := '?';
    fileNameW[1] := #0;
    if processId <> 0 then begin
      if @QueryFullProcessImageName = nil then begin
        QueryFullProcessImageName := KernelProc(CQueryFullProcessImageName);
        if @QueryFullProcessImageName = nil then
          QueryFullProcessImageName := pointer(1);
      end;
      if dword(@QueryFullProcessImageName) > 1 then begin
        ph := OpenProcess(PROCESS_QUERY_INFORMATION, false, processId);
        if ph = 0 then
          ph := OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, processId);
        if ph <> 0 then begin
          len := bufLenInChars;
          result := QueryFullProcessImageName(ph, 0, fileNameW, len);
          CloseHandle(ph);
        end;
      end else begin
        ph := OpenProcess(PROCESS_QUERY_INFORMATION or PROCESS_VM_READ, false, processId);
        if ph <> 0 then begin
          peb32 := GetPeb(ph);
          if (peb32 = 0) and Is64bitProcess(ph) then
            peb64 := GetPeb64(ph)
          else
            peb64 := 0;
          if (peb32 <> 0) or (peb64 <> 0) then begin
            readOk := false;
            ifnLen := 0;
            if peb64 <> 0 then begin
              if ReadProcessMemory64(ph, peb64 + $20, @ pp64, sizeOf( pp64)) and (pp64 <> 0) and
                 ReadProcessMemory64(ph,  pp64 + $08, @flags, sizeOf(flags)) and
                 ReadProcessMemory64(ph,  pp64 + $60, @ifn64, sizeOf(ifn64)) and (ifn64.len > 0) and (ifn64.str <> 0) then begin
                if ifn64.len >= bufLenInChars * 2 then begin
                  ifn64.str := ifn64.str + ifn64.len - (bufLenInChars - 1) * 2;
                  ifn64.len := (bufLenInChars - 1) * 2;
                end;
                if flags and $1 = 0 then  // PROCESS_PARAMETERS_NORMALIZED
                  ifn64.str := ifn64.str + pp64;
                readOk := ReadProcessMemory64(ph, ifn64.str, fileNameW, ifn64.len);
                ifnLen := ifn64.len;
              end;
            end else
              if ReadProcessMemory(ph, pointer(peb32 + $10), @ pp32, sizeOf( pp32), len) and (len = sizeOf( pp32)) and (pp32 <> 0) and
                 ReadProcessMemory(ph, pointer( pp32 + $08), @flags, sizeOf(flags), len) and (len = sizeOf(flags)) and
                 ReadProcessMemory(ph, pointer( pp32 + $38), @ifn32, sizeOf(ifn32), len) and (len = sizeOf(ifn32)) and (ifn32.len > 0) and (ifn32.str <> nil) then begin
                if ifn32.len >= bufLenInChars * 2 then begin
                  ifn32.str := pointer(dword(ifn32.str) + dword(ifn32.len) - (bufLenInChars - 1) * 2);
                  ifn32.len := (bufLenInChars - 1) * 2;
                end;
                if flags and $1 = 0 then  // PROCESS_PARAMETERS_NORMALIZED
                  dword(ifn32.str) := dword(ifn32.str) + dword(pp32);
                readOk := ReadProcessMemory(ph, ifn32.str, fileNameW, ifn32.len, len) and (len = ifn32.len);
                ifnLen := ifn32.len;
              end;
            if readOk then begin
              fileNameW[ifnLen div 2] := #0;
              if (ifnLen > 8) and (fileNameW[0] = '\') and (fileNameW[1] = '?') and (fileNameW[2] = '?') and (fileNameW[3] = '\') then
                Move(fileNameW[4], fileNameW[0], ifnLen + 2 - 4 * 2)
              else
                if (ifnLen > 24) and (fileNameW[0] = '\') and (fileNameW[1] = 'S') and (fileNameW[ 2] = 'y') and (fileNameW[ 3] = 's') and
                                     (fileNameW[4] = 't') and (fileNameW[5] = 'e') and (fileNameW[ 6] = 'm') and (fileNameW[ 7] = 'R') and 
                                     (fileNameW[8] = 'o') and (fileNameW[9] = 'o') and (fileNameW[10] = 't') and (fileNameW[11] = '\') then begin
                  len := GetWindowsDirectoryW(arrChW, MAX_PATH);
                  if dword(ifnLen) div 2 + 1 - 11 + len >= bufLenInChars then begin
                    ifnLen := (bufLenInChars - 1 + 11 - len) * 2;
                    fileNameW[ifnLen div 2] := #0;
                  end;
                  Move(fileNameW[11], fileNameW[len], ifnLen + 2 - 11 * 2);
                  Move(arrChW[0], fileNameW[0], len * 2);
                end;
              result := true;
            end;
          end;
          CloseHandle(ph);
        end;
      end;
    end;
    if (not result) and enumIfNecessary then begin
      prcs := EnumProcesses;
      for i1 := 0 to high(prcs) do
        if prcs[i1].id = processId then begin
          result := true;
          if dword(Length(prcs[i1].exeFile)) >= bufLenInChars then
            SetLength(prcs[i1].exeFile, bufLenInChars - 1);
          lstrcpyW(fileNameW, PWideChar(prcs[i1].exeFile));
          break;
        end;
    end;
    if result and (fileNameA <> nil) then
      WideToAnsi(fileNameW, fileNameA);

    {$ifdef CheckRecursion}
      recTestBuf[3] := 0;
    end;
  {$endif}
  SetLastError(error);
end;

function ProcessIdToFileNameA(processId: dword; fileName: PAnsiChar; bufLenInChars: word) : bool; stdcall;
begin
  result := ProcessIdToFileNameEx(processId, fileName, nil, bufLenInChars, true);
end;

function ProcessIdToFileNameW(processId: dword; fileName: PWideChar; bufLenInChars: word) : bool; stdcall;
begin
  result := ProcessIdToFileNameEx(processId, nil, fileName, bufLenInChars, true);
end;

function ProcessIdToFileName(processId: dword; var fileName: AnsiString) : boolean;
var buf : PWideChar;
begin
  buf := pointer(LocalAlloc(LPTR, 64 * 1024));
  result := ProcessIdToFileNameEx(processId, nil, buf, 32 * 1024, true);
  if result and (buf[0] <> #0) then begin
    SetLength(fileName, lstrlenW(buf));
    WideToAnsi(buf, PAnsiChar(fileName));
  end else
    fileName := '';
  LocalFree(dword(buf));
end;

// ***************************************************************

const
  SERVICE_KERNEL_DRIVER       = $00000001;
  SERVICE_SYSTEM_START        = $00000001;
  SERVICE_ERROR_NORMAL        = $00000001;

  SERVICE_CONTROL_STOP        = 1;
  SERVICE_CONTROL_INTERROGATE = 4;

  SC_MANAGER_ALL_ACCESS       = STANDARD_RIGHTS_REQUIRED or $3f;
  SERVICE_ALL_ACCESS          = $01ff;
  SERVICE_QUERY_STATUS        = $0004;

  SERVICE_STOPPED             = 1;
  SERVICE_RUNNING             = 4;

type
  TServiceStatus = packed record
    dwServiceType             : dword;
    dwCurrentState            : dword;
    dwControlsAccepted        : dword;
    dwWin32ExitCode           : dword;
    dwServiceSpecificExitCode : dword;
    dwCheckpoint              : dword;
    dwWaitHint                : dword;
  end;

  TQueryServiceConfig = packed record
    serviceType               : dword;
    startType                 : dword;
    errorControl              : dword;
    pathName                  : PWideChar;
    loadOrderGroup            : PWideChar;
    tagId                     : dword;
    dependencies              : PWideChar;
    startName                 : PWideChar;
    displayName               : PWideChar;
  end;

var
  CloseServiceHandle   : function (handle: dword) : bool; stdcall;
  ControlService       : function (service, control: dword; var ss: TServiceStatus) : bool; stdcall;
  QueryServiceStatus   : function (service: dword; var ss: TServiceStatus) : bool; stdcall;
  DeleteService        : function (service: dword) : bool; stdcall;
  OpenSCManagerW       : function (machine, database: PWideChar; access: dword) : dword; stdcall;
  OpenServiceW         : function (scMan: dword; service: PWideChar; access: dword) : dword; stdcall;
  StartServiceW        : function (service: dword; argCnt: dword; args: pointer) : bool; stdcall;
  CreateServiceW       : function (scMan: dword; service, display: PWideChar;
                                   access, serviceType, startType, errorControl: dword;
                                   pathName, loadOrderGroup: PWideChar; tagId: pointer;
                                   dependencies, startName, password: PWideChar) : dword; stdcall;
  ChangeServiceConfigW : function (service, serviceType, startType, errorControl: dword;
                                   pathName, loadOrderGroup: PWideChar; tagId: pointer;
                                   dependencies, startName, password, displayName: PWideChar) : bool; stdcall;
  QueryServiceConfigW  : function (service: dword; var buf: TQueryServiceConfig;
                                   size: dword; var needed: dword) : bool; stdcall;

function InitServiceProcs : boolean;
var dll : dword;
begin
  result := @OpenSCManagerW <> nil;
  if (not result) and (GetVersion and $80000000 = 0) then begin
    dll := LoadLibraryA(PAnsiChar(DecryptStr(CAdvApi32)));
    CloseServiceHandle    := GetProcAddress(dll, PAnsiChar(DecryptStr(CCloseServiceHandle)));
    ControlService        := GetProcAddress(dll, PAnsiChar(DecryptStr(CControlService)));
    QueryServiceStatus    := GetProcAddress(dll, PAnsiChar(DecryptStr(CQueryServiceStatus)));
    DeleteService         := GetProcAddress(dll, PAnsiChar(DecryptStr(CDeleteService)));
    OpenSCManagerW        := GetProcAddress(dll, PAnsiChar(DecryptStr(COpenSCManagerW)));
    OpenServiceW          := GetProcAddress(dll, PAnsiChar(DecryptStr(COpenServiceW)));
    StartServiceW         := GetProcAddress(dll, PAnsiChar(DecryptStr(CStartServiceW)));
    CreateServiceW        := GetProcAddress(dll, PAnsiChar(DecryptStr(CCreateServiceW)));
    ChangeServiceConfigW  := GetProcAddress(dll, PAnsiChar(DecryptStr(CChangeServiceConfigW)));
    QueryServiceConfigW   := GetProcAddress(dll, PAnsiChar(DecryptStr(CQueryServiceConfigW)));
    result := @OpenSCManagerW <> nil;
  end;
end;

function CheckService(driverName, fileName, description: PWideChar; start: boolean) : boolean;
var c1, c2, c3 : dword;
    qsc        : ^TQueryServiceConfig;
    ss         : TServiceStatus;
    le         : dword;
begin
  result := false;
  if InitServiceProcs then begin
    // first we contact the service control manager
    c1 := OpenSCManagerW(nil, nil, SC_MANAGER_ALL_ACCESS);
    if c1 <> 0 then begin
      // okay, that worked, now we try to open our service
      c2 := OpenServiceW(c1, driverName, SERVICE_ALL_ACCESS);
      if c2 <> 0 then begin
        // our service is installed, let's check the parameters
        c3 := 0;
        QueryServiceConfigW(c2, TQueryServiceConfig(nil^), 0, c3);
        if c3 <> 0 then begin
          qsc := pointer(LocalAlloc(LPTR, c3 * 2));
          if QueryServiceConfigW(c2, qsc^, c3 * 2, c3) then begin
            if not QueryServiceStatus(c2, ss) then
              ss.dwCurrentState := SERVICE_STOPPED;
            // all is fine if either the parameters are already correct or:
            // (1) if the service isn't running yet and
            // (2) we're able to successfully reset the parameters
            result := ( (qsc^.serviceType  = SERVICE_KERNEL_DRIVER) and
                        (qsc^.startType    = SERVICE_SYSTEM_START ) and
                        (qsc^.errorControl = SERVICE_ERROR_NORMAL ) and
                        (PosText(fileName, qsc^.pathName) > 0)              ) or
                      ( ChangeServiceConfigW(c2, SERVICE_KERNEL_DRIVER, SERVICE_SYSTEM_START, SERVICE_ERROR_NORMAL,
                                             fileName, nil, nil, nil, nil, nil, description) and
                        (ss.dwCurrentState = SERVICE_STOPPED)                    );
            if start then
              result := result and ((ss.dwCurrentState = SERVICE_RUNNING) or StartServiceW(c2, 0, nil));
          end;
          le := GetLastError;
          LocalFree(dword(qsc));
          SetLastError(le);
        end;
        le := GetLastError;
        CloseServiceHandle(c2);
        SetLastError(le);
      end;
      le := GetLastError;
      CloseServiceHandle(c1);
      SetLastError(le);
    end;
  end;
end;

function InstallInjectionDriver(driverName, fileName32bit, fileName64bit, description: PWideChar) : bool; stdcall;
var fileName : PWideChar;
    c1, c2   : dword;
    sla      : AnsiString;
    slw      : AnsiString;
    libA     : PAnsiChar;
    le       : dword;
begin
  result := false;
  libA := nil;
  EnableAllPrivileges;
  if Is64bitOS then
    fileName := fileName64bit
  else
    fileName := fileName32bit;
  if InitServiceProcs and CheckLibFilePath(libA, fileName, sla, slw) then begin
    // first we contact the service control manager
    c1 := OpenSCManagerW(nil, nil, SC_MANAGER_ALL_ACCESS);
    if c1 <> 0 then begin
      // okay, that worked, now we try to open our service
      c2 := OpenServiceW(c1, driverName, SERVICE_ALL_ACCESS);
      if c2 <> 0 then begin
        // our service is already installed, so let's check the parameters
        result := CheckService(driverName, fileName, description, false);
        CloseServiceHandle(c2);
      end else begin
        // probably our service is not installed yet, so we do that now
        c2 := CreateServiceW(c1, driverName, description,
                             SERVICE_ALL_ACCESS or STANDARD_RIGHTS_ALL,
                             SERVICE_KERNEL_DRIVER, SERVICE_SYSTEM_START,
                             SERVICE_ERROR_NORMAL, fileName, nil, nil, nil, nil, nil);
        if c2 <> 0 then begin
          // installation went smooth
          result := true;
          CloseServiceHandle(c2);
        end;
      end;
      le := GetLastError;
      CloseServiceHandle(c1);
      SetLastError(le);
      if result then begin
        result := StartInjectionDriver(driverName);
        if not result then begin
          le := GetLastError;
          UninstallInjectionDriver(driverName);
          SetLastError(le);
        end;
      end;
    end;
  end;
end;

function UninstallInjectionDriver(driverName: PWideChar) : bool; stdcall;
var c1, c2 : dword;
    le     : dword;
begin
  result := false;
  EnableAllPrivileges;
  if InitServiceProcs then begin
    // first we contact the service control manager
    c1 := OpenSCManagerW(nil, nil, SC_MANAGER_ALL_ACCESS);
    if c1 <> 0 then begin
      // okay, that worked, now we try to open our service
      c2 := OpenServiceW(c1, driverName, SERVICE_ALL_ACCESS or _DELETE);
      if c2 <> 0 then begin
        // our service is installed, let's try to remove it
        result := DeleteService(c2);
        le := GetLastError;
        CloseServiceHandle(c2);
        SetLastError(le);
      end;
      le := GetLastError;
      CloseServiceHandle(c1);
      SetLastError(le);
    end;
  end;
end;

// ***************************************************************

const
  FILE_DEVICE_UNKNOWN = $22;
  METHOD_BUFFERED     = 0;
  FILE_READ_ACCESS    = 1;
  FILE_WRITE_ACCESS   = 2;

function CTL_CODE(devtype, func, method, acc: dword) : dword;
begin
  result := devtype shl 16 + acc shl 14 + func shl 2 + method;
end;

function Ioctl_InjectDll    : dword; begin result := CTL_CODE(FILE_DEVICE_UNKNOWN, $800, METHOD_BUFFERED, FILE_READ_ACCESS or FILE_WRITE_ACCESS) end;
function Ioctl_UninjectDll  : dword; begin result := CTL_CODE(FILE_DEVICE_UNKNOWN, $801, METHOD_BUFFERED, FILE_READ_ACCESS or FILE_WRITE_ACCESS) end;
function Ioctl_AllowUnload  : dword; begin result := CTL_CODE(FILE_DEVICE_UNKNOWN, $802, METHOD_BUFFERED, FILE_READ_ACCESS or FILE_WRITE_ACCESS) end;
function Ioctl_InjectMethod : dword; begin result := CTL_CODE(FILE_DEVICE_UNKNOWN, $803, METHOD_BUFFERED, FILE_READ_ACCESS or FILE_WRITE_ACCESS) end;
function Ioctl_EnumInject   : dword; begin result := CTL_CODE(FILE_DEVICE_UNKNOWN, $804, METHOD_BUFFERED, FILE_READ_ACCESS or FILE_WRITE_ACCESS) end;

type
  TDrvCmdHeader = packed record
    Size : dword;
    Hash : TDigest;
  end;
  TDrvCmdAllowUnload = packed record
    Header : TDrvCmdHeader;
    Allow  : boolean;
  end;
  TDrvCmdInjectMethod = packed record
    Header : TDrvCmdHeader;
    Method : boolean;
  end;
  TDrvCmdEnumInject = packed record
    Header : TDrvCmdHeader;
    Index  : integer;
  end;
  TDrvCmdDll = packed record
    Header     : TDrvCmdHeader;
    OwnerHash  : TDigest;
    Session    : dword;
    Flags      : dword;
    Dummy      : array [0..9] of int64;  // used by driver for temporary storage
    Name       : array [0..259] of WideChar;
    IncludeLen : dword;
    ExcludeLen : dword;
    Data       : array [0..0] of WideChar;
  end;

function SendDriverCommand(const driverName: UnicodeString; command: dword; var inbuf: TDrvCmdHeader; outbuf: pointer; outbufSize: dword) : boolean;
var fh, c1 : cardinal;
    pb1    : ^byte;
    i1     : integer;
    byte1  : byte;
    le     : dword;
begin
  fh := CreateFileW(PWideChar('\\.\' + driverName), GENERIC_READ or GENERIC_WRITE, FILE_SHARE_READ or FILE_SHARE_WRITE, nil, OPEN_EXISTING, 0, 0);
  if fh <> dword(-1) then begin
    c1 := GetCurrentThreadId;
    pb1 := pointer(dword(@inbuf) + sizeOf(inbuf));
    for i1 := 0 to integer(inbuf.Size) - sizeOf(inbuf) - 1 do begin
      pb1^ := pb1^ xor byte(c1) xor (i1 and $ff);
      inc(pb1);
    end;
    inbuf.Hash := Hash(pointer(dword(@inbuf) + sizeOf(inbuf)), inbuf.Size - sizeOf(inbuf));
    pb1 := pointer(dword(@inbuf) + sizeOf(inbuf));
    for i1 := 0 to integer(inbuf.Size) - sizeOf(inbuf) - 1 do begin
      byte1 := inbuf.Hash[i1 mod sizeOf(inbuf.Hash)];
      pb1^ := pb1^ xor (byte1 shr 4) xor ((byte1 and $f) shl 4);
      inc(pb1);
    end;
    result := DeviceIoControl(fh, command, @inbuf, inbuf.Size, outbuf, outbufSize, c1, nil);
    le := GetLastError;
    CloseHandle(fh);
    SetLastError(le);
  end else
    result := false;
end;

procedure RegDelTree(key: HKEY; name: PWideChar);
var arrChW : PWideChar;
    hk     : HKEY;
    c1     : cardinal;
begin
  if RegOpenKeyExW(key, name, 0, KEY_ALL_ACCESS, hk) = 0 then begin
    c1 := 0;
    arrChW := pointer(LocalAlloc(LPTR, MAX_PATH * 2));
    while RegEnumKeyW(hk, c1, arrChW, MAX_PATH) = 0 do begin
      RegDelTree(hk, arrChW);
      inc(c1);
    end;
    LocalFree(dword(arrChW));
    RegCloseKey(hk);
  end;
  RegDeleteKeyW(key, name);
end;

function ConvertNtErrorCode(error: dword) : dword;
const CRtlNtStatusToDosError : AnsiString = (* RtlNtStatusToDosError *)  #$07#$21#$39#$1B#$21#$06#$21#$34#$21#$20#$26#$01#$3A#$11#$3A#$26#$10#$27#$27#$3A#$27;
var ns2de : function (ntstatus: dword) : dword; stdcall;
begin
  result := error;
  if result and $c0000000 <> 0 then begin
    ns2de := NtProc(CRtlNtStatusToDosError);
    if @ns2de <> nil then
      result := ns2de(result);
  end;
end;

function StopInjectionDriver(driverName: PWideChar) : bool; stdcall;
var allow_      : TDrvCmdAllowUnload;
    c1, c2      : dword;
    ss          : TServiceStatus;
    stopAllowed : boolean;
    hk1, hk2    : HKEY;
    arrChW2     : array [0..MAX_PATH] of WideChar;
    nud         : function (name: pointer) : dword; stdcall;
    name        : TUnicodeStr;
    le          : dword;
begin
  result := false;
  EnableAllPrivileges;
  allow_.Header.Size := sizeOf(allow_);
  allow_.Allow := true;
  stopAllowed := SendDriverCommand(driverName, Ioctl_AllowUnload, allow_.Header, nil, 0);
  if InitServiceProcs then begin
    c1 := OpenSCManagerW(nil, nil, SC_MANAGER_ALL_ACCESS);
    if c1 = 0 then
      c1 := OpenSCManagerW(nil, nil, 0);
    if c1 <> 0 then begin
      c2 := OpenServiceW(c1, PWideChar(driverName), SERVICE_ALL_ACCESS);
      if c2 <> 0 then begin
        ControlService(c2, SERVICE_CONTROL_STOP, ss);
        result := QueryServiceStatus(c2, ss) and (ss.dwCurrentState = SERVICE_STOPPED);
        CloseServiceHandle(c2);
      end;
      CloseServiceHandle(c1);
      if (not result) and
         (RegOpenKeyExA(HKEY_LOCAL_MACHINE, PAnsiChar(DecryptStr(CSystemCcsServices)), 0, KEY_ALL_ACCESS, hk1) = 0) then begin
        if RegOpenKeyExW(HKEY_LOCAL_MACHINE, driverName, 0, KEY_READ, hk2) <> 0 then begin
          // the service registry key for the driver is not there
          // but we can still communicate with the driver
          // that means that "LoadInjectionDriver" was used
          // in order to unload that driver, we need to restore the registry
          if RegCreateKeyExW(hk1, driverName, 0, nil, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nil, hk2, nil) = 0 then begin
            c1 := 1;
            if RegSetValueExA(hk2, PAnsiChar(DecryptStr(CType)), 0, REG_DWORD, @c1, 4) = 0 then begin
              AnsiToWide(PAnsiChar('\' + DecryptStr(CRegistryMachine) + '\' + DecryptStr(CSystemCcsServices) + '\'), arrChW2);
              lstrcatW(arrChW2, driverName);
              name.str := arrChW2;
              name.len := lstrlenW(name.str) * 2;
              name.maxlen := name.len + 2;
              nud := NtProc(CNtUnloadDriver);
              if @nud <> nil then begin
                SetLastError(ConvertNtErrorCode(nud(@name)));
                result := GetLastError = 0;
              end else
                SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            end;
            le := GetLastError;
            RegCloseKey(hk2);
            RegDelTree(hk1, driverName);
            SetLastError(le);
          end;
        end else begin
          le := GetLastError;
          RegCloseKey(hk2);
          SetLastError(le);
        end;
        le := GetLastError;
        RegCloseKey(hk1);
        SetLastError(le);
      end;
    end;
  end;
  if (not result) and stopAllowed then begin
    // stopping the driver failed, so let's secure stopping again
    allow_.Allow := false;
    le := GetLastError;
    SendDriverCommand(driverName, Ioctl_AllowUnload, allow_.Header, nil, 0);
    SetLastError(le);
  end;
end;

function StartInjectionDriver(driverName: PWideChar) : bool; stdcall;
var c1, c2 : dword;
    ss     : TServiceStatus;
    le     : dword;
begin
  result := false;
  EnableAllPrivileges;
  if InitServiceProcs then begin
    c1 := OpenSCManagerW(nil, nil, SC_MANAGER_ALL_ACCESS);
    if c1 = 0 then
      c1 := OpenSCManagerW(nil, nil, 0);
    if c1 <> 0 then begin
      c2 := OpenServiceW(c1, PWideChar(driverName), SERVICE_ALL_ACCESS);
      if c2 <> 0 then begin
        result := (QueryServiceStatus(c2, ss) and (ss.dwCurrentState = SERVICE_RUNNING)) or
                  StartServiceW(c2, 0, nil);
        le := GetLastError;
        CloseServiceHandle(c2);
        SetLastError(le);
      end;
      le := GetLastError;
      CloseServiceHandle(c1);
      SetLastError(le);
    end;
  end;
end;

function IsInjectionDriverInstalled(driverName: PWideChar) : bool; stdcall;
var c1, c2 : dword;
    le     : dword;
begin
  result := false;
  EnableAllPrivileges;
  if InitServiceProcs then begin
    c1 := OpenSCManagerW(nil, nil, SC_MANAGER_ALL_ACCESS);
    if c1 = 0 then
      c1 := OpenSCManagerW(nil, nil, 0);
    if c1 <> 0 then begin
      c2 := OpenServiceW(c1, PWideChar(driverName), SERVICE_QUERY_STATUS);
      if c2 <> 0 then begin
        result := true;
        CloseServiceHandle(c2);
      end;
      le := GetLastError;
      CloseServiceHandle(c1);
      SetLastError(le);
    end;
  end;
end;

function IsInjectionDriverRunning(driverName: PWideChar) : bool; stdcall;
var fh : dword;
begin
  EnableAllPrivileges;
  fh := CreateFileW(PWideChar('\\.\' + UnicodeString(driverName)), GENERIC_READ, FILE_SHARE_READ or FILE_SHARE_WRITE, nil, OPEN_EXISTING, 0, 0);
  result := fh <> dword(-1);
  if result then
    CloseHandle(fh);
end;

function JectDll(driverName: PWideChar; command: dword; dllFileName: PWideChar; session: dword; systemProcesses: boolean; includeMask, excludeMask: PWideChar) : boolean;
var dll  : ^TDrvCmdDll;
    size : dword;
    sla  : AnsiString;
    slw  : AnsiString;
    libA : PAnsiChar;
    le   : dword;
begin
  result := false;
  libA := nil;
  if not CheckLibFilePath(libA, dllFileName, sla, slw) then
    exit;
  EnableAllPrivileges;
  size := sizeOf(dll^) + lstrlenW(includeMask) * 2 + 2 + lstrlenW(excludeMask) * 2 + 2;
  dll := pointer(LocalAlloc(LPTR, size));
  if dll <> nil then begin
    if session = CURRENT_SESSION then
      session := GetCurrentSessionId;
    dll.Header.Size := size;
    dll.Session := session;
    dll.Flags := 0;
    if systemProcesses then
      dll.Flags := dll.Flags or 1;
    lstrcpyW(dll.Name, dllFileName);
    dll.IncludeLen := lstrlenW(includeMask);
    dll.ExcludeLen := lstrlenW(excludeMask);
    lstrcpyW(dll.Data, includeMask);
    lstrcpyW(pointer(dword(@dll.Data) + dll.IncludeLen * 2 + 2), excludeMask);
    result := SendDriverCommand(driverName, command, dll.Header, nil, 0);
    le := GetLastError;
    LocalFree(dword(dll));
    SetLastError(le);
  end;
end;

function StartDllInjection(driverName, dllFileName: PWideChar; session: dword; systemProcesses: boolean; includeMask, excludeMask: PWideChar) : bool; stdcall;
begin
  result := JectDll(driverName, Ioctl_InjectDll, dllFileName, session, systemProcesses, includeMask, excludeMask);
end;

function StopDllInjection(driverName, dllFileName: PWideChar; session: dword; systemProcesses: boolean; includeMask, excludeMask: PWideChar) : bool; stdcall;
begin
  result := JectDll(driverName, Ioctl_UninjectDll, dllFileName, session, systemProcesses, includeMask, excludeMask);
end;

function LoadInjectionDriver(driverName, fileName32bit, fileName64bit: PWideChar) : bool; stdcall;
var fileName   : PWideChar;
    c1, c2, c3 : dword;
    arrChW2    : array [0..MAX_PATH] of WideChar;
    hk1, hk2   : HKEY;
    nld        : function (name: pointer) : dword; stdcall;
    wImagePath : array [0..9] of WideChar;
    name       : TUnicodeStr;
    sla        : AnsiString;
    slw        : AnsiString;
    libA       : PAnsiChar;
    le         : dword;
begin
  result := false;
  libA := nil;
  if Is64bitOS then
    fileName := fileName64bit
  else
    fileName := fileName32bit;
  if not CheckLibFilePath(libA, fileName, sla, slw) then
    exit;
  EnableAllPrivileges;
  arrChW2[0] := '\';
  arrChW2[1] := '?';
  arrChW2[2] := '?';
  arrChW2[3] := '\';
  arrChW2[4] := #0;
  lstrcatW(arrChW2, fileName);
  if RegOpenKeyExA(HKEY_LOCAL_MACHINE, PAnsiChar(DecryptStr(CSystemCcsServices)), 0, KEY_ALL_ACCESS, hk1) = 0 then begin
    if RegCreateKeyExW(hk1, driverName, 0, nil, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nil, hk2, nil) = 0 then begin
      c1 := 1;
      c2 := 0;
      c3 := 4;
      AnsiToWide(PAnsiChar(DecryptStr(CImagePath)), wImagePath);
      if (RegSetValueExA(hk2, PAnsiChar(DecryptStr(CType        )), 0, REG_DWORD, @c1, 4) = 0) and
         (RegSetValueExA(hk2, PAnsiChar(DecryptStr(CErrorControl)), 0, REG_DWORD, @c2, 4) = 0) and
         (RegSetValueExA(hk2, PAnsiChar(DecryptStr(CStart       )), 0, REG_DWORD, @c3, 4) = 0) and
         (RegSetValueExW(hk2, wImagePath, 0, REG_SZ, @arrChW2, lstrlenW(arrChW2) * 2 + 2) = 0) then begin
        AnsiToWide(PAnsiChar('\' + DecryptStr(CRegistryMachine) + '\' + DecryptStr(CSystemCcsServices) + '\'), arrChW2);
        lstrcatW(arrChW2, driverName);
        name.str := arrChW2;
        name.len := lstrlenW(name.str) * 2;
        name.maxlen := name.len + 2;
        nld := NtProc(CNtLoadDriver);
        if @nld <> nil then begin
          SetLastError(ConvertNtErrorCode(nld(@name)));
          result := GetLastError = 0;
        end else
          SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
      end;
      le := GetLastError;
      RegCloseKey(hk2);
      SetLastError(le);
    end;
    le := GetLastError;
    RegDelTree(hk1, driverName);
    RegCloseKey(hk1);
    SetLastError(le);
  end;
end;

function SetInjectionMethod(driverName: PWideChar; newInjectionMethod: bool) : bool; stdcall;
// set the injection method used by the kernel mode driver
var method : TDrvCmdInjectMethod;
begin
  method.Header.Size := sizeOf(method);
  method.Method := newInjectionMethod;
  result := SendDriverCommand(driverName, Ioctl_InjectMethod, method.Header, nil, 0);
end;

type TDAInjectionRequest = array of ^TDrvCmdDll;

function EnumInjectionRequests(driverName: PWideChar) : TDAInjectionRequest;
var enum  : TDrvCmdEnumInject;
    dll   : ^TDrvCmdDll;
    index : integer;
begin
  result := nil;
  enum.Header.Size := sizeOf(TDrvCmdEnumInject);
  dll := VirtualAlloc(nil, 2 * 1024 * 1024, MEM_COMMIT, PAGE_READWRITE);
  if dll <> nil then begin
    index := 0;
    enum.Index := index;
    while SendDriverCommand(driverName, Ioctl_EnumInject, enum.Header, dll, 2 * 1024 * 1024) do begin
      SetLength(result, Length(result) + 1);
      result[high(result)] := pointer(LocalAlloc(LPTR, dll.Header.Size));
      Move(dll^, result[high(result)]^, dll.Header.Size);
      inc(index);
      enum.Index := index;
    end;
    VirtualFree(dll, 0, MEM_RELEASE);
  end;
end;

// ***************************************************************

function SetMadCHookOption(option: dword; param: PWideChar) : bool; stdcall;
begin
  result := true;
  case option of
    USE_EXTERNAL_DRIVER_FILE        : lstrcpyW(CustomInjDrvFile, param);
    SECURE_MEMORY_MAPS              : SecureMemoryMaps := true;
    DISABLE_CHANGED_CODE_CHECK      : DisableChangedCodeCheck := true;
    USE_NEW_IPC_LOGIC               : if GetVersion and $80000000 = 0 then
                                        // the new IPC logic is only supported in the NT family
                                        UseNewIpcLogic := true
                                      else
                                        result := false;
    SET_INTERNAL_IPC_TIMEOUT        : InternalIpcTimeout := dword(param);
    VMWARE_INJECTION_MODE           : VmWareInjectionMode := true;
    DONT_TOUCH_RUNNING_PROCESSES    : InjectIntoRunningProcesses := false;
    RENEW_OVERWRITTEN_HOOKS         : RenewOverwrittenHooks := true;
    INJECT_INTO_RUNNING_PROCESSES   : InjectIntoRunningProcesses := param <> nil;
    UNINJECT_FROM_RUNNING_PROCESSES : UninjectFromRunningProcesses := param <> nil;
    else                              result := false;
  end;
end;

// ***************************************************************

(* static injection works fine, but unfortunately uninjecting is not possible
  function GetFirstExportIndex(libA: PAnsiChar; libW: PWideChar) : integer;

    function VirtualToRaw(nh: PImageNtHeaders; addr: dword) : dword;
    type TAImageSectionHeader = packed array [0..maxInt shr 6] of TImageSectionHeader;
    var i1 : integer;
        sh : ^TAImageSectionHeader;
    begin
      result := addr;
      dword(sh) := dword(nh) + sizeOf(nh^);
      for i1 := 0 to nh^.FileHeader.NumberOfSections - 1 do
        if (addr >= sh[i1].VirtualAddress) and
           ((i1 = nh^.FileHeader.NumberOfSections - 1) or (addr < sh[i1 + 1].VirtualAddress)) then begin
          result := addr - sh[i1].VirtualAddress + sh[i1].PointerToRawData;
          break;
        end;
    end;

  var fh, map, module : dword;
      nh              : PImageNtHeaders;
      ed              : PImageExportDirectory;
      i1              : integer;
      sa              : TSecurityAttributes;
      sd              : TSecurityDescriptor;
  begin
    result := -1;
    if libW <> nil then
         fh := CreateFileW(libW, GENERIC_READ, FILE_SHARE_READ or FILE_SHARE_WRITE, nil, OPEN_EXISTING, 0, 0)
    else fh := CreateFileA(libA, GENERIC_READ, FILE_SHARE_READ or FILE_SHARE_WRITE, nil, OPEN_EXISTING, 0, 0);
    if fh <> INVALID_HANDLE_VALUE then begin
      InitSecAttr(sa, sd, false);
      map := CreateFileMapping(fh, @sa, PAGE_READONLY, 0, 0, nil);
      if map <> 0 then begin
        module := dword(MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0));
        if module <> 0 then begin
          nh := GetImageNtHeaders(module);
          if nh <> nil then
            try
              dword(ed) := module + VirtualToRaw(nh, nh^.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
              with ed^ do
                for i1 := 0 to NumberOfFunctions - 1 do
                  if TPACardinal(module + VirtualToRaw(nh, AddressOfFunctions))^[i1] <> 0 then begin
                    result := integer(Base) + i1;
                    break;
                  end;
            except end;
          UnmapViewOfFile(pointer(module));
        end;
        CloseHandle(map);
      end;
      CloseHandle(fh);
    end;
  end;

  function InjectLibrariesStatic(const libs: array of AnsiString; const exp: array of dword) : integer;
  const
    InjectMagic = $77712345abcde777;           // magic number for our buffer
    IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT = $b;   // data directory of new bind logic
  type
    TAImageDataDirectory = packed array [0..IMAGE_NUMBEROF_DIRECTORY_ENTRIES - 1] of TImageDataDirectory;
    TImageImportDirectory = packed record
      HintNameArray  : dword;
      TimeDateStamp  : dword;
      ForwarderChain : dword;
      Name_          : dword;
      ThunkArray     : dword;
    end;
    PImageImportDirectory = ^TImageImportDirectory;
    TAImageImportDirectory = packed array [0..maxInt shr 5] of TImageImportDirectory;
    TBufHeader = packed record
      magic     : int64;
      dataCount : integer;
      dataStart : dword;
      dataSize  : dword;
      mainCount : integer;
      mainStart : dword;
      mainSize  : dword;
    end;

    function CountImportDirectories(id: PImageImportDirectory) : integer;
    var buf    : ^TAImageImportDirectory;
        i1, i2 : integer;
    begin
      result := 0;
      GetMem(buf, sizeOf(TImageImportDirectory) * 64);
      i1 := 64;
      while true do begin
        while (not Read(id, buf^, i1 * sizeOf(TImageImportDirectory))) and (i1 > 0) do
          i1 := i1 shr 2;
        for i2 := 0 to i1 - 1 do begin
          with buf^[i2] do
            if (HintNameArray = 0) and (TimeDateStamp = 0) and
               (ForwarderChain = 0) and (Name_ = 0) and (ThunkArray = 0) then
              exit;
          inc(result);
        end;
        inc(id, i1);
      end;
      FreeMem(buf);
    end;

  var module  : dword;                             // remote exe's module handle
      dd      : ^TAImageDataDirectory;             // remote exe's ^data directories
      nh      : TImageNtHeaders;                   // remote exe's nt image headers
      import  : TImageDataDirectory;               // remote exe's import directory
      oldhead : TBufHeader;                        // head of the old remote buffer
      newhead : TBufHeader;                        // head of our new remote buffer
      canFree : boolean;                           // can we free the remote buffer without danger?
      prepBuf : dword;                             // our local preparation buffer
      c1      : cardinal;                          // multi purpose dword
      i1      : integer;                           // multi purpose integer
      i64     : int64;                             // multi purpose int64
  begin
    // this kind of patch works only on not yet initialized processes
    if NotInitializedYet then begin
      // can we locate the main module?
      if GetExeModuleInfos(module, nh, pointer(dd)) then begin
        // we found the main module
        import := nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        // fill IAT directory, if necessary
        with nh.OptionalHeader do
          if int64(DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT]) = 0 then
            Write(@dd^[IMAGE_DIRECTORY_ENTRY_IAT], DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT], 8);
        // did we already patch this process?
        if (not Read(pointer(module + import.VirtualAddress - sizeOf(oldHead)), oldHead, sizeOf(oldHead))) or
           (oldHead.magic <> InjectMagic) then begin
          // nope, so fill the old buffer infos manually
          ZeroMemory(@oldHead, sizeOf(oldHead));
          oldHead.dataStart := module + import.VirtualAddress;
          oldHead.mainStart := module + import.VirtualAddress;
          oldHead.mainCount := CountImportDirectories(pointer(oldHead.mainStart));
          oldHead.mainSize  := (oldHead.mainCount + 1) * sizeOf(TImageImportDirectory);
        end;
        canFree := true;
        // fill new header
        newHead.magic     := InjectMagic;
        newHead.dataCount := oldHead.dataCount + Length(libs);
        newHead.dataSize  := newHead.dataCount * (8 + MAX_PATH);
        newHead.mainCount := oldHead.mainCount + Length(libs);
        newHead.mainSize  := (newHead.mainCount + 1) * sizeOf(TImageImportDirectory);
        newHead.dataStart := dword(AllocMemEx(newHead.dataSize + sizeOf(newHead) + newHead.mainSize, process));
        newHead.mainStart := newHead.dataStart + newHead.dataSize + sizeOf(newHead);
        // use local preparation buffer to fill the remote buffer
        prepBuf := LocalAlloc(LPTR, newHead.dataSize + sizeOf(newHead) + newHead.mainSize);
        // fill the preparation buffer with the old remote buffer or
        // with the original import data
        if Read(pointer(oldHead.dataStart), pointer(prepBuf +                                      newHead.dataSize - oldHead.dataSize)^, oldHead.dataSize) and
           Read(pointer(oldHead.mainStart), pointer(prepBuf + newHead.dataSize + sizeOf(newHead) + newHead.mainSize - oldHead.mainSize)^, oldHead.mainSize) then begin
          // fill the new head into the preparation buffer
          TBufHeader(pointer(prepBuf + newHead.dataSize)^) := newHead;
          // now we do the rest of the preparation
          for i1 := 0 to newHead.dataCount - 1 do begin
            with TAImageImportDirectory(pointer(prepBuf + newHead.dataSize + sizeOf(newHead))^)[i1] do begin
              // we adjust the auxiliary data for the old dlls and fill in the new
              HintNameArray  := 0;
              TimeDateStamp  := 0;
              ForwarderChain := maxCard;
              ThunkArray     := newHead.dataStart + dword(i1) * (8 + MAX_PATH) - module;
              Name_          := ThunkArray + 8;
            end;
            if i1 < Length(libs) then begin
              // the new dlls get a dll path and a 1 element dummy import thunk
              c1 := prepBuf + dword(i1) * (8 + MAX_PATH);
              TPInt64(c1)^ := $80000000 + exp[i1];
              lstrcpyA(PAnsiChar(libs[i1]), PAnsiChar(c1 + 8));
            end;
          end;
          // the preparation buffer is done, now we copy it to the remote buffer
          if Write(pointer(newHead.dataStart), pointer(prepBuf)^, newHead.dataSize + sizeOf(newHead) + newHead.mainSize, false) then begin
            // hopefully the process is still not initialized yet?
            if NotInitializedYet then begin
              // let's do the real modification, namely patching the data directories
              import.VirtualAddress := newHead.mainStart - module;
              import.Size           := newHead.mainSize;
              if Write(@dd^[IMAGE_DIRECTORY_ENTRY_IMPORT], import, 8) then begin
                // we want to convert the new bind method into the old one
                // for this purpose we now kill IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT
                i64 := 0;
                if (int64(nh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT]) = 0) or
                   Write(@dd^[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT], i64, 8) then begin
                  if NotInitializedYet then begin
                    // injection fully succeeded
                    result := 2;
                    if oldHead.dataCount > 0 then
                      // free the old remote buffer (if it exists)
                      FreeMemEx(pointer(oldHead.dataStart), process);
                  end else result := 1;
                end else result := 0;
                canFree := false;
              end else result := 0;
            end else result := 1;
          end else result := 0;
        end else result := 0;
        // we can kill our preparation buffer
        LocalFree(prepBuf);
        if (result < 2) and canFree then
          // injection failed, so we free the remote buffer (if allowed)
          FreeMemEx(pointer(newHead.dataStart), process);
      end else result := 0;
    end else result := 1;
  end;
*)

// ***************************************************************

type
  TInjectDllA = packed record
    name     : packed array [0..MAX_PATH - 1] of AnsiChar;
    owner    : packed array [0..MAX_PATH - 1] of AnsiChar;
    sid      : packed array [0..90] of byte;
    system   : boolean;
    reserved : int64;
    session  : dword;
  end;
  TInjectDllsRecA = packed record
    name  : array [0..7] of AnsiChar;
    count : integer;
    items : packed array [0..maxInt shr 10] of TInjectDllA;
  end;

var KernelEntryPointNextHook : function (module, reason: dword; reserved: pointer) : bool; stdcall;

function KernelEntryPointCallback(module, reason: dword; reserved: pointer) : bool; stdcall;
var arrCh  : array [0..9] of AnsiChar;
    buf    : ^TInjectDllsRecA;
    item   : ^TInjectDllA;
    map    : dword;
    i1, i2 : integer;
    c1     : dword;
    oldErr : dword;
    b1     : boolean;
begin
  result := KernelEntryPointNextHook(module, reason, reserved);
  if reason = DLL_PROCESS_ATTACH then begin
    asm
      mov eax, fs:[$30]     // eax := threadEnvironmentBlock^.processDataBase
      mov eax, [eax + $20]  // eax := processDataBase^.flags
      mov c1, eax           // c1  := eax;
    end;
    if c1 and $30800018 = 0 then begin
      // process is not terminating yet                (30800000)
      // process is neither a DOS nor a win16 process  (00000018)
      // so we load the registered dlls
      item := nil;
      // CDllInjectName - mc2SWDIJ
      arrCh[0] := 'm';
      arrCh[1] := 'c';
      arrCh[2] := '2';
      arrCh[3] := 'S';
      arrCh[4] := 'W';
      arrCh[5] := 'D';
      arrCh[6] := 'I';
      arrCh[7] := 'J';
      arrCh[9] := #0;
      for i1 := 1 to 9 do begin
        b1 := true;
        arrCh[8] := AnsiChar(ord('0') + i1);
        map := OpenFileMappingAFunc(FILE_MAP_READ, false, arrCh);
        if map <> 0 then begin
          buf := MapViewOfFileFunc(map, FILE_MAP_READ, 0, 0, 0);
          if buf <> nil then begin
            if TPInt64(@buf^.name)^ = TPInt64(@arrCh)^ then begin
              b1 := false;
              for i2 := 0 to buf^.count - 1 do begin
                if item = nil then
                  item := LocalAllocFunc(LPTR, sizeOf(item^));
                item^ := buf^.items[i2];
                if item^.name[0] = #0 then begin
                  b1 := true;
                  break;
                end;
                oldErr := SetErrorModeFunc(SEM_FAILCRITICALERRORS);
                LoadLibraryAFunc(item^.name);
                SetErrorModeFunc(oldErr);
              end;
            end;
            UnmapViewOfFileFunc(buf);
          end;
          CloseHandleFunc(map);
        end;
        if b1 then
          break;
      end;
      if item <> nil then
        LocalFreeFunc(item);
    end;
  end;
end;

procedure AutoUnhook2(dll: dword; wait: boolean);
var anh    : array of TPPointer;
    i1, i2 : integer;
begin
  {$ifdef log}
    log('AutoUnhook (module: ' + IntToHexEx(dll) + '; wait: ' + booleanToChar(wait) + ')');
  {$endif}
  if (dll <> 0) and HookReady then begin
    EnterCriticalSection(HookSection);
    try
      SetLength(anh, Length(HookList));
      i2 := 0;
      for i1 := 0 to high(HookList) do
        if (((dll = dword(-1)) and (not wait)) or (HookList[i1].owner = dll)) and
           ((HookList[i1].ch = nil) or (not HookList[i1].ch.FSystemWide)) then begin
          anh[i2] := HookList[i1].nextHook;
          inc(i2);
        end;
    finally LeaveCriticalSection(HookSection) end;
    for i1 := 0 to i2 - 1 do
      UnhookCodeEx(anh[i1]^, false, wait);
  end;
  {$ifdef log}
    log('AutoUnhook (module: ' + IntToHexEx(dll) + '; wait: ' + booleanToChar(wait) + ') -> +');
  {$endif}
end;

procedure AutoUnhook(moduleHandle: dword);
var i1, i2 : integer;
    b1, b2 : boolean;
    map    : dword;
    buf    : ^TInjectDllsRecA;
    itemA  : TInjectDllA;
    s1     : AnsiString;
begin
  {$ifdef log}
    log('AutoUnhook (module: ' + IntToHexEx(moduleHandle) + ')');
  {$endif}
  b2 := false;
  if GetVersion and $80000000 <> 0 then
    for i1 := 1 to 9 do begin
      b1 := true;
      s1 := DecryptStr(CDllInjectName) + AnsiChar(ord('0') + i1);
      map := OpenFileMappingA(FILE_MAP_READ, false, PAnsiChar(s1));
      if map <> 0 then begin
        buf := MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
        if buf <> nil then begin
          if TPInt64(@buf^.name)^ = TPInt64(s1)^ then begin
            b1 := false;
            for i2 := 0 to buf^.count - 1 do begin
              itemA := buf^.items[i2];
              if itemA.name[0] = #0 then begin
                b1 := true;
                break;
              end;
              if GetModuleHandleA(itemA.name) = moduleHandle then begin
                b2 := true;
                break;
              end;
            end;
          end;
          UnmapViewOfFile(buf);
        end;
        CloseHandle(map);
      end;
      if b1 or b2 then
        break;
    end;
  if not b2 then
    AutoUnhook2(moduleHandle, true);
  {$ifdef log}
    log('AutoUnhook (module: ' + IntToHexEx(moduleHandle) + ') -> +');
  {$endif}
end;

var CheckFreeLibraryNextHook : function (module: dword) : bool stdcall = nil;
function CheckFreeLibrary(module: dword) : bool; stdcall;
var arrCh  : array [0..9] of AnsiChar;
    buf    : ^TInjectDllsRecA;
    item   : ^TInjectDllA;
    map    : dword;
    i1, i2 : integer;
    b1, b2 : boolean;
    c1     : dword;
begin
  asm
    mov eax, fs:[$30]     // eax := threadEnvironmentBlock^.processDataBase
    mov eax, [eax + $20]  // eax := processDataBase^.flags
    mov c1, eax           // c1  := eax;
  end;
  b2 := false;
  if c1 and $30800000 = 0 then begin
    // process is not terminating yet, so we load the registered dlls
    item := nil;
    // CDllInjectName - mc2SWDIJ
    arrCh[0] := 'm';
    arrCh[1] := 'c';
    arrCh[2] := '2';
    arrCh[3] := 'S';
    arrCh[4] := 'W';
    arrCh[5] := 'D';
    arrCh[6] := 'I';
    arrCh[7] := 'J';
    arrCh[9] := #0;
    for i1 := 1 to 9 do begin
      b1 := true;
      arrCh[8] := AnsiChar(ord('0') + i1);
      map := OpenFileMappingAFunc(FILE_MAP_READ, false, arrCh);
      if map <> 0 then begin
        buf := MapViewOfFileFunc(map, FILE_MAP_READ, 0, 0, 0);
        if buf <> nil then begin
          if TPInt64(@buf^.name)^ = TPInt64(@arrCh)^ then begin
            b1 := false;
            for i2 := 0 to buf^.count - 1 do begin
              if item = nil then
                item := LocalAllocFunc(LPTR, sizeOf(item^));
              item^ := buf^.items[i2];
              if item^.name[0] = #0 then begin
                b1 := true;
                break;
              end;
              if GetModuleHandleAFunc(item^.name) = module then begin
                b2 := true;
                break;
              end;
            end;
          end;
          UnmapViewOfFileFunc(buf);
        end;
        CloseHandleFunc(map);
      end;
      if b1 or b2 then
        break;
    end;
    if item <> nil then
      LocalFreeFunc(item);
  end;
  if b2 then begin
    result := false;
    SetLastErrorProc(ERROR_ACCESS_DENIED);
  end else
    result := CheckFreeLibraryNextHook(module);
end;

function AddInjectDll(owner: dword; libA: PAnsiChar; libW: PWideChar;
                      system: boolean;
                      var newMap: integer; var newMutex: boolean) : boolean;
var mutex  : dword;
    bufA   : ^TInjectDllsRecA;
    newBuf : boolean;
    map    : dword;
    i1, i2 : integer;
    count  : integer;
    s1     : AnsiString;
begin
  {$ifdef log}
    log('AddInjectDll (owner: ' + IntToHexEx(owner) + '; lib: %1)', libA, libW);
  {$endif}
  result := false;
  newMap := 0;
  s1 := DecryptStr(CDllInjectName);
  mutex := CreateMutexA(nil, false, PAnsiChar(s1 + DecryptStr(CMutex)));
  if mutex <> 0 then begin
    newMutex := GetLastError <> ERROR_ALREADY_EXISTS;
    WaitForSingleObject(mutex, INFINITE);
    try
      count := 1;
      for i1 := 1 to 9 do begin
        count := count shl 2;
        map := CreateFileMappingA(dword(-1), nil, PAGE_READWRITE, 0, 12 + count * (MAX_PATH * 2 + 91 + 1 + 8 + 4), PAnsiChar(s1 + AnsiChar(ord('0') + i1)));
        if map = 0 then
          break;
        newBuf := GetLastError <> ERROR_ALREADY_EXISTS;
        bufA := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if bufA = nil then begin
          CloseHandle(map);
          break;
        end;
        if newBuf then begin
          newMap := i1;
          bufA^.count := count;
          for i2 := 0 to count - 1 do
            bufA^.items[i2].name[0] := #0;
          AtomicMove(pointer(s1), @bufA^.name, 8);
          HandleLiveForever(map);
        end;
        for i2 := 0 to bufA^.count - 1 do
          if bufA^.items[i2].name[0] = #0 then begin
            result := true;
            GetModuleFileNameA(owner, bufA^.items[i2].owner, MAX_PATH);
            ZeroMemory(@bufA^.items[i2].sid, 91);
            bufA^.items[i2].system   := system;
            bufA^.items[i2].reserved := 0;
            bufA^.items[i2].session  := 0;
            lstrcpyA(@bufA^.items[i2].name[1], @libA[1]);
            AtomicMove(libA, @bufA^.items[i2].name, 1);
            break;
          end;
        UnmapViewOfFile(bufA);
        CloseHandle(map);
        if result then
          break;
      end;
    finally ReleaseMutex(mutex) end;
    if newMutex then begin
      HandleLiveForever(mutex);
      HookCodeX(0, 0, '', '', GetKernelEntryPoint, @KernelEntryPointCallback, @KernelEntryPointNextHook, SYSTEM_WIDE_9X);
      HookCodeX(0, 0, '', '', FindRealCode(KernelProc(CFreeLibrary, true)), @CheckFreeLibrary, @CheckFreeLibraryNextHook, SYSTEM_WIDE_9X);
    end;
    CloseHandle(mutex);
  end;
  {$ifdef log}
    log('AddInjectDll (owner: ' + IntToHexEx(owner) + '; lib: %1) -> ' + booleanToChar(result), libA, libW);
  {$endif}
end;

function DelInjectDll(owner: dword; libA: PAnsiChar; libW: PWideChar; system: boolean; ignoreParams: boolean) : boolean;
var mutex      : dword;
    bufsA      : array [1..9] of ^TInjectDllsRecA;
    maps       : array [1..9] of dword;
    dll1, dll2 : PAnsiChar;
    own1, own2 : PAnsiChar;
    sid1, sid2 : pointer;
    sys1, sys2 : TPBoolean;
    rsv1, rsv2 : TPInt64;
    ses1, ses2 : TPCardinal;
    i1, i2, i3 : integer;
    arrChA     : array [0..MAX_PATH] of AnsiChar;
    count      : integer;
    b1         : boolean;

  procedure GetPointers(index: integer; var dll, own: PAnsiChar;
                        var sid: pointer; var sys: TPBoolean; var rsv: TPInt64; var ses: TPCardinal);
  var i1 : integer;
  begin
    for i1 := 1 to 9 do
      if index < bufsA[i1]^.count then begin
        dll := @bufsA[i1]^.items[index].name;
        own := @bufsA[i1]^.items[index].owner;
        sid := @bufsA[i1]^.items[index].sid;
        sys := @bufsA[i1]^.items[index].system;
        rsv := @bufsA[i1]^.items[index].reserved;
        ses := @bufsA[i1]^.items[index].session;
        break;
      end else
        dec(index, bufsA[i1]^.count);
  end;

begin
  {$ifdef log}
    log('DelInjectDll (owner: ' + IntToHexEx(owner) + '; lib: %1)', libA, libW);
  {$endif}
  result := false;
  mutex := OpenMutexA(SYNCHRONIZE, false, PAnsiChar(DecryptStr(CDllInjectName) + DecryptStr(CMutex)));
  if mutex <> 0 then begin
    GetModuleFileNameA(owner, arrChA, MAX_PATH);
    WaitForSingleObject(mutex, INFINITE);
    try
      count := 0;
      for i1 := 1 to 9 do begin
        maps[i1] := OpenFileMappingA(FILE_MAP_ALL_ACCESS, false, PAnsiChar(DecryptStr(CDllInjectName) + AnsiChar(ord('0') + i1)));
        if maps[i1] <> 0 then begin
          bufsA[i1] := MapViewOfFile(maps[i1], FILE_MAP_ALL_ACCESS, 0, 0, 0);
          if bufsA[i1] = nil then begin
            CloseHandle(maps[i1]);
            maps[i1] := 0;
          end else
            inc(count, bufsA[i1]^.count);
        end;
        if maps[i1] = 0 then
          break;
      end;
      for i1 := count - 1 downto 0 do begin
        GetPointers(i1, dll1, own1, sid1, sys1, rsv1, ses1);
        b1 := (dll1^ <> #0) and (lstrcmpiA(dll1, libA) = 0) and (lstrcmpiA(own1, arrChA) = 0);
        if b1 and (ignoreParams or ((sys1^ = system) and (rsv1^ = 0) and (ses1^ = 0))) then begin
          result := true;
          for i2 := i1 to count - 2 do begin
            GetPointers(i2 + 1, dll2, own2, sid2, sys2, rsv2, ses2);
            ZeroMemory(sid1, 91);
            for i3 := 91 - 1 downto 0 do
              PAnsiChar(sid1)[i3] := PAnsiChar(sid2)[i3];
            sys1^ := sys2^;
            rsv1^ := rsv2^;
            ses1^ := ses2^;
            ZeroMemory(own1, MAX_PATH);
            ZeroMemory(dll1, MAX_PATH);
            for i3 := MAX_PATH - 1 downto 0 do
              own1[i3] := own2[i3];
            for i3 := MAX_PATH - 1 downto 1 do
              dll1[i3] := dll2[i3];
            AtomicMove(dll2, dll1, 1);
            dll1 := dll2;
            own1 := own2;
            sid1 := sid2;
            sys1 := sys2;
            rsv1 := rsv2;
            ses1 := ses2;
            if dll1[0] = #0 then
              break;
          end;
          InterlockedExchange(TPInteger(dll1)^, 0);
        end;
      end;
      for i1 := 1 to 9 do begin
        if maps[i1] = 0 then
          break;
        UnmapViewOfFile(bufsA[i1]);
        CloseHandle(maps[i1]);
      end;
    finally ReleaseMutex(mutex) end;
    CloseHandle(mutex);
  end;
  {$ifdef log}
    log('DelInjectDll (owner: ' + IntToHexEx(owner) + '; lib: %1) -> ' + booleanToChar(result), libA, libW);
  {$endif}
end;

// ***************************************************************

function CheckLibFilePath(var libA: PAnsiChar;  var libW: PWideChar;
                          var strA: AnsiString; var strW: AnsiString) : boolean;

  function CheckIt(pathA: PAnsiChar = nil; pathW: PWideChar = nil; killName: boolean = false) : boolean;
  var len : integer;
      i1  : integer;
  begin
    if GetVersion and $80000000 = 0 then begin
      len := lstrlenW(pathW);
      if killName then
        for i1 := len - 2 downto 1 do
          if pathW[i1] = '\' then begin
            pathW[i1] := #0;
            len := i1;
            break;
          end;
      if (len > 0) and (pathW[len - 1] <> '\') then begin
        pathW[len    ] := '\';
        pathW[len + 1] := #0;
      end;
      if libA = nil then
           strW := WideToWideEx(pathW, false) + WideToWideEx (libW)
      else strW := WideToWideEx(pathW, false) + AnsiToWideEx2(libA);
      result := (Length(strW) > 6) and ( (strW[3] = ':') or
                                         ((strW[1] = '\') and (strW[3] = '\')) ) and
                (GetFileAttributesW(pointer(strW)) <> dword(-1));
      if result then
        libW := pointer(strW);
    end else begin
      len := lstrlenA(pathA);
      if killName then
        for i1 := len - 2 downto 1 do
          if pathA[i1] = '\' then begin
            pathA[i1] := #0;
            len := i1;
            break;
          end;
      if (len > 0) and (pathA[len - 1] <> '\') then begin
        pathA[len    ] := '\';
        pathA[len + 1] := #0;
      end;
      if libA = nil then
           strA := AnsiString(pathA) + WideToAnsiEx2(libW)
      else strA := AnsiString(pathA) +   AnsiString (libA);
      result := (Length(strA) > 3) and ( (strA[2] = ':') or
                                         ((strA[1] = '\') and (strA[2] = '\')) ) and
                (GetFileAttributesA(PAnsiChar(strA)) <> dword(-1));
      if result then
        libA := pointer(strA);
    end;
  end;

var arrChA : array [0..MAX_PATH] of AnsiChar;
    arrChW : array [0..MAX_PATH] of WideChar;
begin
  {$ifdef log}
    log('CheckLibFilePath (lib: %1)', libA, libW);
  {$endif}
  result := true;
  if not CheckIt then begin
    GetModuleFileNameA(0, arrChA, MAX_PATH);
    GetModuleFileNameW(0, arrChW, MAX_PATH);
    if not CheckIt(arrChA, arrChW, true) then begin
      GetModuleFileNameA(HInstance, arrChA, MAX_PATH);
      GetModuleFileNameW(HInstance, arrChW, MAX_PATH);
      if not CheckIt(arrChA, arrChW, true) then begin
        GetCurrentDirectoryA(MAX_PATH, arrChA);
        GetCurrentDirectoryW(MAX_PATH, arrChW);
        if not CheckIt(arrChA, arrChW) then begin
          GetSystemDirectoryA(arrChA, MAX_PATH);
          GetSystemDirectoryW(arrChW, MAX_PATH);
          result := CheckIt(arrChA, arrChW);
        end;
      end;
    end;
  end;
  {$ifdef log}
    log('CheckLibFilePath (lib -> %1) -> ' + booleanToChar(result), libA, libW);
  {$endif}
end;

type
  PListEntry32 = ^TListEntry32;
  TListEntry32 = packed record
    Flink : pointer;  // 0x00
    Blink : pointer;  // 0x04
  end;                

  PLdrDataTableEntry32 = ^TLdrDataTableEntry32;
  TLdrDataTableEntry32 = packed record
    InLoadOrderLinks            : TListEntry32;   // 0x00
    InMemoryOrderLinks          : TListEntry32;   // 0x08
    InInitializationOrderLinks  : TListEntry32;   // 0x10
    DllBase                     : pointer;        // 0x18
    EntryPoint                  : pointer;        // 0x1C
    SizeOfImage                 : dword;          // 0x20
    FullDllName                 : TUnicodeStr;    // 0x24
    BaseDllName                 : TUnicodeStr;    // 0x2C
    Flags                       : dword;          // 0x34
    LoadCount                   : word;           // 0x38
    TlsIndex                    : word;           // 0x3A
    HashLinks                   : TListEntry32;   // 0x3C
    LoadedImports               : pointer;        // 0x44
    EntryPointActivationContext : pointer;        // 0x48
    PatchInformation            : pointer;        // 0x4C
    ForwarderLinks              : TListEntry32;   // 0x50
    ServiceTagLinks             : TListEntry32;   // 0x58
    StaticLinks                 : TListEntry32;   // 0x60
  end;

  TInjectRec = packed record
    load                     : boolean;
    libA                     : array [0..MAX_PATH + 10] of AnsiChar;
    libW                     : array [0..MAX_PATH + 10] of WideChar;
    GetVersionFunc           : function : dword; stdcall;
    SharedMem9x_Free         : function (ptr: pointer) : bool; stdcall;
    VirtualFreeFunc          : function (address: pointer; size, flags: dword) : bool; stdcall;
    SetErrorModeFunc         : function (mode: dword) : dword; stdcall;
    LoadLibraryAFunc         : function (lib: PAnsiChar) : dword; stdcall;
    LoadLibraryWFunc         : function (lib: PWideChar) : dword; stdcall;
    GetLastErrorFunc         : function : integer; stdcall;
    GetModuleHandleAFunc     : function (lib: PAnsiChar) : dword; stdcall;
    GetModuleHandleWFunc     : function (lib: PWideChar) : dword; stdcall;
    GetCurrentProcessIdFunc  : function : dword; stdcall;
    OpenFileMappingAFunc     : function (access: dword; inheritHandle: bool; name: PAnsiChar) : dword; stdcall;
    MapViewOfFileFunc        : function (map, access, ofsHi, ofsLo, size: dword) : pointer; stdcall;
    UnmapViewOfFileFunc      : function (addr: pointer) : bool; stdcall;
    CloseHandleFunc          : function (obj: dword) : bool; stdcall;
    FreeLibraryFunc          : function (module: dword) : bool; stdcall;
    EnterCriticalSectionFunc : procedure (var criticalSection: TRTLCriticalSection); stdcall;
    LeaveCriticalSectionFunc : procedure (var criticalSection: TRTLCriticalSection); stdcall;
  end;

function InjectThread(var injRec: TInjectRec) : integer; stdcall;
var module     : dword;
    i1         : integer;
    oldErr     : dword;
    map        : dword;
    buf        : TPPointer;
    arrCh      : array [0..38] of AnsiChar;
    aup        : procedure (moduleHandle: dword);
    ir         : TInjectRec;
    c1         : dword;
    peb        : dword;
    loaderLock : PRtlCriticalSection;
    firstDll   : PLdrDataTableEntry32;
    dll        : PLdrDataTableEntry32;
begin
  asm
    mov eax, fs:[$30]     // eax := threadEnvironmentBlock^.processDataBase
    mov peb, eax
  end;
  result := 0;
  ir := injRec;
  if ir.GetVersionFunc and $80000000 = 0 then
       ir.VirtualFreeFunc(@injRec, 0, MEM_RELEASE)
  else ir.SharedMem9x_Free(@injRec);
  if ir.load then begin
    oldErr := ir.SetErrorModeFunc(SEM_FAILCRITICALERRORS);
    if ir.GetVersionFunc and $80000000 = 0 then
         module := ir.LoadLibraryWFunc(ir.libW)
    else module := ir.LoadLibraryAFunc(ir.libA);
    if module = 0 then
      result := ir.GetLastErrorFunc;
    ir.SetErrorModeFunc(oldErr);
  end else begin
    if ir.GetVersionFunc and $80000000 = 0 then
         module := ir.GetModuleHandleWFunc(ir.libW)
    else module := ir.GetModuleHandleAFunc(ir.libA);
    if module = 0 then
      result := ir.GetLastErrorFunc;
  end;
  if module <> 0 then begin
    if ir.GetVersionFunc and $80000000 = 0 then begin
      loaderLock := TPPointer(peb + $a0)^;
      ir.EnterCriticalSectionFunc(loaderLock^);
      firstDll := TPPointer(TPCardinal(peb + $c)^ + $c)^;  // peb.Ldr.InLoadOrderModuleList.FLink
      if firstDll <> nil then begin
        dll := firstDll;
        repeat
          if dll.DllBase = pointer(module) then begin
            if ir.load then
              dll.LoadCount := $ff
            else
              dll.LoadCount := 1;
            break;
          end;
          dll := dll.InLoadOrderLinks.Flink;
        until dll = firstDll;
      end;
      ir.LeaveCriticalSectionFunc(loaderLock^);
    end;
    if not ir.load then begin
      aup := nil;
      dword(pointer(@arrCh[00])^) := $626f6c47;  // Glob
      dword(pointer(@arrCh[04])^) := $415c6c61;  // al\A
      dword(pointer(@arrCh[08])^) := $556f7475;  // utoU
      dword(pointer(@arrCh[12])^) := $6f6f686e;  // nhoo
      dword(pointer(@arrCh[16])^) := $70614d6b;  // kMap
      arrCh[20] := '$';
      arrCh[29] := '$';
      arrCh[38] := #0;
      c1 := ir.GetCurrentProcessIdFunc;
      for i1 := 28 downto 21 do begin
        if c1 and $f > 9 then
             arrCh[i1] := AnsiChar(ord('a') + c1 and $f - $a)
        else arrCh[i1] := AnsiChar(ord('0') + c1 and $f     );
        c1 := c1 shr 4;
      end;
      c1 := module;
      for i1 := 37 downto 30 do begin
        if c1 and $f > 9 then
             arrCh[i1] := AnsiChar(ord('a') + c1 and $f - $a)
        else arrCh[i1] := AnsiChar(ord('0') + c1 and $f     );
        c1 := c1 shr 4;
      end;
      if ir.GetVersionFunc and $80000000 = 0 then
           map := ir.OpenFileMappingAFunc(FILE_MAP_READ, false, arrCh)
      else map := 0;
      if map = 0 then
        map := ir.OpenFileMappingAFunc(FILE_MAP_READ, false, @arrCh[7]);
      if map = 0 then begin
        arrCh[30] := 'm';
        arrCh[31] := 'c';
        arrCh[32] := 'h';
        arrCh[33] := #0;
        if ir.GetVersionFunc and $80000000 = 0 then
          map := ir.OpenFileMappingAFunc(FILE_MAP_READ, false, arrCh);
        if map = 0 then
          map := ir.OpenFileMappingAFunc(FILE_MAP_READ, false, @arrCh[7]);
      end;
      if map <> 0 then begin
        buf := ir.MapViewOfFileFunc(map, FILE_MAP_READ, 0, 0, 0);
        if buf <> nil then begin
          aup := buf^;
          ir.UnmapViewOfFileFunc(buf);
        end;
        ir.CloseHandleFunc(map);
      end;
      if @aup <> nil then
        aup(module);
      if ir.FreeLibraryFunc(module) then begin
        i1 := 0;
        while ir.FreeLibraryFunc(module) and (i1 < 10) do
          inc(i1);
        result := 0;
      end else
        result := ir.GetLastErrorFunc;
    end;
  end;
end;

function InstallInjectThreadProc(process, pid: dword) : pointer;
type TMchIInjT = packed record
                   name      : array [0..7] of AnsiChar;
                   pid       : dword;
                   injThread : pointer;
                 end;
var map    : dword;
    newMap : boolean;
    buf    : ^TMchIInjT;
    s1     : AnsiString;
    i1     : integer;
    c1     : dword;
begin
  result := nil;
  s1 := DecryptStr(CMchIInjT);
  if GetVersion and $80000000 = 0 then begin
    if pid <> 0 then
         map := CreateGlobalFileMapping(PAnsiChar(s1 + IntToHexEx(pid)), sizeOf(buf^))
    else map := 0;
  end else begin
    InitSharedMem9x(@@SharedMem9x_Alloc, @@SharedMem9x_Free);
    map := CreateFileMappingA(dword(-1), nil, PAGE_READWRITE, 0, sizeOf(buf^), PAnsiChar(s1));
    pid := 0;
  end;
  if map <> 0 then begin
    newMap := GetLastError <> ERROR_ALREADY_EXISTS;
    buf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if buf <> nil then begin
      for i1 := 1 to 50 do
        if newMap or ((TPInt64(@buf^.name)^ = TPInt64(s1)^) and (buf^.pid = pid)) then
             break
        else Sleep(50);
      if newMap or ((TPInt64(@buf^.name)^ <> TPInt64(s1)^) and (buf^.pid = pid)) then begin
        buf^.injThread := CopyFunction(@InjectThread, process, true);
        buf^.pid := pid;
        AtomicMove(pointer(s1), @buf^.name, 8);
        if GetVersion and $80000000 <> 0 then
             HandleLiveForever(map)
        else DuplicateHandle(GetCurrentProcess, map, process, @c1, 0, false, DUPLICATE_SAME_ACCESS);
      end;
      result := buf^.injThread;
      UnmapViewOfFile(buf);
    end;
    CloseHandle(map);
  end;
end;

function StartInjectLibraryX(process, pid: dword; inject: boolean; libA: PAnsiChar; libW: PWideChar) : dword;
var ir   : TInjectRec;
    proc : pointer;
    par  : pointer;
    c1   : dword;
begin
  {$ifdef log}
    log('StartInjectLibraryX (process: ' + '%p' +
                           '; pid: '     + IntToHexEx(pid) +
                           '; inject: '  + booleanToChar(inject) +
                           '; lib: %1)', libA, libW, process);
  {$endif}
  result := 0;
  if not Is64bitProcess(process) then begin
    if libW <> nil then
      lstrcpyW(ir.libW, libW);
    if libA <> nil then
      lstrcpyA(ir.libA, libA);
    ir.load := inject;
    proc := InstallInjectThreadProc(process, pid);
    if proc <> nil then begin
      ir.GetVersionFunc           := GetVersionFunc;
      ir.SharedMem9x_Free         := SharedMem9x_Free;
      ir.VirtualFreeFunc          := VirtualFreeFunc;
      ir.SetErrorModeFunc         := SetErrorModeFunc;
      ir.LoadLibraryAFunc         := LoadLibraryAFunc;
      ir.LoadLibraryWFunc         := LoadLibraryWFunc;
      ir.GetLastErrorFunc         := GetLastErrorFunc;
      ir.GetModuleHandleAFunc     := GetModuleHandleAFunc;
      ir.GetModuleHandleWFunc     := GetModuleHandleWFunc;
      ir.GetCurrentProcessIdFunc  := GetCurrentProcessIdFunc;
      ir.OpenFileMappingAFunc     := OpenFileMappingAFunc;
      ir.MapViewOfFileFunc        := MapViewOfFileFunc;
      ir.UnmapViewOfFileFunc      := UnmapViewOfFileFunc;
      ir.CloseHandleFunc          := CloseHandleFunc;
      ir.FreeLibraryFunc          := FreeLibraryFunc;
      ir.EnterCriticalSectionFunc := EnterCriticalSectionFunc;
      ir.LeaveCriticalSectionFunc := LeaveCriticalSectionFunc;
      par := AllocMemEx(sizeOf(ir), process);
      if (par <> nil) and WriteProcessMemory(process, par, @ir, sizeOf(ir), c1) then begin
        result := CreateRemoteThreadEx(process, nil, 1 * 1024 * 1024, proc, par, 0, c1);
        if result = 0 then begin
          c1 := GetLastError;
          FreeMemEx(par, process);
          SetLastError(c1);
        end;
      end;
    end;
  end;
  {$ifdef log}
    c1 := GetLastError;
    log('StartInjectLibraryX (process: ' + '%p' +
                           '; pid: '     + IntToHexEx(pid) +
                           '; inject: '  + booleanToChar(inject) +
                           '; lib: %1) -> th' + IntToHexEx(result),
                           libA, libW, process);
    SetLastError(c1);
  {$endif}
end;

function WaitForInjectLibraryX(thread, timeOut, time: dword) : boolean;
var error : dword;
begin
  {$ifdef log}
    log('WaitForInjectLibraryX (thread: '  + IntToHexEx(thread) +
                             '; timeOut: ' + IntToHexEx(timeOut) +
                             '; time: '    + IntToHexEx(time) +
                               ')');
  {$endif}
  if timeOut = INFINITE then
       time := 0
  else time := TickDif(time);
  if timeOut > time then begin
    if WaitForSingleObject(thread, timeOut - time) = WAIT_OBJECT_0 then
         GetExitCodeThread(thread, error)
    else error := GetLastError;
  end else
    if timeOut = 0 then
         error := 0
    else error := ERROR_TIMEOUT;
  CloseHandle(thread);
  result := error = 0;
  {$ifdef log}
    log('WaitForInjectLibraryX (thread: '  + IntToHexEx(thread) + ') -> ' + booleanToChar(result) + ' (' + IntToStrEx(error) + ')');
  {$endif}
  if not result then
    SetLastError(error);
end;

{$ifdef log}
  procedure LogProcesses(const prcs: TDAProcess);
  var i1 : integer;
  begin
    log('EnumProcesses -> ' + IntToStrEx(Length(prcs)));
    for i1 := 0 to Length(prcs) - 1 do
      log(' process[' + IntToStrEx(i1) + ']: ' + IntToHexEx(prcs[i1].id, 8) + ' ' + ExtractFileName(prcs[i1].exeFile));
  end;
{$endif}

var CharLowerBuffW : function (str: PWideChar; len: dword) : dword; stdcall = nil;
function SplitNamePath(str: UnicodeString; var path, name: UnicodeString) : boolean;
var i1  : integer;
begin
  if (GetVersion and $80000000 = 0) and (@CharLowerBuffW = nil) then
    CharLowerBuffW := GetProcAddress(GetModuleHandle(user32), PAnsiChar(DecryptStr(CCharLowerBuffW)));
  if @CharLowerBuffW <> nil then begin
    {$ifndef ver120}{$ifndef ver130}
      UniqueString(str);
    {$endif}{$endif}
    CharLowerBuffW(PWideChar(str), Length(str));
  end else
    str := LowStr(str);
  i1 := PosStr('\', str, Length(str) - 1, 1);
  result := i1 <> 0;
  if result then begin
    path := str;
    name := Copy(str, i1 + 1, maxInt);
  end else begin
    path := '';
    name := str;
  end;
end;

function MatchStrArray(const path, name: UnicodeString; const paths, names: array of UnicodeString) : boolean;
var i1 : integer;
begin
  result := false;
  for i1 := 0 to high(paths) do begin
    if (path <> '') and (paths[i1] <> '') then
      result := InternalStrMatchW(path, paths[i1], true)
    else
      result := InternalStrMatchW(name, names[i1], true);
    if result then
      break;
  end;
end;

var LdrLoadDllProc             : function (path: dword; flags: TPCardinal; name: pointer; var handle: dword) : dword stdcall = nil;
    NtProtectVirtualMemoryProc : function (process: dword; var addr: pointer; var size: dword; newProt: dword; var oldProt: dword) : dword stdcall = nil;
function InjectLibraryX(owner       : dword;
                        libA        : PAnsiChar;
                        libW        : PWideChar;
                        multiInject : boolean;
                        process     : dword;
                        driverName  : PWideChar;
                        session     : dword;
                        system      : boolean;
                        includes    : PWideChar;
                        excludes    : PWideChar;
                        excludePIDs : TPACardinal;
                        timeOut     : dword;
                        mayPatch    : boolean;
                        mayThread   : boolean;
                        forcePatch  : boolean) : boolean;
var peb32 : dword;  // 32bit process environment block
    peb64 : int64;  // 64bit process environment block

  function Read(process: dword; addr: pointer; var buf; len: integer) : boolean; overload;
  var c1 : dword;
  begin
    result := ReadProcessMemory(process, addr, @buf, len, c1);
  end;

  function Read(process: dword; addr: dword; var buf; len: integer) : boolean; overload;
  var c1 : dword;
  begin
    result := ReadProcessMemory(process, pointer(addr), @buf, len, c1);
  end;

  function Write(process: dword; addr: pointer; const buf; len: integer; unprotect: boolean = true) : boolean;
  var c1 : dword;
  begin
    result := ( (not unprotect) or
                VirtualProtectEx(process, addr, len, PAGE_EXECUTE_READWRITE, @c1) ) and
              WriteProcessMemory(process, addr, @buf, len, c1);
  end;

  function NotInitializedYet(process, pid: dword) : boolean;
  var c1     : dword;
      i64    : int64;
      ca     : TPACardinal;
      i1, i2 : integer;
  begin
    if GetVersion and $80000000 <> 0 then begin
      result := false;
      dword(ca) := pid xor magic;
      try
        dword(ca) := ca^[17];
        i2 := 0;
        for i1 := 1 to ca^[0] do
          if (ca^[i1 * 2] <> 0) and (ca^[i1 * 2] <> magic) and (ca^[i1 * 2 - 1] and $80000000 = 0) then begin
            // found a valid non inherited handle in the handle table
            // so the process seems to be up and running
            inc(i2);
            if i2 > 1 then
              break;
          end;
        if i2 <= 1 then begin
          dword(ca) := pid xor magic;
          if (ca^[11] = $010001) and                      // thread count = 1
             ((not Magic95) or (ca^[38] = 0)) and         // win98 or (topExcFilter95 = 0)
             (     Magic95  or (ca^[39] = 0)) then begin  // win95 or (topExcFilter98 = 0)
            dword(ca) := ca^[20];
            if (ca^[0] = ca^[1]) and (ca^[0] = ca^[2]) then begin
              dword(ca) := ca^[0];
              if (ca^[0] = 0) and (ca^[1] = 0) and
                 ( (     Magic95  and (TPInteger(ca^[2] + $1b8)^ > 0)) or         // suspend count > 0
                   ((not Magic95) and (TPInteger(ca^[2] + $210)^ > 0))    ) then
                result := true;
            end;
          end;
        end;
      except result := true end;
    end else
      if peb64 <> 0 then
        result := ReadProcessMemory64(process, peb64 + $18, @i64, 8) and (i64 = 0)
      else
        result := (peb32 <> 0) and Read(process, peb32 + $c, c1, 4) and (c1 = 0);
  end;

  function GetExeModuleInfos(process: dword; var module: dword; var nh: TImageNtHeaders; var dd: pointer) : boolean;

    function CheckModule : boolean;
    var w1 : word;
        c1 : dword;
    begin
      result := (module <> 0) and
                Read(process, module,            w1,          2) and (w1 = CEMAGIC) and
                Read(process, module + CENEWHDR, c1,          4) and
                Read(process, module + c1,       nh, sizeOf(nh)) and (nh.Signature = CPEMAGIC) and
                (nh.FileHeader.Characteristics and IMAGE_FILE_DLL = 0);
      if result then
        dd := @PImageNtHeaders(module + w1)^.OptionalHeader.DataDirectory;
    end;

  var p1, p2 : pointer;
      mbi    : TMemoryBasicInformation;
  begin
    result := false;
    if GetVersion and $80000000 <> 0 then begin
      p1 := nil;
      p2 := nil;
      while VirtualQueryEx(process, p1, mbi, sizeOf(mbi)) = sizeOf(mbi) do begin
        if mbi.AllocationBase <> p2 then begin
          module := dword(mbi.AllocationBase);
          if (mbi.State = MEM_COMMIT) and CheckModule then begin
            result := true;
            break;
          end;
          p2 := mbi.AllocationBase;
        end;
        inc(dword(p1), mbi.RegionSize);
      end;
    end else
      result := Read(process, peb32 + $8, module, 4) and CheckModule;
  end;

  type
    TRelJump = packed record
      jmp      : byte;     // $e9
      target   : dword;
    end;

  function InjectLibraryPatch(process, pid: dword) : integer;

    function InjectLibraryPatch9x(process, pid, module: dword; nh: TImageNtHeaders; dd: pointer) : integer;
    type
      T9xPatch1 = packed record
        RestoreDword          : packed record              // mov dword ptr [HookTarget], @OldCode
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $05
                                  target     : dword;      // HookTarget
                                  value      : dword;      // @OldCode
                                end;
        RestoreByte           : packed record              // mov dword ptr [HookTargetPlus4], @OldCodePlus4
                                  opcode     : byte;       // $C6
                                  modRm      : byte;       // $05
                                  target     : dword;      // HookTargetPlus4
                                  value      : byte;       // @OldCodePlus4
                                end;
        PushDllName           : packed record              // push @DllName
                                  opcode     : byte;       // $68
                                  value      : pointer;    // @DllName
                                end;
        PushSize              : packed record              // push $38
                                  opcode     : byte;       // $68
                                  value      : dword;      // $38
                                end;
        PushLPTR              : packed record              // push LPTR
                                  opcode     : byte;       // $6A
                                  value      : byte;       // LPTR
                                end;
        CallLocalAlloc        : packed record              // call dword ptr []     LocalAlloc
                                  opcode     : byte;       // $FF
                                  modRm      : byte;       // $15
                                  absTarget  : pointer;    // @LocalAllocTarget
                                end;
        FillEax00             : packed record              // mov dword [eax], data1
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $00
                                  value      : dword;      // data1
                                end;
        FillEax04             : packed record              // mov dword [eax+$4], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $04
                                  value      : dword;      // data2
                                end;
        FillEax08             : packed record              // mov dword [eax+$8], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $08
                                  value      : dword;      // data2
                                end;
        FillEax0c             : packed record              // mov dword [eax+$C], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $0C
                                  value      : dword;      // data2
                                end;
        FillEax10             : packed record              // mov dword [eax+$C], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $10
                                  value      : dword;      // data2
                                end;
        FillEax14             : packed record              // mov dword [eax+$C], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $14
                                  value      : dword;      // data2
                                end;
        FillEax18             : packed record              // mov dword [eax+$C], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $18
                                  value      : dword;      // data2
                                end;
        FillEax1c             : packed record              // mov dword [eax+$C], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $1c
                                  value      : dword;      // data2
                                end;
        FillEax20             : packed record              // mov dword [eax+$C], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $20
                                  value      : dword;      // data2
                                end;
        FillEax24             : packed record              // mov dword [eax+$C], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $24
                                  value      : dword;      // data2
                                end;
        FillEax28             : packed record              // mov dword [eax+$C], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $28
                                  value      : dword;      // data2
                                end;
        FillEax2c             : packed record              // mov dword [eax+$C], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $2c
                                  value      : dword;      // data2
                                end;
        FillEax30             : packed record              // mov dword [eax+$C], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $30
                                  value      : dword;      // data2
                                end;
        FillEax34             : packed record              // mov dword [eax+$C], data2
                                  opcode     : byte;       // $C7
                                  modRm      : byte;       // $40
                                  add        : byte;       // $34
                                  value      : dword;      // data2
                                end;
        JmpEax                : packed record              // jmp eax
                                  opcode     : byte;       // $FF
                                  modRm      : byte;       // $E0
                                end;
        LocalAllocTarget      : pointer;
        case boolean of
          false : (DllNameA : array [0..MAX_PATH - 1] of AnsiChar);
          true  : (DllNameW : array [0..MAX_PATH - 1] of WideChar);
      end;
      T9xPatch2 = packed record
        MovEaxLoadLibrary     : packed record              // mov eax, target       @LoadLibrary
                                  opcode     : byte;       // $B8
                                  target     : pointer;    // @LoadLibrary
                                end;
        CallEax1              : packed record              // call eax
                                  opcode     : byte;       // $FF
                                  modRm      : byte;       // $D0
                                end;
        PushP9x1              : packed record              // push p9x1
                                  opcode     : byte;       // $68
                                  value      : pointer;    // p9x1
                                end;
        MovEaxSharedMem9xFree : packed record              // mov eax, target       @SharedMem9x_Free
                                  opcode     : byte;       // $B8
                                  target     : pointer;    // @SharedMem9x_Free
                                end;
        CallEax2              : packed record              // call eax
                                  opcode     : byte;       // $FF
                                  modRm      : byte;       // $D0
                                end;
        MovEaxHook            : packed record              // mov eax, target       HookTarget
                                  opcode     : byte;       // $B8
                                  target     : pointer;    // HookTarget
                                end;
        JmpEax                : packed record              // jmp eax
                                  opcode     : byte;       // $FF
                                  modRm      : byte;       // $E0
                                end;
      end;
    const
      C9xPatch1 : T9xPatch1 =
       (RestoreDword          : (opcode: $C7; modRm: $05; target: 0; value:   0);
        RestoreByte           : (opcode: $C6; modRm: $05; target: 0; value:   0);
        PushDllName           : (opcode: $68;                        value: nil);
        PushSize              : (opcode: $68;                        value: $38);
        PushLPTR              : (opcode: $6A;                        value: $40);
        CallLocalAlloc        : (opcode: $FF; modRm: $15;        absTarget: nil);
        FillEax00             : (opcode: $C7; modRm: $00;            value:   0);
        FillEax04             : (opcode: $C7; modRm: $40; add: $04;  value:   0);
        FillEax08             : (opcode: $C7; modRm: $40; add: $08;  value:   0);
        FillEax0c             : (opcode: $C7; modRm: $40; add: $0c;  value:   0);
        FillEax10             : (opcode: $C7; modRm: $40; add: $10;  value:   0);
        FillEax14             : (opcode: $C7; modRm: $40; add: $14;  value:   0);
        FillEax18             : (opcode: $C7; modRm: $40; add: $18;  value:   0);
        FillEax1c             : (opcode: $C7; modRm: $40; add: $1c;  value:   0);
        FillEax20             : (opcode: $C7; modRm: $40; add: $20;  value:   0);
        FillEax24             : (opcode: $C7; modRm: $40; add: $24;  value:   0);
        FillEax28             : (opcode: $C7; modRm: $40; add: $28;  value:   0);
        FillEax2c             : (opcode: $C7; modRm: $40; add: $2c;  value:   0);
        FillEax30             : (opcode: $C7; modRm: $40; add: $30;  value:   0);
        FillEax34             : (opcode: $C7; modRm: $40; add: $34;  value:   0);
        JmpEax                : (opcode: $FF; modRm: $E0                       );
        LocalAllocTarget      : nil);
      C9xPatch2 : T9xPatch2 =
       (MovEaxLoadLibrary     : (opcode: $B8;                       target: nil);
        CallEax1              : (opcode: $FF; modRm: $D0                       );
        PushP9x1              : (opcode: $68;                        value: nil);
        MovEaxSharedMem9xFree : (opcode: $B8;                       target: nil);
        CallEax2              : (opcode: $FF; modRm: $D0                       );
        MovEaxHook            : (opcode: $B8;                       target: nil);
        JmpEax                : (opcode: $FF; modRm: $E0                       ));
    var pp9x1   : ^T9xPatch1;
        p9x1    : T9xPatch1;
        p9x2    : T9xPatch2;
        canFree : boolean;
        rj      : TRelJump;
    begin
      InitSharedMem9x(@@SharedMem9x_Alloc, @@SharedMem9x_Free);
      p9x1 := C9xPatch1;
      p9x2 := C9xPatch2;
      p9x1.LocalAllocTarget := KernelProc(CLocalAlloc, true);
      p9x2.MovEaxLoadLibrary.target := KernelProc(CLoadLibraryA, true);
      p9x2.MovEaxSharedMem9xFree.target := @SharedMem9x_Free;
      p9x2.MovEaxHook.target := pointer(module + nh.OptionalHeader.AddressOfEntryPoint);
      if Read(process, p9x2.MovEaxHook.target, p9x1.RestoreDword.value, 5) then begin
        pp9x1 := AllocMemEx(sizeOf(p9x1), process);
        canFree := true;
        p9x2.PushP9x1.value := pp9x1;
        lstrcpyA(p9x1.DllNameA, libA);
        p9x1.RestoreDWord  .target    := dword(p9x2.MovEaxHook.target);
        p9x1.RestoreByte   .value     := p9x1.RestoreByte.opcode;
        p9x1.RestoreByte   .opcode    := $C6;
        p9x1.RestoreByte   .target    := dword(p9x2.MovEaxHook.target) + 4;
        p9x1.PushDllName   .value     := @pp9x1^.DllNameA;
        p9x1.CallLocalAlloc.absTarget := @pp9x1^.LocalAllocTarget;
        p9x1.FillEax00     .value     := TPACardinal(@p9x2)^[ 0];
        p9x1.FillEax04     .value     := TPACardinal(@p9x2)^[ 1];
        p9x1.FillEax08     .value     := TPACardinal(@p9x2)^[ 2];
        p9x1.FillEax0c     .value     := TPACardinal(@p9x2)^[ 3];
        p9x1.FillEax10     .value     := TPACardinal(@p9x2)^[ 4];
        p9x1.FillEax14     .value     := TPACardinal(@p9x2)^[ 5];
        p9x1.FillEax18     .value     := TPACardinal(@p9x2)^[ 6];
        p9x1.FillEax1c     .value     := TPACardinal(@p9x2)^[ 7];
        p9x1.FillEax20     .value     := TPACardinal(@p9x2)^[ 8];
        p9x1.FillEax24     .value     := TPACardinal(@p9x2)^[ 9];
        p9x1.FillEax28     .value     := TPACardinal(@p9x2)^[10];
        p9x1.FillEax2c     .value     := TPACardinal(@p9x2)^[11];
        p9x1.FillEax30     .value     := TPACardinal(@p9x2)^[12];
        p9x1.FillEax34     .value     := TPACardinal(@p9x2)^[13];
        if Write(process, pp9x1, p9x1, sizeOf(p9x1), false) then begin
          rj.jmp    := $E9;
          rj.target := integer(int64(dword(pp9x1)) - int64(dword(p9x2.MovEaxHook.target)) - 5);
          if forcePatch or NotInitializedYet(process, pid) then begin
            if Write(process, p9x2.MovEaxHook.target, rj, sizeOf(rj)) then begin
              if forcePatch or NotInitializedYet(process, pid) then
                   result := 2
              else result := 1;
              canFree := false;
            end else result := 0;
          end else result := 1;
        end else result := 0;
        if (result < 2) and canFree then
          FreeMemEx(pp9x1, process);
      end else result := 0;
    end;

    function InjectLibraryPatchXp32(process: dword) : integer;
    type
      TInjectRec = packed record
        pEntryPoint            : TPCardinal;
        oldEntryPoint          : dword;
        oldEntryPointFunc      : function (hInstance, reason, reserved: dword) : bool; stdcall;
        NtProtectVirtualMemory : function (process: dword; var addr: pointer; var size: dword; newProt: dword; var oldProt: dword) : dword; stdcall;
        LdrLoadDll             : function (path: dword; flags: TPCardinal; name: pointer; var handle: dword) : dword; stdcall;
        dll                    : record
                                   len    : word;       // length * 2
                                   maxLen : word;       // length * 2 + 2
                                   str    : PWideChar;  // @DllStr
                                   chars  : array [0..259] of WideChar;
                                 end;
        stub                   : packed record
                                   movEax : byte;
                                   buf    : pointer;
                                   jmp    : byte;
                                   target : integer;
                                 end;
      end;

    const
      CNewEntryPoint32 : array [0..154] of byte =
        ($55, $8b, $ec, $83, $c4, $f4, $53, $56, $57, $8b, $d8, $8b, $75, $0c, $83, $fe,
         $01, $75, $49, $8b, $03, $89, $45, $fc, $c7, $45, $f8, $04, $00, $00, $00, $8d,
         $45, $f4, $50, $6a, $40, $8d, $45, $f8, $50, $8d, $45, $fc, $50, $6a, $ff, $ff,
         $53, $0c, $85, $c0, $75, $26, $8b, $53, $04, $8b, $03, $89, $10, $89, $45, $fc,
         $c7, $45, $f8, $04, $00, $00, $00, $8d, $45, $f4, $50, $8b, $45, $f4, $50, $8d,
         $45, $f8, $50, $8d, $45, $fc, $50, $6a, $ff, $ff, $53, $0c, $83, $7b, $04, $00,
         $74, $14, $8b, $45, $10, $50, $56, $8b, $45, $08, $50, $ff, $53, $08, $85, $c0,
         $75, $04, $33, $c0, $eb, $02, $b0, $01, $f6, $d8, $1b, $ff, $83, $fe, $01, $75,
         $0f, $8d, $45, $f8, $50, $8d, $43, $14, $50, $6a, $00, $6a, $00, $ff, $53, $10,
         $8b, $c7, $5f, $5e, $5b, $8b, $e5, $5d, $c2, $0c, $00);
(*
      function NewEntryPoint(var injectRec: TInjectRec; d1, d2, reserved, reason, hInstance: dword) : bool;
      var p1     : pointer;
          c1, c2 : dword;
      begin
        with injectRec do begin
          if reason = DLL_PROCESS_ATTACH then begin
            p1 := pEntryPoint;
            c1 := 4;
            if NtProtectVirtualMemory($ffffffff, p1, c1, PAGE_EXECUTE_READWRITE, c2) = 0 then begin
              pEntryPoint^ := oldEntryPoint;
              p1 := pEntryPoint;
              c1 := 4;
              NtProtectVirtualMemory($ffffffff, p1, c1, c2, c2);
            end;
          end;
          result := (oldEntryPoint = 0) or oldEntryPointFunc(hInstance, reason, reserved);
          if reason = DLL_PROCESS_ATTACH then
            LdrLoadDll(0, nil, @dll, c1);
        end;
      end;
*)
    var c1, c2, c3 : dword;
        ir         : TInjectRec;
        buf        : pointer;
    begin
      result := 0;
      with ir do begin
        pEntryPoint := NtProc('');  // get the address where ntdll.dll's entry point is stored
        {$ifdef log}log('ntdll.dll''s entry point is stored at ' + IntToHexEx(dword(pEntryPoint)));{$endif}
        if (pEntryPoint <> nil) and
           ReadProcessMemory(process, pEntryPoint, @oldEntryPoint, 4, c1) then begin
          buf := AllocMemEx(sizeOf(CNewEntryPoint32) + sizeOf(ir), process);
          if oldEntryPoint = 0 then
               oldEntryPointFunc := nil
          else oldEntryPointFunc := pointer(ntdllhandle + oldEntryPoint);
          NtProtectVirtualMemory := NtProc(CNtProtectVirtualMemory, true);
          LdrLoadDll := NtProc(CLdrLoadDll, true);
          dll.len    := lstrlenW(libW) * 2;
          dll.maxLen := dll.len + 2;
          dll.str    := pointer(dword(buf) + sizeOf(CNewEntryPoint32) + (dword(@dll.chars) - dword(@ir)));
          Move(libW^, dll.chars, dll.maxLen);
          stub.movEax := $b8;  // mov eax, dw
          stub.buf    := pointer(dword(buf) + sizeOf(CNewEntryPoint32));
          stub.jmp    := $e9;  // jmp dw
          stub.target := - sizeOf(CNewEntryPoint32) - sizeOf(ir);
          c2 := dword(buf) + sizeOf(CNewEntryPoint32) + sizeOf(ir) - sizeOf(stub) - ntdllhandle;
          if WriteProcessMemory(process, buf, @CNewEntryPoint32, sizeOf(CNewEntryPoint32), c1) and
             WriteProcessMemory(process, pointer(dword(buf) + sizeOf(CNewEntryPoint32)), @ir, sizeOf(ir), c1) and
             VirtualProtectEx(process, pEntryPoint, 4, PAGE_EXECUTE_READWRITE, @c3) and
             WriteProcessMemory(process, pEntryPoint, @c2, 4, c1) then begin
            VirtualProtectEx(process, pEntryPoint, 4, c3, @c1);
            result := 1;
          end;
        end;
      end;
    end;

    type
      PInjectLib32 = ^TInjectLib32;
      TInjectLib32 = packed record
        movEax     : byte;                         // 0xb8
        param      : PInjectLib32;                 // pointer to "self"
        movEcx     : byte;                         // 0xb9
        proc       : pointer;                      // pointer to "self.injectFunc"
        callEcx    : word;                         // 0xd1ff
        pOldApi    : ^TRelJump;                    // which code location in user land do we patch?
        oldApi     : TRelJump;                     // patch backup buffer, contains overwritten code
        dll        : TUnicodeStr;                  // dll path/name
        dllBuf     : array [0..259] of WideChar;   // string buffer for the dll path/name
        npvm       : pointer;                      // ntdll.NtProtectVirtualMemory
        lld        : pointer;                      // ntdll.LdrLoadDll
        injectFunc : byte;                         // will be filled with CInjectLibFunc32 (see below)
      end;

    const CInjectLibFunc64 : array [0..159] of byte =
      ($40, $53, $48, $83, $EC, $30, $48, $8B, $41, $2B, $48, $8B, $D9, $4C, $8D, $44,
       $24, $48, $48, $89, $44, $24, $50, $48, $8D, $44, $24, $40, $48, $8D, $54, $24,
       $50, $41, $B9, $40, $00, $00, $00, $48, $83, $C9, $FF, $48, $C7, $44, $24, $48,
       $05, $00, $00, $00, $48, $89, $44, $24, $20, $FF, $93, $50, $02, $00, $00, $8B,
       $43, $33, $4C, $8B, $5B, $2B, $4C, $8D, $44, $24, $48, $48, $8D, $54, $24, $50,
       $41, $89, $03, $0F, $B6, $43, $37, $48, $83, $C9, $FF, $41, $88, $43, $04, $44,
       $8B, $4C, $24, $40, $48, $8D, $44, $24, $40, $48, $89, $44, $24, $20, $48, $C7,
       $44, $24, $48, $05, $00, $00, $00, $FF, $93, $50, $02, $00, $00, $4C, $8D, $43,
       $38, $4C, $8D, $4C, $24, $48, $33, $D2, $33, $C9, $FF, $93, $58, $02, $00, $00,
       $48, $83, $C4, $30, $5B, // $C3)
       $48, $83, $C4, $28, $41, $59, $41, $58, $5A, $59, $C3);

    // the CInjectLibFunc64 data is a compilation of the following C++ code
    // it got a manually created extended "tail", though (last 11 bytes)
    // this user land code is copied to newly created 64bit processes to execute the dll injection

    // static void LoadLibrariesFunc( InjectLib64 *buf )
    // {
    //   LPVOID p1 = (LPVOID)buf->pOldApi;
    //   ULONG_PTR c1 = 5;
    //   ULONG c2;
    //
    //   buf->npvm((HANDLE)-1, &p1, &c1, PAGE_EXECUTE_READWRITE, &c2);
    //   *(buf->pOldApi) = buf->oldApi;
    //   c1 = 5;
    //   buf->npvm((HANDLE)-1, &p1, &c1, c2, &c2);
    //   buf->lld(0, NULL, &(buf->dll), (PHANDLE) &c1);
    // }

    const CInjectLibFunc32 : array [0..125] of byte =
      ($55, $8B, $EC, $83, $C4, $F4, $53, $8B, $D8, $89, $6D, $FC, $8B, $43, $0C, $8B,
       $55, $FC, $83, $C2, $04, $89, $02, $8B, $43, $0C, $89, $45, $F4, $C7, $45, $FC,
       $05, $00, $00, $00, $8D, $45, $F8, $50, $6A, $40, $8D, $45, $FC, $50, $8D, $45,
       $F4, $50, $6A, $FF, $FF, $93, $25, $02, $00, $00, $8B, $43, $0C, $8B, $53, $10,
       $89, $10, $8A, $53, $14, $88, $50, $04, $C7, $45, $FC, $05, $00, $00, $00, $8D,
       $45, $F8, $50, $8B, $45, $F8, $50, $8D, $45, $FC, $50, $8D, $45, $F4, $50, $6A,
       $FF, $FF, $93, $25, $02, $00, $00, $8D, $45, $FC, $50, $8D, $43, $15, $50, $6A,
       $00, $6A, $00, $FF, $93, $29, $02, $00, $00, $5B, $8B, $E5, $5D, $C3);

    // the CInjectLibFunc32 data is a compilation of the following Delphi code
    // this user land code is copied to newly created 32bit processes to execute the dll injection

    // procedure LoadLibrariesFunc(var buf: InjectLib32);
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
    //   buf.lld(0, nil, @buf.dll, c1);
    // end;

    function Find32bitPatchAddress(module: dword; const nh: TImageNtHeaders) : pointer;
    begin
      if VmWareInjectionMode or Am64OS then begin
        if nh.OptionalHeader.AddressOfEntryPoint <> 0 then
          result := pointer(module + nh.OptionalHeader.AddressOfEntryPoint)
        else
          result := nil;
      end else
        result := NtProc(CNtTestAlert, true);
    end;

    function InjectLibraryPatchNt32(process, pid, module: dword; const nh: TImageNtHeaders; dd: pointer) : integer;
    // inject a dll into a newly started 32bit process
    var buf : PInjectLib32;
        il  : TInjectLib32;
        rj  : TRelJump;
        c1  : dword;
        op  : dword;
    begin
      result := 0;
      buf := AllocMemEx(sizeOf(TInjectLib32) + sizeof(CInjectLibFunc32), process);
      if buf <> nil then begin
        // we were able to allocate a buffer in the newly created process
        if @LdrLoadDllProc = nil then
          LdrLoadDllProc := NtProc(CLdrLoadDll, true);
        if @NtProtectVirtualMemoryProc = nil then
          NtProtectVirtualMemoryProc := NtProc(CNtProtectVirtualMemory, true);
        il.movEax     := $b8;
        il.param      := buf;
        il.movEcx     := $b9;
        il.proc       := @buf.injectFunc;
        il.callEcx    := $d1ff;
        il.pOldApi    := Find32bitPatchAddress(module, nh);
        il.dll.len    := lstrlenW(libW) * 2;
        il.dll.maxLen := il.dll.len + 2;
        il.dll.str    := buf.dllBuf;
        lstrcpyW(il.dllBuf, libW);
        il.npvm       := @NtProtectVirtualMemoryProc;
        il.lld        := @LdrLoadDllProc;
        if (il.pOldApi <> nil) and
           WriteProcessMemory(process, buf, @il, sizeOf(TInjectLib32), c1) and (c1 = sizeOf(TInjectLib32)) and
           WriteProcessMemory(process, @buf.injectFunc, @CInjectLibFunc32, sizeOf(CInjectLibFunc32), c1) and (c1 = sizeOf(CInjectLibFunc32)) and
           ReadProcessMemory (process, il.pOldApi, @rj, 5, c1) and (c1 = 5) and
           WriteProcessMemory(process, @buf.oldApi, @rj, 5, c1) and (c1 = 5) then begin
          // we've successfully initialized the buffer
          if VirtualProtectEx(process, il.pOldApi, 5, PAGE_EXECUTE_READWRITE, @op) then begin
            // we successfully unprotected the to-be-patched code, so now we can patch it
            rj.jmp    := $e9;
            rj.target := dword(buf) - dword(il.pOldApi) - 5;
            WriteProcessMemory(process, il.pOldApi, @rj, 5, c1);
            // now restore the original page protection
            VirtualProtectEx(process, il.pOldApi, 5, op, @op);
            result := 1;
          end;
        end;
      end;
    end;

    type
      PInjectLib64 = ^TInjectLib64;
      TInjectLib64 = packed record
        movRax     : word;                        // 0xb848
        retAddr    : int64;                       // patched API
        pushRax    : byte;                        // 0x50
        pushRcx    : byte;                        // 0x51
        pushRdx    : byte;                        // 0x52
        pushR8     : word;                        // 0x5041
        pushR9     : word;                        // 0x5141
        subRsp28   : dword;                       // 0x28ec8348
        movRcx     : word;                        // 0xb948
        param      : int64;                       // pointer to "self"
        movRdx     : word;                        // 0xba48
        proc       : int64;                       // pointer to "self.injectFunc"
        jmpEdx     : word;                        // 0xe2ff
        pOldApi    : int64;                       // which code location in user land do we patch?
        oldApi     : TRelJump;                    // patch backup buffer, contains overwritten code
        dll        : TUnicodeStr64;               // dll path/name
        dllBuf     : array [0..259] of WideChar;  // string buffer for the dll path/name
        npvm       : int64;                       // ntdll.NtProtectVirtualMemory
        lld        : int64;                       // ntdll.LdrLoadDll
        injectFunc : byte;                        // will be filled with CInjectLibFunc64 (see below)
      end;

    function InjectLibraryPatchNt64(process, pid, module: dword; const nh: TImageNtHeaders; dd: pointer) : integer;
    // inject a dll into a newly started 64bit process

      function GetModuleHandle64(dll: AnsiString) : dword;

        function GetPeb64 : int64;
        asm
          mov eax, gs:[$60]
          mov edx, gs:[$64]
        end;

      var i64     : int64;
          loopEnd : pointer;
          mi      : TPAPointer;
      begin
        result := 0;
        try
          // first let's get the 64bit PEB (Process Environment Block)
          i64 := GetPeb64;
          if dword(i64) = i64 then begin
            // we got it - and it's only a 32bit pointer, fortunately
            // now let's get the loader data
            i64 := int64(pointer(i64 + $18)^);
            if dword(i64) = i64 then begin
              // fortunately the loader also is only a 32bit pointer
              // now let's loop through the list of dlls
              loopEnd := pointer(i64 + $20);
              mi := loopEnd;
              while (mi[0] <> loopEnd) and (mi[1] = nil) do begin
                // mi[ 0] + mi[ 1] = pointer to next dll
                // mi[ 8] + mi[ 9] = dll handle
                // mi[16] + mi[17] = full dll file name
                mi := mi[0];
                if (mi[16] <> nil) and (mi[17] = nil) and (PosText(dll, AnsiString(UnicodeString(PWideChar(mi[16])))) > 0) then begin
                  // found the dll we're looking for
                  if mi[9] = nil then
                    // and it happens to have a 32bit handle - yippieh!
                    result := dword(mi[8]);
                  break;
                end;
              end;
            end;
          end;
        except end;
      end;

    var buf : PInjectLib64;
        il  : TInjectLib64;
        rj  : TRelJump;
        c1  : dword;
        op  : dword;
        dll : dword;
    begin
      result := 0;
      buf := AllocMemEx(sizeOf(TInjectLib64) + sizeOf(CInjectLibFunc64), process);
      if buf <> nil then begin
        // we were able to allocate a buffer in the newly created process
        dll := GetModuleHandle64('ntdll.dll');
        if dll <> 0 then begin
          il.movRax     := $b848;
          il.retAddr    := int64(GetImageProcAddress(dll, 'NtTestAlert'));
          il.pushRax    := $50;
          il.pushRcx    := $51;
          il.pushRdx    := $52;
          il.pushR8     := $5041;
          il.pushR9     := $5141;
          il.subRsp28   := $28ec8348;
          il.movRcx     := $b948;
          il.param      := int64(buf);
          il.movRdx     := $ba48;
          il.proc       := int64(@buf.injectFunc);
          il.jmpEdx     := $e2ff;
          il.pOldApi    := il.retAddr;
          il.dll.len    := lstrlenW(libW) * 2;
          il.dll.maxLen := il.dll.len + 2;
          il.dll.dummy  := 0;
          il.dll.str    := int64(@buf.dllBuf);
          lstrcpyW(il.dllBuf, libW);
          il.npvm       := int64(GetImageProcAddress(dll, 'NtProtectVirtualMemory'));
          il.lld        := int64(GetImageProcAddress(dll, 'LdrLoadDll'));

          if WriteProcessMemory(process, buf, @il, sizeOf(TInjectLib64), c1) and (c1 = sizeOf(TInjectLib64)) and
             WriteProcessMemory(process, @buf.injectFunc, @CInjectLibFunc64, sizeOf(CInjectLibFunc64), c1) and (c1 = sizeOf(CInjectLibFunc64)) and
             ReadProcessMemory (process, pointer(il.pOldApi), @rj, 5, c1) and (c1 = 5) and
             WriteProcessMemory(process, @buf.oldApi, @rj, 5, c1) and (c1 = 5) then begin
            // we've successfully initialized the buffer
            if VirtualProtectEx(process, pointer(il.pOldApi), 5, PAGE_EXECUTE_READWRITE, @op) then begin
              // we successfully unprotected the to-be-patched code, so now we can patch it
              rj.jmp    := $e9;
              rj.target := dword(buf) - dword(il.pOldApi) - 5;
              WriteProcessMemory(process, pointer(il.pOldApi), @rj, 5, c1);
              // now restore the original page protection
              VirtualProtectEx(process, pointer(il.pOldApi), 5, op, @op);
              result := 1;
            end;
          end;
        end;
      end;
    end;

  var module : dword;
      nh     : TImageNtHeaders;
      dd     : pointer;
      os     : TOsVersionInfo;
  begin
    {$ifdef log}
      log('InjectLibraryPatch (process: %p)', nil, nil, process);
    {$endif}
    if forcePatch or NotInitializedYet(process, pid) then begin
      {$ifdef log}log('NotInitializedYet (1) +');{$endif}
      if GetExeModuleInfos(process, module, nh, dd) then begin
        {$ifdef log}log('GetExeModuleInfos +');{$endif}
        ZeroMemory(@os, sizeOf(os));
        os.dwOSVersionInfoSize := sizeOf(os);
        GetVersionEx(os);
        if os.dwPlatformId = VER_PLATFORM_WIN32_NT then begin
          if Is64bitProcess(process) then
            result := InjectLibraryPatchNt64(process, pid, module, nh, dd)
          else
            {$ifdef InjectLibraryPatchXp}
              if ((os.dwMajorVersion > 5) or ((os.dwMajorVersion = 5) and (os.dwMinorVersion > 0))) and
                 (not Is64bitOS) then
                result := InjectLibraryPatchXp32(process)
              else
            {$endif}
              result := InjectLibraryPatchNt32(process, pid, module, nh, dd);
            if (result = 1) and (forcePatch or NotInitializedYet(process, pid)) then
              result := 2;
        end else
          result := InjectLibraryPatch9x(process, pid, module, nh, dd);
      end else result := 0;
    end else result := 1;
    {$ifdef log}
      log('InjectLibraryPatch (process: %p) -> ' + IntToStrEx(result), nil, nil, process);
    {$endif}
  end;

  function DoInject(process, pid: dword; var thread: dword) : boolean;
  begin
    {$ifdef log}
      log('DoInject (process: %p)', nil, nil, process);
    {$endif}
    result := true;
    thread := 0;
    if GetVersion and $80000000 = 0 then begin
      peb32 := GetPeb(process);
      if Is64bitProcess(process) then
        peb64 := GetPeb64(process)
      else
        peb64 := 0;
    end;
    if forcePatch or NotInitializedYet(process, pid) then begin
      if mayPatch then begin
        case InjectLibraryPatch(process, pid) of
          0 : result := false;
          1 : begin
                if mayThread then
                     thread := StartInjectLibraryX(process, pid, true, libA, libW)
                else thread := 0;
                result := thread <> 0;
              end;
        end;
      end else
        result := false;
    end else begin
      if mayThread then
           thread := StartInjectLibraryX(process, pid, true, libA, libW)
      else thread := 0;
      result := thread <> 0;
    end;
    {$ifdef log}
      log('DoInject (process: %p) -> ' + booleanToChar(result), nil, nil, process);
    {$endif}
  end;

var prcs1, prcs2 : TDAProcess;
    ths          : TDACardinal;
    i1, i2       : integer;
    c1           : dword;
    b1, b2       : boolean;
    newMap       : integer;
    newMutex     : boolean;
    sla          : AnsiString;
    slw          : AnsiString;
    time         : dword;
    delay        : boolean;
    count        : integer;
    ws1          : UnicodeString;
    incPaths     : array of UnicodeString;
    incNames     : array of UnicodeString;
    excPaths     : array of UnicodeString;
    excNames     : array of UnicodeString;
    needFullPath : boolean;
    name         : UnicodeString;
    path         : UnicodeString;
    helperBuf    : PWideChar;
begin
  {$ifdef log}
    log('InjectLibraryX (process: ' + '%p' +
                      '; lib: '     + '%1' +
                      '; timeOut: ' + IntToStrEx(timeOut) +
                        ')', libA, libW, process);
  {$endif}
  prcs1 := nil;
  prcs2 := nil;
  incPaths := nil;
  incNames := nil;
  excPaths := nil;
  excNames := nil;
  result := false;
  time := 0;
  if not CheckLibFilePath(libA, libW, sla, slw) then
    exit;
  EnableAllPrivileges;
  InitProcs(false);
  if multiInject then begin
    if GetVersion and $80000000 = 0 then begin
      c1 := GetSmssProcessHandle;
      if c1 = 0 then begin
        SetLastError(ERROR_ACCESS_DENIED);
        exit;
      end;
      CloseHandle(c1);
      if Is64bitModule(libW) then begin
        SetLastError(ERROR_NOT_SUPPORTED);
        exit;
      end;
      if not StartDllInjection(driverName, libW, session, system, includes, excludes) then
        exit;
    end;
    if InjectIntoRunningProcesses then begin
      if session = CURRENT_SESSION then
        session := GetCurrentSessionId;
      result := (GetVersion and $80000000 = 0) or AddInjectDll(owner, libA, libW, system, newMap, newMutex);
      needFullPath := false;
      if (includes <> nil) and (includes[0] <> #0) then begin
        ws1 := includes;
        SetLength(incPaths, SubStrCount(ws1));
        SetLength(incNames, Length(incPaths));
        for i1 := 0 to high(incPaths) do
          needFullPath := SplitNamePath(SubStr(ws1, i1 + 1), incPaths[i1], incNames[i1]) or needFullPath;
      end;
      if (excludes <> nil) and (excludes[0] <> #0) then begin
        ws1 := excludes;
        SetLength(excPaths, SubStrCount(ws1));
        SetLength(excNames, Length(excPaths));
        for i1 := 0 to high(excPaths) do
          needFullPath := SplitNamePath(SubStr(ws1, i1 + 1), excPaths[i1], excNames[i1]) or needFullPath;
      end;
      if needFullPath then
        helperBuf := pointer(LocalAlloc(LPTR, 64 * 1024))
      else
        helperBuf := nil;
      i1 := Length(excPaths);
      SetLength(excPaths, i1 + 2);
      SetLength(excNames, i1 + 2);
      excNames[i1 + 0] := UnicodeString(DecryptStr(CSmss));
      excNames[i1 + 1] := UnicodeString(DecryptStr(CSLSvc));
      delay := false;
      count := 0;
      repeat
        if delay then
          Sleep(10);
        delay := false;
        b2 := true;
        {$ifdef log} Log('EnumProcesses'); {$endif}
        prcs1 := EnumProcesses;
        {$ifdef log} LogProcesses(prcs1); {$endif}
        SetLength(ths, Length(prcs1));
        for i1 := 0 to high(prcs1) do begin
          ths[i1] := 0;
          b1 := true;
          for i2 := 0 to high(prcs2) do
            if prcs2[i2].id = prcs1[i1].id then begin
              b1 := false;
              break;
            end;
          if excludePIDs <> nil then begin
            i2 := 0;
            while (excludePIDs[i2] <> 0) and (excludePIDs[i2] <> prcs1[i1].id) do
              inc(i2);
            if excludePIDs[i2] <> 0 then
              b1 := false;
          end;
          if b1 then begin
            if (not SplitNamePath(prcs1[i1].exeFile, path, name)) and
               needFullPath and
               ProcessIdToFileNameW(prcs1[i1].id, helperBuf, 32 * 1024) then
              SplitNamePath(helperBuf, path, name);
            if ((incPaths = nil) or MatchStrArray(path, name, incPaths, incNames)) and
               (not MatchStrArray(path, name, excPaths, excNames)) then begin
              b2 := false;
              c1 := OpenProcess(PROCESS_CREATE_THREAD or PROCESS_DUP_HANDLE or PROCESS_QUERY_INFORMATION or
                                PROCESS_VM_OPERATION or PROCESS_VM_READ or PROCESS_VM_WRITE, false, prcs1[i1].id);
              if c1 <> 0 then begin
                if not Is64bitProcess(c1) then begin
                  if ( system or (not IsSystemProcess(c1, prcs1[i1].sid)) ) and
                     ( (session = ALL_SESSIONS) or
                       (GetProcessSessionId(prcs1[i1].id) = session) or
                       ((GetProcessSessionId(prcs1[i1].id) = 0) and IsSystemProcess(c1, prcs1[i1].sid)) ) then
                    DoInject(c1, prcs1[i1].id, ths[i1]);
                end;
                CloseHandle(c1);
              end else begin
                c1 := GetLastError;
                {$ifdef log}
                  log('  OpenProcess (' + IntToHexEx(prcs1[i1].id) + ') -> error ' + IntToHexEx(c1));
                {$endif}
                if (prcs1[i1].id <> 0) and (c1 <> ERROR_ACCESS_DENIED) then begin
                  prcs1[i1].id := 0;
                  delay := true;
                end;
              end;
            end;
          end;
        end;
        if time = 0 then
          time := GetTickCount;
        for i1 := 0 to high(ths) do
          if ths[i1] <> 0 then
            WaitForInjectLibraryX(ths[i1], timeOut, time);
        prcs2 := prcs1;
        {$ifdef log}
          if not b2 then
            log('Did we miss some processes? Check again...');
        {$endif}
        inc(count);
      until b2 or (count = 5);
      if helperBuf <> nil then
        LocalFree(dword(helperBuf));
    end else
      result := true;
  end else
    result := (forcePatch or (not Is64bitProcess(process))) and
              DoInject(process, ProcessHandleToId(process), c1) and
              ( (c1 = 0) or WaitForInjectLibraryX(c1, timeOut, GetTickCount) );
  {$ifdef log}
    log('InjectLibraryX (process: ' + '%p' +
                      '; lib: '     + libA +
                      '; timeOut: ' + IntToStrEx(timeOut) +
                        ') -> ' + booleanToChar(result), nil, nil, process);
  {$endif}
end;

// ***************************************************************

function UninjectLibraryX(owner       : dword;
                          libA        : PAnsiChar;
                          libW        : PWideChar;
                          multiInject : boolean;
                          process     : dword;
                          driverName  : PWideChar;
                          session     : dword;
                          system      : boolean;
                          includes    : PWideChar;
                          excludes    : PWideChar;
                          excludePIDs : TPACardinal;
                          timeOut     : dword) : boolean;
var prcs1, prcs2 : TDAProcess;
    ths          : TDACardinal;
    ph           : dword;
    i1, i2       : integer;
    c1           : dword;
    sla          : AnsiString;
    slw          : AnsiString;
    time         : dword;
    b1, b2       : boolean;
    ws1          : UnicodeString;
    incPaths     : array of UnicodeString;
    incNames     : array of UnicodeString;
    excPaths     : array of UnicodeString;
    excNames     : array of UnicodeString;
    needFullPath : boolean;
    name         : UnicodeString;
    path         : UnicodeString;
    helperBuf    : PWideChar;
begin
  {$ifdef log}
    log('UninjectLibraryX (process: ' + '%p' +
                        '; lib: '     + '%1' +
                        '; timeOut: ' + IntToStrEx(timeOut) +
                          ')', libA, libW, process);
  {$endif}
  prcs1 := nil;
  prcs2 := nil;
  incPaths := nil;
  incNames := nil;
  excPaths := nil;
  excNames := nil;
  result := false;
  time := 0;
  if not CheckLibFilePath(libA, libW, sla, slw) then
    exit;
  EnableAllPrivileges;
  InitProcs(false);
  if multiInject then begin
    if GetVersion and $80000000 = 0 then begin
      c1 := GetSmssProcessHandle;
      if c1 = 0 then begin
        SetLastError(ERROR_ACCESS_DENIED);
        exit;
      end;
      CloseHandle(c1);
      if Is64bitModule(libW) then begin
        SetLastError(ERROR_NOT_SUPPORTED);
        exit;
      end;
      if not StopDllInjection(driverName, libW, session, system, includes, excludes) then
        exit;
    end;
    if UninjectFromRunningProcesses then begin
      if session = CURRENT_SESSION then
        session := GetCurrentSessionId;
      result := (GetVersion and $80000000 = 0) or DelInjectDll(owner, libA, libW, system, false);  //process = STOP_VIRUS);
      needFullPath := false;
      if (includes <> nil) and (includes[0] <> #0) then begin
        ws1 := includes;
        SetLength(incPaths, SubStrCount(ws1));
        SetLength(incNames, Length(incPaths));
        for i1 := 0 to high(incPaths) do
          needFullPath := SplitNamePath(SubStr(ws1, i1 + 1), incPaths[i1], incNames[i1]) or needFullPath;
      end;
      if (excludes <> nil) and (excludes[0] <> #0) then begin
        ws1 := excludes;
        SetLength(excPaths, SubStrCount(ws1));
        SetLength(excNames, Length(excPaths));
        for i1 := 0 to high(excPaths) do
          needFullPath := SplitNamePath(SubStr(ws1, i1 + 1), excPaths[i1], excNames[i1]) or needFullPath;
      end;
      if needFullPath then
        helperBuf := pointer(LocalAlloc(LPTR, 64 * 1024))
      else
        helperBuf := nil;
      i1 := Length(excPaths);
      SetLength(excPaths, i1 + 2);
      SetLength(excNames, i1 + 2);
      excNames[i1 + 0] := UnicodeString(DecryptStr(CSmss));
      excNames[i1 + 1] := UnicodeString(DecryptStr(CSLSvc));
      repeat
        b2 := true;
        {$ifdef log} Log('EnumProcesses'); {$endif}
        prcs1 := EnumProcesses;
        {$ifdef log} LogProcesses(prcs1); {$endif}
        SetLength(ths, Length(prcs1));
        for i1 := 0 to high(prcs1) do begin
          ths[i1] := 0;
          b1 := true;
          for i2 := 0 to high(prcs2) do
            if prcs2[i2].id = prcs1[i1].id then begin
              b1 := false;
              break;
            end;
          if excludePIDs <> nil then begin
            i2 := 0;
            while (excludePIDs[i2] <> 0) and (excludePIDs[i2] <> prcs1[i1].id) do
              inc(i2);
            if excludePIDs[i2] <> 0 then
              b1 := false;
          end;
          if b1 then begin
            if (not SplitNamePath(prcs1[i1].exeFile, path, name)) and
               needFullPath and
               ProcessIdToFileNameW(prcs1[i1].id, helperBuf, 32 * 1024) then
              SplitNamePath(helperBuf, path, name);
            if ((incPaths = nil) or MatchStrArray(path, name, incPaths, incNames)) and
               (not MatchStrArray(path, name, excPaths, excNames)) then begin
              b2 := false;
              ph := OpenProcess(PROCESS_ALL_ACCESS, false, prcs1[i1].id);
              if ph <> 0 then begin
                if (not Is64bitProcess(ph)) and
                   ( system or (not IsSystemProcess(ph, prcs1[i1].sid)) ) and
                   ( (session = ALL_SESSIONS) or
                     (GetProcessSessionId(prcs1[i1].id) = session) or
                     ((GetProcessSessionId(prcs1[i1].id) = 0) and IsSystemProcess(ph, prcs1[i1].sid)) ) then
                  ths[i1] := StartInjectLibraryX(ph, prcs1[i1].id, false, libA, libW);
                CloseHandle(ph);
              end;
            end;
          end;
        end;
        if time = 0 then
          time := GetTickCount;
        for i1 := 0 to high(ths) do
          if ths[i1] <> 0 then
            WaitForInjectLibraryX(ths[i1], timeOut, time);
        prcs2 := prcs1;
        {$ifdef log}
          if not b2 then
            log('Did we miss some processes? Check again...');
        {$endif}
      until b2;
      if helperBuf <> nil then
        LocalFree(dword(helperBuf));
    end else
      result := true;
  end else begin
    c1 := StartInjectLibraryX(process, ProcessHandleToId(process), false, libA, libW);
    result := (c1 <> 0) and WaitForInjectLibraryX(c1, timeOut, GetTickCount);
  end;
  {$ifdef log}
    log('UninjectLibraryX (process: ' + '%p' +
                        '; lib: '     + libA +
                        '; timeOut: ' + IntToStrEx(timeOut) +
                          ') -> ' + booleanToChar(result), nil, nil, process);
  {$endif}
end;

// ***************************************************************

function UninjectAllLibrariesW(driverName: PWideChar; excludePIDs: TPCardinal = nil; timeOutPerUninject: dword = 7000) : bool; stdcall;
var uir        : TDAInjectionRequest;
    i1         : integer;
    incl, excl : PWideChar;
begin
  result := true;
  uir := EnumInjectionRequests(driverName);
  for i1 := 0 to high(uir) do begin
    if uir[i1].IncludeLen > 0 then
      incl := uir[i1].Data
    else
      incl := nil;
    if uir[i1].ExcludeLen > 0 then
      excl := pointer(dword(@uir[i1].Data) + uir[i1].IncludeLen * 2 + 2)
    else
      excl := nil;
    if not UninjectLibraryX(GetCallingModule, nil, uir[i1].Name, true, 0, driverName, uir[i1].Session, uir[i1].Flags and $1 <> 0, incl, excl, TPACardinal(excludePIDs), timeOutPerUninject) then
      result := false;
    LocalFree(dword(uir[i1]));
  end;
end;

function UninjectAllLibrariesA(driverName: PAnsiChar; excludePIDs: TPCardinal = nil; timeOutPerUninject: dword = 7000) : bool; stdcall;
var driverNameW : PWideChar;
begin
  if (driverName <> nil) and (driverName[0] <> #0) then begin
    driverNameW := pointer(LocalAlloc(LPTR, lstrlenA(driverName) * 2 + 2));
    AnsiToWide2(driverName, driverNameW);
  end else
    driverNameW := nil;
  result := UninjectAllLibrariesW(driverNameW, excludePIDs, timeOutPerUninject);
end;

function UninjectAllLibraries(driverName: PAnsiChar; excludePIDs: TPCardinal = nil; timeOutPerUninject: dword = 7000) : bool; stdcall;
begin
  result := UninjectAllLibrariesA(driverName, excludePIDs, timeOutPerUninject);
end;

// ***************************************************************

function CPInject(owner: dword; pi: TProcessInformation; flags: dword; libA: PAnsiChar; libW: PWideChar) : boolean;
begin
  result := InjectLibraryX(owner, libA, libW, false, pi.hProcess, nil, 0, true, nil, nil, nil, INFINITE, true, false, true);
  if result then begin
    if flags and CREATE_SUSPENDED = 0 then
      ResumeThread(pi.hThread);
  end else
    TerminateProcess(pi.hProcess, 0);
end;

function CreateProcessExA(applicationName, commandLine : PAnsiChar;
                          processAttr, threadAttr      : PSecurityAttributes;
                          inheritHandles               : BOOL;
                          creationFlags                : DWORD;
                          environment                  : pointer;
                          currentDirectory             : PAnsiChar;
                          const startupInfo            : {$ifdef UNICODE} TStartupInfoA; {$else} TStartupInfo; {$endif}
                          var processInfo              : TProcessInformation;
                          loadLibrary                  : PAnsiChar          ) : bool; stdcall;
begin
  {$ifdef log}
    log('CreateProcessExA (lib: ' + loadLibrary + ')');
  {$endif}
  result := CreateProcessA(applicationName, commandLine, processAttr, threadAttr,
                           inheritHandles, creationFlags or CREATE_SUSPENDED,
                           environment, currentDirectory, startupInfo, processInfo) and
            CPInject(GetCallingModule, processInfo, creationFlags, loadLibrary, nil);
  {$ifdef log}
    log('CreateProcessExA (lib: ' + loadLibrary + ') -> ' + booleanToChar(result));
  {$endif}
end;

function CreateProcessExW(applicationName, commandLine : PWideChar;
                          processAttr, threadAttr      : PSecurityAttributes;
                          inheritHandles               : bool;
                          creationFlags                : dword;
                          environment                  : pointer;
                          currentDirectory             : PWideChar;
                          const startupInfo            : {$ifdef UNICODE} TStartupInfoW; {$else} TStartupInfo; {$endif}
                          var processInfo              : TProcessInformation;
                          loadLibrary                  : PWideChar          ) : bool; stdcall;
var unicows : dword;
    cpw     : function (app, cmd, pattr, tattr: pointer; inherit: bool; flags: dword;
                        env, dir: pointer; const si: {$ifdef UNICODE} TStartupInfoW; {$else} TStartupInfo; {$endif} var pi: TProcessInformation) : bool; stdcall;
begin
  {$ifdef log}
    log('CreateProcessExW (lib: %1)', nil, loadLibrary);
  {$endif}
  cpw := nil;
  if GetVersion and $80000000 <> 0 then begin
    unicows := GetModuleHandleA(PAnsiChar(DecryptStr(CUnicows)));
    if unicows = 0 then
      unicows := LoadLibraryA(PAnsiChar(DecryptStr(CUnicows)));
    if unicows <> 0 then
      cpw := GetProcAddress(unicows, PAnsiChar(DecryptStr(CCreateProcessW)));
  end;
  if @cpw = nil then
    cpw := @CreateProcessW;
  result := cpw(applicationName, commandLine, processAttr, threadAttr,
                inheritHandles, creationFlags or CREATE_SUSPENDED,
                environment, currentDirectory, startupInfo, processInfo) and
            CPInject(GetCallingModule, processInfo, creationFlags, nil, loadLibrary);
  {$ifdef log}
    log('CreateProcessExW (lib: %1) -> ' + booleanToChar(result), nil, loadLibrary);
  {$endif}
end;

function CreateProcessEx(applicationName, commandLine : PAnsiChar;
                         processAttr, threadAttr      : PSecurityAttributes;
                         inheritHandles               : bool;
                         creationFlags                : dword;
                         environment                  : pointer;
                         currentDirectory             : PAnsiChar;
                         const startupInfo            : {$ifdef UNICODE} TStartupInfoA; {$else} TStartupInfo; {$endif}
                         var processInfo              : TProcessInformation;
                         loadLibrary                  : AnsiString         ) : boolean;
begin
  {$ifdef log}
    log('CreateProcessEx (lib: ' + loadLibrary + ')');
  {$endif}
  result := CreateProcessA(applicationName, commandLine, processAttr, threadAttr,
                           inheritHandles, creationFlags or CREATE_SUSPENDED,
                           environment, currentDirectory, startupInfo, processInfo) and
            CPInject(GetCallingModule, processInfo, creationFlags, PAnsiChar(loadLibrary), nil);
  {$ifdef log}
    log('CreateProcessEx (lib: ' + loadLibrary + ') -> ' + booleanToChar(result));
  {$endif}
end;

// ***************************************************************

function InjectLibraryA(libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
begin
  result := InjectLibraryX(GetCallingModule, libFileName, nil, false, processHandle, nil, 0, true, nil, nil, nil, timeOut, true, true, false);
end;

function InjectLibraryW(libFileName: PWideChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
begin
  result := InjectLibraryX(GetCallingModule, nil, libFileName, false, processHandle, nil, 0, true, nil, nil, nil, timeOut, true, true, false);
end;

function InjectLibrary(libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
begin
  result := InjectLibraryX(GetCallingModule, libFileName, nil, false, processHandle, nil, 0, true, nil, nil, nil, timeOut, true, true, false);
end;

function UninjectLibraryA(libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
begin
  result := UninjectLibraryX(GetCallingModule, libFileName, nil, false, processHandle, nil, 0, true, nil, nil, nil, timeOut);
end;

function UninjectLibraryW(libFileName: PWideChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
begin
  result := UninjectLibraryX(GetCallingModule, nil, libFileName, false, processHandle, nil, 0, true, nil, nil, nil, timeOut);
end;

function UninjectLibrary(libFileName: PAnsiChar; processHandle: dword; timeOut: dword = 7000) : bool; stdcall;
begin
  result := UninjectLibraryX(GetCallingModule, libFileName, nil, false, processHandle, nil, 0, true, nil, nil, nil, timeOut);
end;

function InjectLibraryA(driverName      : PAnsiChar;
                        libFileName     : PAnsiChar;
                        session         : dword;
                        systemProcesses : bool;
                        includeMask     : PAnsiChar  = nil;
                        excludeMask     : PAnsiChar  = nil;
                        excludePIDs     : TPCardinal = nil;
                        timeOut         : dword      = 7000) : bool; stdcall;
var driverNameW : PWideChar;
    includesW   : PWideChar;
    excludesW   : PWideChar;
begin
  if (driverName <> nil) and (driverName[0] <> #0) then begin
    driverNameW := pointer(LocalAlloc(LPTR, lstrlenA(driverName) * 2 + 2));
    AnsiToWide2(driverName, driverNameW);
  end else
    driverNameW := nil;
  if (includeMask <> nil) and (includeMask[0] <> #0) then begin
    includesW := pointer(LocalAlloc(LPTR, lstrlenA(includeMask) * 2 + 2));
    AnsiToWide2(includeMask, includesW);
  end else
    includesW := nil;
  if (excludeMask <> nil) and (excludeMask[0] <> #0) then begin
    excludesW := pointer(LocalAlloc(LPTR, lstrlenA(excludeMask) * 2 + 2));
    AnsiToWide2(excludeMask, excludesW);
  end else
    excludesW := nil;
  result := InjectLibraryX(GetCallingModule, libFileName, nil, true, 0, driverNameW, session, systemProcesses, includesW, excludesW, TPACardinal(excludePIDs), timeOut, true, true, false);
  if driverNameW <> nil then
    LocalFree(dword(driverNameW));
  if includesW <> nil then
    LocalFree(dword(includesW));
  if excludesW <> nil then
    LocalFree(dword(excludesW));
end;

function InjectLibraryW(driverName      : PWideChar;
                        libFileName     : PWideChar;
                        session         : dword;
                        systemProcesses : bool;
                        includeMask     : PWideChar  = nil;
                        excludeMask     : PWideChar  = nil;
                        excludePIDs     : TPCardinal = nil;
                        timeOut         : dword      = 7000) : bool; stdcall;
begin
  result := InjectLibraryX(GetCallingModule, nil, libFileName, true, 0, driverName, session, systemProcesses, includeMask, excludeMask, TPACardinal(excludePIDs), timeOut, true, true, false);
end;

function InjectLibrary(driverName      : PAnsiChar;
                       libFileName     : PAnsiChar;
                       session         : dword;
                       systemProcesses : bool;
                       includeMask     : PAnsiChar  = nil;
                       excludeMask     : PAnsiChar  = nil;
                       excludePIDs     : TPCardinal = nil;
                       timeOut         : dword      = 7000) : bool; stdcall;
begin
  result := InjectLibraryA(driverName, libFileName, session, systemProcesses, includeMask, excludeMask, excludePIDs, timeOut);
end;

function UninjectLibraryA(driverName      : PAnsiChar;
                          libFileName     : PAnsiChar;
                          session         : dword;
                          systemProcesses : bool;
                          includeMask     : PAnsiChar  = nil;
                          excludeMask     : PAnsiChar  = nil;
                          excludePIDs     : TPCardinal = nil;
                          timeOut         : dword      = 7000) : bool; stdcall;
var driverNameW : PWideChar;
    includesW   : PWideChar;
    excludesW   : PWideChar;
begin
  if (driverName <> nil) and (driverName[0] <> #0) then begin
    driverNameW := pointer(LocalAlloc(LPTR, lstrlenA(driverName) * 2 + 2));
    AnsiToWide2(driverName, driverNameW);
  end else
    driverNameW := nil;
  if (includeMask <> nil) and (includeMask[0] <> #0) then begin
    includesW := pointer(LocalAlloc(LPTR, lstrlenA(includeMask) * 2 + 2));
    AnsiToWide2(includeMask, includesW);
  end else
    includesW := nil;
  if (excludeMask <> nil) and (excludeMask[0] <> #0) then begin
    excludesW := pointer(LocalAlloc(LPTR, lstrlenA(excludeMask) * 2 + 2));
    AnsiToWide2(excludeMask, excludesW);
  end else
    excludesW := nil;
  result := UninjectLibraryX(GetCallingModule, libFileName, nil, true, 0, driverNameW, session, systemProcesses, includesW, excludesW, TPACardinal(excludePIDs), timeOut);
  if driverNameW <> nil then
    LocalFree(dword(driverNameW));
  if includesW <> nil then
    LocalFree(dword(includesW));
  if excludesW <> nil then
    LocalFree(dword(excludesW));
end;

function UninjectLibraryW(driverName      : PWideChar;
                          libFileName     : PWideChar;
                          session         : dword;
                          systemProcesses : bool;
                          includeMask     : PWideChar  = nil;
                          excludeMask     : PWideChar  = nil;
                          excludePIDs     : TPCardinal = nil;
                          timeOut         : dword      = 7000) : bool; stdcall;
begin
  result := UninjectLibraryX(GetCallingModule, nil, libFileName, true, 0, driverName, session, systemProcesses, includeMask, excludeMask, TPACardinal(excludePIDs), timeOut);
end;

function UninjectLibrary(driverName      : PAnsiChar;
                         libFileName     : PAnsiChar;
                         session         : dword;
                         systemProcesses : bool;
                         includeMask     : PAnsiChar  = nil;
                         excludeMask     : PAnsiChar  = nil;
                         excludePIDs     : TPCardinal = nil;
                         timeOut         : dword      = 7000) : bool; stdcall;
begin
  result := UninjectLibraryA(driverName, libFileName, session, systemProcesses, includeMask, excludeMask, excludePIDs, timeOut);
end;

// ***************************************************************

type
  TIpcAnswer = record
    map    : dword;
    buf    : pointer;
    len    : dword;
    event1 : dword;
    event2 : dword;
  end;

procedure CloseIpcAnswer(var answer: TIpcAnswer);
begin
  if answer.buf <> nil then
    UnmapViewOfFile(answer.buf);
  if answer.map <> 0 then begin
    CloseHandle(answer.map);
    answer.map := 0;
  end;
  if answer.event1 <> 0 then begin
    CloseHandle(answer.event1);
    answer.event1 := 0;
  end;
  if answer.event2 <> 0 then begin
    CloseHandle(answer.event2);
    answer.event2 := 0;
  end;
end;

function InitIpcAnswer(create: boolean; name: AnsiString; counter, pid: dword; var answer: TIpcAnswer; session: dword = 0) : boolean;
var s1, s2 : AnsiString;
    ch1    : AnsiChar;
begin
  if answer.len > 0 then begin
    s1 := name + DecryptStr(CAnswerBuf) + IntToStrEx(counter);
    if pid <> 0 then
      s1 := s1 + IntToHexEx(pid);
    if create then begin
      answer.map := InternalCreateFileMapping(PAnsiChar(s1 + DecryptStr(CMap)), answer.len, true);
      ch1 := '1'; answer.event1 := CreateGlobalEvent(PAnsiChar(s1 + DecryptStr(CEvent) + ch1), false, false);
      ch1 := '2'; answer.event2 := CreateGlobalEvent(PAnsiChar(s1 + DecryptStr(CEvent) + ch1), false, false);
    end else begin
      s2 := DecryptStr(CSession) + IntToStrEx(session) + '\';
      answer.map := OpenGlobalFileMapping(PAnsiChar(s1 + DecryptStr(CMap)), true);
      if answer.map = 0 then
        // if the IPC sender doesn't have the SeCreateGlobalPrivilege in w2k3
        // or in xp sp2 or higher then the objects were not created in global
        // namespace but in the namespace of the sender
        // so if opening of the file map didn't work, we try to open the
        // objects in the session of the sender
        answer.map := OpenGlobalFileMapping(PAnsiChar(s2 + s1 + DecryptStr(CMap)), true);
      ch1 := '1'; answer.event1 := OpenGlobalEvent(PAnsiChar(s1 + DecryptStr(CEvent) + ch1));
      ch1 := '2'; answer.event2 := OpenGlobalEvent(PAnsiChar(s1 + DecryptStr(CEvent) + ch1));
      if answer.event1 = 0 then begin
        ch1 := '1'; answer.event1 := OpenGlobalEvent(PAnsiChar(s2 + s1 + DecryptStr(CEvent) + ch1));
        ch1 := '2'; answer.event2 := OpenGlobalEvent(PAnsiChar(s2 + s1 + DecryptStr(CEvent) + ch1));
      end;
    end;
    if answer.map <> 0 then
      answer.buf := MapViewOfFile(answer.map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    result := (answer.event1 <> 0) and (answer.event2 <> 0) and (answer.buf <> nil);
    if result then begin
      if create then
        ZeroMemory(answer.buf, answer.len);
    end else
      CloseIpcAnswer(answer);
  end else begin
    answer.map := 0;
    answer.buf := nil;
    answer.event1 := 0;
    answer.event2 := 0;
    result := true;
  end;
end;

type
  TPipedIpcRec = record
    map       : dword;
    pid       : dword;
    rp, wp    : dword;
    callback  : TIpcCallback;
    mxThreads : dword;
    mxQueue   : dword;
    flags     : dword;
    th        : dword;
    name      : array [0..MAX_PATH] of AnsiChar;
    counter   : dword;
    msg       : record
                  buf    : PAnsiChar;
                  len    : dword;
                end;
    answer    : TIpcAnswer;
  end;

function OpenPipedIpcMap(name: AnsiString; var ir: TPipedIpcRec; pmutex: TPCardinal = nil; destroy: boolean = false) : boolean;
var buf   : ^TPipedIpcRec;
    mutex : dword;
    map   : dword;
begin
  result := false;
  mutex := CreateGlobalMutex(PAnsiChar(name + DecryptStr(CIpc) + DecryptStr(CMutex)));
  if mutex <> 0 then begin
    WaitForSingleObject(mutex, INFINITE);
    try
      map := OpenGlobalFileMapping(PAnsiChar(name + DecryptStr(CIpc) + DecryptStr(CMap)), true);
      if map <> 0 then begin
        buf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if buf <> nil then begin
          result := true;
          inc(buf^.counter);
          ir := buf^;
          if destroy then
            if buf^.pid = GetCurrentProcessID then
              CloseHandle(buf^.map)
            else
              result := false;
          UnmapViewOfFile(buf);
        end;
        CloseHandle(map);
      end;
    finally
      if (not result) or (pmutex = nil) then begin
        ReleaseMutex(mutex);
        CloseHandle(mutex);
      end else
        pmutex^ := mutex;
    end;
  end;
end;

type
  TSecurityQualityOfService = packed record
    size  : dword;
    dummy : array [0..1] of dword;
  end;
  TLpcMessageData = array [0..59] of dword;
  TLpcMessagePrivate = packed record
    pid                 : dword;
    msgLen              : dword;
    counter             : dword;
    session             : dword;
    answerLen           : dword;
    data                : TLpcMessageData;
  end;
  TPLpcMessage = ^TLpcMessage;
  TLpcMessage = packed record
    next                : TPLpcMessage;
    name                : AnsiString;
    callback            : TIpcCallback;
    answer              : TIpcAnswer;
    prv                 : dword;
    actualMessageLength : word;
    totalMessageLength  : word;
    messageType         : word;
    dataInfoOffset      : word;
    clientProcessId     : dword;
    clientThreadId      : dword;
    messageId           : dword;
    sharedSectionSize   : dword;
  end;
  TLpcSectionInfo = packed record
    size              : dword;
    sectionHandle     : dword;
    param1            : dword;
    sectionSize       : dword;
    clientBaseAddress : pointer;
    serverBaseAddress : pointer;
  end;
  TLpcSectionMapInfo = packed record
    size              : dword;
    sectionSize       : dword;
    serverBaseAddress : pointer;
  end;
  TLpcSectionMapInfo64 = packed record
    size              : dword;
    dummy1            : dword;
    sectionSize       : dword;
    dummy2            : dword;
    serverBaseAddress : pointer;
    dummy3            : pointer;
  end;

const LPC_CONNECTION_REQUEST = $a;

var
  NtCreatePort           : function (var portHandle: dword; var objAttr: TObjectAttributes; maxConnectInfoLen, maxDataLen, unknown: dword) : dword; stdcall;
  NtConnectPort          : function (var portHandle: dword; var portName: TUnicodeStr; var qs: TSecurityQualityOfService; sectionInfo, mapInfo, unknown2, connectInfo: pointer; var connectInfoLen: dword) : dword; stdcall;
  NtReplyWaitReceivePort : function (portHandle: dword; unknown: dword; messageOut, messageIn: pointer) : dword; stdcall;
  NtAcceptConnectPort    : function (var portHandle: dword; unknown1: dword; msg: pointer; acceptIt, unknown2: dword; mapInfo: pointer) : dword; stdcall;
  NtCompleteConnectPort  : function (portHandle: dword) : dword; stdcall;

type
  TPLpcWorkerThread = ^TLpcWorkerThread;
  TLpcWorkerThread = record
    handle          : dword;
    parentSemaphore : dword;
    event           : dword;
    pm              : TPLpcMessage;
    lastActive      : dword;
  end;

  TPLpcQueue = ^TLpcQueue;
  TLpcQueue = record
    name           : AnsiString;
    callback       : TIpcCallback;
    maxThreadCount : dword;
    madQueueLen    : dword;
    port           : dword;
    counter        : dword;
    pm             : TPLpcMessage;
    section        : TRtlCriticalSection;
    semaphore      : dword;
    shutdown       : boolean;
    portThread     : dword;
    dispatchThread : dword;
    workerThreads  : array of TPLpcWorkerThread;
  end;

var
  LpcSection    : TRTLCriticalSection;
  LpcReady      : boolean = false;
  LpcList       : array of TPLpcQueue;
  LpcCounterBuf : ^integer = nil;

procedure InitLpcName(ipc: AnsiString; var ws: AnsiString; var uniStr: TUnicodeStr);
begin
  ws := AnsiToWideEx(DecryptStr(CRpcControlIpc) + ipc);
  uniStr.maxLen := length(ws);
  uniStr.len := uniStr.maxLen - 2;
  uniStr.str := pointer(ws);
end;

procedure InitLpcFuncs;
begin
  if @NtCreatePort = nil then begin
    NtCreatePort           := NtProc(CNtCreatePort);
    NtConnectPort          := NtProc(CNtConnectPort);
    NtReplyWaitReceivePort := NtProc(CNtReplyWaitReceivePort);
    NtAcceptConnectPort    := NtProc(CNtAcceptConnectPort);
    NtCompleteConnectPort  := NtProc(CNtCompleteConnectPort);
  end;
end;

function CreateIpcQueueEx(ipc: PAnsiChar; callback: TIpcCallback; maxThreadCount, maxQueueLen: dword) : bool; stdcall;

  function CreateLpcQueue : boolean;
  // new ipc queue logic based on NT native LPC (local procedure call) APIs
  // implemented mainly for Vista, cause Vista doesn't like the old approach
  // probably in a later version I'll also use the new approach for older OSs
  // but the new approach shall prove itself first

    function LpcWorkerThread(var wt: TLpcWorkerThread) : integer; stdcall;
    // the worker thread handles one message after the other, if it's told to
    var pm2 : pointer;
        prv : ^TLpcMessagePrivate;
    begin
      result := 0;
      with wt do
        while true do begin
          if (WaitForSingleObject(event, INFINITE) <> WAIT_OBJECT_0) or (pm = nil) then
            break;
          dword(prv) := dword(pm) + pm.prv;
          pm.callback(PAnsiChar(pm.name), @prv.data, prv.msgLen, pm.answer.buf, pm.answer.len);
          if pm.answer.len <> 0 then begin
            SetEvent(pm.answer.event2);
            CloseIpcAnswer(pm.answer);
          end;
          lastActive := GetTickCount;
          pm^.name := '';
          pm2 := pm;
          pm := nil;
          LocalFree(dword(pm2));
          ReleaseSemaphore(parentSemaphore, 1, nil);
        end;
    end;

    function LpcDispatchThread(var iq: TLpcQueue) : integer; stdcall;
    // this thread takes all ipc messages and feeds them to the worker threads
    // worker threads are dynamically created and deleted, if it makes sense
    var pm     : TPLpcMessage;
        tid    : dword;
        i1, i2 : integer;
        p1     : pointer;
    begin
      while true do begin
        WaitForSingleObject(iq.semaphore, INFINITE);
        if iq.shutdown then
          break;
        EnterCriticalSection(iq.section);
        try
          pm := iq.pm;
          if pm <> nil then
            iq.pm := iq.pm^.next;
        finally LeaveCriticalSection(iq.section) end;
        while pm <> nil do begin
          // loop until we found someone who is willing to handle this message
          for i1 := 0 to high(iq.workerThreads) do begin
            if iq.workerThreads[i1].pm = nil then begin
              iq.workerThreads[i1].pm := pm;
              SetEvent(iq.workerThreads[i1].event);
              pm := nil;
              break;
            end;
          end;
          if pm <> nil then begin
            // we still have a message to handle, but all worker threads are busy
            if iq.maxThreadCount > dword(Length(iq.workerThreads)) then begin
              // let's create a new worker thread
              i1 := Length(iq.workerThreads);
              SetLength(iq.workerThreads, i1 + 1);
              iq.workerThreads[i1] := pointer(LocalAlloc(LPTR, sizeOf(TLpcWorkerThread)));
              iq.workerThreads[i1].parentSemaphore := iq.semaphore;
              iq.workerThreads[i1].event           := CreateEvent(nil, false, true, nil);
              iq.workerThreads[i1].pm              := pm;
              iq.workerThreads[i1].handle          := CreateThread(nil, 0, @LpcWorkerThread, iq.workerThreads[i1], 0, tid);
              if iq.workerThreads[i1].handle <> 0 then begin
                SetThreadPriority(iq.workerThreads[i1].handle, THREAD_PRIORITY_NORMAL);
                pm := nil;
              end else begin
                CloseHandle(iq.workerThreads[i1].event);
                LocalFree(dword(iq.workerThreads[i1]));
                SetLength(iq.workerThreads, i1 - 1);
              end;
            end;
            if pm <> nil then begin
              // we still have a message to handle, so we wait
              // until a worker thread is ready to handle this message
              WaitForSingleObject(iq.semaphore, INFINITE);
              if iq.shutdown then
                break;
            end;
          end;
        end;
        if iq.shutdown then
          break;
        i2 := Length(iq.workerThreads);
        for i1 := high(iq.workerThreads) downto 0 do
          if ((i1 > 0) or (i2 > 1)) and (iq.workerThreads[i1].pm = nil) and (GetTickCount - iq.workerThreads[i1].lastActive > 100) then begin
            // this thread was idle for more than 100ms, so we close it down
            dec(i2);
            SetEvent(iq.workerThreads[i1].event);
            WaitForSingleObject(iq.workerThreads[i1].handle, 100);
            CloseHandle(iq.workerThreads[i1].event);
            CloseHandle(iq.workerThreads[i1].handle);
            p1 := iq.workerThreads[i1];
            iq.workerThreads[i1] := nil;
            LocalFree(dword(p1));
            p1 := iq.workerThreads[i2];
            iq.workerThreads[i2] := nil;
            iq.workerThreads[i1] := p1;
          end;
        if i2 <> Length(iq.workerThreads) then
          SetLength(iq.workerThreads, i2);
      end;
      result := 0;
    end;

    function LpcPortThread(var iq: TLpcQueue) : integer; stdcall;
    // this thread receives ipc messages from the LPC port
    // all incoming messages are stored in a pointer list
    var port     : dword;
        pm1, pm2 : TPLpcMessage;
        ppm      : ^TPLpcMessage;
        mi       : ^TLpcSectionMapInfo;
        prv      : ^TLpcMessagePrivate;
        prevPort : dword;
        b1       : boolean;
    begin
      result := 0;
      prevPort := 0;
      while true do begin
        pm1 := pointer(LocalAlloc(LPTR, 512));
        b1 := (NtReplyWaitReceivePort(iq.port, 0, nil, @pm1.actualMessageLength) = 0) and
              (pm1.messageType = LPC_CONNECTION_REQUEST);
        if prevPort <> 0 then begin
          CloseHandle(prevPort);
          prevPort := 0;
        end;
        if b1 then begin
          pm1.prv := dword(@pm1.actualMessageLength) - dword(pm1) + pm1.totalMessageLength - sizeOf(TLpcMessagePrivate);
          dword(prv) := dword(pm1) + pm1.prv;
          if iq.shutdown then begin
            NtAcceptConnectPort(port, 0, @pm1.actualMessageLength, 0, 0, nil);
            LocalFree(dword(pm1));
            break;
          end;
          prv.pid := GetCurrentProcessId;
          port := 0;
          mi := pointer(LocalAlloc(LPTR, sizeOf(TLpcSectionMapInfo64)));
          if Am64OS then
               mi.size := sizeOf(TLpcSectionMapInfo64)
          else mi.size := sizeOf(TLpcSectionMapInfo  );
          if NtAcceptConnectPort(port, 0, @pm1.actualMessageLength, 1, 0, mi) = 0 then begin
            if Am64OS then begin
              mi.sectionSize       := TLpcSectionMapInfo64(pointer(mi)^).sectionSize;
              mi.serverBaseAddress := TLpcSectionMapInfo64(pointer(mi)^).serverBaseAddress;
            end;
            if mi.sectionSize > 0 then begin
              pm2 := pointer(LocalAlloc(LPTR, dword(@prv.data) - dword(pm1) + prv.msgLen));
              Move(pm1^, pm2^, dword(@prv.data) - dword(pm1));
              dword(prv) := dword(pm2) + pm2.prv;   // pm2.prv -> read access nil?
              Move(mi.serverBaseAddress^, prv.data, prv.msgLen);
              LocalFree(dword(pm1));
              pm1 := pm2;
            end;
            NtCompleteConnectPort(port);
            prevPort := port;
            pm1.answer.len := prv.answerLen;
            if InitIpcAnswer(false, iq.name, prv.counter, pm1.clientProcessId, pm1.answer, prv.session) then begin
              if pm1.answer.len <> 0 then
                SetEvent(pm1.answer.event1);
              pm1.name := iq.name;
              pm1.callback := iq.callback;
              EnterCriticalSection(iq.section);
              try
                ppm := @iq.pm;
                while ppm^ <> nil do
                  ppm := @ppm^^.next;
                ppm^ := pm1;
              finally LeaveCriticalSection(iq.section) end;
              ReleaseSemaphore(iq.semaphore, 1, nil);
            end else
              LocalFree(dword(pm1));
          end else begin
            prv.pid := 0;
            NtAcceptConnectPort(port, 0, @pm1.actualMessageLength, 0, 0, nil);
            LocalFree(dword(pm1));
          end;
          LocalFree(dword(mi));
        end else
          LocalFree(dword(pm1));
      end;
      if prevPort <> 0 then 
        CloseHandle(prevPort);
    end;

  var objAttr : ^TObjectAttributes;
      uniStr  : TUnicodeStr;
      sd      : TSecurityDescriptor;
      ws      : AnsiString;
      tid     : dword;
      i1      : integer;
      port    : dword;
      ph      : dword;
  begin
    if DuplicateHandle(GetCurrentProcess, GetCurrentProcess, GetCurrentProcess, @ph, 0, false, DUPLICATE_SAME_ACCESS) then begin
      AddAccessForEveryone(ph, SYNCHRONIZE);
      CloseHandle(ph);
    end;
    if not LpcReady then begin
      InitializeCriticalSection(LpcSection);
      LpcReady := true;
    end;
    result := true;
    EnterCriticalSection(LpcSection);
    try
      for i1 := 0 to high(LpcList) do
        if IsTextEqual(LpcList[i1].name, ipc) then begin
          result := false;
          break;
        end;
    finally LeaveCriticalSection(LpcSection) end;
    if result then begin
      InitLpcFuncs;
      InitLpcName(ipc, ws, uniStr);
      InitializeSecurityDescriptor(@sd, SECURITY_DESCRIPTOR_REVISION);
      SetSecurityDescriptorDacl(@sd, true, nil, false);
      objAttr := pointer(LocalAlloc(LPTR, sizeOf(TObjectAttributes)));
      objAttr.length := sizeOf(TObjectAttributes);
      objAttr.name   := @uniStr;
      objAttr.sd     := @sd;
      port := dword(-1);
      result := NtCreatePort(port, objAttr^, 260, 40 + 260, 0) = 0;
      LocalFree(dword(objAttr));
      if result then begin
        if maxThreadCount > 64 then
          maxThreadCount := 64;
        EnterCriticalSection(LpcSection);
        try
          i1 := Length(LpcList);
          SetLength(LpcList, i1 + 1);
          LpcList[i1] := pointer(LocalAlloc(LPTR, sizeOf(TLpcQueue)));
          LpcList[i1].name           := ipc;
          LpcList[i1].callback       := callback;
          LpcList[i1].maxThreadCount := maxThreadCount;
          LpcList[i1].madQueueLen    := maxQueueLen;
          LpcList[i1].port           := port;
          LpcList[i1].semaphore      := CreateSemaphore(nil, 0, maxInt, nil);
          LpcList[i1].portThread     := CreateThread(nil, 0, @LpcPortThread,     LpcList[i1], 0, tid);
          LpcList[i1].dispatchThread := CreateThread(nil, 0, @LpcDispatchThread, LpcList[i1], 0, tid);
          SetThreadPriority(LpcList[i1].    portThread, 7);
          SetThreadPriority(LpcList[i1].dispatchThread, THREAD_PRIORITY_ABOVE_NORMAL);
          InitializeCriticalSection(LpcList[i1].section);
        finally LeaveCriticalSection(LpcSection) end;
      end;
    end;
  end;

  function CreatePipedIpcQueue : boolean;
  // this is the old IPC solution based on (unnamed) pipes

    procedure HandlePipedIpcMessage(var ir: TPipedIpcRec);
    begin
      ir.callback(ir.name, ir.msg.buf, ir.msg.len, ir.answer.buf, ir.answer.len);
      if ir.answer.len <> 0 then begin
        SetEvent(ir.answer.event2);
        CloseIpcAnswer(ir.answer);
      end;
      LocalFree(dword(ir.msg.buf));
    end;

    function PipedIpcThread2(var ir: TPipedIpcRec) : integer; stdcall;
    begin
      result := 0;
      HandlePipedIpcMessage(ir);
      LocalFree(dword(@ir));
    end;

    function PipedIpcThread1(var ir: TPipedIpcRec) : integer; stdcall;
    var c1      : dword;
        pir     : ^TPipedIpcRec;
        tid     : dword;
        session : dword;
    begin
      result := 0;
      while ReadFile(ir.rp, ir.msg.len, 4, c1, nil) and (c1 = 4) do begin
        ir.msg.buf := pointer(LocalAlloc(LPTR, ir.msg.len));
        if ir.msg.buf <> nil then begin
          if ReadFile(ir.rp, ir.counter,    4,          c1, nil) and (c1 = 4) and
             ReadFile(ir.rp, session,       4,          c1, nil) and (c1 = 4) and
             ReadFile(ir.rp, ir.answer.len, 4,          c1, nil) and (c1 = 4) and
             ReadFile(ir.rp, ir.msg.buf^,   ir.msg.len, c1, nil) and (c1 = ir.msg.len) and
             InitIpcAnswer(false, ir.name, ir.counter, 0, ir.answer, session) then begin
            if ir.answer.len <> 0 then
              SetEvent(ir.answer.event1);
            if ir.mxThreads > 1 then begin
              pir := pointer(LocalAlloc(LPTR, sizeOf(ir)));
              pir^ := ir;
              c1 := CreateThread(nil, 0, @PipedIpcThread2, pir, 0, tid);
              if c1 <> 0 then begin
                SetThreadPriority(c1, THREAD_PRIORITY_NORMAL);
                CloseHandle(c1);
              end else begin
                LocalFree(dword(pir));
                LocalFree(dword(ir.msg.buf));
                CloseIpcAnswer(ir.answer);
              end;
            end else
              HandlePipedIpcMessage(ir);
          end else
            LocalFree(dword(ir.msg.buf));
        end;
        if GetVersion and $80000000 <> 0 then
          // this fixes a stability problem with IPC in win9x
          // when lots of IPC messages are sent in short time
          // please don't ask me why this helps...   :-O
          Sleep(0);
      end;
      LocalFree(dword(@ir));
    end;

  var mutex      : dword;
      map        : dword;
      buf1, buf2 : ^TPipedIpcRec;
      s1         : AnsiString;
      sa         : TSecurityAttributes;
      sd         : TSecurityDescriptor;
      tid        : dword;
      ph         : dword;
  begin
    result := false;
    if DuplicateHandle(GetCurrentProcess, GetCurrentProcess, GetCurrentProcess, @ph, 0, false, DUPLICATE_SAME_ACCESS) then begin
      AddAccessForEveryone(ph, PROCESS_DUP_HANDLE or SYNCHRONIZE);
      CloseHandle(ph);
    end;
    s1 := ipc;
    if Length(s1) > MAX_PATH then
      SetLength(s1, MAX_PATH);
    mutex := CreateGlobalMutex(PAnsiChar(s1 + DecryptStr(CIpc) + DecryptStr(CMutex)));
    if mutex <> 0 then begin
      WaitForSingleObject(mutex, INFINITE);
      try
        map := CreateGlobalFileMapping(PAnsiChar(s1 + DecryptStr(CIpc) + DecryptStr(CMap)), sizeOf(TPipedIpcRec));
        if map <> 0 then
          if GetLastError <> ERROR_ALREADY_EXISTS then begin
            buf1 := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
            if buf1 <> nil then begin
              result := true;
              ZeroMemory(buf1, sizeOf(TPipedIpcRec));
              if GetVersion and $80000000 = 0 then begin
                InitSecAttr(sa, sd, false);
                CreatePipe(buf1^.rp, buf1^.wp, @sa, 0);
              end else
                CreatePipe(buf1^.rp, buf1^.wp, nil, 0);
              buf1^.map       := map;
              buf1^.pid       := GetCurrentProcessID;
              buf1^.callback  := callback;
              buf1^.mxThreads := maxThreadCount;
              buf1^.mxQueue   := maxQueueLen;
              Move(PAnsiChar(s1)^, buf1^.name, Length(s1) + 1);
              buf2 := pointer(LocalAlloc(LPTR, sizeOf(buf2^)));
              buf2^ := buf1^;
              buf1^.th        := CreateThread(nil, 0, @PipedIpcThread1, buf2, 0, tid);
              SetThreadPriority(buf1^.th, 7);
              buf1^.counter   := 0;
              UnmapViewOfFile(buf1);
            end else
              CloseHandle(map);
          end else
            CloseHandle(map);
      finally
        ReleaseMutex(mutex);
        CloseHandle(mutex);
      end;
    end;
  end;

begin
  result := false;
  EnableAllPrivileges;
  if maxThreadCount = 0 then
    maxThreadCount := 16;
  if @callback <> nil then
    if IsVista or Am64OS or UseNewIpcLogic then
      result := CreateLpcQueue
    else
      result := CreatePipedIpcQueue;
end;

function CreateIpcQueue(ipc: PAnsiChar; callback: TIpcCallback) : bool; stdcall;
begin
  result := CreateIpcQueueEx(ipc, callback);
end;

function SendIpcMessage(ipc: PAnsiChar; messageBuf: pointer; messageLen: dword;
                        answerBuf: pointer = nil; answerLen: dword = 0;
                        answerTimeOut: dword = INFINITE; handleMessages: bool = true) : bool; stdcall;

  function WaitFor(h1, h2, timeOut: dword) : boolean;
  var handles : array [0..1] of dword;
      msg     : TMsg;
  begin
    result := false;
    handles[0] := h1;
    handles[1] := h2;
    if handleMessages then begin
      while true do
        case MsgWaitForMultipleObjects(2, handles, false, timeOut, QS_ALLINPUT) of
          WAIT_OBJECT_0     : begin
                                result := true;
                                break;
                              end;
          WAIT_OBJECT_0 + 2 : while PeekMessage(msg, 0, 0, 0, PM_REMOVE) do begin
                                TranslateMessage(msg);
                                DispatchMessage(msg);
                              end;
          else                break;
        end;
    end else
      result := WaitForMultipleObjects(2, @handles, false, timeOut) = WAIT_OBJECT_0;
  end;

  function GetLpcCounter : dword;

    function InterlockedIncrementEx(var value: integer) : integer;
    asm
      mov edx, value
      mov eax, [edx]
     @loop:
      mov ecx, eax
      inc ecx
      lock cmpxchg [edx], ecx
      jnz @loop
    end;

  var s1  : AnsiString;
      map : dword;
  begin
    if LpcCounterBuf = nil then begin
      s1 := DecryptStr(CIpc) + DecryptStr(CCounter) + IntToHexEx(GetCurrentProcessId);
      map := CreateFileMappingA(dword(-1), nil, PAGE_READWRITE, 0, 4, PAnsiChar(s1));
      LpcCounterBuf := MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
      LpcCounterBuf^ := 0;
      CloseHandle(map);
    end;
    if LpcCounterBuf <> nil then
      result := dword(InterlockedIncrementEx(LpcCounterBuf^))
    else
      result := 0;
  end;

var {$ifdef CheckRecursion}
      recTestBuf : array [0..3] of dword;
    {$endif}
    port       : dword;
    uniStr     : TUnicodeStr;
    qs         : TSecurityQualityOfService;
    buf        : TLpcMessagePrivate;
    bufLen     : dword;
    ws         : AnsiString;
    c1         : dword;
    si         : TLpcSectionInfo;
    mi         : TLpcSectionMapInfo;
    psi        : ^TLpcSectionInfo;
    pmi        : ^TLpcSectionMapInfo;
    ph         : dword;
    ir         : TPipedIpcRec;
    mutex      : dword;
    error      : dword;
    b1         : boolean;
    session    : dword;
begin
  error := GetLastError;
  result := false;
  {$ifdef CheckRecursion}
    if NotRecursedYet(@SendIpcMessage, @recTestBuf) then begin
  {$endif}
    ph := 0;
    if IsVista or Am64OS or UseNewIpcLogic then begin
      ZeroMemory(@buf, sizeOf(buf));
      buf.counter := GetLpcCounter;
      ir.answer.len := answerLen;
      b1 := InitIpcAnswer(true, ipc, buf.counter, GetCurrentProcessId, ir.answer);
      if b1 then begin
        InitLpcFuncs;
        InitLpcName(ipc, ws, uniStr);
        ZeroMemory(@si, sizeOf(si));
        ZeroMemory(@qs, sizeOf(qs));
        port := 0;
        buf.msgLen    := messageLen;
        buf.session   := GetCurrentSessionId;
        buf.answerLen := answerLen;
        if messageLen > sizeOf(buf.data) then begin
//          bufLen := sizeOf(buf) - sizeOf(buf.data);
          ZeroMemory(@mi, sizeOf(mi));
          si.size              := sizeOf(si);
          si.sectionHandle     := CreateFileMapping(dword(-1), nil, PAGE_READWRITE, 0, messageLen, nil);
          si.sectionSize       := messageLen;
          si.clientBaseAddress := MapViewOfFile(si.sectionHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
          if si.clientBaseAddress <> nil then begin
            Move(messageBuf^, si.clientBaseAddress^, messageLen);
            UnmapViewOfFile(si.clientBaseAddress);
            si.clientBaseAddress := nil;
          end;
          mi.size        := sizeOf(mi);
          mi.sectionSize := messageLen;
          psi := @si;
          pmi := @mi;
        end else begin
//          bufLen := sizeOf(buf) - sizeOf(buf.data) + messageLen;
          Move(messageBuf^, buf.data, messageLen);
          psi := nil;
          pmi := nil;
        end;
        bufLen := sizeOf(buf);
        c1 := NtConnectPort(port, uniStr, qs, psi, pmi, nil, @buf, bufLen);
        if c1 = 0 then
          CloseHandle(port);
        if si.sectionHandle <> 0 then
          CloseHandle(si.sectionHandle);
        result := buf.pid <> 0;
        if result and (answerLen <> 0) then
          ph := OpenProcess(SYNCHRONIZE, false, buf.pid);
      end;
    end else begin
      b1 := OpenPipedIpcMap(ipc, ir, @mutex);
      if b1 then begin
        ir.answer.len := answerLen;
        b1 := InitIpcAnswer(true, ipc, ir.counter, 0, ir.answer);
        if b1 then begin
          ph := OpenProcess(PROCESS_DUP_HANDLE or SYNCHRONIZE, false, ir.pid);
          if (ph <> 0) and
             DuplicateHandle(ph, ir.wp, GetCurrentProcess, @ir.wp, 0, false, DUPLICATE_SAME_ACCESS) then begin
            session := GetCurrentSessionId;
            result := WriteFile(ir.wp, messageLen,  4,          c1, nil) and (c1 = 4) and
                      WriteFile(ir.wp, ir.counter,  4,          c1, nil) and (c1 = 4) and
                      WriteFile(ir.wp, session,     4,          c1, nil) and (c1 = 4) and
                      WriteFile(ir.wp, answerLen,   4,          c1, nil) and (c1 = 4) and
                      WriteFile(ir.wp, messageBuf^, messageLen, c1, nil) and (c1 = messageLen);
            CloseHandle(ir.wp);
          end;
        end;
        ReleaseMutex(mutex);
        CloseHandle(mutex);
      end;
    end;
    if ph <> 0 then begin
      if result and (answerLen <> 0) then
        if WaitFor(ir.answer.event1, ph, InternalIpcTimeout) then begin
          if WaitFor(ir.answer.event2, ph, answerTimeOut) then begin
            Move(ir.answer.buf^, answerBuf^, answerLen);
          end else
            result := false;
        end else
          result := false;
      CloseHandle(ph);
    end;
    if b1 then
      CloseIpcAnswer(ir.answer);
    {$ifdef CheckRecursion}
      recTestBuf[3] := 0;
    end;
  {$endif}
  SetLastError(error);
end;

function DestroyIpcQueue(ipc: PAnsiChar) : bool; stdcall;

  function DestroyLpcQueue : boolean;
  var port   : dword;
      uniStr : TUnicodeStr;
      qs     : TSecurityQualityOfService;
      bufLen : dword;
      ws     : AnsiString;
      iq     : TPLpcQueue;
      i1     : integer;
  begin
    result := false;
    if LpcReady then begin
      EnterCriticalSection(LpcSection);
      try
        iq := nil;
        for i1 := 0 to high(LpcList) do
          if IsTextEqual(LpcList[i1].name, ipc) then begin
            iq := LpcList[i1];
            LpcList[i1] := LpcList[high(LpcList)];
            SetLength(LpcList, high(LpcList));
            break;
          end;
      finally LeaveCriticalSection(LpcSection) end;
      if iq <> nil then begin
        iq.shutdown := true;
        ReleaseSemaphore(iq.semaphore, 1, nil);
        InitLpcName(iq.name, ws, uniStr);
        ZeroMemory(@qs, sizeOf(qs));
        port := 0;
        bufLen := 0;
        NtConnectPort(port, uniStr, qs, nil, nil, nil, nil, bufLen);
        WaitForSingleObject(iq.portThread, 100);
        TerminateThread(iq.portThread, 0);
        CloseHandle(iq.portThread);
        WaitForSingleObject(iq.dispatchThread, 100);
        TerminateThread(iq.dispatchThread, 0);
        CloseHandle(iq.dispatchThread);
        for i1 := 0 to high(iq.workerThreads) do
          if iq.workerThreads[i1] <> nil then begin
            SetEvent(iq.workerThreads[i1].event);
            WaitForSingleObject(iq.workerThreads[i1].handle, 100);
            TerminateThread(iq.workerThreads[i1].handle, 0);
            CloseHandle(iq.workerThreads[i1].handle);
            CloseHandle(iq.workerThreads[i1].event);
            if iq.workerThreads[i1].pm <> nil then begin
              iq.workerThreads[i1].pm.name := '';
              LocalFree(dword(iq.workerThreads[i1].pm));
            end;
            LocalFree(dword(iq.workerThreads[i1]));
          end;
        iq.workerThreads := nil;
        CloseHandle(iq.port);
        CloseHandle(iq.semaphore);
        DeleteCriticalSection(iq.section);
        iq.name := '';
        LocalFree(dword(iq));
        result := true;
      end;
    end;
  end;

  function DestroyPipedIpcQueue : boolean;
  var ir : TPipedIpcRec;
  begin
    result := OpenPipedIpcMap(ipc, ir, nil, true);
    if result then begin
      CloseHandle(ir.wp);
      WaitForSingleObject(ir.th, 100);
      TerminateThread(ir.th, 0);
      CloseHandle(ir.rp);
      CloseHandle(ir.th);
    end;
  end;

begin
  if IsVista or Am64OS or UseNewIpcLogic then
    result := DestroyLpcQueue
  else
    result := DestroyPipedIpcQueue;
end;

// ***************************************************************

function AddAccessForEveryone(processOrService, access: dword) : bool; stdcall;
const CEveryoneSia : TSidIdentifierAuthority = (value: (0, 0, 0, 0, 0, 1));
type
  TTrustee = packed record
    multiple : pointer;
    multop   : dword;
    form     : dword;
    type_    : dword;
    sid      : PSid;
  end;
  TExplicitAccess = packed record
    access  : dword;
    mode    : dword;
    inherit : dword;
    trustee : TTrustee;
  end;
var sd               : PSecurityDescriptor;
    ea               : TExplicitAccess;
    oldDacl, newDacl : PAcl;
    sid              : PSid;
    dll              : dword;
    gsi              : function (handle, objectType, securityInfo: dword; owner, group, dacl, sacl, sd: pointer) : dword; stdcall;
    ssi              : function (handle, objectType, securityInfo: dword; owner, group: PSid; dacl, sacl: PAcl) : dword; stdcall;
    seia             : function (count: dword; var ees: TExplicitAccess; oldAcl: PAcl; var newAcl: PAcl) : dword; stdcall;
    typ              : dword;
begin
  result := false;
  dll  := LoadLibraryA(PAnsiChar(DecryptStr(CAdvApi32)));
  gsi  := GetProcAddress(dll, PAnsiChar(DecryptStr(CGetSecurityInfo)));
  ssi  := GetProcAddress(dll, PAnsiChar(DecryptStr(CSetSecurityInfo)));
  seia := GetProcAddress(dll, PAnsiChar(DecryptStr(CSetEntriesInAclA)));
  if (@gsi <> nil) and (@ssi <> nil) and (@seia <> nil) then begin
    if      gsi(processOrService, 6, DACL_SECURITY_INFORMATION, nil, nil, @oldDacl, nil, @sd) = 0 then typ := 6
    else if gsi(processOrService, 2, DACL_SECURITY_INFORMATION, nil, nil, @oldDacl, nil, @sd) = 0 then typ := 2
    else                                                                                               typ := 0;
    if typ <> 0 then begin
      if AllocateAndInitializeSid(CEveryoneSia, 1, 0, 0, 0, 0, 0, 0, 0, 0, sid) then begin
        ZeroMemory(@ea, sizeOf(ea));
        ea.access        := access;
        ea.mode          := 1; // GRANT_ACCESS
        ea.trustee.form  := 0; // TRUSTEE_IS_SID
        ea.trustee.type_ := 1; // TRUSTEE_IS_USER
        ea.trustee.sid   := sid;
        if seia(1, ea, oldDacl, newDacl) = 0 then begin
          result := ssi(processOrService, typ, DACL_SECURITY_INFORMATION, nil, nil, newDacl, nil) = 0;
          LocalFree(dword(newDacl));
        end;
        FreeSid(sid);
      end;
      LocalFree(dword(sd));
    end;
  end;
  FreeLibrary(dll);
end;

// ***************************************************************

(*function CheckSLSvc(process: dword) : boolean;
const MEMBER_ACCESS = 1;
const idAuthorityNt : TSidIdentifierAuthority = (value: (0, 0, 0, 0, 0, 5));
var prcToken   : dword;
    impToken   : dword;
    sid        : PSid;
    sd         : TSecurityDescriptor;
    dacl       : PAcl;
    daclSize   : dword;
    genMap     : TGenericMapping;
    privBuf    : array [0..sizeOf(TPrivilegeSet) + 3 * sizeof(TLuidAndAttributes) - 1] of byte;
    priv       : PPrivilegeSet;
    privLen    : dword;
    accGranted : dword;
    accStatus  : bool;
begin
  result := false;
  if OpenProcessToken(process, TOKEN_QUERY or TOKEN_DUPLICATE, prcToken) then begin
    if DuplicateToken(prcToken, SecurityImpersonation, @impToken) then begin
      if AllocateAndInitializeSid(idAuthorityNt, 6, 80, 2119565420, 4155874467, 2934723793, 509086461, 374458824, 0, 0, sid) then begin
        daclSize := sizeOf(TAcl) + 12 + 3 * GetLengthSid(sid);
        dacl := pointer(LocalAlloc(LMEM_ZEROINIT, daclSize));
        if dacl <> nil then begin
          genMap.GenericRead    := STANDARD_RIGHTS_READ or MEMBER_ACCESS;
          genMap.GenericWrite   := STANDARD_RIGHTS_EXECUTE;
          genMap.GenericExecute := STANDARD_RIGHTS_WRITE;
          genMap.GenericAll     := STANDARD_RIGHTS_ALL or MEMBER_ACCESS;
          priv       := @privBuf;
          privLen    := sizeOf(privBuf);
          accGranted := 0;
          accStatus  := false;
          result := InitializeSecurityDescriptor(@sd, SECURITY_DESCRIPTOR_REVISION) and
                    SetSecurityDescriptorOwner(@sd, sid, false) and
                    SetSecurityDescriptorGroup(@sd, sid, false) and
                    InitializeAcl(dacl^, daclSize, 2) and
                    AddAccessAllowedAce(dacl^, 2, MEMBER_ACCESS, sid) and
                    SetSecurityDescriptorDacl(@sd, true, dacl, false) and
                    AccessCheck(@sd, impToken, MEMBER_ACCESS, genMap, priv^, privLen, accGranted, accStatus) and
                    accStatus and (accGranted = MEMBER_ACCESS);
          LocalFree(dword(dacl));
        end;
        FreeSid(sid);
      end;
      CloseHandle(impToken);
    end;
    CloseHandle(prcToken);
  end;
end;*)

// ***************************************************************

var PrivilegesEnabled : boolean = false;
procedure EnableAllPrivileges;
type TTokenPrivileges = record
       PrivilegeCount : dword;
       Privileges     : array [0..maxInt shr 4 - 1] of TLUIDAndAttributes;
     end;
var c1, c2 : dword;
    i1     : integer;
    ptp    : ^TTokenPrivileges;
    backup, restore, owner : int64;
begin
  if PrivilegesEnabled then
    exit;
  if OpenProcessToken(windows.GetCurrentProcess, TOKEN_ADJUST_PRIVILEGES or TOKEN_QUERY, c1) then
    try
      c2 := 0;
      GetTokenInformation(c1, TokenPrivileges, nil, 0, c2);
      if c2 <> 0 then begin
        ptp := pointer(LocalAlloc(LPTR, c2 * 2));
        if GetTokenInformation(c1, TokenPrivileges, ptp, c2 * 2, c2) then begin
          // enabling backup/restore privileges breaks Explorer's Samba support
          if not LookupPrivilegeValueA(nil, PAnsiChar(DecryptStr(CSeBackupPrivilege       )), backup ) then backup  := 0;
          if not LookupPrivilegeValueA(nil, PAnsiChar(DecryptStr(CSeRestorePrivilege      )), restore) then restore := 0;
          if not LookupPrivilegeValueA(nil, PAnsiChar(DecryptStr(CSeTakeOwnershipPrivilege)), owner  ) then owner   := 0;
          for i1 := 0 to integer(ptp^.PrivilegeCount) - 1 do
            if (ptp^.Privileges[i1].Luid <> backup ) and
               (ptp^.Privileges[i1].Luid <> restore) and
               (ptp^.Privileges[i1].Luid <> owner  ) then
              ptp^.Privileges[i1].Attributes := ptp^.Privileges[i1].Attributes or SE_PRIVILEGE_ENABLED;
          AdjustTokenPrivileges(c1, false, PTokenPrivileges(ptp)^, c2, PTokenPrivileges(nil)^, cardinal(pointer(nil)^));
        end;
        LocalFree(dword(ptp));
      end;
    finally CloseHandle(c1) end;
  PrivilegesEnabled := true;
end;

procedure InitializeMadCHook;
const PROCESSOR_ARCHITECTURE_AMD64 = 9;
var gnsi : procedure (var si: TSystemInfo) stdcall;
    si   : TSystemInfo;
begin
  IsMultiThread := true;
  {$ifdef log}
    log('initialization begin');
  {$endif}
  InitTryExceptFinally;
  gnsi := KernelProc(CGetNativeSystemInfo);
  if @gnsi <> nil then begin
    ZeroMemory(@si, sizeOf(si));
    gnsi(si);
    Am64OS := si.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
  end;
  madTools.NeedModuleFileMapEx := madCodeHook.CollectCache_NeedModuleFileMap;
  IsWine := NtProc(CWineGetVersion) <> nil;
  madTools.IsWine := IsWine;
  CustomInjDrvFile[0] := #0;
  SetLastError(0);
  {$ifdef log}
    log('initialization end');
  {$endif}
end;

procedure FinalizeMadCHook;
begin
  {$ifdef log}
    log('finalization begin');
  {$endif}
  AutoUnhook2(dword(-1), false);
  {$ifdef forbiddenAPIs}
    if forbiddenMutex <> 0 then begin
      CloseHandle(forbiddenMutex);
      forbiddenMutex := 0;
    end;
  {$endif}
  if HookReady and (HookList = nil) then begin
    if AutoUnhookMap <> 0 then begin
      CloseHandle(AutoUnhookMap);
      AutoUnhookMap := 0;
    end;
    HookReady := false;
    DeleteCriticalSection(HookSection);
  end;
  if LpcReady then begin
    LpcReady := false;
    DeleteCriticalSection(LpcSection);
  end;
  if LpcCounterBuf <> nil then
    UnmapViewOfFile(LpcCounterBuf);
  {$ifdef log}
    log('finalization end');
  {$endif}
end;

// ***************************************************************

initialization
  InitializeMadCHook;

finalization
  FinalizeMadCHook;

{$endif}

end.
