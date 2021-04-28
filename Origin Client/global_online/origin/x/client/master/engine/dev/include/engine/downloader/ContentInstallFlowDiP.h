/////////////////////////////////////////////////////////////////////////////
// ContentInstallFlowDiP.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CONTENTINSTALLFLOWDIP_H
#define CONTENTINSTALLFLOWDIP_H

#include "ContentInstallFlowContainers.h"
#include "services/settings/Setting.h"
#include "services/connection/ConnectionStates.h"
#include "services/plugin/PluginAPI.h"
#include "engine/content/DynamicContentTypes.h"
#include "engine/downloader/IContentInstallFlow.h"
#include "engine/login/User.h"
#include "engine/content/ContentTypes.h"
#include <QFile>
#include <QList>
#include <QMap>
#include <QTemporaryFile>

#include <stdint.h>

namespace Origin
{

namespace Downloader
{
    class DiPManifest;
    class DiPUpdate;
    class ContentServices;
    class ContentProtocolPackage;
    class InstallSession;
    class InstallProgress;
            
    /// \brief Directs the download/installation, update, and repair of Download In Place content.
    class ORIGIN_PLUGIN_API ContentInstallFlowDiP : public IContentInstallFlow
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
            /// \return QString An absolute path to the DiP install path that should be used for the content.
            virtual QString getDownloadLocation();

            /// \brief The current number of consecutive times the install flow has retried the download after an http 403 error.
            virtual quint16 getErrorRetryCount() const;
            
            /// \brief Initializes installation parameters.
            ///
            /// \param installLocale The locale to use for installation, i.e. en_US.
            /// \param srcFile For ITO, the disc file to install. Paramter will be empty for network downloads.
            /// \param flags Contains bit flag details about the install.
            virtual void initInstallParameters (QString& installLocale, QString& srcFile, ContentInstallFlags flags);
                               
            /// \brief The UI invokes this method in response to the pendingInstallArguments signal.
            ///
            /// \param args Contains user input from the install info dialog.
            Q_INVOKABLE virtual void setInstallArguments (const Origin::Downloader::InstallArgumentResponse& args);
            
            /// \brief The UI invokes this method in response to the pendingInstallLanguage signal.
            ///
            /// \param lang Contains user input from the language selection dialog.
            Q_INVOKABLE virtual void setEulaLanguage (const Origin::Downloader::EulaLanguageResponse& lang);
            
            /// \brief The UI invokes this method in response to the pendingEulaState signal.
            ///
            /// \param eulaState Contains user response to eula dialogs.
            Q_INVOKABLE virtual void setEulaState (const Origin::Downloader::EulaStateResponse& eulaState);
            
            /// \brief Starts the content install flow.
            ///
            /// If ContentInstallFlowDiP::canBegin returns false, emits a warn signal with error type
            /// FlowError::CommandInvalid and does not start the install flow.
            Q_INVOKABLE virtual void begin();
            
            /// \brief Resumes a paused install flow.
            ///
            /// If ContentInstallFlowDiP::canResume returns false, emits a warn signal with error type
            /// FlowError::CommandInvalid and does not resume the install flow.
            Q_INVOKABLE virtual void resume();
            
            /// \brief Pauses a content install flow.
            /// 
            /// - If ContentInstallFlowDiP::canPause returns false and autoresume is false, emits a warn signal with error type
            /// FlowError::CommandInvalid and does not pause the install flow.
            /// - If ContentInstallFlowDiP::canPause returns false and autoresume is set to true,
            /// ContentInstallFlowDiP::suspend is executed.
            ///
            /// \param autoresume Set the "autoresume" parameter in the install manifest to this value. If true, will cause the flow to automatically resume on Origin restart or coming online after going offline.
            Q_INVOKABLE virtual void pause(bool autoresume);
            
            /// \brief  Cancels a content install flow.
            ///
            /// If ContentInstallFlowDiP::canCancel returns false, emits a warn signal with error type
            /// FlowError::CommandInvalid and does not cancel the install flow.
            Q_INVOKABLE virtual void cancel(bool forceCancelAndUninstall = false);
            
            /// \brief Used during ITO installs to resume after a disc change.
            ///
            /// If ContentInstallFlowDiP::canRetry returns false, emits a warn signal with error type
            /// FlowError::CommandInvalid and does not retry the install flow.
            Q_INVOKABLE virtual void retry();

			/// \brief Used to start a download off the download queue
			Q_INVOKABLE virtual void startTransferFromQueue (bool manual_start = false);
            
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
            /// \return bool Returns true if ContentInstallFlowDiP::begin can be safely invoked.
            virtual bool canBegin();
            
            /// \brief Is the content install flow currently in a state where an orderly shutdown can occur?
            ///
            /// \return bool Returns true if ContentInstallFlowDiP::cancel can be safely invoked.
            virtual bool canCancel();
            
