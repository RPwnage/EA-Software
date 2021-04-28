/////////////////////////////////////////////////////////////////////////////
// eathread_sync_gcc.h
//
// Copyright (c) 2010, Electronic Arts Inc. All rights reserved.
//
// Functionality related to memory and code generation synchronization.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_GCC_EATHREAD_SYNC_GCC_H
#define EATHREAD_GCC_EATHREAD_SYNC_GCC_H


#include <EABase/eabase.h>


#define EA_THREAD_SYNC_IMPLEMENTED


// EAProcessorPause
// Intel has defined a 'pause' instruction for x86 processors starting with the P4, though this simply
// maps to the otherwise undocumented 'rep nop' instruction. This pause instruction is important for 
// high performance spinning, as otherwise a high performance penalty incurs. 

#if defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)
	#define EAProcessorPause() __asm__ __volatile__ ("rep ; nop")
#else
	#define EAProcessorPause()
#endif



// EAReadBarrier / EAWriteBarrier / EAReadWriteBarrier
// The x86 processor memory architecture ensures read and write consistency on both single and
// multi processing systems. This makes programming simpler but limits maximimum system performance.
// We define EAReadBarrier here to be the same as EACompilerMemory barrier in order to limit the 
// compiler from making any assumptions at its level about memory usage. Year 2003+ versions of the 
// Microsoft SDK define a 'MemoryBarrier' statement which has the same effect as EAReadWriteBarrier.

#if (((__GNUC__ * 100) + __GNUC_MINOR__) >= 401) // GCC 4.1 or later
	#define EAReadBarrier      __sync_synchronize
	#define EAWriteBarrier     __sync_synchronize
	#define EAReadWriteBarrier __sync_synchronize
#else
	#define EAReadBarrier      EACompilerMemoryBarrier
	#define EAWriteBarrier     EACompilerMemoryBarrier
	#define EAReadWriteBarrier EACompilerMemoryBarrier
#endif


// EACompilerMemoryBarrier

#if defined(EA_PROCESSOR_ARM) || defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)
	#define EACompilerMemoryBarrier() __asm__ __volatile__ ("":::"memory")
#else
	#define EACompilerMemoryBarrier()
#endif



#endif // EATHREAD_GCC_EATHREAD_SYNC_GCC_H








