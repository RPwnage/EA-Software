/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#include <EATDF/tdfobject.h>


namespace EA
{
namespace TDF
{

void TdfObjectAllocHelper::fixupRefCount(TdfObject* obj)
{
    obj->startRefCounting();
}

void TdfObject::startRefCounting()
{
    if (isNotRefCounted())
    {
        mRefCount = 0;
    }
}

void* TdfObject::alloc(size_t size, EA::Allocator::ICoreAllocator& allocator, const char* name, unsigned int flags)
{
    /* Make the allocation with kOffset in front to allow us to store count*/
    EA::Allocator::ICoreAllocator** p = reinterpret_cast<EA::Allocator::ICoreAllocator**>(allocator.Alloc(EA::Allocator::detail::kOffset + size, name, flags));
    /* put the allocator address in front of the allocation.*/
    *p = &allocator;
    return reinterpret_cast<uint8_t*>(p) + EA::Allocator::detail::kOffset;
}

void* TdfObject::alloc(size_t size, EA::Allocator::ICoreAllocator& allocator, const EA::Allocator::ICoreAllocator::DebugParams debugParams, unsigned int flags)
{
    /* Make the allocation with kOffset in front to allow us to store count*/
    EA::Allocator::ICoreAllocator** p = reinterpret_cast<EA::Allocator::ICoreAllocator**>(allocator.AllocDebug(EA::Allocator::detail::kOffset + size, debugParams, flags));
    /* put the allocator address in front of the allocation.*/
    *p = &allocator;
    return reinterpret_cast<uint8_t*>(p) + EA::Allocator::detail::kOffset;
}


void TdfObject::free(void* p)
{
    if (p != nullptr)
    {
        uint8_t* alloc = reinterpret_cast<uint8_t*>(p) - EA::Allocator::detail::kOffset;
        EA::Allocator::ICoreAllocator* allocator = *reinterpret_cast<EA::Allocator::ICoreAllocator**>(alloc);
        allocator->Free(alloc);
    }
}


} //namespace TDF

} //namespace EA

