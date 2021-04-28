///////////////////////////////////////////////////////////////////////////////
// MainFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef MAINFLOW_H
#define MAINFLOW_H

#include "AbstractFlow.h"
#include "FlowResult.h"
#include "LogoutExitFlow.h"
#include "SSOFlow.h"

#include "engine/content/Entitlement.h"
#include "engine/backup/GuardFile.h"
#include "services/plugin/PluginAPI.h"

#include <qpointer.h>
#include <qscopedpointer.h>
#include <qmap.h>

namespace Origin
{
    namespace Services
    {
        class Setting;
        class Variant;
    }

    namespace Client
    {
        class FirstLaunchFlow;
        class LoginFlow;
        class LogoutExitFlow;
        class ClientFlow;
        class MOTDFlow;
        class SingleLoginFlow;
        class ContentFlowController;
        class RTPFlow;
        class SSOFlow;
        class InstallerFlow;
        class NonOriginGameFlow;
        class FeatureCalloutController;
        class DirtyBitsTrafficDebugView;

        /// \brief Handles all high-level actions related to the main flow.
        class ORIGIN_PLUGIN_API MainFlow : public AbstractFlow
        {
            Q_OBJECT

        public:
            /// \brief Initiates the flow execution.
            virtual void start();

            /// \brief Creates a new MainFlow object.
            static void create();

            /// \brief Destroys the current MainFlow object.
            static void destroy();

            /// \brief Gets the current MainFlow object.
            /// \return The current MainFlow object.
            static MainFlow* instance();

            /// \brief Handles a message (command line args for example).
            /// \param message The message to handle.
            /// \return True if the message was handled.
            Q_INVOKABLE bool handleMessage(QString const& message);

            /// \brief Gets the ContentFlowController.
            /// \return The ContentFlowController.
            ContentFlowController* contentFlowController();

            /// \brief Gets the FeatureCalloutController.
            /// \return The FeatureCalloutController.
            FeatureCalloutController* featureCalloutController();

            /// \brief Shows the dirty bits traffic tool. Because this isn't user session dependent
            /// we want to put this here instead of the clientflow.
            void showDirtyBitsTrafficTool();

            /// \brief Gets the RTPFlow.
            /// \return The RTPFlow.
            RTPFlow* rtpFlow();

            /// \brief Attemps to go online.
            /// \todo What does this have to do with social?
            void socialGoOnline();

            /// \brief Attemps to go offline.
            /// \todo What does this have to do with social?
            void socialGoOffline();

            /// \brief Triggers the logout flow.
            /// \param reason The reason for the logout.
            void logout(ClientLogoutExitReason reason);

            /// \brief Triggers the exit flow.
            /// \param reason The reason for exiting.
            void exit(ClientLogoutExitReason reason = ClientLogoutExitReason_Exit);

            /// \brief Triggers the restart flow.
            /// \param reason The reason for restarting.
            Q_INVOKABLE void restart(ClientLogoutExitReason reason = ClientLogoutExitReason_Restart);

            /// \brief Translates a URL parameter into a settings value.
            /// \param setting The setting to populate.
            void consumeUrlParameter (const Origin::Services::Setting &setting);

            /// \brief Starts the non-Origin game remove flow.
            ///
            /// \param game The non-Origin game whose properties need to be removed.
            void removeNonOriginGame(Origin::Engine::Content::EntitlementRef game);

            /// \brief Maximize client window when called
            void reactivateClient ();

            /// \brief Returns whether we have previously been through the login flow
            bool firstTimeThroughLogin() const {return mFirstTimeThroughLogin;}

            void setNewUser(bool setting);

            bool newUser() const {return mNewUser;}

            /// \brief Returns true if an under age user completed the age up flow during login
            bool wasAgeUpFlowCompleted() const;

            /// \brief Starts the SSO flow
            Q_INVOKABLE void startSSOFlow (Origin::Client::SSOFlow::SSOLoginType loginType);

            bool ssoFlowActive ();

            void showAddGameWindow();

            void setShowAgeUpFlow();

