#include "PyroSDK/pyrosdk.h"

#if defined(EA_PLATFORM_NX)

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nn/init.h>
#include <nn/os.h>
#include <nn/socket.h>
#include <nn/fs.h>

#include <NucleusSDK/nucleussdk.h>

#include "IEAController/IEAController.h"
#include "EAControllerUserPairing/EAControllerUserPairingServer.h"
#include "EASystemEventMessageDispatcher/SystemEventMessageDispatcher.h"
#include "EAControllerUserPairing/Messages.h"

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "render.h"

#include "Ignition/Ignition.h"

namespace Ignition {

class IgnitionIEAMessageHandler : public EA::Messaging::IHandler
{
    public:
        IgnitionIEAMessageHandler()
        {
            NetPrintf(("EACUP event handler: initializing\n"));
            ds_memclr(aLocalUsers, sizeof(aLocalUsers));
        }

        bool HandleMessage(EA::Messaging::MessageId messageId, void* pMessage)
        {
            bool bSuccess = true;
            const EA::User::IEAUser *pLocalUser;
            EA::Pairing::UserMessage *pUserMessage;

            switch (messageId)
            {

            case EA::Pairing::E_USER_ADDED:
                NetPrintf(("EACUP event handler: E_USER_ADDED\n"));
                pUserMessage = (EA::Pairing::UserMessage *)pMessage;
                pLocalUser = pUserMessage->GetUser();
                pLocalUser->AddRef();
                bSuccess = AddLocalUser(pLocalUser);
                break;

            case EA::Pairing::E_USER_REMOVED:
                NetPrintf(("EACUP event handler: E_USER_REMOVED\n"));
                pUserMessage = (EA::Pairing::UserMessage *)pMessage;
                pLocalUser = pUserMessage->GetUser();
                bSuccess = RemoveLocalUser(pLocalUser);
                pLocalUser->Release();
                break;

            default:
                NetPrintf(("EACUP event handler: unsupported event (%d)\n", messageId));
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
                NetPrintf(("E_USER_ADDED failed\n"));
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
                NetPrintf(("E_USER_REMOVED failed\n"));
            }

            return(bSuccess);
        }

    private:
        static const int32_t iMaxLocalUsers = NETCONN_MAXLOCALUSERS;
        const EA::User::IEAUser *aLocalUsers[iMaxLocalUsers];
};
EA::Pairing::EAControllerUserPairingServer *gpEacup;
EA::SEMD::SystemEventMessageDispatcher *gpEasmd;
IgnitionIEAMessageHandler *gpEAmsghdlr;


/*** Public Functions ******************************************************************/

void initializePlatform()
{
    EA::SEMD::SystemEventMessageDispatcherSettings semdSettings;
    semdSettings.useCompanionHttpd = false;
    semdSettings.useCompanionUtil = false;
    semdSettings.useSystemService = true;
    semdSettings.useUserService = true;
    gpEasmd = EA::SEMD::SystemEventMessageDispatcher::CreateInstance(semdSettings, &gsAllocator);

    // startup dirtysdk
    int32_t netConnErr = NetConnStartup("-servicename=ignition");

    // initialize NucleusSDK
    EA::Nucleus::Globals::SetAllocator(&gsAllocator);

    // create eacup
    gpEacup = new EA::Pairing::EAControllerUserPairingServer(gpEasmd, &gsAllocator);
    gpEAmsghdlr = new IgnitionIEAMessageHandler();
    gpEacup->Init();
    gpEacup->AddMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_ADDED, false, 0);
    gpEacup->AddMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_REMOVED, false, 0);

    // fetch and add user to netconn
    EA::User::UserList eacupUserList = gpEacup->GetUsers();
    EA::User::UserList::iterator i;

    for (i = eacupUserList.begin(); i != eacupUserList.end(); i++)
    {
        const EA::User::IEAUser *pLocalUser = *i;
        pLocalUser->AddRef();
        gpEAmsghdlr->AddLocalUser(pLocalUser);
    }

    if (netConnErr < 0)
        printf("NX network initialization error: %d", netConnErr);
}

void tickPlatform()
{
    gpEasmd->Tick();
    NetConnIdle();
    gpEacup->Tick();
}

void finalizePlatform()
{
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

    EA::SEMD::SystemEventMessageDispatcher::GetInstance()->DestroyInstance();
}

} // namespace Ignition

extern "C" void nninitStartup()
{
    const size_t ApplicationMemorySize = 1024 * 1024 * 1024;
    const size_t MallocMemorySize = 512 * 1024 * 1024;
    nn::Result result = nn::os::SetMemoryHeapSize(ApplicationMemorySize);
    NN_ABORT_UNLESS_RESULT_SUCCESS(result);
    uintptr_t address;
    result = nn::os::AllocateMemoryBlock(&address, MallocMemorySize);
    NN_ABORT_UNLESS_RESULT_SUCCESS(result);
    nn::init::InitializeAllocator(reinterpret_cast<void*>(address), MallocMemorySize);
}

#endif // EA_PLATFORM_NX
