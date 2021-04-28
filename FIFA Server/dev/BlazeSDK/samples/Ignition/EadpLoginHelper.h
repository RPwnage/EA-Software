#ifndef EADP_LOGIN_HELPER_H
#define EADP_LOGIN_HELPER_H

#include "BlazeSDK/blazehub.h"
#include "Ignition/Ignition.h"

#include "eadp/foundation/Hub.h"
#include "eadp/authentication/ID20AuthService.h"

#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS4_CROSSGEN) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_NX) || defined(EA_PLATFORM_STADIA)
#define BLAZE_USING_EADP_SDK
#endif

namespace Ignition
{

class EadpLoginListener
{
public:
    virtual void onEadpLoginByAuthCode(const char8_t* authCode, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt) = 0;
    virtual void onEadpLoginByAccessToken(const char8_t* accessToken, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt) = 0;
    virtual void onEadpUpdateAccessToken(const char8_t* accessToken) = 0;
};

class EadpLoginHelper
{
public:
    EadpLoginHelper(EadpLoginListener& owner);
    ~EadpLoginHelper();

    void onStartLoginProcess(const char8_t* productName, const Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt, const LoginAuthenticationOption authOption);
    void requestAccessToken();
    void onAccessTokenReceived(const char8_t* accessToken);
    void requestAuthCode();
    void onAuthCodeReceived(const char8_t* authCode);
    void onLogout();
    void updateAccessToken();

private:
    void initOriginService();
    eadp::foundation::PlatformUserPtr retrieveUserInfo();

private:
    EadpLoginListener& mOwner;

    char8_t mProductName[Blaze::MAX_PRODUCTNAME_LENGTH];
    Blaze::Authentication::CrossPlatformOptSetting mCrossPlatformOpt;
    LoginAuthenticationOption mAuthOption;

    eadp::foundation::SharedPtr<eadp::authentication::ID20AuthService> mId20AuthService;
    eadp::foundation::Callback::PersistencePtr mPersistenceObject;

#if defined(EA_PLATFORM_WINDOWS)
    eadp::foundation::SharedPtr<ea::origin::IOriginService> mOriginService;
    bool mIsOriginClientConnected;
#else
    eadp::foundation::SharedPtr<eadp::browserprovider::IBrowserProvider> mBrowserProvider;
#endif
};

}  //namespace Ignition

#endif
