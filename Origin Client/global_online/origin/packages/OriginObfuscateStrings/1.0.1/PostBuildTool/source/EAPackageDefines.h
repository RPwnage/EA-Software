#include <coreallocator/icoreallocator_interface.h>

void* operator new[](size_t size, char const*, int, unsigned int, char const*, int)
{
    return new char[size];
}

namespace EA
{

namespace Allocator
{

class OSAllocator : public ICoreAllocator
{
public:
    virtual void* Alloc(size_t size, const char*, unsigned int)
    {
        return new char[size];
    }
    
    virtual void* Alloc(size_t size, const char*, unsigned int, unsigned int, unsigned int)
    {
        return new char[size];
    }
    
    virtual void Free(void* block, size_t)
    {
        delete[] (char*)block;
    }
};
    
OSAllocator gAllocator;
    
ICoreAllocator* ICoreAllocator::GetDefaultAllocator()
{
    return &gAllocator;
}
    
}
    
}