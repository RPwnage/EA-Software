///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/HTTP.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_HTTP_H
#define EAPATCHCLIENT_HTTP_H


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Base.h>
#include <EAPatchClient/Error.h>
#include <EAPatchClient/Server.h>
#include <EAPatchClient/Allocator.h>
#include <EASTL/list.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_futex.h>

// DirtySDK
#include <DirtySDK/proto/protohttpmanager.h>
#include <DirtySDK/dirtyvers.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace IO
    {
        class IStream;
    }

    namespace Patch
    {
        /// HTTP
        ///
        /// Implements HTTP downloads from an HTTP or HTTPS server.
        /// This class isn't a singleton; multiple instances of this class
        /// can exist and be active simultaneously. However, you cannot use a single 
        /// instance of this class to simultaneously do multiple downloads, for example
        /// by calling GetFilesBlocking twice from two threads at the same time on a 
        /// single instance of this class.
        /// While this class can be used for simple file download jobs, a larger
        /// aggregating job managing class may be warranted for downloading many 
        /// large files in parallel.
        ///
        class EAPATCHCLIENT_API HTTP : public ErrorBase
        {
        public:
            HTTP();
            virtual ~HTTP();

            /// GetServer/SetServer
            /// Gets/Sets the EA::Patch::Server to use. If SetServer is not called then
            /// the global server returned by EA::Patch::GetServer is used. The supplied 
            /// Server must be valid for the lifetime of retrieval operations.
            Server* GetServer();
            void    SetServer(Server* pServer);

            /// GetHttpManager/SetHttpManager
            HttpManagerRefT* GetHttpManager() const;
            void SetHttpManager(HttpManagerRefT* pHttpManager);

            /// GetClientLimits / SetClientLimits
            /// Gets/Sets the ClientLimits to use. If SetClientLimits is not called then
            /// the global defaults returned by EA::Patch::GetDefaultClientLimits is used
            ClientLimits& GetClientLimits();
            void          SetClientLimits(const ClientLimits& clientLimits);

            /// Describes a download request and result.
            /// This is used in batch downloading where the user of this HTTP class specifies a 
            /// number of files to download. 
            /// A range of bytes from the source file can be read. If the entire range cannot 
            /// be read then an error occurs.
            struct DownloadJob
            {
                String           mSourceURL;    /// Input. Full URL to the server file. 
                String           mDestPath;     /// Input. Alternative to mpStream.
                EA::IO::IStream* mpStream;      /// Input. Alternative to mDestPath.
                uint64_t         mRangeBegin;   /// Input. Beginning byte index from the file to read. 0 means the first byte.
                uint64_t         mRangeCount;   /// Input. Count of bytes to read. A rangeCount of UINT64_MAX means to get from rangeBegin to the end of the file. 0 means to read all file bytes, regardless of the value of mRangeBegin. 
                AsyncStatus      mStatus;       /// Output.
                Error            mError;        /// Output.

                DownloadJob() : mSourceURL(), mDestPath(), mpStream(NULL), mRangeBegin(0), mRangeCount(0), mStatus(kAsyncStatusNone), mError() {}
            };

            typedef eastl::vector<EA::Patch::HTTP::DownloadJob*, CoreAllocator> DownloadJobPtrArray;

            // Event handling
            enum Event
            {
                kEventPreDownload,              /// Called just before the DownloadJob is started. User can do things like open a file, allocate resources as necessary, etc. DownloadJob::mStatus will be kAsyncStatusNone and DownloadJob::mError should be none.
                kEventPostDownload,             /// Called just after a DownloadJob is ended (successfully or otherwise). The state is found in DownloadJob::mStatus and DownloadJob::mError.
                kEventWrite,                    /// Called during downloading when there is a write to do. In this case the pData (input), dataSize (input), and bWriteHandled (output) arguments are used. If the callback leaves bWriteHandled false then HTTP writes directly to the end of the job stream itself.
                kEventIdle,                     /// Called during downloading while waiting for data to arrive.
                kEventNetworkUnavailable,       /// The network needed for the job appears to now be unavailable.
                kEventNetworkAvailable,         /// The network needed for the job appears to now be available.
                kEventRetryableDownloadError,   /// An error occurred which can be re-tried (e.g. network down, download timeout, unexpected disconnection). This event may be followed by a kEventNetworkUnavailable event.
                kEventNonretryableDownloadError /// An error occurred which is not beneficial to be re-tried (e.g. authentication error). This event may be followed by a kEventPostDownload event (with error).
            };

            /// User-specified function type which is called during HTTP job processing.
            /// A return value of false indicates that the job should be set to be in error.
            typedef bool (*HandleEvent)(Event eventType, HTTP* pHTTP, DownloadJob* pDownloadJob, intptr_t handleEventContext,
                                         const void* pData, uint64_t dataSize, bool& bWriteHandled);

            /// Sets a user-specfied callback function to be henceforth used.
            /// The caller can use any useful pointer value for handleEventContext.
            void SetEventHandler(HandleEvent pHandleEvent, intptr_t handleEventContext);

            /// Clears all information in preparation for a new round of usage.
            /// Any previous execution must have completed on its own or cancelled via CancelRetrieval.
            void Reset();

            /// Returns true if the error refers to an error that occurred due to loss of network connectivity.
            /// If a download fails due to a network availability error, then presumably the user lost their 
            /// (Internet) network connection and the application needs to pause until it becomes available again.
            bool ErrorIsNetworkAvailability(EAPatchErrorId errorId);

            /// Gets a file from a URL and writes it to pDestFilePath.
            /// Otherwise acts the same as GetFileBlocking(..., pStream, ...).
            bool GetFileBlocking(const char8_t* pURL, const char8_t* pDestFilePath, uint64_t rangeBegin = 0, uint64_t rangeCount = UINT64_MAX);

            /// Blocks until the operation completes.
            ///
            /// The return value indicates whether the action completed successfully or with error.
            /// If the operation was cancelled via CancelRetrieval, the return value will be 
            /// true (as there was no error), HasError will return false, but GetAsyncStatus will
            /// return kAsyncStatusCancelled instead of kAsyncStatusCompleted. 
            /// If the return value is false, then errorResult contains the error information and 
            /// GetAsyncStatus will return kAsyncStatusAborted. To tell if the operation completed
            /// successfully, check for GetAsyncStatus == kAsyncStatusCompleted, as it will only 
            /// ever be kAsyncStatusCompleted if it completed without error. Note that 
            ///
            /// This function should be executed in a different thread than the application's
            /// UI thread, as otherwise it would block the UI.
            /// rangeBegin/rangeCount indicates the sub-range within the file that you want 
            /// to retrieve. A rangeCount of 0 means to read all file bytes, regardless of 
            /// the value of mRangeBegin. A rangeCount of UINT64_MAX means to get from rangeBegin 
            /// to the end of the file. The return value indicates if the job could be successfully completed.
            bool GetFileBlocking(const char8_t* pURL, EA::IO::IStream* pStream, uint64_t rangeBegin = 0, uint64_t rangeCount = UINT64_MAX);

            /// Downloads one or more files from the server specifie by SetServer.
            /// The user specifies the DownloadJob input fields, and this function downloads the
            /// files and writes the output result fields. 
            /// The number of jobs passed to this function can be arbitrarily high. It internally
            /// throttles how much gets processed at once.
            /// The DownloadJob pointers in the array must be valid throughout the entire call.
            /// The DownloadJob array is an array of pointers instead of array of objects so that 
            /// the user can subclass DownloadJob for their own purposes and pass pointers to that.
            /// See GetFileBlocking for a description of the return value.
            /// 
            /// Example usage:
            ///        DownloadJob downloadJob[2];
            ///        . . .
            ///        DownloadJob* downloadJobPtrArray[2] = { &downloadJob[0], &downloadJob[1] };
            ///        http.GetFilesBlocking(downloadJobPtrArray, EAArrayCount(downloadJobPtrArray));
            ///
            bool GetFilesBlocking(DownloadJob* pDownloadJobPtrArray[], size_t downloadJobArrayCount);

            /// Cancels an existing blocking or async download if it's active. 
            /// If waitForCompletion is true then this function doesn't return until the 
            /// cancel operation is complete and all of its resources are released.
            /// A new call to GetFileBlocking or GetFilesBlocking can be made at any time after 
            /// calling CancelRetrieval. Beware that due to the multithreaded nature of this library,
            /// if you call CancelRetrieval it's still slightly possible that the operation will complete 
            /// with a status other than kAsyncStatusCancelled.
            void CancelRetrieval(bool waitForCompletion);

            /// Gets the current status of GetFileBlocking / GetFilesBlocking.
            /// GetFilesBlocking is completed when the AsyncStatus is one of the 
            /// done states (completed, cancelled, aborted). 
            /// Error may come with any AsyncStatus state except kAsyncStatusNone
            /// and kAsyncStatusCompleted. Check this class' ErrorBase for the Error.
            AsyncStatus GetAsyncStatus() const;

            /// Async version of GetFilesBlocking.
            /// The pDownloadJobPtrArray must be valid for the entire duration of the async operation,
            /// and not just until this function returns. This is because results are written to 
            /// this array.
            bool GetFilesAsync(DownloadJob* pDownloadJobPtrArray[], size_t downloadJobArrayCount);


            /// DownloadStats
            struct DownloadStats
            {
                uint64_t mBytesDownloaded;      /// Bytes downloaded so far.
                uint64_t mBytesExpected;        /// Bytes to download. This can increase during a download job if mTotalFileCount > (mCompletedFileCount + mBusyFileCount)
                uint64_t mCompletedFileCount;   /// 
                uint64_t mActiveFileCount;      /// 
                uint64_t mFailureFileCount;     /// 
                uint64_t mTotalFileCount;       /// 

                DownloadStats() : mBytesDownloaded(0), mBytesExpected(0), mCompletedFileCount(0), 
                                  mActiveFileCount(0), mFailureFileCount(0), mTotalFileCount(0) { }
            };

            /// Returns the current download statistics.
            void GetDownloadStats(DownloadStats& stats);


            // To do: Implement a function which just checks for the existence of a file on the 
            // server instead of downloading. Possibly make the enabling of this be part of the 
            // DownloadJob class. One purpose of this is to allow us to check for the continued
            // existence of the .eaPatchImpl file for the patch, so that we know it hasn't been
            // removed from the server while we are downloading patch files and encounter errors.

            /// Returns the measured download rate in bytes per millisecond. See the DataRateMeasure class.
            /// This should not be considered a generic measurement of network bandwidth, though it should
            /// be a decent measure of HTTP read bandwidth, as it includes the overhead of issuing HTTP
            /// requests to the server and includes writing received HTTP data to disk.
            double GetNetworkReadDataRate() const;

        protected:
            #if defined(DIRTYSDK_VERSION) && (DIRTYSDK_VERSION >= 1301000600)
                #define DirtySDKDataSizeType int64_t
            #else
                #define DirtySDKDataSizeType uint32_t
            #endif
            friend int32_t ProtoHttpCustomHeaderCbT(ProtoHttpRefT*, char*, uint32_t, const char*, DirtySDKDataSizeType, void*);
            friend int32_t ProtoHttpReceiveHeaderCbT(ProtoHttpRefT*, const char* pHeader, uint32_t headerSize, void*);

            // For each user-specified DownloadJob, we have a DownloadJobInternal.
            struct DownloadJobInternal
            {
                HTTP*          mpHTTP;                  /// The HTTP class that owns this instance.
                DownloadJob*   mpDownloadJob;           /// User-provided download job. This is a pointer to the DownloadJob the user passed us.
                int32_t        mHttpMangerHandle;       /// DirtySDK HTTP client handle.
                uint64_t       mExpectedSize;           /// file size reported by server, or UINT64_MAX if not set yet (download hasn't started yet).
                uint64_t       mDownloadedSize;         /// The amount successfully downloaded thus far.
                uint32_t       mTryCount;               /// How many times we've tried, but failed.
              //Error          mLastError;              /// 

                DownloadJobInternal() : mpHTTP(NULL), mpDownloadJob(NULL), mHttpMangerHandle(-1), mExpectedSize(UINT64_MAX), mDownloadedSize(0), mTryCount(0) /*, mError()*/ { }
            };

            typedef eastl::list<DownloadJobInternal, CoreAllocator> DownloadJobInternalList;

        protected:
            bool ShouldContinue();
            void GetHTTPFailureInfo(DownloadJobInternal& dji, HttpManagerRefT* pHttpManager, int32_t httpManagerHandle, const char8_t* pURL, int32_t dirtySDKResult,
                                    ErrorClass& errorClass, SystemErrorId& systemErrorId, EAPatchErrorId& eaPatchErrorId, String& sErrorDescription);
            void GetJobDebugDescription(DownloadJobInternal& dji, HttpManagerRefT* pHttpManager, int32_t httpManagerHandle, const char8_t* pStringPrefix, const char8_t* pURL, String& sDescription);

            // Used by GetFilesAsync.
            enum ThreadResult
            {
                kThreadSuccess =  0,
                kThreadError   = -1
            };

            static intptr_t ThreadFunctionStatic(void* pThis);
            intptr_t        ThreadFunction();

        protected:
            mutable EA::Thread::Futex   mFutex;                         /// Guards access to mAsyncStatus and mPatchDirectory.
            Server*                     mpServer;                       /// The server used for download requests.
            HttpManagerRefT*            mpHttpManager;                  /// The DirtySDK HttpManager that's used. Can be set by the user.
            ClientLimits                mClientLimits;                  /// If these are not explicitly specified by the user then the global ClientLimits default is used.
            HandleEvent                 mHandleEventFunction;           /// 
            intptr_t                    mHandleEventContext;            /// User-defined context. Can be cast to a pointer.
            volatile AsyncStatus        mAsyncStatus;                   /// Indicates the current status of the retrieval. 
            bool                        mPaused;                        /// If true then we are paused until mTimeOfNextTrySeconds. Usually this is because we have had an Internet disconnection.
            uint64_t                    mTimeOfNextTrySeconds;          /// Specifies the absolute time of the next retry after download failure (usually due to a lost Internet connection). 
            uint64_t                    mTimeNetworkUnavailable;        /// The GetTimeSeconds time that the network was first noticed to be appear unavailable (offline). 
            uint8_t                     mBuffer[2048];                  /// 
            DataRateMeasure             mNetworkReadDataRateMeasure;    /// Measures the speed of downloading, which is useful for metrics.
            EA::Thread::Thread          mThread;                        /// Thread for async patch builds triggered by GetFilesAsync.
            HTTP::DownloadJob**         mpAsyncDownloadJobPtrArray;     /// Used to pass data to the thread function.
            size_t                      mAsyncDownloadJobArrayCount;    /// Used to pass data to the thread function.
            DownloadJobInternalList     mDJIWaitingList;                /// List of DownloadJobs yet to start. Started jobs move to the djiActiveList.
            DownloadJobInternalList     mDJICompletedList;              /// List of completed jobs. When all jobs are complete, this will have all jobs.
            DownloadJobInternalList     mDJIActiveList;                 /// List of actively downloading jobs. Completed jobs move to .
            DownloadJobInternalList     mDJIFailureList;                /// List of permanently failed jobs. Jobs that failed even after retries.
            DownloadStats               mDownloadStats;

		protected:
			HTTP& operator=(const HTTP&);	// Unimplemented - to fix warnings - assigment is not necessarily wrong for this object but needs to be implemented manually.
			HTTP(const HTTP&);				// Unimplemented - to fix warnings - copy construct is not necessarily wrong for this object but needs to be implemented manually.
        };

    } // namespace Patch

} // namespace EA





