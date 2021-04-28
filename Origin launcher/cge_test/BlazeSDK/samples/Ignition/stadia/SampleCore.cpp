#include "PyroSDK/pyrosdk.h"

#if defined(EA_PLATFORM_STADIA)

#include "Ignition/IgnitionDefines.h"
#include "EASystemEventMessageDispatcher/SystemEventMessageDispatcher.h"
#include "EAControllerUserPairing/EAControllerUserPairingServer.h"
#include "EAControllerUserPairing/Messages.h"

#include <ggp/application.h>

namespace Ignition
{
    class IgnitionIEAMessageHandler : public EA::Messaging::IHandler
    {
    public:
        IgnitionIEAMessageHandler()
        {
            NetPrintf(("SampleCore: [EACUP event handler] initializing\n"));
            memset(aLocalUsers, 0, sizeof(aLocalUsers));
        }

        bool HandleMessage(EA::Messaging::MessageId messageId, void* pMessage)
        {
            bool bSuccess = true;
            const EA::User::IEAUser *pLocalUser;
            EA::Pairing::UserMessage *pUserMessage;

            switch (messageId)
            {
                case EA::Pairing::E_USER_ADDED:
                    NetPrintf(("SampleCore: [EACUP event handler] E_USER_ADDED\n"));
                    pUserMessage = (EA::Pairing::UserMessage *)pMessage;
                    pLocalUser = pUserMessage->GetUser();
                    pLocalUser->AddRef();
                    bSuccess = AddLocalUser(pLocalUser);
                    break;

                case EA::Pairing::E_USER_REMOVED:
                    NetPrintf(("SampleCore: [EACUP event handler] E_USER_REMOVED\n"));
                    pUserMessage = (EA::Pairing::UserMessage *)pMessage;
                    pLocalUser = pUserMessage->GetUser();
                    bSuccess = RemoveLocalUser(pLocalUser);
                    pLocalUser->Release();
                    break;

                default:
                    NetPrintf(("SampleCore: [EACUP event handler] unsupported event (%d)\n", messageId));
                    bSuccess = false;
                    break;
            }

            return(bSuccess);
        }

        bool AddLocalUser(const EA::User::IEAUser *pLocalUser)
        {
            bool bSuccess = false;
            int32_t iLocalUserIndex;

            for (iLocalUserIndex = 0; iLocalUserIndex < iMaxLocalUsers; iLocalUserIndex++)
            {
                if (aLocalUsers[iLocalUserIndex] == nullptr)
                {
                    aLocalUsers[iLocalUserIndex] = pLocalUser;
                    if (NetConnAddLocalUser(iLocalUserIndex, pLocalUser) == 0)
                    {
                        bSuccess = true;
                    }
                    break;
                }
            }

            if (!bSuccess)
            {
                NetPrintf(("SampleCore: E_USER_ADDED failed\n"));
            }

            return(bSuccess);
        }

        bool RemoveLocalUser(const EA::User::IEAUser * pLocalUser)
        {
            bool bSuccess = false;
            int32_t iLocalUserIndex;

            for (iLocalUserIndex = 0; iLocalUserIndex < iMaxLocalUsers; iLocalUserIndex++)
            {
                if (aLocalUsers[iLocalUserIndex] == pLocalUser)
                {
                    if (NetConnRemoveLocalUser(iLocalUserIndex, pLocalUser) == 0)
                    {
                        bSuccess = true;
                    }
                    aLocalUsers[iLocalUserIndex] = nullptr;
                    break;
                }
            }

            if (!bSuccess)
            {
                NetPrintf(("SampleCore: E_USER_REMOVED failed\n"));
            }

            return(bSuccess);
        }

    private:
        static const int32_t iMaxLocalUsers = 16;
        const EA::User::IEAUser *aLocalUsers[iMaxLocalUsers];
    };
    EA::Pairing::EAControllerUserPairingServer *gpEacup;
    EA::SEMD::SystemEventMessageDispatcher *gpEasmd;
    IgnitionIEAMessageHandler *gpEAmsghdlr;

void initializePlatform()
{
    // Called prior to EventQueue usage:
    GgpInitialize(nullptr);

    // STADIA_TODO - See from Sample if need to load any system libs here
    // instantiate system event message dispatcher
    EA::SEMD::SystemEventMessageDispatcherSettings semdSettings;
    semdSettings.useCompanionHttpd = false;
    semdSettings.useCompanionUtil = false;
    semdSettings.useSystemService = true;
    semdSettings.useUserService = true;
    EA::SEMD::SystemEventMessageDispatcher::CreateInstance(semdSettings, &gsAllocator);

    // instantiate EACUP server
    gpEasmd = EA::SEMD::SystemEventMessageDispatcher::GetInstance();
    gpEacup = new EA::Pairing::EAControllerUserPairingServer(gpEasmd, &gsAllocator);
    gpEAmsghdlr = new IgnitionIEAMessageHandler();
    gpEacup->Init();
    gpEacup->AddMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_ADDED, false, 0);
    gpEacup->AddMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_REMOVED, false, 0);
    EA::User::UserList eacupUserList = gpEacup->GetUsers();

    NetConnStartup("-servicename=ignition");
    // NetConnControl('spam', 2, 0, NULL, NULL); // Increase DirtySock logging level if needed

    // to be called after NetConnStartup()
    NetPrintf(("SampleCore: IFrameworkView::Initialize   tick = %d\n", NetTick()));

    // tell netconn about the first set of users
    EA::User::UserList::iterator i;
    for(i = eacupUserList.begin(); i != eacupUserList.end(); i++)
    {
        const EA::User::IEAUser *pLocalUser = *i;
        pLocalUser->AddRef();
        gpEAmsghdlr->AddLocalUser(pLocalUser);
    }
}

void tickPlatform()
{
    gpEasmd->Tick();
    gpEacup->Tick();
    // STADIA_TODO - Need any system tick?
}

void finalizePlatform()
{
    NetPrintf(("SampleCore: IFrameworkView::Uninitialize  tick = %d\n", NetTick()));
    while (NetConnShutdown(0) == -1)
    {
        NetConnSleep(16);
    }

    // free EACUP server
    gpEacup->RemoveMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_ADDED);
    gpEacup->RemoveMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_REMOVED);
    delete gpEAmsghdlr;
    gpEacup->Shutdown();
    delete gpEacup;
    gpEasmd->DestroyInstance();
    
    // STADIA_TODO - Need any system clean up?

    
}

}
#endif //EA_PLATFORM_STADIA