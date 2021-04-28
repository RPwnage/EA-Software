/////////////////////////////////////////////////////////////////////////////
// Allocator.h
//
// Copyright (c) 2011, Electronic Arts. All Rights Reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_ALLOCATOR_H
#define EATRACE_ALLOCATOR_H


#include <EATrace/internal/Config.h>
#include <coreallocator/icoreallocator_interface.h>


namespace EA
{
    namespace Trace
    {
        /// GetAllocator
        /// 
        /// Gets the default EATrace ICoreAllocator set by the SetAllocator function.
        /// If SetAllocator hasn't been called, the ICoreAllocator::GetDefaultAllocator 
        /// allocator is returned.
        ///
        EATRACE_API Allocator::ICoreAllocator* GetAllocator();


        /// SetAllocator
        /// 
        /// This function lets the user specify the default memory allocator this library will use.
        /// For the most part, this library avoids memory allocations. But there are times 
        /// when allocations are required, especially during startup.
        ///
        EATRACE_API void SetAllocator(Allocator::ICoreAllocator* pCoreAllocator);

    }
}

#endif // Header include guard














