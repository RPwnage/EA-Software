//  LoginController.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "engine/login/LoginController.h"
#include "engine/login/User.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/rest/AuthenticationData.h"
#include "services/rest/OriginServiceMaps.h"
#include "services/session/SessionService.h"
#include "services/settings/SettingsManager.h"
#include "services/config/OriginConfigService.h"
#include "TelemetryAPIDLL.h"
#include "engine/social/SocialController.h"
#include "engine/social/UserNamesCache.h"

using namespace Origin;
using namespace Origin::Services;

namespace Origin
{
    namespace Engine
    {
        static LoginController::Error toLoginControllerError(Session::SessionError error);

        static LoginController* sInstance = NULL;
        Session::SessionToken sInvalidToken;

        void LoginController::init()
        {
            if (!sInstance)
            {
                sInstance = new LoginController();
                ORIGIN_VERIFY_CONNECT(
                    Session::SessionService::instance(), SIGNAL(sessionTokensChanged(Origin::Services::Session::SessionRef, Origin::Services::Session::AccessToken, Origin::Services::Session::RefreshToken, Origin::Services::Session::SessionError)),
                    sInstance, SLOT(onSessionTokensChanged(Origin::Services::Session::SessionRef, Origin::Services::Session::AccessToken, Origin::Services::Session::RefreshToken, Origin::Services::Session::SessionError)));

                ORIGIN_VERIFY_CONNECT(
                    Session::SessionService::instance(), 
                    SIGNAL(relogin (Origin::Services::Session::SessionRef)),
                    sInstance, 
                    SLOT(onBeginReloginAsync (Origin::Services::Session::SessionRef)));
            }
        }

        void LoginController::release()
        {
            delete sInstance;
            sInstance = NULL;
        }

        LoginController* LoginController::instance()
        {
            return sInstance;
        }
        
        void LoginController::renewAccessTokenAsync()
        {
            if (Session::SessionService::hasValidSession())
                Session::SessionService::instance()->renewAccessTokenAsync(Session::SessionService::currentSession());
            else
                instance()->tokenRefreshComplete(LOGIN_ERROR_NO_SESSION);
        }

        void LoginController::nucleus20LoginAsync(const QString& authorizationCode, const QString& idToken)
        {
            Session::LoginRegistrationConfiguration2 config;
            config.setAuthorizationCode(authorizationCode);
            config.setIdToken(idToken);
            Session::SessionService::beginSessionAsync(Session::LoginRegistrationSession::create, config);
        }

        void LoginController::nucleus20OfflineLoginAsync(const QString& primaryOfflineKey, const QString& backupOfflineKey)
        {
            Session::LoginRegistrationConfiguration2 config;
            config.setOfflineKey(primaryOfflineKey);
            config.setBackupOfflineKey(backupOfflineKey);
            Session::SessionService::beginSessionAsync(Session::LoginRegistrationSession::create, config);
        }
        void LoginController::loginType(const QString& loginType, bool isAutoLogin)
        {
            instance()->mTypeOfLogin = loginType;
            instance()->mAutoLogin = isAutoLogin;
        }
        void LoginController::logoutAsync(Session::SessionRef session)
        {
#ifdef _DEBUG
            // Test that current user object is destroyed after logout
            new LogoutCleanupChecker(instance()->mCurrentUser);            
#endif
            if (!Session::SessionService::isValidSession(session))
                return;
            
            emit instance()->userAboutToLogout(instance()->mCurrentUser);

            Session::SessionService::endSessionAsync(session);
        }

        UserRef LoginController::currentUser()
        {
            LoginController* controller = instance();
            if (controller)
                return controller->mCurrentUser;
            
            else
                return UserRef();
        }

        bool LoginController::isUserLoggedIn()
        {
            return !instance()->mCurrentUser.isNull();
        }

        bool LoginController::isCurrentSessionAutoLogin()
        {
            return instance()->mAutoLogin;
        }

