/*************************************************************************************************/
/*!
    \file   updateprofaneavatarname_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateProfaneAvatarNameCommand

    Update a users avatar name to a default one

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// pro clubs includes
#include "proclubscustom/rpc/proclubscustomslave/updateprofaneavatarname_stub.h"
#include "proclubscustomslaveimpl.h"
#include "proclubscustom/tdf/proclubscustomtypes.h"
#include "proclubscustom/tdf/proclubscustom_server.h"

namespace Blaze
{
namespace proclubscustom
{

class UpdateProfaneAvatarNameCommand : public UpdateProfaneAvatarNameCommandStub
{
public:
	UpdateProfaneAvatarNameCommand(Message* message, updateProfaneAvatarNameRequest* request, ProclubsCustomSlaveImpl* componentImpl)
        : UpdateProfaneAvatarNameCommandStub(message, request)
    {
    }

    ~UpdateProfaneAvatarNameCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
	UpdateProfaneAvatarNameCommandStub::Errors execute() override
    {
        TRACE_LOG("[UpdateProfaneAvatarNameCommand].execute");

		BlazeRpcError error = Blaze::ERR_SYSTEM;

		mComponent->flagAvatarNameAsProfane(mRequest.getBlazeId(), error);

        return commandErrorFromBlazeError(error);
    }

	// Not owned memory.
	ProclubsCustomSlaveImpl* mComponent;

};
// static factory method impl
UpdateProfaneAvatarNameCommandStub* UpdateProfaneAvatarNameCommandStub::create(Message* message, updateProfaneAvatarNameRequest* request, proclubscustomSlave* componentImpl)
{
	return BLAZE_NEW UpdateProfaneAvatarNameCommand(message, request, static_cast<ProclubsCustomSlaveImpl*>(componentImpl));
}

} // Clubs
} // Blaze
