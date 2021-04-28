/*************************************************************************************************/
/*!
    \file   countmessages_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CountMessagesCommand

    Count petition/invitations associated with club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

// clubs includes
#include "clubs/rpc/clubsslave/countmessages_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class CountMessagesCommand : public CountMessagesCommandStub, private ClubsCommandBase
{
public:
    CountMessagesCommand(Message* message, CountMessagesRequest* request, ClubsSlaveImpl* componentImpl)
        : CountMessagesCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~CountMessagesCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    CountMessagesCommandStub::Errors execute() override
    {
        TRACE_LOG("[CountMessagesCommand] start");

        ClubIdList requestCludIdList;
        requestCludIdList.push_back(mRequest.getClubId());
        ClubMessageCountMap countMap;

        BlazeRpcError error = mComponent->countMessagesForClubs(
            requestCludIdList,
            mRequest.getMessageType(),
            countMap);

        if (countMap.find(mRequest.getClubId()) != countMap.end())
        {
            mResponse.setCount(countMap[mRequest.getClubId()]);
        }
        else
        {
            mResponse.setCount(0);
        }

        return commandErrorFromBlazeError(error);
    }

};
// static factory method impl
DEFINE_COUNTMESSAGES_CREATE()

} // Clubs
} // Blaze
