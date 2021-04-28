// ***************************************************************
//  DriverEvents.c            version: 1.0.2  ·  date: 2011-11-19
//  -------------------------------------------------------------
//  handles all the various driver events
//  -------------------------------------------------------------
//  Copyright (C) 1999 - 2011 www.madshi.net, All Rights Reserved
// ***************************************************************

// 2011-11-19 1.0.2 added support for Windows 8
// 2010-11-18 1.0.1 (1) DriverEvent_NewImage added
//                  (2) DriverEvent_InjectMethodChange added
//                  (3) DriverEvent_EnumInjectRequest added
// 2010-01-10 1.0.0 initial release

#include <ntddk.h>
#ifndef _WIN64
  #undef ExAllocatePool
  #undef ExFreePool      // NT4 doesn't support ExFreePoolWithTag
#endif

#include "ToolFuncs.h"
#include "InjectLibrary.h"
#include "ProcessList.h"
#include "DllList.h"
#include "SessionList.h"
#include "Config.h"
#include "StrMatch.h"

#ifdef _WIN64
  #include "Wow64ProcessList.h"
#endif

// ********************************************************************

BOOLEAN DriverEvent_InitDriver(PDRIVER_OBJECT DriverObject)
// driver got loaded
{
  __try
  {
    return ( (IsValidConfig()) && (InitToolFuncs()) && (InitInjectLibrary(NULL)) );

  } __except (1) {

    // ooops!
    LogUlonglong(L"\\??\\C:\\InitDriver crash", 0);
    return FALSE;
  }
}

void DriverEvent_UnloadDriver(PDRIVER_OBJECT DriverObject)
// driver is about to be unloaded
{
  __try
  {
    // cleanup our allocated structures
    FreeDllList();
    FreeProcessList();
    FreeSessionList();

  } __except (1) {

    // ooops!
    LogUlonglong(L"\\??\\C:\\UnloadDriver crash", 0);
  }
}

// ********************************************************************

// did user land contact us and ask for permission to stop the driver yet?
BOOLEAN StopAllowed = FALSE; 

BOOLEAN DriverEvent_AllowUnloadRequest(BOOLEAN Allow, BOOLEAN *DriverUnloadEnabled)
// user land asks us to allow/disallow unloading of the driver
{
  __try
  {

    // check if the driver was configured to support stopping
    if (IsSafeUnloadAllowed())
    {
      // we allow stopping only if:
      // (1) the driver was originally configured to allow stopping
      // (2) IOCTL_ALLOW_STOP was called to allow stopping
      // (3) no dll injection is currently active
      StopAllowed = (Allow != FALSE);
      if ((StopAllowed) && (!EnumDlls(0, NULL)))
       *DriverUnloadEnabled = TRUE;
      else
        *DriverUnloadEnabled = FALSE;
      return TRUE;
    }
  } __except (1) {

    // ooops!
    LogUlonglong(L"\\??\\C:\\AllowUnloadRequest crash", 0);
  }

  return FALSE;
}


// ********************************************************************

BOOLEAN DriverEvent_InjectMethodChange(BOOLEAN NewInjectionMethod)
// user land tells us to use a specific injection method
{
  SetInjectionMethod(NewInjectionMethod);
  return TRUE;
}

// ********************************************************************

void DriverEvent_SessionGone(ULONG Session)
// a session has just closed down
{
  __try
  {

    DelSessionDlls(Session);

  } __except (1) {

    // ooops!
    LogUlonglong(L"\\??\\C:\\SessionGone crash", 0);
  }
}

// ********************************************************************

