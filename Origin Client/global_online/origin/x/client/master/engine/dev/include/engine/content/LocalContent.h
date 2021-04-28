//  LocalContent.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef LOCALCONTENT_H
#define LOCALCONTENT_H

#include <limits>
#include <QDateTime>
#include <QObject>
#include <QAtomicInt>
#include <QMutex>
#include <QSharedPointer>
#include <QElapsedTimer>

#include "services/settings/Setting.h"
#include "services/common/Accessor.h"
#include "services/plugin/PluginAPI.h"
#include "engine/downloader/IContentInstallFlow.h"
#include "services/process/IProcess.h"
#include "engine/content/ContentTypes.h"
#include "engine/content/Entitlement.h"

namespace Origin
{
    namespace Downloader
    {
        class IContentInstallFlow;
        class LocalInstallProperties;
    }



    namespace Engine
    {
        class User;

        namespace Content
        {
            class CloudContent;
            class PlayFlow;

            /// \brief A data structure to describe each launcher
            class ORIGIN_PLUGIN_API LauncherDescription
            {
            public:
                /// \brief Clears the data structure
                void Clear();

                /// \brief This should be a collection of localized names for this launcher
                /// Key is locale, Value is the caption
                QMap<QString, QString> mLanguageToCaptionMap;

                /// \brief The thing to launch
                QString msPath;

                /// \brief Optional parameters to pass to the launcher
                QString msParameters;

                /// \brief Optional flag to execute this launcher elevated
                bool    mbExecuteElevated;

                /// \brief Optional flag to specify that this launcher should only be listed on 64 bit OSs.
                bool    mbRequires64BitOS;

                /// \brief Optional flag to specify that this launcher should only be used by timed trials.
                bool    mbIsTimedTrial;
            };

            typedef QList<LauncherDescription> tLaunchers;



            /// \class LocalContent
            /// \brief A factory that creates the entitlement controller class.
            class ORIGIN_PLUGIN_API LocalContent : public QObject
            {
                Q_OBJECT

                friend class Entitlement;
                friend class CloudContent;
                friend class ContentController;

            public:

                /// \brief TBD.
                enum State
                {
                    Undefined = 0x0,                    ///< TBD.

                    //enums as bit flags to allow for querying multiple states in ContentController::entitlementByState
                    //but the states themselves are used as mutually exclusive
                    ReadyToDownload     = 0x1<<0,       ///< TBD.
                    Downloading         = 0x1<<1,       ///< TBD.
                    ReadyToInstall      = 0x1<<2,       ///< TBD.
                    Installing          = 0x1<<3,       ///< TBD.
                    Installed           = 0x1<<4,       ///< TBD.
                    ReadyToPlay         = 0x1<<5,       ///< TBD.
                    Playing             = 0x1<<6,       ///< TBD.
                    Busy                = 0x1<<7,       ///< TBD.
                    Paused              = 0x1<<8,       ///< TBD.
                    Preparing           = 0x1<<9,       ///< TBD.
                    FinalizingDownload  = 0x1<<10,      ///< TBD.
                    ReadyToUnpack       = 0x1<<11,      ///< TBD.
                    Unpacking           = 0x1<<12,      ///< TBD.
                    ReadyToDecrypt      = 0x1<<13,      ///< TBD.
                    Decrypting          = 0x1<<14,      ///< TBD.
                    Unreleased          = 0x1<<15,      ///< TBD.
                    NotPurchased        = 0x1<<16,      ///< TBD.
                    PrimaryStateMask    = 0xFFFF,       ///< TBD.

                    // Flags
                    Blocked = 0x80000000                ///< TBD.
                };

                /// \brief TBD.
                enum RCState
                {
                    RC_Undefined,   ///< Initial Value, we should never get this.
                    RC_Unreleased,  ///< Game is unreleased and cannot be downloaded.
                    RC_Preload,     ///< Game is ready to preload, but cannot be played.
                    RC_Released,    ///< Game is released and can be downloaded and played.
                    RC_Expired      ///< Game is expired and can no longer be played.
                };