///////////////////////////////////////////////////////////////////////////////
// inlines
///////////////////////////////////////////////////////////////////////////////

namespace EA
{
    namespace Patch
    {
        inline Server* HTTP::GetServer()
        {
            return mpServer;
        }


        inline void HTTP::SetServer(Server* pServer)
        {
            mpServer = pServer;
        }


        inline HttpManagerRefT* HTTP::GetHttpManager() const
        {
            return mpHttpManager;
        }

        inline void HTTP::SetHttpManager(HttpManagerRefT* pHttpManager)
        {
            mpHttpManager = pHttpManager;
        }


        inline ClientLimits& HTTP::GetClientLimits()
        {
            return mClientLimits;
        }


        inline void HTTP::SetClientLimits(const ClientLimits& clientLimits)
        {
            mClientLimits = clientLimits;
        }


        inline void HTTP::SetEventHandler(HandleEvent pHandleEvent, intptr_t handleEventContext)
        {
            mHandleEventFunction = pHandleEvent;
            mHandleEventContext  = handleEventContext;
        }


        inline AsyncStatus HTTP::GetAsyncStatus() const
        {
            return mAsyncStatus;
        }


        inline double HTTP::GetNetworkReadDataRate() const
        {
            return mNetworkReadDataRateMeasure.GetDataRate();
        }

    } // namespace Patch

} // namespace EA




#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 




