#include "Ignition/NucleusLoginHelper.h"
#include "BlazeSDK/blazesdkdefs.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"

// Include to get access to the gpEacup for the User:
#if defined(EA_PLATFORM_NX)
#include "Ignition/nx/SampleCore.h"
#elif defined(EA_PLATFORM_XBSX)
#include "Ignition/xboxgdk/SampleCore.h"
#elif defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
#include "Ignition/ps5/SampleCore.h"
#elif defined(EA_PLATFORM_STADIA)
#include "Ignition/stadia/SampleCore.h"
#endif

namespace Ignition
{
#if defined(BLAZE_USING_NUCLEUS_SDK) && defined(EA_DEBUG)
NucleusSdkTracer gNucleusSdkTracer;
#endif

NucleusLoginHelper::NucleusLoginHelper(NucleusLoginListener& owner)
    : mOwner(owner),
      mLoginInitStarted(false),
      mLoginCancelled(false),
      mIdlerState(IDLER_STATE_NONE)
#ifdef BLAZE_USING_NUCLEUS_SDK
      ,mIdentitySession(nullptr),
      mLoginTaskId(EA::Nucleus::kInvalidAsyncCallId)
#endif
{
    mProductName[0] = '\0';
}

NucleusLoginHelper::~NucleusLoginHelper()
{
    cleanupIdentitySession();
    gBlazeHub->getScheduler()->removeByAssociatedObject(this);
    shutdownNucleusSDK();
}

void NucleusLoginHelper::finishLogin(Blaze::BlazeError err, bool clearIdlerState, bool delayCleanup)
{
    mLoginInitStarted = false;
    bool doCallback = (err != Blaze::ERR_OK);

    // If err != ERR_OK but the idler is already in state NONE, that means an error was returned for a
    // NucleusSDK task we don't care about anymore. Don't invoke the onNucleusLoginFailure callback in this case
    if (clearIdlerState)
        doCallback = (setIdlerState(IDLER_STATE_NONE, delayCleanup) && doCallback);

    if (doCallback)
        mOwner.onNucleusLoginFailure(err);
}

void NucleusLoginHelper::cancelLogin()
{
    mLoginCancelled = true;
}

void NucleusLoginHelper::onStartLoginProcess(uint32_t dirtySockUserIndex, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt)
{
#if defined(BLAZE_USING_NUCLEUS_SDK)
    EA::Nucleus::Globals::SetAllocator(&gsAllocator);
#endif
#if defined(BLAZE_USING_NUCLEUS_SDK) && defined(EA_DEBUG)
    gNucleusSdkTracer.Init(*mOwner.getSharedUiDriver());
#endif

    if (mLoginInitStarted)
    {
        mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  ERROR [NucleusLoginHelperBase::onStartLoginProcess()] login initialization already in progress");
        return;
    }
    else
    {
        mLoginInitStarted = true;
    }

    mLoginCancelled = false;
    mDirtySockUserIndex = dirtySockUserIndex;
    mProductName[0] = '\0';
    blaze_strnzcpy(mProductName, productName, sizeof(mProductName));
    mCrossPlatformOpt = crossPlatformOpt;

    if (mIdentityConfig.getConfig().empty())
    {
        Blaze::Util::UtilComponent* util = gBlazeHub->getComponentManager()->getUtilComponent();

        if (util != nullptr)
        {
            Blaze::Util::FetchClientConfigRequest req;
            req.setConfigSection(IDENTITY_CONFIG_SECTION);

            Blaze::Util::UtilComponent::FetchClientConfigCb cb(this, &NucleusLoginHelper::onFetchClientConfig);
            util->fetchClientConfig(req, cb);
            return;
        }
    }

    startConsoleLogin();
}

void NucleusLoginHelper::onFetchClientConfig(const Blaze::Util::FetchConfigResponse* response, Blaze::BlazeError err, Blaze::JobId jobId)
{
    if (err != Blaze::ERR_OK)
    {
        finishLogin(err);
    }
    else
    {
        response->copyInto(mIdentityConfig);
        startConsoleLogin();
    }
}

void NucleusLoginHelper::cleanupIdentitySession()
{
#ifdef BLAZE_USING_NUCLEUS_SDK
    if (mIdentitySession != nullptr)
    {
        if (!mIdentitySession->IsAllTaskDone())
            mIdentitySession->CancelAll();
        EA::Nucleus::IIdentitySession::Destroy(mIdentitySession);
        mIdentitySession = nullptr;
    }
#endif
}

void NucleusLoginHelper::startConsoleLogin()
{
#ifdef BLAZE_USING_NUCLEUS_SDK
    if (getIdlerState() != IDLER_STATE_NONE || mIdentitySession != nullptr)
    {
        mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  ERROR [NucleusLoginHelper::startConsoleLogin()] Idler state should be NONE and mIdentitySession should be nullptr; actual idler state is %s and mIdentitySession is%s nullptr",
            getIdlerStateAsString(mIdlerState), (mIdentitySession == nullptr ? "" : " not"));

        finishLogin(Blaze::ERR_SYSTEM, true);
        return;
    }