                /// \brief TBD.
                enum Error
                {
                    ErrorNone,         ///< TBD.
                    ErrorFolderInUse,   ///< TBD.
                    ErrorTimedTrialAccount,       ///< This account has already used Timed Trial for this offer
                    ErrorTimedTrialSystem,        ///< Some other account has already used Timed Trial for this offer on this system
                    ErrorGameTimeAccount,       ///< This account has already used Game Time for this offer
                    ErrorGameTimeSystem,        ///< Some other account has already used Game Time for this offer on this system
                    ErrorCOPPADownload, //game cannot be download due to COPPA
                    ErrorCOPPAPlay,     //game cannot be played due to COPPA
                    ErrorNothingUpdated,     // Updated completed, but no files where touched. Not an "error" by purist of definitions.
                    ErrorParentNeedsUpdate, // Parent needs to be updated before entitlement can be downloaded
                    ErrorForbidden,
                };

                /// \brief TBD.
                Q_DECLARE_FLAGS(States, State)

                /// \brief TBD.
                enum Item
                {
                    Base = 0x1,        ///< TBD.
                    Boxart = 0x2,      ///< TBD.
                    Banner = 0x4,      ///< TBD.
                    All = 0xFFFFFFFF   ///< TBD.
                };

                /// \brief TBD.
                enum InstallState
                {
                    ContentInstalled,      ///< The content is installed.
                    ContentNotInstalled,   ///< The content isn't installed.
                    NoInstallCheck         ///< The install state can't be determined due to the content configuration.
                };

                /// \brief TBD.
                Q_DECLARE_FLAGS(Items, Item)

                enum TrialTimeLeftError
                {
                    TrialError_DisabledByAdmin = -1,
                    TrialError_LimitedTrialNotStarted = -2,
                    TrialError_NotTrial = -3
                };

                /// \brief TBD.
                const EntitlementRef entitlement() const { return m_Entitlement; }
                EntitlementRef entitlement() { return m_Entitlement; }

                /// \brief TBD.
                ACCESSOR(CloudContent, cloudContent);

                /// \brief The LocalContent destructor; releases the resources of a LocalContent object.
                ~LocalContent();
                
                /// \brief Delete the install flow associated with the content (running on a separate thread ->
                /// asynchronuous behavior -> emits installFlowDeleted() once done)
                void deleteInstallFlow();

                /// \brief Returns the current state of the content.
                ///
                /// \return States TBD.
                States state() const;

                /// \brief Returns the associated play flow.
                ///
                /// \return PlayFlow TBD.
                QSharedPointer<PlayFlow> playFlow();

                /// \brief Our associated install flow.
                Downloader::IContentInstallFlow * installFlow();

                /// Reasons why the content might not be downloadable.
                enum DownloadableStatus
                {
                    DS_Downloadable,
                    DS_WrongPlatform,
                    DS_ReleaseControl,
                    DS_SubscriptionControl, 
                    DS_NotSubscriber,
                    DS_NotOnline,
                    DS_PULC,
                    DS_NotOwned,
                    DS_BaseGameNotInstalled,
                    DS_Busy,
                    DS_WrongArchitecture, // 32/64 bit compatibility
                    DS_NotDownloadable // catch-all for unlikely cases
                };

                /// Return the status of the content with respect to downloads
                DownloadableStatus downloadableStatus() const;

                /// \brief Returns true if the install flow can start.
                bool downloadable() const { return downloadableStatus() == DS_Downloadable; }

                enum DownloadFlags
                {
                    kDownloadFlag_isITO = 0x0001,
                    kDownloadFlag_eulas_accepted = 0x0002,
                    kDownloadFlag_default_install = 0x0004,
                    kDownloadFlag_try_to_move_to_top = 0x0008,
                };

                /// \brief Starts a download.
                ///
                /// \param installLocale TBD.
                /// \param isITO TBD.
                /// \param srcFile TBD.
                bool download(QString installLocale, int download_flags = 0, QString srcFile = "");

                /// \brief Resume a download
                ///
                /// \param silent If true, the resume will silently fail if an error is encountered.
                bool resumeDownload(bool silent = false);

                /// \brief Starts the touchup installer/installer.exe.
                void install();

                /// \brief Returns whether something can be installed.
                bool installable() const;

                /// \brief Returns whether something is installed.
                /// This is a wrapper around installState
                bool installed(bool forceUpdate = false) const;

                /// \brief Returns whether something is on the queue to be downloaded and installed
                bool installInProgressQueued() const;

                /// \brief Return whether any parent of this PDLC is installed.  Returns false for non PDLC.
                /// forceUpdate causes LocalContent to recheck installCheck/disk rather than use cached value.
                bool isAnyParentInstalled(bool forceUpdate = false, bool parentInQueueOK = true) const;

