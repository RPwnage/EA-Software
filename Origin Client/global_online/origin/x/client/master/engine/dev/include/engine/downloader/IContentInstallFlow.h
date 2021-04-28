#ifndef ICONTENTINSTALLFLOW_H
#define ICONTENTINSTALLFLOW_H

#include "ContentInstallFlowContainers.h"

#include "services/downloader/Parameterizer.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "services/downloader/DownloaderErrors.h"
#include "services/plugin/PluginAPI.h"
#include "engine/login/User.h"
#include "engine/content/ContentTypes.h"

#include <QObject>
#include <QMutex>
#include <QSignalTransition>
#include <QState>
#include <QStateMachine>

namespace Origin
{
/// \brief The base %Downloader namespace, which contains the namespaces shown in the following diagram.
///
/// <span style="font-size:14pt;color:#404040;"><b>See the relevant wiki docs:</b></span> <a style="font-weight:bold;font-size:14pt;color:#eeaa00;" href="https://developer.origin.com/documentation/display/EBI/Origin+Downloader">Origin Downloader</a>
/// \dot
/// digraph g {
/// graph [rankdir="LR"];
/// node [shape="box" fontname="Helvetica" fontsize="9" fontcolor="black" fillcolor="cadetblue1" style="filled"];
/// edge [];
///
/// "node0" -> "node1";
/// "node1" -> "node2";
/// "node1" -> "node3";
/// "node1" -> "node4";
/// "node1" -> "node5";
/// "node1" -> "node6";
/// "node1" -> "node7";
/// "node1" -> "node8";
/// "node1" -> "node9";
/// "node1" -> "node10";
/// "node1" -> "node11";
/// "node1" -> "node12";
/// "node1" -> "node13";
/// "node1" -> "node14";
/// "node14" [label="ZLibDecompressor" URL="\ref ZLibDecompressor"]
/// "node13" [label="Util" URL="\ref Util"]
/// "node12" [label="UnpackType" URL="\ref UnpackType"]
/// "node11" [label="UnpackError" URL="\ref UnpackError"]
/// "node10" [label="StringHelpers" URL="\ref StringHelpers"]
/// "node9" [label="ProtocolError" URL="\ref ProtocolError"]
/// "node8" [label="PatchBuilderError" URL="\ref PatchBuilderError"]
/// "node7" [label="LogHelpers" URL="\ref LogHelpers"]
/// "node6" [label="InstallError" URL="\ref InstallError"]
/// "node5" [label="FlowError" URL="\ref FlowError"]
/// "node4" [label="DownloadError" URL="\ref DownloadError"]
/// "node3" [label="ContentPackageType" URL="\ref ContentPackageType"]
/// "node2" [label="ContentDownloadError" URL="\ref ContentDownloadError"]
/// "node1" [label="Downloader" URL="\ref Downloader"]
/// "node0" [label="Origin" URL="\ref Origin"]
/// }
/// \enddot
namespace Downloader
{
        /// \extends QStateMachine
        /// \brief Base class for content install flows.
        ///
        /// Acts as a director for the processing that must occur when downloading and installing content.
        /// Defines a set of public states for derived classes to use. Public state changes are visible to
        /// the client through stateChanged signals. Derived classes may define additional 'private' sub-states
        /// which are invisible to the client when entered.
        class ORIGIN_PLUGIN_API IContentInstallFlow : public QStateMachine
        {
            Q_OBJECT
                signals:
                        
                        /// \brief Emitted when the content install flow state machine changes states.
                        ///
                        /// \param newState The state which the content install flow has just entered.
                        void stateChanged (Origin::Downloader::ContentInstallFlowState newState);
                        
                        /// \brief Emitted when progress toward job completion has occurred within a content install flow state.
                        ///
                        /// Can be used to communicate progress during content download, file crc check during repair,
                        /// install progress, etc.
                        ///
                        /// \param state The content install flow state in which the progress occurred.
                        /// \param stateProgress Details about the progress.
                        void progressUpdate (Origin::Downloader::ContentInstallFlowState state, const Origin::Downloader::InstallProgressState& stateProgress);
                        
