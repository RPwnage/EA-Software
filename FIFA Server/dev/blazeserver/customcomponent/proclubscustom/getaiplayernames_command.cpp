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

// pro clubs includes
#include "proclubscustom/rpc/proclubscustomslave/getaiplayernames_stub.h"
#include "proclubscustomslaveimpl.h"
#include "proclubscustom/tdf/proclubscustomtypes.h"

namespace Blaze
{
namespace proclubscustom
{

class GetAiPlayerNamesCommand : public GetAiPlayerNamesCommandStub
{
public:
	GetAiPlayerNamesCommand(Message* message, GetAiPlayerNamesRequest* request, ProclubsCustomSlaveImpl* componentImpl)
        : GetAiPlayerNamesCommandStub(message, request), mComponent(componentImpl)
    {
    }

    ~GetAiPlayerNamesCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
	GetAiPlayerNamesCommand::Errors execute() override
    {
		TRACE_LOG("[GetClubNameCommand].execute() (clubid = " << mRequest.getClubId() << ")");

		Blaze::BlazeRpcError error = Blaze::ERR_SYSTEM;

		uint64_t clubId = mRequest.getClubId();

		if (clubId > 0)
		{
			eastl::string playerNames;
			mComponent->fetchAiPlayerNames(clubId, playerNames, error);
			mResponse.setName(playerNames.c_str());
		}	

		return GetAiPlayerNamesCommand::commandErrorFromBlazeError(error);
    }

	// Not owned memory.
	ProclubsCustomSlaveImpl* mComponent;

};
// static factory method impl
GetAiPlayerNamesCommandStub* GetAiPlayerNamesCommandStub::create(Message* message, GetAiPlayerNamesRequest* request, proclubscustomSlave* componentImpl)
{
	return BLAZE_NEW GetAiPlayerNamesCommand(message, request, static_cast<ProclubsCustomSlaveImpl*>(componentImpl));
}

} // Clubs
} // Blaze