                /// \brief Return whether any parent of this PDLC is playable.  Returns false for non PDLC.
                bool isAnyParentPlayable() const;

                /// \brief Returns the current install state.
                ///
                /// \param forceUpdate TBD.
                InstallState installState(bool forceUpdate = false) const;

                /// \brief Returns true if a non-monitored install is in progress.
                bool isNonMonitoredInstalling();

                /// \brief Returns true is the content is uninstallable.
                bool unInstallable() const;

                /// \brief Returns whether the game has its own uninstaller defined.
                bool hasUninstaller() const;

                /// \brief Uninstall.
                /// Run game-specific uninstaller if found (No impl on Mac yet). Otherwise opens Add or Remove Programs for PC/Do a clean wipe for Mac.
                bool unInstall(bool silent = false);

                class PlayResult
                {
                public:
                    enum Result
                    {
                        PLAY_SUCCESS,
                        PLAY_FAILED_MIN_OS,
                        PLAY_FAILED_REQUIRED_UPDATE,
                        PLAY_FAILED_AUTH_CODE_RETRIEVAL,
                        PLAY_FAILED_PLUGIN_INCOMPATIBLE,
                        PLAY_FAILED_GENERAL
                    };

                    PlayResult() : result(PLAY_SUCCESS) {}
                    PlayResult(Result playResult) : result(playResult) {}
                    PlayResult(Result playResult, QString const& resultInfo)
                        : result(playResult)
                        , info(resultInfo)
                    {
                    }

                    Result result;
                    QString info;  // additional info, depending on Result
                };

                /// \brief Does all the things required to launch the game
                /// should connect to listen for launched() signal if interested in result of launch()
                /// Returns PLAY_SUCCESS if the game was launched successfully.
                Q_INVOKABLE void play();

                /// \brief Content not wrapped in RtP will call this method through
                /// LSX when externally started to force the UI to show it in
                /// a playing state.
                Q_INVOKABLE void playNonRtp();

                /// \brief Externally started content that is not wrapped in RtP will call this method
                /// through LSX on shutdown to force the UI to show it in a ready to play state.
                Q_INVOKABLE void stopNonRtpPlay();

                /// \brief to be called when launching an alternate flow (e.g. battlelog)
                /// need to emit changed state so that the game tile state will be properly reflected
                Q_INVOKABLE void alternateGameLaunched();

                /// \brief Create process watcher/reset content state after we reattached to a running game (Origin cient was closed in between)
                Origin::Services::IProcess* playRunningInstance(uint32_t pid, const QString& executablePath);

                /// Reasons why an entitlement might not be playable
                enum PlayableStatus 
                {
                    PS_Playable,
                    PS_WrongPlatform, 
                    PS_ReleaseControl, 
                    PS_NotInstalled, 
                    PS_TrialNotOnline, 
                    PS_SubscriptionControl, 
                    PS_Playing,
                    PS_Busy,
                    PS_WrongArchitecture, // 32/64-bit issues
                    PS_BaseGameNotPlayable,
                    PS_NotOwned,
                    PS_NotPlayable // catch-all for unlikely cases
                };

                /// Return the status of the content with respect to playing
                ///
                /// \param checkORC checkORC is set to false to ignore ORC.
                PlayableStatus playableStatus(bool checkORC = true) const;

                /// Return whether the content is allowed to be played, without regard for whether it is installed, etc.
                /// This is primarily for making sure we don't show Progressive Install UI for games that are unreleased or preload
                bool isAllowedToPlay() const;

                /// \brief Return true if the game is playable.
                ///
                /// \param checkORC checkORC is set to false to ignore ORC.
                bool playable(bool checkORC = true) const { return playableStatus(checkORC) == PS_Playable; }

                /// \brief Checks whether a game is currently playing.
                bool playing() const;

                /// \brief Checks whether a game is in the Trash or not.
                bool inTrash() const;

                /// \brief Moves the application to the correct folder, so it can be played.
                bool restoreFromTrash();

                /// \brief Start an installation repair.
                bool repair(bool silent = false);

                /// \brief Returns true if an installation is repairable?
                bool repairable() const;

                /// \brief Returns true if this entitlement is currently repairing.
                bool repairing() const;

                /// \brief Returns true if this content needs repair.  (Missing map.crc file)
                bool needsRepair() const;

                /// \brief Starts an update of ???
                ///
                /// \param silent Set silent to true to suppress dialogs during the update; defaulted to false.
                bool update(bool silent = false);