        void LoginController::onBeginSessionComplete(
            Session::SessionError sessionError, 
            Session::SessionRef sessionRef,
            QSharedPointer<Session::AbstractSessionConfiguration> sessionConfiguration)
        {
            Session::LoginRegistrationConfiguration2* config = dynamic_cast<Session::LoginRegistrationConfiguration2*>(sessionConfiguration.data());
            ORIGIN_UNUSED(config);
            ORIGIN_ASSERT(config);
            if (sessionError == Session::SESSION_NO_ERROR)
            {
                try
                {
                    Session::LoginRegistrationSession* session = dynamic_cast<Session::LoginRegistrationSession*>(sessionRef.data());
                    ORIGIN_ASSERT(session);

                    // save the login information
                    /*Origin::Services::Authentication const& authentication(session->authenticationInfo());
                    QString const& user(authentication.user.isUnderage ? authentication.user.originId : authentication.user.email);
                    Origin::Services::writeSetting(Origin::Services::SETTING_REMEMBERMEEMAIL, user);*/

                    Origin::Services::Authentication2 const& authentication(session->authentication2Info());
                    QString country = authentication.user.country;
                    GetTelemetryInterface()->setPersona(authentication.user.userId, country.toUtf8().data(), authentication.user.isUnderage );

                    // create a new user
                    if ( !mCurrentUser )
                    {
                        mCurrentUser = User::create(sessionRef, authentication);
                    }

                    bool mode = !Session::SessionService::accessToken(sessionRef).isEmpty();
                    bool isHwOptOut = Origin::Services::readSetting(Origin::Services::SETTING_HW_SURVEY_OPTOUT, sessionRef);
                    
                    bool isLoginInvisible = (0==QString::compare(authentication.mIsLoginInvisible, "true" , Qt::CaseInsensitive));
                    Origin::Services::writeSetting(Origin::Services::SETTING_LOGIN_AS_INVISIBLE, isLoginInvisible, sessionRef);
                    GetTelemetryInterface()->Metric_LOGIN_INVISIBLE(isLoginInvisible);

                    GetTelemetryInterface()->Metric_LOGIN_OK(mode, authentication.user.isUnderage, mTypeOfLogin.toUtf8().data(), isHwOptOut);

                    // Now that we're logged in and Telemetry is sure to be active, propagate any configuration issues
                    if (!Services::OriginConfigService::instance()->configurationSuccessful())
                    {
                        QPair<QString, QString> configurationErrors = Services::OriginConfigService::instance()->configurationErrors();
                        GetTelemetryInterface()->Metric_ERROR_NOTIFY(configurationErrors.first.toLocal8Bit().constData(), configurationErrors.second.toLocal8Bit().constData());
                    }
                    
                    // Populate our Origin ID cache with our own info
                    mCurrentUser->socialControllerInstance()->userNames()->setOriginIdForNucleusId(
                            authentication.user.userId,
                            authentication.user.originId);
                }
                catch(std::bad_alloc)
                {
                    Session::SessionService::endSessionAsync(sessionRef);
                    sessionError = restErrorUnknown;
                }
            }
            else if (sessionError.restErrorCode() != restErrorSuccess)
            {
                //  Filter out login errors if it is due to auto-logins missing the token file (*.olc).
                GetTelemetryInterface()->Metric_LOGIN_FAIL(
                    QString::number(sessionError.restErrorCode()).toLatin1().data(),
                    getLoginErrorString(sessionError.restErrorCode()).toLocal8Bit().data(),
                    mTypeOfLogin.toUtf8().data());
            }

            if (sessionError == Session::SESSION_NO_ERROR)
            {
                ORIGIN_ASSERT(mCurrentUser);
                emit userLoggedIn(mCurrentUser);

                //local host listens for this, to determine if the login was an sso login
                emit typeOfLogin(mTypeOfLogin);
            }

            Error error = toLoginControllerError(sessionError);
            emit loginComplete(error);
        }

        void LoginController::onEndSessionComplete(Origin::Services::Session::SessionError error)
        {
            if (error == Session::SESSION_NO_ERROR)
            {
                ORIGIN_ASSERT(!mCurrentUser.isNull());
                if (!mCurrentUser.isNull())
                {
                    emit userLoggedOut(mCurrentUser);
                    mCurrentUser->cleanUp();
                    mCurrentUser.clear();
                }

                emit logoutComplete(LOGIN_SUCCEEDED);
            }
        }

        void LoginController::onSessionTokensChanged(
            Origin::Services::Session::SessionRef /*session*/,
            Origin::Services::Session::AccessToken /*authToken*/,
            Origin::Services::Session::RefreshToken refreshToken,
            Session::SessionError error)
        {
            emit tokenRefreshComplete(toLoginControllerError(error));
        }

        void LoginController::onBeginReloginAsync (Origin::Services::Session::SessionRef session)
        {
            Session::RefreshToken refreshToken (Session::SessionService::refreshToken (session));
            if (refreshToken.isEmpty())   //the user would have logged in offline mode
            {
                ORIGIN_LOG_EVENT <<"session refresh token is empty";
            }

            Session::LoginRegistrationConfiguration2 config;
            config.setRefreshToken(refreshToken);

            // after the relogin response, existing session will be updated
            Session::SessionService::updateSessionAsync(session, config);
        }

