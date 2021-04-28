///////////////////////////////////////////////////////////////////////////////
// RTPFlow.h (Required To Play)
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef RTPFLOW_H
#define RTPFLOW_H

#include "flows/AbstractFlow.h"
#include "qstringlist.h"
#include "engine/login/LoginController.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "engine/content/Entitlement.h"
#include "services/log/LogService.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class RTPViewController;

        /// \brief Handles all high-level actions related to RTP.
        class ORIGIN_PLUGIN_API RTPFlow : public AbstractFlow
        {
            Q_OBJECT

        public:

            /// \brief RTPFlowStatus different states we are tracking (currently used by local host)
            enum RTPFlowStatus
            {
                GameLaunchInactive,
                GameLaunchCheckingLogin,
                GameLaunchWaitingToLogin,
                GameLaunchDownloading,
                GameLaunchPending,
                GameLaunchSuccess,
                GameLaunchFailed,
                GameLaunchAlreadyPlaying
            };

            enum RTPPendingEntitlementFlags
            {
                RTP_NOTWAITING = 0,
                RTP_WAITINGFOR_UPDATE = 0x1,
                RTP_WAITINGFOR_EXTRACONTENT = 0x2,
            };

            /// \brief The RTPFlow constructor.
            RTPFlow();

            /// \brief The RTPFlow destructor.
            virtual ~RTPFlow();

            /// \brief Public interface for starting the RTP flow.
            virtual void start();

            /// \brief Checks if a RTP game is launching.
            /// \param contentId The content ID of the game to check for, if any.
            /// \return True if a RTP game is launching.
            bool  getRtpLaunchActive(const QString contentId = QString());

            /// \brief Gets any parameters for the given RTP game.
            /// \param contentId The content ID of the game to check.
            /// \return The launch parameters of the given RTP game.
            QString getRtpLaunchParams(const QString contentId);

            /// \brief Clears any game-specific launch info.
            /// \param contentId The content ID of the game to clear info of.
            void clearRtpLaunchInfo(const QString contentId);

            /// \brief Gets list of all content or offer IDs associated with the launching game.
            /// \return The list of all content or offer IDs associated with the launching game.
            const QStringList& getLaunchGameIdList() const { return mLaunchGameIdList; }

            /// \brief Gets the title of the launching game.
            /// \return The title of the launching game.
            const QString& getLaunchGameTitle() const { return mLaunchGameTitle; }

            /// \brief Gets the launch parameters of the launching game.
            /// \return The launch parameters of the launching game.
            const QString& getLaunchCmdParams() const { return mLaunchGameCmdParams; }

            /// \brief Checks if any RTP title is pending.
            /// \return True if any RTP title is pending.
            bool isPending()
            {
                return !(mLaunchGameIdList.isEmpty() || mLaunchGameTitle.isEmpty());
            }

            /// \brief Check if product page is shown via the 'Free Trial Expired' dialog
            bool getShowProductPage() const { return mShowProductPage; }

            /// \brief Checks if the client should start minimized.
            /// \return True if the client should start minimized.
            bool startClientMinimized ();

            /// \brief Sets information about the title that is pending launch.
            /// \param idList List of all content or offer IDs associated with the game.
            /// \param gameTitle The title of the game.
            /// \param cmdParams Any launch parmeters to send to the game executable.
            /// \param autoDownload  True if the title should auto-download on Origin start.
            /// \param gameRestart True if the title is restarting itself.
            /// \param itemSubType If present will be a string containing integer matching Origin::Services::Publishing::ItemSubType for the game being launched.  Only from jumplist launches
            /// \param id An optional id used to track the pending launch. Currently used only by localhost
            /// \param forceOnline if the client should be forced into Online mode when launching this entitlement
            void setPendingLaunch(const QStringList& idList, const QString& gameTitle, const QString& cmdParams, bool autoDownload = false, bool gameRestart = false, const QString& itemSubType = "", const QString &id = "", bool forceOnline = false);

            /// \brief Attempts to launch the currently pending game.
            void attemptPendingLaunch() { attemptGameLaunch(mLaunchGameFailedAction); }

            /// \brief Sets whether the product has been redeemed.
            /// \param codeRedeemed True if the product has been redeemed.
            void setCodeRedeemed(bool codeRedeemed) { mbCodeRedeemed = codeRedeemed; }

            /// \brief Gets whether the product has been redeemed.
            /// \return True if the product has been redeemed.
            bool getCodeRedeemed() { return mbCodeRedeemed; }

            /// \brief Gets whether the client is launching in forceOnline mode. If so single login dialogs are disabled for the length of the RTP flow.
            /// \return True if the RTP flow is in force online mode
            bool isInForceOnlineMode() { return mForceOnline; }

            /// \brief Return true if the Origin window should be minimized as a result of
            /// the RTP game launch
            bool shouldMainWindowMinimize() const;

            /// \brief Return the status of the RTP launch
            RTPFlowStatus status() { return mRTPStatus; }

            /// \brief Returns the uuid associated with this launch
            QString uuid() {return mUUID; }

            /// \brief Sets the mAutoDownload flag
            void setAutoDownload(bool flag);

            /// \brief Called when a user's credentials are expired and need to be relogged in. Clears a connection we make when forceOnline is true.
            void clearManualOnlineSlot();

        signals:
            /// \brief Emitted when the RTP flow is finished.
            /// \param result The result of the RTP flow.
            void finished(RTPFlowResult result);

            void entitlementFound();

        private slots:
            /// \brief Slot that is triggered when the auto-update check completes.
            /// \param autoUpdating True if the game is updating.
            /// \param autoUpdateList List of content IDs that are auto-updating.
            void onAutoUpdateCheckCompleted(bool autoUpdating, QStringList autoUpdateList);

            void onExtraContentUpdate(); ///< Slot that is triggered when the extra content controller updates.

            void onEntitlementContentUpdated(); ///< Slot that is triggered when the content controller updates.
            void onRetryGameStart(Origin::Engine::Content::EntitlementRef entitlementRef); ///< Call this when the trail time has updated.

            void onRedeemButton(); ///< Slot that is triggered when the user clicks "Redeem" on the RTP dialog.
            void onNotThisUserButton(); ///< Slot that is triggered when the user clicks "Not this User" on the RTP dialog.
            void onPurchaseGame(const QString& masterTitleId); ///< Slot that is triggered when the user clicks "Buy in Store" on the RTP dialog.
            void onRenewSubscriptionMembership(); ///< Slot that is triggered when the user clicks "Renew Subscription Membership" on the RTP dialog.
            void onClientFlowStarted(); ///< Slot that is triggered when the client flow starts.
            void onGoOnline(); ///< Requests for the client to go online

            void onUserForcedOnline(bool isOnline, Origin::Services::Session::SessionRef session); ///< Slot that is triggered when the user goes online

            /// \brief Slot that is triggered when the download state changes.
            /// \param entitlementRef The entitlement that is downloading.
            void onDownloadStateChanged (Origin::Engine::Content::EntitlementRef entitlementRef);

            /// \brief Slot that is triggered when the install flow state changes.
            /// \param newState The new install flow state.
            void onInstallFlowStateChanged (Origin::Downloader::ContentInstallFlowState newState);

            void onRedemptionPageShown();
            void onRedemptionPageClosed();
            void onCancelLaunch();
            void onShowDeveloperTool();

        private:
            /// \brief Determines if the dialog will be shown in case of failure to launch.
            typedef enum {
                ifLaunchFailsDoNothing, ///< If launch fails, do nothing.
                ifLaunchFailsShowDialog, ///< If launch fails, show dialog.
                ifLaunchFailsTryDownload ///< If launch fails, try download.
            } LaunchFailedAction;

            /// \brief Attempts to launch the RTP game.
            /// \param aLaunchFailedAction Action to take if the launch fails.
            bool attemptGameLaunch(LaunchFailedAction aLaunchFailedAction);

            /// \brief Show the Activate dialog for when no RTP entitlement was found.
            void showActivateDialog();

            /// \brief Clears the pending launch info.
            void clearPendingLaunch();

            /// \brief Sets whether we should show the My Games tab.
            /// \param offerId Offer ID of the game to scroll to.
            void showGamesTabNowOrOnLogin( const QString offerId );

            /// \brief Clears redemption flag, disconnects redemption dialog slots
            void clearRedemptionStates();

            RTPViewController *mRTPViewController; ///< Pointer to the RTP view controller.

            QStringList mLaunchGameIdList; ///< List of all content or offer IDs associated with the RTP game.
            QString mLaunchGameTitle; ///< Title of the RTP game.
            QString	mLaunchGameCmdParams; ///< Launch parameters to send to the RTP game executable.
            bool mLaunchGameRestart; ///< Indicates that the entitlement is restarting itself with this command, which has implications for process monitoring and playing state.

            LaunchFailedAction	mLaunchGameFailedAction; ///< Action to take if the launch fails.
            bool mbCodeRedeemed; ///< True if the product has been redeemed.

            static QString sRtpLaunchCmdParams; ///< Launch parameters to send to the RTP game executable.
            static bool sRtpLaunchActive; ///< True if a RTP game is launching.
            static QString sRtpLaunchGameContentId; ///< Content ID of the RTP game that is launching.

            bool mRetryOnceAfterInitialRefresh; ///< True if we should refresh again once the user redeems the game.
            bool mAutoUpdating; ///< True if the title is auto-updating.
            bool mShowProductPage; ///< True if we will show the product page
            bool mRedemptionActive; ///< True if we are currently redeeming a code for this entitlement
            bool mForceOnline; ///< True if Origin should be forced online before running

            int mRetryCount;  ///< The number of tries we tried to restart the trial.

            /// used by local host
            RTPFlowStatus mRTPStatus; ///< status of the RTP Launch
            int mPendingEntitlementFlag; ///< are we waiting for an entitlement refresh?
            QString mUUID; ///< an arbitrary id used to identify this launch

            bool mAutoDownload; ///< True if title will try to download if not installed
        };
    }
}

#endif // RTPFLOW_H