                /// \brief Returns true if this entitlement is updating.
                bool updating() const;

                /// \brief Returns true if DiP updates are supported.
                bool updatable() const;

                /// \brief Returns true if there is an update available.
                bool updateAvailable();

                bool unInstalling() const { return mUninstalling; }

                /// \brief Returns the last time the game was played.
                QDateTime lastPlayDate() const;

                /// \brief Returns the total number of seconds this game has been played.
                qint64 totalPlaySeconds() const;

                /// \brief Returns the installation directory.
                QString dipInstallPath() const;

                /// \brief Returns the installer folder path for non-dip titles.
                ///
                /// \param createIfNotExists TBD.
                QString nonDipInstallerPath(bool createIfNotExists = false);

                /// \brief Returns the executable path.
                QString executablePath() const;

                /// \brief Returns the local content cache folder path
                ///
                /// For example, C:\\progamdata\\origin\\localcontent\\battlefield 3
                /// This folder stores items like the mnft and the version installed.
                ///
                /// \param createIfNotExists TBD.
                /// \param migrateLegacy TBD.
                /// \param elevation_result TBD.
                QString cacheFolderPath(bool createIfNotExists = false, bool migrateLegacy = false, int *elevation_result = NULL, int *elevation_error = NULL);

                /// \brief Returns the installed version number.
                QString installedVersion() { return mInstalledVersion ;}

                /// \brief Sets the installed locale.
                ///
                /// \param locale TBD.
                void setInstalledLocale(QString locale) { mInstalledLocale = locale; }

                /// \brief Returns the installed locale.
                QString installedLocale() { return mInstalledLocale; }

                /// \brief Sets the installed locale.
                ///
                /// \param locale TBD.
                void setPreviousInstalledLocale(const QString& locale) { mPreviousInstalledLocale = locale; }

                /// \brief Returns the installed locale.
                QString previousInstalledLocale() const { return mPreviousInstalledLocale; }

                /// \brief Returns the available locales - with grey market filter.
                QStringList availableLocales(const QStringList& manifestLocales, const QStringList& manifestContentIds, const QString& manifestVersion);

                /// \brief Deletes the installer folder associated with this entitlement (non-dip).
                void deleteTempInstallerFiles();

                /// \brief Returns the release control state.
                ///
                /// \return RCState The release control state: Unrelease, Preload, Released, or Expired.
                LocalContent::RCState releaseControlState() const;

                /// \brief Begins/Resumes download of the content if it has been marked for autostart
                /// \param startDownload indicates whether to actually trigger the download if applicable
                /// \param uacFailed indicates whether the user allowed the download/accepted the UAC dialog
                /// \return true if the content is actually ready for download - otherwise false.
                bool autoStartMainContent(bool startDownload, bool uacFailed);

                /// \brief Returns the set of PDLC available for download.
                /// \param Because PDLC is shared across content, we pass a set that we used to suppress multiple starts for same PDLC
                void collectAllPendingPDLC(QSet<QString>& pendingPDLC);

                bool isPDLCReadyForDownload(EntitlementRef entitlement);

                /// \brief starts the download for all available PDLC
                /// \param Because PDLC is shared across content, we pass a set that we used to suppress multiple starts for same PDLC
                /// \param A bool flag (in-out parameter) indicating whether permissions have already been requested and should not be re-requested
                /// \param A bool flag (in-out parameter) indicating whether UAC has failed already and should not be re-attempted
                /// \return bool False if permission to start was not granted by the user.  True otherwise.
                void downloadAllPDLC(QSet<QString>& autoStartedPDLC, bool& permissionsRequested, bool &uacFailed);

                void downloadPDLC(Origin::Engine::Content::EntitlementRef child, QSet<QString>& autoStartedPDLC, bool& permissionsRequested, bool &uacFailed);

                /// \brief starts the repair for all installed PDLC
                void repairAllPDLC();

                /// \brief Sets the install check for addons.
                ///
                /// \param installCheck TBD.
                void setAddonInstallCheck(const QString& installCheck);

                /// \brief sets up timers and orc state based on the last server time retrieved.
                void refreshReleaseControlStatesAndTimers();

                /// \brief sets up timers for game expiration based on the last server time retrieved.
                void refreshReleaseControlExpiredTimer( const QDateTime& serverTime, const QDateTime& terminationDate );

