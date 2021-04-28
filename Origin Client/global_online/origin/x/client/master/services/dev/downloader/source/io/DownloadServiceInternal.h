#ifndef DOWNLOADSVC_INTERNAL_H
#define DOWNLOADSVC_INTERNAL_H

#include "services/downloader/DownloadService.h"
#include "services/downloader/MemBuffer.h"
#include "services/downloader/MovingAverage.h"
#include "services/session/AbstractSession.h"
#include "DownloadPriorityQueue.h"
#include "DownloadRateLimiter.h"

#include <QFile>
#include <QTimer>
#include <QRunnable>
#include <QMutex>
#include <QThread>
#include <QReadWriteLock>

//#define DEBUG_DOWNLOAD_SERVICE 1
//#define DEBUG_DOWNLOAD_MULTI_CONNECTION
//#define DEBUG_DOWNLOAD_MULTI_WORKER
//#define DEBUG_DOWNLOAD_MEMORY_USE

#define IMPROVED_PIPELINING // for single request workers, don't wait for decompression do a deep copy and start next chunk immediately


namespace Origin
{
namespace Downloader
{
    class DownloadRequest;
    class DownloadWorker;
    class ChannelInterface;
    class SyncRequestHelper;

    class DownloadSession : public IDownloadSession
    {
        friend class DownloadRateLimiter;

        Q_OBJECT
    public:
        DownloadSession(const QString& url, const QString& id, bool isPhysicalMedia, qint64 idealRequestSize, qint16 maxWorkers, bool safeMode, Services::Session::SessionWRef session );
        virtual ~DownloadSession();

        virtual QString const& getUrl() const;
        virtual QString const& getHostName() const;
        QString getPrimaryHostIP() const;
        virtual bool isReady() const;
		virtual bool isRunning() const;

        // async calls
        Q_INVOKABLE virtual void initialize();

        Q_INVOKABLE virtual void submitRequest(RequestId reqId, QSet<int> priorityGroupIds, bool priorityGroupEnabled);
        Q_INVOKABLE virtual void submitRequests(QVector<RequestId> reqIds, int priorityGroupId, bool priorityGroupEnabled);

        Q_INVOKABLE virtual void priorityMoveToTop(int priorityGroupId);
        Q_INVOKABLE virtual void prioritySetQueueOrder(QVector<int> priorityGroupIds);
        Q_INVOKABLE virtual void prioritySetGroupEnableState(int priorityGroupId, bool enabled);

        // sync calls
        virtual bool requestAndWait(RequestId reqId, int timeout = 0);
        virtual void cancelSyncRequests();

        virtual QVector<int> priorityGetQueueOrder();

        // create/destroy requests
        virtual RequestId createRequest(qint64 byteStart, qint64 byteEnd);
        virtual IDownloadRequest* getRequestById(RequestId reqId);
        Q_INVOKABLE virtual void closeRequest(RequestId reqId);

        Q_INVOKABLE virtual void shutdown();

        virtual const QString& id() const;
        virtual const QUuid& uuid() const;

        virtual void sendHostIPTelemetry(int errorType, int errorCode);

        virtual qint64 getIdealRequestSize() const;
        virtual qint64 getMinRequestSize() const;
        virtual qint64 getRequestSize() const;
        virtual qint64 getContentLength();
		bool isPhysicalMediaSession() const { return mPhysicalMedia; }
		bool isFileOverride() const { return mFileOverride; }
        bool isSafeMode() const { return mSafeMode; }

        void priorityQueuePostponeActiveWorkers();
        QVector<int> priorityQueueGetUpdatedOrder();
        void priorityQueueSendUpdatedOrder();

        void workerChunkCompleted(DownloadWorker* worker);

        void workerChunkAvailable(RequestId reqId);
        void limiterChunkAvailable(RequestId reqId);
        void workerError(QVector<DownloadRequest*> requests, qint32 errorType, qint32 errorCode, DownloadWorker* worker = NULL);
        void workerPostponed(RequestId reqId);

        bool isErrorRetryable(DownloadError::type errorType, qint32 errorCode) const;