            /// \brief Is the content install flow currently in a state where a pause can occur with minimal data loss?
            ///
            /// \return bool Returns true if ContentInstallFlowDiP::pause can be safely invoked.
            virtual bool canPause();
            
            /// \brief Is the content install flow currently in a valid state to resume?
            ///
            /// \return bool Returns true if ContentInstallFlowDiP::resume can be safely invoked.
            virtual bool canResume();
            
            /// \brief Is the content install flow currently in a valid state to resume?
            ///
            /// \return bool Returns true if ContentInstallFlowDiP::retry can be safely invoked.
            virtual bool canRetry();
            
            /// \brief Determines if the content install flow is active.
            ///
            /// A flow is not active only in the following states: kReadyToStart, kCompleted, and kInvalid.
            /// 
            /// \return bool True if active.
            virtual bool isActive();
            
            /// \brief Determines if the content install flow will perform a repair job.
            ///
            /// \return bool True if the repair flag was set in initInstallParameters.
            virtual bool isRepairing() const;
            
            /// \brief Determines if the content install flow will perform an update job.
            ///
            /// \return bool True if the content is installed.
            virtual bool isUpdate() const;
            
            /// \brief Used to determine if the flow is using a local source for content download, i.e. disc install.
            ///
            /// \return bool True if the flow is not downloading via the network.
            virtual bool isUsingLocalSource() const;

            /// \brief Used to determine if the flow is currently operating in 'non-DiP-to-DiP update' mode
            ///
            /// \return bool True if the flow is currently operating on a non-DiP install.
            virtual bool isNonDipInstall() const;

            /// \brief Used to determine if the installation is being initiated externally (i.e. install from disk or from downloaded installer)
            ///
            /// \return bool True if the installation is launched externally
            virtual bool isITO() const;

            /// \brief Used to determine if the installation is currently acting as a Dynamic Download/Progressive Installation
            ///
            /// \return bool True if the installation is a DD/PI
            virtual bool isDynamicDownload(bool *contentPlayableStatus = NULL) const;

            /// \brief Used to determine if the content is using a touch-up installer that emits progress percentages.
            ///
            /// \return bool True if the content has a touch-up installer that emits progress.
            virtual bool isInstallProgressConnected() const;

            /// \brief Used to access the currently selected language 
            ///
            /// \return Language if set, or empty if not.
            virtual const QString& currentLanguage() const { return mEulaLanguageResponse.selectedLanguage; }

        signals:
                    
            /// \brief Emitted when an initial install is started.
            ///
            /// Transitions:
            /// -  ContentInstallFlowState::kReadyToStart to ContentInstallFlowState::kInitializing
			void beginInitializeProtocol();
            
            /// \brief Emitted after destaging on initial installs and on some resumes.
            ///
            /// Transition:
            /// - ContentInstallFlowState::kPostTransfer to ContentInstallFlowState::kInstalling.
            /// - ContentInstallFlowState::kReadyToInstall to ContentInstallFlowState::kInstalling.
            void beginInstall();
            
            /// \brief Emitted when the install protocol emits a SourceFound signal.
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kPendingDiscChange to ContentInstallFlowState::kTransferring
            void discChanged();
            
            /// \brief Emitted when updated EULAs are accepted.
            void eulaStateSet();

            /// \brief Emitted when the user has accepted all End User License Agreements.
            void finishedEulas();
            
            /// \brief Emitted when all optional End User License Agreements have been processed.
            void finishedOptionalEulas();
            
            /// \brief Emitted when the installer has finshed and an install check has been passed.
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kInstalling to ContentInstallFlowState::kCompleted
            void finishedTouchupInstaller();
            
            /// \brief Emitted when arguments for the installer exe have been collected from the user or aren't necessary.
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kPendingInstallInfo to ContentInstallFlowState::kPendingEula
            void installArgumentsSet();
            
            /// \brief Emitted when an error occurs or the client shuts down during the install state.
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kInstalling to ContentInstallFlow::kReadyToInstall
            /// - ContentInstallFlowState::kError to ContentInstallFlow::kReadyToInstall
            void pauseInstall();
            
            /// \brief Emitted when an error occurs during the transfer state or ContentInstallFlowDiP::pause is executed.
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kTransferring to ContentInstallFlowState::kPausing
            /// - ContentInstallFlowState::kError to ContentInstallFlowState::kPausing
            void pausing();
            
            /// \brief Emitted after a check of OS prerequistes for the content has passed.
			void prequisitesMet();
            
            /// \brief Emitted after DiP manifest excludes, deletes, and ignores have been processed.
			void processedExclusionsAndDeletes();
            
            /// \brief Emitted after the DiP manifest file has been received and loaded into ContentInstallFlowDiP::mManifest.
			void processedManifest();
            
