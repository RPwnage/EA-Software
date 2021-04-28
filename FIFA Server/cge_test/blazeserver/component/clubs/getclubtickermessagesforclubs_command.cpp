/*************************************************************************************************/
/*!
    \file   getclubrickermessagesforclubs_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class getClubTickerMessagesForClubsCommand

    Get ticker messages for the given list of clubs.

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
#include "clubs/rpc/clubsslave/getclubtickermessagesforclubs_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class GetClubTickerMessagesForClubsCommand : public GetClubTickerMessagesForClubsCommandStub, private ClubsCommandBase
{
public:
    GetClubTickerMessagesForClubsCommand(Message* message, GetClubTickerMessagesForClubsRequest* request, ClubsSlaveImpl* componentImpl)
        : GetClubTickerMessagesForClubsCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetClubTickerMessagesForClubsCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:

    GetClubTickerMessagesForClubsCommandStub::Errors execute() override
    {
        Blaze::BlazeRpcError result = mComponent->getClubTickerMessagesForClubs(mRequest.getClubIdList(),
            mRequest.getOldestTimestamp(),
            gCurrentUserSession->getSessionLocale(),
            mResponse.getMsgListMap());

        return commandErrorFromBlazeError(result);
    }
};

// static factory method impl
DEFINE_GETCLUBTICKERMESSAGESFORCLUBS_CREATE()

} // Clubs
} // Blaze
