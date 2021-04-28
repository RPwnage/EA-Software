/////////////////////////////////////////////////////////////////////////////
// eathread_sync_armgcc.h
//
// Copyright (c) 2010, Electronic Arts Inc. All rights reserved.
//
// Functionality related to memory and code generation synchronization.
//
// Created by Rob Parolin 
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_ARMGCC_EATHREAD_SYNC_ARMGCC_H
#define EATHREAD_ARMGCC_EATHREAD_SYNC_ARMGCC_H

// Header file should not be included directly.  Provided here for backwards compatibility.
// Please use eathread_sync.h

#if defined(EA_PROCESSOR_ARM) 
	#include <eathread/arm/eathread_sync_arm.h>
#endif

#endif