            /// \brief Used during ITO. Emitted when the install protocol emits CannotFindSource.
            ///
            /// Transtitions:
            /// - ContentInstallFlowState::kTransferring to ContentInstallFlowState::kPendingDiscChange
            ///
            /// \param currentDisc Not used.
            /// \param numDiscs Not used.
            /// \param wrongDisc Not used.
            void protocolCannotFindSource(int currentDisc, int numDiscs, int wrongDisc);
            
            /// \brief Emitted when the content protocol has finished processing a transfer.
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kTransferring to ContentInstallFlowState::kPostTransfer
            /// - ContentInstallFlowState::kPendingEula to ContentInstallFlowState::kCompleted. This transition occurs on updates when the content is current.
            void protocolFinished();
            
            /// \brief Emitted when the install protocol emits Initialized.
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kInitializing to ContentInstallFlowState::kPreTransfer
            void protocolInitialized();
            
            /// \brief Emitted when the install protocol emits Paused, an error occurs in post transfer, or the max number of retrys has been met for a retryable error during transfer.
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kTransferring to ContentInstallFlowState::kPaused
            /// - ContentInstallFlowState::kPostTransfer to ContentInstallFlowState::kPaused
            /// - ContentInstallFlowState::kPausing to ContentInstallFlowState::kPaused
            /// - ContentInstallFlowState::kResuming to ContentInstallFlowState::kPaused
            /// - ContentInstallFlowState::kError to ContentInstallFlowState::kPaused
            void protocolPaused();
            
            /// \brief Emitted when the content protocol emits Resumed.
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kResuming to ContentInstallFlowState::kTransferring
            void protocolResumed();
            
            /// \brief Emitted when the content protocol emits Started.
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kEnqueued to ContentInstallFlowState::kTransferring
            void protocolStarted();
            
            /// \brief Emitted when ContentInstallFlowDiP::setContentSize is executed.
            ///
            /// \param numBytesInstalled The size of the content in bytes on disk after decompression.
            /// \param numBytesToDownload The size of the content in bytes that need to be downloaded.  (compressed)
            void receivedContentSize(qint64 numBytesInstalled, qint64 numBytesToDownload);
            
            /// \brief Emitted when ContentInstallFlowDiP::receiveFileData is executed or a desired file already exists in ContentInstallFlowDiP::mPendingTransferFiles.
            ///
            /// \param filePath The absolute path to the received file.
            void receivedFile(const QString& filePath);
            
            /// \brief If in the paused state, emitted when ContentInstallFlowDiP::resume is executed.
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kPaused to ContentInstallFlowState::kResuming
            void resuming();

			/// \brief Emitted when the pending Eula is done and ready to enqueue.
			///
			/// Transitions:
			/// - ContentInstallFlowState::kPendingEula to ContentInstallFlowState::kEnqueued
			void pendingEulaComplete();

			/// \brief Emitted when repair is to be enqueued.
			///
			/// Transitions:
			/// - ContentInstallFlowState::kReadyToStart to ContentInstallFlowState::kEnqueued
			void enqueueRepair();

            /// \brief If in the transferring state, emitted when the flow has determined that all required game components have been downloaded and are ready to be installed (destaged).
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kTransferring to ContentInstallFlowState::kPostTransfer
            void dynamicDownloadRequiredChunksReadyToInstall();

            /// \brief If in the transferring state, emitted when the flow has determined that all required game components have been installed (destaged).
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kPostTransfer to ContentInstallFlowState::kInstalling
            void dynamicDownloadRequiredChunksComplete();

            /// \brief If in the complete state (post touchup), emitted when the flow has determined that the protocol is still downloading, and we should resume waiting for the transfer.
            ///
            /// Transitions:
            /// - ContentInstallFlowState::kComplete to ContentInstallFlowState::kTransferring
            void resumeExistingTransfer();

			void postInstallToComplete();

        public:
            
            /// \brief The ContentInstallFlowDiP constructor.
            ///
            /// \param content The content which this flow will install.
            /// \param user The owner of the content.
            ContentInstallFlowDiP(Origin::Engine::Content::EntitlementRef content, Origin::Engine::UserRef user);
            
            /// \brief The ContentInstallFlowDiP destructor; releases the resources of an instance of the ContentInstallFlowDip class.
            ~ContentInstallFlowDiP();
            
            /// \brief Retrieve the DiP manifest object for this install flow.
            ///
            /// \return DiPManifest A pointer to the DiPManifest.
            DiPManifest* getDiPManifest() const;
            
            /// \brief Retrieve the protocol for this install flow.
            ///
            /// The protocol will be NULL while the flow is paused or not active.
            ///
            /// \return ContentProtocolPackage A pointer to the ContentProtocolPackage.
            ContentProtocolPackage* getProtocol() const;
            
