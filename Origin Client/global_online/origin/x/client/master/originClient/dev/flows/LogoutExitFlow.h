///////////////////////////////////////////////////////////////////////////////
// LogoutExitFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef LOGOUTEXITFLOW_H
#define LOGOUTEXITFLOW_H

#include "AbstractFlow.h"
#include "FlowResult.h"
#include "engine/content/ContentTypes.h"
#include "services/plugin/PluginAPI.h"
#include "LogoutExitViewController.h"

namespace Origin
{
    namespace Client
    {
        /// \brief Enumeration of different reasons a user may enter the logout/exit flow.
        enum ClientLogoutExitReason
        {
            ClientLogoutExitReason_Logout, ///< The user selected "logout".
            ClientLogoutExitReason_Logout_Silent, ///< The user is being logged out silently.
            ClientLogoutExitReason_Logout_Silent_BadEncryptedToken,  ///< The user is being logged out silently because encrypted token is bad, need to distinguish from just plain silent to show correct eror msg
            ClientLogoutExitReason_Logout_Silent_CredentialsChanged, ///< The user is being logged out silently because we received notification from DirtyBits that their credentials have changed, need to distinguish from just plain silent error to show correct error msg
            ClientLogoutExitReason_Logout_onLoginError,             ///< The user is going back thru the login flow after encountering an error during the login process (gets triggered when cid can't be extracted)
            ClientLogoutExitReason_Exit, ///< The user selected "exit".
            ClientLogoutExitReason_Exit_NoConfirmation, ///< The client is exiting silently.
            ClientLogoutExitReason_Restart ///< The user wanted to restart.
        };
        
        /// \brief Handles all high-level actions related to the logout/exit flow.
        class ORIGIN_PLUGIN_API LogoutExitFlow : public AbstractFlow
        {
            Q_OBJECT

        public:
            /// \brief The LogoutExitFlow constructor.
            /// \param aReason The reason for entering this flow.
            LogoutExitFlow(ClientLogoutExitReason aReason);

            /// \brief Gets the reason for entering this flow.
            /// \return The reason.
            ClientLogoutExitReason reason() { return mReason; }
            
            /// \brief The public interface for starting the logout/exit flow.
            virtual void start();

            ///
            /// returns false if we are in the middle of operation that should block the user from logging out, e.g. installing a game
            ///
            static bool canLogout ();

            ///
            /// returns true if we've gone thru all the checks and confirmation (if applicable) and have started the logout process
            ///
            bool confirmedAndStartedLoggingOut() { return mIsActuallyLoggingOut; }

            ///
            /// clears out the rememberme cookie so it won't load it on next login attempt
            ///
            static void clearRememberMe ();


            ///
            /// returns true if mReason is some type of exit (as opposed to logging out)
            ///
            bool isExiting() { return (mReason == ClientLogoutExitReason_Exit || mReason == ClientLogoutExitReason_Exit_NoConfirmation);}



        signals:
            /// \brief Emitted when the logout/exit flow has completed.
            /// \param result The result of the logout/exit flow.
            void finished(LogoutExitFlowResult result);

        private slots:
            void checkForCloudSaves(); ///< Checks for any active cloud saves.
            void checkForInstallations(); ///< Checks for any active installations.
            void checkForPlayingGame(); ///< Checks for any games playing.
            void checkForODT(); ///< Checks if the ODT window is open.  Attempts to close if so.
            void beginLogoutExit(); ///< actually start the logout/exit process
            void pauseActiveInstallFlows(); ///< Suspends all active install flows.
            void cleanupInstallFlows(); ///< Clean up all the install flows.
            void logoutExitSuccess(); ///< Sends telemetry and emits finished.
            void logoutExitCancelled(); ///< Emits finished with a result of USER_CANCELLED.

            void onCancelCloudSync();   ///< user chose to cancel out of cloud sync, so proceed with logout
            void onCancelLogoutFromCloudSync(); ///< user closed the cloud sync dialog, so cancel the logout/exit flow

        private:
            void sendTelemetry(); ///< Sends any pending telemetry prior to exiting or logging out.
            void doLogoutTasks(); ///< Completes tasks related to logout.
            void doExitTasks(); ///< Completes tasks related to exiting.

            /// \brief Returns true if there is an SDK game or PI game being played.
            /// \param gamename Reference that contains the name of the game being played.
            bool isOriginRequiredGameBeingPlayed(QString& gamename, const QList<Origin::Engine::Content::EntitlementRef>& playing);

            QScopedPointer<LogoutExitViewController> mLogoutExitViewController; ///< The object that controls the logout/exit GUI.
            ClientLogoutExitReason mReason; ///< Reason for entering this flow.
            /// \brief set to true when we've actually gone thru all the checks and have started the logout process
            bool mIsActuallyLoggingOut;
        };
    }
}

#endif // LOGOUTEXITFLOW_H
