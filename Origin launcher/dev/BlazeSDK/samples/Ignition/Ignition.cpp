
#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"
#include "Ignition/Connection.h"
#include "Ignition/ByteVault.h"
#if !defined(EA_PLATFORM_NX)
#include "Ignition/voip/VoipConfig.h"
#endif
#include "Ignition/LocalUserConfig.h"
#include "Ignition/custom/dirtysockdev.h"
#include "Ignition/Example.h"
#include "Ignition/Census.h"
#include "Ignition/GameManagement.h"
#include "Ignition/Stats.h"
#include "Ignition/Leaderboards.h"
#include "Ignition/Clubs.h"
#include "Ignition/LoginAndAccountCreation.h"
#include "Ignition/AssociationLists.h"
#include "Ignition/Achievements.h"
#include "Ignition/Friends.h"
#include "Ignition/UserExtendedData.h"
#include "Ignition/UserManager.h"
#include "Ignition/PlatformFeatures.h"

#ifdef EA_PLATFORM_PS4
#include "Ignition/ps4/render.h"
#include "EASystemEventMessageDispatcher/SystemEventMessageDispatcher.h"
#endif

#ifdef EA_PLATFORM_PS5
#include "Ignition/ps5/render.h"
#endif

#ifdef EA_PLATFORM_NX
#include "Ignition/nx/render.h"
#include <nn/init.h>
#include <nn/os.h>

#include <nn/account/account_Api.h>
#include <nn/account/account_ApiForApplications.h>
#include <nn/account/account_NintendoAccountAuthorizationRequest.h>
#include <nn/account/account_Result.h>
#include <nn/account/account_Selector.h>
#include <nn/nifm/nifm_Api.h>
#include <nn/nifm/nifm_ApiNetworkConnection.h>

#endif

namespace Ignition
{

Allocator gsAllocator;

Blaze::BlazeHub *gBlazeHub;
Blaze::BlazeNetworkAdapter::ConnApiAdapter *gConnApiAdapter;
Transmission2 gTransmission2;
#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_NX)
T2Render gRender;
#endif

BlazeHubUiDriver *gBlazeHubUiDriver = nullptr;

eadp::foundation::SharedPtr<eadp::foundation::IHub> gEadpHub = nullptr;

// provide our own default DirtyMemAlloc/Free
//------------------------------------------------------------------------------
// DirtyMemAlloc and DirtyMemFree are required for DirtySock
#ifdef BLAZESDK_DLL

void *DirtyMemAllocDll(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    return malloc(static_cast<size_t>(iSize));
}

//------------------------------------------------------------------------------
void DirtyMemFreeDll(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    free(pMem);
}

#endif

#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_ANDROID) || defined(EA_PLATFORM_IPHONE) || defined(EA_PLATFORM_OSX) || (defined(EA_PLATFORM_LINUX) && !defined(EA_PLATFORM_STADIA))
    void initializePlatform()
    {
        uint32_t uConnStatus;
        int32_t iCounter, netConnErr;

        // start up network interface
#ifdef BLAZESDK_DLL

        //Providing  provide your own DirtyMemFree and DirtyMemAlloc override with DirtyMemSetFunc()
        DirtyMemFuncSet(&DirtyMemAllocDll, &DirtyMemFreeDll);

#endif
        netConnErr = NetConnStartup("-servicename=ignition");
        if (netConnErr < 0)
        {
            return;
        }

        NetConnConnect(nullptr, nullptr, 0);

        for (iCounter = 0; ; iCounter++)
        {
            uConnStatus = (uint32_t)NetConnStatus('conn', 0, nullptr, 0);
            if (uConnStatus == '+onl')
            {
                NetPrintf(("samplecore: connected to internet.\n"));
                break;
            }
            else if (uConnStatus >> 24 == '-')
            {
                NetPrintf(("samplecore: error '%C' connecting to internet\n", uConnStatus));
                break;
            }

            NetConnIdle();
            NetConnSleep(100);
        }

    }

    void tickPlatform() { }

    void finalizePlatform()
    {
        while (NetConnShutdown(0) == -1)
        {
            NetConnSleep(16);
        }
    }
#elif defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
    void initializePlatform()
    {
        int32_t netConnErr = NetConnStartup("-mixedsecure");
        if (netConnErr < 0)
        {
            //getUiDriver()->addLog_va(Pyro::ErrorLevel::ERR, "NetConnStartup returned an error: %d", netConnErr);
            return;
        }
    }

