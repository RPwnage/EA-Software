/////////////////////////////////////////////////////////////////////////////
// eathread_global.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
//
// Created by Rob Parolin 
//
// Provides a unified method of access to EAThread global variables that 
// (when specified by the user) can become DLL safe by adding a dependency 
// on EAStdC EAGlobal implementation.
/////////////////////////////////////////////////////////////////////////////

#ifndef EATHREAD_INTERNAL_GLOBAL_H
#define EATHREAD_INTERNAL_GLOBAL_H

#if EATHREAD_GLOBAL_VARIABLE_DLL_SAFETY
	#include <EAStdC/EAGlobal.h>

	#define EATHREAD_GLOBALVARS (*EA::StdC::AutoStaticOSGlobalPtr<EA::Thread::EAThreadGlobalVars, 0xdabbad00>().get())
	#define EATHREAD_GLOBALVARS_CREATE_INSTANCE  EA::StdC::AutoStaticOSGlobalPtr<EA::Thread::EAThreadGlobalVars, 0xdabbad00> gGlobalVarsInstance;
	#define EATHREAD_GLOBALVARS_EXTERN_INSTANCE  

#else 
	#define EATHREAD_GLOBALVARS gEAThreadGlobalVars
	#define EATHREAD_GLOBALVARS_CREATE_INSTANCE EA::Thread::EAThreadGlobalVars gEAThreadGlobalVars
	#define EATHREAD_GLOBALVARS_EXTERN_INSTANCE extern EA::Thread::EAThreadGlobalVars gEAThreadGlobalVars 

#endif

#endif
