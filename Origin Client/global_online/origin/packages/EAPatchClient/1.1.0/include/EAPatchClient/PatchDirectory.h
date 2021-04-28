///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/PatchDirectory.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_PATCHDIRECTORY_H
#define EAPATCHCLIENT_PATCHDIRECTORY_H


#include <EAPatchClient/Config.h>
#include <EAPatchClient/PatchInfo.h>
#include <EAPatchClient/Locale.h>
#include <EAPatchClient/Error.h>
#include <EAPatchClient/Base.h>
#include <EAPatchClient/HTTP.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_futex.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace Patch
    {
        class PatchDirectoryRetriever;
        class TelemetryEventCallback;


        /// PatchDirectory
        ///
        /// A list of patches, like a directory listing. Usually a list of the currently available 
        /// patches for a given title.
        ///
        /// This class does not have Error state, as it is merely a container which doesn't 
        /// execute fallable actions.
        ///
        /// This class supports C++ copy semantics. So you can copy construct and assign instances
        /// of PatchDirectory with each other.
        ///
        /// As of this writing, a .eaPatchDir file represents a PatchDirectory listing. It is simply a 
        /// text file which has one URL for a .eaPatchInfo file per line. This PatchDirectory class
        /// is a container for the information in those .eaPatchInfo files, which were listed in the
        /// .eaPatchDir file.
        ///
        class EAPATCHCLIENT_API PatchDirectory
        {
        public:
            PatchDirectory();
           ~PatchDirectory();

            const PatchInfoArray& GetPatchInfoArray() const;
            PatchInfoArray& GetPatchInfoArray();

            void Reset();

            /// Adds a PatchInfo to the directory. Verifies that the given PatchInfo isn't already present.
            /// Returns false and doesn't add the PatchInfo if the patch was already present.
            bool AddPatchInfo(const PatchInfo& patchInfo);

            /// Removes a PatchInfo (identified by its patchId).
            /// Returns true if it was found (and thus removed).
            bool RemovePatchInfo(const char8_t* pPatchId);

        protected:
            PatchInfoArray mPatchInfoArray;     // All the available patches in this directory listing.
        };



        /// PatchDirectoryEventCallback
        ///
        /// PatchDirectoryRetriever events are sent to the registered PatchDirectoryEventCallback.
        /// This is useful for providing high level user feedback on the directory retrieval process.
        /// The userContext argument is an arbitrary value specified by the user upon registration.
        /// There is no need to respond to any of the callbacks, though you can call back into 
        /// the PatchDirectoryRetriever in a callback. However, you should not block in the callback, as it
        /// is being called while the PatchDirectoryRetriever is in the process of downloading.
        ///
        /// The EAPatchError callback occurs in the case of an error that prevents the PatchDirectoryRetriever
        /// from completing. You can respond to this error within the callback or wait until the 
        /// PatchDirectoryRetriever returns. 
        ///
        /// Some of the arguments are file paths. If you want to break down the file paths into
        /// Their components, the EAIO PathString functions will correctly do this for all platforms.
        ///
        /// The download-related callbacks can (and usually will) be called in an interleaved way. 
        /// For example, EAPatchBeginFileDownload will be called multiple times for multiple files before 
        /// EAPatchEndFileDownload is called for any of them. This is because files are downloaded
        /// in parallel, with some completeing before others while additional files start downloading.
        ///
        /// For a call to RetrievePatchDirectoryXXX, the first event received will always be EAPatchStart,  
        /// and the last event received will be EAPatchStop, which can be used to detect that a patch build 
        /// process has ended with the given AsyncStatus (which may indicate an error of some type).
        /// 
        class PatchDirectoryEventCallback
        {
        public:
            virtual void EAPatchStart              (PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext);
            virtual void EAPatchProgress           (PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, double progress); // values are in range of [0.0, 1.0].
            virtual void EAPatchNetworkAvailability(PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, bool available);
            virtual void EAPatchError              (PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, Error& error);
            virtual void EAPatchNewState           (PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, int newState); // newState is one of enum PatchDirectoryRetriever::State
            virtual void EAPatchBeginFileDownload  (PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, const char8_t* pFilePath, const char8_t* pFileURL);
            virtual void EAPatchEndFileDownload    (PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, const char8_t* pFilePath, const char8_t* pFileURL);
            virtual void EAPatchStop               (PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, AsyncStatus asyncStatus);

            virtual ~PatchDirectoryEventCallback(){}
        };


        /// PatchDirectoryRetriever
        ///
        /// The first thing an app does as part of the patching process is
        /// to retrieve a list of available PatchInfos from the patch server.
        /// This list is a PatchDirectory, and this class retrieves that 
        /// directory listing and fills in a PatchDirectory instance.
        /// Executing the PatchDirectoryRetriever does not amount to applying a
        /// patch; it is instead about getting a listing of patches when might
        /// subsequently get applied.
        ///
        /// Retrieval of a PatchDirectory consists of downloading a .eaPatchDir file,
        /// reading its list of .eaPatchInfo URLs, downloading each of the .eaPatchInfo
        /// files and then reading their contents into a PatchDirectory instance.
        ///
        class EAPATCHCLIENT_API PatchDirectoryRetriever : public ErrorBase
        {
        public:
            PatchDirectoryRetriever();
           ~PatchDirectoryRetriever();

            /// Gets/Sets the EA::Patch::Server to use. If SetServer is not called then
            /// the global server returned by EA::Patch::GetServer is used. The supplied Server
            /// must be valid for the lifetime of retrieval operations.
            Server* GetServer();
            void    SetServer(Server* pServer);

            /// Clears all information in preparation for a new round of usage.
            /// Any previous execution must have completed on its own or cancelled via Cancel.
            void Reset();

            /// Patch directory location.
            /// GetPatchDirectoryURL returns a non-NULL string, but that string will be empty 
            /// if SetPatchDirectoryURL was never called.
            /// Must be called before calling RetrievePatchDirectoryBlocking or RetrieveDirectoryAsync.
            /// Example usage:
            ///      SetPatchDirectoryURL("https://patch.ea.com:1234/FIFA/2014/PS3/PatchDirectory.eaPatchDir");
            const char8_t* GetPatchDirectoryURL() const;
            void           SetPatchDirectoryURL(const char8_t* pURL);

            /// Returns a copy of the retrieved PatchDirectory.
            /// The return value is true if the PatchDirectory is valid as a result of having 
            /// been successfully retrieved.
            /// You would call this function after successfully completing RetrievePatchDirectoryBlocking
            /// or RetrieveDirectoryAsync.
            bool GetPatchDirectory(PatchDirectory& patchDirectory);

            /// As with the rest of the EAPatchClient, errors are reported as they occur to 
            /// the handler set by RegisterUserErrorHandler. Only a single download can be 
            /// active at a time for an instance of PatchDirectoryRetriever, and false will 
            /// be returned (kEAPatchErrorIdAlreadyActive) if a retrieval is active.
            /// This is a blocking call which probably should be called in a thread other 
            /// than the main thread.
            ///
            /// The return value indicates whether the action completed successfully or with error.
            /// If the operation was cancelled via CancelRetrieval, the return value will be 
            /// true (as there was no error), HasError will return false, but GetAsyncStatus will
            /// return kAsyncStatusCancelled instead of kAsyncStatusCompleted. 
            /// If the return value is false, then errorResult contains the error information and 
            /// GetAsyncStatus will return kAsyncStatusAborted. To tell if the operation completed
            /// successfully, check for GetAsyncStatus == kAsyncStatusCompleted, as it will only 
            /// ever be kAsyncStatusCompleted if it completed without error.
            bool RetrievePatchDirectoryBlocking();

            /// This is an async version of RetrievePatchDirectoryBlocking, and it returns
            /// immediately, after the creation of a background task to do the retrieval.
            /// The return value indicates whether the async operation started successfully.
            /// You can use GetAsyncStatus to poll the state of this operation.
            bool RetrievePatchDirectoryAsync();

            /// Cancels an existing download if it's active. 
            /// If waitForCompletion is true then this function doesn't return until the 
            /// cancel operation is complete and all of its resources are released.
            /// Can be called while either RetrievePatchDirectoryBlocking or RetrievePatchDirectoryAsync
            /// is executing. If RetrievePatchDirectoryBlocking is executing then this causes it
            /// to return false. If RetrievePatchDirectoryAsync is executing then this causes the
            /// AsyncStatus to become kAsyncStatusCancelled. In both cases the operation is halted.
            /// A new call to RetrievePatchDirectory can be made at any time after calling CancelRetrieval.
            void CancelRetrieval(bool waitForCompletion);

            /// Gets the current status of RetrievePatchDirectoryAsync.
            /// Tells if RetrievePatchDirectoryAsync is busy downloading a PatchDirectory, for example.
            /// RetrievePatchDirectoryAsync is completed when the AsyncStatus is one of the 
            /// done states (completed, cancelled, aborted). 
            /// Error may come with any AsyncStatus state except kAsyncStatusNone
            /// and kAsyncStatusCompleted. Check this class' ErrorBase for the Error.
            AsyncStatus GetAsyncStatus() const;

            enum State
            {
                kStateNone,
                kStateBegin,
                kStateDownloadDirectory,        // Downloading the .eaPatchDir file.
                kStateProcessDirectory,         // Parsing the .eaPatchDir file.
                kStateDownloadDirectoryEntries, // Downloading zero or more .eaPatchInfo files.
                kStateEnd
            };

            /// Returns the current state of the build. You can also passively listen for 
            /// state changes via the PatchDirectoryEventCallback mechanism. You cannot set 
            /// the state; it can be set only internally during processing.
            State GetState();

            /// Returns a string describing the state. This is primarily for developer usage.
            static const char8_t* GetStateString(State state);

            /// Only one callback can be registered.
            void RegisterPatchDirectoryEventCallback(PatchDirectoryEventCallback* pCallback, intptr_t userContext);

            /// Only one callback can be registered.
            void RegisterTelemetryEventCallback(TelemetryEventCallback* pCallback, intptr_t userContext);

        protected:
            enum ThreadResult
            {
                kThreadSuccess =  0,
                kThreadError   = -1
            };

            struct Job
            {
                PatchDirectoryRetriever* mpRetriever;
                bool                     mbThreaded;
                volatile bool            mbCancelJob;
    
                Job(PatchDirectoryRetriever* pRetriever, bool bThreaded) : mpRetriever(pRetriever), mbThreaded(bThreaded), mbCancelJob(false){}
            };

            static intptr_t ThreadFunctionStatic(void* pContext);
            intptr_t        ThreadFunction();
            bool            StartThreadJob();
            void            EndJob();
            bool            RetrievePatchDirectoryBlocking(Job* pJob);
            void            SetState(State newState);
            bool            ShouldContinue();
            void            EnsureURLIsAbsolute(String& sURL);

            // PatchDirectoryEventCallback notification functions.
            void NotifyStart();
            void NotifyProgress(double progress);
            void NotifyNetworkAvailability(bool available);
            void NotifyError(Error& error);
            void NotifyNewState(int newState);
            void NotifyBeginFileDownload(const char8_t* pFilePath, const char8_t* pFileURL);
            void NotifyEndFileDownload(const char8_t* pFilePath, const char8_t* pFileURL);
            void NotifyStop();

            // Telemetry notification functions.
            void TelemetryNotifyDirectoryRetrievalBegin();
            void TelemetryNotifyDirectoryRetrievalEnd();

            // Hide these.
            PatchDirectoryRetriever(const PatchDirectoryRetriever&) : ErrorBase(){}
            void operator=(const PatchDirectoryRetriever&){}

        protected:
            Server*                         mpServer;                           /// 
            String                          mServerURLBase;                     /// e.g. http://patch.ea.com:80/
            String                          mPatchDirectoryURL;                 /// 
            PatchDirectory                  mPatchDirectory;                    /// 
            volatile State                  mState;                             /// 
            volatile AsyncStatus            mAsyncStatus;                       /// Indicates the current status of the retrieval. 
            EA::Thread::Thread              mThread;                            /// 
            mutable EA::Thread::Futex       mFutex;                             /// Guards access to mAsyncStatus and mPatchDirectory.
            Job*                            mpCurrentJob;                       ///
            HTTP                            mHTTP;                              /// HTTP client used for downloading.
            uint64_t                        mPatchDirDownloadVolume;            /// How many bytes have been downloaded as the .eaPatchDir file.
            uint64_t                        mPatchInfoDownloadVolume;           /// How many bytes have been downloaded as .eaPatchInfo files.
            PatchDirectoryEventCallback*    mpPatchDirectoryEventCallback;      /// 
            intptr_t                        mPatchDirectoryEventUserContext;    /// 
            TelemetryEventCallback*         mpTelemetryEventCallback;           ///
            intptr_t                        mTelemetryEventUserContext;         /// 
        };

    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard


