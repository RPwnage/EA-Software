///////////////////////////////////////////////////////////////////////////////
// Allocator.cpp
// 
// Copyright (c) 2011, Electronic Arts. All Rights Reserved.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EATrace/internal/Config.h>
#include <EATrace/Allocator.h>
#include <coreallocator/icoreallocator_interface.h>


namespace EA
{
    namespace Trace
    {
        EA::Allocator::ICoreAllocator* gpCoreAllocator = NULL;


        EATRACE_API EA::Allocator::ICoreAllocator* GetAllocator()
        {
            #if EATRACE_DEFAULT_ALLOCATOR_ENABLED
                if(!gpCoreAllocator)
                    gpCoreAllocator = EA::Allocator::ICoreAllocator::GetDefaultAllocator();
            #endif

            return gpCoreAllocator;
        }


        EATRACE_API void SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator)
        {
            gpCoreAllocator = pCoreAllocator;
        }

    } // namespace IO

} // namespace EA