            /// \brief Retrieve the install manifest for this install flow.
            ///
            /// \return Parameterizer A reference to the install manifest object.
            Parameterizer& getInstallManifest();

            /// \brief Called when InstallerFlow is created to avoid race condition where pendingInstallArguments is emitted but the InstallerFlow isn't created yet
            void onInstallerFlowCreated();

            /// \brief Called when the DiPUpdate sub-machine pauses due to the content running, this just emits a signal from the flow
            void onPendingContentExit();

            /// \brief Removes special characters from EULA filename.
            ///
            /// \return QString Returns a sanitized version of the EULA filename for use in the install manifest.
            QString getCleanEulaID(const QString& originalFilename);

        public slots:
            
            /// \brief Executed when the protocol emits ContentProtocolPackage::FileDataReceived in response to the install flow invoking ContentProtocolPackage::GetSingleFile.
            ///
            /// \param archivePath The relative path of the received file in the content's central directory.
            /// \param diskPath The absolute path where the file was locally stored.
            void receiveFileData(const QString& archivePath, const QString& diskPath);
            
            /// \brief Executed when the protocol emits IContentProtocol::ContentLength in response to the install flow invoking IContentProtocol::GetContentLength.
            ///
            /// \param numBytesInstalled The size of the content in bytes on disk after decompression.
            /// \param numBytesToDownload The size of the content in bytes that need to be downloaded.  (compressed)
            void setContentSize(qint64 numBytesInstalled, qint64 numBytesToDownload);
            
            /// \brief Executed when the protocol emits IContentProtocol::CannotFindSource.
            ///
            /// Emits IContentProtocol::pendingDiscChange to the client.
            ///
            /// \param nextDiscNum The next disc required to resume the install.
            /// \param totalDiscNum The total number of discs required to install the content.
            /// \param wrongDisc If the user inserted the wrong disc during the last prompt, the number of the disc that was inserted.
            /// If < 0, the user inserted the correct disc during the last prompt.
            void promptForDiscChange (int nextDiscNum, int totalDiscNum, int wrongDisc);

        private slots:
            
            /// \brief Determines if the user's OS meets the minimum OS specified in the DiP manifest.
            void checkPrerequisites();
            
            /// \brief Requests DiP manifest retrieval from the protocol.
            void retrieveDipManifest();
            
            /// \brief Requests file retrieval from the protocol for each End User License Agreement listed in the DiP manifest.
            void getEulas();
            
            /// \brief Initiates JIT url generation, creates the protocol, sets protocol repair and update flags.
            void initializeProtocol();
            
            /// \brief Executed when IContentProtocol::Canceled is emitted.
            ///
            /// Resets the main state machine, destroys the protocol, and reinitializes data structures.
            void onCanceled();
            
            /// \brief Executed when ContentInstallFlowState::kCompleted is entered.
            ///
            /// Disconnects install session and install progress signals. Destroys the protocol.
            /// Resets the main state machine. Destroys the update finalization state machine.
            void onComplete();
            
            /// \brief Executed when ContentInstallFlowState::kPendingEulaLangSelection is entered.
            /// 
            /// If the install language has not already been selected, initiates language selection from the user
            /// by emitting IContentInstallFlow::pendingInstallLanguage.
            void onEulaLanguageSelection();
            
            /// \brief Executed when ContentInstallFlowState::kError is entered.
            ///
            /// Causes transition to ContentInstallFlowState::kPaused if error occurred during
            /// ContentInstallFlowState::kTransferring, ContentInstallFlowState::kPostTransfer, or
            /// ContentInstallFlowState::kResuming.
            ///
            /// Causes transition to ContentInstallFlowState::kReadyToInstall if error occurred
            /// during ContentInstallFlowState::kInstalling.
            ///
            /// Maintains state if error occurred in ContentInstallFlowState::kPaused or
            /// ContentInstallFlowState::kReadyToInstall.
            ///
            /// For all other states, resets the main state machine back to ContentInstallFlowState::kReadyToStart.
            void onFlowError();
            
            /// \brief  Executed when ContentInstallFlowState::kReadyToStart is entered.
            void onReadyToStart();
            
            /// \brief  Executed when ContentInstallFlowState::kInstalling is entered.
            ///
            /// Installer is executed for all initial downloads with an installer, all repairs with an
            /// installer, and updates where the installer file changed or the DiP manifest has the force
            /// touchup installer flag set.
            void onInstall();
            
            /// \brief Executed when the install session emits InstallSession::installFailed.
            ///
            /// Will transition to ContentInstallFlowState::kReadyToInstall.
            ///
            /// \param errorInfo Error details.
			void onInstallerError(Origin::Downloader::DownloadErrorInfo errorInfo);
            
