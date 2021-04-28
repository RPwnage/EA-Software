/*! ***************************************************************************/
/*!
    \file 
    
    Blaze Vector Wrapper


    \attention
    (c) Electronic Arts. All Rights Reserved.

*****************************************************************************/

#ifndef BLAZE_EASTL_VECTOR_H
#define BLAZE_EASTL_VECTOR_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blaze_eastl/blazeallocator.h"
#include "EASTL/vector.h"

// TODO BLAZE3.0 - blaze_eastl should be moved out from BlazeSDK public interface

namespace Blaze
{

/*! ****************************************************************************/
/*! \class vector

   \brief Wrapper class for eastl::vector that adds in Blaze allocation.
********************************************************************************/
template <typename T>
class vector : public eastl::vector<T, blaze_eastl_allocator >
{
public:
    typedef vector<T> this_type;
    typedef eastl::vector<T, blaze_eastl_allocator > base_type;

    /*! ***************************************************************************/
    /*!    \brief Constructor
        \param memGroupId    id of the memory group to be used to create the stl storage
        \param debugMemName  Debug allocation name to assign to this instance.
    *******************************************************************************/
    vector(MemoryGroupId memGroupId, const char8_t *debugMemName) :
        eastl::vector<T, blaze_eastl_allocator>(blaze_eastl_allocator(memGroupId, debugMemName, ((memGroupId & MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM) ))
        {
        }

    /*! ***************************************************************************/
    /*!    \brief Constructor

        \param memGroupId    id of the memory group allocator to be used to create the stl storage
        \param n             Initial size of the vector (contents initialzed to default or 0)
        \param debugMemName  Debug allocation name to assign to this instance.
    *******************************************************************************/    
    vector(MemoryGroupId memGroupId, typename base_type::size_type n, const char8_t *debugMemName) :
        eastl::vector<T, blaze_eastl_allocator>(n, blaze_eastl_allocator(memGroupId, debugMemName, ((memGroupId & MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM) ))
        {
        }

};

}        // namespace Blaze

#endif    // BLAZE_EASTL_VECTOR_H
