
#include "NisBase.h"

#include "coreallocator/icoreallocator.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "EAStdC/EASprintf.h"

#ifdef EA_PLATFORM_KETTLE
// Set default heap size
size_t sceLibcHeapSize = SCE_LIBC_HEAP_SIZE_EXTENDED_ALLOC_NO_LIMIT;
unsigned int sceLibcHeapExtendedAlloc = 1; // Enable
#endif

#if defined(EA_PLATFORM_NX)
#include <nn/init.h>
#include <nn/os.h>
#endif

#if defined(BLAZESAMPLE_EASEMD_ENABLED)
#include "EASystemEventMessageDispatcher/SystemEventMessageDispatcher.h"
#endif

#ifdef BLAZESAMPLE_EACUP_ENABLED
#include "EAControllerUserPairing/EAControllerUserPairingServer.h"
#include "EAControllerUserPairing/Messages.h"
#endif

namespace NonInteractiveSamples
{

    class NisAllocator
        : public EA::Allocator::ICoreAllocator
    {
    public:
        virtual void *Alloc(size_t size, const char *name, unsigned int flags) { return malloc(size); }
        virtual void *Alloc(size_t size, const char *name, unsigned int flags, unsigned int align, unsigned int alignoffset) { return malloc(size); }
        virtual void *AllocDebug(size_t size, const DebugParams debugParams, unsigned int flags) { return malloc(size); }
        virtual void *AllocDebug(size_t size, const DebugParams debugParams, unsigned int flags, unsigned int align, unsigned int alignOffset) { return malloc(size); }
        virtual void Free(void *ptr, size_t) { free(ptr); }
    };

#if defined(BLAZESAMPLE_EACUP_ENABLED) && !defined(EA_PLATFORM_XBOXONE)
    class NisIEAMessageHandler : public EA::Messaging::IHandler
    {
    public:
        NisIEAMessageHandler()
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
    NisIEAMessageHandler *gpEAmsghdlr;
#endif  // defined(BLAZESAMPLE_EACUP_ENABLED) && !defined(EA_PLATFORM_XBOXONE)

    NisAllocator gsNisAllocator;

    NisBase::NisBase()
        : Pyro::UiBuilder("NisBase"),
        mBlazeHub(nullptr),
        mRunningNis(nullptr),
        mKillRunningNis(false),
        mActionConnectAndLogin(nullptr)
    {
        setText("Non-Interactive Samples");

        strcpy(mInitParameters.ServiceName, "blazesamples-latest-pc");
        strncpy(mInitParameters.ClientName, "Blaze Sample", Blaze::CLIENT_NAME_MAX_LENGTH - 1);
        strncpy(mInitParameters.ClientVersion, "0.1", Blaze::CLIENT_VERSION_MAX_LENGTH - 1);
        strncpy(mInitParameters.ClientSkuId, "BlazeSampleSku", Blaze::CLIENT_SKU_ID_MAX_LENGTH - 1);
        mInitParameters.Environment = Blaze::ENVIRONMENT_SDEV;
        mInitParameters.Secure = false;

        strcpy(mEmail, "nexus001@ea.com");
        strcpy(mPassword, "nexus");
        strcpy(mPersonaName, "nexus001");

        onStarting();
    }

    NisBase::~NisBase()
    {
        delete mNisCensus;
        delete mNisAssociationList;
        delete mNisGameBrowsing;
        delete mNisEntitlements;
        delete mNisGameReporting;
        delete mNisGeoIp;
        delete mNisJoinGame;
        delete mNisLeaderboardList;
        delete mNisLeaderboards;
        delete mNisMessaging;
        delete mNisQos;
        delete mNisServerConfig;
        delete mNisSmallUserStorage;
        delete mNisStats;
        delete mNisTelemetry;
    }

