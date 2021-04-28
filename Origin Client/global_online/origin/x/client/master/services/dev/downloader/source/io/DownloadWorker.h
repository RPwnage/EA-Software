#ifndef DOWNLOADCHANNEL_H
#define DOWNLOADCHANNEL_H

#include <QUrl>
#include <QReadWriteLock>
#include "io/ChannelInterface.h"
#include "services/downloader/Common.h"
#include "services/downloader/DownloaderErrors.h"
#include "services/downloader/MemBuffer.h"
#include "services/downloader/Timer.h"
#include "io/DownloadServiceInternal.h"

namespace Origin
{
namespace Downloader
{

// this class provides actual interaction with ChannelInterfaces downloading data from file or HTTP
class DownloadWorker : protected ChannelSpy
{
public:

    enum E_WorkerState
    {
        kStUninitialized,
        kStInitialized,
        kStRunning,
        kStDownloadComplete,
        kStCanceling,
        kStCanceled,
        kStPostponing,
        kStPostponed,
        kStCompleted,
        kStChannelError,
        kStIdle,
        kStError
    };

    enum E_WorkerType
    {
        kTypeSingle,
        kTypeCluster,
        kTypeGeneric
    };

    DownloadWorker(DownloadSession* downloadSession, Services::Session::SessionWRef session, quint64 startOffset = 0, quint64 endOffset = 0);
    virtual ~DownloadWorker();

#ifdef _DEBUG
    quint64 id() { return mId; }
#endif

    void reset();
    void tick();

    virtual E_WorkerType getType() { return kTypeGeneric; }

    quint64 getStartOffset() const;
    quint64 getEndOffset() const;
    
    // Only valid when state = kStCompleted
    quint64 getRunTime() const { return mnEndTimeTick - mnStartTimeTick; }//{ return GetTick() - mnStartTimeTick; }

    quint64 getDataAvailableOffset() const;
    bool getData(MemBuffer *mem, bool copyData = false) const;
    bool getData(MemBuffer *mem, quint64 startOffset, quint64 endOffset, bool copyData = false) const;

    E_WorkerState getState() const;

    bool cancel();

    virtual void requestClosed(RequestId req, bool shuttingDown){};

    DownloadError::type getCurrentErrorType() { return mCurrentErrorType; }
    qint32 getCurrentErrorCode() { return mCurrentErrorCode; }

    void setUrl(QString url);
    QString const& getUrl() { return mUrl; }

    void setToIdle();
    virtual bool canRemoveOnError() { return true; }

    void destroyBuffer();

    bool isTransferring();

public:
    // ChannelSpy notifications
    virtual void onConnected(ChannelInterface*);
    virtual void onDataAvailable(ChannelInterface*, ulong* pBytes, void** ppBuffer_ret);
    virtual void onDataReceived(ChannelInterface*, ulong bytes);
    virtual void onConnectionClosed(ChannelInterface*);
    virtual void onError(ChannelInterface*, DownloadError::type error);

protected:
    DownloadError::type mCurrentErrorType;
    qint32 mCurrentErrorCode;

    void setState(E_WorkerState newState);

    bool initializeWorker(quint64 startOffset, quint64 endOffset);
    virtual bool setupWorker();
    virtual void handleIncomingData(bool dataComplete);
    virtual void handleWorkerError();
    virtual void handleWorkerCanceled(){}
    virtual void handleWorkerPostponed(){}

    DownloadSession* mDownloadSession;

private:
    bool start();
    bool reserveBuffer();
    bool requestData();
    bool receiveData();
    void shutdown();

    // Job parameters
    qint32	getChunkSize() const;		// Requested chunk size (use getDataAvailableOffset() for actual downloades size)

#ifdef _DEBUG
    static quint64 sNextId;
    quint64 mId;
#endif


    QReadWriteLock mLock;
    
    MemBuffer* mBuffer;

    quint64     mnStartTimeTick;
    quint64     mnEndTimeTick;

     // Stall related watch
    quint64     mnTimestampOfLastProgress;  // the timestamp of the last time the worker noted that data had been flowing
    quint64     mnBytesDownloadedAtLastProgress;    // the number of bytes that had been downloaded the last time progress had been made.  (if this value changes, then mnTimestampOfLastProgress gets updated)

    quint64	mnDownloaded;
    quint64 mStartOffset;
    quint64 mEndOffset;
    ChannelInterface* mChannelInterface;
    E_WorkerState mState;

    QString mUrl;
};

class DownloadClusterWorker : public DownloadWorker
{
    public:
        DownloadClusterWorker(DownloadSession *downloadSession, Services::Session::SessionWRef session, const QVector<DownloadRequest*>& requests, quint64 clusterSize);
        virtual ~DownloadClusterWorker();
        void init(const QVector<DownloadRequest*>& requests, quint64 clusterSize);

        virtual E_WorkerType getType() { return kTypeCluster; }

        virtual void requestClosed(RequestId reqId, bool shuttingDown);

        QVector<DownloadRequest*>& getRequests();

    protected:
        virtual bool setupWorker();
        virtual void handleIncomingData(bool dataComplete);
        virtual void handleWorkerError();
        virtual void handleWorkerCanceled();
        virtual void handleWorkerPostponed();

    private:
        QVector<DownloadRequest*> mRequests;
        quint64 mClusterSize;
        qint32 mCompletedRequests;
        qint32 mNextRequest;
        bool   mWaitingForRequestsToClose;
};

class DownloadSingleRequestWorker : public DownloadWorker
{
    public:
        DownloadSingleRequestWorker(DownloadSession *downloadSession, Services::Session::SessionWRef session, DownloadRequest *req);
        virtual ~DownloadSingleRequestWorker();

        void init(DownloadRequest* req);

        virtual E_WorkerType getType() { return kTypeSingle; }

        DownloadRequest* getRequest();

        virtual void requestClosed(RequestId reqId, bool shuttingDown);
        virtual bool canRemoveOnError();

    protected:
        virtual bool setupWorker();
        virtual void handleIncomingData(bool dataComplete);
        virtual void handleWorkerError();
        virtual void handleWorkerCanceled();
        virtual void handleWorkerPostponed();

    private:
        DownloadRequest *mRequest;

        quint64 mBytesRead;
        quint64 mCurrentChunkSize;
        quint64 mSessionRequestSize;
		qint32  mOrderIndex;	// to make sure queued chunks in a request are kept in order for decompression

        bool    mWaitingOnChunkProcessing;

        bool determineNextChunk(quint64& startOffset, quint64& endOffset);
};

} // namespace Downloader
} // namespace Origin

#endif // DOWNLOADCHANNEL_H
