/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include <EABase/eabase.h>
#include <coreallocator\icoreallocator_interface.h>

///////////////////////////////////////////////////////////////////////////////
// CoreAllocatorMalloc
//
class CoreAllocatorMalloc : public EA::Allocator::ICoreAllocator
{
public:
    CoreAllocatorMalloc();
    CoreAllocatorMalloc(const CoreAllocatorMalloc&);
    ~CoreAllocatorMalloc();
    CoreAllocatorMalloc& operator=(const CoreAllocatorMalloc&);

    void* Alloc(size_t size, const char* /*name*/, unsigned /*flags*/);
    void* Alloc(size_t size, const char* /*name*/, unsigned /*flags*/,
        unsigned /*align*/, unsigned /*alignOffset*/);
    void Free(void* block, size_t /*size*/);
};

static CoreAllocatorMalloc gDefaultAllocator;

CoreAllocatorMalloc::CoreAllocatorMalloc()
{
}

CoreAllocatorMalloc::CoreAllocatorMalloc(const CoreAllocatorMalloc &)
{
}

CoreAllocatorMalloc::~CoreAllocatorMalloc()
{
}

CoreAllocatorMalloc& CoreAllocatorMalloc::operator=(const CoreAllocatorMalloc&)
{
    return *this;
}

void* CoreAllocatorMalloc::Alloc(size_t size, const char* /*name*/, unsigned /*flags*/)
{
    // NOTE: this 'new' gets forwarded to EAGeneralAllocatorDebug by the EATest harness
    char8_t* pMemory = new char8_t[size];
    return pMemory;
}

void* CoreAllocatorMalloc::Alloc(size_t size, const char* /*name*/, unsigned /*flags*/,
    unsigned /*align*/, unsigned /*alignOffset*/)
{
    // NOTE: this 'new' gets forwarded to EAGeneralAllocatorDebug by the EATest harness
    char8_t* pMemory = new char8_t[size];
    return pMemory;
}

void CoreAllocatorMalloc::Free(void* block, size_t /*size*/)
{
    // NOTE: this 'delete' gets forwarded to EAGeneralAllocatorDebug by the EATest harness
    delete[](char8_t*)block;
}

namespace EA
{
    namespace Allocator
    {
        ICoreAllocator * ICoreAllocator::GetDefaultAllocator(void)
        {
            return &gDefaultAllocator;
        }
    }
}


// need to define these operators, they are needed for eastl
void * operator new[](unsigned __int64 size, char const *, int, unsigned int, char const *, int)
{
    return new char8_t[size];
}

void * operator new[](unsigned __int64 size, unsigned __int64, unsigned __int64, char const *, int, unsigned int, char const *, int)
{
    return new char8_t[size];
}