    if (filloutUserInfo() != 0)
    {
        finishLogin(Blaze::ERR_SYSTEM);
        return;
    }

    //client id and secret of each platform are valid for nucleus int env
#if defined(EA_PLATFORM_XBOXONE)
    EA::Nucleus::Globals::SetAuthenticationSource("100002");
    mIdentitySession = EA::Nucleus::IIdentitySession::Create("EADP_GS_BLZDEV_XONE_CLIENT", "1TLw8A0c6SJ0ccQX3qynVa7VaqWUZNv9ZqjhJStR04cr3eq9VSYbNVxuesGt9TULxGnqcoX1LOpEUFJG", this, nullptr);
#elif defined(EA_PLATFORM_NX)
    EA::Nucleus::Globals::SetAuthenticationSource("100005");
    mIdentitySession = EA::Nucleus::IIdentitySession::Create("EADP_GS_BLZDEV_NX_CLIENT", "0L6TVymXsMkCN7K9Np5mS6rBCgokFOW2BPwD07D6uAtSyjW6M26hX3Xu0x1KvoaGnyc9rI1wZUljBjbh", this, nullptr);
#elif defined(EA_PLATFORM_PS5)
    EA::Nucleus::Globals::SetAuthenticationSource("100008");
    mIdentitySession = EA::Nucleus::IIdentitySession::Create("EADP_GS_BLZDEV_PS5_CLIENT", "q8kZJ5uWB6ApVAn3spC5ycd5ZwRwqv3kWYOnT7uE5HKe96zq1r6QmlvksKR88bCqk1zClpVkh3SU3Bu0", this, nullptr);
#elif defined(EA_PLATFORM_PS4_CROSSGEN)
    EA::Nucleus::Globals::SetAuthenticationSource("100003");
    mIdentitySession = EA::Nucleus::IIdentitySession::Create("EADP_GS_BLZDEV_PS4_XGEN_CLIENT", "SxJ8sgxPfG7dTC3rT44Sz2FjtzoN8dCe6nDuDo0a8w1wKLMNya7ORJ0xV6l42aJYfIQ1ChyorFK8nACY", this, nullptr);
#elif defined(EA_PLATFORM_PS4)
    EA::Nucleus::Globals::SetAuthenticationSource("100003");
    mIdentitySession = EA::Nucleus::IIdentitySession::Create("EADP_GS_BLZDEV_PS4_CLIENT", "3glLXIvqSVLIdLcLn8iBkGmKdQ2VToQabCV0HQgE8XH9KOOdole4MiErmIx0GKpo2SqIZtt346Mgi7li", this, nullptr);
#elif defined(EA_PLATFORM_XBSX)
    EA::Nucleus::Globals::SetAuthenticationSource("100009");
    mIdentitySession = EA::Nucleus::IIdentitySession::Create("EADP_GS_BLZDEV_XBSX_CLIENT", "y5fi5ViryydjFBkpBcBjaFE8nLbgjmElS46eCpupsiBphL6S7lLeIcxD2KcEiXkiNjPDdPniURoNGLbA", this, nullptr);
#elif defined(EA_PLATFORM_STADIA)
    EA::Nucleus::Globals::SetAuthenticationSource("100011");
    mIdentitySession = EA::Nucleus::IIdentitySession::Create("EADP_GS_BLZDEV_SDA_CLIENT", "zZD3dGXOJuiJh06d4PoTU99V5EyDMKPeZnKO8rmJeE8GuOYewG9Ii1lqQvunMMwOEkqBO6Ixhv3sZJua", this, nullptr);
#endif
    if (mIdentitySession == nullptr)
    {
        mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  ERROR [NucleusLoginHelper::startConsoleLogin()] Failed to create IIdentitySession");
        finishLogin(Blaze::ERR_SYSTEM);
        return;
    }
    initNucleusSDK(NP_CLIENT_ID);

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_NX)
    EA::User::IEAUser *pIEAUser;
    if (NetConnStatus('eusr', mDirtySockUserIndex, &pIEAUser, sizeof(pIEAUser)) < 0)
    {
        mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  ERROR [NucleusLoginHelper::startConsoleLogin()] NetConnStatus 'eusr' failed to return a user at index %d.", mDirtySockUserIndex);
        finishLogin(Blaze::ERR_SYSTEM);
    }
    mLoginTaskId = mIdentitySession->Login((EA::Nucleus::UserInfo*)pIEAUser);
