///////////////////////////////////////////////////////////////////////////////
// ContentProtocolPackage.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef CONTENTPROTOCOLZIP_H
#define CONTENTPROTOCOLZIP_H

#include "ContentProtocols.h"

#include <QTimer>
#include <QUrl>
#include <QFile>
#include <QMap>

#include "engine/content/DynamicContentTypes.h"

#include "services/downloader/DownloadService.h"
#include "services/downloader/UnpackService.h"
#include "services/downloader/CRCMap.h"

#include "services/downloader/TransferStats.h"
#include "services/downloader/MovingAverage.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Downloader
{

    class ContentServices;
    class IDownloadSession;
    class IDownloadRequest;

    class FileHandleCache;
    class FileHandleCache;
    class FileProgressMap;
    class PackageFile;
    class PackageFileEntry;
    class CRCMap;
    class CRCMapVerifyProgressProxy;
    class PatchBuilder;

    namespace ContentPackageType
    {
        /// \brief Enumeration of the different supported package types.
        enum code
        {
            Unknown = 0, ///< Unknown package type.
            Zip, ///< Zip package type.  See Downloader::ZipFileInfo.
            Viv ///< Viv package type.  See Downloader::BigFileInfo.
        };
    }

    // Used by dynamic downloads
    enum PriorityGroupEntryStatus { kEntryStatusNone = 0, kEntryStatusQueued, kEntryStatusDownloading, kEntryStatusCompleted, kEntryStatusInstalled, kEntryStatusError };

    namespace Internal
    {
        class DynamicDownloadInstanceData;
    }

    /// \brief Manages the download sessions for a packaged download (zip, viv, etc).
    ///
    /// Zip/Viv-specific implementation, this interface is not public.  Used only by DiPContentInstallFlow.
    class ORIGIN_PLUGIN_API ContentProtocolPackage : public IContentProtocol
    {
        Q_OBJECT
#ifdef _DEBUG
            friend class Internal::DynamicDownloadInstanceData;
#endif
            public:
                /// \brief The ContentProtocolPackage constructor.
                /// \param productId The product ID associated with this content.
                /// \param cachePath The path to the cache for this content.
                /// \param crcMapPrefix The path which will be prepended to all CRC map queries
                /// \param session A weak reference to the user's session.
                ContentProtocolPackage(const QString& productId, const QString& cachePath, const QString& crcMapPrefix, Services::Session::SessionWRef session);
                
                /// \brief The ContentProtocolPackage destructor.
                ~ContentProtocolPackage();

            // IContentProtocol members
            public:
                /// \brief Initializes the protocol.
                /// \param url The url to retrieve files from.
                /// \param target The directory to download files to.
                /// \param isPhysicalMedia True if this is an ITO install.
                Q_INVOKABLE virtual void Initialize( const QString& url, const QString& target, bool isPhysicalMedia = false );
                
                /// \brief Starts the protocol.
                Q_INVOKABLE virtual void Start();
                
                /// \brief Resumes the protocol.
                Q_INVOKABLE virtual void Resume();
                
                /// \brief Pauses the protocol.
                /// \param forcePause True if the protocol should stop all synchronous requests, otherwise it will pause after the current operation.
                Q_INVOKABLE virtual void Pause( bool forcePause = false );
                
                /// \brief Cancels the protocol.
                Q_INVOKABLE virtual void Cancel();
                
                /// \brief Retries the download.
                Q_INVOKABLE virtual void Retry();
                
                /// \brief Requests that the protocol retrieve the content length from the host.
                Q_INVOKABLE virtual void GetContentLength();
                
                /// \brief Requests that the protocol complete any post-transfer tasks (destaging, etc).
                Q_INVOKABLE virtual void Finalize();

            signals:
                /// \brief Emitted when a file has been successfully received from the host.
                /// \param archivePath The relative path to the file from the root install directory.
                /// \param diskPath The path to the file on disk.
                void FileDataReceived(const QString& archivePath, const QString& diskPath);

                void DynamicChunkDefined(int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT requirement, QString internalName, QSet<QString> fileList);

                void DynamicChunkUpdate(int chunkId, float chunkProgress, qint64 chunkRead, qint64 chunkSize, Origin::Engine::Content::DynamicDownloadChunkStateT chunkState);

                void DynamicChunkError(int chunkId, qint32 errCode);

                void DynamicRequiredPortionSizeComputed(qint64 requiredPortionSize);

                void DynamicChunkOrderUpdated(QVector<int> chunkIds);

                void DynamicChunkReadyToInstall(int chunkId);

                void DynamicChunkInstalled(int chunkId);

            public:
                // Asynchronous methods
                /// \brief Transfers the given single file.
                /// \param archivePath The relative path to the file from the root install directory.
                /// \param diskPath The path to the file on disk.
                Q_INVOKABLE void GetSingleFile(const QString& archivePath, const QString& diskPath);

                // Synchronous methods
                /// \brief Excludes a list of files from the files to be downloaded.
                /// \param excludePatterns The set of wildcard (EA::IO::FnMatch) patterns to be excluded from the package.  i.e. don't download or update.
                void SetExcludes(const QStringList& excludePatterns);

                /// \brief Queues a directory for deletion.
                /// \param deletePatterns The set of wildcard (EA::IO::FnMatch) patterns to be deleted after an update.  The game folders are scanned recursively to match these patterns.
                void SetDeletes(const QStringList& deletePatterns);

                /// \brief Excludes a list of files from the differential updating mechanism.
                /// \param diffExcludePatterns The set of wildcard (EA::IO::FnMatch) patterns to be excluded from differential updating.
                void SetDiffExcludes(const QStringList& diffExcludePatterns);

                void SetAutoExcludesFromIncludeList(const QStringList& allIncludePatterns, const QStringList& curIncludes);

                Q_INVOKABLE void InstallDynamicChunk(int priorityGroupId);

                Q_INVOKABLE void ActivateNonRequiredDynamicChunks();

                Q_INVOKABLE void CreateRuntimeDynamicChunk(int priorityGroupId, QString internalName, QStringList includePatterns);

                void SetPriorityGroup(int priorityGroupId, Engine::Content::DynamicDownloadChunkRequirementT requirement, const QString& internalName, const QStringList& includePatterns);

                void SetPriorityGroupOrder(QVector<int> priorityGroupIds);

                QVector<int> GetPriorityGroupOrder();

                /// \brief Flags the job as an update.
                /// \param flag True if the job is an update.
                void SetUpdateFlag(bool flag);

                /// \brief Flags the job as a repair.
                /// \param flag True if the job is a repair.
                void SetRepairFlag(bool flag);

                /// \brief Indicates to the protocol that a cancel should not wipe out game files.  (Set prior to transferring state)
                void SetSafeCancelFlag(bool flag);

                /// \brief Flags the job as eligible for differential updating.
                /// \param flag True if the job is differentially updatable.
				/// \param syncPackageUrl TBD.
                void SetDifferentialFlag(bool flag, const QString& syncPackageUrl);

                /// \brief Flags the job as a dynamic download.
                /// \param flag True if the job is a dynamic download.
                void SetDynamicDownloadFlag(bool flag);

                /// \brief Sets the unpack directory of the job.
                /// \param dir The directory to unpack files to.
                void SetUnpackDirectory(const QString& dir);

                /// \param skipCrcVerification Skips the CRC verification check - used when just downloading single files from package
                void SetSkipCrcVerification(bool skipCrcVerification);

                /// \param contentTagBlock A key-value dictionary of content tags (for watermarking)
                void SetContentTagBlock(QMap<QString,QString> contentTagBlock);

                /// \brief Assigns an key used for differentiating between parent and child content in the CRCMap.
                ///
                /// Since main content and pdlc files are stored in the same crc map, an id has to be
                /// assigned to each map entry so update and repair jobs know which files to check.
                /// \param key The ID to assign.
                void SetCrcMapKey(const QString& key);

                /// \brief Checks if a file is staged.
                /// \param archivePath The file to check.
                /// \param stagedFilename This function will set this value to the staged filename if one exists.
                /// \param newFileCRC The CRC of the file which will be downloaded.
                /// \return True if the file has been staged.
                bool IsStaged(const QString& archivePath, QString& stagedFilename, quint32& newFileCRC);

                /// \brief Checks if a given file will be downloaded in the current download job.  Can be used to determine if a file was modified.
                /// \param archivePath The file in the zip archive to check.
                /// \param newFileCRC The CRC of the file which will be downloaded.
                /// \return True if the file has been staged.
                bool IsFileInJob(const QString& archivePath, quint32& newFileCRC);

                /// \brief Gets the number of staged files.
                /// \return The number of staged files.
                qint64 stagedFileCount();

                /// \brief Processes the package file entry map and computes the download job size.
                /// \return True if the package file entry map was processed successfully.
                bool ComputeDownloadJobSizeBegin();

                /// \brief Checks if protocol is initialized and ready to go
                /// \return True if protocol is initialized and ready to go.
                bool IsInitialized() { return _protocolReady; }

            protected:
                void SendProgressUpdate();

                void SaveCRCMapPeriodically();

                /// \brief Clears all data related to the current session.
                /// \param resetProtocolState True if the protocol's state should be set to kContentProtocolUnknown.
                /// \todo Remove this now that protocols are deleted on pause?
                void ClearRunState(bool resetProtocolState = true);

                /// \brief Deletes any staged files for the download.
                /// \param zipFileDirectory A Downloader::PackageFile object containing all of the files to delete.
                void DeleteStagedFileData(PackageFile& zipFileDirectory);

                /// \brief Deletes any empty folders left over after a cleanup.
                /// \param zipFileDirectory A Download::PackageFile object containing all of the folders to check.
                void DeleteEmptyFolders(PackageFile& zipFileDirectory);

                void ClearTransferState(); ///< Clears all data related to the current download session and stops all download activity.
                void SuspendAll(); ///< Shuts down the download session.
                void ResumeAll(); ///< Resumes the transfer.
                void CheckTransferState(); ///< Makes sure that if we need to put the brakes on everything, we wait for everything to complete and then clean up.
                void Cleanup(); ///< Cleans up.
                void CleanupFreshInstallData(); ///< Cleans up all files.
                void CleanupUpdateRepairData(); ///< Cleans up only the files that were modified as part of the update/repair.

                void CreatePatchBuilder(); ///< Creates a PatchBuilder object if necessary.
                void DestroyPatchBuilder(); ///< Destroys a PatchBuilder object if necessary.
                DiffFileId GetNextPatchBuilderId(); ///< Gets a DiffFileId from PatchBuilder.  (Creates it if necessary)

                void InitializeStart(); ///< Creates the download session and initializes the protocol.

                /// \brief Called when host replies with the content length.  See GetContentLength.
                /// \param contentLength The length of the content in bytes.
                void InitializeConnected(qint64 contentLength);

                void InitializeVerify(); ///< Sets up CRC map.
                void InitializeVerified(); ///< Called when CRC map calculations have completed.
                void InitializeEnd(); ///< Finishes the initializing state.

                /// \brief Emits an error based on the given error code.
                /// \param errorCode The error encountered.
                void InitializeFail(qint32 errorCode);
                
                /// \brief Transfers the given single file.
                /// \param archivePath The relative path to the file from the root install directory.
                /// \param diskPath The path to the file on disk.
                void TransferSingleFile(const QString& archivePath, const QString& diskPath);

                void FinishGetContentLength(); ///< Finishes the job of GetContentLength(), which can complete asynchronously

                void TransferStart(); ///< Completes pre-download tasks such as preallocation and directory creation.
                void TransferStartJobReady(); ///< Continues with transfer startup after job computation.
                void TransferIssueRequests(); ///< Issues download requests.

                /// \brief Re-issues request for the given entry.
                /// \param pEntry The entry to re-issue request for.
                /// \param reasonErrType TBD.
                /// \param clearExistingFile TBD.
                /// \param enableDiagnostics Indicates that the request should have diagnostics enabled.  (For data corruption problems)
                /// \return ContentDownloadError::OK if the request was issued successfully, otherwise an error code.
                ContentDownloadError::type TransferReIssueRequest(PackageFileEntry* pEntry, ContentDownloadError::type reasonErrType, bool clearExistingFile, bool enableDiagnostics = false);

                /// \brief Creates a download request and unpack file for the given entry.
                /// \param pEntry The entry to create a download request and unpack file for.
                /// \param downloadReqId This function will populate this value with the download request ID.
                /// \param unpackFileId This function will populate this value with the unpack file ID.
                /// \param clearExistingFiles Whether or not to tell the unpack service to clobber any files that already existed.
                /// \param endOfDisc This function will populate this value with true if data will be truncated because we reached the end of the disc.
                /// \return ContentDownloadError::OK if the download request and unpack file were created successfully, otherwise an error code.
                ContentDownloadError::type CreateDownloadUnpackRequest(PackageFileEntry* pEntry, RequestId &downloadReqId, UnpackStreamFileId &unpackFileId, bool clearExistingFiles, bool &endOfDisc);

                /// \brief Deletes any existing files and renames any staged files.
                /// \return True if destaging was successful.
                bool FinalizeDestageFiles(int& errorCode);

                /// \brief Deletes any existing files and renames any staged files.
                /// \param priorityGroup Indicates which priority group should be destaged.
                /// \return True if destaging was successful.
                bool FinalizeDestageFilesForGroup(int priorityGroup, int& errorCode);

                /// \brief Checks if the given file should be staged.
                /// \param file The file to check.
                /// \return True if the file should be staged.
                bool ShouldStageFile(const QString& file) const;

                /// \brief Checks if the given package file is new (i.e. not updating an already existing file)
                /// \param pEntry The package file entry to check.
                /// \param existingSize An out-param that returns the existing size of the file we already have, if any.
                /// \return True if the file is new to the package.
                bool IsFileNew(const PackageFileEntry* pEntry, qint64 &existingSize) const;

                /// \brief Checks if the given package file should be differentially updated (if available)
                /// \param pEntry The package file entry to check.
                /// \param existingSize The existing size of the file we already have.
                /// \return True if the file should be differentially updated.
                bool ShouldDiffUpdateFile(const PackageFileEntry* pEntry, qint64 existingSize) const;

                /// \brief Gets the absolute path to the given file.
                /// \param file The file to get the absolute path for.
                /// \return The absolute path to the given file.
                QString GetFileFullPath(const QString& file) const;

                /// \brief Gets the staged filename of the given file.
                /// \param file The file to get the staged name for.
                /// \return The staged filename of the file.
                QString GetStagedFilename(const QString& file) const;

                /// \brief Gets the filename of the package manifest.
                /// \return The filename of the package manifest.
                QString GetPackageManifestFilename() const;

                /// \brief Gets the filename of the CRCMap.
                /// \return The filename of the CRCMap.
                QString GetCRCMapFilename() const;

                /// \brief Returns the filename for use with the CRC map.  (Might have the metadata install path prepended)
                /// \return The filename to use with the CRC map.
                QString GetCRCMapFilePath(const QString& filename) const;

                /// \brief Processes the CRC Map. This happens on a separate thread so this function will return immediately.
                /// \return True if the CRC map has begun being processed.
                bool ProcessCRCMap(); 

                void ComputeDownloadJobSizeEnd(bool usePatchBuilder, qint64 totalBytesToPatch);

                static void GetFileDetails(QString fullPath, bool followSymlinks, bool &fileExists, qint64 &fileSize, bool &isSymlink);

                /// \brief Processes the package file entry map and computes the download job size.
                /// \return True if the package file entry map was processed successfully.
                void ProcessPackageFileEntryMap();

                /// \brief Queues a chunk for processing.
                /// \param req The Downloader::IDownloadRequest to pull raw data from.
                /// \return True if the chunk was queued successfully.
                bool ProcessChunk(IDownloadRequest* req);

                /// \brief Checks for completion.  Emits IContentProtocol::Finished if completed.
                void ScanForCompletion();

                /// \brief Gets the path to the package file on the given disc.
                /// \param disc The disc number to build a path for.
                /// \return The path to the package file on the given disc.
                QString getPathForDisc(const int &disc);

                /// \brief Checks if the necessary disc is accessible.
                /// \return True if the necessary disc is accessible.
                bool ChangeDisc();

                /// \brief Checks if a disc change is necessary and if so, checks if it is accessible.
                /// \return True if a disc change is unnecessary, or true if it is necessary and accessible.
                bool ChangeDiscIfNeeded();

                /// \brief Checks that the given disc is accessible.
                /// \param disc The disc number to check for.
                /// \return True if the disc is accessible.
                bool IsDiscAvailable(const int &disc);

                /// \brief Checks if the given path is accessible.
                /// \param fileURL The path to check for.
                /// \return True if the given path is accessible.
				bool IsPathAvailable(const QString& fileURL);

                /// \brief Calculates offset to the file data for the given entry.
                /// \param pEntry The entry to calculate the offset for.
                /// \return True if the calculation succeeded.
                bool RetrieveOffsetToFileData( PackageFileEntry *pEntry );

                /// \brief Scans the folder structure and deletes any files or folders that match the patterns in _deletePatterns
                bool ProcessDeletes();

                void RemoveUnusedFiles();

                void UpdatePriorityGroupAggregateStats();

                void UpdatePriorityGroupChunkMap( PackageFileEntry *pEntry, qint64 totalBytesRead, qint64 totalBytesRequested, PriorityGroupEntryStatus entryStatus, qint32 errorCode = 0 );

                void CheckNonRequiredGroupsEnabled();

                QSet<int> GetEntryPriorityGroups(PackageFileEntry *pEntry);

            protected slots:
                /// \brief Slot that is triggered when a Downloader::CRCMap makes progress.
                /// \param bytesProcessed The number of bytes that have been processed.
                /// \param totalBytes The total number of bytes to process.
                /// \param bytesPerSecond The average bytes processed per second.
                /// \param estimatedSecondsRemaining The estimated time remaining in seconds.
                void onCRCVerifyProgress(qint64 bytesProcessed, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining);

                /// \brief Slot that is triggered when a Downloader::CRCMap finishes verifying CRCs.
                void onCRCVerifyCompleted();

                /// \brief Slot that is triggered when a PatchBuilder makes progress during its initialization phase
                void onPatchBuilderInitializeMetadataProgress(qint64 bytesProcessed, qint64 bytesTotal);

                /// \brief Slot that is triggered when a PatchBuilder decides it won't/can't update certain files
                void onPatchBuilderInitializeRejectFiles(DiffFileErrorList rejectedFiles);

                /// \brief Slot that is triggered when a PatchBuilder finishes initializing and indicates how large the patch data will be.
                void onPatchBuilderInitialized(qint64 totalBytesToPatch);

                /// \brief Slot that is triggered when a PatchBuilder emits progress information about a file being built
                void onPatchBuilderFileProgress(DiffFileId diffFileId, qint64 bytesProcessed, qint64 bytesTotal);

                /// \brief Slot that is triggered when a PatchBuilder emits progress information about a file being completed
                void onPatchBuilderFileComplete(DiffFileId diffFileId);

                /// \brief Slot that is triggered when a PatchBuilder completes all its work
                void onPatchBuilderComplete();

                /// \brief Slot that is triggered when a PatchBuilder is done pausing
                void onPatchBuilderPaused();

                /// \brief Slot that is triggered when a PatchBuilder is done canceling
                void onPatchBuilderCanceled();

                /// \brief Slot that is triggered when a PatchBuilder encounters a fatal error
                void onPatchBuilderFatalError(Origin::Downloader::DownloadErrorInfo errorInfo);

                void onCheckDiskTimeOut(); ///< Slot that is triggered when the check disc timer times out.
				void onPauseCancelTimeOut(); ///< Slot that is triggered when the pause/cancel timer times out.
                void onProgressUpdateTimeOut(); ///< Slot that is triggered when we want to emit transfer progress and recompute metadata.

                /// \brief Slot that is triggered when the download session has been initialized after a disc change.
                /// \param contentLength The length of the content in bytes.
                void onDownloadSessionInitializedForDiscChange(qint64 contentLength);

                /// \brief Slot that is triggered when the download session has been initialized.
                /// \param contentLength The length of the content in bytes.
                void onDownloadSessionInitialized(qint64 contentLength);
                
                /// \brief Slot that is triggered when the download session has been shut down.
				void onDownloadSessionShutdown();

                /// \brief Slot that is triggered when the download session encounters an error.
                /// \param errorInfo Downloader::DownloadErrorInfo object containing relevant information about the error.
                void onDownloadSessionError(Origin::Downloader::DownloadErrorInfo errorInfo);

                /// \brief Slot that is triggered when a new chunk is available.
                /// \param req The unique ID of the download request.
                void onDownloadRequestChunkAvailable(RequestId req);
                
                /// \brief Slot that is triggered when the download request encounters an error.
                /// \param req The unique ID of the download request.
                /// \param errorType The general type of error encountered.
                /// \param errorCode The specific error code of the error encountered.
                void onDownloadRequestError(RequestId req, qint32 errorType, qint32 errorCode);

                /// \brief Slot that is triggered when the download request is postponed.  (Meaning it got delayed for higher priority data)
                /// \param req The unique ID of the download request.
                void onDownloadRequestPostponed(RequestId req);

                /// \brief Slot that is triggered when the unpack file processes a chunk.
                /// \param id The unique ID of the unpack stream file.
                /// \param byteCount The number of bytes that have been processed.
                /// \param completed True if the chunk is the last chunk of the file.
                void onUnpackFileChunkProcessed(UnpackStreamFileId id, qint64 downloadedByteCount, qint64 processedByteCount, bool completed);

                /// \brief Slot that is triggered when the unpack service encounters an error.
                /// \param id The unique ID of the unpack stream file.
                /// \param errorInfo Downloader::DownloadErrorInfo object containing relevant information about the error.
                void onUnpackFileError(UnpackStreamFileId id, Origin::Downloader::DownloadErrorInfo errorInfo);

            private:
                QString _unpackPath; ///< Path to unpack all files and directories to.
                QString _cachePath; ///< Path to the cache.
                QString _crcMapPrefix; ///< Path to prepend on CRC map queries.
                QString _url; ///< URL to retrieve files from.
                QString _syncPackageUrl; ///< URL to retrieve the sync package from.
				bool    _physicalMedia; ///< True if this is an ITO install.
                ContentPackageType::code _packageType; ///< The package type (zip, viv, etc).
                bool    _enableITOCRCErrors;
                bool    _skipCrcVerification;
                QMap<QString,QString> _contentTagBlock;

                DownloadRateSampler mTransferStatsDecompressed;
                DownloadRateSampler mTransferStatsDownload;
                MovingAverage<double> mAverageTransferRateDecompressed;
                MovingAverage<double> mAverageTransferRateDownload;
                qint64 mLastUpdateTick;
                qint64 mLastCRCSaveTick;
                TransferStats mTransferStats; ///< TransferStats object that keeps track of download rate, etc.
                TransferStats mPatchBuilderTransferStats; ///< TransferStats object that keeps track of PatchBuilder initialization.

                bool _protocolForcePaused; ///< True if the protocol has been suspended/force-paused.

                // Run state
                bool _protocolReady; ///< True if the protocol has been initialized.
                bool _protocolShutdown; ///< True if the protocol has been shut down.

                qint64 _dataSaved; ///< The number of bytes written to disk. (uncompressed)
                qint64 _dataToSave; ///< The total number of bytes to write to disk.
                qint64 _dataAdded; ///< The total number of bytes to be added.  (New files)
                qint64 _dataDownloaded; /// The total number of compressed bytes already downloaded.
                qint64 _dataToDownload; ///< The total number of compressed bytes needed to be downloaded.

                quint32 _filesToSave; ///< The total number of files that are going to be modified.
                quint32 _filesPatchRejected; ///< The total number of files rejected by the patch builder.

                bool _fIsUpdate; ///< True if the job is an update.
                bool _fIsRepair; ///< True if the job is a repair.
                bool _fSafeCancel; ///< True if the job has not yet reached the state where the user has accepted the download.  (Disables deleting game files during a cancel)

                bool _fIsDynamicDownloadingAvailable; ///< True if the job can be used for dynamic/progressive downloading.
                bool _fIsDiffUpdatingAvailable; ///< True if the job can use differential updating.
                PatchBuilder* _patchBuilder; ///< The object used to build differential patches.
                bool          _patchBuilderInitialized; ///< True if the patch builder object is initialized.

                PackageFile* _packageFileDirectory; ///< Downloader::PackageFile object containing all of the files and directories in this download.
                CRCMap*		 _crcMap; ///< Downloader::CRCMap object containing CRCs for all of the files in this download.
                CRCMapVerifyProgressProxy* _crcMapVerifyProxy; ///< Downloader::CRCMapVerifyProgressProxy object which sends the appropriate signals for a CRC verify async operation.
                QString      _crcMapKey; ///< Key used for differentiating between parent and child content in the CRCMap.  See SetCrcMapKey.

                /// \brief Stores information about the state of a Downloader::PackageFileEntry.
                struct ORIGIN_PLUGIN_API PackageFileEntryMetadata
                {
                    /// \brief The PackageFileEntryMetadata constructor.
                    PackageFileEntryMetadata() : bytesDownloaded(0), bytesSaved(0), isStaged(false), isBeingDiffUpdated(false), downloadId(-1), diffId(-1), retryCount(0), restartCount(0), lastErrorType(static_cast<ContentDownloadError::type>(ProtocolError::UNKNOWN)) {}
                    QString diskFilename; ///< The filename on disk.
                    qint64 bytesDownloaded; ///< The number of bytes downloaded so far.
                    qint64 bytesSaved; ///< The number of bytes saved to disk so far.
                    bool isStaged; ///< True if the file has been staged.
                    bool isBeingDiffUpdated; ///< True if the file is being differentially updated with the Patch Builder.
                    RequestId downloadId; ///< A unique identifier that will link this entry with a download session request. (If any)
                    DiffFileId diffId; ///< A unique identifier that will be used to track this file with the patch builder.  (If applicable)
                    qint16 retryCount; ///< Number of times we have retried to download this entry.
                    qint16 restartCount; ///< Number of times we have restarted downloading this entry.  (Deleted existing file)
                    ContentDownloadError::type lastErrorType; ///< Last download error that led to re-download
                };

                QMap<PackageFileEntry*, PackageFileEntryMetadata> _packageFiles; ///< Map of entries in the package to their metadata objects.

                struct ORIGIN_PLUGIN_API DiffPatchMetadata
                {
                    DiffPatchMetadata() : diffId(-1), bytesBuilt(0), bytesToBuild(0), isComplete(false), packageFileEntry(NULL) {}
                    
                    DiffFileId diffId; ///< A unique identifier that will be used to track this file with the patch builder.
                    qint64 bytesBuilt; ///< The number of bytes that have already been built by the patch builder.
                    qint64 bytesToBuild; ///< The total number of bytes that will be built by the patch builder.
                    bool isComplete; ///< Whether the given file is complete or not.
                    PackageFileEntry *packageFileEntry; ///< The entry this request is in charge of downloading.
                };

                QMap<DiffFileId, DiffPatchMetadata> _diffPatchFiles;

                //QMap<QString, bool> _deleteFiles; ///< Map of files pending deletion.
                //QMap<QString, bool> _deleteDirectories; ///< Map of directories pending deletion.
                QStringList _deletePatterns; /// wildcard pattern for deletes

                QList<PackageFileEntry *> _packageFilesSorted; ///< Sorted list (by disc number) of the package file entries.

                // Transfer state
                IDownloadSession *_downloadSession; ///< Pointer to the Downloader::IDownloadSession that issues download requests.
                IUnpackStream *_unpackStream; ///< Pointer to the Downloader::IUnpackStream that handles unpacking/decompressing.

                /// \brief Stores information about the state of a Downloader::DownloadRequest.
                struct ORIGIN_PLUGIN_API DownloadRequestMetadata
                {
                    /// \brief The DownloadRequestMetadata constructor.
                    DownloadRequestMetadata() : unpackStreamId(-1), packageFileEntry(NULL), headerConsumed(false), totalBytesRead(0), totalBytesRequested(0) {}
                    UnpackStreamFileId unpackStreamId; ///< Unique ID of the unpack stream for this request.
                    PackageFileEntry *packageFileEntry; ///< The entry this request is in charge of downloading.
                    bool headerConsumed; ///< A flag which indicates whether the zip local file header has already been read, or whether it needs to be read.
                    qint64 totalBytesRead; ///< The total number of bytes that we have received so far for this request.
                    qint64 totalBytesRequested; ///< The total number of bytes that we requested.
                };

                QMap<RequestId, DownloadRequestMetadata> _downloadReqMetadata; ///< Map of DownloadRequestMetadata indexed by the download request ID.

                struct DownloadPriorityGroupEntryMetadata
                {
                    DownloadPriorityGroupEntryMetadata() : status(kEntryStatusNone), errorCode(0), totalBytesRead(0), totalBytesRequested(0) {}

                    PriorityGroupEntryStatus status;
                    qint32 errorCode;
                    qint64 totalBytesRead; ///< The total number of bytes that we have received so far for this request.
                    qint64 totalBytesRequested; ///< The total number of bytes that we requested.
                };

                struct DownloadPriorityGroupMetadata
                {
                    DownloadPriorityGroupMetadata() : groupId(-1), groupEnabled(true), requirement(Engine::Content::kDynamicDownloadChunkRequirementUnknown), status(Engine::Content::kDynamicDownloadChunkState_Unknown), hadUpdates(false), inactivityCount(-1), totalBytesRead(0), totalBytesRequested(0), pctComplete(0.0) {}

                    int groupId;
                    bool groupEnabled;
                    Engine::Content::DynamicDownloadChunkRequirementT requirement;
                    Engine::Content::DynamicDownloadChunkStateT status;
                    QString internalName;
                    bool hadUpdates;
                    int inactivityCount;
                    qint64 totalBytesRead; ///< The total number of bytes that we have received so far for this group.
                    qint64 totalBytesRequested; ///< The total number of bytes that we requested.
                    double pctComplete;
                    QHash<PackageFileEntry*, DownloadPriorityGroupEntryMetadata> entries;
                };

                QMap<int, DownloadPriorityGroupMetadata > _downloadReqPriorityGroups; ///< Map of Download Priority group IDs to associated package file entries.  (Entries can be in multiple groups)

                bool _anyPriorityGroupsUpdated;
                bool _fSteppedDownloadEnabled;
                bool _allRequiredGroupsDownloaded;
                QMap<Engine::Content::DynamicDownloadChunkRequirementT, bool> _downloadReqPriorityGroupsEnableState;

                /// \brief Stores information about the state of a Downloader::UnpackFile.
                struct ORIGIN_PLUGIN_API UnpackFileMetadata
                {
                    /// \brief The UnpackFileMetadata constructor.
                    UnpackFileMetadata() : downloadRequestId(-1), packageFileEntry(NULL) {}
                    RequestId downloadRequestId; ///< Unique ID of the download request for this unpack file.
                    PackageFileEntry *packageFileEntry; ///< The entry this unpack file is in charge of unpacking.
                };

                QMap<UnpackStreamFileId, UnpackFileMetadata> _unpackFiles; ///< Map of UnpackFileMetadata indexed by unpack file ID.

                QMap<QString, qint64> _fileNameToDiscStartMap; ///< Map of files indexed by the disc number they exist on.  Only valid for ITO installs.  Used when a file spans a disc.
                QMap<int, qint64> _discIndexToDiscSizeMap; ///< Map of disc size in bytes indexed by disc index.  Only valid for ITO installs.  Used when a file spans a disc.
				QMap<qint64, qint64> _headerOffsetTofileDataOffsetMap; ///< Map of offsets to a file's data indexed by offsets to that file's header.
                QTimer _checkDiscTimer; ///< Gives the user 5 seconds to insert the correct disc before checking/prompting again.
				QTimer _cancelPauseTimer; ///< Gives a Pause/Cancel operation 60 seconds before forcing the operation.
                QTimer _progressUpdateTimer; ///< Handles periodic progress updates and periodic metadata recalculation.

                int _requestsCompleted; ///< Number of requests completed this session.  Used for debugging only.
                int _numDiscs; ///< Number of discs in this ITO install.  Valid if this is an ITO install, 1 otherwise.
                int _currentDisc; ///< Index of current disc in this ITO install.  Valid if this is an ITO isntall, 0 otherwise.
                int _requestedDisc; ///< Index of the disc needed to continue this ITO install.  Valid if this is an ITO install, 0 otherwise.
                qint64 _currentDiscSize; ///< Size of the current size in bytes.  Valid if this is an ITO install, 0 otherwise.
                bool _discEjected; ///< Set to 'true' if we are waiting for a disc to be re-inserted (not disc change)

                QWaitCondition _delayedCancelWaitCondition; ///< QWaitCondition object in charge of waking all cancel requests that were delayed because of an synchronous network request.
                QAtomicInt _activeSyncRequestCount; ///< Number of active synchronous requests.

                /// \brief Delays a cancel until any synchronous requests have completed.
                class ORIGIN_PLUGIN_API DelayedCancel : public QRunnable
                {
                    public:
                        /// \brief The DelayedCancel constructor.
                        /// \param parent The parent protocol.
                        /// \param waitCondition The object in charge of waking all cancel requests that were delayed because of an synchronous network request.
                        DelayedCancel(ContentProtocolPackage* parent, QWaitCondition& waitCondition);

                        /// \brief Delays a cancel until any synchronous requests have completed.
                        virtual void run();

                    private:
                        ContentProtocolPackage* mParent; ///< The parent protocol.
                        QMutex mMutex; ///< The mutex.
                        QWaitCondition& mWaitCondition; ///< The QWaitCondition object in charge of waking all cancel requests that were delayed because of an synchronous network request.

                };

                /// \brief Checks if there is still work to do.
                /// \return True if there is still work to do.
                bool checkIfWorkToDo();
    };

} // namespace Downloader
} // namespace Origin

#endif // CONTENTPROTOCOLZIP_H