    void NisBase::onStarting()
    {
        mActionConnectAndLogin = &getActions().add("Connect and Login", "Connect and login to a Blaze server", this, &NisBase::actionConnectAndLogin);
        mActionConnectAndLogin->getParameters().addString("serviceName", mInitParameters.ServiceName, "Service Name");
        mActionConnectAndLogin->getParameters().addEnum("environmentType", mInitParameters.Environment, "Environment Type");
        mActionConnectAndLogin->getParameters().addString("email", mEmail, "Email");
        mActionConnectAndLogin->getParameters().addString("password", mPassword, "Password");
        mActionConnectAndLogin->getParameters().addString("personaName", mPersonaName, "Persona Name");

        Pyro::UiNodeAction *action;

        mNisAssociationList = new NisAssociationList(*this);
        action = &getActions().add("Run Association List Sample", "Association List non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisAssociationList);

        mNisCensus = new NisCensus(*this);
        action = &getActions().add("Run Census Sample", "Census non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisCensus);

        mNisGameBrowsing = new NisGameBrowsing(*this);
        action = &getActions().add("Run Game Browsing Sample", "Game Browsing non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisGameBrowsing);

        mNisEntitlements = new NisEntitlements(*this);
        action = &getActions().add("Run Entitlements Sample", "Entitlements non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisEntitlements);

        mNisGameReporting = new NisGameReporting(*this);
        action = &getActions().add("Run Game Reporting Sample", "Game Reporting non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisGameReporting);

        mNisGeoIp = new NisGeoIp(*this);
        action = &getActions().add("Run Geo Ip Sample", "Geo Ip non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisGeoIp);

        mNisJoinGame = new NisJoinGame(*this);
        action = &getActions().add("Run Join Game Sample", "Join Game non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisJoinGame);

        mNisLeaderboardList = new NisLeaderboardList(*this);
        action = &getActions().add("Run Leaderboard List Sample", "Leaderboard List non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisLeaderboardList);

        mNisLeaderboards = new NisLeaderboards(*this);
        action = &getActions().add("Run Leaderboards Sample", "Leaderboards non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisLeaderboards);

        mNisMessaging = new NisMessaging(*this);
        action = &getActions().add("Run Messaging Sample", "Messaging non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisMessaging);

        mNisQos = new NisQos(*this);
        action = &getActions().add("Run Qos Sample", "Qos non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisQos);

        mNisServerConfig = new NisServerConfig(*this);
        action = &getActions().add("Run Server Config Sample", "Server Config non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisServerConfig);

        mNisSmallUserStorage = new NisSmallUserStorage(*this);
        action = &getActions().add("Run Small User Storage Sample", "Small User Storage non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisSmallUserStorage);

        mNisStats = new NisStats(*this);
        action = &getActions().add("Run Stats Sample", "Stats non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisStats);

        mNisTelemetry = new NisTelemetry(*this);
        action = &getActions().add("Run Telemetry Sample", "Telemetry non-interactive sample", this, &NisBase::actionRunNisSample);
        action->setContextObject((NisSample *)mNisTelemetry);
    }

    void NisBase::onStopping()
    {
    }

    void NisBase::onIdle()
    {
        if (mBlazeHub != nullptr)
        {
            NetConnIdle();
            mBlazeHub->idle();
        }

        if (mKillRunningNis)
        {
            mKillRunningNis = false;

        getUiDriver()->addLog(Pyro::ErrorLevel::NONE, "Finished running sample.");

            mRunningNis->onCleanup();
            mRunningNis = nullptr;

            getUiDriver()->addLog(Pyro::ErrorLevel::NONE, "Finished running sample.");
        }

        tickPlatform();
    }

    void NisBase::killRunningNis()
    {
        mKillRunningNis = true;
    }

    void NisBase::actionConnectAndLogin(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
    {
        strcpy(mInitParameters.ServiceName, parameters["serviceName"]);
        mInitParameters.Environment = parameters["environmentType"];

        strcpy(mEmail, parameters["email"]);
        strcpy(mPassword, parameters["password"]);
        strcpy(mPersonaName, parameters["personaName"]);

        Blaze::BlazeError error = Blaze::BlazeHub::initialize(&mBlazeHub, mInitParameters, &gsNisAllocator, NisBase::BlazeSdkLogFunction, this);
        if (error == Blaze::ERR_OK)
        {
            mNetAdapter = new Blaze::BlazeNetworkAdapter::ConnApiAdapter();

            Blaze::GameManager::GameManagerAPI::GameManagerApiParams gameManagerParams(mNetAdapter);
            Blaze::GameManager::GameManagerAPI::createAPI(*mBlazeHub, gameManagerParams);

            Blaze::CensusData::CensusDataAPI::createAPI(*mBlazeHub);

            Blaze::Association::AssociationListAPI::AssociationListApiParams params;
            params.mMaxLocalAssociationLists = 100;
            Blaze::Association::AssociationListAPI::createAPI(*mBlazeHub, params);

            Blaze::Stats::StatsAPI::createAPI(*mBlazeHub);

            Blaze::Messaging::MessagingAPI::createAPI(*mBlazeHub);

            Blaze::Util::UtilAPI::createAPI(*mBlazeHub);

            if (mRunningNis != nullptr)
            {
                mRunningNis->onCreateApis();
            }

            mBlazeHub->addUserStateEventHandler(this);

            mBlazeHub->getConnectionManager()->connect(
                Blaze::ConnectionManager::ConnectionManager::ConnectionMessagesCb(this, &NisBase::connectionMessagesCb),
                false);
        }
    }

    void NisBase::actionRunNisSample(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
    {
        if (mRunningNis == nullptr)
        {
            mRunningNis = (NisSample *)action->getContextObject();

            if (mBlazeHub == nullptr)
            {
                getUiDriver()->requestFeedback(*mActionConnectAndLogin);
            }
            else
            {
                mRunningNis->onCreateApis();
                mRunningNis->onRun();
            }
        }
    }

    void NisBase::connectionMessagesCb(Blaze::BlazeError blazeError, const Blaze::Redirector::DisplayMessageList *displayMessageList)
    {
        for (Blaze::Redirector::DisplayMessageList::const_iterator i = displayMessageList->begin();
            i != displayMessageList->end();
            i++)
        {
            getUiDriver()->addLog_va("%s", (*i).c_str());
        }

        const Blaze::ConnectionManager::ConnectionStatus& connStatus = mBlazeHub->getConnectionManager()->getConnectionStatus();
        uint32_t uConnStatus = (uint32_t)(connStatus.mNetConnStatus);

        getUiDriver()->addLog_va("ExtendedError=%s", mBlazeHub->getErrorName(connStatus.mError));
        getUiDriver()->addLog_va("NetConnStatus=%c%c%c%c", (uConnStatus >> 24) & 0xff, (uConnStatus >> 16) & 0xff, (uConnStatus >> 8) & 0xff, (uConnStatus >> 0) & 0xff);
        getUiDriver()->addLog_va("ProtoSSLError=%d", connStatus.mProtoSslError);
        getUiDriver()->addLog_va("SocketError=%d", connStatus.mSocketError);
    }

    void NisBase::BlazeSdkLogFunction(const char8_t *pText, void *data)
    {
        NisBase *nisBase = (NisBase *)data;

        nisBase->getUiDriver()->addLog(pText);
    }

    void NisBase::onConnected()
    {
        Blaze::Authentication::AuthenticationComponent *auth = mBlazeHub->getComponentManager()->getAuthenticationComponent();

        Blaze::Authentication::ExpressLoginRequest request;
        request.setEmail(mEmail);
        request.setPassword(mPassword);
        request.setPersonaName(mPersonaName);

        auth->expressLogin(request);
    }

    void NisBase::onAuthenticated(uint32_t userIndex)
    {
        if (mRunningNis != nullptr)
        {
            mRunningNis->onRun();
        }

    }


    static Pyro::PyroFramework gPyroFramework;
    //static Pyro::InternalProxyServer gInternalProxyServer;

    class NisUiDriver
        : public Pyro::UiDriverMaster
    {
    public:
        NisUiDriver()
            : Pyro::UiDriverMaster()
        {
            setText("Non-Interactive Samples");

            mNisBase = new NisBase();
            getUiBuilders().add(mNisBase);
        }

        virtual void onIdle()
        {
            if ((mNisBase != nullptr) && (mNisBase->getBlazeHub() != nullptr))
            {
                NetConnIdle();
                mNisBase->getBlazeHub()->idle();

                mNisBase->onIdle();
            }

            tickPlatform();
        }

    private:
        NisBasePtr mNisBase;
    };

    void initializePlatform()
    {
#if defined(BLAZESAMPLE_EASEMD_ENABLED)
        // instantiate system event message dispatcher
        EA::SEMD::SystemEventMessageDispatcherSettings semdSettings;
        semdSettings.useCompanionHttpd = false;
        semdSettings.useCompanionUtil = false;
        semdSettings.useSystemService = true;
        semdSettings.useUserService = true;
        EA::SEMD::SystemEventMessageDispatcher::CreateInstance(semdSettings, &gsNisAllocator);
#endif

#if defined(BLAZESAMPLE_EACUP_ENABLED) && !defined(EA_PLATFORM_XBOXONE)

#ifdef EA_PLATFORM_KETTLE
        // initialize sce user service
        SceUserServiceInitializeParams sceUserServiceInitParams;
        memset(&sceUserServiceInitParams, 0, sizeof(sceUserServiceInitParams));
        sceUserServiceInitParams.priority = SCE_KERNEL_PRIO_FIFO_DEFAULT;
        sceUserServiceInitialize(&sceUserServiceInitParams);
#endif

        // instantiate EACUP server
        gpEacup = new EA::Pairing::EAControllerUserPairingServer(EA::SEMD::SystemEventMessageDispatcher::GetInstance(), &gsNisAllocator);
        gpEAmsghdlr = new NisIEAMessageHandler();
        gpEacup->Init();
        gpEacup->AddMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_ADDED, false, 0);
        gpEacup->AddMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_REMOVED, false, 0);
        EA::User::UserList eacupUserList = gpEacup->GetUsers();
#endif

        NetConnStartup("-servicename=nis");

#if defined(BLAZESAMPLE_EACUP_ENABLED) && !defined(EA_PLATFORM_XBOXONE)
        // tell netconn about the first set of users
        EA::User::UserList::iterator i;
        for (i = eacupUserList.begin(); i != eacupUserList.end(); i++)
        {
            const EA::User::IEAUser *pLocalUser = *i;
            pLocalUser->AddRef();
            gpEAmsghdlr->AddLocalUser(pLocalUser);
        }
#endif
    }

    void tickPlatform()
    {
#if defined(BLAZESAMPLE_EACUP_ENABLED) && !defined(EA_PLATFORM_XBOXONE)
        gpEacup->Tick();
#endif

#if defined(BLAZESAMPLE_EASEMD_ENABLED)
        EA::SEMD::SystemEventMessageDispatcher::GetInstance()->Tick();
#endif
    }

    void finalizePlatform()
    {
        NetConnShutdown(0);

#if defined(BLAZESAMPLE_EACUP_ENABLED) && !defined(EA_PLATFORM_XBOXONE)
        // free EACUP server
        gpEacup->RemoveMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_ADDED);
        gpEacup->RemoveMessageHandler(gpEAmsghdlr, EA::Pairing::E_USER_REMOVED);
        delete gpEAmsghdlr;
        gpEacup->Shutdown();
        delete gpEacup;

#ifdef EA_PLATFORM_KETTLE
        // terminate sce user service
        sceUserServiceTerminate();
#endif
#endif

#if defined(EA_PLATFORM_KETTLE) || defined(EA_PLATFORM_NX)
    EA::SEMD::SystemEventMessageDispatcher::GetInstance()->DestroyInstance();
#endif
    }

}

class NisProxyServerListener
    : public Pyro::ProxyServerListenerEx<NonInteractiveSamples::NisUiDriver>
{
public:
    NisProxyServerListener()
    {
        getInfo().setCaption("Non-Interactive Samples for Blaze");
        getInfo().setDescription("Provides sample code and functionality covering many aspects of Blaze");
        getInfo().setVersion(Blaze::getBlazeSdkVersionString());
        getInfo().setSupportsMultipleClients(false);
    }
} gNisProxyServerListener;

#ifdef NIS_PYINT
PYRO_DEFINE_INTERNAL_PROXY_EXPORTS(gNisProxyServerListener, NonInteractiveSamples::initializePlatform(), NonInteractiveSamples::finalizePlatform());
#else
#ifdef EA_PLATFORM_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    NonInteractiveSamples::initializePlatform();

    Pyro::PyroFramework pyro(&gNisProxyServerListener);
    pyro.run(lpCmdLine);

    NonInteractiveSamples::finalizePlatform();
}
#elif defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
#include "xboxone/SampleCore.h"
int main(Platform::Array<Platform::String^>^ args)
{
    auto xboxOneViewFactory = ref new NonInteractiveSamples::XboxOneViewFactory(gNisProxyServerListener);
    Windows::ApplicationModel::Core::CoreApplication::Run(xboxOneViewFactory);
}
#elif defined(EA_PLATFORM_XBOX_GDK)
int32_t APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, char *cmdline, int32_t show)
{
    // Gemini TODO
}
#elif defined(EA_PLATFORM_NX)
extern "C" void nnMain()
{
    NonInteractiveSamples::initializePlatform();

    Pyro::PyroFramework pyro(&gNisProxyServerListener);
    pyro.run(nn::os::GetHostArgc(), nn::os::GetHostArgv());

    NonInteractiveSamples::finalizePlatform();
}
#else
int main(int32_t argc, char8_t *argv[])
{
    NonInteractiveSamples::initializePlatform();

    Pyro::PyroFramework pyro(&gNisProxyServerListener);
    pyro.run(argc, argv);

    NonInteractiveSamples::finalizePlatform();
}
#endif
#endif

#ifndef BLAZESDK_DLL  // If this is not built with Dirtysock as a DLL... provide our own default DirtyMemAlloc/Free
//------------------------------------------------------------------------------
// DirtyMemAlloc and DirtyMemFree are required for DirtySock
void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    return malloc(static_cast<size_t>(iSize));
}

//------------------------------------------------------------------------------
void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    free(pMem);
}
#endif
//------------------------------------------------------------------------------
//EASTL New operator
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return new char[size];
}

//------------------------------------------------------------------------------
//EASTL New operator
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return new char[size];
}

//------------------------------------------------------------------------------
// A default allocator
class TestAllocator : public EA::Allocator::ICoreAllocator
{
    virtual void *Alloc(size_t size, const char * name, unsigned int flags) { return malloc(size); }
    virtual void *Alloc(size_t size, const char * name, unsigned int flags, unsigned int align, unsigned int alignoffset) { return malloc(size); }
    virtual void *AllocDebug(size_t size, const DebugParams debugParams, unsigned int flags) { return malloc(size); }
    virtual void *AllocDebug(size_t size, const DebugParams debugParams, unsigned int flags, unsigned int align, unsigned int alignOffset) { return malloc(size); }
    virtual void Free(void *ptr, size_t) { free(ptr); }
};

//------------------------------------------------------------------------------
ICOREALLOCATOR_INTERFACE_API EA::Allocator::ICoreAllocator *EA::Allocator::ICoreAllocator::GetDefaultAllocator()
{
    // NOTE: we can't just use another global allocator object, since we'd have an initialization race condition between the two globals.
    //   to avoid that, we use a static function var, which is init'd the first time the function is called.
    static TestAllocator sTestAllocator; // static function members are only constructed on first function call
    return &sTestAllocator;
}

//Vsnprintf8 for eastl
int Vsnprintf8(char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments); //necessary for stupid codewarrior warning.
int Vsnprintf8(char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}