                        /// \brief Emitted when an unrecoverable error has occurred.
                        ///
                        /// \param errorType The type of error. See FlowError, ProtocolError, DownloadError, UnpackError, InstallError
                        /// \param errorCode More specific info about the error, i.e. error type DownloadError::HttpError might be associated with code 403.
                        /// \param errorContext String representation of the context of the error. For example, a filename for an IO error, etc.
                        /// \param key The id key of the content that was being downloaded when the error occurred.
                        void IContentInstallFlow_error (qint32 errorType, qint32 errorCode , QString errorContext, QString key);

                        /// \brief Emitted when an error occured. Download is going to retry.
                        ///
                        /// \param errorType The type of error. See FlowError, ProtocolError, DownloadError, UnpackError, InstallError
                        /// \param errorCode More specific info about the error, i.e. error type DownloadError::HttpError might be associated with code 403.
                        /// \param errorContext String representation of the context of the error. For example, a filename for an IO error, etc.
                        /// \param key The id key of the content that was being downloaded when the error occurred.
                        void IContentInstallFlow_errorRetry(qint32 errorType, qint32 errorCode, QString errorContext, QString key);
                        
                        /// \brief Emitted when an error has occurred which can be ignored by the content install flow such as a request to pause during an invalid state.
                        ///
                        /// \param warnCode See FlowError.
                        /// \param warnMessage String description of the warning.
                        void warn (qint32 warnCode, const QString& warnMessage);
                        
                        /// \brief Emitted when the content install flow expects the UI to show the install info dialog.
                        ///
                        /// \param request Contains display info for the dialog. 
                        void pendingInstallArguments (const Origin::Downloader::InstallArgumentRequest& request);
                        
                        /// \brief Emitted when the content install flow expects the UI to show a language selection dialog.
                        ///
                        /// \param request Contains a list of available languages.
                        void pendingInstallLanguage (const Origin::Downloader::InstallLanguageRequest& request);
                        
                        /// \brief Emitted when the content install flow expects the UI to show eula dialog(s).
                        ///
                        /// \param request Contains info required by the eula dialogl.
                        void pendingEulaState (const Origin::Downloader::EulaStateRequest& request);
                        
                        /// \brief Emitted when the content install flow expects the UI to show a change disc dialog.
                        ///
                        /// \param request Contains info required by the change disc dialog.
                        void pendingDiscChange (const Origin::Downloader::InstallArgumentRequest& request);
                        
                        /// \brief Emitted when the content install flow expects the UI to show an "Are you sure you want to cancel?" dialog.
                        ///
                        /// \param request Contains display info for the dialog.
                        void pendingCancelDialog(const Origin::Downloader::CancelDialogRequest& request);
                        
                        /// \brief Emitted when the content install flow expects the UI to show a 3PDD install dialog.
                        ///
                        /// \param request Contains display info for the 3PDD install dialog.
                        void pending3PDDDialog(const Origin::Downloader::ThirdPartyDDRequest& request);

                        /// \brief Emitted when the content install flow cannot continue while the content is still running.  (For updates)
                        void pendingContentExit();
                        
                        /// \brief Emitted when the flow has suspended.
                        ///
                        /// \param key The content id key associated with the suspended install flow.
                        void suspendComplete(const QString& key);

						/// \brief Emitted when an install did not complete successfully
						void installDidNotComplete();

                        /// \brief emitted when the flow state machine has finished (used to be finished() pre-Qt5)
                        void finishedState();

                        /// \brief Emitted when IContentInstallFlow::cancel is executed.
                        ///
                        /// Initiates a state transition to ContentInstallFlowState::kCanceling.
                        /// The transition can occur from all states except ContentInstallFlowState::kPausing, ContentInstallFlowState::kResuming,
                        /// ContentInstallFlowState::kReadyToStart, and ContentInstallFlowState::kError.
                        void canceling();

						/// \brief Emitted when IContentInstallFlow state is kPaused
						///
						void paused();

						/// \brief Emitted when required install is completed and the title is playable
						///
						void contentPlayable();

                        /// \brief INTERNAL signal only - Emitted from error() state to reset to kReadyToStart from an error in kReadyToStart
                        void resetForReadyToStart();

                        /// \brief Emitted once protocol has been deleted (from deleteProtocol)
                        void protocolDeleted();

                public:
                        