void InjectIntoProcess(HANDLE ph, BOOLEAN b64Process)
// helper function, eventually called by
// - DriverEvent_NewProcess
// - DriverEvent_NewImage
{
  __try
  {

    // what kind of process is this?
    BOOLEAN natProcess = IsNativeProcess(ph);
    BOOLEAN sysProcess = IsSystemProcess(ph);
    ULONG session = GetProcessSessionId(ph);

    if (!natProcess)
    {
      // this is not a native process (into which we generally don't inject)
      // now let's check all dll injection requests
      int i1 = 0;
      PDllItem dll;
      LPWSTR buf = NULL, pathBuf = NULL, nameBuf = NULL;
      int pathLen = 0, nameLen = 0;
      BOOLEAN fileNameDone = FALSE;

      while (EnumDlls(i1, &dll))
      {
        if ( ((dll->Session == (ULONG) -1) || (dll->Session == session)) &&
             ((dll->Flags &1) || (!sysProcess)) &&
             (((dll->Flags & 0x80000000) != 0) == b64Process) )
        {
          // looking good, but we still need to check includes/excludes
          BOOLEAN doInject = TRUE;

          if ((dll->IncludeLen) || (dll->ExcludeLen))
          {
            if (!fileNameDone)
            {
              fileNameDone = TRUE;
              buf = (LPWSTR) ExAllocatePool(PagedPool, 64 * 1024);
              if (GetExeFileName(ph, buf, 32 * 1024))
              {
                _wcslwr(buf);
                SplitNamePath(buf, &pathBuf, &nameBuf, &pathLen, &nameLen);
              }
            }
            if ( ((pathLen) || (nameLen)) &&
                 ( ((dll->IncCount) && (!MatchStrArray(pathBuf, nameBuf, pathLen, nameLen, dll->IncCount, dll->IncPathBuf, dll->IncNameBuf, dll->IncPathLen, dll->IncNameLen))) ||
                   ((dll->ExcCount) && ( MatchStrArray(pathBuf, nameBuf, pathLen, nameLen, dll->ExcCount, dll->ExcPathBuf, dll->ExcNameBuf, dll->ExcPathLen, dll->ExcNameLen)))    ) )
              // the new process is not supposed to get this dll injected
              // based on the exe file name and the include/exclude lists
              doInject = FALSE;
          }

          if (doInject)
            // we got a full match here, so let's inject the dll
            InjectLibrary(ph, dll->Name);
        }
        i1++;
      }

      if (buf)
        ExFreePool(buf);
    }

  } __except (1) {

    // ooops!
    LogUlonglong(L"\\??\\C:\\InjectIntoProcess crash", 0);
  }
}

// ********************************************************************

void DriverEvent_NewProcess(HANDLE ProcessId)
// a new process was just created
{
  __try
  {
    HANDLE ph = OpenProcess(ProcessId);

    if (ph)
    {
      // we were able to open a process handle for the newly created process

      #ifdef _WIN64

        // 32bit processes on a 64bit process are a pain in the a**
        // the 32bit ntdll.dll is not loaded at this moment yet,
        // but we need that, so we can't inject right now

        if (Is64bitProcess(ph))
        {
          // this is a native 64bit process, so we can inject right away
          InjectIntoProcess(ph, TRUE);
        }
        else
          if (IsVistaOrNewer())
          {
            // in Vista x64 (and newer OSs) we inject in DriverEvent_NewImage
            // we need to maintain a list of newly created wow64 processes
            AddNewWow64Process(ProcessId);
          }
          else
          {
            // XP/2003 x64 need the old injection method for wow64 processes
            // which is done here, the new one is done in DriverEvent_NewImage
            InjectIntoProcess(ph, FALSE);
          }

      #else

        // this is a native 32bit process on a native 32bit OS, so no problem
        InjectIntoProcess(ph, FALSE);

      #endif

      // let's update our session process count information
      IncSessionCount(GetProcessSessionId(ph));

      // cleanup
      ZwClose(ph);
    }
  } __except (1) {

    // ooops!
    LogUlonglong(L"\\??\\C:\\NewProcess crash", 0);
  }
}

void DriverEvent_ProcessGone(HANDLE ProcessId)
// a running process has just closed down
{
  __try
  {
    HANDLE ph = OpenProcess(ProcessId);

    if (ph)
    {
      // we were able to open a process handle for the closed down process

      // let's update our session process count information
      ULONG session = GetProcessSessionId(ph);
      if (DecSessionCount(session))
        DriverEvent_SessionGone(session);

      #ifdef _WIN64
        // remove the process from our list of newly created wow64 processes
        if (IsVistaOrNewer())
          DelNewWow64Process(ProcessId);
      #endif

      // cleanup
      ZwClose(ph);
    }

    // remove the process from the list of known processes
    DelKnownProcess(ProcessId);

  } __except (1) {

    // ooops!
    LogUlonglong(L"\\??\\C:\\ProcessGone crash", 0);
  }
}

