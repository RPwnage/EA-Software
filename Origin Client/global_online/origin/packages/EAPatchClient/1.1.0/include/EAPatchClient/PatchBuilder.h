///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/PatchBuilder.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_PATCHBUILDER_H
#define EAPATCHCLIENT_PATCHBUILDER_H


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Base.h>
#include <EAPatchClient/Error.h>
#include <EAPatchClient/HTTP.h>
#include <EAPatchClient/PatchInfo.h>
#include <EAPatchClient/PatchImpl.h>
#include <EAPatchClient/internal/PatchBuilderInternal.h>
#include <eathread/eathread_thread.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace Patch
    {
        // Forward declarations
        class PatchBuilder;
        class TelemetryEventCallback;


        /// PatchBuilderEventCallback
        ///
        /// PatchBuilder events are sent to the registered PatchBuilderEventCallback.
        /// This is useful for providing high level user feedback on the patching process.
        /// The userContext argument is an arbitrary value specified by the user upon registration.
        /// There is no need to respond to any of the callbacks, though you can call back into 
        /// the PatchBuilder in a callback. However, you should not block in the callback, as it
        /// is being called while the PatchBuilder is in the process of building the patch.
        ///
        /// The EAPatchError callback occurs in the case of an error that prevents the PatchBuilder
        /// from completing. You can respond to this error within the callback or wait until the 
        /// PatchBuilder returns. 
        ///
        /// Many of the arguments are file paths. If you want to break down the file paths into
        /// Their components, the EAIO PathString functions will correctly do this for all platforms.
        /// You can get the base directory for the paths with the sPatchDestinationDirPath returned
        /// by the GetDirPaths function.
        ///
        /// The download-related callbacks can (and usually will) be called in an interleaved way. 
        /// For example, EAPatchBeginFileDownload will be called multiple times for multiple files before 
        /// EAPatchEndFileDownload is called for any of them. This is because files are downloaded
        /// in parallel, with some completeing before others while additional files start downloading.
        ///
        /// For a call to BuildPatchXXX, the first event received will always be EAPatchStart, and the 
        /// last event received will be EAPatchStop, which can be used to detect that a directory download
        /// process has ended with the given AsyncStatus (which may indicate an error of some type).
        /// 
        class PatchBuilderEventCallback
        {
        public:
            virtual void EAPatchStart                (PatchBuilder* pPatchBuilder, intptr_t userContext);
            virtual void EAPatchProgress             (PatchBuilder* pPatchBuilder, intptr_t userContext, double patchProgress); // patchProgress is in the range of [0.0, 1.0].
            virtual void EAPatchRetryableNetworkError(PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pURL);
            virtual void EAPatchNetworkAvailability  (PatchBuilder* pPatchBuilder, intptr_t userContext, bool available);
            virtual void EAPatchError                (PatchBuilder* pPatchBuilder, intptr_t userContext, Error& error);
            virtual void EAPatchNewState             (PatchBuilder* pPatchBuilder, intptr_t userContext, int newState); // newState is one of enum PatchBuilder::State
            virtual void EAPatchBeginFileDownload    (PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pFilePath, const char8_t* pFileURL);
            virtual void EAPatchEndFileDownload      (PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pFilePath, const char8_t* pFileURL);
            virtual void EAPatchRenameFile           (PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pPrevFilePath, const char8_t* pNewFilePath);
            virtual void EAPatchDeleteFile           (PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pFilePath);
            virtual void EAPatchRenameDirectory      (PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pPrevDirPath, const char8_t* pNewDirPath);
            virtual void EAPatchDeleteDirectory      (PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pDirPath);
            virtual void EAPatchStop                 (PatchBuilder* pPatchBuilder, intptr_t userContext, AsyncStatus asyncStatus);

            virtual ~PatchBuilderEventCallback(){}
       };



        /// PatchBuilder
        ///
        /// Builds a directory of files from the information in a PatchImpl.
        /// This class does the diffing of local files against remote patch files,
        /// downloading of the minimal amount of remote file data, and writing the 
        /// destination files. This class also handles deleting local files as 
        /// necessary, since patching a directory tree may include deleting files 
        /// the same as it includes adding and modifying files.
        ///
        /// This class doesn't handle downloading patch directories (lists of available
        /// patches) and their associated .eaPatchInfo files. It doesn't present user
        /// interface.
        ///
        class EAPATCHCLIENT_API PatchBuilder : public ErrorBase
        {
        public:
            PatchBuilder();
            virtual ~PatchBuilder();

            /// Gets.Sets the EA::Patch::Server to use. If SetServer is not called then
            /// the global server returned by EA::Patch::GetServer is used. The supplied Server
            /// must be valid for the lifetime of retrieval operations.
            void SetServer(Server* pServer);

            /// Gets/Sets the ClientLimits to use. If SetClientLimits is not called then
            /// the global defaults returned by EA::Patch::GetDefaultClientLimits is used
            ClientLimits& GetClientLimits();
            void          SetClientLimits(const ClientLimits& clientLimits);

            /// Sets the Locale to use. By default the global local returned by GetDefaultLocale()
            /// is used. This function allows you to override that with an alternative locale.
            /// The specified locale can be a wildcard locale, such as en_* to indicate english
            /// in any country, or simply *_* (kLocaleAny) to indicate to use any available locale data.
            void SetLocale(const char8_t* pLocale);

            /// pPatchDestinationDirPath is the directory where the patch is written. The PatchInfo has 
            /// a mPatchBaseDirectory member which is a relative directory that's appended to this.
            /// pPatchLocalDirPath is the directory where the pre-patch image exists. If this is an in-place 
            /// patch then pPatchLocalDirPath should be equal to pPatchDestinationDirPath.
            /// If either string parameter is NULL then it is not used and the default (empty) is used.
            /// If bInPlacePatch is true then the patch destination dir and local dir should be equal.
            /// This function must be called before calling BuildPatchXXX.
            void SetDirPaths(const char8_t* pPatchDestinationDirPath, const char8_t* pPatchLocalDirPath, bool bDeprecated = false);
            void GetDirPaths(String& sPatchDestinationDirPath, String& sPatchLocalDirPath, bool& bInPlacePatch);

            /// Sets the patch to be installed with BuildPatchXXX.
            /// This function must be called before calling BuildPatchXXX.
            void SetPatchInfo(const PatchInfo& patchInfo);

            /// Allows you identify a patch by just the minimum required fields.
            /// This is an alternative to the other SetPatchInfo function, and is useful for the
            /// case that you aren't downloading a .eaPatchInfo file but rather are directly
            /// executing a patch at a known URL for the .eaPatchImpl file.
            /// The arguments to this function are the minimal required to execute a patch, though
            /// some use cases may require additional fields to be set (e.g. PatchInfo::mPatchBaseDirectory),
            /// in which case you can manually set them through GetPatchInfo before building the patch 
            /// or call SetPatchInfo(const PatchInfo&). 
            void SetPatchInfo(const char8_t* pPatchId, const char8_t* pPatchImplURL);

            /// Returns a reference to the PatchInfo set by SetPatchInfo.
            /// Modifications to the returned PatchInfo must be done before a call to BuildPatchXXX.
            PatchInfo& GetPatchInfo();

            /// Clears all state associated with this class and frees any allocated memory.
            void Reset();

            /// Clears information in preparation for stopping, cancelling, or restarting the existing session.
            /// Any previous execution must have completed on its own or cancelled via Cancel.
            void ResetSession();

            /// Main entrypoint for building (downloading, installing) a patch.
            /// SetDirPaths and SetPatchInfo must have been called before calling this.
            ///
            /// The return value indicates whether the action completed successfully or with error.
            /// If the operation was cancelled via CancelRetrieval, the return value will be 
            /// true (as there was no error), HasError will return false, but GetAsyncStatus will
            /// return kAsyncStatusCancelled instead of kAsyncStatusCompleted. 
            /// If the return value is false, then errorResult contains the error information and 
            /// GetAsyncStatus will return kAsyncStatusAborted. To tell if the operation completed
            /// successfully, check for GetAsyncStatus == kAsyncStatusCompleted, as it will only 
            /// ever be kAsyncStatusCompleted if it completed without error. 
            bool BuildPatchBlocking();

            /// This is an async version of BuildPatchBlocking, and it returns immediately, 
            /// after the creation of a background task which begins executing BuildPatchBlocking.
            /// The return value indicates whether the async operation started successfully.
            /// You can use GetAsyncStatus and GetState to poll the state of this operation.
            /// SetDirPaths and SetPatchInfo must have been called before calling this.
            bool BuildPatchAsync();

            /// Halts an existing patch build operation if it's active. 
            /// If waitForCompletion is true then this function doesn't return until the 
            /// patching has stopped and all of the resources are released.
            /// This function does not remove the partially installed patch from the system
            /// but rather leaves everything as it is. If you want to remove the partially
            /// installed patch as well, then call the Cancel function.
            /// The patch can be resumed at a later time with another call to BuildPatchXXX,
            /// or the patch can be cancelled at a later time with a call to Cancel.
            /// Sets bPatchStopped to true if a patch was being built which could be stopped. Note that 
            /// it's possible that at the time of the Stop call BuildPatchXXX may be executing, but 
            /// at that same instant it completes and results in Stop setting bPatchStopped to false.
            /// The return value is the PatchBuilder class error state upon return of Stop.
            /// The difference between this and Cancel is that Cancel rolls back the patch.
            bool Stop(bool waitForCompletion, bool& bPatchStopped);

            /// This stops BuildPatchBlocking/BuildPatchAsync if they are executing, and rolls
            /// back the patch directory to its pre-patch state. The difference between this and
            /// Stop is that Stop doesn't roll back the patch.
            bool CancelPatchBlocking();

            /// Stops an existing patch build operation if it's active, and cancels the patch
            /// application process by removing the partially installed patch files and renaming
            /// any renamed files and directories to their original names. 
            /// If waitForCompletion is true then this function doesn't return until the 
            /// cancel operation is complete and all of its resources are released.
            /// Sets bPatchStopped to true if a patch was being built which could be cancelled. Note that 
            /// it's possible that at the time of the Cancel call BuildPatchXXX may be executing, but 
            /// at that same instant it completes and results in Cancel setting bPatchStopped to false.
            /// The return value is the PatchBuilder class error state upon return of Cancel.
            /// The difference between this and Stop is that Stop doesn't roll back the patch.
            bool Cancel(bool waitForCompletion, bool& bPatchCancelled);

            /// Gets the current status of BuildPatchBlocking.
            /// BuildPatchBlocking is completed when the AsyncStatus is one of the 
            /// done states (completed, cancelled, aborted). 
            /// Error may come with any AsyncStatus state except kAsyncStatusNone
            /// and kAsyncStatusCompleted. Check this class' ErrorBase for the Error.
            AsyncStatus GetAsyncStatus() const;

            /// State defines the current step of the Patch build or cancellation.
            /// When building a patch, the state progresses forward from kStateBegin to
            /// kStateEnd. When cancelling a patch, the state progresses backwards from
            /// the state at cancellation back to kStateNone.
            enum State
            {
                kStateNone,                             // 
                kStateBegin,                            // 
                kStateWritingPatchInfoFile,             // X -> Patch2.eaPatchInfo
                kStateDownloadingPatchImplFile,         // X -> Patch2.eaPatchImpl
                kStateProcessingPatchImplFile,          // 
                kStateDownloadingPatchDiffFiles,        // X -> Audio.big.eaPatchDiff
                kStateBuildingDestinationFileInfo,      // X -> Audio.big.eaPatchBFI
                kStateBuildingDestinationFiles,         // X -> Audio.big.eaPatchTemp
                kStateRenamingOriginalFiles,            // Audio.big -> Audio.big.eaPatchOld
                kStateRenamingDestinationFiles,         // Audio.big.eaPatchTemp -> Audio.big
                kStateRemovingIntermediateFiles,        // Patch2.eaPatchDiff -> X
                kStateRemovingOriginalFiles,            // Audio.big.eaPatchOld -> X
                kStateWritingPatchInstalledFile,        // Patch2.eaPatchImpl -> Patch2.eaPatchInstalled
                kStateFinalizing,                       // Patch2.eaPatchState -> X, mAsynState = ___
                kStateEnd                               // 
            };

            /// Returns the current state of the build. You can also passively listen for 
            /// state changes via the PatchBuilderEventCallback mechanism. You cannot set 
            /// the state; it can be set only internally during processing.
            State GetState();

            /// Returns a string describing the state. This is primarily for developer usage.
            static const char8_t* GetStateString(State state);

            /// Indicates whether the patch is being built or being cancelled.
            enum BuildDirection
            {
                kBDNone,
                kBDBuilding,
                kBDCancelling
            };

            /// Indicates whether we are building a patch, cancelling (rolling back) a patch, or doing neither.
            BuildDirection GetBuildDirection() const;

            /// Indicates whether the patch is being built for the first time or is a resume of a previously started (but not completed) patch.
            enum PatchCourse
            {
                kPCNone,        /// 
                kPCNew,         /// Indicates that the patching operation is new and not a resume of a previously interrupted patch.
                kPCResume       /// Indicates that the patching operation is a resume of a previously iterrupted patch process.
            };

            /// Currently applies only to the case of building a patch and not cancelling a patch.
            PatchCourse GetPatchCourse() const;

            /// Returns a value in the range of [0.0, 1.0] indicating completeness of the patch build.
            /// During cancellation, the completeness moves backwards from its value to 0.0, though for 
            /// representing this visually you can always make it look like it's moving towards 1.0 (100%).
            double GetCompletionValue();

            /// Only one callback can be registered.
            void RegisterPatchBuilderEventCallback(PatchBuilderEventCallback* pCallback, intptr_t userContext);

            /// If enabled then patch builder events are echoed to debug output for debug builds or 
            /// if EAPATCH_DEBUG is enabled. These are the same events that are also sent to the user 
            /// PatchBuilderEventCallback if enabled. Disabled by default, in order to cut down on noise.
            void TracePatchBuilderEvents(bool enable);

            /// Only one callback can be registered.
            void RegisterTelemetryEventCallback(TelemetryEventCallback* pCallback, intptr_t userContext);

            /// If enabled then telemetry events are echoed to debug output for debug builds or 
            /// if EAPATCH_DEBUG is enabled. These are the same events that are also sent to the user 
            /// TelemetryEventCallback if enabled. Disabled by default, in order to cut down on noise.
            void TraceTelemetryEvents(bool enable);

            /// Returns the measured disk write speed in bytes per millisecond. See the DataRateMeasure class.
            /// This should not be considered a generic hard drive benchmark, as it applies only to certain
            /// operations we do, and it encompasses both reading and writing. Most importantly, it doesn't
            /// see the fact that disk caching can make disk operations seem faster than they are.
            double GetDiskDataRate() const;

            // Used by GenerateBuiltFileInfo.
            // This is an event listening callback for just GenerateBuiltFileInfo. It exists by itself 
            // becuase GenerateBuiltFileInfo is a standalone (static) public function that users can use
            // outside of PatchBuilder.
            class GenerateBuiltFileInfoEventCallback
            {
            public:
                // Report on the progress of the GenerateBuiltFileInfo call.
                // byteProgress is the number of bytes from builtFileInfo.mLocalFilePath that have been 
                // processed out of builtFileInfo.mLocalFileSize.
                virtual void GenerateBuiltFileInfoProgress(const DiffData& diffData, const BuiltFileInfo& builtFileInfo, 
                                                           const Error& error, intptr_t userContext, uint64_t byteProgress) = 0;
                virtual ~GenerateBuiltFileInfoEventCallback(){}
            };

            /// Core function for converting files plus DiffData to BuiltFileInfo.
            static bool GenerateBuiltFileInfo(const DiffData& diffData, BuiltFileInfo& builtFileInfo, Error& error, GenerateBuiltFileInfoEventCallback* pEventCallback = NULL, intptr_t userContext = 0);

        protected:
            friend class Maker;

            // The main functions
            bool BeginPatch();
            bool WritePatchInfoFile();
            bool DownloadPatchImplFile();
            bool ProcessPatchImplFile();
            bool DownloadPatchDiffFiles();
            bool BuildDestinationFileInfo();
            bool BuildDestinationFiles();
            bool RenameOriginalFiles();
            bool RenameDestinationfiles();
            bool RemoveIntermediateFiles();
            bool RemoveOriginalFiles();
            bool WritePatchInstalledFile();
            bool Finalize();
            bool EndPatch();

            // Undo functions
            bool UndoRemoveIntermediateFiles();
            bool UndoRenameDestinationFiles();
            bool UndoRenameOriginalFiles();
            bool UndoBuildDestinationFiles();
            bool UndoBuildDestinationFileInfo();
            bool UndoDownloadPatchDiffFiles();
            bool UndoProcessPatchImplFile();
            bool UndoDownloadPatchImplFile();
            bool UndoWritePatchInfoFile();
            bool UndoStateBegin();

            // Helper functions
            bool  ShouldContinue();
            bool  SetupDirectoryPaths();
            bool  GenerateBuiltFileInfo(const DiffData& diffData, BuiltFileInfo& builtFileInfo, GenerateBuiltFileInfoEventCallback* pEventCallback = NULL, intptr_t userContext = 0);
            bool  SetState(State newState);
            bool  ReadPatchStateFile(State& state, bool allowFailure);
            bool  WritePatchStateFile(State state);
            bool  FileRename(const char8_t* pPathSource, const char8_t* pPathDestination, bool bOverwriteIfPresent = true);
            bool  FileRemove(const char8_t* pPath);
            bool  DirectoryRename(const char8_t* pPathSource, const char8_t* pPathDestination);
            bool  DirectoryRemove(const char8_t* pPathSource, bool bAllowRecursiveRemoval);
            void  EnsureURLIsAbsolute(String& sURL);

            // PatchBuilderEventCallback notification functions.
            void NotifyStart();
            void NotifyProgress();
            void NotifyRetryableNetworkError(const char8_t* pURL);
            void NotifyNetworkAvailability(bool available);
            void NotifyError(Error& error);
            void NotifyNewState(int newState);
            void NotifyBeginFileDownload(const char8_t* pFilePath, const char8_t* pFileURL);
            void NotifyEndFileDownload(const char8_t* pFilePath, const char8_t* pFileURL);
            void NotifyRenameFile(const char8_t* pPrevFilePath, const char8_t* pNewFilePath);
            void NotifyDeleteFile(const char8_t* pFilePath);
            void NotifyRenameDirectory(const char8_t* pPrevDirPath, const char8_t* pNewDirPath);
            void NotifyDeleteDirectory(const char8_t* pDirPath);
            void NotifyStop();

            // Telemetry notification functions.
            void TelemetryNotifyPatchBuildBegin();
            void TelemetryNotifyPatchBuildEnd();
            void TelemetryNotifyPatchCancelBegin();
            void TelemetryNotifyPatchCancelEnd();

            // Used by BuildPatchAsync.
            static intptr_t ThreadFunctionStatic(void* pContext);
            intptr_t        ThreadFunction();

            // Hide these.
            PatchBuilder(const PatchBuilder&) : ErrorBase(){}
            void operator=(const PatchBuilder&){}

        public: // Functions that are public but only due to complications if made private.
            /// HTTP callback function for DownloadPatchDiffFiles.
            bool HandlePatchDiffHTTPEvent(HTTP::Event eventType, HTTP* pHTTP, HTTP::DownloadJob* pDownloadJob);

            /// HTTP callback function for BuildDestinationFiles.
            bool HandleBuildFileHTTPEvent(HTTP::Event eventType, HTTP* pHTTP, HTTP::DownloadJob* pDownloadJob,
                                         const void* pData, uint64_t dataSize, bool& bWriteHandled);

        protected:
            enum ThreadResult
            {
                kThreadSuccess =  0,
                kThreadError   = -1
            };

            /// DiffFileDownloadJob
            /// Describes an individual .eaPatchDiff download job.
            struct DiffFileDownloadJob : public EA::Patch::HTTP::DownloadJob
            {
                PatchEntry*  mpPatchEntry;            /// The associated PatchEntry from mPatchImpl.mPatchEntryArray.
                String       msDestFullPath;          /// .eaPatchDiff path
                String       msDestFullPathTemp;      /// .eaPatchDiff.eaPatchTemp path
                File         mFileStreamTemp;         /// .eaPatchDiff.eaPatchTemp file object. Will be renamed to .eaPatchDiff when complete.

                DiffFileDownloadJob();
            };

            typedef eastl::list<DiffFileDownloadJob, CoreAllocator> DiffFileDownloadJobList;

            /// FileSpanDownloadJob
            /// Describes an individual file segment download job.
            struct FileSpanDownloadJob : public EA::Patch::HTTP::DownloadJob
            {
                PatchEntry*                 mpPatchEntry;         /// Which PatchEntry this refers to.
                BuiltFileInfo*              mpBuiltFileInfo;      /// Which BuiltFileInfo this download refers to.
                BuiltFileSpanList::iterator mBuiltFileSpanIt;     /// Which span with the file this download refers to.

                FileSpanDownloadJob();
            };

            typedef eastl::list<FileSpanDownloadJob, CoreAllocator> FileSpanDownloadJobList;

        protected:
            mutable EA::Thread::Futex   mFutex;                         /// Guards access to mAsyncStatus, mState, etc.
            volatile BuildDirection     mBuildDirection;                /// Indicates whether we are building a patch or cancelling (rolling back) a patch.
            volatile PatchCourse        mPatchCourse;                   /// 
            volatile State              mState;                         /// 
            volatile AsyncStatus        mAsyncStatus;                   /// Indicates the current status of the patch build. 
            volatile bool               mbBuildPatchBlockingActive;     /// Indicates if BuildPatchBlocking is executing.
            EA::Thread::Thread          mThread;                        /// Thread for async patch builds triggered by BuildPatchAsync.
            String                      mServerURLBase;                 /// e.g. http://patch.ea.com:80/
            String                      mPatchBaseDirectoryDest;        /// The directory where the patch is written. The PatchInfo has a mPatchBaseDirectory member which is a relative directory that's appended to this.
            String                      mPatchBaseDirectoryLocal;       /// The directory where the pre-patch image exists. If mbInPlacePatch is true then this will be equal to mPatchBaseDirectoryDest.
            PatchInfo                   mPatchInfo;                     /// 
            String                      mPatchInfoFileName;             /// e.g. Patch1.eaPathInfo
            String                      mPatchInfoFilePath;             /// e.g. /SomeApp/Data/Patch1.eaPathInfo
            PatchImpl                   mPatchImpl;                     /// 
            String                      mPatchImplURL;                  /// e.g. http://patch.ea.com/SomeApp/Patch1/Patch%201.eaPathImpl
            String                      mPatchImplFilePath;             /// e.g. /SomeApp/Data/Patch1.eaPathImpl -- The file path for the .eaPatchImpl file in our destination directory.
            String                      mPatchStateFilePath;            /// e.g. /SomeApp/Data/Patch1.eaPatchState
            Server*                     mpServer;                       /// 
            ClientLimits                mClientLimits;                  /// If these are not explicitly specified by the user then the global ClientLimits default is used.
            String                      msLocale;                       /// 
            uint64_t                    mStartTimeMs;                   /// Time of the start of the patch build or patch cancellation. See EA::Patch::GetTimeSeconds.
            uint64_t                    mEndTimeMs;                     /// Time of the end of the patch build or patch cancellation. See EA::Patch::GetTimeSeconds.
            DiffFileDownloadJobList     mDiffFileDownloadList;          ///
            eastl_size_t                mDiffFileDownloadCount;         /// How many from mDiffFileDownloadList have been downloaded already. Used for metrics output.
            BuiltFileInfoArray          mBuiltFileInfoArray;            /// This container is parallel to mPatchImpl.mPatchEntryArray, entry for entry.
            FileSpanDownloadJobList     mFileSpanDownloadJobList;       /// 
            uint64_t                    mPatchImplDownloadVolume;       /// How many bytes have been downloaded as the .eaPatchImpl file.
            uint64_t                    mDiffFileDownloadVolume;        /// How many bytes have been downloaded as .eaPatchDiff files.
            uint64_t                    mFileDownloadVolume;            /// How much of patch files have thus far come from downloaded bytes. 
            uint64_t                    mFileDownloadVolumeFinal;       /// How much of patch files will be coming from downloaded bytes. 
            uint64_t                    mFileCopyVolume;                /// How much of patch files have thus far come from copied file bytes.
            uint64_t                    mFileCopyVolumeFinal;           /// How much of patch files will be coming from copied file bytes.
            HTTP                        mHTTP;                          /// 
            bool                        mbInPlacePatch;                 /// If true then the patch is replacing a directory as opposed to creating a new directory. In the former case the patch replaces an existing installation; in the latter it adds a new installation and leaves the original alone.
            PatchBuilderEventCallback*  mpPatchBuilderEventCallback;    /// 
            intptr_t                    mPatchBuilderEventUserContext;  /// 
            bool                        mbPatchBuilderEventTraceEnabled;/// If true then we echo builder events to debug output.
            TelemetryEventCallback*     mpTelemetryEventCallback;       ///
            intptr_t                    mTelemetryEventUserContext;     /// 
            bool                        mbTelemetryEventTraceEnabled;   /// If true then we echo telemetry events to debug output.
            DataRateMeasure             mDiskWriteDataRateMeasure;      /// Measures the speed of downloading, which is useful for metrics.
            double                      mProgressValue;                 /// Value in the range of [0.0, 1.0] indicating where between kStateNone and kStateEnd the patching (or cancellation) process currently is.
            double                      mProgressValueLastNotify;       /// The value of mProgressValue the last time the user callback was notified. Used to prevent spamming the user with notifications and to prevent going backwards in completion % due to calculation errors/roundoffs.
        };

    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 




