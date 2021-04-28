/*************************************************************************************************/
/*!
    \file   updateprofaneaiplayernames_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateProfaneAiPlayerNamesCommand

    Reset to a defaul the AI Player names of a specific club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// pro clubs includes
#include "proclubscustom/rpc/proclubscustomslave/updateprofaneaiplayernames_stub.h"
#include "proclubscustomslaveimpl.h"
#include "proclubscustom/tdf/proclubscustomtypes.h"
#include "clubs/tdf/clubs_base.h"

namespace Blaze
{
namespace proclubscustom
{

class UpdateProfaneAiPlayerNamesCommand : public UpdateProfaneAiPlayerNamesCommandStub
{
public:
    UpdateProfaneAiPlayerNamesCommand(Message* message, UpdateProfaneAiPlayerNamesRequest* request, ProclubsCustomSlaveImpl* componentImpl)
        : UpdateProfaneAiPlayerNamesCommandStub(message, request), mComponent(componentImpl)
    {
    }

    ~UpdateProfaneAiPlayerNamesCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
	UpdateProfaneAiPlayerNamesCommand::Errors execute() override
    {
        BlazeRpcError error = Blaze::ERR_SYSTEM;

		uint64_t clubId = mRequest.getClubId();
		eastl::string statusMessage;
		TRACE_LOG("[UpdateProfaneAiPlayerNames].execute clubid " << clubId);

		if (clubId != Blaze::Clubs::INVALID_CLUB_ID)
		{
			mComponent->resetAiPlayerNames(clubId, statusMessage, error);
		}
		else
		{
			WARN_LOG("[UpdateProfaneAiPlayerNames].execute: Invalid clubId provided");
			statusMessage = "Invalid clubId provided";
		}

		mResponse.setStatusMessage(statusMessage.c_str());

        return commandErrorFromBlazeError(error);
    }

	// Not owned memory.
	ProclubsCustomSlaveImpl* mComponent;

};
// static factory method impl
UpdateProfaneAiPlayerNamesCommandStub* UpdateProfaneAiPlayerNamesCommandStub::create(Message* message, UpdateProfaneAiPlayerNamesRequest* request, proclubscustomSlave* componentImpl)
{
	return BLAZE_NEW UpdateProfaneAiPlayerNamesCommand(message, request, static_cast<ProclubsCustomSlaveImpl*>(componentImpl));
}

} // proclubscustom
} // Blaze