            /// \brief Starts the login flow.
            /// \param initialLoginMessage The error message to show initially.
            void startLoginFlow(QString const& initialLoginMessage = QString(), const int iconType = 0);

            /// \brief Returns the single login flow.
            SingleLoginFlow* singleLoginFlow();
        signals:
            /// \brief Emitted when received close event from OS (PC)
            void gotCloseEvent ();

            /// \brief Emitted when the ITO flow should start.
            void startITOviaCommandLine();

            /// \brief Emitted when the pending action should start.
            void startPendingAction();

            /// \brief Emitted when the user has logged out.
            void logoutConcluded();

            /// \brief Emitted when SSOFlow succeeded and user logged in.
            void ssoLoggedIn();

            /// \brief Emitted when SSOFlow succeeded and user logged in.
            void singleLoginFlowFinished();

            /// \brief Emitted when single login flow shows the Account in Use dialog
            void singleLoginAccountInUse();

            /// \brief Emitted when there is a login error, signal triggered by the signal in LoginViewController
            void loginError();

            /// \brief Emitted when we cannot determine if the requested SSO user is already logged in (triggered by signal in LoginViewController)
            void ssoIdentifyError();

            /// \brief Emitted when the SSOFlow is complete
            void ssoFlowComplete(SSOFlowResult, bool);

        protected slots:
            /// \brief Slot that is triggered when the login flow finishes.
            /// \param result The result of the login flow.
            void onLoginFlowFinished(LoginFlowResult result);

            /// \brief Slot that is triggered when the logout/exit flow finishes.
            /// \param result The result of the logout/exit flow.
            void onLogoutExitFlowFinished(LogoutExitFlowResult result);

            /// \brief Slot that is triggered when the logout/exit WebView from EADP loads.
            void onLogoutExit();

            /// \brief Slot that is triggered when the client flow finishes.
            /// \param result The result of the client flow.
            void onClientFlowFinished(ClientFlowResult result);

            /// \brief Slot that is triggered when the single login flow finishes.
            /// \param result The result of the single login flow.
            void onSingleLoginFlowFinished(SingleLoginFlowResult result);

            /// \brief Slot that is triggered when the RTP flow finishes.
            /// \param result The result of the RTP flow.
            void onRTPFlowFinished (RTPFlowResult result);

            /// \brief Slot that is triggered when the SSO flow finishes.
            /// \param result The result of the SSO flow.
            void onSSOFlowFinished (SSOFlowResult result);

            /// \brief Slot that is triggered when the non-Origin Game flow finishes.
            /// \param result The result of the non-Origin Game flow.
            void onNonOriginGameFlowFinished (NonOriginGameFlowResult result);

            /// \brief Slot that is triggered when user loses authetication
            void onLostAuthentication();

            /// \brief Slot that is triggered when token renewal fails
            void onFailedTokenRenewal();

            /// \brief Slot that is triggered when we get dirtybit message that credentials (e-mail/originId/password) have changed
            void onCredentialsChanged();

            /// \brief Slot that is triggered when the user requests to logout.
            void onRequestLogout();

            /// \brief Slot that is triggered when the user requests to exit.
            void onRequestExit();

            /// \brief Slot that is triggered when we want to quietly exit.
            void onRequestSilentExit();

            /// \brief Slot that is triggered when token renewal succeeds but we still need to go thru single login check before user is fully authenticated
            void onReAuthentication ();

            /// \brief Slot that is triggered when the user requests to add a non origin game to the library.
            void onAddNonOriginGame();

            /// \brief continues exit flow after signing out of EADP
            void onContinueExitFlow(bool);

            /// \brief Triggered when an underage user completes the age up flow during login
            void onAgeUpFlowCompleted();

            /// \brief slot triggered when received exit from OS
            void onGotAppCloseEvent ();

            /// \brief slot triggered when Dirty Bits Traffic Tool is closed
            void onDirtyBitsTrafficToolClosed();