                /// \brief online required to start a free trial.
                bool isOnlineRequiredForFreeTrial();

                /// \brief TBD.
                void saveNonUserSpecificData();

                /// \brief If content is playing, detaches from current process so it can start a new one
                /// Because it skips certain end of playing cleanup (cloud saves, promo browser, social status), 
                /// this function should not be used unless the code is going to relaunch the exact same 
                /// entitlement immediately afterwards.
                void prepareForRestart();

                /// \brief Computes and returns current time remaining (in milliseconds) for a trial
                qint64 trialTimeRemaining(bool bActualTime = false, const QDateTime& overrideTerminationDate = QDateTime()) const;

                bool isTimeRemainingUpdated() { return m_lastTrialUpdate.isValid(); }

                /// \brief Get the Free Trial elapsed timer
                QElapsedTimer& freeTrialElapsedTimer() { return mFreeTrialElapsedTimer; }

                /// \brief Early Trial set time remaining.
                /// \param initial The total trial time initially.
                /// \param remaining The amount of time that is still remaining on the trial.
                /// \param slice The amount of time to burn in this trial time slice. 
                /// \param timeStamp The UTC time when this information was obtained, so actual time remaining can be calculated.
                void setTrialTimeRemaining(const int& initial, const int& remaining, const int& slice, const QDateTime& timeStamp);

                /// \ brief Indicates whether the Early Trial has started.
                bool hasEarlyTrialStarted() const;

                // The amount of time in milliseconds before disconnection of the trial if -1 the connection is established. If 0 the connection is disconnected.
                int trialReconnectingTimeLeft() const;

                /// \brief Set the amount of time before disconnection.
                /// \param timeLeft in milli seconds. if -1 the connection is re-established. If 0 the connection is disconnected.
                void setTrailReconnectingTimeLeft(int timeLeft);

                /// \brief fired when /ecommerce/products call is finished
                //void addUnownedContent(Origin::Services::EntitlementDataVector& extraContent);

                /// \brief True if this item is configured to treat updates as mandatory
                bool treatUpdatesAsMandatory() const;

                /// \brief True if we can launch this content even if it's already playing (e.g. Sparta).
                bool allowMultipleInstances() const;

                /// \brief True if the version installed is at least the minimum required for online use
                bool hasMinimumVersionRequiredForOnlineUse() const;

                /// \brief Accessor for mLocalInstallProperties
                /// Can be NULL if the content is not DiP
                Downloader::LocalInstallProperties* localInstallProperties() const { return mLocalInstallProperties; }

                /// \brief Returns the list of all launch titles.
                QStringList  launchTitleList();

                /// \brief Returns the default launcher based on 32/64 bit OS match of the current OS. If none match then it returns the first launcher.
                QString      defaultLauncher();

                /// \brief given a game launcher title, return the associated launcher
                const Engine::Content::LauncherDescription* findLauncherByTitle (QString& launchTitle);

                /// \brief returns whether the currently associated launcher will work on current OS
                bool        validateLauncher (QString& launchTitle);

                /// \brief Determine whether to hide the game (i.e., for R&D mode)
                bool shouldHide();

                /// \brief emit the coppa error
                void notifyCOPPAError(bool isDownload);

                /// \brief should we stop the user from playing or downloading
                bool shouldRestrictUnderageUser();

                /// \brief TBD.
                QString installFolderName() const;

                /// \brief append product id string to list of DLC that need to be downloaded because they have just been purchased but the auto-update setting is off (EBIBUGS-28701)
                void appendPendingPurchasedDLCDownloads(const QString productId);

                /// \brief returns time stamp of when the game was installed (in number of seconds that have passed since 1970-01-01T00:00:00)
                quint32 initialInstalledTimestamp() {return mInitialInstalledTimestamp;}

                /// \brief timer in millisecs that tells us how long the user has been playing a game.
                QDateTime playSessionStartTime() const { return mPlaySessionStartTime; }

            public slots:
                void listenForParentStateChange();
                void stopListeningForParentStateChange();
                void updateTrialTime();

            signals:

                /// \brief Emitted before we're removed.
                void aboutToRemove();

                /// \brief Emitted when the flow state has changed.
                void stateChanged(Origin::Engine::Content::EntitlementRef);

                ///
                /// \brief Emitted when progress has changed
                ///
                void progressChanged (Origin::Engine::Content::EntitlementRef);