// ********************************************************************

#ifdef _WIN64

  // this structure is used to transport information to the work item
  typedef struct _NewImageContext
  {
    KEVENT       Event;
    PIO_WORKITEM WorkItem;
    HANDLE       ProcessId;
    PIMAGE_INFO  ImageInfo;
  } NewImageContext, *PNewImageContext;

  VOID NewImageWorkerRoutine(PDEVICE_OBJECT DeviceObject, PVOID Context)
  // we inject the library in a work item, just to be safe
  // this guarantees that we inject at PASSIVE_LEVEL
  {
    PNewImageContext context = Context;

    HANDLE ph = OpenProcess(context->ProcessId);

    if (ph)
    {
      // we were able to open a process handle for the newly created process

      // just double checking that this is really a wow64 process
      // and whether the newly loaded image is the needed wow64 ntdll
      if ((!Is64bitProcess(ph)) && (IsRemoteModule32bit(ph, context->ImageInfo->ImageBase)))
      {

        // remove the wow64 process from the list
        // we only want to inject into a new wow64 process once!
        DelNewWow64Process(context->ProcessId);

        // inform injection library about the 32bit ntdll
        Set32bitNtdllInfo(context->ImageInfo->ImageBase,
                          GetRemoteAddressOfEntryPoint(ph, context->ImageInfo->ImageBase),
                          GetRemoteProcAddress(ph, context->ImageInfo->ImageBase, "NtProtectVirtualMemory"),
                          GetRemoteProcAddress(ph, context->ImageInfo->ImageBase, "LdrLoadDll"),
                          GetRemoteProcAddress(ph, context->ImageInfo->ImageBase, "NtTestAlert"));

        // as expected, this is a wow64 process, so we finally inject
        InjectIntoProcess(ph, FALSE);

      }

      // cleanup
      ZwClose(ph);
    }

    // we're done, so we delete the work item
    IoFreeWorkItem(context->WorkItem);

    // finally, let's signal that we're done
    KeSetEvent(&context->Event, 0, FALSE);
  }

  void DriverEvent_NewImage(PDEVICE_OBJECT DeviceObject, HANDLE ProcessId, PUNICODE_STRING FullImageName, PIMAGE_INFO ImageInfo)
  // a new image was just loaded into a process
  // this event is needed for the new wow64 injection logic
  // which unfortunately only works for Vista and newer x64 OSs
  {
    __try
    {

      if ( (ImageInfo) && (!ImageInfo->SystemModeImage) &&
           (FullImageName) && (FullImageName->Length >= 18) &&
           (KeGetCurrentIrql() <= APC_LEVEL) )
      {
        // this is not a driver / kernel mode image
        // and the image name is long enough to be what we're looking for

        WCHAR dllName [10];
        RtlCopyMemory(dllName, &(FullImageName->Buffer[((int) FullImageName->Length) / 2 - 9]), 18);
        dllName[9] = 0;

        if ((!_wcsicmp(dllName, L"ntdll.dll")) && (IsNewWow64Process(ProcessId)))
        {
          // this is the 32bit ntdll in a new wow64 process
          // finally the wow64 process is ready for dll injection

          NewImageContext context;

          // there's no guarantee that we're at PASSIVE_LEVEL at this time
          // our DLL injection routine needs PASSIVE_LEVEL, though
          // so we move DLL injection to a work item

          context.WorkItem  = IoAllocateWorkItem(DeviceObject);
          context.ProcessId = ProcessId;
          context.ImageInfo = ImageInfo;

          if (context.WorkItem)
          {
            // work item was successfully created

            // now let's init a notification event and execute the work item
            KeInitializeEvent(&context.Event, NotificationEvent, FALSE);
            IoQueueWorkItem(context.WorkItem, NewImageWorkerRoutine, DelayedWorkQueue, &context);

            // finally we wait for the work item to complete
            KeWaitForSingleObject(&context.Event, Executive, KernelMode, FALSE, NULL);
          }
        }
      }

    } __except (1) {

      // ooops!
      LogUlonglong(L"\\??\\C:\\NewImage crash", 0);
    }
  }

