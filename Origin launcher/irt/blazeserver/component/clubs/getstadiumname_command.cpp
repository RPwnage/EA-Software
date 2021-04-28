/*************************************************************************************************/
/*!
    \file   getstadiumbname_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetStadiumName

    Get stadium name of a specified club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

//clubs includes
#include "clubs/rpc/clubsslave/getstadiumname_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"
#include "clubs/tdf/clubs_base.h"
#include "clubs/tdf/clubs_server.h"

namespace Blaze
{
namespace Clubs
{

class GetStadiumNameCommand : public GetStadiumNameCommandStub,  private ClubsCommandBase
{
public:
	GetStadiumNameCommand(Message* message, GetStadiumNameRequest* request, ClubsSlaveImpl* componentImpl)
        : GetStadiumNameCommandStub(message, request)
        , ClubsCommandBase(componentImpl)
    {
    }

    ~GetStadiumNameCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
	GetStadiumNameCommand::Errors execute() override
    {
		TRACE_LOG("[GetClubNameCommand].execute() (clubid = " << mRequest.getClubId() << ")");

        eastl::string playerNames;
		Blaze::BlazeRpcError error = Blaze::ERR_OK;

        ClubId clubId = mRequest.getClubId();

        if (mComponent->getConfig().getDisableAbuseReporting())
        {
            error = CLUBS_ERR_NO_PRIVILEGE;
        }

        if (Blaze::ERR_OK == error)
        {
            if (clubId == INVALID_CLUB_ID)
            {
                error = CLUBS_ERR_INVALID_CLUB_ID;                
            }
        }

        if (Blaze::ERR_OK == error)
        {
            error = mComponent->fetchStadiumName(clubId, playerNames);
        }

        if (Blaze::ERR_OK == error)
        {
            mResponse.setStadiumName(playerNames.c_str());
        }

		return GetStadiumNameCommand::commandErrorFromBlazeError(error);
    }
};

// static factory method impl
DEFINE_GETSTADIUMNAME_CREATE()

} // Clubs
} // Blaze