                /// \brief Emitted when the flow state is active/inactive.
                ///
                /// Checked as sent on every state change.
                void flowIsActive(const QString &, const bool &);

                /// \brief Emitted when there are local content errors.
                void error(Origin::Engine::Content::EntitlementRef, int);

                /// \brief Emitted when the download portion of the install is finished.
                void downloadingFinished();

                /// \brief Emitted when the install flow is canceled.
                void installFlowCanceled();

                /// \brief Emitted when game launch completes with the appropriate result code.
                void launched (Origin::Engine::Content::LocalContent::PlayResult);

                /// \brief Emitted when the game starts playing.
                ///
                /// \param externalNonRtp TBD.
                void playStarted(Origin::Engine::Content::EntitlementRef, bool externalNonRtp);

                /// \brief Emitted when the game finishes playing.
                ///
                /// \param entitlement TBD.
                void playFinished(Origin::Engine::Content::EntitlementRef entitlement);

                /// \brief Emitted when the game is about to expire.
                ///
                /// \param timeRemainingInMinutes time left before game expires.
                void expireNotify(int timeRemainingInMinutes);

                /// \brief Emitted when the early access game is about to expire.
                ///
                /// \param timeRemainingInMinutes time left before game expires.
                void timedTrialExpireNotify(const int& timeRemainingInMinutes);

                /// \brief Emitted when the unowned content associated with this entitlement changed
                ///
                ///
                void unownedContentChanged();

                /// \brief Emitted once the install flow was deleted
                void installFlowDeleted();

            private:
                //path related functions

                /// \brief TBD.
                void deleteCacheFolderIfUninstalled();

                /// \brief TBD.
                ///
                /// \return ContentConfiguration TBD.
                const  ContentConfigurationRef contentConfiguration() const;

                /// \brief TBD.
                ///
                /// \return ContentConfiguration TBD.
                ContentConfigurationRef contentConfiguration();

                /// \brief The LocalContent constructor.
                ///
                /// \param user TBD.
                /// \param entitlement TBD.
                explicit LocalContent(UserRef user, EntitlementRef entitlement);

                /// \brief TBD.
                void init();

                /// \brief TBD.
                UserRef user() const;

                /// \brief TBD.
                ///
                /// \param path TBD.
                /// \param createIfNotExists TBD.
                bool buildSerializationPath(QString& path, bool createIfNotExists = false);

                /// \brief TBD.
                bool loadNonUserSpecificData();

                /// \brief Called when another base game of the same master title has been installed or updated
                void reloadInstallProperties();

                /// \brief TBD.
                void migrateOldFileNames();

                /// \brief TBD.
                ///
                /// \param cacheFolderPath TBD.
                void migrateLegacyInformation(const QString& cacheFolderPath);

                ///
                /// \brief Called when the progress of the download has changed
                ///
                void handleProgressChanged();

                /// \brief TBD.
                void trackTimePlayedStart();

                /// \brief TBD.
                void trackTimePlayedStop();

                /// \brief TBD.
                ///
                /// \param locale TBD.
                /// \param path TBD.
                /// \param flags TBD.
                bool beginInstallFlow(QString &locale, QString &path, const Downloader::IContentInstallFlow::ContentInstallFlags &flags);

                /// \brief TBD.
                void updateReleaseControlState();

            public:
                /// \brief TBD.
                QString getLaunchAdditionalEnvironmentVars ();

            private:

                /// \brief TBD.
                bool inDownloadState() const;

                /// \brief TBD.
                ///
                /// \return Entitlement TBD.
                EntitlementRef entitlementRef() const { return m_Entitlement; }

                /// \brief TBD.
                ///
                /// \param executable TBD.
                void runEscalated(const QString& executable) const;

                /// \brief TBD.
                ///
                /// \param folder TBD.
                bool createElevatedFolder(const QString & folder, int *elevation_result = NULL, int *elevation_error = NULL);

                /// \brief Start a timer which checks whether a free trial was starte
                void initFreeTrialStartedCheck();

                /// \brief returns true if the path is an existing folder.  However, if the path is
                /// a Mac .app bundle folder, then this function will return false.
                bool isPathANormalFolder(QString path) const;

                /// \brief Start a timer to track amount of time elapsed for Free Trial
                void initFreeTrialElapsedTimer();

                /// \brief Termination date for Free Trial has passed
                bool isTerminationDatePassed() const;

