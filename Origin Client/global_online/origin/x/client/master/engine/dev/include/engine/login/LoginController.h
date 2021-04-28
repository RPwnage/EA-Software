//  LoginController.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef LOGINCONTROLLER_H
#define LOGINCONTROLLER_H

#include "services/session/LoginRegistrationSession.h"
#include "services/session/AbstractSessionCredentials.h"
#include "services/plugin/PluginAPI.h"
#include "engine/login/User.h"

#include <QtCore>

namespace Origin
{
    namespace Engine
    {
        /// \brief Defines the API for creating and ending a login session. 
        class ORIGIN_PLUGIN_API LoginController : public QObject
        {
            Q_OBJECT

        public:

            /// \brief Enumeration of errors that could occur during login.
            enum Error
            {
                LOGIN_SUCCEEDED, ///< The login succeeded.
                LOGIN_ERROR_GENERIC, ///< An unspecific login error occurred.
                LOGIN_ERROR_UNKNOWN, ///< An unknown login error occurred.
                LOGIN_ERROR_NO_SESSION, ///< The current session is invalid.
                LOGIN_ERROR_OFFLINE, ///< The network connection is offline
                LOGIN_ERROR_BAD_AUTHENTICATION, ///< The user entered invalid credentials.
                LOGIN_ERROR_BAD_TOKEN_AUTHENTICATION, ///< The user attempted to log in with a bad token.
                LOGIN_ERROR_BAD_SSO_AUTHORIZATION, ///< The user attempted to log in with a bad SSO token.
                LOGIN_ERROR_STATUS_UNAVAILABLE, ///< TBD.
                LOGIN_ERROR_NETWORK_UNAVAILABLE, ///< No network connection detected.
                LOGIN_ERROR_ACCOUNT_PENDING, ///< The account is still pending.
                LOGIN_ERROR_ACCOUNT_BANNED, ///< The account has been banned.
                LOGIN_ERROR_ACCOUNT_DEACTIVATED, ///< The account has been deactivated.
                LOGIN_ERROR_ACCOUNT_DISABLED, ///< The account has been disabled.
                LOGIN_ERROR_ACCOUNT_DELETED, ///< The account has been deleted.
                LOGIN_ERROR_OFFLINE_AUTHENTICATION_FAILED, ///< Offline cache exists but couldn't decrypt the cache.
                LOGIN_ERROR_OFFLINE_AUTHENTICATION_UNAVAILABLE, ///< No offline cache exists.
                LOGIN_ERROR_MUST_CHANGE_PASSWORD, ///< The user must change his password (bonehead password)
                LOGIN_ERROR_OLC_NOT_FOUND,       //OLC file not found, request credentials, separate from AUTHENTICATION_UNAVAILABLE because we're actually not offline and  don't want to
                                                //prompt an error message tha says you must be online for the first time, we just want to request credentials
                LOGIN_ERROR_UPDATE_PENDING,      //used to request credentials if logging in via auto-login or SSSO when update is pending
                LOGIN_ERROR_ACCOUNT_NOT_IN_TRIAL,    //special messed to inform user they are not part of the Mac Alpha Trial
                LOGIN_ERROR_FAILED_EXTRACT_CID,        //unable to extract the CID from idToken, need to put user thru offline login flow
                LOGIN_ERROR_NUCLEUS, ///< Unable to retrieve an authorization code from nucleus.
            };

            /// \brief Prepares the LoginController for use.
            static void init();

            /// \brief Frees up the LoginController.
            static void release();

            /// \brief Returns the current LoginController instance.
            /// \return The current LoginController instance.
            static LoginController* instance();

            /// \brief Initiates a token refresh, which if it works, makes the login screen unnecessary
            static void renewAccessTokenAsync();

            static void nucleus20LoginAsync(const QString& authorizationCode, const QString& idToken);
            static void nucleus20OfflineLoginAsync(const QString& primaryOfflineKey, const QString& backupOfflineKey);
            static void loginType(const QString& loginType, bool autoLogin);

