#ifndef IGNITION_LOGIN_HELPER_H
#define IGNITION_LOGIN_HELPER_H

#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/component/framework/tdf/userdefines.h"
#include "BlazeSDK/dispatcher.h"
#include "BlazeSDK/loginmanager/loginmanagerlistener.h"
#include "BlazeSDK/internal/loginmanager/loginmanagerimpl.h"
#include "Ignition/Ignition.h"

#if defined(BLAZESDK_CONSOLE_PLATFORM)
#define BLAZE_USING_NUCLEUS_SDK
#endif

#if defined(BLAZE_USING_NUCLEUS_SDK)
#include <NucleusSDK/nucleussdk.h>
#ifdef EA_DEBUG
#include "Ignition/NucleusSdkTracer.h"
#endif
#endif



namespace Ignition
{

class NucleusLoginListener
{
public:
    virtual void onNucleusLogin(const char8_t* code, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt) = 0;
    virtual void onNucleusLoginFailure(Blaze::BlazeError blazeError) = 0;
    virtual Pyro::UiDriver * getSharedUiDriver() = 0;
};

class NucleusLoginHelper :
#ifdef BLAZE_USING_NUCLEUS_SDK
    public EA::Nucleus::IIdentityEventListener,
#endif
    public Blaze::Idler
{
public:
    NucleusLoginHelper(NucleusLoginListener& owner);
    ~NucleusLoginHelper();
    void onStartLoginProcess(uint32_t dirtySockUserIndex, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt);
    void cancelLogin();
    bool isLoginInProgress() { return mLoginInitStarted; }

    void idle(const uint32_t currentTime, const uint32_t elapsedTime) override;

#ifdef BLAZE_USING_NUCLEUS_SDK
    static Blaze::BlazeError blazeErrorFromNucleusError(EA::Nucleus::NucleusError error);

    // IIdentityEventListener interface methods
    void OnLoginSuccess(EA::Nucleus::AsyncCallId apiRequestId, const char8_t *clientID) override;
    void OnTaskSuccess(EA::Nucleus::AsyncCallId apiRequestId) override;
    void OnError(EA::Nucleus::AsyncCallId apiRequestId, EA::Nucleus::NucleusError errorCode, const char8_t * msg) override;
    void OnAuthCodeReceived(EA::Nucleus::AsyncCallId apiRequestId, const char8_t *clientID, const char8_t *authCode) override;
    void OnAccessTokenReceived(EA::Nucleus::AsyncCallId apiRequestId, const char8_t *clientID, const char8_t *scope, const char8_t *accessToken) override;
#endif

    static void initNucleusSDK(const char8_t* sonyPsnClientId);
    static void shutdownNucleusSDK();

private:

    enum IdlerState
    {
        IDLER_STATE_NONE,
        IDLER_STATE_NUCLEUS_LOGIN,
        IDLER_STATE_REQUESTING_AUTHCODE
    };

    void startConsoleLogin();
    void onFetchClientConfig(const Blaze::Util::FetchConfigResponse* response, Blaze::BlazeError err, Blaze::JobId jobId);
    int32_t filloutUserInfo();
    void cleanupIdentitySession();
    void finishLogin(Blaze::BlazeError err, bool clearIdlerState = false, bool delayCleanup = false);

    IdlerState getIdlerState() { return mIdlerState; }
    bool setIdlerState(IdlerState idlerState, bool delayCleanup = false);
    static const char8_t* getIdlerStateAsString(IdlerState idlerState);

    NucleusLoginListener& mOwner;
    bool mLoginInitStarted;
    bool mLoginCancelled;
    IdlerState mIdlerState;

    Blaze::Util::FetchConfigResponse mIdentityConfig;
    char8_t mProductName[Blaze::MAX_PRODUCTNAME_LENGTH];
    uint32_t mDirtySockUserIndex;
    Blaze::Authentication::CrossPlatformOptSetting mCrossPlatformOpt;

#ifdef BLAZE_USING_NUCLEUS_SDK
//By Blaze/Identity spec this is intentionally the same/reused for both PS5 and cross-gen PS4 (for Blaze core's S2S user tokens):
#define NP_CLIENT_ID        "c87351d1-f7d9-4d53-8354-8608e71c1416"  //Harcoded Client Id from Sony
    EA::Nucleus::IIdentitySession* mIdentitySession;
    EA::Nucleus::AsyncCallId mLoginTaskId;
#endif
};

#if defined(BLAZE_USING_NUCLEUS_SDK) && defined(EA_DEBUG)
extern NucleusSdkTracer gNucleusSdkTracer;
#endif

}
#endif