    void tickPlatform() { }

    void finalizePlatform()
    {
    }
#else
    // Other platforms have additional initialization requirements and can be found
    // in the platform specific folders
#endif

BlazeHubUiDriver::BlazeHubUiDriver()
{
    EA_ASSERT(gBlazeHubUiDriver == nullptr);
    gBlazeHubUiDriver = this;

    setText("BlazeHub and Connection");
    setIconImage(Pyro::IconImage::CLASS);

    getMainValue("Blaze SDK Version") = Blaze::getBlazeSdkVersionString();

    mBlazeInitialization = new BlazeInitialization();
    getUiBuilders().add(mBlazeInitialization);
}

BlazeHubUiDriver::~BlazeHubUiDriver()
{
    EA_ASSERT(gBlazeHubUiDriver != nullptr);
    gBlazeHubUiDriver = nullptr;
}

void BlazeHubUiDriver::onBlazeHubInitialized()
{
    mLocalUserAuthenticationDispatchers = new LocalUserAuthenticationDispatcher[gBlazeHub->getInitParams().UserCount];

    addIgnitionUiBuilder(mConnection);
#if !defined(EA_PLATFORM_NX)
    addIgnitionUiBuilder(mVoipConfig);
#endif
    addIgnitionUiBuilder(mExample);
    addIgnitionUiBuilder(mCensus);
    addIgnitionUiBuilder(mGameManagement);
    addIgnitionUiBuilder(mStats);
    addIgnitionUiBuilder(mLeaderboards);
    addIgnitionUiBuilder(mClubs);
    addIgnitionUiBuilder(mDirtySockDev);
    addIgnitionUiBuilder(mLocalUserConfig);

    for (uint32_t userIndex = 0; userIndex < gBlazeHub->getInitParams().UserCount; ++userIndex)
    {
        LocalUserUiDriver *localUserUiDriver = new LocalUserUiDriver(userIndex);
        getSlaves().add(localUserUiDriver);
    }

    onDisconnected(Blaze::ERR_OK);

    gBlazeHub->addUserStateEventHandler(this);
    gBlazeHub->getUserManager()->addPrimaryUserListener(this);
}

void BlazeHubUiDriver::onBlazeHubFinalized()
{
    gBlazeHub->getUserManager()->removePrimaryUserListener(this);
    gBlazeHub->removeUserStateEventHandler(this);

    onDisconnected(Blaze::ERR_OK);

    getSlaves().clear();

    removeIgnitionUiBuilder(mConnection);
#if !defined(EA_PLATFORM_NX)
    removeIgnitionUiBuilder(mVoipConfig);
#endif
    removeIgnitionUiBuilder(mExample);
    removeIgnitionUiBuilder(mCensus);
    removeIgnitionUiBuilder(mGameManagement);
    removeIgnitionUiBuilder(mStats);
    removeIgnitionUiBuilder(mLeaderboards);
    removeIgnitionUiBuilder(mClubs);
    removeIgnitionUiBuilder(mDirtySockDev);
    removeIgnitionUiBuilder(mLocalUserConfig);

    delete[] mLocalUserAuthenticationDispatchers;
}

void BlazeHubUiDriver::addBlazeHubConnectionListener(BlazeHubConnectionListener *listener, bool callDisconnected)
{
    mBlazeHubConnectionDispatcher.addDispatchee(listener);
    if (callDisconnected)
        listener->onDisconnected();
}

void BlazeHubUiDriver::removeBlazeHubConnectionListener(BlazeHubConnectionListener *listener)
{
    mBlazeHubConnectionDispatcher.removeDispatchee(listener);
}

void BlazeHubUiDriver::addPrimaryLocalUserAuthenticationListener(PrimaryLocalUserAuthenticationListener *listener, bool callDeAuthenticated)
{
    mPrimaryLocalUserAuthenticationDispatcher.addDispatchee(listener);
    if (callDeAuthenticated)
        listener->onDeAuthenticatedPrimaryUser();
}

void BlazeHubUiDriver::removePrimaryLocalUserAuthenticationListener(PrimaryLocalUserAuthenticationListener *listener)
{
    mPrimaryLocalUserAuthenticationDispatcher.removeDispatchee(listener);
}

void BlazeHubUiDriver::addLocalUserAuthenticationListener(uint32_t userIndex, LocalUserAuthenticationListener *listener, bool doCallback)
{
    mLocalUserAuthenticationDispatchers[userIndex].addDispatchee(listener);
    if (doCallback)
    {
        Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(userIndex);
        if (localUser != nullptr)
            listener->onAuthenticated();
        else
            listener->onDeAuthenticated();
    }
}

void BlazeHubUiDriver::removeLocalUserAuthenticationListener(uint32_t userIndex, LocalUserAuthenticationListener *listener)
{
    mLocalUserAuthenticationDispatchers[userIndex].removeDispatchee(listener);
}

void BlazeHubUiDriver::addPrimaryLocalUserChangedListener(PrimaryLocalUserChangedListener *listener, bool doCallback)
{
    mPrimaryLocalUserChangedDispatchers.addDispatchee(listener);
    if (doCallback)
    {
        listener->onPrimaryLocalUserChanged(gBlazeHub->getUserManager()->getPrimaryLocalUserIndex());
    }
}

void BlazeHubUiDriver::removePrimaryLocalUserChangedListener(PrimaryLocalUserChangedListener *listener)
{
    mPrimaryLocalUserChangedDispatchers.removeDispatchee(listener);
}

void BlazeHubUiDriver::onConnected()
{
    mBlazeHubConnectionDispatcher.dispatch("onConnected", &BlazeHubConnectionListener::onConnected);

    Pyro::UiDriver::Enumerator localUserEnumerator(getSlaves());
    while (localUserEnumerator.moveNext())
    {
        localUserEnumerator.getCurrent()->setIsVisible(true);
    }
}

void BlazeHubUiDriver::onDisconnected(Blaze::BlazeError errorCode)
{
    for (uint32_t userIndex = 0; userIndex < gBlazeHub->getInitParams().UserCount; ++userIndex)
    {
        onDeAuthenticated(userIndex);
    }

    Pyro::UiDriver::Enumerator localUserEnumerator(getSlaves());
    while (localUserEnumerator.moveNext())
    {
        localUserEnumerator.getCurrent()->setIsVisible(false);
    }

    onPrimaryLocalUserDeAuthenticated(0);

    mBlazeHubConnectionDispatcher.dispatch("onDisconnected", &BlazeHubConnectionListener::onDisconnected);
}

void BlazeHubUiDriver::onAuthenticated(uint32_t userIndex)
{
    mLocalUserAuthenticationDispatchers[userIndex].dispatch("onAuthenticated", &LocalUserAuthenticationListener::onAuthenticated);
}

void BlazeHubUiDriver::onDeAuthenticated(uint32_t userIndex)
{
    mLocalUserAuthenticationDispatchers[userIndex].dispatch("onDeAuthenticated", &LocalUserAuthenticationListener::onDeAuthenticated);
}

void BlazeHubUiDriver::onForcedDeAuthenticated(uint32_t userIndex, Blaze::UserSessionForcedLogoutType forcedLogoutType)
{
    mLocalUserAuthenticationDispatchers[userIndex].dispatch("onForcedDeAuthenticated", &LocalUserAuthenticationListener::onForcedDeAuthenticated, forcedLogoutType);
}

void BlazeHubUiDriver::onPrimaryLocalUserChanged(uint32_t primaryUserIndex)
{
    mPrimaryLocalUserChangedDispatchers.dispatch("onPrimaryLocalUserChanged", &PrimaryLocalUserChangedListener::onPrimaryLocalUserChanged, primaryUserIndex);
}

void BlazeHubUiDriver::onPrimaryLocalUserAuthenticated(uint32_t primaryUserIndex)
{
    mPrimaryLocalUserAuthenticationDispatcher.dispatch("onAuthenticatedPrimaryUser", &PrimaryLocalUserAuthenticationListener::onAuthenticatedPrimaryUser);
}

void BlazeHubUiDriver::onPrimaryLocalUserDeAuthenticated(uint32_t primaryUserIndex)
{
    mPrimaryLocalUserAuthenticationDispatcher.dispatch("onDeAuthenticatedPrimaryUser", &PrimaryLocalUserAuthenticationListener::onDeAuthenticatedPrimaryUser);
}

void BlazeHubUiDriver::onInitialize()
{
}

void BlazeHubUiDriver::onFinalize()
{
    mBlazeInitialization->actionShutdownBlaze(nullptr, nullptr);
}

void BlazeHubUiDriver::onIdle()
{
    NetConnIdle();  // give time to DS as soon as we can, this can be useful for processes like upnp

    if (gBlazeHub != nullptr)
    {
        gBlazeHub->idle();
    }

    if (gEadpHub != nullptr)
    {
        gEadpHub->idle();
        gEadpHub->invokeCallbacks();
    }

#ifdef EA_PLATFORM_PS4
    //we wait till we have a connapiadapter so that the DirtySessionManager is around to receive an invite event
    //which may come in very early if accepting the invite from the xmb
    if(gConnApiAdapter != nullptr)
    {
        EA::SEMD::SystemEventMessageDispatcher::GetInstance()->Tick();
    }
#endif
}

void BlazeHubUiDriver::runSendMessageToGroup(uint32_t fromUserIndex, const Blaze::UserGroup *userGroup, const char8_t *text)
{
    Blaze::Messaging::SendMessageParameters sendMessageParameters;

    sendMessageParameters.setTargetGroup(userGroup);

    sendMessageParameters.getFlags().setEcho();
    sendMessageParameters.getFlags().setFilterProfanity();

    sendMessageParameters.getAttrMap().insert(
        eastl::make_pair(Blaze::Messaging::MSG_ATTR_SUBJECT, text));

    getUiDriver()->addDiagnosticCallFunc(gBlazeHub->getMessagingAPI(fromUserIndex)->sendMessage(sendMessageParameters,
        Blaze::Messaging::MessagingAPI::SendMessageCb(this, &BlazeHubUiDriver::SendMessageCb)));
}

void BlazeHubUiDriver::SendMessageCb(const Blaze::Messaging::SendMessageResponse *sendMessageResponse, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK("BlazeHubUiDriver::SendMessageCb");
    REPORT_BLAZE_ERROR(blazeError);
}

void BlazeHubUiDriver::reportBlazeError(Blaze::BlazeError blazeError, const char8_t *filename, const char8_t *functionName, int32_t lineNumber)
{
    if (blazeError != Blaze::ERR_OK)
    {
        getUiDriver()->addDiagnostic_va(
            filename, functionName, lineNumber, 1,
            Pyro::ErrorLevel::ERR, "BlazeSDK Error", "Error: %s (%d)",
            (gBlazeHub != nullptr ? gBlazeHub->getErrorName(blazeError) : ""), blazeError);

        getUiDriver()->showMessage_va(
            Pyro::ErrorLevel::ERR, "BlazeSDK Error", "Error: %s (%d)",
            (gBlazeHub != nullptr ? gBlazeHub->getErrorName(blazeError) : ""), blazeError);
    }
}

LocalUserUiDriver::LocalUserUiDriver(uint32_t userIndex)
    : Pyro::UiDriverSlave(eastl::string(eastl::string::CtorSprintf(), "%d", userIndex).c_str()),
      mUserIndex(userIndex)
{
    setIconImage(Pyro::IconImage::USER);
    setHideWhenAdded(true);

    addLocalUserUiBuilder(mLoginAndAccountCreation);
    addLocalUserUiBuilder(mUserManager);
    addLocalUserUiBuilder(mAssociationLists);
    addLocalUserUiBuilder(mMessaging);
    addLocalUserUiBuilder(mAchievements);
    addLocalUserUiBuilder(mFriends);
    addLocalUserUiBuilder(mUserExtendedData);
    addLocalUserUiBuilder(mPlatformFeatures);
    addLocalUserUiBuilder(mByteVault);
}

LocalUserUiDriver::~LocalUserUiDriver()
{
    removeLocalUserUiBuilder(mLoginAndAccountCreation);
    removeLocalUserUiBuilder(mUserManager);
    removeLocalUserUiBuilder(mAssociationLists);
    removeLocalUserUiBuilder(mMessaging);
    removeLocalUserUiBuilder(mAchievements);
    removeLocalUserUiBuilder(mFriends);
    removeLocalUserUiBuilder(mUserExtendedData);
    removeLocalUserUiBuilder(mPlatformFeatures);
    removeLocalUserUiBuilder(mByteVault);
}

}

