/*************************************************************************************************/
/*!
    \file   findclubs_commandbase.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FindClubsCommandBase

    Abstract class that defines the bulk of functionality for FindClubsCommand/FindClubsAsyncCommand classes.

    \note
        Implements the common methods. Subclasses(FindClubsCommand/FindClubsAsyncCommand) need
        to simply call the functions.
*/
/*************************************************************************************************/

#ifndef BLAZE_FINDCLUBS_COMMANDBASE_H
#define BLAZE_FINDCLUBS_COMMANDBASE_H

/*** Include Files *******************************************************************************/

// clubs includes
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class FindClubsCommandBase : public ClubsCommandBase
{
public:
    ~FindClubsCommandBase() override
    {
    }

protected:
    // This class is supposed to be used only as a base for subclasses.
    FindClubsCommandBase(ClubsSlaveImpl* componentImpl, const char8_t* cmdName);

    // find cached clubs in memory.
    BlazeRpcError findCachedClubs(
        const FindClubsRequest& request,
        const MemberOnlineStatusList* memberOnlineStatusFilter,
        const uint32_t memberOnlineStatusSum,
        ClubList& resultList);

    // the util function
    BlazeRpcError findClubsUtil(
        const FindClubsRequest& request,
        const MemberOnlineStatusList* memberOnlineStatusFilter,
        const uint32_t memberOnlineStatusSum,
        ClubList& resultList,
        uint32_t& totalCount,
        uint32_t* sequenceID,
        uint32_t seqNum = 0);

private:
    const char8_t* mCmdName;
    static const uint32_t MAX_FIND_RESULT_COUNT = 10000;
};

} // Clubs
} // Blaze

#endif // BLAZE_FINDCLUBS_COMMANDBASE_H