                        /// \brief Bit flags used when initializing installation parameters.
                        enum ContentInstallFlags
                        {
                            kNone = 0,                          ///< No flags are set.
                            kRepair = 0x000000001,              ///< The install flow should execute a repair of its associated content.
                            kSuppressDialogs = 0x000000002,      ///< Do not ask the UI to display user input dialogs.
                            kITO = 0x000000004,
                            kLocalSource = 0x00000008,
                            kEulasAccepted = 0x00000010,
                            kDefaultInstall = 0x00000020,
                            kTryToMoveToTop = 0x00000040,       ///< try to move this entitlement to the top of the queue
                        };
                        
                        /// \brief The IContentInstallFlow destructor; releases the resources of an instance of the %IContentInstallFlow class.
                        ~IContentInstallFlow();
                        
                        /// \brief Delayed delete to allow internal async mechanisms/references (like protocols running on separate threads) to stop in a "controlled way" / avoid race conditions
                        Q_INVOKABLE virtual void deleteAsync() = 0;

                        /// \brief Used to determine if the flow is using a local source for content download, i.e. disc install.
                        ///
                        /// \return bool True if the flow is not downloading via the network.
                        virtual bool isUsingLocalSource() const;

                        /// \brief Used to determine if the installation is being initiated externally (i.e. install from disk or from downloaded installer)
                        ///
                        /// \return bool True if the installation is launched externally
                        virtual bool isITO() const;

                        /// \brief Used to determine if the installation is currently acting as a Dynamic Download/Progressive Installation
                        ///
                        /// \return bool True if the installation is a DD/PI.  Always false for non-DiP flows.
                        virtual bool isDynamicDownload(bool *contentPlayableStatus = NULL) const;

                        /// \brief Used to determine if the content is using a touch-up installer that emits progress percentages.
                        ///
                        /// \return bool True if the content has a touch-up installer that emits progress.
                        virtual bool isInstallProgressConnected() const;
                        
                        /// \brief Retrieves the content object in which this content install flow is associated.
                        ///
                        /// \return Engine::Content::Entitlement A pointer to %Engine::Content::Entitlement, which contains ???.
                        virtual const Engine::Content::EntitlementRef getContent() const;
                        
                        /// \brief Retrieves the current state of this content install flow.
                        ///
                        /// \return ContentInstallFlowState The current state.
                        virtual ContentInstallFlowState getFlowState() const;
                        
                        /// \brief Retrieves the state of this content install flow prior to transitioning to the current state.
                        ///
                        /// \return ContentInstallFlowState The previous state.
                        virtual ContentInstallFlowState getPreviousState();
                        
                        /// \brief Determines if the auto resume flag has been set in the install manifest.
                        ///
                        /// \return bool True if "autoresume" is set to true in the install manifest.
                        virtual bool isAutoResumeSet() const;

                        /// \brief Assigns a value to the auto resume flag in the install manifest.
                        virtual void setAutoResumeFlag(bool autoResume);
                        
                        /// \brief Determines if the auto start flag has been set in the install manifest.
                        ///
                        /// \return bool True if "autostart" is set to true in the install manifest.
                        virtual bool isAutoStartSet() const;

                        /// \brief Assigns a value to the auto start flag in the install manifest.
                        virtual void setAutoStartFlag(bool autoStart);

                        /// \brief Determine if this flow will be asking the UI to display user input dialogs.
                        ///
                        /// \return bool True if the flow will not request the display of user input dialogs during install.
                        virtual bool suppressDialogs () const;

                        /// \brief Set whether this flow will be asking the UI to display user input dialogs or errors.
                        virtual void setSuppressDialogs(bool suppress);

						bool pausedForUnretryableError() const
						{
							return mPausedForUnretryableError;
						}

						void setPausedForUnretryableError(bool pause_for_error)
						{
							mPausedForUnretryableError = pause_for_error;
						}

                        /// \brief Returns the UUID that uniquely identifies this download job.
                        /// \return The UUID associated with this download job.
                        QString currentDownloadJobId() const;

                        /// \brief Retrieves the total number of bytes that comprise the content associated with this content install flow.
                        ///
                        /// \return qint64 The value assigned to "totalbytes" in the install manifest.
                        virtual qint64 getContentTotalByteCount();
                        
                        /// \brief Retrieves the number of bytes that have been downloaded.
                        ///
                        /// \return qint64 The value assigned to "savedbytes" in the install manifest.
                        virtual qint64 getDownloadedByteCount();

