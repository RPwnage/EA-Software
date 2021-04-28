/*************************************************************************************************/
/*!
    \file   findclubs2_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FindClubs2Command

    Find a set of clubs using a complex search criteria.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/findclubs2_stub.h"
#include "clubsslaveimpl.h"
#include "findclubs_commandbase.h"


namespace Blaze
{
namespace Clubs
{

class FindClubs2Command : public FindClubs2CommandStub, private FindClubsCommandBase
{
public:
    FindClubs2Command(Message* message, FindClubs2Request* request, ClubsSlaveImpl* componentImpl)
        : FindClubs2CommandStub(message, request),
        FindClubsCommandBase(componentImpl, "FindClubs2Command")
    {
    }

    ~FindClubs2Command() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    FindClubs2CommandStub::Errors execute() override
    {
        uint32_t totalCount = 0;
        BlazeRpcError result = FindClubsCommandBase::findClubsUtil(
            mRequest.getParams(),
            &mRequest.getMemberOnlineStatusFilter(),
            mRequest.getMemberOnlineStatusSum(),
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
DEFINE_FINDCLUBS2_CREATE()

} // Clubs
} // Blaze
