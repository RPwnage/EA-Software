/*************************************************************************************************/
/*!
    \file   getavatarname_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetAvatarName

    Get members for a specify club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// pro clubs includes
#include "proclubscustom/rpc/proclubscustomslave/getavatarname_stub.h"
#include "proclubscustomslaveimpl.h"
#include "proclubscustom/tdf/proclubscustomtypes.h"

namespace Blaze
{
namespace proclubscustom
{

class GetAvatarNameCommand : public GetAvatarNameCommandStub
{
public:
	GetAvatarNameCommand(Message* message, getAvatarNameRequest* request, ProclubsCustomSlaveImpl* componentImpl)
        : GetAvatarNameCommandStub(message, request), mComponent(componentImpl)
    {
    }

    ~GetAvatarNameCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
	GetAvatarNameCommand::Errors execute() override
    {
		TRACE_LOG("[GetAvatarNameCommand].execute() (blazeId = " << mRequest.getBlazeId() << ")");

		Blaze::BlazeRpcError error = Blaze::ERR_SYSTEM;

		uint64_t blazeId = mRequest.getBlazeId();
		eastl::string avatarFirstName;
		eastl::string avatarLastName;

		mComponent->fetchAvatarName(blazeId, avatarFirstName, avatarLastName, error);

		if (error == Blaze::ERR_OK)
		{
			mResponse.setFirstname(avatarFirstName.c_str());
			mResponse.setLastname(avatarLastName.c_str());
		}

		return GetAvatarNameCommand::commandErrorFromBlazeError(error);
    }

	// Not owned memory.
	ProclubsCustomSlaveImpl* mComponent;

};
// static factory method impl
GetAvatarNameCommandStub* GetAvatarNameCommandStub::create(Message* message, getAvatarNameRequest* request, proclubscustomSlave* componentImpl)
{
	return BLAZE_NEW GetAvatarNameCommand(message, request, static_cast<ProclubsCustomSlaveImpl*>(componentImpl));
}

} // Clubs
} // Blaze
