/*************************************************************************************************/
/*!
    \file   findclubs_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FindClubsCommand

    Find a set of clubs using a complex search criteria.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/findclubs_stub.h"
#include "clubsslaveimpl.h"
#include "findclubs_commandbase.h"


namespace Blaze
{
namespace Clubs
{

class FindClubsCommand : public FindClubsCommandStub, private FindClubsCommandBase
{
public:
    FindClubsCommand(Message* message, FindClubsRequest* request, ClubsSlaveImpl* componentImpl)
        : FindClubsCommandStub(message, request),
        FindClubsCommandBase(componentImpl, "FindClubsCommand")
    {
    }

    ~FindClubsCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    FindClubsCommandStub::Errors execute() override
    {
        uint32_t totalCount = 0;
        BlazeRpcError result = FindClubsCommandBase::findClubsUtil(
            mRequest,
            nullptr,     // memberOnlineStatusFilter
            0,     // memberOnlineStatusSum
            mResponse.getClubList(),
            totalCount,
            nullptr);    // sequenceID

        if (result == Blaze::ERR_OK)
        {
            // setup response
            mResponse.setTotalCount(totalCount);
        }

        return commandErrorFromBlazeError(result);
    }
};

// static factory method impl
DEFINE_FINDCLUBS_CREATE()

} // Clubs
} // Blaze
