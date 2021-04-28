/*************************************************************************************************/
/*!
    \file   getclubawards_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubAwards

    Get awards for a specific club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/getclubawards_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class GetClubAwardsCommand : public GetClubAwardsCommandStub, private ClubsCommandBase
{
public:
    GetClubAwardsCommand(Message* message, GetClubAwardsRequest* request, ClubsSlaveImpl* componentImpl)
        : GetClubAwardsCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetClubAwardsCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetClubAwardsCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetClubAwardsCommand] start");

        ClubsDbConnector dbc(mComponent); 
        if (dbc.acquireDbConnection(true) == nullptr)
            return ERR_SYSTEM;

        BlazeRpcError result =
            dbc.getDb().getClubAwards(
            mRequest.getClubId(), 
            mResponse.getClubAwardList());
        dbc.releaseConnection();

        if (result != Blaze::ERR_OK && result != Blaze::CLUBS_ERR_INVALID_CLUB_ID)
        {
            ERR_LOG("[GetClubAwardsCommand] Database error (" << result << ")");
        }

        return (GetClubAwardsCommandStub::Errors)result;
    }

};
// static factory method impl
DEFINE_GETCLUBAWARDS_CREATE()

} // Clubs
} // Blaze
