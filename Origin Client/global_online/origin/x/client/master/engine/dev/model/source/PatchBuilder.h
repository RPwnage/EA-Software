///////////////////////////////////////////////////////////////////////////////
// PatchBuilder.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ORIGINPATCHBUILDER_H
#define ORIGINPATCHBUILDER_H

#include "ContentProtocols.h"
#include "services/downloader/DownloadService.h"
#include <EAPatchClient/PatchBuilder.h>
#include <EAPatchClient/Error.h>
#include <EAPatchClient/Debug.h>

#include <QObject>
#include <QString>
#include <QThread>
#include <QMap>
#include <QList>


namespace Origin
{
namespace Downloader
{
    // Forward declarations
    class PackageFile;
    class PackageFileEntry;
    class PatchBuilder;
    class MemBuffer;


    /// \brief Describes information we need about a file to be patched.
    struct PatchBuilderFileMetadata
    {
        PatchBuilderFileMetadata();

        DiffFileId diffId;                  ///< A unique identifier that will be used to track this file
        QString    originalFilename;        ///< The relative filename from the original package.
        QString    diskFilename;            ///< The relative filename to use on disk. May not match actual filename, as it may have something like DiP_STAGED appended to what the actual file name is.
        qint64     fileCompressedSize;      ///< The compressed filesize of the completed file.
        qint64     fileUncompressedSize;    ///< The uncompressed filesize of the completed file.
        quint32    fileCRC;                 ///< The final CRC value used to validate the completed file.
    };

    // PatchBuilderFileMetadata list
    typedef QList<PatchBuilderFileMetadata> PatchBuilderFileMetadataQList;


    /// \brief Implements a Qt factory for PatchBuilder.
    ///
    class PatchBuilderFactory : public QObject
    {
        Q_OBJECT

        private:
            PatchBuilderFactory();
            QMap<PatchBuilder*, QThread*> _patchBuilderThreads;

            static PatchBuilderFactory* GetInstance();

        public:
            static PatchBuilder* CreatePatchBuilder(QString rootPath, QString productId, QString gameTitle, Services::Session::SessionWRef curSession);
            static void DestroyPatchBuilder(PatchBuilder* patchBuilder);

        public slots:
            void onPatchBuilderInstanceDestroyed(QObject* obj);
    };


