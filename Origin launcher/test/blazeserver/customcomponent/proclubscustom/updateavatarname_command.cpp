/*************************************************************************************************/
/*!
    \file   updateavatarname_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateAvatarName

    Get members for a specify club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// pro clubs includes
#include "proclubscustom/rpc/proclubscustomslave/updateavatarname_stub.h"
#include "proclubscustomslaveimpl.h"
#include "proclubscustom/tdf/proclubscustomtypes.h"

namespace Blaze
{
namespace proclubscustom
{

class UpdateAvatarNameCommand : public UpdateAvatarNameCommandStub
{
public:
	UpdateAvatarNameCommand(Message* message, updateAvatarNameRequest* request, ProclubsCustomSlaveImpl* componentImpl)
        : UpdateAvatarNameCommandStub(message, request), mComponent(componentImpl)
    {
    }

    ~UpdateAvatarNameCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
	UpdateAvatarNameCommand::Errors execute() override
    {
		TRACE_LOG("[UpdateAvatarNameCommand].execute() (firstname = " << mRequest.getFirstname() << " lastname =" << mRequest.getLastname() << " )");

		Blaze::BlazeRpcError error = Blaze::ERR_SYSTEM;

		int64_t userid = gCurrentUserSession->getBlazeId();

		eastl::string avatarFirstName = mRequest.getFirstname();
		eastl::string avatarLastName = mRequest.getLastname();

		if (userid != INVALID_BLAZE_ID)
		{
			mComponent->updateAvatarName(userid, avatarFirstName, avatarLastName, error);
		}

		return UpdateAvatarNameCommand::commandErrorFromBlazeError(error);
    }

	// Not owned memory.
	ProclubsCustomSlaveImpl* mComponent;

};
// static factory method impl
UpdateAvatarNameCommandStub* UpdateAvatarNameCommandStub::create(Message* message, updateAvatarNameRequest* request, proclubscustomSlave* componentImpl)
{
	return BLAZE_NEW UpdateAvatarNameCommand(message, request, static_cast<ProclubsCustomSlaveImpl*>(componentImpl));
}

} // Clubs
} // Blaze