        LoginController::LoginController()
        {
            ORIGIN_VERIFY_CONNECT(
                Session::SessionService::instance(), SIGNAL(beginSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef, QSharedPointer<Origin::Services::Session::AbstractSessionConfiguration>)),
                this, SLOT(onBeginSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef, QSharedPointer<Origin::Services::Session::AbstractSessionConfiguration>)));
            ORIGIN_VERIFY_CONNECT(
                Session::SessionService::instance(), SIGNAL(endSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef)),
                this, SLOT(onEndSessionComplete(Origin::Services::Session::SessionError)));
        }

        LoginController::Error toLoginControllerError(Session::SessionError error)
        {
            switch (error.errorCode())
            {
            case Session::SESSION_NO_ERROR:
                return LoginController::LOGIN_SUCCEEDED;

            case Session::SESSION_FAILED_IDENTITY:
                {
                    switch (error.restErrorCode())
                    {
                        case restErrorIdentityGetTokens:
                        case restErrorIdentityRefreshTokens:
                        case restErrorIdentityGetUserProfile:
                        case restErrorIdentityGetUserPersona:
                        case restErrorAtomGetAppSettings:
                        //Nucleus 2.0 authentication errors
                        case restErrorNucleus2InvalidRequest:
                        case restErrorNucleus2InvalidClient:
                        case restErrorNucleus2InvalidGrant:
                        case restErrorNucleus2InvalidScope:
                        case restErrorNucleus2InvalidToken:
                        case restErrorNucleus2UnauthorizedClient:
                        case restErrorNucleus2UnsupportedGrantType:
                        default:
                            return LoginController::LOGIN_ERROR_BAD_AUTHENTICATION;
                    };
                }

            case Session::SESSION_CREDENTIAL_AUTHENTICATION_FAILED:
                return LoginController::LOGIN_ERROR_BAD_AUTHENTICATION;

            case Session::SESSION_UNABLE_TO_AUTOLOGIN:
            case Session::SESSION_TOKEN_AUTHENTICATION_FAILED:
                return LoginController::LOGIN_ERROR_BAD_TOKEN_AUTHENTICATION;

            case Session::SESSION_FAILED_UNKNOWN:
                return LoginController::LOGIN_ERROR_UNKNOWN;

            case Session::SESSION_NETWORK_ERROR:
                return LoginController::LOGIN_ERROR_NETWORK_UNAVAILABLE;

            case Session::SESSION_OFFLINE_AUTHENTICATION_FAILED:
                return LoginController::LOGIN_ERROR_OFFLINE_AUTHENTICATION_FAILED;

            case Session::SESSION_OFFLINE_AUTHENTICATION_UNAVAILABLE:
                return LoginController::LOGIN_ERROR_OFFLINE_AUTHENTICATION_UNAVAILABLE;

            case Session::SESSION_MUST_CHANGE_PASSWORD:
                return LoginController::LOGIN_ERROR_MUST_CHANGE_PASSWORD;

            case Session::SESSION_FAILED_OLC_NOT_FOUND:
                return LoginController::LOGIN_ERROR_OLC_NOT_FOUND;

            case Session::SESSION_UPDATE_PENDING:
                return LoginController::LOGIN_ERROR_UPDATE_PENDING;
                    
            case Session::SESSION_USER_NOT_IN_MAC_TRIAL:
                    return LoginController::LOGIN_ERROR_ACCOUNT_NOT_IN_TRIAL;

            case Session::SESSION_FAILED_EXTRACT_CID:
                    return LoginController::LOGIN_ERROR_FAILED_EXTRACT_CID;

            case Session::SESSION_FAILED_NUCLEUS:
                    return LoginController::LOGIN_ERROR_NUCLEUS;
                    
            default:
                return LoginController::LOGIN_ERROR_UNKNOWN;
            }
        }

#ifdef _DEBUG
        LogoutCleanupChecker::LogoutCleanupChecker(UserWRef user)
            : QObject(NULL)
            , mUser(user)
        {
            // Confirm that after 10 seconds the user object is cleaned up
            QTimer::singleShot(10 * 1000, this, SLOT(onConfirmUserCleanup()));
        }

        void LogoutCleanupChecker::onConfirmUserCleanup() 
        {
            // :TODO: this assert is firing for x/DL right now. Need to fix!
            // check that the current user object has been destroyed
            ORIGIN_ASSERT_MESSAGE(!mUser.toStrongRef(), "User object not destroyed after logout");
            deleteLater();
        }
#endif
    }
}
