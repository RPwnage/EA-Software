
#include "Messaging.h"

#include "BlazeInitialization.h" // for REPORT_BLAZE_ERROR in SendMessageCb(), Ignition::gBlazeHub in runSendMessageToGroup(), Pyro::UiDriver in ctor

namespace Ignition
{

Messaging::Messaging(uint32_t userIndex)
    : LocalUserUiBuilder("Messaging", userIndex)
{
    setIsVisible(false);
}

Messaging::~Messaging()
{
}

void Messaging::runSendMessageToGroup(const Blaze::UserGroup *userGroup, const char8_t *text)
{
    Blaze::Messaging::SendMessageParameters sendMessageParameters;

    sendMessageParameters.setTargetGroup(userGroup);

    sendMessageParameters.getFlags().setEcho();
    sendMessageParameters.getFlags().setFilterProfanity();

    sendMessageParameters.getAttrMap().insert(
        eastl::make_pair(Blaze::Messaging::MSG_ATTR_SUBJECT, text));

    getUiDriver()->addDiagnosticCallFunc(gBlazeHub->getMessagingAPI(getUserIndex())->sendMessage(sendMessageParameters,
        Blaze::Messaging::MessagingAPI::SendMessageCb(this, &Messaging::SendMessageCb)));
}

void Messaging::SendMessageCb(const Blaze::Messaging::SendMessageResponse *sendMessageResponse, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    getUiDriver()->addDiagnosticCallback();
    REPORT_BLAZE_ERROR(blazeError);
}

}