#else // 'eusr' not implemented for other platforms (yet) so we just use the primary user- meaning MLU won't work with nucleuslogin helper
    mLoginTaskId = mIdentitySession->Login((EA::Nucleus::UserInfo*)gpEacup->GetPrimaryUser());
#endif 

    setIdlerState(IDLER_STATE_NUCLEUS_LOGIN);
#else
    mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  ERROR [NucleusLoginHelper::startConsoleLogin()] Logging in via NucleusLoginHelper is not supported on the current platform!");
    finishLogin(Blaze::ERR_SYSTEM);
#endif
}

bool NucleusLoginHelper::setIdlerState(IdlerState idlerState, bool delayCleanup /*= false*/)
{
    if (idlerState == mIdlerState)
    {
        mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  INFO [NucleusLoginHelper::setIdlerState()] Idler is already in state %s", getIdlerStateAsString(mIdlerState));
        if (idlerState != IDLER_STATE_NONE) // if idler state is being set to NONE, there may be some cleanup to do
            return false;
    }

    if (idlerState == IDLER_STATE_NONE)
    {
#ifdef BLAZE_USING_NUCLEUS_SDK
        mLoginTaskId = EA::Nucleus::kInvalidAsyncCallId;
        if (mIdentitySession != nullptr && !mIdentitySession->IsAllTaskDone())
            mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  INFO [NucleusLoginHelper::setIdlerState()] Transitioning to idler state NONE (from state %s); pending NucleusSDK tasks will be cancelled %s.", getIdlerStateAsString(mIdlerState), (delayCleanup ? "on next idle()" : "immediately"));
#endif
        bool changedState = (mIdlerState != IDLER_STATE_NONE);
        mIdlerState = idlerState;

        // We may need to delay cleanupIdentitySession() until the next idle().
        // (If we're setting the idler state from a NucleusSDK callback, another Tick() may be called on mIdentitySession's task pool after we exit here.)
        if (!delayCleanup)
        {
            gBlazeHub->removeIdler(this);
            cleanupIdentitySession();
            shutdownNucleusSDK();
        }
        return changedState;
    }

    if (idlerState == IDLER_STATE_NUCLEUS_LOGIN && mIdlerState == IDLER_STATE_NONE)
    {
        mIdlerState = idlerState;
        gBlazeHub->addIdler(this, "NucleusLoginHelper");
    }
    else if (idlerState == IDLER_STATE_REQUESTING_AUTHCODE && mIdlerState == IDLER_STATE_NUCLEUS_LOGIN)
    {
        mIdlerState = idlerState;
    }

    if (mIdlerState != idlerState)
        mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  INFO [NucleusLoginHelper::setIdlerState()] Ignoring attempt to transition from idler state %s to idler state %s", getIdlerStateAsString(mIdlerState), getIdlerStateAsString(idlerState));

    return (mIdlerState == idlerState);
}

