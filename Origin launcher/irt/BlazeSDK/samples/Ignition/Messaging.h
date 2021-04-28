
#ifndef MESSAGING_H
#define MESSAGING_H

#include "Ignition/Ignition.h"

#include "BlazeSDK/messaging/messaging.h"
#include "BlazeSDK/messaging/messagingapi.h"
#include "BlazeSDK/gamemanager/matchmakingsession.h"

namespace Pyro
{
    class UiDriver;
}
namespace Ignition
{

class Messaging :
    public LocalUserUiBuilder
{
    NON_COPYABLE(Messaging);

    public:
        Messaging(uint32_t userIndex);
        virtual ~Messaging();

        void runSendMessageToGroup(const Blaze::UserGroup *userGroup, const char8_t *text);

    private:
        void SendMessageCb(const Blaze::Messaging::SendMessageResponse *sendMessageResponse, Blaze::BlazeError blazeError, Blaze::JobId jobId);
};

}

#endif
