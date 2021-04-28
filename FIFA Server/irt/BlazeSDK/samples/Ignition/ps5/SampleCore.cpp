
#include "PyroSDK/pyrosdk.h"

#ifdef EA_PLATFORM_PS5

#include "Ignition/ps5/SampleCore.h"
#include "Ignition/IgnitionDefines.h"

#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/netconn.h"

#include <EASystemEventMessageDispatcher/SystemEventMessageDispatcher.h>
#include "EAControllerUserPairing/EAControllerUserPairingServer.h"
#include "EAControllerUserPairing/Messages.h"

#include <np/np_common.h> //for SCE_NP_TITLE_SECRET_SIZE in initializePlatform
#include <stdlib.h>
#include <net.h>
#include <libsysmodule.h>
#include <error_dialog.h>
#include <sce_random.h>
#include <NucleusSDK/nucleussdk.h>
#include "GameIntentsSample.h"

// Set default heap size
size_t sceLibcHeapSize = SCE_LIBC_HEAP_SIZE_EXTENDED_ALLOC_NO_LIMIT;
unsigned int sceLibcHeapExtendedAlloc = 1; // Enable

namespace Ignition
{
    class IgnitionIEAMessageHandler : public EA::Messaging::IHandler
    {
    public:
        IgnitionIEAMessageHandler()
        {
            NetPrintf(("EACUP event handler: initializing\n"));
            memset(aLocalUsers, 0, sizeof(aLocalUsers));
        }

        bool HandleMessage(EA::Messaging::MessageId messageId, void* pMessage) override
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
        static const int32_t iMaxLocalUsers = 16;
        const EA::User::IEAUser *aLocalUsers[iMaxLocalUsers];
    };
    EA::Pairing::EAControllerUserPairingServer *gpEacup;
    EA::SEMD::SystemEventMessageDispatcher *gpEasmd;
    IgnitionIEAMessageHandler *gpEAmsghdlr;


    // Sample Product BlazeSDK2020ps5 (https://p.siedev.net/products/147030, NPWR19527_00, serviceLabel 0) (North America SKU):
    #define NP_TITLE_SECRET_HEX_NA_PS5 "86c7b5f135d2056d166d2dd81aa90c2231cd4944bb62e08e76898309c2ac3fb2d1a321415496541e0d15f1b28d113ddb7ce41a1efebd22c88adc7139253c8fa796958294a1428e5d748c4ce9ffa7113f2e83b2f6ae5f32def70fc7c2859cbfcb8111a04cf7e79a1a118759faa1008279b4f554b75a57336172eeb3080479da38"
    #define NP_TITLE_ID_NA_PS5         "NPXX53573_00"

    // Sample Product BlazeSDK3000ps5 (https://p.siedev.net/products/147840, NPWR19958_00, serviceLabel 1) (North America SKU)
    #define NP_TITLE_SECRET_HEX_NA_PS5_2 "aa995185b797da3440d94da3b4a3875f4a5952550056bdaeb8cc2fe6c27623d9551a01bad9615f1f91ddfebbe9d3adb218ab6c93db7858e8b4cf27d46f251216e72a88336b2ad979eecd3912f44a89556d017734f1511c94d354da70b75b60044431a7986a9b9d417d586332c88bf0fcec1b97d7fc71f267c74f1d6f46c61dcf"
    #define NP_TITLE_ID_NA_PS5_2       "NPXX53638_00"

    // To test other title ids update these:
    //#define NP_TITLE_SECRET_HEX NP_TITLE_SECRET_HEX_NA_PS5
    //#define NP_TITLE_ID         NP_TITLE_ID_NA_PS5
    #define NP_TITLE_SECRET_HEX NP_TITLE_SECRET_HEX_NA_PS5_2
    #define NP_TITLE_ID         NP_TITLE_ID_NA_PS5_2


    // helpers (see PS4 SDK sample's api_common_dialog
    uint8_t hexCharToUint(char ch)
    {
        uint8_t val = 0;
        if ( isdigit(ch) )
            val  = (ch - '0');
        else if ( isupper(ch) )
            val  = (ch - 'A' + 10);
        else
            val  = (ch - 'a' + 10);
        return val;
    }

    void hexStrToBin(const char *pHexStr, uint8_t *pBinBuf, size_t binBufSize)
    {
        uint8_t val = 0;
        int hexStrLen = strlen(pHexStr);
        int binOffset = 0;
        for (int i = 0; i < hexStrLen; i++)
        {
            val |= hexCharToUint(*(pHexStr + i));
            if (i % 2 == 0)
            {
                val <<= 4;
            }
            else
            {
                if (pBinBuf != nullptr && binOffset < binBufSize)
                {
                    memcpy(pBinBuf + binOffset, &val, 1);
                    val = 0;
                }
                binOffset++;
            }
        }
        if (val != 0 && pBinBuf != nullptr && binOffset < binBufSize)
            memcpy(pBinBuf + binOffset, &val, 1);
    }

//Use to handle Sony Error Code according to RX034
void errorHandler(int32_t iError)
{
    int32_t iRet = 0;
    SceErrorDialogParam param;
    sceErrorDialogParamInitialize( &param );
    param.errorCode = iError;

    if ((iRet = sceErrorDialogInitialize()) != SCE_OK ) 
    {
        if (iRet != SCE_ERROR_DIALOG_ERROR_ALREADY_INITIALIZED)
        {
            printf("Fail to initialize Error Dialog\n");
            return;
        }
    }

    sceErrorDialogOpen(&param);
}

void loadSysUtils()
{
    int32_t iResult;

    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_VOICE)) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleLoadModule(SCE_SYSMODULE_VOICE) failed (err=0x%x)\n", iResult));
    }
    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_NP_AUTH)) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleLoadModule(SCE_SYSMODULE_NP_AUTH) failed (err=0x%x)\n", iResult));
    }
    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_PLAYER_INVITATION_DIALOG)) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleLoadModule(SCE_SYSMODULE_PLAYER_INVITATION_DIALOG) failed (err=0x%x)\n", iResult));
    }
    if (sceSysmoduleLoadModule(SCE_SYSMODULE_ERROR_DIALOG) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleLoadModule(SCE_SYSMODULE_ERROR_DIALOG) failed (err=0x%x)\n", iResult));
    }
    if (sceSysmoduleLoadModule(SCE_SYSMODULE_APP_CONTENT) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleLoadModule(SCE_SYSMODULE_APP_CONTENT) failed (err=0x%x)\n", iResult));
    }
    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_RANDOM)) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleLoadModule(SCE_SYSMODULE_RANDOM) failed (err=0x%x)\n", iResult));
    }
}

