///////////////////////////////////////////////////////////////////////////////
// Allocator.cpp
// 
// Copyright (c) 2007 Criterion Software Limited and its licensors. All Rights Reserved.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EACallstack/internal/Config.h>
#include <EACallstack/Allocator.h>
#include <coreallocator/icoreallocator_interface.h>


namespace EA
{
namespace Callstack
{


EA::Allocator::ICoreAllocator* gpCoreAllocator = NULL;


EACALLSTACK_API Allocator::ICoreAllocator* GetAllocator()
{
    #if EACALLSTACK_DEFAULT_ALLOCATOR_ENABLED
        if(!gpCoreAllocator)
            gpCoreAllocator = EA::Allocator::ICoreAllocator::GetDefaultAllocator();
    #endif

    return gpCoreAllocator;
}


EACALLSTACK_API void SetAllocator(Allocator::ICoreAllocator* pCoreAllocator)
{
    gpCoreAllocator = pCoreAllocator;
}



} // namespace Callstack

} // namespace EA












