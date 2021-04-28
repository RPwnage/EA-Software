/*! ***************************************************************************/
/*!
    \file 
    
    Blaze Vector Map wrapper


    \attention
    (c) Electronic Arts. All Rights Reserved.

*****************************************************************************/

#ifndef BLAZE_EASTL_VECTOR_MAP_H
#define BLAZE_EASTL_VECTOR_MAP_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blaze_eastl/blazeallocator.h"
#include "EASTL/vector_map.h"

// TODO BLAZE3.0 - blaze_eastl should be moved out from BlazeSDK public interface

namespace Blaze
{

/*! ****************************************************************************/
/*! \class vector_map

    \brief Wrapper class for eastl::vector_map that adds in Blaze allocation.
********************************************************************************/
template <typename Key, typename T, typename Compare = eastl::less<Key> >
class vector_map : public eastl::vector_map<Key, T, Compare, blaze_eastl_allocator >
{
public:

    /*! ***************************************************************************/
    /*!    \brief Constructor

        \param memGroupId    id of the memory group to be used to create the stl storage
        \param debugMemName  Debug allocation name to assign to this instance.
    *******************************************************************************/
    vector_map(MemoryGroupId memGroupId, const char8_t *debugMemName) :
        eastl::vector_map<Key, T, Compare, blaze_eastl_allocator>(blaze_eastl_allocator(memGroupId, debugMemName, ((memGroupId & MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM) ))
        {
        }

};

}        // namespace Blaze

#endif    // BLAZE_EASTL_VECTOR_MAP_H