class IgnitionProxyServerListener : public Pyro::ProxyServerListenerEx<Ignition::BlazeHubUiDriver>
{
public:
    IgnitionProxyServerListener()
    {
        getInfo().setCaption("Blaze Ignition");
        getInfo().setDescription("Provides sample code and functionality covering many aspects of Blaze");
        getInfo().setVersion(Blaze::getBlazeSdkVersionString());
        getInfo().setSupportsMultipleClients(false);
    }
};

static IgnitionProxyServerListener gsIgnitionProxyServerListener;

#if defined(EA_PLATFORM_WINDOWS)
    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
    {
        Ignition::initializePlatform();

        Pyro::PyroFramework pyro(&gsIgnitionProxyServerListener);
        // Potentially we could make Windows use runAsync like other platforms to standardize our flow
        pyro.run(lpCmdLine);

        Ignition::finalizePlatform();
    }
#elif defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
    #include "Ignition/xboxone/SampleCore.h"
    int main(Platform::Array<Platform::String^>^ args)
    {
        auto xboxOneViewFactory = ref new Ignition::XboxOneViewFactory(gsIgnitionProxyServerListener);
        Windows::ApplicationModel::Core::CoreApplication::Run(xboxOneViewFactory);
    }
#elif defined(EA_PLATFORM_XBOX_GDK)
#include "Ignition/xboxgdk/SampleCore.h"
int32_t APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, char *cmdline, int32_t iShow)
{
    Ignition::Sample sample(gsIgnitionProxyServerListener);
    if (sample.Initialize(inst, iShow))
    {
        sample.Run();
        sample.Uninitialize();
    }
}
#elif defined(EA_PLATFORM_NX)
extern "C" void nnMain()
{
    Ignition::initializePlatform();

    Pyro::PyroFramework pyro(&gsIgnitionProxyServerListener);
    pyro.runAsync(nn::os::GetHostArgc(), nn::os::GetHostArgv());

    //prepare for rendering and update it
    T2RenderInit(&Ignition::gRender);

    while (!pyro.isTerminationRequested())
    {
        T2RenderUpdate(&Ignition::gRender);
        Ignition::tickPlatform();
        NetConnSleep(10);
    }
    T2RenderTerm(&Ignition::gRender);

    Ignition::finalizePlatform();
}
#elif defined(EA_PLATFORM_STADIA)
#include "Ignition/stadia/SampleCore.h"
    int main(int32_t argc, char **argv)
    {
        Ignition::initializePlatform();

        Pyro::PyroFramework pyro(&gsIgnitionProxyServerListener);
        pyro.runAsync(argc, argv);


        while (!pyro.isTerminationRequested())
        {
            Ignition::tickPlatform();
            NetConnSleep(16);
        }

        Ignition::finalizePlatform();
        return(0);
    }
