/////////////////////////////////////////////////////////////////////////////
// Allocator.h
//
// Copyright (c) 2007 Criterion Software Limited and its licensors. All Rights Reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#ifndef EACALLSTACK_ALLOCATOR_H
#define EACALLSTACK_ALLOCATOR_H


#include <EACallstack/internal/Config.h>
#include <coreallocator/icoreallocator_interface.h>


namespace EA
{
	namespace Callstack
	{
		/// GetAllocator
		/// 
		/// Gets the default ICoreAllocator set by the SetAllocator function.
		///
		EACALLSTACK_API EA::Allocator::ICoreAllocator* GetAllocator();


		/// SetAllocator
		/// 
		/// This function lets the user specify the default memory allocator this library will use.
		///
		EACALLSTACK_API void SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator);

	}

}

#endif // Header include guard