            /// \brief schedule a restart
            /// \param entitlement The entitlement taking part in the restart.
            /// \param forceUpdateGame Force the game to update, even if it is not configured as such.
            /// \param forceUpdateDLC Force DLC to be updated to the latest version. [not implemented]
            /// \param commandLineArgs Command line arguments to give to the restarting game.
            void scheduleRestart(Origin::Engine::Content::EntitlementRef entitlement, bool forceUpdateGame, bool forceUpdateDLC, QString commandLineArgs);

        private:
            /// \brief The MainFlow constructor.
            MainFlow();

            /// \brief The MainFlow destructor.
            virtual ~MainFlow();

            /// \brief Restarts the login flow.
            void restartLoginFlow(QString const& restartLoginMessage = QString());

            /// \brief Starts the logout/exit flow.
            /// \param reason The reason for entering the logout/exit flow.
            void startLogoutExitFlow(ClientLogoutExitReason reason);

            void startFirstLaunchFlow(); ///< Starts the first launch flow.
            void startContentFlowController(); ///< Starts the first launch flow.
            void startClientFlow(); ///< Starts the client flow.
            void startMOTDFlow(); ///< Starts the MOTD flow.
            void startRTPFlow(); ///< Starts the RTP flow.
            void startSingleLoginFlow(); ///< Starts the single login flow.

            /// \brief Checks if the client flow has started.
            /// \return True if the client flow has started.
            bool isClientFlowStarted() const;

            ///\brief handles logging out the user when we need to re-authenticate them
            ///\param reason = to show proper message on the re-login window
            void authenticationRequired(ClientLogoutExitReason reason);

            /// \brief Loads EADP URL to signal Single Sign Out flow.
            /// \param reason Exit, logout, or other reason.
            void signOutEADP(ClientLogoutExitReason reason);

            /// \brief Notifies the RTP flow of a pending launch.
            /// \param id optional uuid used to identify this RTP launch (used by local host).
            void setPendingRTPlaunch(const QString& id = "");

            void testPrematureTermination();

            QScopedPointer<FirstLaunchFlow> mFirstLaunchFlow; ///< Pointer to the FirstLaunchFlow object.
            QScopedPointer<ContentFlowController> mContentFlowController; ///< Pointer to the ContentFlowController object.
            QScopedPointer<LoginFlow> mLoginFlow; ///< Pointer to the LoginFlow object.
            QScopedPointer<LogoutExitFlow> mLogoutExitFlow; ///< Pointer to the LogoutExitFlow object.
            QPointer<ClientFlow> mClientFlow; ///< Pointer to the ClientFlow object.
            QScopedPointer<MOTDFlow> mMOTDFlow; ///< Pointer to the MOTDFlow object.
            QScopedPointer<SingleLoginFlow> mSingleLoginFlow; ///< Pointer to the SingleLoginFlow object.
            QScopedPointer<RTPFlow> mRTPFlow; ///< Pointer to the RTPFlow object.
            QScopedPointer<SSOFlow> mSSOFlow; ///< Pointer to the SSOFlow object.
            QScopedPointer<FeatureCalloutController> mFeatureCalloutController; ///< Pointer to the FeatureCalloutController object.
            QScopedPointer<DirtyBitsTrafficDebugView> mDirtyBitsTrafficTool; ///< Pointer to the DirtyBitsTrafficDebugView object.

            typedef QMap<quint32, NonOriginGameFlow *> NonOriginGameFlowMap;
            NonOriginGameFlowMap mNonOriginGameFlowsMap; ///< Map of all non-Origin game flows by operation.

            bool mFirstTimeThroughLogin; ///< used to keep track of whether the login window should be minimized if Origin started via auto-start

            bool mNewUser; // user to track whether the user is new or not
            bool mAgeUpFlowCompleted; ///< Used to track if an underage user completed the age up flow during login

            LogoutExitFlowResult mReason; /// < Used to track the logout or exit reason after logging out from EADP

            QScopedPointer<Origin::Engine::Backup::GuardFile>           mGuardfile; ///< Pointer to the guard file class.

        };
    }
}

#endif // MAINFLOW_H