void NucleusLoginHelper::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
#ifdef BLAZE_USING_NUCLEUS_SDK
    if (mLoginCancelled)
    {
        // The login has been cancelled before it could complete, so abort this attempt
        mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  INFO [NucleusLoginHelper::idle()] Login attempt has been cancelled by client");

        mLoginCancelled = false;
        mIdentitySession->GetBrowserProvider()->Shutdown();
        finishLogin(Blaze::ERR_OK, true, false);
        return;
    }

    // If the conn is bad just fail out, the connection manager will handle it from here.
    int32_t connStatus = NetConnStatus('conn', 0, nullptr, 0);
    if ((connStatus >> 24) == '-')
    {
        mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  ERROR [NucleusLoginHelper::idle()] NetConnStatus returned an error %d", connStatus);
        setIdlerState(IDLER_STATE_NONE);
    }
    else if (getIdlerState() == IDLER_STATE_NONE)
    {
        gBlazeHub->removeIdler(this);
        cleanupIdentitySession();
        shutdownNucleusSDK();
    }
    else
    {
        mIdentitySession->Tick();
    }
#endif
}

// static
const char8_t* NucleusLoginHelper::getIdlerStateAsString(IdlerState idlerState)
{
    switch (idlerState)
    {
    case IDLER_STATE_NONE:
        return "NONE";
    case IDLER_STATE_NUCLEUS_LOGIN:
        return "NUCLEUS_LOGIN";
    case IDLER_STATE_REQUESTING_AUTHCODE:
        return "REQUESTING_AUTHCODE";
    default:
        return "UNKNOWN";
    }
}

#ifdef BLAZE_USING_NUCLEUS_SDK
// static
Blaze::BlazeError NucleusLoginHelper::blazeErrorFromNucleusError(EA::Nucleus::NucleusError error)
{
    switch (error)
    {
    case EA::Nucleus::kNcsErrorOK:
        return Blaze::ERR_OK;
    case EA::Nucleus::kNcsErrorCancelled:
    case EA::Nucleus::kNcsErrorLoginCancelled:
        return Blaze::SDK_ERR_RPC_CANCELED;
    case EA::Nucleus::kNcsErrorNucleusUserStatusInvalidError:
        return Blaze::AUTH_ERR_INVALID_USER;
    case EA::Nucleus::kNcsErrorNucleusPersonaStatusInvalidError:
        return Blaze::AUTH_ERR_INVALID_PERSONA;
    case EA::Nucleus::kNcsErrorNucleusUserUnderageError:
        return Blaze::AUTH_ERR_TOO_YOUNG;
    default:
        return Blaze::SDK_ERR_NUCLEUS_RESPONSE;
    }
}

void NucleusLoginHelper::OnLoginSuccess(EA::Nucleus::AsyncCallId apiRequestId, const char8_t *clientID)
{
    mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  INFO [NucleusLoginHelper::OnLoginSuccess()] API request id: %d; client id: %s", apiRequestId, clientID);

    if (mLoginTaskId != apiRequestId)
    {
        mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  ERROR [NucleusLoginHelper::OnLoginSuccess()] API request id (%d) does not match current request id (%d); login cannot proceed");
        finishLogin(Blaze::ERR_SYSTEM, true, true);
    }
}

void NucleusLoginHelper::OnTaskSuccess(EA::Nucleus::AsyncCallId apiRequestId)
{
    mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  INFO [NucleusLoginHelper::OnTaskSuccess()] current request id: %d; API request id: %d", mLoginTaskId, apiRequestId);
}

void NucleusLoginHelper::OnError(EA::Nucleus::AsyncCallId apiRequestId, EA::Nucleus::NucleusError errorCode, const char8_t * msg)
{
    if (mOwner.getSharedUiDriver() != nullptr)
        mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  ERROR [NucleusLoginHelper::OnError()] API request id (%d) failed with error code(%d); current request id: %d; error message: %s", apiRequestId, errorCode, mLoginTaskId, msg);
    finishLogin(blazeErrorFromNucleusError(errorCode), true, true);
}

