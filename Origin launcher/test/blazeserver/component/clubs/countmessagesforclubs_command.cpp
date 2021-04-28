/*************************************************************************************************/
/*!
    \file   CountMessagesForClubs_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CountMessagesForClubsCommand

    Count petition/invitations associated with clubs.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

// clubs includes
#include "clubs/rpc/clubsslave/countmessagesforclubs_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class CountMessagesForClubsCommand : public CountMessagesForClubsCommandStub, private ClubsCommandBase
{
public:
    CountMessagesForClubsCommand(Message* message, CountMessagesForClubsRequest* request, ClubsSlaveImpl* componentImpl)
        : CountMessagesForClubsCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~CountMessagesForClubsCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:

    CountMessagesForClubsCommandStub::Errors execute() override
    {
        TRACE_LOG("[CountMessagesForClubsCommand] start");

        BlazeRpcError error = mComponent->countMessagesForClubs(
            mRequest.getClubIdList(),
            mRequest.getMessageType(),
            mResponse.getCountMap());

        return commandErrorFromBlazeError(error);           
    }
};

// static factory method impl
DEFINE_COUNTMESSAGESFORCLUBS_CREATE()

} // Clubs
} // Blaze
