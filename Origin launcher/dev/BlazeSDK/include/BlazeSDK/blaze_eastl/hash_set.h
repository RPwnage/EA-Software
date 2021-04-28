/*! ***************************************************************************/
/*!
    \file 
    
    Blaze Hash Set wrapper


    \attention
    (c) Electronic Arts.  All Rights Reserved.

*****************************************************************************/

#ifndef BLAZE_EASTL_HASH_SET_H
#define BLAZE_EASTL_HASH_SET_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blaze_eastl/blazeallocator.h"
#include "EASTL/hash_set.h"

// TODO BLAZE3.0 - blaze_eastl should be moved out from BlazeSDK public interface

namespace Blaze
{

/*! ****************************************************************************/
/*! \class hash_set

    \brief Wrapper class for eastl::hash_set that adds in Blaze allocation.
********************************************************************************/
template <typename Key, typename Hash = eastl::hash<Key>, typename Predicate = eastl::equal_to<Key> >
class hash_set : public eastl::hash_set<Key, Hash, Predicate, blaze_eastl_allocator >
{
public:

    /*! ***************************************************************************/
    /*!    \brief Constructor

        \param memGroupId    id of the memory group to be used to create the stl storage
        \param debugMemName  Debug allocation name to assign to this instance.
    *******************************************************************************/
    hash_set(MemoryGroupId memGroupId, const char8_t *debugMemName) :
        eastl::hash_set<Key, Hash, Predicate, blaze_eastl_allocator>(blaze_eastl_allocator(memGroupId, debugMemName, ((memGroupId & MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM) ))
        {
        }

};

}        // namespace Blaze

#endif    // BLAZE_EASTL_HASH_SET_H
