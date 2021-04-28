/*************************************************************************************************/
/*!
    \file   getuserids_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/updateusersessionsubscription_stub.h"

namespace Blaze
{
namespace Arson
{
class UpdateUserSessionSubscriptionCommand : public UpdateUserSessionSubscriptionCommandStub
{
public:
    UpdateUserSessionSubscriptionCommand(
        Message* message, Arson::UpdateUserSessionSubscriptionRequest* request, ArsonSlaveImpl* componentImpl)
        : UpdateUserSessionSubscriptionCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~UpdateUserSessionSubscriptionCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    UpdateUserSessionSubscriptionCommandStub::Errors execute() override
    {
        mComponent->updateSessionSubscription(gUserSessionMaster->getLocalSession(gCurrentLocalUserSession->getId()).get(), mRequest.getUserSessionId(), mRequest.getIsSubscribed());
        return ERR_OK;
    }

};

DEFINE_UPDATEUSERSESSIONSUBSCRIPTION_CREATE()

} //Arson
} //Blaze
