
#ifndef NISMESSAGING_H
#define NISMESSAGING_H

#include "NisSample.h"

#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/messaging/messaging.h"

namespace NonInteractiveSamples
{

class NisMessaging : 
    public NisSample
{
    public:
        NisMessaging(NisBase &nisBase);
        virtual ~NisMessaging();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    private:
        void            UserMessageCb(const Blaze::Messaging::ServerMessage* msg, const Blaze::UserManager::User* user);
        void            FetchMessagesCb(const Blaze::Messaging::FetchMessageResponse* response, Blaze::BlazeError error, Blaze::JobId id);
        void            PurgeMessagesCb(const Blaze::Messaging::PurgeMessageResponse* response, Blaze::BlazeError error, Blaze::JobId id);
        void            LookupUserCb(Blaze::BlazeError error, Blaze::JobId id, const Blaze::UserManager::User* user);
        void            SendMessageCb(const Blaze::Messaging::SendMessageResponse* response, Blaze::BlazeError error, Blaze::JobId id);
};

}

#endif // NISMESSAGING_H
