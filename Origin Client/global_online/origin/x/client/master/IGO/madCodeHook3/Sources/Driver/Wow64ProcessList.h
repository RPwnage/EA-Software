// ***************************************************************
//  Wow64ProcessList.h        version: 1.0.0  ·  date: 2010-06-17
//  -------------------------------------------------------------
//  this list contains all newly created wow64 processes
//  -------------------------------------------------------------
//  Copyright (C) 1999 - 2010 www.madshi.net, All Rights Reserved
// ***************************************************************

// 2010-06-17 1.0.0 initial release

#ifndef _Wow64ProcessList_
#define _Wow64ProcessList_

#include <ntddk.h>
#ifndef _WIN64
  #undef ExAllocatePool
  #undef ExFreePool      // NT4 doesn't support ExFreePoolWithTag
#endif

// ********************************************************************

// add a new wow64 process to our list
BOOLEAN AddNewWow64Process (HANDLE ProcessId);

// remove the specified new wow64 process from our list
BOOLEAN DelNewWow64Process (HANDLE ProcessId);

// is the specified process contained in our list?
BOOLEAN IsNewWow64Process (HANDLE ProcessId);

// initialize the wow64 process list - called from driver initialization
void InitWow64ProcessList (void);

// free the wow64 process list - used for driver shutdown
void FreeWow64ProcessList (void);

// ********************************************************************

#endif
