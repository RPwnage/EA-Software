///////////////////////////////////////////////////////////////////////////////
// Engine::PlayFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ENGINEPLAYFLOW_H
#define ENGINEPLAYFLOW_H

#include <QObject>
#include <QSignalTransition>
#include <QState>
#include <QStateMachine>
#include <qfinalstate.h>

#include "engine/content/localcontent.h"
#include "engine/content/UpdateCheckFlow.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            /// \brief TBD.
            enum PlayFlowState
            {
                kInvalid = -1,              ///< TBD.
                kError,                     ///< TBD.
                kCanceling,                 ///< TBD.
                kReadyToStart,              ///< TBD.
                kNonMonitoredInstall,       ///< TBD.
                kCheckForOperation,         ///< TBD.
                kCheckForUpdate,            ///< TBD.
                kCheckIGOSettings,          ///< for Mac, warn user if their IGO settings is not correct
                kUpgradeSaveGameWarning,    ///< TBD.
                kCheckForLocalSaveBackup,   ///< TBD.
                kPendingCloudSync,          ///< TBD.
                kPendingCloudSaveBackup,     ///< TBD.
                kPendingMultiLaunch,        ///< TBD.
                kPendingLaunch,             ///< TBD.
                kShow3PDDPlayDialogCheck,   ///< TBD.
                kPDLCDownloadingCheck,      ///< TBD.
                kTrialCheck,                ///< TBD.
                kFirstLaunchMessage,        ///< TBD.
                kLaunchGame,                ///< TBD.
                kLaunchAlternateUrl,        ///< launch the alternate url
                kLaunchAlternateSoftwareId, ///< launch the alternate software ID
                //if you add new states below this, you need to make sure you update the for loop in initializePlayFlowStates()
                kFinalState                 ///< TBD.
            };

            /// \brief TBD.
            class ORIGIN_PLUGIN_API PlayFlow : public QStateMachine
            {
                Q_OBJECT

                public:
                
                    /// \brief TBD.
                    Q_INVOKABLE virtual void begin();

                    /// \brief TBD.
                    Q_INVOKABLE virtual void cancel();

                    /// \brief TBD.
                    ///
                    /// \param doUpdate TBD.
                    void pauseAndSkipUpdate();

                    /// \brief TBD.
                    void playWhileUpdating();

                    /// \brief TBD.
                    void setSkipPDLCCheck();

                    /// \brief TBD.
                    ///
                    /// \param doPlay TBD.
                    void setResponseTo3PDDPlayDialog (bool doPlay);

                    /// \brief TBD.
                    //void setPendingLaunchFinished();

                    /// \brief TBD.
                    void setCloudSyncFinished(const bool& proceedToPlay);

                    /// \brief TBD.
                    void setForceSkipUpdate ();

                    void setLaunchedFromRTP(bool launchedFromRTP) { mLaunchedFromRTP = launchedFromRTP; }
                    bool launchedFromRTP() const {return mLaunchedFromRTP;}

#if defined (ORIGIN_MAC)
                    /// \brief used to specify the response from the client UI to advance the checkIGOsetting state
                    void setCheckIGOSettingsResponse(bool doPlay);
#endif

                signals:
                    //state transition signals

                    /// \brief TBD.
                    void beginFlow();

                    /// \brief TBD.
                    void nonMonitoredInstallCheck();

                    /// \brief TBD.
                    void nonMonitoredInstalled();

                    /// \brief TBD.
                    void promptUpdateConfirmation ();

                    /// \brief TBD.
                    void skipUpdate ();

                    /// \brief TBD.
                    void updatePlayNow();

                    /// \brief TBD.
                    void show3PDDPlayDialogComplete();

                    /// \brief TBD.
                    void showInstallFlowResuming();

                    /// \brief TBD.
                    void operationStarted();

                    /// \brief TBD.
                    void noOperation();

                    /// \brief Mac-specific. Used to ask the PlayViewController to prompt the user to change their system settings for IGO
                    void checkIGOSettings();

                    /// \brief used to advance checkIGOsetting state to the next one
                    void playIGOConfirmed ();

                    /// \brief TBD.
                    void cloudSyncComplete();

                    /// \brief TBD.
                    void cloudSaveBackupComplete();

                    /// \brief TBD.
                    void multiLaunchCheckComplete();

                    /// \brief TBD.
                    void pendingLaunchComplete();

                    /// \brief TBD.
                    void trialCheckComplete();

                    /// \brief TBD.
                    void firstLaunchMessageCheckComplete();

                    void upgradeSaveGameWarningComplete();
                    void showUpgradeSaveGameWarning();

                    void localBackupSaveCheckComplete();
                    void showLocalBackupSaveWarning();

                    /// \brief signal to initiate launching the alternate url
                    ///
                    /// Configured via the gameLauncherURL attribute.
                    void launchAlternateUrl();

                    /// \brief signal to initiate launching the content via separate software ID
                    ///
                    /// Configured via the alternateLaunchSoftwareId (TBD).
                    void launchAlternateSoftwareId();

                    /// \brief TBD.
                    void launchCanceled();

                    /// \brief TBD.
                    void endplayflow();
                    

                    /// \brief TBD.
                    ///
                    /// \param newState TBD.
                    void stateChanged (Origin::Engine::Content::PlayFlowState newState);

                    /// \brief Tells the user with a message saying that there is an update which is required to launch.
                    /// \param productId TBD.
                    void promptUserRequiredUpdateExists(const QString &productId);

                    /// \brief Tells the user with a message saying that there is an update available.
                    /// \param productId of the entitlement being launched.
                    void promptUserGeneralUpdateExists(const QString &productId);

                    /// \brief TBD.
                    ///
                    /// \param productId TBD.
                    /// \param bRequired Implies the update is required to launch.
                    void promptUserPlayWhileUpdating (const QString &productId);

                    /// \brief TBD.
                    ///
                    /// \param productId TBD.
                    void show3PDDPlayDialog(const QString &productId);

                    /// \brief TBD.
                    ///
                    /// \param productId TBD.
                    void showLaunchingDialog (const QString &productId);

                    /// \brief TBD.
                    ///
                    /// \param productId TBD.
                    void showMultiLaunchDialog(const QString &productId);

                    /// \brief TBD.
                    ///
                    /// \param productId TBD.
                    void showPDLCDownloadingWarning (const QString &productId);

                    /// \brief TBD.
                    ///
                    /// \param productId TBD.
                    void showStartingLimitedTrialDialog(const QString &productId);

                    /// \brief TBD.
                    ///
                    /// \param productId TBD.
                    void showFirstLaunchMessage();

                    /// \brief TBD.
                    ///
                    /// \param productId TBD.
                    void showStartingTimedTrialDialog(const QString &productId);
                    
                    /// \brief emitted when we need to show an online required dialog for free trials.
                    ///
                    /// \param productId TBD.
                    void showOnlineRequiredForTrialDialog (const QString &productId);
                    
                    /// \brief TBD.
                    ///
                    /// \param timeRemainingInMinutes TBD.
                    void showTrialEndingDialog(int timeRemainingInMinutes);

                    /// \brief TBD.
                    void pdlcDownloadCheckComplete();

                    /// \brief TBD.
                    ///
                    /// \param productId TBD.
                    void launched (Origin::Engine::Content::LocalContent::PlayResult);

                    /// \brief emitted once the alternate exe/url is launched
                    ///
                    /// \ param productId
                    void launchedAlternate(
                        const QString &productId, 
                        const QString &softwareId = QString(), 
                        const QString &launchParams = QString());

                    /// \brief TBD.
                    ///
                    /// \param productId TBD.
                    void canceled (const QString &productId);

                    /// \brief TBD.
                    void playNowWithoutUpdate();

                    /// \brief
                    void launchGameFailed(
                            Origin::Engine::Content::LocalContent::PlayResult,
                            QString const& productId);

                    void startSaveBackup(const QString& productId);

                public:

                    /// \brief The PlayFlow constructor.
                    ///
                    /// \param content TBD.
                    PlayFlow(Origin::Engine::Content::EntitlementRef content);

                    /// \brief The PlayFlow destructor; releases the resources used by a PlayFlow class instance.
                    ~PlayFlow();
                    
                    /// \brief TBD.
                    ///
                    /// \return PlayFlowState TBD.
                    PlayFlowState getFlowState();

                    /// \brief TBD.
                    void startPlayNowWithoutUpdate();

                    void startOperationStarted();

                    /// \brief TBD.
                    ///
                    /// \param launchParameters TBD.
                    void setLaunchParameters (const QString& launchParameters);

                    /// \brief TBD.
                    void clearLaunchParameters ();

                    /// \brief Clears the launcher selected by content that contains multiple launchers.
                    void clearLauncher();

                    /// \brief Multilauncher support
                    /// \return true if able to find and set launcher that matches title
                    bool setLauncherByTitle(const QString& launcherTitle);

                    /// \brief If the user has selected a launcher for content that contains multiple launchers it may be stored here.
                    /// This may be empty in which case the default should be used from the server config.
                    QString launcher() const { return mLauncher; }

                    /// \brief TBD.
                    QString launchParameters() const { return mLaunchParameters; }

                    /// \brief If true, run through the OriginClientService.
                    bool launchElevated() const { return mbLaunchElevated; }

                    /// \brief Sets any alternate launch parameters.
                    /// \param altLaunchParams The parameters to pass to the launch URL.
                    void setAlternateLaunchInviteParameters(const QString& altLaunchParams) {mAltLaunchParameters = altLaunchParams;};

                    /// \brief Clears the alternate launch parameters
                    void clearAlternateLaunchInviteParameters () {mAltLaunchParameters.clear();};

                    /// \brief Special begin function that allows us to launch a game even if it's not playable
                    void beginFromJoin();

                public slots:

                    void onLogout();

                protected:

                    /// \brief TBD.
                    ///
                    /// \param type TBD.
                    /// \return QState A pointer to ???.
                    QState* getState(PlayFlowState type);

                    /// \brief TBD.
                    ///
                    /// \param state TBD.
                    /// \return PlayFlowState TBD.
                    PlayFlowState getStateType(QState* state);

                    /// \brief TBD.
                    ///
                    /// \param type TBD.
                    void setFlowState(PlayFlowState type);

                    /// \brief TBD.
                    ///
                    /// \param type TBD.
                    void setStartState(PlayFlowState type);

                private slots:

                    /// \brief TBD.
                    void onStateChange();

                    /// \brief TBD.
                    void onReadyToStart();

                    /// \brief TBD.
                    void onCheckForOperation();

                    /// \brief TBD.
                    void onCheckForUpdate();

                    /// \brief TDB.
                    void onUpdateExists();

                    /// \brief show IGO warning for Mac
                    void onCheckIGOSettings();
                    
                    void onUpgradeSaveGameWarning();

                    /// \brief show warning for incompatitable save
                    void onCheckForLocalSaveBackup();

                    /// \brief TBD.
                    void onPendingCloudSync();

                    /// \brief TBD.
                    void onPendingCloudSaveBackup();

                    /// \brief TBD.
                    void onPendingMultiLaunch();

                    /// \brief slot called to launch an alternate URL
                    ///
                    /// Configured by gameLauncherURL
                    void onLaunchAlternateUrl ();

                    /// \brief slot called to launch the alternate software ID
                    ///
                    /// Configured by alternateLaunchSoftwareId (TBD).
                    void onLaunchAlternateSoftwareId ();

                    /// \brief slot called when the authorization code is retrieved
                    void onAuthCodeRetrieved ();

                    /// \brief TBD.
                    void onPendingLaunch();

                    /// \brief TBD.
                    void onLaunchGame();

                    /// \brief slot called when we LocalContent emits result of launch
                    void onLaunchResult (Origin::Engine::Content::LocalContent::PlayResult playResult);

                    /// \brief TBD.
                    void onCanceling();

                    /// \brief TBD.
                    void onNonMonitoredInstall();

                    /// \brief TBD.
                    void onShow3PDDPlayDialogCheck();

                    /// \brief TBD.
                    void onPDLCDownloadingCheck();

                    /// \brief TBD.
                    void onTrialCheck();

                    /// \brief TBD.
                    void onFirstLaunchMessageCheck();

                private:

                    /// \brief TBD.
                    void initializePlayFlow();

                    /// \brief TBD.
                    void initializePlayFlowStates();

                    /// \brief TBD.
                    void setInitialStartStates ();

                    /// \brief TBD.
                    void finishFlow ();

                    /// \brief handles the actual loading of the launchUrl to external browser
                    void doLaunchAlternateUrl (QString launchUrl);

                    /// \brief TBD.
                    typedef QMap<PlayFlowState, QState*> StateMap;

                    /// \brief TBD.
                    typedef QMap<QState*, PlayFlowState> ReverseStateMap;

                    /// \brief TBD.
                    StateMap mTypeToStateMap;

                    /// \brief TBD.
                    ReverseStateMap mStateToTypeMap;

                    /// \brief TBD.
                    Origin::Engine::Content::EntitlementRef mContent;

                    /// \brief TBD.
                    PlayFlowState mCurrentState;

                    /// \brief TBD.
                    QFinalState* mFinalState;

                    /// \brief For multi-launcher support, this may be set.  If not use the default server configured launcher.
                    QString mLauncher;

                    /// \brief TBD.
                    QString mLaunchParameters;

                    /// \brief Alternate launch parameters attached to the launch URL
                    QString mAltLaunchParameters;

                    /// \brief Whether to launch through the OriginClientService (elevated)
                    bool mbLaunchElevated;

                    /// \brief TBD.
                    bool mForceSkipUpdate;

                    /// \brief true if the game was started through RTP, i.e. from outside the client.
                    bool mLaunchedFromRTP;

                    /// \brief true if the game doesn't show cloud sync launch or multi launch.
                    bool mShowDefaultLaunch;

                    UpdateCheckFlow mUpdateCheckFlow;

                    /// \brief need this to kill flow if game isn't playable when begin() is called
                    bool mWasPlayableAtStart;

                    /// \brief Need this to determine whether we can launch content configured with allowMultipleInstances
                    bool mWasPlayingAtStart;
            };
        } //namespace Content
    } //namespace Engine
} // namespace Origin

#endif // PLAYFLOW_H
