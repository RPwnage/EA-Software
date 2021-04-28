/*************************************************************************************************/
/*!
    \file   getclubrickermessages_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class getClubTickerMessagesCommand

    Get ticker messages for specified club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbutil.h"
#include "framework/database/query.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/localization.h"

// clubs includes
#include "clubs/rpc/clubsslave/getclubtickermessages_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class GetClubTickerMessagesCommand : public GetClubTickerMessagesCommandStub, private ClubsCommandBase
{
public:
    GetClubTickerMessagesCommand(Message* message, GetClubTickerMessagesRequest* request, ClubsSlaveImpl* componentImpl)
        : GetClubTickerMessagesCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetClubTickerMessagesCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetClubTickerMessagesCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetClubTickerMessagesCommand] start");

        ClubTickerMessageListMap clubTickerMessageListMap;

        ClubIdList clubIdList;
        clubIdList.push_back(mRequest.getClubId());

        clubTickerMessageListMap[mRequest.getClubId()] = &(mResponse.getMsgList());

        Blaze::BlazeRpcError result = mComponent->getClubTickerMessagesForClubs(clubIdList,
            mRequest.getOldestTimestamp(),
            gCurrentUserSession->getSessionLocale(),
            clubTickerMessageListMap);

        clubTickerMessageListMap.clear();

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_GETCLUBTICKERMESSAGES_CREATE()

} // Clubs
} // Blaze
