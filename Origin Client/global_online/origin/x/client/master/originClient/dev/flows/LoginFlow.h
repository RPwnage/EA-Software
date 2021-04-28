///////////////////////////////////////////////////////////////////////////////
// LoginFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef LOGINFLOW_H
#define LOGINFLOW_H

#include "flows/AbstractFlow.h"
#include "services/session/SessionService.h"
#include "services/plugin/PluginAPI.h"
#include "engine/login/LoginCredentials.h"
#include "engine/login/LoginController.h"
#include "widgets/login/source/LoginViewController.h"

#include "services/rest/AuthenticationData.h"

namespace Origin
{
    namespace Client
    {
        class LoginViewController;
        
        /// \brief Handles all high-level actions related to the login flow.
        class ORIGIN_PLUGIN_API LoginFlow : public AbstractFlow
        {
            Q_OBJECT

        public:
            /// \brief The LoginFlow constructor.
            /// \param startupState How the login window will be show to the user.
            /// \param initialLoginErrorMessage The login error message to display initially.
            LoginFlow(const StartupState& startupState = StartNormal, QString const& initialLoginErrorMessage=QString(), const Origin::UIToolkit::OriginBanner::IconType initialIcon = Origin::UIToolkit::OriginBanner::IconUninitialized);

            /// \brief Public interface for starting the login flow.
            virtual void start();

            /// \brief Restarts the login flow.
            /// \return True if able to restart.
            bool restart(QString const& initialLoginErrorMessage=QString());

            /// \brief Returns if the login dialog is current up
            /// \return True if showing
            bool isLoginDialogShowing();

            /// \brief returns if we're in the process of auto-login
            bool autoLoginInProgress();

            /// \brief Returns if the login flow is currently processing a login request
            /// \return True if the user is in the procress of logging in
            bool isLoginRequestInProcess() { return mLoginRequestInProcess; }

            void setLoginBanner(UIToolkit::OriginBanner::IconType icon, const QString& text);

            void setShowAgeUpFlow (bool show);

        signals:
            /// \brief Emitted when when the login flow has finished.
            /// \param result The result of the login flow.
            void finished(LoginFlowResult result);

            /// \brief Emitted when when the login flow has started.
            /// \param mLoginType The type of login.
            /// \param mAutoLogin TBD.
            void typeOfLogin(const QString& mLoginType, bool mAutoLogin);

            /// \brief Emitted when a login error occurs, the signal is triggered from a signal in the view controller
            void loginError();

            /// \brief Emitted when an underage user has completed the age up flow during login
            void ageUpFlowCompleted();

        protected:


        protected slots:
            /// \brief Slot that is triggered when the login is complete.
            /// \param error The error encountered, if any.
            void onLoginComplete(Origin::Engine::LoginController::Error error);
            void onUserLoggedIn(Origin::Engine::UserRef);
            void onTypeOfLogin(const QString&, bool autoLogin);

            /// \brief Slot that is triggered whent he user cancels login.
            void onUserCancelled();
            void onPortalLoginComplete(const QString& authorizationCode, const QString& idToken);
            void onOfflinePortalLoginComplete(const QString& primaryOfflineKey, const QString& backupOfflineKey);

            //JS void onLoginError();

        private:

            QScopedPointer<LoginViewController> mLoginViewController; ///< Pointer tot he login view controller.
            QString mInitialLoginErrorMessage; ///< The login message to show initially.
            bool mLoginRequestInProcess; ///< True if a backend login request is in process. (it's already gone thru EADP login)
            const StartupState mStartupState; ///< StartNormal How the login window will be show to the user.
            Origin::UIToolkit::OriginBanner::IconType mInitialIcon;
        };
    }
}

#endif // LOGINFLOW_H