            /// \brief Logs out the current session, if any.
            /// \param session The current session.
            static void logoutAsync(Origin::Services::Session::SessionRef session);

            /// \brief Returns true if any users are logged in, false otherwise.
            /// \return True if any users are logged in, false otherwise.
            static bool isUserLoggedIn();

            /// \brief Returns the currently logged in user.
            /// \note This API may be deprecated in the future if we support multiple simultaneous logins.
            /// \return The currently logged in user.
            static UserRef currentUser();

            /// \brief Returns whether this session was started from an auto login.
            /// \return True if the current session was started from an auto login.
            static bool isCurrentSessionAutoLogin();

        signals:

            /// \brief This signal is triggered when a login request completes.
            /// \param error The error encountered, if any.
            void loginComplete(Origin::Engine::LoginController::Error error);

            /// \brief This signal is triggered when a logout request completes.
            /// \param error The error encountered, if any.
            void logoutComplete(Origin::Engine::LoginController::Error error);

            /// \brief This signal is triggered when a user has logged in.
            /// \param user The user that has logged in.
            void userLoggedIn(Origin::Engine::UserRef user);

            /// \brief This signal is triggered when a user is about to log out.
            /// \param user The user that is logging out.
            void userAboutToLogout(Origin::Engine::UserRef user);

            /// \brief This signal is triggered when a user has logged out.
            /// \param user The user that has logged out.
            void userLoggedOut(Origin::Engine::UserRef user);

            /// \brief This signal is triggered when the token refresh attempt has completed.
            /// \param error The error encountered, if any.
            void tokenRefreshComplete(Origin::Engine::LoginController::Error error);

            /// \brief This signal is triggered when the begin session is complete. It emits the type of login that just completed
            /// \param QString The type of login (e.g. sso, etc).
            void typeOfLogin(QString);

        protected slots:

            /// \brief This slot gets called when a request to start a session authenticated using an SSO token has started.
            /// \param error The error encountered, if any.
            /// \param sessionRef The current session.
            /// \param sessionConfiguration TBD.
            void onBeginSessionComplete(
                Origin::Services::Session::SessionError error, 
                Origin::Services::Session::SessionRef sessionRef,
                QSharedPointer<Origin::Services::Session::AbstractSessionConfiguration> sessionConfiguration);

            /// \brief This slot gets called when a session has terminated.
            /// \param error The error encountered, if any.
            void onEndSessionComplete(
                Origin::Services::Session::SessionError error);

            /// \brief This slot gets called when a session's tokens have changed.
            /// \param session The current session.
            /// \param accessToken The auth token.
            /// \param refreshToken The encrypted token.
            /// \param error The error encountered, if any.
            void onSessionTokensChanged(
                Origin::Services::Session::SessionRef session,
                Origin::Services::Session::AccessToken accessToken,
                Origin::Services::Session::RefreshToken refreshToken,
                Origin::Services::Session::SessionError error);

            /// \brief called when session request relogin to be initiated
            /// \param session that is requesting the relogin
            void onBeginReloginAsync (Origin::Services::Session::SessionRef session);

        private:

            /// \brief The LoginController constructor.
            LoginController();

            /// \brief The LoginController copy constructor.
            /// \param from The LoginController object to copy from.
            LoginController(LoginController const& from);

            /// \brief The LoginController "=" operator.
            /// \param from The LoginController object to copy from.
            /// \return The LoginController object that was copied to.
            LoginController& operator=(LoginController const& from);

            QString mTypeOfLogin;
            bool mAutoLogin;

            UserRef mCurrentUser; ///< Reference to the current user.
        };

#ifdef _DEBUG
        class ORIGIN_PLUGIN_API LogoutCleanupChecker : public QObject
        {
            Q_OBJECT
        public:
            LogoutCleanupChecker(UserWRef user);

        protected slots:
            void onConfirmUserCleanup();

        private:
            UserWRef mUser;
        };
#endif
    }
}

#endif // LOGINCONTROLLER_H
