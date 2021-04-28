/*! ***************************************************************************/
/*!
    \file
     Source file for allocation routines.


    \attention
    (c) Electronic Arts. All Rights Reserved.

*******************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/alloc.h"
#include <string.h>

namespace Blaze
{

#if (ENABLE_BLAZE_SDK_LOGGING == 1) || defined(EA_DEBUG)
    static const char8_t *_MemGroupNames[] =
    {
        "MEM_GROUP_DEFAULT",
        "MEM_GROUP_FRAMEWORK",
        "MEM_GROUP_GAMEMANAGER",
        "MEM_GROUP_GAMEBROWSER",
        "MEM_GROUP_STATS",
        "MEM_GROUP_LEADERBOARD",
        "MEM_GROUP_LOGINMANAGER",
        "MEM_GROUP_MESSAGING",
        "MEM_GROUP_MAIL",
        "MEM_GROUP_NETWORKADAPTER",
        "MEM_GROUP_CENSUSDATA",
        "MEM_GROUP_ASSOCIATIONLIST",
        "MEM_GROUP_UTIL",
        "MEM_GROUP_TELEMETRY",
        "MEM_GROUP_TITLE",
        "MEM_GROUP_TITLE_2",
        "MEM_GROUP_TITLE_3",
        "MEM_GROUP_TITLE_4",
        "MEM_GROUP_TITLE_5",
        "MEM_GROUP_TITLE_6",
        "MEM_GROUP_TITLE_7",
        "MEM_GROUP_TITLE_8"
    };
#endif

EA::Allocator::ICoreAllocator* Allocator::msAllocator[MEM_GROUP_MAX] = {};

/*! ****************************************************************************/
/*! \brief get the assigned allocator for the memory group

    \param[in] memGroupId memory group ID

    \return pointer to the allocator
********************************************************************************/
EA::Allocator::ICoreAllocator *Allocator::getAllocator(MemoryGroupId memGroupId)
{
    // Stripped out mem flag from memGroupId parameter
    const MemoryGroupId flaglessMemGroupId = (MemoryGroupId)(memGroupId & ~MEM_GROUP_TEMP_FLAG);

    BlazeAssertMsg(flaglessMemGroupId < MEM_GROUP_MAX, "invalid memory group!");

    return msAllocator[flaglessMemGroupId];
}

/*! ****************************************************************************/
/*! \brief set allocator for the default memory group, (and all other unset memory groups)

    \param[in] ator pointer to the default allocator
********************************************************************************/
void Allocator::setAllocator(EA::Allocator::ICoreAllocator* ator) 
{
    BlazeAssertMsg((ator != nullptr), "allocator cannot be nullptr!");

    for(uint32_t i = 0; i < MEM_GROUP_MAX; ++i)
    {
        if(msAllocator[i] == nullptr)
        {
            msAllocator[i] = ator;
        }
        else
        {
            BlazeAssertMsg(msAllocator[i] == ator, "allocator cannot be changed after being assigned");
        }
    }
}

/*! ****************************************************************************/
/*! \brief set the allocator for a memory group

    \param[in] memGroupId memory group identifier
    \param[in] allocator  pointer to the allocator
********************************************************************************/
void Allocator::setAllocator(MemoryGroupId memGroupId, EA::Allocator::ICoreAllocator *ator) 
{
    // Stripped out mem flag from memGroupId parameter
    const MemoryGroupId flaglessMemGroupId = (MemoryGroupId) (memGroupId & ~MEM_GROUP_TEMP_FLAG);

    BlazeAssertMsg((flaglessMemGroupId < MEM_GROUP_MAX), "invalid memory group!");

    BlazeAssertMsg((ator != nullptr), "allocator cannot be nullptr!");

    // Warn if allocator is set with a mem group id that was ORed with MEM_GROUP_TEMP_FLAG
    if ((memGroupId & MEM_GROUP_TEMP_FLAG) != 0)
    {
        BLAZE_SDK_DEBUGF(
            "Blaze::setAllocator() used with mem group %s ORed with MEM_GROUP_TEMP_FLAG - ignoring flag...\n", 
            _MemGroupNames[flaglessMemGroupId]);
    }

    //changing allocator after it has been set is very bad
    BlazeAssertMsg(
        ((msAllocator[flaglessMemGroupId] == nullptr) || 
        (msAllocator[flaglessMemGroupId] == msAllocator[MEM_GROUP_DEFAULT])), 
        "allocator should not be changed after being assigned a non-default value");


    msAllocator[flaglessMemGroupId] = ator;
}

void Allocator::clearAllocator(MemoryGroupId memGroupId)
{
    const MemoryGroupId flaglessMemGroupId = (MemoryGroupId) (memGroupId & ~MEM_GROUP_TEMP_FLAG);
    msAllocator[flaglessMemGroupId] = nullptr;
}

void Allocator::clearAllAllocators()
{
    for(uint32_t i = 0; i < MEM_GROUP_MAX; ++i)
    {
        msAllocator[i] = nullptr;
    }
}


/*! ****************************************************************************/
/*! \brief get the memory group matching this allocator

    \param[in] allocator pointer to the allocator

    \return mem group ID
    
    WARNING: This function will return the first matching memory group 
             using supplied allocator!
********************************************************************************/
MemoryGroupId Allocator::getMemGroupId(EA::Allocator::ICoreAllocator* ator)
{
    if (ator == nullptr)
        return (MEM_GROUP_DEFAULT);


    for (uint8_t memGroupId = 0; memGroupId < MEM_GROUP_MAX; memGroupId++)
    {
        if (msAllocator[memGroupId] == ator)
        {
            return memGroupId;
        }
    }

    BlazeAssertMsg(false, "allocator has never been associated with a mem group");
    return (MEM_GROUP_DEFAULT); 
}


#ifdef EA_DEBUG

/*! ****************************************************************************/
/*! \brief build a debug name string from a mem group id and a user provided string

    \param[in] memGroupId   memory group ID
    \param[in] userString   user-provided string

    \return   pointer to charbuf.buf string

    NOTE: This function IS reentrant.
********************************************************************************/
const char8_t* Allocator::buildDebugNameString(MemoryGroupId memGroupId, const char8_t *) 
{
    // Stripped out mem flag from memGroupId parameter
    MemoryGroupId flaglessMemGroupId = (MemoryGroupId)(memGroupId & ~MEM_GROUP_TEMP_FLAG);

    // Make sure mem group id is valid
    BlazeAssertMsg((flaglessMemGroupId < MEM_GROUP_MAX), "no valid debug name string for the specified mem group id");

    const char8_t* str;
    if (flaglessMemGroupId < MEM_GROUP_MAX)
    {
        str = _MemGroupNames[flaglessMemGroupId];
    }
    else
    {
        str = "INVALID_MEM_GROUP_ID";
    }
    return (str);
}

#endif


} // namespace Blaze



