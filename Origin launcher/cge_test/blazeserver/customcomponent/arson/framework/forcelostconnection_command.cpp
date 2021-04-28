/*************************************************************************************************/
/*!
    \file   forcelostconnection_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/forcelostconnection_stub.h"


namespace Blaze
{
namespace Arson
{
class ForceLostConnectionCommand : public ForceLostConnectionCommandStub
{
public:
    ForceLostConnectionCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : ForceLostConnectionCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~ForceLostConnectionCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl *mComponent;

    static void doDisconnect(UserSessionMasterPtr session)
    {
        session->forceDisconnect();
    }

    ForceLostConnectionCommandStub::Errors execute() override
    {
        if(!gCurrentUserSession)
        {
            return ERR_SYSTEM;
        }

        //Schedule a disconnect for some time in the future.
        gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + 1000000, &ForceLostConnectionCommand::doDisconnect, gCurrentLocalUserSession.get(), "doDisconnect");

        return ERR_OK;
    }
};

DEFINE_FORCELOSTCONNECTION_CREATE()

} //Arson
} //Blaze