#else
    int main(int32_t argc, char8_t *argv[])
    {
        Ignition::initializePlatform();

        Pyro::PyroFramework pyro(&gsIgnitionProxyServerListener);
        pyro.runAsync(argc, argv);

        //prepare for rendering and update it
#if defined(EA_PLATFORM_PS4)
        T2RenderInit(&Ignition::gRender);
#elif defined(EA_PLATFORM_PS5)
        // start rendering
        int32_t iMemModule = 'blaz';
        int32_t iMemGroup;
        void *pMemGroupUserData;
        DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
        T2RenderInit(iMemModule, iMemGroup, pMemGroupUserData);
#endif
        while (!pyro.isTerminationRequested())
        {
#if defined(EA_PLATFORM_PS4)
            T2RenderUpdate(&Ignition::gRender);
#elif defined(EA_PLATFORM_PS5)
            T2RenderUpdate();
#endif
            Ignition::tickPlatform();
            NetConnSleep(10);
        }
#if defined(EA_PLATFORM_PS4)
        T2RenderTerm(&Ignition::gRender);
#elif defined(EA_PLATFORM_PS5)
        T2RenderTerm();
#endif

        Ignition::finalizePlatform();
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

#ifndef BLAZESDK_DLL

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
// A default allocator
class TestAllocator : public EA::Allocator::ICoreAllocator
{
    virtual void *Alloc(size_t size, const char * name, unsigned int flags) { return malloc(size); }
    virtual void *Alloc(size_t size, const char * name, unsigned int flags, unsigned int align, unsigned int alignoffset)  { return malloc(size); }
    virtual void *AllocDebug(size_t size, const DebugParams debugParams, unsigned int flags)  { return malloc(size); }
    virtual void *AllocDebug(size_t size, const DebugParams debugParams, unsigned int flags, unsigned int align, unsigned int alignOffset)  { return malloc(size); }
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
int Vsnprintf8 (char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments); //necessary for stupid codewarrior warning.
int Vsnprintf8 (char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}

//Vsnprintf16 for eastl
int Vsnprintf16 (char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments); //necessary for stupid codewarrior warning.
int Vsnprintf16 (char16_t*  pDestination, size_t n, const char16_t*  pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}