void unloadSysUtils()
{
    int32_t iResult;
    if ((iResult = sceSysmoduleUnloadModule(SCE_SYSMODULE_VOICE)) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleUnloadModule(SCE_SYSMODULE_VOICE) failed (err=0x%x)\n", iResult));
    }
    if ((iResult = sceSysmoduleUnloadModule(SCE_SYSMODULE_NP_AUTH)) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleUnloadModule(SCE_SYSMODULE_NP_AUTH) failed (err=0x%x)\n", iResult));
    }
    if ((iResult = sceSysmoduleUnloadModule(SCE_SYSMODULE_PLAYER_INVITATION_DIALOG)) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleUnloadModule(SCE_SYSMODULE_PLAYER_INVITATION_DIALOG) failed (err=0x%x)\n", iResult));
    }
    if (sceSysmoduleUnloadModule(SCE_SYSMODULE_ERROR_DIALOG) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleLoadModule(SCE_SYSMODULE_ERROR_DIALOG) failed (err=0x%x)\n", iResult));
    }
    if (sceSysmoduleUnloadModule(SCE_SYSMODULE_APP_CONTENT) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleLoadModule(SCE_SYSMODULE_APP_CONTENT) failed (err=0x%x)\n", iResult));
    }
    if (sceSysmoduleUnloadModule(SCE_SYSMODULE_RANDOM) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleLoadModule(SCE_SYSMODULE_RANDOM) failed (err=0x%x)\n", iResult));
    }
}

void initializePlatform()
{
    //setup eatech event dispatcher
    EA::SEMD::SystemEventMessageDispatcherSettings semdSettings;
    semdSettings.useCompanionHttpd = false;
    semdSettings.useCompanionUtil = false;
    semdSettings.useSystemService = true;
    semdSettings.useUserService = true;
    EA::SEMD::SystemEventMessageDispatcher::CreateInstance(semdSettings, &gsAllocator);

    loadSysUtils();

    DirtyErrAppCallbackSet(&errorHandler);

    eastl::string netConnStartupParams;
    netConnStartupParams.sprintf("-servicename=ignition -titleid=%s", NP_TITLE_ID);
    int32_t netConnErr = 0;//declared on own line 1st to avoid 'unused netConnErr' opt build warn
    netConnErr = NetConnStartup(netConnStartupParams.c_str());
    NetPrintf(("Ignition NetConnStartUp %d\n", netConnErr));


    // initialize sce user service
    SceUserServiceInitializeParams sceUserServiceInitParams;
    memset(&sceUserServiceInitParams, 0, sizeof(sceUserServiceInitParams));
    sceUserServiceInitParams.priority = SCE_KERNEL_PRIO_FIFO_DEFAULT;
    sceUserServiceInitialize(&sceUserServiceInitParams);

    // initialize sce game intents
    Ignition::GameIntentsSample::init();

    // instantiate EACUP server
    gpEasmd = EA::SEMD::SystemEventMessageDispatcher::GetInstance();
    gpEacup = new EA::Pairing::EAControllerUserPairingServer(gpEasmd, &gsAllocator);
    gpEAmsghdlr = new IgnitionIEAMessageHandler();
    gpEacup->Init();
    gpEacup->AddMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_ADDED, false, 0);
    gpEacup->AddMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_REMOVED, false, 0);
    EA::User::UserList eacupUserList = gpEacup->GetUsers();

    // tell netconn about the first set of users
    EA::User::UserList::iterator i;
    for (i = eacupUserList.begin(); i != eacupUserList.end(); i++)
    {
        const EA::User::IEAUser *pLocalUser = *i;
        pLocalUser->AddRef();
        gpEAmsghdlr->AddLocalUser(pLocalUser);
        NetPrintf(("User added: %s\n", pLocalUser->GetGamerTag().c_str()));
    }

    uint8_t titleSecret[SCE_NP_TITLE_SECRET_SIZE];
    memset( titleSecret, 0, sizeof( titleSecret ) );
    hexStrToBin( NP_TITLE_SECRET_HEX, titleSecret, sizeof( titleSecret ) );

    NetConnControl('npsc', 128, 0, (void *)titleSecret, nullptr);
    NetPrintf(("Ps5 network initialization: %d\n", netConnErr));

    // give life to NetConn
    NetConnIdle();
}

void tickPlatform()
{
    NetConnIdle();
    gpEasmd->Tick();
    gpEacup->Tick();
    Ignition::GameIntentsSample::idle();
}

void finalizePlatform()
{
    // terminate sce game intent. Do this before 
    // NetConnShutdown() to ensure destroy web api called first.
    Ignition::GameIntentsSample::shutdown();

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

    // terminate sce user service
    sceUserServiceTerminate();

    sceErrorDialogTerminate();
    
    unloadSysUtils();
}

}

#endif // EA_PLATFORM_PS5