            /// \brief Executed when install session emits InstallSession::installFinished.
            ///
            /// Does an install check and initiates a transition to ContentInstallFlowState::kCompleted.
            void onInstallFinished();
            
            /// \brief Executed when ContentInstallFlowState::kPendingInstallInfo is entered.
            ///
            /// No op for PDLC and updates. For initial main content download, instructs the client to
            /// display the download information dialog.
			void onInstallInfo();
            
            /// \brief Handles changes in the global inter-net connection status.
            ///
            /// Handles computer wakeup events and ensures that active downloads are resumed
            void onGlobalConnectionStateChanged(Origin::Services::Connection::ConnectionStateField field);

            /// \brief Executed when ContentInstallFlowState::kPaused is entered.
            ///
            /// Will execute ContentInstallFlowDiP::resume if the pause was the result of a retriable error.
			void onPaused();

            /// \brief Executed pre-start for updates, to check if there are any EULAs
            void onPendingUpdateEulaState();
            
            /// \brief Executed when ContentInstallFlowState::kPendingEula is entered.
            ///
            /// No op if the flow is executing an update or the EULAs have already been accepted. Otherwise,
            /// the client is instructed to show a set of eula files.
			void onPendingEulaState();

			/// \brief Executed when ContentInstallFlowState::kEnqueued is entered.
			///
			/// Place this job on the download queue
			void onEnqueued();
            
            /// \brief Executed when ContentInstallFlowState::kPendingDiscChange is entered.
            ///
            /// The install flow waits in this state until ContentInstallFlowDiP::discChanged
            /// is emitted.
			void onPendingDiscChange();

			/// \brief Executed when ContentInstallFlowState::kPostInstall is entered.
			void OnPostInstall();

            /// \brief Executed when ContentInstallFlowState::kPostTransfer is entered.
            ///
            /// If the install flow is for PDLC or is executing an update, a Downloader::DiPUpdate
            /// finalization state machine is started.
			void onPostTransfer();
            
            /// \brief Executed when ContentInstallFlowState::kPreTransfer is entered.
            ///
            /// Starts up a mini state machine to retrieve and process the DiP manifest.
			void onPreTransfer();
            
            /// \brief Informs the protocol which files to exclude, delete, and ignore.
            ///
            /// Information is obtained from the DiP manifest.
			void onProcessExclusionsAndDeletes();
            
            /// \brief Executed when the protocol emits IContentProtocol::IContentProtocol_Error.
            ///
            /// - Ignores protocol errors if offline and not using a local source for download.
            /// - Ignores DownloadError::NotConnectedError.
            /// - Initiates a retry of the download for HTTP 403 errors and JIT URL request errors.
            /// - Emits IContentInstallFlow::IContentInstallFlow_error for all other errors.
            ///
            /// \param errorInfo Container for error details.
			void onProtocolError(Origin::Downloader::DownloadErrorInfo errorInfo);
            
            /// \brief Executed when the protocol emits IContentProtocol::Finished.
            ///
            /// Emits ContentInstallFlowDiP::protocolFinished
            void onProtocolFinished();

            /// \brief Executed when the protocol emits DynamicRequiredPortionSizeComputed
            ///
            /// Used only for dynamic downloads to indicate what the computed size of the dynamic portion is
            void onDynamicRequiredPortionSizeComputed(qint64 requiredPortionSize);

            /// \brief Executed when the protocol emits ContentProtocolPackage::DynamicChunkReadyToInstall.
            ///
            /// Tracks internal state.
            void onDynamicChunkReadyToInstall(int chunkId);

            /// \brief Executed when the DiPUpdate sub-machine emits DiPUpdate::finalizeUpdate.
            ///
            /// Performs update finalization actions.  (Destaging or installing chunks)
            void onUpdateReadyToFinalize();

            /// \brief Executed when the protocol emits ContentProtocolPackage::Finalized.
            ///
            /// Resumes the completion of an update after the protocol destaged everything.
            void onUpdateFinalized();

            /// \brief Executed when the protocol emits ContentProtocolPackage::DynamicChunkInstalled.
            ///
            /// Emits ContentInstallFlowDiP::dynamicDownloadRequiredChunksComplete (if all necessary chunks are available)
            void onDynamicChunkInstalled(int chunkId);
            
            /// \brief Executed when the protocol emits IContentProtocol::Initialized.
            ///
            /// Emits ContentInstallFlowDiP::protocolInitialized.
            void onProtocolInitialized();
            
            /// \brief Executed when the protocol emits IContentProtocol::Paused.
            ///
            /// - Stops the main state machine for flows that were suspended prior to reaching
            /// ContentInstallFlowState::kTransferring.
            /// - For install flows not using a local source, destroys the protocol.
            /// - Emits ContentInstallFlowDiP::protocolPaused.
            void onProtocolPaused();
            