                        /// \brief Retrieves the destination location for this flow.
                        ///
                        /// \return QString The full path of the download location.  For DiPs this is usually the DiP folder, for non-DIPs this is usually the download cache, but is persisted in case the user changes it.
                        virtual QString getDownloadLocation() = 0;
                        
                        /// \brief Removes any user-sensitive information (such as authtoken) from a download URL.
                        ///
                        /// \return QString The download URL with all user-sensitive information removed.
                        QString getLogSafeUrl() const;
                        
                        /// \brief Retrieves the total size in bytes and type of the last installflow action
                        ///
                        /// \return LastActionInfo The LastActionInfo will contain the EntState enum and a qint64 holding the total bytes on data of the last action
                        LastActionInfo lastFlowAction() const;

                        /// \brief Returns the progress of the install; provides more detailed info.
                        ///
                        /// \return InstallProgressState The progress of the install.
                        InstallProgressState progressDetail();

                        /// \brief STOP - DON'T CALL THIS. This is for debug ONLY.
                        void fakeDownloadErrorRetry(qint32 errorType, qint32 errorCode, QString errorContext, QString key);

                        /// \brief The current number of consecutive times the install flow has retried the download after an http 403 error.
                        virtual quint16 getErrorRetryCount() const = 0;

                        /// \brief Initializes installation parameters.
                        ///
                        /// \param installLocale The locale to use for installation, i.e. en_US.
                        /// \param srcFile For ITO, the disc file to install. Paramter will be empty for network downloads.
                        /// \param flags Contains bit flag details about the install.
                        virtual void initInstallParameters (QString& installLocale, QString& srcFile, ContentInstallFlags flags) = 0;
                        
                        /// \brief The UI invokes this method in response to the pendingInstallArguments signal.
                        ///
                        /// \param response Contains user input from the install info dialog.
                        Q_INVOKABLE virtual void setInstallArguments (const Origin::Downloader::InstallArgumentResponse& response) = 0;
                        
                        /// \brief The UI invokes this method in response to the pendingInstallLanguage signal.
                        ///
                        /// \param response Contains user input from the language selection dialog
                        Q_INVOKABLE virtual void setEulaLanguage (const Origin::Downloader::EulaLanguageResponse& response) = 0;
                        
                        /// \brief The UI invokes this method in response to the pendingEulaState signal.
                        ///
                        /// \param response Contains user response to eula dialogs.
                        Q_INVOKABLE virtual void setEulaState (const Origin::Downloader::EulaStateResponse& response) = 0;
                        
                        /// \brief Starts the content install flow. 
                        Q_INVOKABLE virtual void begin() = 0;
                        
                        /// \brief Resumes a paused install flow.
                        Q_INVOKABLE virtual void resume() = 0;
                        
                        /// \brief Pauses a content install flow.
                        ///
                        /// \param autoresume True if the content install flow should automatically resume without user input.
                        Q_INVOKABLE virtual void pause(bool autoresume) = 0;
                        
                        /// \brief Cancels a content install flow.
                        Q_INVOKABLE virtual void cancel(bool forceCancelAndUninstall = false) = 0;
                        
                        /// \brief Used during ITO installs to resume after a disc change.
                        Q_INVOKABLE virtual void retry () = 0;
                        
                        /// \brief Causes the pendingCancelDialog signal to be emitted.
                        ///
                        /// \param isIGO True if the cancel dialog will be displayed within the IGO.
                        Q_INVOKABLE virtual void requestCancel(bool isIGO = false);
                        
                        /// \brief Invoked during 3PDD install when the user accepts or rejects the install dialog.
                        ///
                        /// \param continueInstall False if the user rejected the install dialog.
                        Q_INVOKABLE virtual void install(bool continueInstall);

						/// \brief Used to start a download off the download queue
						Q_INVOKABLE virtual void startTransferFromQueue (bool manual_start = false) = 0;

                        /// \brief Determines if the content install flow current job/operation type.
                        ///
                        /// \return ContentOperation the current operation type
                        virtual ContentOperationType operationType();
                        
                        /// \brief Processes errors that occur during content download/install.
                        ///
                        /// Gets called automagically when the flow emits an error signal.
                        ///
                        /// \param errorType The type of error. See ContentDownloadError.
                        /// \param errorCode Additional info about the error, i.e. 403 for type DownloadError::HttpError
                        /// \param errorMessage Description of the error.
                        Q_INVOKABLE virtual void handleError(qint32 errorType, qint32 errorCode , QString errorMessage) = 0;
                        
