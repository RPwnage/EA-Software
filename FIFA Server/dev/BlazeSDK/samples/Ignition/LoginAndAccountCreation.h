
#ifndef LOGIN_AND_ACCOUNT_CREATION_H
#define LOGIN_AND_ACCOUNT_CREATION_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"
#include "Ignition/NucleusLoginHelper.h"
#include "Ignition/EadpLoginHelper.h"
#include "Ignition/SteamHelper.h"
#include "BlazeSDK/blazesdkdefs.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/loginmanager/loginmanagerlistener.h"

namespace Ignition
{

class LoginAndAccountCreation
    : public LocalUserUiBuilder,
      public BlazeHubUiDriver::PrimaryLocalUserAuthenticationListener,
      public BlazeHubUiDriver::PrimaryLocalUserChangedListener,
      protected Blaze::LoginManager::LoginManagerListener,
      public NucleusLoginListener,
      public EadpLoginListener
{
    friend class BlazeInitialization;
    public:
        LoginAndAccountCreation(uint32_t userIndex);
        virtual ~LoginAndAccountCreation();

        // Used by Ignition::Connection::connectAndLogin
        void loginUser(Pyro::UiNodeParameterStructPtr loginParameters);
        void expressLoginUser(Pyro::UiNodeParameterStructPtr loginParameters);
        static void setupCommonLoginParameters(Pyro::UiNodeAction &action);
        static void setupExpressLoginParameters(Pyro::UiNodeAction &action);

        // NucleusLoginListener methods
        void onNucleusLogin(const char8_t* code, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt);
        void onNucleusLoginFailure(Blaze::BlazeError blazeError);
        Pyro::UiDriver * getSharedUiDriver() { return getUiDriver(); }

        // EadpLoginListener methods
        void onEadpLoginByAuthCode(const char8_t* authCode, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt);
        void onEadpLoginByAccessToken(const char8_t* accessToken, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt);
        void onEadpUpdateAccessToken(const char8_t* accessToken);
        void onEadpUpdateAccessTokenFailure(Blaze::BlazeError blazeError);

    protected:
        void loginUserByAuthCode(const char8_t* code, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt);
        void loginUserByAccessToken(const char8_t* accessToken, const char8_t* productName, Blaze::Authentication::CrossPlatformOptSetting crossPlatformOpt);
        void updateAccessToken(const char8_t* accessToken);

        virtual void onAuthenticatedPrimaryUser();
        virtual void onDeAuthenticatedPrimaryUser();

        virtual void onAuthenticated();
        virtual void onDeAuthenticated();
        virtual void onForcedDeAuthenticated(Blaze::UserSessionForcedLogoutType forcedLogoutType);

        
        //LoginManagerListener Login methods
        virtual void onLoginFailure(Blaze::BlazeError blazeError, const char8_t* portalUrl);
        virtual void onGetAccountInfoError(Blaze::BlazeError blazeError);
        virtual void onGuestLogin(Blaze::BlazeError blazeError, const Blaze::SessionInfo* info);

        virtual void onSdkError(Blaze::BlazeError blazeError);

        //PrimaryLocalUserChanged listener
        virtual void onPrimaryLocalUserChanged(uint32_t userIndex);

    private:
        PYRO_ACTION(LoginAndAccountCreation, StartLogin);
        PYRO_ACTION(LoginAndAccountCreation, AuthCodeLogin);
        PYRO_ACTION(LoginAndAccountCreation, AccessTokenLogin);
        PYRO_ACTION(LoginAndAccountCreation, StartGuestLogin);
        PYRO_ACTION(LoginAndAccountCreation, Logout);
        PYRO_ACTION(LoginAndAccountCreation, ForceOwnSessionLogout);
        PYRO_ACTION(LoginAndAccountCreation, CreateWalUserSession);
        PYRO_ACTION(LoginAndAccountCreation, BackDisplay);
        PYRO_ACTION(LoginAndAccountCreation, CancelLogin);
        PYRO_ACTION(LoginAndAccountCreation, AssociateAccount);
#if !defined(EA_PLATFORM_NX)
        PYRO_ACTION(LoginAndAccountCreation, SelectAudioDevices);
#endif
        PYRO_ACTION(LoginAndAccountCreation, ExpressLogin);
        PYRO_ACTION(LoginAndAccountCreation, ExpressLoginNexus);
        PYRO_ACTION(LoginAndAccountCreation, SetConsoleUserIndex);
        PYRO_ACTION(LoginAndAccountCreation, UpdateAccessToken);

        int32_t SubmitEvent();

        void ExpressLoginCb(const Blaze::Authentication::LoginResponse *response, Blaze::BlazeError blazeError, Blaze::JobId JobId);
        void CancelLoginCb(Blaze::BlazeError error);
        void ForceOwnSessionLogoutCb(Blaze::BlazeError blazeError, Blaze::JobId JobId);
        void CreateWalUserSessionCb(const Blaze::Authentication::UserLoginInfo* userLoginInfo, Blaze::BlazeError blazeError, Blaze::JobId JobId);
        void UpdateAccessTokenCb(Blaze::BlazeError error);

        NucleusLoginHelper mNucleusLoginHelper;   // NucleusLoginHelper is a utility class; it doesn't have its own UI
        EadpLoginHelper mEadpLoginHelper;
};

}

#endif