            /// \brief Executed when the protocol emits IContentProtocol::Resumed.
            ///
            /// Emits ContentInstallFlowDiP::protocolResumed.
            void onProtocolResumed();
            
            /// \brief Executed when the protocol emits IContentProtocol::Started.
            ///
            /// Emits ContentInstallFlowDiP::protocolStarted.
            void onProtocolStarted();
            
            /// \brief Allows the state machine to continue after the protocol has retrieved all optional EULAs.
            void onReceivedOptionalEula();
            
            /// \brief Executed when the main state machine emits QStateMachine::stopped.
            ///
            /// - Destroys the DiP manifest object.
            /// - Destroys transitions between the main state machine and mini state machines.
            /// - Cleans up cached and pending file data structures.
            /// - Sets start state to ContentInstallFlowState::kReadyToStart and restarts the main state machine.
            void onStopped();
            
            /// \brief Executed when ContentInstallFlowState::kTransferring is entered.
            void onTransferring();
            
            /// \brief Executed when the protocol emits IContentProtocol::CannotFindSource.
            ///
            /// Emits IContentInstallFlowDiP::protocolCannotFindSource
            /// \param currentDisc Not used.
            /// \param numDiscs Not used.
            /// \param wrongDisc Not used.
            /// \param discEjected The current disc was ejected during an ITO install
            void onProtocolCannotFindSource(int currentDisc, int numDiscs, int wrongDisc, bool discEjected);
            
            /// \brief Executed when the protocol emits IContentProtocol::SourceFound.
            /// \param discEjected True if the disc was ejected in the middle of an install.
            void onProtocolSourceFound( bool discEjected );

            /// \brief Process information contained within the DiP manifest.
            ///
            /// - Creates Downloader::DiPManifest object
            /// - Removes unsupported languages related to gray market
            /// - Populates fields in ContentInstallFlowDiP::mInstallArgumentRequest, ContentInstallFlowDiP::mInstallLanguageRequest,
            /// and ContentInstallFlowDiP::mEulaStateRequest.
            void processDipManifest();
            
            /// \brief Executed when the protocol emits IContentProtocol::TransferProgress.
            ///
            /// - Saves current progress to the install manifest.
            /// - Emits the progress update to the client.
            /// 
            /// \param bytesDownloaded Number of bytes downloaded so far.
            /// \param totalBytes Total number of bytes to download.
            /// \param bytesPerSecond Current rate of byte download.
            /// \param estimatedSecondRemaining Number of seconds to complete download.
            void protocolTransferProgress(qint64 bytesDownloaded, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondRemaining);
            
            /// \brief Requests retrieval of optional eulas files from the protocol.
            void requestOptionalEulas();
            
            /// \brief Instructs the protocol to resume.
            ///
            /// If not using a local source for download, checks for network connection prior
            /// to resuming the protocol.
            void resumeProtocol();
            
            /// \brief Instructs the protocol to start.
            ///
            /// If not using a local source for download, checks for network connection prior
            /// to starting the protocol.
            void startProtocol();
            
            /// \brief Initializes the protocol after receiving a JIT URL from Services::DownloadUrlServiceClient.
            ///
            /// Executed when Services::DownloadUrlResponse::finished is emitted.
            void completeInitializeProtocolWithJitUrl();
            
            /// \brief Sets ContentInstallFlowDiP::mInstallProgressConnected when ContentInstallFlowDiP::mInstallProgress
            /// emits Downloader::InstallProgress::Connected.
            void onInstallProgressConnected();
            
            /// \brief Unsets ContentInstallFlowDiP::mInstallProgressConnected when ContentInstallFlowDiP::mInstallProgress
            /// emits Downloader::InstallProgress::Disconnected.
            void onInstallProgressDisconnected();
            
            /// \brief Informs the client of installer progress.
            ///
            /// Executed when ContentInstallFlowDiP::mInstallProgress emits Downloader::InstallProgress::ReceivedStatus
            /// 
            /// \param progress Percent complete. 0.0 - 1.0
            /// \param description Not used.
            void onInstallProgressUpdate(float progress, QString description);
            
            /// \brief Informs the client of crc check progress during a repair.
            ///
            /// \param currentProgress How many units have been processed so far. 
            /// \param totalProgressToComplete Total number of units to process.
            /// \param progressPerSecond Current progress rate.
            /// \param estimatedSecondsRemaining Number of seconds to complete processing.
			void onNonTransferProgressUpdate(qint64 currentProgress, qint64 totalProgressToComplete, qint64 progressPerSecond, qint64 estimatedSecondsRemaining);

            /// \brief Requests a JIT URL for content download from Services::DownloadUrlServiceClient.
            void makeJitUrlRequest();

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
            
