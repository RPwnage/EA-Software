
#include "PyroSDK/pyrosdk.h"

#ifdef EA_PLATFORM_PS4

#include "Ignition/ps4/SampleCore.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include <EASystemEventMessageDispatcher/SystemEventMessageDispatcher.h>
#include "Ignition/IgnitionDefines.h"

#include "DirtySDK/dirtysock/netconn.h"
#include "EAControllerUserPairing/EAControllerUserPairingServer.h"
#include "EAControllerUserPairing/Messages.h"

#include <np/np_common.h> //for SCE_NP_TITLE_SECRET_SIZE in initializePlatform
#include <stdlib.h>
#include <net.h>
#include <libsysmodule.h>
#include <error_dialog.h>
#include <sce_random.h>
#if (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
#include "Ignition/ps5/GameIntentsSample.h"
#include <np_cg.h>
#include <player_invitation_dialog.h>
#endif

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
        static const int32_t iMaxLocalUsers = 16;
        const EA::User::IEAUser *aLocalUsers[iMaxLocalUsers];
    };
    EA::Pairing::EAControllerUserPairingServer *gpEacup;
    EA::SEMD::SystemEventMessageDispatcher *gpEasmd;
    IgnitionIEAMessageHandler *gpEAmsghdlr;

#if defined(EA_PLATFORM_PS4_CROSSGEN)
    // Product 'BlazeSDK3000ps4crossgen' (NA SKU) (https://p.siedev.net/products/153867#services/cross-gen_sdk, SessionManager(CommunicationId NPWR19958_00) linked to Identity S2S auth service CWAA03003_00(ServiceLabel 1))
    #define NP_TITLE_SECRET_HEX "92cc60997750ee25eeb4029aa3d76b20faa59c3e30be159486a5beec15c9b48f2023efb23c654f5824ed7537e4acda86b8b405f8343a7d79bff627e92d21170d05b9b6c4f487c1cb18e03e521f4f682f63ad1ae26d86dd670786f9cfcc31fa8d4598006fa610a07fd0b5bed5dbe8330188908865572b837b9594a582f0221cbc"
    #define NP_TITLE_ID         "NPXX53945_00"

    // Product 'BlazeSDK2020ps4' (NA SKU) (https://p.siedev.net/products/147029#services/cross-gen_sdk, SessionManager(CommunicationId NPWR19527_00) linked to Identity S2S auth service CWAA03003_00(ServiceLabel 0))
    //#define NP_TITLE_SECRET_HEX "9f7b7e0883564ff78cf7e1496e55f8be96eabb6a1e3c48faba9a327ba1549d3ba3d58bb961b5fb350e5863bc9db08d4b8856f37a5f35e8e20d650eaef1ac3d9824afe55a1f59d6749c5350a66c661ff0350adce9b102b86baaa99e90da50e513ced01641af4127880acde38c4647ce89690cbe6cead571c090449ca810a0bf52"
    //#define NP_TITLE_ID         "NPXX53572_00"