void NucleusLoginHelper::OnAuthCodeReceived(EA::Nucleus::AsyncCallId apiRequestId, const char8_t *clientID, const char8_t *authCode)
{
    mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  INFO [NucleusLoginHelper::OnAuthCodeReceived()] API request id: %d; client id: %s; authCode: %s", apiRequestId, clientID, authCode);

    if (getIdlerState() != IDLER_STATE_REQUESTING_AUTHCODE || mLoginTaskId != apiRequestId)
    {
        mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  ERROR [NucleusLoginHelper::OnAuthCodeReceived()] Unexpected current request id(%s) or idler state(%s); login cannot proceed", mLoginTaskId, getIdlerStateAsString(mIdlerState));
        finishLogin(Blaze::ERR_SYSTEM, true, true);
    }
    else
    {
        finishLogin(Blaze::ERR_OK, true, true);
        mOwner.onNucleusLogin(authCode, mProductName, mCrossPlatformOpt);
    }
}

void NucleusLoginHelper::OnAccessTokenReceived(EA::Nucleus::AsyncCallId apiRequestId, const char8_t *clientID, const char8_t *scope, const char8_t *accessToken)
{
    // OnAccessTokenReceived is also invoked when an access token is refreshed, so ignore any mismatch between api request id and current request id
    mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  INFO [NucleusLoginHelper::OnAccessTokenReceived()] login request id: %d; API request id: %d; client id: %s; scope: %s; accessToken: %s", mLoginTaskId, apiRequestId, clientID, scope, accessToken);

    if (setIdlerState(IDLER_STATE_REQUESTING_AUTHCODE))
    {
        // We're now requesting an auth code for the blazeserver's client id, so we need to use the corresponding redirect url
        EA::Nucleus::Globals::SetClientSideRedirectUri(mIdentityConfig.getConfig()[BLAZESDK_CONFIG_KEY_IDENTITY_REDIRECT]);
        mLoginTaskId = mIdentitySession->RequestAuthCode(gBlazeHub->getConnectionManager()->getClientId());
    }
}
#endif

int32_t NucleusLoginHelper::filloutUserInfo()
{
#if defined(BLAZE_USING_NUCLEUS_SDK)
    // Stadia uses the StadiaUser instead of the UserInfo type.  This prevents the use of the Nucleus::UserInfo dummy structure. 
    return 0;
#else
    mOwner.getSharedUiDriver()->addLog_va(Pyro::ErrorLevel::NONE, "Ignition  ERROR [NucleusLoginHelper::filloutUserInfo()] filloutUserInfo is only implemented on the NX platform!");
    return -1;
#endif
}

void NucleusLoginHelper::initNucleusSDK(const char8_t* sonyPsnClientId)
{
#ifdef BLAZE_USING_NUCLEUS_SDK
    // Hardcode the redirect url associated with the client id
    EA::Nucleus::Globals::SetClientSideRedirectUri("http://127.0.0.1/success");

    const char8_t* configStr;
    if (gBlazeHub->getConnectionManager()->getServerConfigString(BLAZESDK_CONFIG_KEY_IDENTITY_CONNECT_HOST, &configStr))
        EA::Nucleus::SetNucleusBaseUrl(configStr);
    else
        EA::Nucleus::SetNucleusBaseUrl(EA::Nucleus::getKNucleusIntBaseUrl());
    if (gBlazeHub->getConnectionManager()->getServerConfigString(BLAZESDK_CONFIG_KEY_IDENTITY_PROXY_HOST, &configStr))
        EA::Nucleus::SetNucleusBaseProxyUrl(configStr);
    else
        EA::Nucleus::SetNucleusBaseUrl(EA::Nucleus::getKNucleusIntProxyUrl());
    if (gBlazeHub->getConnectionManager()->getServerConfigString(BLAZESDK_CONFIG_KEY_IDENTITY_PORTAL_HOST, &configStr))
        EA::Nucleus::SetNucleusBasePortalUrl(configStr);
    else
        EA::Nucleus::SetNucleusBasePortalUrl(EA::Nucleus::getKNucleusIntPortalUrl());

  #if defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_PS4_CROSSGEN)
    EA::Nucleus::Globals::SetPSNClientId(sonyPsnClientId);
  #endif
#endif
}

void NucleusLoginHelper::shutdownNucleusSDK()
{
#ifdef BLAZE_USING_NUCLEUS_SDK
    EA::Nucleus::Globals::ClearCACerts();
    EA::Nucleus::Globals::ClearGlobals();
#endif
}

}