            /// \brief Sets the initial values of install manifest parameters and saves the manifest.
            ///
            /// Additional parameters added for DiP flows:
            /// - bool eulasaccepted
            /// - bool installDesktopShortCut
            /// - bool installStartMenuShortCut
            /// - int optionalComponentsToInstall
            /// - bool isito
            /// - bool installerChanged
            /// - int stagedFileCount
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
            
            /// \brief Instanitates the protocol object and sets up signal connections.
            void createProtocol();
            
            /// \brief Deletes files downloaded to temporary locations.
            ///
            /// Clears ContentInstallFlowDiP::mTemporaryFiles and ContentInstallFlowDiP::mPendingTransferFiles.
            void cleanupTemporaryFiles();
            
            /// \brief Checks if the content associated with this install flow is PDLC.
            ///
            /// \return bool Returns true if the content is PDLC.
            bool contentIsPdlc() const;
            
            /// \brief Builds an absolute path to the content's installer file.
            ///
            /// \return QString An absolute path to the content's installer file or an empty string if the content does not have an installer.
            QString getTouchupInstallerExecutable();
            
            /// \brief Builds a string of parameters need by the content's installer.
            ///
            /// The parameter list is obtained from the DiP manifest and parameter values are
            /// obtained from user responses to install dialogs presented by the client.
            ///
            /// Possible parameters included in the string:
            /// - install location
            /// - install locale
            /// - create a desk top icon
            /// - create a start menu icon
            /// - list of optional components to install
            ///
            /// \return QString A string of parameters.
            QString getTouchupInstallerParameters();
            
            /// \brief Sets initial values for ContentInstallFlowDiP::mEulaStateRequest.
            void initializeEulaStateRequest();
            
            /// \brief Sets initial values for ContentInstallFlowDiP::mInstallArgumentRequest.
            void initializeIntstallArgumentRequest();
            
            /// \brief Instantiates ContentInstallFlowDiP::mUpdateFinalizationStateMachine and connects signals.
            void initializeUpdate();
            
            /// \brief Determines if this content flow will perform an update job when started and sets ContentInstallFlowDiP::mIsUpdate.
            void initializeUpdateFlag();
            
            /// \brief When a download/install is performed over more than one Origin session, this method reads information from the install manifest that was stored during the previous session.
            void initiateIntersessionResume();
            
            /// \brief Loads the DiP manifest file into ContentInstallFlowDiP::mManifest.
            ///
            /// \return bool Returns true if the load was successful.
            bool loadDipManifestFile();
            
            /// \brief Determines if the protocol has downloaded all eula files.
            ///
            /// \return bool Returns true if all eula files are available.
			bool receivedAllEulas();
            
            /// \brief Resumes a download that was paused during ContentInstallFlowState::kTransferring or ContentInstallFlowState::kPostTransfer.
            ///
            /// - Creates a mini state machine to re-download the DiP manifest.
            /// - Initializes the protocol.
            void resumeTransfer();
            
            /// \brief Checks if the current BF3 install is at a version where the crc map is incorrect and the install needs to be repaired.
            ///
            /// EBISUP-3905
            ///
            /// \return bool True if the BF3 install needs to be repaired
            bool checkIfShouldRepairBF3Installation() const;

            bool checkIfRequiredChunksReadyToInstall() const;

            void installAllReadyChunks();

            bool checkIfRequiredChunksInstalled() const;

            bool checkIfAllChunksInstalled() const;

            /// \brief Starts up a mini state machine that downloads and processes the DiP manifest.
            ///
            /// \param previousState Which state is the main state machine currently in.
            /// \param nextState Which state in the main state machine should previousState transition to when the mini state machine finishes.
            void StartDiPManifestMachine(QState* previousState, QState* nextState);
            
            /// \brief Starts up a mini state machine that performs administrative tasks prior to content transfer, such as eula retrieval.
            ///
            /// \param previousState Which state is the main state machine currently in.
            /// \param nextState Which state in the main state machine should previousState transition to when the mini state machine finishes.
            void StartPreTransferMachine(QState* previousState, QState* nextState);
            
			/// \brief Send telemetry when a download is about to begin
            ///
            /// \param status What status is the download in (new or resume..)
            /// \param savedBytes How many bytes have been saved if this is a resume
			void sendDownloadStartTelemetry(const QString& status, qint64 savedBytes);

            /// \brief The protocol which this install flow uses to download the content.
            ContentProtocolPackage* mProtocol;
            
            /// \brief The archive path to disk path of files.
            QMap<QString,QString> mPendingTransferFiles;
            
            /// \brief List of temporary files downloaded by the protocol which should be deleted when the flow is complete or stopped.
            QList<QString> mTemporaryFiles;
            
            /// \brief Container for information provided by the content's DiP manifest file..
            DiPManifest* mManifest;
            
            /// \brief A state machine used to finalize update jobs after file transfer.
            DiPUpdate* mUpdateFinalizationStateMachine;
            