#else
    // Product BlazeSDK2015         (https://ps4.siedev.net/products/126097) shares session/invitation(CommunicationId NPWR04673_00) linked to Identity CWAA00059_00(serviceLabel 0)
    #define NP_TITLE_SECRET_HEX "538d8144fbd97dc19a1602881d5e95470dc563972788c79a2a4bb5a356911ec8647c6b4eedfd4d140ce91699b8ad5522eb8bb296cd96a431b93de340a40d04fbbe3e96a86ef11a7b5bcab3c30d4dbfb19543d36981d0bbf620721090e3abd53b13a4232d8576c23166b48f1f5dd5d32107b40df630c5998eb37dae5f0a73469a"
    #define NP_TITLE_ID         "NPXX52039_00"

    // TitleID BlazeSample2016_BlazeSample2Client shares session/invitation with CWAA00059_00 for BlazeServer s2s service lable 0
    //#define NP_TITLE_SECRET_HEX "a52f8b571a50435e7c8f65f083d7328f36231062609ef428b8e7acb095ee81cd065b201ecac46b589198c118eedbe4f0c3f60546ba1dce1f77b155cdc30c251896810986a66f3084deb5bcfce8fae86f269003a8666ffa0f363f303a08d9de563dbc075c08e2c72bd34c260569169c666bb017f27ca6647743d576e7a5dbab84"
    //#define NP_TITLE_ID         "NPXX52038_00"

    // Product BlazeSDK2015_2 (https://ps4.siedev.net/titles/107691/products/126739):
    //#define NP_TITLE_SECRET_HEX "a37d28a09fa0264863f14ba324fce25ede4b4ddef21049fd828177b9e9b372c2320bcfc2a81a5f085c3ada1fbe1a38c085538fc7e450c0329c769f2f2de680c74d215cad1061266381555b65cab400d6216b798c5bc1165c3206a72593b06d9916088f4133ab0d707b344a248d5026416bea425ced44e4ef9071b102d3b04a6e"
    //#define NP_TITLE_ID         "NPXX52114_00"
#endif

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
        return;
    }

//Use to handle Sony Error Code according to TRC4034
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
#if (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_PLAYER_INVITATION_DIALOG)) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleLoadModule(SCE_SYSMODULE_PLAYER_INVITATION_DIALOG) failed (err=0x%x)\n", iResult));
    }
#else
    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_INVITATION_DIALOG)) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleLoadModule(SCE_SYSMODULE_INVITATION_DIALOG) failed (err=0x%x)\n", iResult));
    }
#endif
    if ( sceSysmoduleLoadModule(SCE_SYSMODULE_ERROR_DIALOG) != SCE_OK ) 
    {
        NetPrintf(("sceSysmoduleLoadModule(SCE_SYSMODULE_ERROR_DIALOG) failed (err=0x%x)\n", iResult));
    }
    if ( sceSysmoduleLoadModule(SCE_SYSMODULE_APP_CONTENT) != SCE_OK ) 
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
#if (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
    if ((iResult = sceSysmoduleUnloadModule(SCE_SYSMODULE_PLAYER_INVITATION_DIALOG)) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleUnloadModule(SCE_SYSMODULE_PLAYER_INVITATION_DIALOG) failed (err=0x%x)\n", iResult));
    }
#else
    if ((iResult = sceSysmoduleUnloadModule(SCE_SYSMODULE_INVITATION_DIALOG)) != SCE_OK)
    {
        NetPrintf(("sceSysmoduleUnloadModule(SCE_SYSMODULE_INVITATION_DIALOG) failed (err=0x%x)\n", iResult));
    }
#endif
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

#if (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
    // initialize sce game intents
    Ignition::GameIntentsSample::init();
#endif

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
    for(i = eacupUserList.begin(); i != eacupUserList.end(); i++)
    {
        const EA::User::IEAUser *pLocalUser = *i;
        pLocalUser->AddRef();
        gpEAmsghdlr->AddLocalUser(pLocalUser);
    }
    uint8_t titleSecret[SCE_NP_TITLE_SECRET_SIZE];
    memset( titleSecret, 0, sizeof( titleSecret ) );
    hexStrToBin( NP_TITLE_SECRET_HEX, titleSecret, sizeof( titleSecret ) );

    NetConnControl('npsc', 128, 0, (void *)titleSecret, nullptr);
    NetPrintf(("PS4 network initialization: %d\n", netConnErr));
}

void tickPlatform()
{
    gpEasmd->Tick();
    gpEacup->Tick();
#if (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
    Ignition::GameIntentsSample::idle();
#endif
}

void finalizePlatform()
{
#if (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
    // terminate sce game intent. Do this before 
    // NetConnShutdown() to ensure destroy web api called first.
    Ignition::GameIntentsSample::shutdown();
#endif

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

    EA::SEMD::SystemEventMessageDispatcher::GetInstance()->DestroyInstance();

    sceErrorDialogTerminate();

    unloadSysUtils();
}

}

#endif // EA_PLATFORM_PS4
