/*************************************************************************************************/
/*!
    \file   getclubname_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubName

    Get members for a specify club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/getclubname_stub.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class GetClubNameCommand : public GetClubNameCommandStub, private ClubsCommandBase
{
public:
    GetClubNameCommand(Message* message, GetClubNameRequest* request, ClubsSlaveImpl* componentImpl)
        : GetClubNameCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetClubNameCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetClubNameCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetClubNameCommand].execute");

        BlazeRpcError error = Blaze::ERR_OK;

        if (mComponent->getConfig().getDisableAbuseReporting())
        {
            return CLUBS_ERR_NO_PRIVILEGE;
        }

        ClubsDbConnector dbc(mComponent);
        if (dbc.acquireDbConnection(false) == nullptr)
            return ERR_SYSTEM;

        ClubId clubId = mRequest.getClubId();

        if (clubId == INVALID_CLUB_ID)
            return CLUBS_ERR_INVALID_CLUB_ID;

        Club club;
        uint64_t version = 0;
        error = dbc.getDb().getClub(clubId, &club, version);

        if (error == Blaze::ERR_OK)
        {
            mResponse.setName(club.getName());
        }

        return commandErrorFromBlazeError(error);
    }

};
// static factory method impl
DEFINE_GETCLUBNAME_CREATE()

} // Clubs
} // Blaze
