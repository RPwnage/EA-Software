// ***************************************************************
//  Wow64ProcessList.h        version: 1.0.0  ·  date: 2010-06-17
//  -------------------------------------------------------------
//  this list contains all newly created wow64 processes
//  -------------------------------------------------------------
//  Copyright (C) 1999 - 2010 www.madshi.net, All Rights Reserved
// ***************************************************************

// 2010-06-17 1.0.0 initial release

#include <ntddk.h>
#ifndef _WIN64
  #undef ExAllocatePool
  #undef ExFreePool      // NT4 doesn't support ExFreePoolWithTag
#endif

#include "Wow64ProcessList.h"

// ********************************************************************

// wow64 process list management
ULONG Count = 0;
ULONG Capacity = 0;
HANDLE *List = NULL;
FAST_MUTEX Mutex;

// ********************************************************************

BOOLEAN AddNewWow64Process(HANDLE ProcessId)
// add a new wow64 process to our list
{
  ExAcquireFastMutex(&Mutex);
  __try
  {

    // first let's check whether this process is already in the list
    int i1;
    for (i1 = 0; i1 < (int) Count; i1++)
      if (List[i1] == ProcessId)
      {
        // it's already in the list, so we do nothing
        return TRUE;
      }

    // we have a new wow64 process

    if (Count == Capacity)
    {
      // the wow64 process list is either full or not initialized yet
      // so we need to do some (re)allocation work
      HANDLE *newList;
      if (Capacity)
        Capacity = Capacity * 2;
      else
        Capacity = 64;
      newList = ExAllocatePool(PagedPool, Capacity * sizeof(HANDLE));
      if (newList)
      {
        RtlZeroMemory(newList, Capacity * sizeof(HANDLE));
        if (Count)
          memcpy(newList, List, Count * sizeof(HANDLE));
      }
      else
      {
        Count = 0;
        Capacity = 0;
      }
      if (List)
        ExFreePool(List);
      List = newList;
    }

    // finally let's store the new wow64 process into the list
    if (List)
    {
      List[Count++] = ProcessId;
      return TRUE;
    }

  }
  __finally
  {
    ExReleaseFastMutex(&Mutex);
  }

  return FALSE;
}

// ********************************************************************

BOOLEAN DelNewWow64Process(HANDLE ProcessId)
// remove the specified new wow64 process from our list
{
  ExAcquireFastMutex(&Mutex);
  __try
  {

    int i1;
    for (i1 = 0; i1 < (int) Count; i1++)
      if (List[i1] == ProcessId)
      {
        // we found the record we're searching for, so let's remove it from the list
        int i2;
        Count--;
        for (i2 = i1; i2 < (int) Count; i2++)
          List[i2] = List[i2 + 1];
        List[Count] = NULL;
        return TRUE;
      }

  }
  __finally
  {
    ExReleaseFastMutex(&Mutex);
  }

  return FALSE;
}

// ********************************************************************

BOOLEAN IsNewWow64Process(HANDLE ProcessId)
// is the specified process contained in our list?
{
  ExAcquireFastMutex(&Mutex);
  __try
  {

    int i1;
    for (i1 = 0; i1 < (int) Count; i1++)
      if (List[i1] == ProcessId)
      {
        // yes - it is!
        return TRUE;
      }

  }
  __finally
  {
    ExReleaseFastMutex(&Mutex);
  }

  return FALSE;
}

// ********************************************************************

void InitWow64ProcessList(void)
// initialize the wow64 process list - called from driver initialization
{
  ExInitializeFastMutex(&Mutex);
}

void FreeWow64ProcessList(void)
// free the wow64 process list - used for driver shutdown
{
  if (List)
  {
    ExFreePool(List);
    List = NULL;
  }
  ExInitializeFastMutex(&Mutex);
}

// ********************************************************************