                        /// \brief Let install flow know that the entitlement's configuration was updated and give it a chance
                        /// to reconfigure itself.
                        Q_INVOKABLE virtual void contentConfigurationUpdated() = 0;

                        /// \brief Is the content install flow currently in a valid state to begin?
                        ///
                        /// \return bool True if it is currently permissible to invoke the begin method.
                        virtual bool canBegin() = 0;
                        
                        /// \brief Is the content install flow currently in a state where an orderly shutdown can occur?
                        ///
                        /// \return bool True if it is currently permissible to invoke the cancel method.
                        virtual bool canCancel() = 0;
                        
                        /// \brief Is the content install flow currently in a state where a pause can occur with minimal data loss?
                        ///
                        /// \return bool True if it is currently permissible to invoke the pause method.
                        virtual bool canPause() = 0;
                        
                        /// \brief Is the content install flow currently in a valid state to resume?
                        ///
                        /// \return bool True if it is currently permissible to invoke the resume method.
                        virtual bool canResume() = 0;
                        
                        /// \brief Is the content install flow currently in a valid state to retry?
                        ///
                        /// \return bool True if it is currently permissible to invoke the retry method.
                        virtual bool canRetry() = 0;
                        
                        /// \brief Determines if the content install flow is active.
                        ///
                        /// \return bool True if active.
                        virtual bool isActive() = 0;
                        
                        /// \brief Determines if the content install flow will perform a repair job.
                        ///
                        /// \return bool True if the repair flag was set in initInstallParameters.
                        virtual bool isRepairing() const = 0;
                        
                        /// \brief Determines if the content install flow will perform an update job.
                        ///
                        /// \return bool True if the content is installed.
                        virtual bool isUpdate() const = 0;


                        /// \brief Determines whether the conten is being worked on, and is not paused.
                        ///
                        /// \return bool True if the content is being worked on. 
                        virtual bool isBusy() { return operationType() != kOperation_None && getFlowState() != ContentInstallFlowState::kPaused; }

                        /// \brief Determines if the content is queued at the head but paused
                        virtual bool isEnqueuedAndPaused() { return mEnqueuedAndPaused; }

                        /// \brief Determines if the content is queued at the head but paused
                        virtual void setEnqueuedAndPaused(bool qnp) { mEnqueuedAndPaused = qnp; }

                        /// \brief Called when InstallerFlow is created to avoid race condition where pendingInstallArguments is emitted but the InstallerFlow isn't created yet
                        virtual void onInstallerFlowCreated() {};

                protected:
                        
                        /// \brief The IContentInstallFlow constructor.
                        ///
                        /// \param content The content which this flow will install.
                        /// \param user The owner of the content.
                        IContentInstallFlow(Origin::Engine::Content::EntitlementRef content, Engine::UserRef user);
                        
                        /// \brief Used to determine if the client is still online.
                        ///
                        /// \param warningMsgIfOnline If desired, logs a warning message if we are offline.
                        /// \return bool True if we are online and using a network URL. 
                        bool makeOnlineCheck(const QString& warningMsgIfOnline = "") const;
                        
                        /// \brief Setup the content install flow.
                        void initializeFlow();

                        /// \brief Start the flow state machine itself and send out the current progress state (if any).
                        void startFlow();
                        
                        /// \brief Check escalation service is available/that the user enables proper privileges before beginning or resuming a download
                        /// \return bool True if the escalation service is running with the proper priviledges to support the download
                        bool escalationServiceEnabled();

                        /// \brief Creates the transitions between states that are visible to the client through the stateChange signal.
                        virtual void initializePublicStateMachine() = 0;
                        
                        /// \brief During initialization, checks a saved install manifest for valid install flow start or resume state.
                        ///
                        /// A saved install manifest might jndicate an invalid state to start or resume if origin
                        /// was shutdown in the middle of a download/install or a crash occurred.
                        ///
                        /// \return bool True if the install manifest was valid or could be repaired.
                        virtual bool validateInstallManifest() = 0;
                        
                        /// \brief Sets the initial values of install manifest parameters and saves the manifest.
                        virtual void initializeInstallManifest() = 0;
                        
                        /// \brief Initiates the migration of 8.X install info to 9.0 format.
                        virtual void migrateLegacyFlowInformation();
                        
