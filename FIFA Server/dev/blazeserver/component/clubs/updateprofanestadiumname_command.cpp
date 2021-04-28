/*************************************************************************************************/
/*!
    \file   updateprofanestadiumname_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateProfaneStadiumNameCommand

    Reset to a default the stadium name of a specific club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

//clubs includes
#include "clubs/rpc/clubsslave/updateprofanestadiumname_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"
#include "clubs/tdf/clubs_base.h"
#include "clubs/tdf/clubs_server.h"


namespace Blaze
{
namespace Clubs
{

class UpdateProfaneStadiumNameCommand : public UpdateProfaneStadiumNameCommandStub, private ClubsCommandBase
{
public:
    UpdateProfaneStadiumNameCommand(Message* message, UpdateProfaneStadiumNameRequest* request, ClubsSlaveImpl* componentImpl)
        : UpdateProfaneStadiumNameCommandStub(message, request)
        , ClubsCommandBase(componentImpl)
    {
    }

    ~UpdateProfaneStadiumNameCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
	UpdateProfaneStadiumNameCommand::Errors execute() override
    {        
        BlazeRpcError error = Blaze::ERR_OK;

		uint64_t clubId = mRequest.getClubId();
		eastl::string statusMessage;

        TRACE_LOG("[UpdateProfaneStadiumName].execute clubid " << clubId);

        if (mComponent->getConfig().getDisableAbuseReporting())
        {
            error = CLUBS_ERR_NO_PRIVILEGE;
            statusMessage = "Abuse reporting disabled";
        }

        if (Blaze::ERR_OK == error)
        {
            if (clubId == INVALID_CLUB_ID)
            {
                error = CLUBS_ERR_INVALID_CLUB_ID;
            }
            else
            {
                WARN_LOG("[UpdateProfaneStadiumName].execute: Invalid clubId provided");
                statusMessage = "Invalid clubId provided";
            }
        }

		
		if (Blaze::ERR_OK == error)
		{
			error = mComponent->resetStadiumName(clubId, statusMessage);
		}
	

		mResponse.setStatusMessage(statusMessage.c_str());

        return commandErrorFromBlazeError(error);
    }
};
// static factory method impl
DEFINE_UPDATEPROFANESTADIUMNAME_CREATE()

} // proclubscustom
} // Blaze