        void adjustMemoryAllocated(qint64 bytes);
        qint64 getMemoryHighWatermark() { return mMemoryHighWatermark; }
        void addClusterRequestCount(qint32 amount) { mClusterRequestCount += amount; }
        void addSingleFileChunkCount(qint32 amount) { mSingleFileChunkCount += amount; }
        qint32 getClusterRequestCount() { return mClusterRequestCount; }
        qint32 getSingleFileChunkCount() { return mSingleFileChunkCount; }

        qint16 getActiveDownloadWorkers();
        qint16 getActiveDownloadChannels();
        QString getWorkerReportString();

    protected slots:
        void Heartbeat();
        void OnEventThreadStartup();
        void CleanupThreadData();

    private:
        QMutex mSyncRequestLock;
        bool mSyncRequestsCanceled;

        QSet<SyncRequestHelper*> mActiveSyncRequests;
        void addActiveSyncRequest(SyncRequestHelper* request);
        void removeActiveSyncRequest(SyncRequestHelper* request);
		qint16 getWorkerIndex(DownloadWorker* workerToFind);

        QThread* mEventThread;
        bool     mHeartbeatRunning;
        bool     mShuttingDownSession;
		bool     mPhysicalMedia;
		bool     mFileOverride;

        DownloadRateLimiter mRateLimiter;

        QString mUrl;
        QString mHostName;
        QString mPrimaryHostIP; // Main IP, for non-multi-session usage
        QString mId;
        
        QUuid mUuid;
        
        qint64 mCompletedWorkers;

        bool mSafeMode;
        quint64 mIdealRequestSize;
        quint64 mCurRequestSize;
        qint16 mMaxWorkers;
        qint64 mWorkerErrorLevel;
        qint64 mConsecutiveSuccessfulWorkers;

		ChunkSizeSpeedTracker	mChunkSpeedTracker;

        QMutex mDownloadRequestMapLock;
        QMap<RequestId, DownloadRequest*> mDownloadRequests;
        DownloadPriorityQueue mPriorityQueue;

        QList<DownloadWorker*> mActiveDownloads;
        QList<DownloadWorker*> mWorkerCache[2];

        bool mIoDeviceReady;
        Services::Session::SessionWRef mSession;

        DNSResMap mDNSResMap;

        qint64 mMemoryHighWatermark;
        qint64 mMemoryAllocated_bytes;

        qint32 mClusterRequestCount;
        qint32 mSingleFileChunkCount;

        void servicePendingRequestQueue();
		void servicePendingRequestQueueExtraWorkers();
        void tickDownloadWorkers();
        void fastForwardDownloadWorkers();
        void adjustRequestSize(DownloadWorker* worker);
        void checkRetryOnError(DownloadWorker* worker);

        bool retryRequest(DownloadRequest* req);
    };

    struct  ChunkDataContainerType
    {
        ChunkDataContainerType() : index(0), used(false), diagnostic_info(NULL) { }
        ~ChunkDataContainerType() { clearDiagnosticInfo(); }
        void clearDiagnosticInfo() 
        { 
            diagnostic_info.clear();
        }

        qint32 index;
        bool used;
        MemBuffer data_buffer;
        QSharedPointer<DownloadChunkDiagnosticInfo> diagnostic_info;
    };

    #define kMaxChunkDataContainers 4

    class DownloadRequest : public IDownloadRequest
    {
        Q_OBJECT

    public:
        DownloadRequest(DownloadSession* parentSession, qint64 byteStart, qint64 byteEnd);
       ~DownloadRequest();

        const DownloadSession* getSession() const;

        // IDownloadRequest members
        virtual RequestId getRequestId() const;
        virtual qint64 getStartOffset() const;
        virtual qint64 getEndOffset() const;
        virtual qint64 getTotalBytesRead() const;
        virtual qint64 getTotalBytesRequested() const;

        virtual bool isSubmitted() const;
        virtual bool isInProgress() const;
        virtual bool isComplete() const;
        virtual bool isCanceled() const;
        virtual bool isPostponed() const;
        virtual bool isDiagnosticMode() const;
        virtual bool hasMoreDataToDownload() const;
        virtual void markCanceled();
        virtual void setDiagnosticMode(bool val);
        virtual qint32 getErrorState() const;
        virtual qint32 getErrorCode() const;

