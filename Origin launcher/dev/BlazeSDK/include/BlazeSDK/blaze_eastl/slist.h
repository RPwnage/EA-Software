/*! ***************************************************************************/
/*!
    \file 
    
    Blaze sList Wrapper


    \attention
        (c) Electronic Arts. All Rights Reserved.

*****************************************************************************/

#ifndef BLAZE_EASTL_SLIST_H
#define BLAZE_EASTL_SLIST_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blaze_eastl/blazeallocator.h"
#include "EASTL/slist.h"

// TODO BLAZE3.0 - blaze_eastl should be moved out from BlazeSDK public interface

namespace Blaze
{

/*! ****************************************************************************/
/*! \class slist

    \brief Wrapper class for eastl::slist that adds in Blaze allocation.
********************************************************************************/
template <typename T>
class slist : public eastl::slist<T, blaze_eastl_allocator>
{
public:

    /*! ***************************************************************************/
    /*!    \brief Constructor

        \param memGroupId    id of the memory group to be used to create the stl storage
        \param debugMemName  Debug allocation name to assign to this instance.
    *******************************************************************************/
    slist(MemoryGroupId memGroupId, const char8_t *debugMemName) :
        eastl::slist<T, blaze_eastl_allocator>(blaze_eastl_allocator(memGroupId, debugMemName, ((memGroupId & MEM_GROUP_TEMP_FLAG) ? EA::Allocator::MEM_TEMP : EA::Allocator::MEM_PERM) ))
        {
        }

};

}        // namespace Blaze

#endif    // BLAZE_EASTL_SLIST_H
