
/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/sendcustompinevent_stub.h"

namespace Blaze
{
namespace Arson
{
class SendCustomPinEventCommand : public SendCustomPinEventCommandStub
{
public:
    SendCustomPinEventCommand(Message* message, SendCustomPinEventRequest* request, ArsonSlaveImpl* componentImpl)
        : SendCustomPinEventCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~SendCustomPinEventCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    SendCustomPinEventCommandStub::Errors execute() override
    {
        if(gCurrentUserSession == nullptr)
        {
            ERR_LOG("[SendCustomPinEventCommandStub].execute: No current usersession.");
            return ERR_SYSTEM;
        }

        Blaze::UserSessionId sessionId = gCurrentUserSession->getUserSessionId();

        RiverPoster::LoginEventPtr loginEvent = BLAZE_NEW RiverPoster::LoginEvent;
        loginEvent->setStatus("success");
        loginEvent->setType(RiverPoster::PIN_SDK_TYPE_BLAZE);

        RiverPoster::PINEventHeaderCore &headerCore = loginEvent->getHeaderCore();
        headerCore.setEventName("custom");
        // Filled custom subs field, arson-test will expect a string "EA_ACCESS,ORIGIN_ACCESS" in the pin event retrieved
        headerCore.getSubscriptions().push_back("EA_ACCESS");
        headerCore.getSubscriptions().push_back("ORIGIN_ACCESS");

        if (strcmp(mRequest.getCustomEventType(), ""))
            headerCore.setEventType(mRequest.getCustomEventType());

        if (gCurrentLocalUserSession != nullptr &&
            gCurrentLocalUserSession->getPeerInfo() != nullptr &&
            gCurrentLocalUserSession->getPeerInfo()->getClientInfo() != nullptr)
        {
            headerCore.setSdkVersion(gCurrentLocalUserSession->getPeerInfo()->getClientInfo()->getBlazeSDKVersion());
        }

        PINSubmissionPtr req = BLAZE_NEW PINSubmission;
        RiverPoster::PINEventList *pinEventList = req->getEventsMap().allocate_element();
        EA::TDF::tdf_ptr<EA::TDF::VariableTdfBase> pinEvent = pinEventList->pull_back();
        pinEvent->set(*loginEvent);
        req->getEventsMap()[sessionId] = pinEventList;
        gUserSessionManager->sendPINEvents(req);

        return ERR_OK;     
    }
};

DEFINE_SENDCUSTOMPINEVENT_CREATE()

} //Arson
} //Blaze
