#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include <assert.h>
#include <stdlib.h>

#include "impl/OriginSDKimpl.h"
#include "impl/OriginErrorFunctions.h"

static bool gMemAllocatorsFrozen = false;
static OriginAllocFunc _allocFunc = &malloc; 
static OriginFreeFunc _freeFunc = &free;
#if defined ORIGIN_MAC

void* _aligned_malloc(size_t size, size_t align)
{
    void* mem = malloc(size + (align - 1) + sizeof(void*));
    
    char* amem = ((char*)mem) + sizeof(void*);
    amem += align - ((uintptr_t)amem & (align - 1));
    
    ((void**)amem)[-1] = mem;
    return amem;
}

void _aligned_free(void* mem)
{
    free(((void**)mem)[-1]);
}
#endif
static OriginAllocAlignedFunc _allocAlignFunc = &_aligned_malloc;
static OriginFreeAlignedFunc _freeAlignFunc = &_aligned_free;

namespace Origin {

	void FreezeMemoryAllocators ()
	{
		gMemAllocatorsFrozen = true;
	}

	void ThawMemoryAllocators ()
	{
		gMemAllocatorsFrozen = false;
	}

    void* AllocFunc(size_t size)
    {
        return _allocFunc(size);
    }

    void FreeFunc(void *p)
    {
        _freeFunc(p);
    }

    void* AllocAlignFunc(size_t size, size_t alignment)
    {
        return _allocAlignFunc(size, alignment);
    }

    void FreeAlignFunc(void *p)
    {
        _freeAlignFunc(p);
    }

}; // namespace Origin

OriginErrorT OriginSetupMemory(OriginAllocFunc alloc, 
							 OriginFreeFunc free,
							 OriginAllocAlignedFunc alloc_align, 
							 OriginFreeAlignedFunc free_align)
{
	if(!gMemAllocatorsFrozen)
	{
		// Check whether we have a matching alloc and free combinations.

		if((alloc != NULL && free == NULL) || (alloc == NULL && free != NULL))
			return REPORTERROR(ORIGIN_ERROR_SDK_INVALID_ALLOCATOR_DEALLOCATOR_COMBINATION);

		if((alloc_align != NULL && free_align == NULL) || (alloc_align == NULL && free_align != NULL))
			return REPORTERROR(ORIGIN_ERROR_SDK_INVALID_ALLOCATOR_DEALLOCATOR_COMBINATION);

		// Replace default memory framework.

		if(alloc != NULL)
		{
			_allocFunc = alloc; 
			_freeFunc = free; 
		}
		else
		{
			_allocFunc = &::malloc;
			_freeFunc = &::free;
		}

		if(alloc_align != NULL)
		{
			_allocAlignFunc = alloc_align;
			_freeAlignFunc = free_align; 
		}
		else
		{
			_allocAlignFunc = &::_aligned_malloc;
			_freeAlignFunc =  &::_aligned_free;
		}

		return REPORTERROR(ORIGIN_SUCCESS);
	}
	else
	{
		return REPORTERROR(ORIGIN_ERROR_SDK_IS_RUNNING);
	}
}


OriginErrorT OriginResetMemory()
{
	if(!gMemAllocatorsFrozen)
	{
		_allocFunc = &malloc; 
		_freeFunc = &free; 
		_allocAlignFunc = &_aligned_malloc;
		_freeAlignFunc = &_aligned_free; 

		return REPORTERROR(ORIGIN_SUCCESS);
	}
	else
	{
		return REPORTERROR(ORIGIN_ERROR_SDK_IS_RUNNING);
	}
}
