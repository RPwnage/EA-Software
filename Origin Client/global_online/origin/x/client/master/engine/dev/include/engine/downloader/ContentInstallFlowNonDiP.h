/////////////////////////////////////////////////////////////////////////////
// ContentInstallFlowNonDiP.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CONTENTINSTALLFLOWNONDIP_H
#define CONTENTINSTALLFLOWNONDIP_H

#include "engine/downloader/IContentInstallFlow.h"
#include "services/settings/Setting.h"
#include "services/downloader/MounterQt.h"
#include "services/connection/ConnectionStates.h"
#include "engine/content/ContentTypes.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Downloader
{
        class ContentProtocolSingleFile;
        class ContentProtocolPackage;
        class IContentProtocol;

        class CryptSession;
        class InstallSession;

        /// \brief TBD.
        class ORIGIN_PLUGIN_API ContentInstallFlowNonDiP : public IContentInstallFlow
        {
            Q_OBJECT

            public:

                /// \brief Delayed delete to allow internal async mechanisms/references (like protocols running on separate threads) to stop in a "controlled way" / avoid race conditions
                Q_INVOKABLE virtual void deleteAsync();

                /// \brief Retrieves the total number of bytes that comprise the content associated with this content install flow.
                ///
                /// \return qint64 The value assigned to "totalbytes" in the install manifest.
                virtual qint64 getContentTotalByteCount();

                /// \brief Retrieves the total number of bytes that comprise the content associated with this content install flow.
                ///
                /// \return qint64 The total compressed size of the game that need to be downloaded
                virtual qint64 getContentSizeToDownload();

                /// \brief Retrieves the number of bytes that have been downloaded.
                ///
                /// \return qint64 The value assigned to "savedbytes" in the install manifest.
                virtual qint64 getDownloadedByteCount();

                /// \brief Retrieves the path of the on-disk folder where the content will be installed.  Persisted at the start of the flow.
                ///
                /// \return QString An absolute path to the non-DiP download cache path that should be used for the content.
                virtual QString getDownloadLocation();

                /// \brief The current number of consecutive times the install flow has retried the download after an http 403 error.
                virtual quint16 getErrorRetryCount() const;

                /// \brief Initializes installation parameters.
                ///
                /// \param installLocale The locale to use for installation, i.e. en_US.
                /// \param srcFile For ITO, the disc file to install. Parameter will be empty for network downloads.
                /// \param flags Contains bit flag details about the install.
                virtual void initInstallParameters (QString& installLocale, QString& srcFile, ContentInstallFlags flags);

                /// \brief The UI invokes this method in response to the pendingInstallArguments signal.
                ///
                /// \param args Contains user input from the install info dialog.
                Q_INVOKABLE virtual void setInstallArguments (const Origin::Downloader::InstallArgumentResponse& args);

                /// \brief No-op.
                ///
                /// \param lang Not used.
                Q_INVOKABLE virtual void setEulaLanguage (const Origin::Downloader::EulaLanguageResponse& lang);

                /// \brief No-op.
                ///
                /// \param eulaState Not used.
                Q_INVOKABLE virtual void setEulaState (const Origin::Downloader::EulaStateResponse& eulaState);

                /// \brief Starts the content install flow.
                ///
                /// If ContentInstallFlowNonDiP::canBegin returns false, emits a warn signal with error type
                /// FlowError::CommandInvalid and does not start the install flow.
                Q_INVOKABLE virtual void begin();

                /// \brief Resumes a paused install flow.
                ///
                /// If ContentInstallFlowNonDiP::canResume returns false, emits a warn signal with error type
                /// FlowError::CommandInvalid and does not resume the install flow.
                Q_INVOKABLE virtual void resume();

                /// \brief Pauses a content install flow.
                /// 
                /// - If ContentInstallFlowNonDiP::canPause returns false and autoresume is false, emits a warn signal with error type
                /// FlowError::CommandInvalid and does not pause the install flow.
                /// - If ContentInstallFlowNonDiP::canPause returns false and autoresume is set to true,
                /// ContentInstallFlowDiP::suspend is executed.
                ///
                /// \param autoresume Set the "autoresume" parameter in the install manifest to this value. If true, will cause the flow to automatically resume on Origin restart or coming online after going offline.
                Q_INVOKABLE virtual void pause(bool autoresume);

                /// \brief  Cancels a content install flow.
                ///
                /// If ContentInstallFlowNonDiP::canCancel returns false, emits a warn signal with error type
                /// FlowError::CommandInvalid and does not cancel the install flow.                /// \brief TBD.
                Q_INVOKABLE virtual void cancel(bool forceCancelAndUninstall = false);

                /// \brief No-op. ITO is not used for non DIP content.
                Q_INVOKABLE virtual void retry();

				/// \brief Used to start a download off the download queue
				Q_INVOKABLE virtual void startTransferFromQueue (bool manual_start = false);

                /// \brief Invoked by the 3PDD install dialog.
                ///
                /// \param continueInstall If true, the install dialog was accepted.
                Q_INVOKABLE virtual void install(bool continueInstall);

                /// \brief Processes errors that occur during content download/install.
                ///
                /// Gets executed automagically when the flow emits an error signal.
                ///
                /// \param errorType The type of error. See ContentDownloadError.
                /// \param errorCode Additional info about the error, i.e. 403 for type DownloadError::HttpError
                /// \param errorMessage Description of the error.
                Q_INVOKABLE virtual void handleError(qint32 errorType, qint32 errorCode , QString errorMessage);

                /// \brief Let install flow know that the entitlement's configuration was updated and give it a chance
                /// to reconfigure itself.
                Q_INVOKABLE virtual void contentConfigurationUpdated();

                /// \brief Is the content install flow currently in a valid state to begin?
                ///
                /// \return bool Returns true if ContentInstallFlowNonDiP::begin can be safely invoked.
                virtual bool canBegin();

                /// \brief Is the content install flow currently in a state where an orderly shutdown can occur?
                ///
                /// \return bool Returns true if ContentInstallFlowNonDiP::cancel can be safely invoked.
                virtual bool canCancel();

                /// \brief Is the content install flow currently in a state where a pause can occur with minimal data loss?
                ///
                /// \return bool Returns true if ContentInstallFlowNonDiP::pause can be safely invoked.
                virtual bool canPause();

                /// \brief Is the content install flow currently in a valid state to resume?
                ///
                /// \return bool Returns true if ContentInstallFlowNonDiP::resume can be safely invoked.
                virtual bool canResume();

                /// \brief ITO is not used for non DIP content.
                ///
                /// \return bool false.
                virtual bool canRetry();

                /// \brief Determines if the content install flow is active.
                ///
                /// A flow is not active only in the following states: kReadyToStart, kCompleted, and kInvalid.
                /// 
                /// \return bool True if active.
                virtual bool isActive();

                /// \brief Non DIP content can't be repaired.
                ///
                /// \return bool false.
                virtual bool isRepairing() const;

                /// \brief Non DIP content can't be updated.
                ///
                /// \return bool false.
                virtual bool isUpdate() const;

            signals:

                /// \brief Emitted when a 3PDD install dialog is rejected or a suspend occurrs during ContentInstallFlowState::kPreInstall.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kPreInstall to ContentInstallFlowState::kReadyToInstall
				void BackToReadyToInstall();

                /// \brief Emitted when an initial install is started.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kReadyToStart to ContentInstallFlowState::kInitializing
                void BeginInitializeProtocol();

                /// \brief Emitted after file transfer or when a 3PDD install dialog is accepted.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kPostTransfer to ContentInstallFlowState::kReadyToInstall
                /// - ContentInstallFlowState::kPreInstall to ContentInstallFlowState::kInstalling
                void BeginInstall();

                /// \brief Emitted when the content protocol has finished processing a transfer.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kTransferring to ContentInstallFlowState::kPostTransfer
                void DownloadProtocolFinished();

                /// \brief Emitted when the content protocol emits Started.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kPreTransfer to ContentInstallFlowState::kTransferring
                void DownloadProtocolStarted();

                /// \brief Emitted when the installer has finshed and an install check has been passed or when the content uses an unmonitored installer.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kInstalling to ContentInstallFlowState::kPostInstall
                void FinishInstall();

                /// \brief Emitted when the client invokes ContentInstallFlowNonDiP::setInstallArguments.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kPendingInstallInfo to ContentInstallFlowState::kPreTransfer
                void InstallArgumentsSet();

                /// \brief Emitted when an error occurred during ContentInstallFlowState::kInstalling.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kError to ContentInstallFlowState::kReadyToInstall
                void InstallFailed();

                /// \brief Emitted when the flow is started for 3PDD content and the installer exists but an install check fails.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kReadyToStart to ContentInstallFlowState::kReadyToInstall
                void InstallFor3pddIncomplete();

                /// \brief Emitted when an install check fails after the installer finishes, a suspend occurs during installer execution, or the installer reports an error.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kInstalling to ContentInstallFlowState::kReadyToInstall
                void PauseInstall();

                /// \brief Emitted when an error occurs during the transfer state or ContentInstallFlowNonDiP::pause is executed.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kTransferring to ContentInstallFlowState::kPausing
                /// - ContentInstallFlowState::kError to ContentInstallFlowState::kPausing
                void Pausing();

                /// \brief Emitted when the install protocol emits Paused, an error occurs in post transfer, or the max number of retrys has been met for a retryable error during transfer.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kTransferring to ContentInstallFlowState::kPaused
                /// - ContentInstallFlowState::kPostTransfer to ContentInstallFlowState::kPaused
                /// - ContentInstallFlowState::kPausing to ContentInstallFlowState::kPaused
                /// - ContentInstallFlowState::kError to ContentInstallFlowState::kPaused
                void ProtocolPaused();

                /// \brief Emitted when ContentInstallFlowNonDiP::SetContentSize is executed.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kInitializing to ContentInstallFlowState::kPendingInstallInfo
                /// - ContentInstallFlowState::kInstalling to ContentInstallFlowState::kPreTransfer
                void ReceivedContentSize(qint64, qint64);

                /// \brief Emitted when ContentInstallFlowNonDiP::resume is called, the current state is ContentInstallFlowState::kReadyToInstall and the content is not 3PDD.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kReadyToInstall to ContentInstallFlowState::kInstalling
                void ResumeInstall();

                /// \brief Emitted when the content protocol emits Resumed.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kResuming to ContentInstallFlowState::kTransferring
                void ResumeTransfer();

                /// \brief Emitted when the flow is in the ContentInstallFlowState::kPaused state and the content protocol emits Initialized.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kPaused to ContentInstallFlowState::kResuming
                void Resuming();

                /// \brief Emitted for 3PDD content when the flow is started and the installer is already downloaded.
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kReadyToStart to ContentInstallFlowState::kPreInstall
                void ShowUnmonitoredInstallDialog();

                /// \brief Emitted when ContentInstallFlowNonDiP::resume is called, the current state is ContentInstallFlowState::kReadyToInstall and the content is 3PDD.
                ///
                /// - ContentInstallFlowState::kReadyToInstall to ContentInstallFlowState::kPreInstall
                void WaitingToInstall();
            
                /// \brief Emitted when the mount process has begun
                ///
                /// Transitions:
                /// - ContentInstallFlowState::kPostTransfer to ContentInstallState::kMounting
                void BeginMount();
            
                /// \brief Emitted when the unmount process has begun
                ///
                /// Transitions: 
                /// - ContentInstallFlowState::kInstalling to ContentInstallState::kUnmounting
                void BeginUnmount();

				/// \brief Emitted when the enqueued entitlement is ready to start downloading
				///
				/// Transitions: 
				/// - ContentInstallFlowState::kEnqueued to kPreTransfer
				void StartPreTransfer();

            
            public:

                /// \brief The ContentInstallFlowNonDiP constructor.
                ///
                /// \param content TBD.
                /// \param user TBD.
                ContentInstallFlowNonDiP(Origin::Engine::Content::EntitlementRef content, Origin::Engine::UserRef user);

                /// \brief The ContentInstallFlowNonDiP destructor; releases the resources of an instance of the ContentInstallFlowNonDiP class.
                ~ContentInstallFlowNonDiP();

            private slots:

                /// \brief Initializes the protocol after receiving a JIT URL from Services::DownloadUrlServiceClient.
                ///
                /// Executed when Services::DownloadUrlResponse::finished is emitted.
                void CompleteSetupProtocolWithJitUrl();

                /// \brief Initiates JIT url generation and creates the protocol.
                void SetupDownloadProtocol();

                /// \brief Executed when IContentProtocol::Canceled is emitted.
                ///
                /// - Resets the state machine
                /// - Destroys the protocol
                /// - Reinitializes data structures.
                void OnCanceled();

                /// \brief Executed when ContentInstallFlowState::kCompleted is entered.
                ///
                /// - Destroys the protocol.
                /// - Resets the state machine.
                /// - Deletes files in the download cache if the user has not selected to keep installers.
                void OnComplete();

                /// \brief Executed when the protocol emits IContentProtocol::Resumed.
                ///
                /// Emits ContentInstallFlowNonDiP::ResumeTransfer.
                void OnDownloadProtocolResumed();

                /// \brief Executed when ContentInstallFlowState::kError is entered.
                ///
                /// - Causes transition to ContentInstallFlowState::kPaused if error occurred during
                /// ContentInstallFlowState::kTransferring, ContentInstallFlowState::kPostTransfer, or
                /// ContentInstallFlowState::kResuming.
                ///
                /// - Causes transition to ContentInstallFlowState::kReadyToInstall if error occurred
                /// during ContentInstallFlowState::kInstalling.
                ///
                /// - Maintains state if error occurred in ContentInstallFlowState::kPaused or
                /// ContentInstallFlowState::kReadyToInstall.
                ///
                /// - For all other states, resets the main state machine back to ContentInstallFlowState::kReadyToStart.
                void OnFlowError();

                /// \brief Executed when ContentInstallFlowState::kPreInstall is entered.
                ///
                /// For 3PDD content, instructs the client to show the 3PDD install dialog.
                void OnPreInstall();

                /// \brief  Executed when ContentInstallFlowState::kInstalling is entered.
                ///
                /// - If downloaded content is an executable file, the file is executed.
                /// - If the downloaded content is a packed file which contains an installer,
                /// the installer is executed.
                void OnInstall();

                /// \brief Executed when the install session emits InstallSession::installFailed.
                ///
                /// Will initiate a transition to ContentInstallFlowState::kReadyToInstall.
                ///
                /// \param errorInfo Error details.
                void OnInstallerError(Origin::Downloader::DownloadErrorInfo errorInfo);

                /// \brief Executed when install session emits InstallSession::installFinished.
                ///
                /// Does an install check and initiates a transition to ContentInstallFlowState::kCompleted.
                void OnInstallerFinished();

                /// \brief Executed when ContentInstallFlowState::kPendingInstallInfo is entered.
                ///
                /// Instructs the client to display the download information dialog.
                void OnInstallInfo();

				/// \brief Executed when ContentInstallFlowState::kEnqueued is entered.
				///
				/// Place this job on the download queue
				void onEnqueued();

                /// \brief Handles changes in the global inter-net connection status.
                ///
                /// Handles computer wakeup events and ensures that active downloads are resumed
                void OnGlobalConnectionStateChanged(Origin::Services::Connection::ConnectionStateField field);

                /// \brief Executed when ContentInstallFlowState::kPaused is entered.
                ///
                /// Will execute ContentInstallFlowNonDiP::resume if the pause was the result of a retriable error.
                void OnPaused();

                /// \brief Executed when ContentInstallFlowState::kPostInstall is entered.
                void OnPostInstall();

                /// \brief Executed when ContentInstallFlowState::kPostTransfer is entered.
                ///
                /// - For single file content, confirms that the file exists. If not, emits an error which puts
                /// the flow into a paused state allowing a download retry.
                /// - For single file executables and unpacked files, initiates a transition to the install state.
                // All other file types transition to the completed state.
                void OnPostTransfer();

                /// \brief Executed when ContentInstallFlowState::kPreTransfer is entered.
                ///
                /// Starts up the protocol.
                void OnPreTransfer();

                /// \brief Executed when the protocol emits IContentProtocol::IContentProtocol_Error.
                ///
                /// - Ignores protocol errors if offline.
                /// - Ignores DownloadError::NotConnectedError.
                /// - Initiates a retry of the download for HTTP 403 errors and JIT URL request errors.
                /// - Emits IContentInstallFlow::IContentInstallFlow_error for all other errors.
                ///
                /// \param errorInfo Container for error details.
                void OnProtocolError(Origin::Downloader::DownloadErrorInfo errorInfo);

                /// \brief Executed when the protocol emits IContentProtocol::Finished.
                ///
                /// Emits ContentInstallFlowNonDiP::DownloadProtocolFinished
                void OnDownloadProtocolFinished();

                /// \brief Executed when the protocol emits IContentProtocol::Initialized.
                ///
                /// Instructs the protocol to retrieve the content size unless the flow is currently paused which
                /// indicates a retriable protocol error occurred during transfer; the protocol was reinitialized,
                /// and now must be told to resume.
                void OnDownloadProtocolInitialized();

                /// \brief Executed when the protocol emits IContentProtocol::Paused.
                ///
                /// - Stops the state machine for flows that were suspended prior to reaching
                /// ContentInstallFlowState::kTransferring.
                /// - Destroys the protocol.
                /// - Emits ContentInstallFlowDiP::protocolPaused.
                void OnProtocolPaused();

                /// \brief  Executed when ContentInstallFlowState::kReadyToStart is entered.
                ///
                /// Jumps to the ready to install state for content that has an unmonitored install, an existing
                /// installer, and is not installed.
                void OnReadyToStart();

				/// \brief  Executed when ContentInstallFlowState::kReadyToInstall is entered.
				///
				/// used to put title on Completed queue when ready to install
				void OnReadyToInstall();

                /// \brief Executed when the protocol emits IContentProtocol::Started.
                ///
                /// Emits ContentInstallFlowNonDiP::DownloadProtocolStarted.
                void OnDownloadProtocolStarted();

                /// \brief Executed when QStateMachine::stopped is emitted.
                ///
                /// Sets start state to ContentInstallFlowState::kReadyToStart and restarts the state machine.
                void OnStopped();

                /// \brief Executed when the protocol emits IContentProtocol::TransferProgress.
                ///
                /// - Saves current progress to the install manifest.
                /// - Emits the progress update to the client.
                /// 
                /// \param bytesDownloaded Number of bytes downloaded so far.
                /// \param totalBytes Total number of bytes to download.
                /// \param bytesPerSecond Current rate of byte download.
                /// \param estimatedSecondsRemaining Number of seconds to complete download.
                void OnTransferProgress(qint64 bytesDownloaded, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining);

                /// \brief Executed when ContentInstallFlowState::kTransferring is entered.
                void OnTransferring();
            
                /// \brief Executes when the ContentInstallFlow::kMounting state is entered
                /// 
                void OnMounting();
            
                /// \brief Executes when the mount process is complete
                ///
                /// - Emits BeginInstall()
                void OnMountComplete(int status, QString output);
            
                /// \brief Executes when the ContentInstallFlow::kUnmounting state is entered
                ///
                void OnUnmounting();
            
                /// \brief Executes when the unmount process is complete
                ///
                /// - Emits FinishInstall()
                void OnUnmountComplete(int status);
            
                /// \brief Executed when the protocol emits IContentProtocol::ContentLength in response to the install flow invoking IContentProtocol::GetContentLength.
                ///
                /// Emits ContentInstallFlowNonDiP::ReceivedContentSize
                ///
                /// \param numBytesInstalled The size of the content in bytes on disk after decompression.
                /// \param numBytesToDownload The size of the content in bytes that need to be downloaded.  (compressed)
                void SetContentSize(qint64 numBytesInstalled, qint64 numBytesToDownload);

                /// \brief Requests a JIT URL for content download from Services::DownloadUrlServiceClient.
                void MakeJitUrlRequest();

                /// \brief once the async content is deleted, delete itself
                void onProtocolDeleted();
            
            protected:

                /// \brief  Creates the transitions between states that are visible to the client through the stateChange signal.
                virtual void initializePublicStateMachine();

                /// \brief During initialization, checks a saved install manifest for valid install flow start or resume state.
                ///
                /// A saved install manifest might indicate an invalid state to start or resume if origin
                /// was shutdown in the middle of a download/install or a crash occurred.
                ///
                /// \return bool True if the install manifest was valid or could be repaired.
                virtual bool validateInstallManifest();

                /// \brief If in ready to install state, checks to make sure the configured installer still exists
                /// If not, resets the flow and returns false.
                virtual bool validateReadyToInstallState();

                /// \brief Sets the initial values of install manifest parameters and saves the manifest.
                ///
                /// Additional parameters added for DiP flows:
                /// - QString serverfile
                virtual void initializeInstallManifest();

                /// \brief Pauses or stops the flow depending on current state.
                ///
                /// This should only be called in rare instances, like when Origin is about to shutdown.
                /// Flows which have not progressed to the transferring state may get canceled.
                /// Sets flows that get paused to auto resume and those that get canceled to auto start.
                ///
                /// \param autostart If true, the flow will automatically re-start on entitlment refresh.
                virtual void suspend(bool autostart);

            private:

                /// \brief Initializes ContentInstallFlowNonDiP::mDownloadProtocol.
                void BeginDownloadProtocolInitialization();

                /// \brief Instantiates ContentInstallFlowNonDiP::mDownloadProtocol.
                ///
                /// Determines which subclass of Downloader::IContentProtocol to create by examining
                /// the content package type and server file extension.
                void CreateDownloadProtocol();

                /// \brief Builds an absolute path for single file download location.
                ///
                /// - If it is downloaded as a single file and not unpacked, this is where the file
                /// on the server ends up after download.
                /// - Path string contains forward slashes '/'.
                ///
                /// \return QString The absolute path to server file download location.
                QString GetServerFile() const;

                /// \brief Determines if a string references an executable file extension.
                ///
                /// \param extension The string to test.
                /// \return bool True for extensions "exe" and "msi".
                bool IsExecutableFileExtension(const QString& extension) const;
            
                /// \brief Determines if a string references a mountable disk image
                ///
                /// \param extension The string to test.
                /// \return bool True for extensions "dmg" and "iso".
                bool IsDiskImageFileExtension(const QString& extension) const;

                /// \brief Determines if a string references a packed file extension.
                ///
                /// \param extension The string to test.
                /// \return bool True for extensions "viv" and "zip".
                bool IsPackedFileExtension(const QString& extension) const;

                /// \brief Builds an absolute path to the content's installer file.
                ///
                /// - Path string contains forward slashes '/'.
                /// - Defaults to "[Content Download Cache Path]/setup.exe".
                /// \return QString An absolute path to the content's installer file.
                QString GetInstallerPath() const;

                /// \brief Send telemetry when a download is about to begin
                ///
                /// \param status What status is the download in (new or resume..)
                /// \param savedBytes How many bytes have been saved if this is a resume
                void sendDownloadStartTelemetry(const QString& status, qint64 savedBytes);

                /// \brief The protocol which this install flow uses to download the content.
                IContentProtocol* mDownloadProtocol;

                /// \brief Executes the content's installer.
                InstallSession* mInstaller;

                /// \brief Flag to indicate an unmonitored install was initiated. 
                bool mUnmonitoredInstallInProgress;

                /// \brief Flag to indicate if the install flow should try a resume when pausing is completed, after a download error is encountered.
                bool mResumeOnPaused;

                /// \brief Flag to indicate that we're suspending the flow for logout or exit.
                bool mProcessingSuspend;
            
                /// \brief Wrapper object that houses mount / unmount platform signals
                MounterQt mounter;

        };


} // namespace Downloader
} // namespace Origin

#endif // CONTENTINSTALLFLOWNONDIP_H