                        /// \brief Stops the content install flow during any state.
                        ///
                        /// This should only be called in rare instances, like when %Origin is about to shutdown.
                        /// Flows which have not progressed to the transferring state may get canceled.
                        /// Sets flows that get paused to auto resume and those that get canceled to auto start.
                        ///
                        /// \param autostart Should the flow start/resume without user input?
                        virtual void suspend(bool autostart) = 0;
                        
                        /// \brief TBD.
                        void initUrl();
                        
                        /// \brief Tells the content install flow in which state to begin when started or resumed.
                        ///
                        /// \param state The start state.
                        void setStartState(ContentInstallFlowState state);
                        
                        /// \brief Sets "currentstate" and "previousstate" in the install manifest.
                        ///
                        /// This method does not cause the state machine to change states. It just
                        /// sets parameters in the install manifest. "previousstate" is set to
                        /// "currentstate" and "currentstate" is set to whatever was specified as
                        /// the method parameter.
                        ///
                        /// \param state The current state.
                        void setFlowState(ContentInstallFlowState state);
                        
                        /// \brief Translates a ContentInstallFlowState enumeration value into a QState.
                        ///
                        /// \param type A ContentInstallFlowState value that represents a state.
                        /// \return QState A pointer to the QState associated with the given ContentInstallFlowState enum.
                        QState* getState(ContentInstallFlowState type);
                        
                        /// \brief Translates a QState into a ContentInstallFlowState enumeration value.
                        ///
                        /// \param state A QState that is associated with a ContentInstallFlowState value.
                        /// \return ContentInstallFlowState The enumeration value associated with the given QState.
                        ContentInstallFlowState getStateType(QState* state);

                        /// \brief Updates the mCurrentProgress.mProgress. mProgress is based off of state and operation
                        void updateProgress(const bool& forceUpdate = false);

                        /// \brief Generates a new UUID that will uniquely identify this download job, for telemetry and other internal purposes.
                        ///
                        /// New UUIDs will be generated when each unique operation is started (e.g. download, update) and will
                        /// be persisted until the operation is canceled or completed.
                        void generateNewJobId();

                        /// \brief Clears the UUID that uniquely identifies this download job, for telemetry and other internal purposes.
                        ///
                        /// This ID gets cleared when an operation is canceled or completed.
                        void clearJobId();
                        
                        /// \brief Loads the install manifest for content that has completed or is in the process of downloading/installing.
                        ///
                        /// If the install manifest file does not exist, the install flow was never started or the content
                        /// was uninstalled.
                        ///
                        /// \return bool True if the manifest exists and was readable.
                        bool loadInstallManifest();
                        
                        /// \brief The number of times the flow will retry downloading when http 403 are encountered.
                        static quint16 sMaxDownloadRetries;
                        
                        /// \brief Converts the content's type to a string value so that telemetry is human-readable.
                        ///
                        /// \return QString A string representing the content's type.
                        QString getContentTypeString() const;
                        
                        /// \brief Converts the content's package type to a string value so that telemetry is human-readable.  Only being used by telemetry, currently.
                        ///
                        /// \return QString A string representing the content's package type.
                        QString getPackageTypeString() const;
                        
                        /// \brief Checks if the given path contains a symbolic link (i.e. junction).
                        ///
                        /// \param path A string representing the path to a file.
                        /// \return bool True if the path contains a symbolic link.
                        bool isPathSymlink(const QString& path) const;

                        /// \brief Removes game from queue, and cancels DLC if removeChildren is true.
                        void removeFromQueue(const bool& removeChildren);
                        
                        /// \brief Safeguards read and write of "currentstate" and "previousstate" in the install manifest.
                        mutable QMutex mMutex;

                        /// \brief TBD.
                        mutable QMutex mInstallProgressLock;

                        /// \brief The UUID which is associated with this download job. (If any)
                        QString mJobId;
                        
                        /// \brief The object representing the content that this flow will download/install.
                        Origin::Engine::Content::EntitlementRef mContent;
                        
                        /// \brief The owner of the content associated with this install flow.
                        Engine::UserWRef mUser;
                        