        virtual bool chunkIsAvailable();
        virtual MemBuffer* getDataBuffer();
        virtual void chunkMarkAsRead();
        virtual QSharedPointer<DownloadChunkDiagnosticInfo> getChunkDiagnosticInfo();

		bool isDataContainerAvailable(qint32 index = -1);
        ChunkDataContainerType* getAvailableDataContainer(qint32 order_index = 0);
        void queueDataContainer(ChunkDataContainerType*);
		qint32 incrementOrderIndex();
        void setBytesRemaining(qint64 bytes_remaining);
        void setCurrentBytesRequested(qint64 bytes_requested) { mCurrentBytesRequested= bytes_requested; }
        qint64 getCurrentBytesRequested() { return mCurrentBytesRequested; }
        bool isProcessingData() { return mDataContainersProcessing; }
        void setProcessingDataFlag(bool processing);
        bool isDownloadingComplete() { return mIsDownloadingComplete; }
        void setDownloadingComplete(bool complete);
        bool isHandlingWorkerError() { return mIsHandlingWorkerError; }
        void handleWorkerError(DownloadWorker *worker, bool retrying);
        bool areAllWorkersIdle();
        bool containsWorker(DownloadWorker *worker);
        bool canRemoveWorkers() { return mIsRemoveWorkers; }

        // internal members
        void markSubmitted();
        void markInProgress(bool val);
        void markPostponed(bool val);
        void setErrorState(qint32 errorState, qint32 errorCode = 0);

        bool retry(bool resetBytesRead = true);
        bool retry(DownloadWorker* singleWorker, bool resetBytesRead = true);
        qint8 retryCount();
        bool waitBeforeRetry();

        void setRequestState(qint64 bytesRead, bool chunkAvailable, bool isComplete, qint32 errorState, qint32 errorCode = 0);

        DownloadWorker* getWorker();
        void setWorker(DownloadWorker* worker);
        bool removeWorker(DownloadWorker* worker);
        int numWorkers();

#if defined(_DEBUG)
        static QMutex mInstanceDebugMapMutex;
        static QSet<DownloadRequest*> mInstanceDebugMap;
        static void checkForMemoryLeak();
        void insertMe();
        void removeMe();
#else
        static void checkForMemoryLeak(){}
        void insertMe(){}
        void removeMe(){}
#endif

    private:
        qint64 mRequestId;

        DownloadSession* mSession;

        // If this request has a download worker
        QMutex mRequestWorkerListLock;
        QList<DownloadWorker*> mRequestWorkerList;

        bool _isSubmitted;
        bool _isComplete;
        bool _isCanceled;
        bool _isPostponed;
        bool _isDiagnosticMode;

        qint8 _retryCount;

        qint64 _lastAttempt;

        qint32 _errorState;
        qint32 _errorCode;

        qint64 _byteStart;
        qint64 _byteEnd;

        qint64 mBytesRead;

        // Multiple chunk data buffers per request
        bool   mIsRemoveWorkers;
        bool   mIsDownloadingComplete;
        bool   mIsHandlingWorkerError;
        bool   mDataContainersProcessing;   // true when a chain of queued data containers are being processed
		qint64 mBytesRemaining;
        qint64 mCurrentBytesRequested;  // total bytes assigned to workers to download
		qint32 mProcessingIndex;	// chunk index that we are waiting for next to process
        qint32 mOrderIndex;			// chunk index given to next container when a buffer is requested for writing to

        QMutex mDataContainersLock;
        ChunkDataContainerType mDataContainers[kMaxChunkDataContainers];
        QList<ChunkDataContainerType*> mAvailableDataContainers;
        QList<ChunkDataContainerType*> mQueuedDataContainers;

        MemBuffer _chunkData;
        bool	  _chunkDataReady;

        bool _isInProgress;
    };

} // namespace Downloader
} // namespace Origin

#endif
