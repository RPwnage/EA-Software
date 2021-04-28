/*************************************************************************************************/
/*!
    \file   getavatardata_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetAvatarData

    Get members for a specify club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// pro clubs includes
#include "proclubscustom/rpc/proclubscustomslave/getavatardata_stub.h"
#include "proclubscustomslaveimpl.h"
#include "proclubscustom/tdf/proclubscustomtypes.h"

namespace Blaze
{
namespace proclubscustom
{

class GetAvatarDataCommand : public GetAvatarDataCommandStub
{
public:
	GetAvatarDataCommand(Message* message, getAvatarDataRequest* request, ProclubsCustomSlaveImpl* componentImpl)
        : GetAvatarDataCommandStub(message, request), mComponent(componentImpl)
    {
    }

    ~GetAvatarDataCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
	GetAvatarDataCommand::Errors execute() override
    {
		TRACE_LOG("[GetAvatarDataCommand].execute() (blazeId = " << mRequest.getBlazeId() << ")");

		Blaze::BlazeRpcError error = Blaze::ERR_SYSTEM;

		uint64_t blazeId = mRequest.getBlazeId();
		Blaze::proclubscustom::AvatarData avatarData;

		mComponent->fetchAvatarData(blazeId, mResponse.getAvatarData(), error);

		return GetAvatarDataCommand::commandErrorFromBlazeError(error);
    }

	// Not owned memory.
	ProclubsCustomSlaveImpl* mComponent;

};
// static factory method impl
GetAvatarDataCommandStub* GetAvatarDataCommandStub::create(Message* message, getAvatarDataRequest* request, proclubscustomSlave* componentImpl)
{
	return BLAZE_NEW GetAvatarDataCommand(message, request, static_cast<ProclubsCustomSlaveImpl*>(componentImpl));
}

} // Clubs
} // Blaze
