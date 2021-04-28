///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Allocator.cpp
// 
// Copyright (c) Electronic Arts. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Allocator.h>
#include <coreallocator/icoreallocator_interface.h>


namespace EA
{
namespace Patch
{



EA::Allocator::ICoreAllocator* gpCoreAllocator = NULL;


EAPATCHCLIENT_API Allocator::ICoreAllocator* GetAllocator()
{
    if(!gpCoreAllocator)
        gpCoreAllocator = EA::Allocator::ICoreAllocator::GetDefaultAllocator();

    return gpCoreAllocator;
}


EAPATCHCLIENT_API void SetAllocator(Allocator::ICoreAllocator* pCoreAllocator)
{
    gpCoreAllocator = pCoreAllocator;
}



} // namespace IO

} // namespace EA












