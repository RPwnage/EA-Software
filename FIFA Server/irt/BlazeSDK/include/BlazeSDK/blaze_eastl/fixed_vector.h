/*! ***************************************************************************/
/*!
    \file 
    
    Blaze Fixed Vector Wrapper


    \attention
    (c) Electronic Arts. All Rights Reserved.

*****************************************************************************/

#ifndef BLAZE_EASTL_FIXED_VECTOR_H
#define BLAZE_EASTL_FIXED_VECTOR_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blaze_eastl/blazeallocator.h"
#include "EASTL/fixed_vector.h"

namespace Blaze
{

/*! ****************************************************************************/
/*! \class fixed_vector

    \brief Wrapper class for eastl::fixed_vector that adds in Blaze allocation.
********************************************************************************/
template <typename T, size_t nodeCount, bool bEnableOverflow>
class fixed_vector : public eastl::fixed_vector<T, nodeCount, bEnableOverflow, blaze_eastl_allocator >
{
public:
    typedef eastl::fixed_vector<T, nodeCount, bEnableOverflow, blaze_eastl_allocator > base_type;

    /*! ***************************************************************************/
    /*!    \brief Constructor

        \param memGroupId id of the memory group to be used to create the stl storage
        \param debugMemName Debug allocation name to assign to this instance.
    *******************************************************************************/
    fixed_vector(MemoryGroupId memGroupId, const char8_t *debugMemName) : base_type()
    {
        base_type::get_allocator().get_overflow_allocator().set_allocator(Blaze::Allocator::getAllocator(memGroupId));
        base_type::get_allocator().get_overflow_allocator().set_flags(((memGroupId & MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM));

       // MClouatre - May 26 2009. Do not use the function below as it does not copy the string, but it only stores the string pointer in the core allocator adapter.
       // base_type::get_allocator().get_overflow_allocator().set_name(debugMemName);
    }

    /*! ***************************************************************************/
    /*!    \brief Constructor

        \param memGroupId id of the memory group to be used to create the stl storage
        \param n Number of items to initialize.
        \param debugMemName Debug allocation name to assign to this instance.
    *******************************************************************************/
    fixed_vector(MemoryGroupId memGroupId, typename base_type::size_type n, const char8_t *debugMemName) 
        : base_type(n)
    {
        base_type::get_allocator().get_overflow_allocator().set_allocator(Blaze::Allocator::getAllocator(memGroupId));
        base_type::get_allocator().get_overflow_allocator().set_flags(((memGroupId & MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM));

       // MClouatre - May 26 2009. Do not use the function below as it does not copy the string, but it only stores the string pointer in the core allocator adapter.
       // base_type::get_allocator().get_overflow_allocator().set_name(debugMemName);
    }

};

}        // namespace Blaze

#endif    // BLAZE_EASTL_FIXED_VECTOR_H
