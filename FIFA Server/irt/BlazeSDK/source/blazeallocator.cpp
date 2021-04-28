/*! ***************************************************************************/
/*!
    \file 

    Blaze Allocator for EASTL wrappers.


    \attention
    (c) Electronic Arts. All Rights Reserved.

*******************************************************************************/
#include "BlazeSDK/blaze_eastl/blazeallocator.h"
#include "BlazeSDK/debug.h"

namespace Blaze
{

blaze_eastl_allocator::blaze_eastl_allocator(const blaze_eastl_allocator& x) : mAllocator(x.mAllocator) {}
blaze_eastl_allocator::blaze_eastl_allocator(const blaze_eastl_allocator& x, const char8_t* debugName) : mAllocator(x.mAllocator, debugName) {}  
blaze_eastl_allocator::blaze_eastl_allocator() : mAllocator(nullptr, Allocator::getAllocator()) {};
blaze_eastl_allocator::blaze_eastl_allocator(const char8_t* debugName, EA::Allocator::ICoreAllocator* pAllocator, int32_t flags)
        : mAllocator(debugName, pAllocator, flags) {};

blaze_eastl_allocator::blaze_eastl_allocator(const char8_t *debugName, int32_t flags)
    : mAllocator(debugName, Allocator::getAllocator())
{    
    BlazeAssert(mAllocator.mpCoreAllocator != nullptr);
    mAllocator.set_flags(flags);
}

blaze_eastl_allocator::blaze_eastl_allocator(MemoryGroupId memGroupId, const char8_t *debugName, int32_t flags)
    : mAllocator(debugName, Allocator::getAllocator(memGroupId))
{    
    BlazeAssert(mAllocator.mpCoreAllocator != nullptr);
    mAllocator.set_flags(flags);
}

blaze_eastl_allocator& blaze_eastl_allocator::operator=(const blaze_eastl_allocator& x)
{ 
    mAllocator = x.mAllocator; 
    return *this;
}            

void* blaze_eastl_allocator::allocate(size_t n, int32_t flags /*= 0*/) 
{ 
    return mAllocator.allocate(n, flags); 
}

void* blaze_eastl_allocator::allocate(size_t n, size_t alignment, size_t offset, int32_t flags /*= 0*/) 
{ 
    return mAllocator.allocate(n, alignment, offset, flags); 
}

void blaze_eastl_allocator::deallocate(void* p, size_t n) 
{ 
    mAllocator.deallocate(p, n); 
}

EA::Allocator::ICoreAllocator* blaze_eastl_allocator::get_allocator() const 
{ 
    return mAllocator.get_allocator(); 
}

void blaze_eastl_allocator::set_allocator(EA::Allocator::ICoreAllocator* pAllocator) 
{ 
    mAllocator.set_allocator(pAllocator); 
}

int32_t blaze_eastl_allocator::get_flags() const 
{ 
    return mAllocator.get_flags(); 
}

void blaze_eastl_allocator::set_flags(int32_t flags) 
{ 
    mAllocator.set_flags(flags); 
}

const char8_t* blaze_eastl_allocator::get_name() const 
{ 
    return mAllocator.get_name(); 
}

void blaze_eastl_allocator::set_name(const char8_t *debugName) 
{ 
    mAllocator.set_name(debugName); 
}

bool operator==(const blaze_eastl_allocator& a, const blaze_eastl_allocator& b)
{ 
    return (a.mAllocator.mpCoreAllocator == b.mAllocator.mpCoreAllocator) &&
        (a.mAllocator.mnFlags == b.mAllocator.mnFlags);
}         

bool operator!=(const blaze_eastl_allocator& a, const blaze_eastl_allocator& b)
{ 
    return (a.mAllocator.mpCoreAllocator != b.mAllocator.mpCoreAllocator) ||
        (a.mAllocator.mnFlags != b.mAllocator.mnFlags);
} 

} // Blaze