                /// \brief Add a child entitlement (ie. PDLC) to the list of
                /// entitlements which should not auto-download
                /// \return True if added, false if was already on the list
                bool setChildManualOnlyDownload(const QString& productId);

                /// \brief Check whether a child entitlement should auto-download
                /// \return True if auto-download is disabled, false if
                /// auto-download is allowed.
                bool isChildManualOnlyDownload(const QString& productId) const;

                /// \brief Re-enable auto-download for all child entitlements
                void clearChildManualOnlyDownloads(bool saveToDisk = true);

                /// \brief Find all parents of this PDLC and call setChildManualOnlyDownload on them
                void setChildManualOnlyDownloadOnAllParents();

                /// \brief Calculate the RTP launch code based on the algorithm described here:
                /// http://confluence.ea.com/display/EATech/Origin-Game+RtP+Handshake+mechansim
                quint32 getRTPHandshakeCode();

                /// \brief load (or refresh) the content configuration based on the local Manifest.  
                /// This is only pertinent to DiP content.
                void loadLocalInstallProperties();

                /// \brief Send telemetry about game session start
                void sendGameSessionStartTelemetry(const QString& launchType, const QString& childrenContent, const bool& isNonOriginGame);

            private slots:
                /// \brief TBD.
                ///
                /// \param exitCode TBD.
                void onProcessFinished(uint32_t pid, int, enum Origin::Services::IProcess::ExitStatus);

                /// \brief TBD.
                ///
                /// \param state TBD.
                void onProcessStateChanged(enum Origin::Services::IProcess::ProcessState state);

                /// \brief TBD.
                ///
                /// \param state TBD.
                /// \param progressUpdate TBD.
                void onProgressUpdate( Origin::Downloader::ContentInstallFlowState state, const Origin::Downloader::InstallProgressState& progressUpdate );

                /// \brief TBD.
                ///
                /// \param newState TBD.
                void onStateChanged(Origin::Downloader::ContentInstallFlowState newState);

                /// \brief Called when the base entitlement's install flow state changes.
                void onParentStateChanged(Origin::Downloader::ContentInstallFlowState newState);

                /// \brief TBD.
                void emitPendingProgressChangedSignal();

                /// \brief TBD.
                void onReleaseControlChangeTimeOut();

                /// \brief fired when the expired timer times out.
                void onReleaseControlExpiredTimeOut();

                /// \brief fired when it is time to check whether a free trial has started
                void onFreeTrialStartedCheckTimeOut();

                /// \brief fired when the limited trial termination date query succeeds
                void onEntitlementQueryFinished();

                /// \brief fired when authorization code has been retrieved
                void onAuthorizationCodeRetrieved ();

                /// \brief fired by timer, to detect uninstalled games
                void checkForUninstall();

                /// \brief fired by the QRunnable when the uninstaller has finished
                void uninstallFinished();

                /// \brief fired by the ContentOperationQueueController when the head of the queue has changed.
                void onHeadChanged(Origin::Engine::Content::EntitlementRef newHead, Origin::Engine::Content::EntitlementRef oldHead);

            private:

                /// \brief Invalidates our caches related to the local content.
                void invalidateCaches();

                /// \brief Destroys our running process object.
                ///
                /// Thread safe.
                void cleanupRunningProcessObject();

                /// \brief handles the actual launch of the process
                void launch ();

                /// \brief handles the post launch cleanup and emitting of launched(bool) signal
                void finishLaunch(PlayResult result);

                //mutexes

                /// \brief TBD.
                mutable QMutex mProcessLaunchLock;

                /// \brief TBD.
                QMutex mDataSerializationLock;

                /// \brief TBD.
                mutable QMutex mInstallStateLock;


                /// \brief TBD.
                Downloader::IContentInstallFlow * mInstallFlow;

                /// \brief TBD.
                QSharedPointer<PlayFlow>                        mPlayFlow;

                /// \brief TBD.
                Origin::Services::IProcess *mRunning;

                /// \brief TBD.
                enum  Origin::Services::IProcess::ProcessState mRunningState;

                /// \brief TBD.
                QAtomicInt mStateChangeSignalPending;

                //persisted non-user specific values

                /// \brief TBD.
                QString mInstalledLocale;

                /// \brief TBD.
                QString mPreviousInstalledLocale;

                /// \brief TBD.
                QString mInstalledVersion;

                /// \brief TBD.
                QString mInstalledFolder;