                        /// \brief Stores status information about an active install flow.
                        ///
                        /// Keys common to all install flow derived classes:
                        /// - bool autoresume
                        /// - bool autostart
                        /// - qint64 contentversion
                        /// - QString currentstate
                        /// - QString id
                        /// - bool installdesktopshortcut
                        /// - bool installstartmenushortcut
                        /// - QString previousstate
                        /// - qint64 savedbytes
                        /// - qint64 totalbytes
                        ///
                        Parameterizer mInstallManifest;
                        
                        /// \brief The url currently being used for this install flow.
                        QString mUrl;
                        
                        /// \brief Is there an override url in the ini file?
                        bool mIsOverrideUrl;
                        
                        /// \brief The current number of consecutive times the install flow has retried the download after an http 403 error.
                        quint16 mDownloadRetryCount;

                        /// \brief Whether or not this flow should have its error dialogs suppressed
                        bool mSuppressDialogs;

						/// \brief Is this install flow paused because an unretry-able error occurred - for adding new items to download queue above this
						bool mPausedForUnretryableError;

                        /// \brief Does this content have an error that needs to pause the queue (i.e. not move to bottom of queue - e.g. out of disk space)
                        bool mErrorPauseQueue;

                protected slots:
                        
                        /// \brief Called by derived classes when a progressUpdate signal needs to be emitted.
                        ///
                        /// \param bytesProgressed Number of bytes downloaded so far.
                        /// \param totalBytes Total number of bytes to download.
                        /// \param bytesPerSecond Current rate of byte download.
                        /// \param estimatedSecondsRemaining Number of seconds to complete download.
                        void onProgressUpdate(qint64 bytesProgressed, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining);
                        
                        /// \brief Uploads error information to the telemetry servers.  Called whenever we encounter a non-retryable error.
                        ///
                        /// \param errorInfo Object containing any relevant information about the error encountered.
                        void sendCriticalErrorTelemetry(Origin::Downloader::DownloadErrorInfo errorInfo);

                private slots:
                        
                        /// \brief Called upon entering an install flow state.
                        ///
                        /// Logs the state change and emits a stateChanged signal.
                        void onStateChange();
                        
                        /// \brief Called upon going offline or coming online.
                        ///
                        /// When going offline, pauses or suspends install flows that require a network connection.
                        /// Flows are restarted in ContentController::processAutoDownloads when My Games is refreshed
                        /// after coming back online.
                        ///
                        /// \param onlineAuth Set to true if online and authenticated.
                        void onConnectionStateChanged(bool onlineAuth);

                        ///  \brief Resets our progress values
                        void onStopped();

                private:
                        
                        /// \brief Creates all states enumerated in ContentInstallFlowState and adds them to the state machine.
                        void initializeStates();
                        
                        /// \brief Sets up a transition to the error state for all states created in initializeStates.
                        void initializeErrorTransitions();
                        
                        /// \brief Stores the type of installflow action that is currently running, e.g. downloading, updating, repairing etc.
                        void storeFlowAction();

                        /// \brief Map of states enumerated in ContentInstallFlowState to their corresponding QState object.
                        typedef QMap<ContentInstallFlowState, QState*> StateMap;
                        
                        /// \brief Map of QState object pointers in the state machine to their corresponding state enum.
                        typedef QMap<QState*, ContentInstallFlowState> ReverseStateMap;
                        
                        /// \brief Map of states enumerated in ContentInstallFlowState to their corresponding QState object..
                        StateMap mTypeToStateMap;
                        
                        /// \brief Map of QState object pointers in the state machine to their corresponding state enum..
                        ReverseStateMap mStateToTypeMap;
                        
                        /// \brief The current progress of a flow in the install state.
                        InstallProgressState mCurrentProgress;

                        LastActionInfo mLastFlowAction;

                        /// \brief Is this content paused at the head of the queue
                        bool mEnqueuedAndPaused;
        };
        
                        
        /// \brief Calls a content install flow's handleError method when the transition is fired. 
        class ORIGIN_PLUGIN_API ErrorTransition : public QSignalTransition
        {
            public:
                        
                /// \brief Constructor.
                ///
                /// \param flow The content install flow who's handleError method should be called.
                ErrorTransition(IContentInstallFlow* flow);

            protected:
                        
                /// \brief Calls handleError with error info contained in a QEvent.
                ///
                /// \param event The state transition event.
                virtual void onTransition(QEvent* event);
        };

} // namespace Downloader
} // namespace Origin

#endif // CONTENTSERVICES_H