#endif

// ********************************************************************

BOOLEAN InjectionUninjectionRequest(PDllItem Dll, BOOLEAN Inject, HANDLE CallingProcessId, BOOLEAN *DriverUnloadEnabled)
// user land asks us to start/stop injection of a specific dll
{
  BOOLEAN result = FALSE;
  BOOLEAN freeDll = TRUE;
  UCHAR exeHash [20];
  BOOLEAN ok;

  ok = FindKnownProcess(CallingProcessId, exeHash);
  if (!ok)
  {
    // the calling process calls our driver for the first time
    // so we open the exe file of the caller and calculate a hash

    HANDLE ph = OpenProcess(CallingProcessId);
    if (ph)
    {
      WCHAR exeFile[260];
      RtlZeroMemory(exeFile, sizeof(exeFile));
      ok = ( GetExeFileName(ph, exeFile, 260) &&
             GetFileHash(exeFile, exeHash, 1 * 1024 * 1024, NULL, NULL) );
      if (ok)
        // cache the hash for better performance
        AddKnownProcess(CallingProcessId, exeHash);
      ZwClose(ph);
    }
  }

  if (ok)
  {
    // we were able to get a hash of the exe file which called us

    // first let's bind the dll record to the calling exe file
    // only the exe file which started the injection can stop it
    memcpy(Dll->OwnerHash, exeHash, 20);

    if (Inject)
    {
      if (AddDll(Dll))
      {
        // this worked out fine
        // as long as injection is active we disallow stopping the driver
        *DriverUnloadEnabled = FALSE;
        freeDll = FALSE;
        result = TRUE;
      }
    } else {
      if (DelDll(Dll))
      {
        // the dll injection was stopped as requested
        // let's check whether we can allow stopping the driver now
        if ((StopAllowed) && (!EnumDlls(0, NULL)))
          *DriverUnloadEnabled = TRUE;
        result = TRUE;
      }
    }
  }

  if (freeDll)
    ExFreePool(Dll);

  return result;
}

BOOLEAN DriverEvent_InjectionRequest(PDllItem Dll, HANDLE CallingProcessId, BOOLEAN *DriverUnloadEnabled)
// user land asks us to start injection of a specific dll
{
  __try
  {
    if (InitInjectLibrary(CallingProcessId))
    {
      ULONG session = Dll->Session;
      BOOLEAN result = InjectionUninjectionRequest(Dll, TRUE, CallingProcessId, DriverUnloadEnabled);

      if ((result) && (session != (ULONG) -1))
        InitSessionCount(session);

      return result;
    }

  } __except (1) {

    // ooops!
    LogUlonglong(L"\\??\\C:\\InjectionRequest crash", 0);
  }

  return FALSE;
}

BOOLEAN DriverEvent_UninjectionRequest(PDllItem Dll, HANDLE CallingProcessId, BOOLEAN *DriverUnloadEnabled)
// user land asks us to stop injection of a specific dll
{
  __try
  {
    return InjectionUninjectionRequest(Dll, FALSE, CallingProcessId, DriverUnloadEnabled);

  } __except (1) {

    // ooops!
    LogUlonglong(L"\\??\\C:\\UninjectionRequest crash", 0);
  }

  return FALSE;
}

// ********************************************************************

BOOLEAN DriverEvent_EnumInjectRequest(ULONG Index, PDllItem Dll, ULONG BufSize)
// user land wants to enumerate the active dll injection requests
{
  __try
  {

    PDllItem temp = NULL;
    BOOLEAN result = EnumDlls(Index, &temp) && (temp) && (temp->Size <= BufSize);
    if (result)
      memcpy(Dll, temp, temp->Size);

    return result;

  } __except (1) {

    // ooops!
    LogUlonglong(L"\\??\\C:\\EnumInjectRequest crash", 0);
  }

  return FALSE;
}

// ********************************************************************