            /// \brief Executes the content's installer.
            InstallSession* mInstaller;
            
            /// \brief Provides install progress to the client for some content.
            InstallProgress* mInstallProgress;
            
            /// \brief  Container for information to display in the client's install info dialog.
            InstallArgumentRequest  mInstallArgumentRequest;
            
            /// \brief Container for information to display in the client's install language dialog.
            InstallLanguageRequest  mInstallLanguageRequest;
            
            /// \brief  Container for information to display in the client's eula dialog(s).
            EulaStateRequest        mEulaStateRequest;
            
            /// \brief Container for user responses to the client's install info dialog.
            InstallArgumentResponse mInstallArgumentResponse;
            
            /// \brief Container for user responses to the client's eula dialog(s).
            EulaStateResponse       mEulaStateResponse;
            
            /// \brief Container for user responses to the client's install language dialog.
            EulaLanguageResponse    mEulaLanguageResponse;
            
            /// \brief Flag to indicate if this install flow performing a repair job.
            bool mRepair;
            
            /// \brief Flag to indicate if ContentInstallFlowDiP::mInstallProgress exists and is connected to the install flow.
            bool mInstallProgressConnected;
            
            /// \brief Flag to indicate if the install progress signals were hooked up (and would need to be unhooked)
            bool mInstallProgressSignalsConnected;

            /// \brief Flag to indicate if this install flow is performing an update job.
            bool mIsUpdate;

            /// \brief Flag to indicate if this repair is being done before the game is fully installed.
            bool mIsPreinstallRepair;
            
            /// \brief Flag to indicate if the install flow should try a resume when pausing is completed, after a download error is encountered.
            bool mResumeOnPaused;

            /// \brief Flag to indicate that we're suspending the flow for logout or exit.
            bool mProcessingSuspend;

            /// \brief Flag to indicate that we're running in dynamic download mode.  (Progressive Install)
            bool mIsDynamicDownload;

            /// \brief Flag to indicate that we need to finalize the dynamic download.
            bool mIsDynamicUpdateFinalize;

            struct DynamicDownloadChunkState
            {
                enum ChunkInstallState { kUnknown = 0, kSubmitted, kReadyToInstall, kInstalling, kInstalled };

                DynamicDownloadChunkState() : chunkId(-1), state(kUnknown), requirement(Origin::Engine::Content::kDynamicDownloadChunkRequirementUnknown) { }
                int chunkId;
                ChunkInstallState state;
                Origin::Engine::Content::DynamicDownloadChunkRequirementT requirement;
            };

            /// \brief Map of priority group IDs to chunk states
            QMap<int, DynamicDownloadChunkState> mChunkState;

            /// \brief Stores the computed size of the required portion for dynamic downloads.
            qint64 mDynamicRequiredPortionSize;

            /// \brief URL/Path to the SyncPackage, if applicable (DiP 3.0+)
            QString mSyncPackageUrl;

			/// \brief Transition from post transfer to installing triggered by mUpdateFinalizationStateMachine finished signal. Needs to be removed when the update machine is deleted.
            QSignalTransition* mUpdateFinalizeTransition;

            /// \brief Main state machine transition created in ContentInstallFlowDiP::StartDipManifestMachine
            QSignalTransition* mDiPManifestFinishedTransition;

            /// \brief Main state machine transition created in ContentInstallFlowDiP::StartPreTransferMachine
            QSignalTransition* mPreTransferFinishedTransition;
            QStateMachine* mDiPManifestMachine;
            QStateMachine* mPreTransferMachine;

            /// \brief The number of times the download has been retried due to data corruption
            int mDataCorruptionRetryCount;

            /// \brief The last CDN that was in use
            QString mLastCDN;

            /// \brief Flag to indicate whether the next JIT URL request should specify to use the 'other' CDN, whatever that was
            bool mUseAlternateCDN;

            /// \brief Flag to indicate whether this session should use Downloader Safe Mode
            bool mUseDownloaderSafeMode;

            /// \brief Flag to indicate if we are waiting for the installer flow before emitting pendingInstallArguments signal in kPendingInstallInfo state
            bool mWaitingForInstallerFlow;

            /// \brief Flag to indicate if the installer flow has been created
            bool mInstallerFlowCreated;

            /// \brief Flag to indicate if the eulas have been accepted already (used for free to play optimizations where the eulas are accepted on the webpage)
            bool mEulasAccepted;

            /// \brief Flag to indicate to use the default install options (install location, create desktop/start menu options)
            bool mUseDefaultInstallOptions;

            /// \brief Flag to indicate to try to move the entitlement to the top of the queue
            bool mTryToMoveToTop;
    };

} // namespace Downloader
} // namespace Origin

#endif // CONTENTINSTALLFLOWDIP_H