                /// \brief String which stores the alternate uninstall path used for legacy DLC uninstall (where DiP manifest does not specify)
                QString mAlternateUninstallPath;

                /// \brief Flag specifying whether a free trial was started
                bool mFreeTrialStarted;

                /// \brief Child entitlements on this list do not auto-download
                QStringList mChildManualOnlyDownloads;

                /// \brief Flag specifying whether any unowned extra content is available
                bool mHasUnownedContent;

                /// \brief TBD.
                QString mAddonInstallCheck;

                /// \brief Version currently downloading.
                QString mDownloadingVersion;

                /// \brief TBD.
                RCState mReleaseControlState;

                /// \brief timer in millisecs left until the release control state changes.
                QTimer *mReleaseControlStateChangeTimer;

                /// \brief timer in millisecs left until the game is expired (also fires off a set intervals before the game is about to expire.
                QTimer *mReleaseControlExpiredTimer;

                /// \brief timer in millisecs left until the free trial start check is made.
                QTimer mFreeTrialStartedCheckTimer;

                /// \brief timer in millisecs that tells us how long the user has been playing a game.
                QDateTime mPlaySessionStartTime;

                /// \brief Milliseconds passed since free trial started
                QElapsedTimer mFreeTrialElapsedTimer;

                struct ExpiredIntervalData
                {
                    int timeRemaining;
                    int timeTilNextTimeOut;
                };

                // Early Trial Information
                qint64 m_initialTrialDuration;
                qint64 m_remainingTrialDuration;
                qint64 m_currentTrialTimeSlice;
                QDateTime m_lastTrialUpdate;

                int m_reconnectingTrialTimeLeft;
                QDateTime m_reconnectingTrialTimeStart;

                /// \brief intervals at which to send notifications for expiring games.
                QStack<ExpiredIntervalData> mExpiredTimerIntervals;

                /// \brief TBD.
                bool mNonMonitoredInstallInProgress;

                // Override flags to set externally started content not wrapped in RtP
                // to playing and ready to play states

                /// \brief TBD.
                bool mPlayingOverride;

                /// \brief TBD.
                bool mPlayableOverride;

                // Cache our installed state
                QTimer mUninstallCheckTimer;

                /// \brief TBD.
                mutable bool mInstalled;

                /// \brief TBD.
                mutable QElapsedTimer mLastInstallCheck;

                /// \brief TBD.
                mutable bool mExecutableExists;

                /// \brief TBD.
                mutable QElapsedTimer mLastExecutableCheck;

                /// \brief returns true if the executable path is an existing folder path. We use this to cache the folder check in install state
                mutable bool mExecutablePathIsExistingFolderPath;

                /// \brief returns true if the content needs repair.  This usually occurs if the map.crc file is missing
                mutable bool mNeedsRepair;

                /// \brief Milliseconds passed since downloadstarted
                QElapsedTimer mDownloadElapsedTimer;

            private:

                /// \brief TBD.
                Engine::UserWRef mUser;

                EntitlementRef m_Entitlement;

                /// \brief TBD
                bool mUACExpired;

                /// \brief Locally defined content properties from the DiP Manifest.  Only pertinent for DiP.
                Origin::Downloader::LocalInstallProperties* mLocalInstallProperties;

                QString mAuthorizationCode;     //ID2 authorization code retrieved with game's client_id
                QTimer* mAuthCodeRetrievalTimer;
                QSet<QString> mPendingPurchasedDLCDownloadList;  // list of just purchased DLC we want to download on refresh when auto-updates are off (EBIBUGS-28701)
                bool mUninstalling;

                /// \brief time stamp of when the game was installed (in number of seconds that have passed since 1970-01-01T00:00:00) (will be zero for games that were already downloaded before this change was added)
                quint32 mInitialInstalledTimestamp;
            };

            class UninstallTask : public QObject, public QRunnable
            {
                Q_OBJECT

            public:
                UninstallTask(const QString& productId, const QString& command, const QString& arguments);
                void setManualDLCUninstallArgs(const QString& cacheFolderPath, const QString& gameInstallFolder, const QString& dlcInstallCheck);

                virtual void run();

signals:
                void finished();

            private:
                QString mProductId;
                QString mCommand;
                QString mArguments;
                QString mCacheFolderPath;
                QString mGameInstallFolder;
                QString mDlcInstallCheck;
            };
        } //namespace Content
    } //namespace Engine
}//namespace Origin

#endif // LOCALCONTENT_H
