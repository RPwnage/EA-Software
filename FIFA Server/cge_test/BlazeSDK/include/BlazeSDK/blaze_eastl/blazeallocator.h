/*! ***************************************************************************/
/*!
    \file 
    
    Blaze Allocator for EASTL wrappers.


    \attention
    (c) Electronic Arts. All Rights Reserved.

*****************************************************************************/

#ifndef BLAZE_EASTL_ALLOCATOR_H
#define BLAZE_EASTL_ALLOCATOR_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/alloc.h"
#include "BlazeSDK/allocdefines.h"
#include "EASTL/core_allocator_adapter.h"

namespace Blaze
{

/*! ****************************************************************************/
/*! \class blaze_eastl_allocator

    \brief Custom allocator for Blaze eastl class wrappers.
********************************************************************************/

// We make the CoreAllocatorAdapter a member instead of base class because the latter would cause the CoreAllocatorAdapter (the base class)
// to be dllexported along with this class (the subclass). That could result into linker collisions.
class BLAZESDK_API blaze_eastl_allocator
{
    
public:
    /*! ***************************************************************************/
    /*!    \brief Constructor
    
    \param debugName Memory name to allocate with.  Can be nullptr for release builds.
    \param flags Type of memory to allocate (EA::Allocator::MEM_TEMP, MEM_PERM)
    *******************************************************************************/
    blaze_eastl_allocator(const char8_t *debugName, int32_t flags = EA::Allocator::MEM_PERM);

    /*! ***************************************************************************/
    /*!    \brief Constructor

        \param memGroupId memory group id to use to create the stl storage
        \param debugName Memory name to allocate with.  Can be nullptr for release builds.
        \param flags Type of memory to allocate (EA::Allocator::MEM_TEMP, MEM_PERM)
    *******************************************************************************/
    blaze_eastl_allocator(MemoryGroupId memGroupId, const char8_t *debugName, int32_t flags = EA::Allocator::MEM_PERM);
    
    /*! ***************************************************************************/
    /*!    \brief Default constructor needed for fixed vector allocator.
    *******************************************************************************/
    blaze_eastl_allocator();    
    
    blaze_eastl_allocator(const blaze_eastl_allocator& x);
    blaze_eastl_allocator(const blaze_eastl_allocator& x, const char8_t* debugName);
    blaze_eastl_allocator(const char8_t* debugName, EA::Allocator::ICoreAllocator* pAllocator, int32_t flags);

    blaze_eastl_allocator& operator=(const blaze_eastl_allocator& x);

    void* allocate(size_t n, int32_t flags = 0);
    void* allocate(size_t n, size_t alignment, size_t offset, int32_t flags = 0);
    void deallocate(void* p, size_t n);

    EA::Allocator::ICoreAllocator* get_allocator() const;
    void set_allocator(EA::Allocator::ICoreAllocator* pAllocator);

    int32_t get_flags() const;
    void set_flags(int32_t flags);

    const char8_t* get_name() const;
    void set_name(const char8_t *debugName);

protected:
    friend bool operator==(const blaze_eastl_allocator& a, const blaze_eastl_allocator& b);      
    friend bool operator!=(const blaze_eastl_allocator& a, const blaze_eastl_allocator& b);

private:
    typedef EA::Allocator::CoreAllocatorAdapter<EA::Allocator::ICoreAllocator> base_type;
    base_type mAllocator;    

};

bool operator==(const blaze_eastl_allocator& a, const blaze_eastl_allocator& b);      
bool operator!=(const blaze_eastl_allocator& a, const blaze_eastl_allocator& b);

}        // namespace Blaze

#endif    // BLAZE_EASTL_ALLOCATOR_H