    /// \brief Implements differential patching
    ///
    /// This class implements differential patching via the EAPatch scheme, which is conceptually similar to rsync/zsync.
    /// The term "sync package" is used here a bit, and it refers to the differential patching information, with the "sync"
    /// part of the name coming from the sync part of rsync/zsync. A sync package in fact is just a .zip file of the app
    /// install image, along with .eaPatchDiff files next to each regular file, and all of these uncompressed (thus the 
    /// .zip file is merely a container for a directory tree).
    ///
    class PatchBuilder : public QObject,
                         public EA::Patch::DebugTraceHandler,
                         public EA::Patch::PatchBuilder::GenerateBuiltFileInfoEventCallback,
                         public EA::Patch::ErrorBase
    {
        Q_OBJECT

        friend class PatchBuilderFactory;

        private:
            PatchBuilder(const QString& rootPath, const QString& productId, const QString& gameTitle, Services::Session::SessionWRef curSession);
           ~PatchBuilder();

        public:
            // Synchronouse interface methods

            // Create a new diff file id for use in tracking files to be patched
            DiffFileId CreateDiffFileId();

            // Asynchronous interface methods

            // Initialize and begin processing metadata
            Q_INVOKABLE void Initialize(QString syncPackageUrl, PatchBuilderFileMetadataQList filesToPatch);

            // Start actual download/patching
            Q_INVOKABLE void Start();

            // Pause download/patching
            Q_INVOKABLE void Pause();

            // Cancel download/patching
            Q_INVOKABLE void Cancel();

        signals:
            // Signals emitted during the first (initialization, diff collection) phase of our work:
            void InitializeMetadataProgress(qint64 bytesProcessed, qint64 bytesTotal);                  /// We emit this when we have completed some incremental amount of completing the initialization phase. 
            void InitializeRejectFiles(DiffFileErrorList rejectedFiles);                                /// We emit this when we have determined that certain files cannot be differentially updated, for one reason or another
            void Initialized(qint64 totalBytesToPatch);                                                 /// We emit this upon completing the first (initialization) phase. The totalBytesToPatch argument reports the total number of downloaded bytes (JobSizeMetrics::jobFileDownloadSize) required to build the files in all the jobs.

            // Signals we emit during the second (patching) phase of our work:
            void Started();                                                                             /// We emit this when we begin executing the Start function in our thread.
            void Paused();                                                                              /// We emit this when we begin executing the Pause function in our thread.
            void Canceled();                                                                            /// We emit this when we begin executing the Canceled function in our thread.
            void Complete();                                                                            /// We emit this when we are done building all the files in our Job list.
            void PatchFileProgress(DiffFileId diffFileId, qint64 bytesProcessed, qint64 bytesTotal);    /// We emit this when we have completed some incremental amount of building the file identified by diffFileId.
            void PatchFileComplete(DiffFileId diffFileId);                                              /// We emit this when we complete the building of the file identified by diffFileId.

            // Generic error signal(s)
            // To do: Add more detail to this. EAPatch has a bit of detail in its error reporting.
            void PatchFatalError(Origin::Downloader::DownloadErrorInfo errorInfo);                                                     /// To do: Revise the function argument(s) to provide more info.

        private slots:
            void onDownloadSessionInitialized(qint64 contentLength);
            void onDownloadSessionError(Origin::Downloader::DownloadErrorInfo errInfo);
            void onDownloadSessionShutdown();
            void onDownloadRequestChunkAvailable(RequestId requestId);
            void onDownloadRequestError(RequestId requestId, qint32 errType, qint32 errCode);

        private:
            // Forward declarations
            struct Job;
            struct DownloadRequestData;

            // \brief Identifies the patching state.
            enum State
            {
                kStateNone,                             // 
                kStateBegin,                            // 
                kStateDownloadingPatchDiffFiles,        // e.g. Downloading files like /SomeDir/Audio.big.eaPatchDiff
                kStateBuildingDestinationFileInfo,      // e.g. Building data and files like C:\SomeApp\SomeDir\Audio.big_Staged.eaPatchBFI
                kStateDownloadingDestinationFiles,      // e.g. Building data and files like C:\SomeApp\SomeDir\Audio.big_Staged.eaPatchTemp from server data at /SomeDir/Audio.big
                kStateCopyingLocalFiles,                // e.g. Building data and files like C:\SomeApp\SomeDir\Audio.big_Staged.eaPatchTemp from C:\SomeApp\SomeDir\Audio.big 
                kStateRenamingDestinationFiles,         // e.g. C:\SomeApp\SomeDir\Audio.big_Staged.eaPatchTemp -> C:\SomeApp\SomeDir\Audio.big_Staged
                kStateEnd                               // 
            };


            /// \brief Stores information about the state of a Downloader::DownloadRequest.
            /// To consider: make a version of this for kStateDownloadingPatchDiffFiles and one for kStateDownloadingDestinationFiles. They would have small differences.
            struct DownloadRequestData
            {
                Job*                          mpJob;            // The Job associated with this request. This is a gateway to finding other information about the download request.
                EA::Patch::BuiltFileSpan*     mpBuiltFileSpan;  // This DownloadRequestData has a 1:1 correspondence to an EAPatch BuildFileSpanList entry, and upon receiving the data for this request, we'll need to update the span list entry. This field is not relevent when downloading .eaPatchDiff data.
                Origin::Downloader::RequestId mRequestId;       // The handle to the download request. 
                int64_t                       mDestFileStart;   // The position in the file build built that the [mServerFileStart, mServerFileEnd] range is based. For the case that we are downloading an entire file with a single mServerFileStart/mServerFileEnd range request, mDestFileStart will be 0.
                int64_t                       mServerFileStart; // Offset within the server .zip file for this request. 
                int64_t                       mServerFileEnd;   // Offset within the server .zip file for one-past the end of this request. (mServerFileEnd - mServerFileEnd) == download count.
                int64_t                       mBytesWritten;    // How many bytes from (mByteStart - mByteEnd) have been written. This thus indicates where the next received data will be written. The download is complete when (mBytesWritten == (mServerFileEnd - mServerFileEnd)).

                DownloadRequestData();
            };

            typedef QMap<RequestId, DownloadRequestData*> DownloadRequestDataMap;
            typedef QList<DownloadRequestData>            DownloadRequestDataList;


            /// \brief Stores information about a differential patch job, with each Job corresponding to one file.
            /// To do: Clean up the names below to have a consistent scheme using terms like "sync package," "file," etc.
            struct Job : public EA::Patch::ErrorBase
            {
                PatchBuilderFileMetadata mDiffFileMetadata;                 // File metadata that was passed in from the client

                const PackageFileEntry*  mSyncPackageDiffFileEntry;         // File entry corresponding to a .eaPatchDiff file. It's equivalent regular file is represented by mSyncPackageFileEntry below.
                EA::Patch::String        mSyncPackageDiffFileRelativePath;  // e.g. SomeDir/SomeFile.dat.eaPatchDiff. Relative path of this .eaPatchDiff file in the sync package at _syncPackageUrl. 
                EA::Patch::String        mDestinationFileRelativePath;      // e.g. SomeDir/SomeFile.dat_DiP_Staged.  Relative path of the file that will be written to.
                DownloadRequestData      mDiffFileDownloadData;             // The diff file download is always a single download of the entire file.
                EA::Patch::String        mDiffFilePath;                     // e.g. C:\SomeApp\SomeDir\SomeFile.dat.eaPatchDiff.
                EA::Patch::String        mDiffFileTempPath;                 // e.g. C:\SomeApp\SomeDir\SomeFile.dat.eaPatchDiff.eaPatchTemp. Path to the file at mDiffFileTempStream while it's being written. This path is changes from .eaPatchDiff.eaPatchTemp to .eaPatchDiff when it's complete.
                EA::Patch::File          mDiffFileTempStream;               // The .eaPatchDiff.eaPatchTemp file object. Will be renamed to .eaPatchDiff when the download of it is complete. Will be deleted when the Job is complete 
                EA::Patch::DiffData      mDiffData;                         // The DiffData itself for this file (job). It's on disk in the .eaPatchDiff file.

                const PackageFileEntry*  mSyncPackageFileEntry;             // File entry corresponding to a file. It's equivalent .eaPatchDiff file is represented by mSyncPackageDiffFileEntry above.
                EA::Patch::String        mSyncPackageFileRelativePath;      // e.g. SomeDir/SomeFile.dat. Relative path of this file in the sync package at _syncPackageUrl. This URL will be from the SyncPackage .zip file and not the base package .zip file.
                EA::Patch::String        mDestBFIFilePath;                  // e.g. C:\SomeApp\SomeDir\SomeFile.dat_Staged.eaPatchBFI.
                DownloadRequestDataList  mFileDownloadDataList;             // The file download consists of 0 or more segments from the server file. To consider: Change this from a list to something that's faster to iterate.
                EA::Patch::BuiltFileInfo mBuiltFileInfo;                    // The information on file to build, including the disk File stream itself as well as some of the file paths we use.

                Job();

                void Cleanup();
            };

            typedef QList<Job> JobList;

            /// \brief Stores information about how much work we need to do and how much has been completed already.
            struct JobSizeMetrics
            {
                qint64 jobDiffFileDownloadSize;       /// Total number of bytes we need to download of the .eaPatchDiff files.
                qint64 jobDiffFileDownloadCurrent;    /// Current number of bytes of .eaPatchDiff files we've downloaded, out of _jobDiffFileDownloadSize.
                qint64 jobDiffLocalReadSize;          /// Total number of bytes we need to read from local files in order to finish the Initialize (diff processing) phase. 
                qint64 jobDiffLocalReadCurrent;       /// Current number of bytes we've read from local files in order to finish the Initialize (diff processing) phase. 

                qint64 jobFileDownloadSize;           /// Total number of bytes we need to download of the files we're building.
                qint64 jobFileDownloadCurrent;        /// Current number of bytes we've downloaded of the files we're building.
                qint64 jobFileLocalCopySize;          /// Total number of bytes we need to copy from local files to the files we're building.
                qint64 jobFileLocalCopyCurrent;       /// Current number of bytes we've copied from local files to the files we're building.

                JobSizeMetrics() { memset(this, 0, sizeof(*this)); }
            };

        private:
            void Shutdown();

            void StopAllTransfers();
            void ShutdownIfAble();

            void ProcessReceivedDiffData(DownloadRequestData* pDownloadRequestData, const MemBuffer * receivedData);
            void ProcessCompletedDiffFileDownload(Job& job);

            void ProcessReceivedFileSpanData(DownloadRequestData* pDownloadRequestData, const MemBuffer * receivedData);
            void ProcessCompletedFileDownload(Job& job);

            void HandleEAPatchDebugTrace(const EA::Patch::DebugTraceInfo& info, intptr_t userContex); // This function implements our parent class DebugTraceHandler interface.
            
            // Sets our error state and calls PropogateDownloaderError.
            // There are a number of functions here, but they all do the same thing, simply via different starting inputs.
            void HandleEAPatchError(Job* pJob, EA::Patch::ErrorClass errorClass, EA::Patch::SystemErrorId systemErrorId, EA::Patch::EAPatchErrorId eaPatchErrorId, const char* pFile, int line);
            void HandleEAPatchError(Job* pJob, const EA::Patch::Error& eaPatchError, const Origin::Downloader::DownloadErrorInfo& errInfo);
            void HandleDownloaderError(Job* pJob, const Origin::Downloader::DownloadErrorInfo& errInfo);
            void HandleDownloaderError(Job* pJob, qint32 downloaderErrorType, qint32 downloaderErrorCode, const char* pFile, int line);

            QString GetEAPatchContextString(const EA::Patch::Error& eaPatchError);

            // The Propogate functions serve to pass on errors to our owner. They don't do anything more than this.
            void PropogateEAPatchError(Job* pJob, const EA::Patch::Error& eaPatchError, const Origin::Downloader::DownloadErrorInfo& errInfo);                        // This simply translates the error and calls PropogateDownloaderError.
            void PropogateDownloaderError(Job* pJob, const EA::Patch::Error& eaPatchError, const Origin::Downloader::DownloadErrorInfo& errInfo);
          //void PropogateDownloaderError(Job* pJob, qint32 downloaderErrorType, qint32 downloaderErrorCode, const char* pFile, int line);  // Creates a DownloadErrorInfo and calls PropogateDownloaderError with it.

            // Files related to clearing temp files, usually as part of cancelling the job.
            bool RemoveAllTempFiles(Job* pJob);
            bool RemoveAllTempFiles();

            void CancelJob(Job* pJob);
            void CancelJobDownloads(Job* pJob);

        public:

            static void ClearTempFilesForEntry(QString root, QString fileRelativePath, QString stagedFileRelativePath);

        private:
            // Debug function which prints a list of all jobs.
            #if defined(_DEBUG) || defined(EA_DEBUG)
                void TraceJobList();
            #endif

            /// Internal function which indicates if we have been cancelled, encountered an error, or some other reason we should stop and quit.
            bool ShouldContinue();

            /// Returns the current state of the build. You can also passively listen for 
            /// state changes via the PatchBuilderEventCallback mechanism. You cannot set 
            /// the state; it can be set only internally during processing.
            State GetState() const;
            void  SetState(PatchBuilder::State state);

            /// Returns a string describing the state. This is primarily for developer usage.
            static const char8_t* GetStateString(State state);

            // GenerateBuiltFileInfoProgress following implements the EA::Patch::PatchBuilder::GenerateBuiltFileInfoEventCallback listening interface.
            void GenerateBuiltFileInfoProgress(const EA::Patch::DiffData& diffData, const EA::Patch::BuiltFileInfo& builtFileInfo, 
                                                const EA::Patch::Error& error, intptr_t userContext, uint64_t byteProgress);

            // State functions
            void BeginStateBegin();
            void CompleteStateBegin();

            void BeginStateDownloadingPatchDiffFiles();
            void CheckStateDownloadingPatchDiffFiles();
            void CompleteStateDownloadingPatchDiffFiles();

            void BeginStateBuildingDestinationFileInfo();
            void CompleteStateBuildingDestinationFileInfo();

            void BeginStateDownloadingDestinationFiles();
            void CheckStateDownloadingDestinationFiles();
            void CompleteStateDownloadingDestinationFiles();

            void BeginStateCopyingLocalFiles();
            void CompleteStateCopyingLocalFiles();

            void BeginStateRenamingDestinationFiles();
            void CompleteStateRenamingDestinationFiles();

            void BeginStateEnd();
            void CompleteStateEnd();

        private:
            volatile State                  _state;                     // See enum State.
            volatile bool                   _shutdown;                  // Indicates that we are shut down and should not execute any more.
            volatile bool                   _shutdownCancel;            // Indicates whether a shutdown should result in a cancellation (i.e. clean up all files)
            Services::Session::SessionWRef  _curSession;                // Passed to us in our constructor; valid for our entire lifetime.
            IDownloadSession*               _downloadSession;           // DownloadSession that downloads from _syncPackageUrl. Created by us upon our init, destroyed by us upon our shutdown.
            QString                         _rootPath;                  // File path to the directory we are writing to. e.g. "C:\SomeDir\SomeGame\"
            QString                         _productId;                 // The product id string of the app we are patching.
            QString                         _gameTitle;                 // The game title of the app we are patching.
            QString                         _logPrefix;                 // String used for log/debug traces.
            QString                         _syncPackageUrl;            // URL to the app SyncPackage .zip file, with the uncompressed image plus .eaPatchDiff files. We use this to do our differential patching.
            PackageFile*                    _syncPackageFile;           // PackageFile contents for the SyncPackage .zip file. All the files we want to download are in this.
            DiffFileId                      _nextDiffFileId;            // A counter which we will use to create unique identifiers for tracking each file
            JobList                         _jobList;                   // List of Jobs, one for each file we intend to build.
            JobSizeMetrics                  _jobSizeMetrics;                // Records current progress of data downloading and processing.
            DownloadRequestDataMap          _downloadRequestDataMap;    // Used for both downloading of Job::mDiffFileDownloadData, and Job::mFileDownloadDataList, depending on our state.
            #if defined(_DEBUG) || defined(EA_DEBUG)
            char                            _debugBuffer[4096];         // Used for debug printfs.
            #endif
    };

}//namespace Downloader

}//namespace Origin

#endif
