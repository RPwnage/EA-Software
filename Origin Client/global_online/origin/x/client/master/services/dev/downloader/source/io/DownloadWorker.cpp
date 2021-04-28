#include "io/DownloadWorker.h"
#include "ChannelInterface.h"
#include "io/DownloadServiceInternal.h"

#include "services/downloader/Timer.h"
#include "services/downloader/LogHelpers.h"

#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "services/platform/PlatformService.h"
#include "services/settings/SettingsManager.h"

#include "TelemetryAPIDLL.h"

#include <QReadLocker>
#include <QWriteLocker>

#define WORKERLOG_PREFIX "[ID:" << mDownloadSession->id() << "] "

#include "services/downloader/CRCMap.h"

#define MAX_CONTAINER_STARVATION_COUNT 2000	// was 100 but was getting too many false positives where slow connections took more than 9-10 secs to dl ~16MB - effectively turn off for now

using namespace Origin::Downloader::LogHelpers;

namespace Origin
{
namespace Downloader
{

// Stall detection constant
// 2 min 'cuz QT's default is 2 min.  (Same as Channel interface default timeout)  
// If a channel doesn't download a chunk in this period it is considered timed out.
// QtNetworkReply currently doesn't always return 
const qint64 ProgressTimeout = 120*1000;
// for Physical Media (DVD) the timeout for no progress should be much shorter (15 secs)
// since a missing disc can't recover even if the disc is re-inserted before the timeout.
const qint64 PhysicalMediaProgressTimeout = 15*1000;

#ifdef _DEBUG
quint64 DownloadWorker::sNextId = 1;
#endif
quint64 gIdleWorkerTotal = 0;
quint64 gIdleWorkerTotalCluster = 0;
quint64 gIdleWorkerStartTicks = 0;
quint64 gIdleWorkerEndTicks = 0;
extern bool g_logRequestResponseHeader;


DownloadWorker::DownloadWorker(DownloadSession* downloadSession, Services::Session::SessionWRef session,
    quint64 startOffset, quint64 endOffset)
: mCurrentErrorType(DownloadError::OK)
, mCurrentErrorCode(0)
, mDownloadSession(downloadSession)
#ifdef _DEBUG
, mId(sNextId++)
#endif
, mBuffer(NULL)
, mnEndTimeTick(0)
, mStartOffset(startOffset)
, mEndOffset(endOffset)
, mChannelInterface(0)
, mState(kStUninitialized)
{
    mChannelInterface = ChannelInterface::Create(mDownloadSession->getUrl(), session);

    if(mChannelInterface != NULL)
    {
        mChannelInterface->SetAsyncMode(true);				
        mChannelInterface->AddSpy(this);
        mChannelInterface->AddRequestInfo("Host", mDownloadSession->getHostName());
        
        if(!mChannelInterface->Connect())
        {
            ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Failed to connect channel interface for URL: " << logSafeDownloadUrl(mDownloadSession->getUrl());
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mDownloadSession->id().toLocal8Bit().constData(), "ChannelConnectionFailed", logSafeDownloadUrl(mDownloadSession->getUrl()).toLocal8Bit().constData(), __FILE__, __LINE__);
            setState(kStError);
            mCurrentErrorType = DownloadError::ContentUnavailable;
        }
    }
    else
    {
        ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Failed to create valid channel in download worker.";
        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mDownloadSession->id().toLocal8Bit().constData(), "CreateChannelInWorkerFailed", logSafeDownloadUrl(mDownloadSession->getUrl()).toLocal8Bit().constData(), __FILE__, __LINE__);
        setState(kStError);
        mCurrentErrorType = DownloadError::UNKNOWN;
    }
}

DownloadWorker::~DownloadWorker()
{
    shutdown();	
    if (mBuffer) 
    {
#ifdef DEBUG_DOWNLOAD_MEMORY_USE
        if (mBuffer->IsOwner())
        {
            if (getType() == kTypeSingle)
            {
                ORIGIN_LOG_DEBUG << "Single";
            }
            else
            {
                ORIGIN_LOG_DEBUG << "Cluster";
            }
            mDownloadSession->adjustMemoryAllocated(-(qint64) mBuffer->TotalSize());
        }
#endif
        delete mBuffer;
    }
}


void DownloadWorker::shutdown()
{
    if (mChannelInterface)
    {
        // moved here from LegacyDownloadWorker::Tick() to handle other case like cancelling
        mChannelInterface->CloseRequest();
        mChannelInterface->RemoveRequestInfo("Range");
        mChannelInterface->RemoveRequestInfo("Host");

        // make sure the channel is completely done before sending the channel back in the pool
        // as a lot of bad things can happen (for file loading) if the mChannel is cleared while
        // in use on another thread as well as variables getting reset after the channel has been
        // reassigned to another worker.
        while (mChannelInterface->IsTransferring())
            Origin::Services::PlatformService::sleep(10);

        mChannelInterface->RemoveSpy(this);
        delete mChannelInterface;
    }
}

bool DownloadWorker::initializeWorker(quint64 startOffset, quint64 endOffset)
{
    Q_ASSERT(mState != kStRunning);

    if(mState != kStRunning && endOffset > startOffset)
    {
        mStartOffset = startOffset;
        mEndOffset = endOffset;
        mnDownloaded = 0;
        setState(kStInitialized);
        return true;
    }
    else
    {
        ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Failed to initialize download worker [currentState: " << mState << ", startOffset: " << startOffset << ", endOffset: " << endOffset << "]";
        return false;
    }
}

void DownloadWorker::setUrl(QString url)
{
    mUrl = url;

    mChannelInterface->CloseConnection();
    mChannelInterface->SetURL(url);
    mChannelInterface->Connect();
}

bool DownloadWorker::setupWorker()
{
    return initializeWorker(mStartOffset, mEndOffset);
}

bool DownloadWorker::start()
{
    if(mState == kStInitialized)
    {
        if(reserveBuffer() && requestData())
        {
            mnStartTimeTick = GetTick();
            setState(kStRunning);
        }
        else
        {
            ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Failed to start download in DownloadWorker.";
            GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mDownloadSession->id().toLocal8Bit().constData(), "StartDownloadFail", logSafeDownloadUrl(mDownloadSession->getUrl()).toLocal8Bit().constData(), __FILE__, __LINE__);
            handleWorkerError();
            setState(kStError);
        }
    }

    return (mState == kStRunning);
}

bool DownloadWorker::cancel()
{
    QWriteLocker writeLock(&mLock);

    setState(kStCanceling);
    return true;
}

void DownloadWorker::setToIdle()
{
    setState(kStIdle);
}

bool DownloadWorker::isTransferring()
{
    if (mChannelInterface == NULL)
        return false;

    return mChannelInterface->IsTransferring();
}

qint32 DownloadWorker::getChunkSize() const
{
    return (int32_t) (mEndOffset - mStartOffset);
}

bool DownloadWorker::receiveData()
{
    qint32 nBytesRequested = getChunkSize();

    // Check for stalled download if still downloading
    if ( mnDownloaded < (quint64)nBytesRequested )
    {
        // If progress had been made, update the timestamp to current progress
        if (mnDownloaded != mnBytesDownloadedAtLastProgress)
        {
            mnBytesDownloadedAtLastProgress = mnDownloaded;
            mnTimestampOfLastProgress = GetTick();
        }
        else
        {
            // Check that the channel still has data incoming
            qint64  nTimeDelta = GetTick() - mnTimestampOfLastProgress;

            qint64 progress_timeout = ProgressTimeout;
            if (mDownloadSession->isPhysicalMediaSession())
                progress_timeout = PhysicalMediaProgressTimeout;
#if 0            
            static qint64 test = 0;

            if (((test++) % 200) == 0)
            {
                if (!(nBytesRequested & 0xf))
                {
                    nTimeDelta = progress_timeout + 1;
                }
            }
#endif

            if (nTimeDelta > progress_timeout)
            {
                // Something has gone wrong and this worker needs to be restarted
                GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mDownloadSession->id().toLocal8Bit().constData(), "TimeOutAndMissingData", logSafeDownloadUrl(mDownloadSession->getUrl()).toLocal8Bit().constData(), __FILE__, __LINE__);
                ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Bytes Downloaded:" << mnDownloaded << " is less than requested:" << nBytesRequested << " but channel is no longer transferring.  Worker must be restarted.";
                mCurrentErrorType = DownloadError::TimeOut;
                mCurrentErrorCode = 0;
                setState(kStError);
                handleWorkerError();
                return false;
            }
        }

        return true;
    }
    else
    {
		mnEndTimeTick = GetTick();
        gIdleWorkerStartTicks = GetTick();
        setState(kStDownloadComplete);
        return false;
    }
}

bool DownloadWorker::requestData()
{
    mnDownloaded = 0;

    mnTimestampOfLastProgress = GetTick();
    mnBytesDownloadedAtLastProgress = 0;		

    if ( !mChannelInterface || !mChannelInterface->IsConnected() )
    {
        mCurrentErrorType = DownloadError::ContentUnavailable;
        mCurrentErrorCode = -1;
        ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Connection is not open!";
        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mDownloadSession->id().toLocal8Bit().constData(), "ConnectionNotOpen", logSafeDownloadUrl(mDownloadSession->getUrl()).toLocal8Bit().constData(), __FILE__, __LINE__);
        setState(kStError);
        return false;
    }

    //Create the range. Exclude the last byte!!!
    QString sRequest = QString("bytes=%1-%2").arg(QString::number(mStartOffset)).arg(QString::number(mEndOffset-1));
    QString rangeStr = "Range";
    mChannelInterface->RemoveRequestInfo(rangeStr);
    mChannelInterface->AddRequestInfo(rangeStr, sRequest);

    // Okay go get some data
    setState(kStRunning);

    if(g_logRequestResponseHeader)
    {
        ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << " Sending GET request with URL (" << mDownloadSession->getUrl() << ").";

        QHash<QString, QString> headerSet = mChannelInterface->GetAllRequestInfo();
        if (!headerSet.empty())
        {
            int idx = 0;
            for (QHash<QString,QString>::const_iterator it = headerSet.begin(); it != headerSet.end(); it++)
            {
                ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << " GET request key[" << idx << "] = " << it.key() << "; value[" << idx << "] = " << it.value();
                ++idx;
            }
        }
    }

    if ( !mChannelInterface->SendRequest("GET", QString(), QString() ) || mChannelInterface->GetLastErrorType() != DownloadError::OK )
    {
        mCurrentErrorType = mChannelInterface->GetLastErrorType();
        mCurrentErrorCode = mChannelInterface->GetErrorCode();
        ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Couldn't send HTTP Request! GetLastError: " << mCurrentErrorType << "\n";
        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mDownloadSession->id().toLocal8Bit().constData(), "HTTPRequestFailed", logSafeDownloadUrl(mDownloadSession->getUrl()).toLocal8Bit().constData(), __FILE__, __LINE__);
        setState(kStError);
        return false;
    }

    return true;
}

void DownloadWorker::setState(E_WorkerState newState)
{
    mState = newState;
}

void DownloadWorker::tick()
{
    QReadLocker readLock(&mLock);

    switch(mState)
    {
        case kStUninitialized:
        {
            if(setupWorker())
            {
                setState(kStInitialized);
            }
            else
            {
				ORIGIN_LOG_ERROR << "Worker error";
                setState(kStError);
            }
            break;
        }
        case kStInitialized:
        {
            start();
            break;
        }
        case kStRunning:
            receiveData(); 
            // Deliberate case drop through
        case kStDownloadComplete:
            handleIncomingData(mState == kStDownloadComplete);
            break;
        case kStCanceling:
            handleWorkerCanceled();
            setState(kStCanceled);

            if(mChannelInterface->IsTransferring())
            {
                mChannelInterface->CloseRequest();
                mChannelInterface->RemoveRequestInfo("Range");

                // make sure the channel is completely done before sending the channel back in the pool
                // as a lot of bad things can happen (for file loading) if the mChannel is cleared while
                // in use on another thread as well as variables getting reset after the channel has been
                // reassigned to another worker.
                while (mChannelInterface->IsTransferring())
                    Origin::Services::PlatformService::sleep(10);
            }
            break;
        case kStPostponing:
            handleWorkerPostponed();
            setState(kStPostponed);

            if(mChannelInterface->IsTransferring())
            {
                mChannelInterface->CloseRequest();
                mChannelInterface->RemoveRequestInfo("Range");

                // make sure the channel is completely done before sending the channel back in the pool
                // as a lot of bad things can happen (for file loading) if the mChannel is cleared while
                // in use on another thread as well as variables getting reset after the channel has been
                // reassigned to another worker.
                while (mChannelInterface->IsTransferring())
                    Origin::Services::PlatformService::sleep(10);
            }
            break;
        case kStChannelError:
            handleWorkerError();
            setState(kStError);
            break;
        case kStError:
            if (getType() == DownloadWorker::kTypeSingle)
            {
                DownloadSingleRequestWorker* singleWorker = static_cast<DownloadSingleRequestWorker*>(this);
                if (!singleWorker->getRequest()->containsWorker(this))    // this means to cancel because one of the workers encountered an error
                {
                    setState(kStCompleted);
                }
            }
            break;

        default: // do nothing
            break;
    }
}

void DownloadWorker::handleIncomingData(bool dataComplete)
{
    if(dataComplete)
    {
        setState(kStCompleted);
    }
}

void DownloadWorker::handleWorkerError()
{
    ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Error in download worker [type=" << mCurrentErrorType << ", code=" << mCurrentErrorCode << "]";
    // create an empty download session vector
    QVector<DownloadRequest*> requests;
    mDownloadSession->workerError(requests, mCurrentErrorType, mCurrentErrorCode);
}

quint64 DownloadWorker::getStartOffset() const
{
    return mStartOffset;
}

quint64 DownloadWorker::getEndOffset() const
{
    return mEndOffset;
}

quint64 DownloadWorker::getDataAvailableOffset() const
{
    return mnDownloaded;
}

DownloadWorker::E_WorkerState DownloadWorker::getState() const
{
    return mState;
}

void DownloadWorker::reset()
{
    setState(kStUninitialized);
    mCurrentErrorCode = 0;
    mCurrentErrorType = DownloadError::OK;
    mnDownloaded = 0;
}

bool DownloadWorker::getData(MemBuffer *mem, quint64 startOffset, quint64 endOffset, bool copyData) const
{
    if(startOffset >= mnDownloaded ||
       (endOffset > mnDownloaded))
    {
        return false;
    }
    else
    {
        if(mBuffer)
        {
            if(copyData)
            {
#ifdef DEBUG_DOWNLOAD_MEMORY_USE
                ORIGIN_LOG_DEBUG << "Cluster";
                mDownloadSession->adjustMemoryAllocated(endOffset - startOffset);
#endif
                mem->DeepCopy(mBuffer->GetBufferPtr() + startOffset, endOffset - startOffset);
            }
            else
            {
#ifdef DEBUG_DOWNLOAD_MEMORY_USE
                if (mem->IsOwner())
                {
                    ORIGIN_LOG_DEBUG << "Cluster";

                    mDownloadSession->adjustMemoryAllocated(-(qint64) mem->TotalSize());
                }
#endif
                mem->Destroy();
                mem->Assign(mBuffer->GetBufferPtr() + startOffset, endOffset - startOffset, false);
            }

            return true;
        }
        else
        {
            return false;
        }
    }
}

bool DownloadWorker::getData(MemBuffer *mem, bool copyData) const
{
//	 TODO we may do some refactoring on the lecacy worker here to
//	 be able to pull the buffer out and avoid copying data...
    if ( mBuffer )
    {
        if(copyData)
        {
#ifdef DEBUG_DOWNLOAD_MEMORY_USE
            ORIGIN_LOG_DEBUG << "Single";
            mDownloadSession->adjustMemoryAllocated(mnDownloaded);
#endif
            mem->DeepCopy(mBuffer->GetBufferPtr(), mnDownloaded);
        }
        else
        {
            mem->Assign(*mBuffer);
        }
        
        return true;
    }
    return false;
}

bool DownloadWorker::reserveBuffer()
{
    quint32 chunkSize = getChunkSize();
    ORIGIN_ASSERT(chunkSize > 0);

    if(mBuffer != NULL)// && chunkSize > mBuffer->TotalSize())
    {
#ifdef DEBUG_DOWNLOAD_MEMORY_USE
        if (mBuffer->IsOwner())
        {
            if (getType() == kTypeSingle)
            {
                ORIGIN_LOG_DEBUG << "Single";
            }
            else
            {
                ORIGIN_LOG_DEBUG << "Cluster";
            }

            mDownloadSession->adjustMemoryAllocated(-(qint64) mBuffer->TotalSize());
        }
#endif
        delete mBuffer;
        mBuffer = NULL;
    }

    if(mBuffer == NULL)
    {
#ifdef DEBUG_DOWNLOAD_MEMORY_USE
        if (getType() == kTypeSingle)
        {
            ORIGIN_LOG_DEBUG << "Single";
        }
        else
        {
            ORIGIN_LOG_DEBUG << "Cluster";
        }
        mDownloadSession->adjustMemoryAllocated(chunkSize);
#endif
        mBuffer = new MemBuffer(chunkSize);
        if ( mBuffer == NULL )
        {
            ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Failed to allocate buffer of " << chunkSize << "!";
            return false;
        }
    }

    return true;
}

void DownloadWorker::destroyBuffer()
{
    if (mBuffer)
    {
#ifdef DEBUG_DOWNLOAD_MEMORY_USE
        if (mBuffer->IsOwner())
        {
            if (getType() == kTypeSingle)
            {
                ORIGIN_LOG_DEBUG << "Single";
            }
            else
            {
                ORIGIN_LOG_DEBUG << "Cluster";
            }
            mDownloadSession->adjustMemoryAllocated(-(qint64) mBuffer->TotalSize());
        }
#endif
        delete mBuffer;
        mBuffer = NULL;
    }
}

void DownloadWorker::onConnected(ChannelInterface*)
{

}

void DownloadWorker::onDataAvailable(ChannelInterface*,
                                         ulong* pBytes,
                                         void** ppBuffer_ret)
{
    int32_t lMaxBytesToDL = ((int32_t) getChunkSize() - (int32_t) mnDownloaded);
    if (lMaxBytesToDL == 0)
        return;

    // Reduce expected delivery size if larger than buffer to receive it
    if ((int32_t) *pBytes > lMaxBytesToDL)
        *pBytes = (uint32_t) lMaxBytesToDL;

    // Set read buffer
    ORIGIN_ASSERT(mBuffer);
    ORIGIN_ASSERT(ppBuffer_ret);
    if (ppBuffer_ret)
        *ppBuffer_ret = mBuffer->GetBufferPtr() + mnDownloaded;
}

void DownloadWorker::onDataReceived(ChannelInterface*,
                                        ulong lBytes)
{
    mnDownloaded += lBytes;

    int32_t lTotalBytesToDL = getChunkSize();

    if (mnDownloaded >= (quint64)lTotalBytesToDL || lBytes == 0)
    {
        // Make a note in the log if finishing before requested block was sent
        if (lBytes == 0 && mnDownloaded < (quint64)lTotalBytesToDL)
        {
            ORIGIN_LOG_WARNING << WORKERLOG_PREFIX << "Rcv'd a block shorter than requested. Requested bytes " << mStartOffset << " thru " << mEndOffset << " (" << getChunkSize() << " bytes). Rcv'd " << mnDownloaded << " bytes";
            ORIGIN_ASSERT(false);
        }
    }
}

void DownloadWorker::onConnectionClosed(ChannelInterface*)
{

}

void DownloadWorker::onError(ChannelInterface* channelInterface, DownloadError::type errorType)
{		
    ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Received error from the " << channelInterface->GetType() << " channel: " << ErrorTranslator::Translate((ContentDownloadError::type)errorType) << " with code [" << channelInterface->GetErrorCode() << "]";
    mCurrentErrorType = errorType;
    setState(kStChannelError);

    if (channelInterface)
    {
        mCurrentErrorCode = channelInterface->GetErrorCode();
    }
}

DownloadClusterWorker::DownloadClusterWorker(DownloadSession* downloadSession, Services::Session::SessionWRef session, const QVector<DownloadRequest*>& requests, quint64 clusterSize) :
    DownloadWorker(downloadSession, session)
{
    init(requests, clusterSize);
}

DownloadClusterWorker::~DownloadClusterWorker()
{

}

QVector<DownloadRequest*>& DownloadClusterWorker::getRequests()
{
    return mRequests;
}

void DownloadClusterWorker::init(const QVector<DownloadRequest*>& requests, quint64 clusterSize)
{
    mRequests = requests;
    mClusterSize = clusterSize;
    mCompletedRequests = 0;
    mNextRequest = 0;
    mWaitingForRequestsToClose = false;

    foreach(DownloadRequest* req, mRequests)
    {
        req->markInProgress(true);
        req->markPostponed(false);
        req->setWorker(this);
    }

    this->setState(kStUninitialized);
}

void DownloadClusterWorker::requestClosed(RequestId req, bool shuttingDown)
{
    mCompletedRequests++;
    if (mCompletedRequests == mRequests.size())
    {
        mDownloadSession->workerChunkCompleted(this);

        foreach(DownloadRequest* req, mRequests)
        {
            if (!req->removeWorker(this))
                req->deleteLater();    // only delete if the request has no other workers attached
        }

        mRequests.clear();

        if(!shuttingDown)
        {
            setState(kStCompleted);
            destroyBuffer();    // clear when done instead of keeping the largest buffer as it can idly hog 40+MB 
        }
    }
}

void DownloadClusterWorker::handleWorkerCanceled()
{
    foreach(DownloadRequest* mRequest, mRequests)
    {
        mRequest->markInProgress(false);
        mRequest->setErrorState(DownloadError::DownloadWorkerCancelled);
        mRequest->setWorker(NULL);
    }
    mDownloadSession->workerError(mRequests, DownloadError::DownloadWorkerCancelled, 0);
}

void DownloadClusterWorker::handleWorkerPostponed()
{
    foreach(DownloadRequest* mRequest, mRequests)
    {
        mRequest->markInProgress(false);
        mRequest->markPostponed(true);
        mRequest->setWorker(NULL);
        mDownloadSession->workerPostponed(mRequest->getRequestId());
    }
}

bool DownloadClusterWorker::setupWorker()
{
    if (mClusterSize == 0)
    {
        ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Invalid request length in cluster download task";

        foreach(DownloadRequest* req, mRequests)
        {
            req->setErrorState(DownloadError::InvalidDownloadRequest);
        }
        mDownloadSession->workerError(mRequests, DownloadError::InvalidDownloadRequest, 0);
        return false;
    }
    else
    {

#ifdef DEBUG_DOWNLOAD_SERVICE
        ORIGIN_LOG_DEBUG << WORKERLOG_PREFIX << "Setting up clustered request from " << mRequests.front()->getStartOffset() << " to " << mRequests.back()->getEndOffset();
#endif
        return initializeWorker(mRequests.front()->getStartOffset(), mRequests.back()->getEndOffset());
    }
}

void DownloadClusterWorker::handleWorkerError()
{
    foreach(DownloadRequest* req, mRequests)
    {
        req->markInProgress(false);
        req->setErrorState(mCurrentErrorType, mCurrentErrorCode);
        req->setWorker(NULL);
    }
    mDownloadSession->workerError(mRequests, mCurrentErrorType, mCurrentErrorCode, this);
}

void DownloadClusterWorker::handleIncomingData(bool dataComplete)
{
    if(mWaitingForRequestsToClose)
    {
        return;
    }

    quint64 downloaded = getDataAvailableOffset();

    if (dataComplete && downloaded != mClusterSize)
    {
        ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Cluster read error, expected " << mClusterSize << " but only received " << downloaded << " bytes.";

        QVector<Origin::Downloader::DownloadRequest *> requests;
        int i=0;
        foreach(DownloadRequest* req, mRequests)
        {
            if(!req->isComplete())
            {
                requests[i++] = req;
                req->setErrorState(DownloadError::ClusterRead);
            }
        }
        if(!requests.isEmpty())
            mDownloadSession->workerError(requests, DownloadError::ClusterRead, 0);
    }
    else
    {
        qint64 startOffset = mRequests.front()->getStartOffset();

        for(qint32 i = mNextRequest; i < mRequests.size(); i++)
        {
            // Only process up to last completed request
            if((qint64)downloaded >= (mRequests[i]->getEndOffset() - startOffset))
            {
                ChunkDataContainerType* container = mRequests[i]->getAvailableDataContainer();

                getData(&container->data_buffer, mRequests[i]->getStartOffset() - startOffset, mRequests[i]->getEndOffset() - startOffset);

                mRequests[i]->queueDataContainer(container);

                // usually close the request
                mRequests[i]->setRequestState(mRequests[i]->getTotalBytesRequested(), true, true, 0);

                // notify the downloadSession, that data is available
                mDownloadSession->workerChunkAvailable(mRequests[i]->getRequestId());
            }
            else
            {
                mNextRequest = i;
                // break out early and save our position
                break;
            }
        }
        
        if(dataComplete)
        {
#ifdef DEBUG_DOWNLOAD_SERVICE
            ORIGIN_LOG_DEBUG << WORKERLOG_PREFIX << "Successfully received cluster of size " << mClusterSize;
#endif
            mWaitingForRequestsToClose = true;

            mDownloadSession->addClusterRequestCount(mRequests.size());
        }
    }
}

DownloadSingleRequestWorker::DownloadSingleRequestWorker(DownloadSession *downloadSession, Services::Session::SessionWRef session, DownloadRequest *req) :
    DownloadWorker(downloadSession, session),
    mRequest(NULL),
    mBytesRead(0),
    mCurrentChunkSize(0),
    mWaitingOnChunkProcessing(false)
{
    init(req);
}

DownloadSingleRequestWorker::~DownloadSingleRequestWorker()
{

}

void DownloadSingleRequestWorker::init(DownloadRequest* req)
{
    mRequest = req;
    QVector<Origin::Downloader::DownloadRequest *> requests;
    if (mRequest == NULL)
    {
        ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Download request is NULL";
        // use an empty requests vector
        mDownloadSession->workerError(requests, DownloadError::InvalidDownloadRequest, 0);
        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mDownloadSession->id().toLocal8Bit().constData(), "NULLDownloadReq", logSafeDownloadUrl(mDownloadSession->getUrl()).toLocal8Bit().constData(), __FILE__, __LINE__);
        setState(kStError);
    }
    else if (mRequest->getTotalBytesRequested() <= 0)
    {
        ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Invalid download length requested";
        mRequest->setErrorState(DownloadError::InvalidDownloadRequest);
        requests.append(mRequest);
        mDownloadSession->workerError(requests, DownloadError::InvalidDownloadRequest, 0);
        GetTelemetryInterface()->Metric_DL_IMMEDIATE_ERROR(mDownloadSession->id().toLocal8Bit().constData(), "InvalidDownloadLenReq", logSafeDownloadUrl(mDownloadSession->getUrl()).toLocal8Bit().constData(), __FILE__, __LINE__);
        setState(kStError);
    }
    else
    {
        mSessionRequestSize = mDownloadSession->getRequestSize();
        mCurrentChunkSize = 0;
		mOrderIndex = 0;
        mBytesRead = mRequest->getTotalBytesRead();
        mWaitingOnChunkProcessing = false;
        mRequest->setWorker(this);
        mRequest->markPostponed(false);
    }
}

DownloadRequest* DownloadSingleRequestWorker::getRequest()
{
    return mRequest;
}

void DownloadSingleRequestWorker::requestClosed(RequestId reqId, bool shuttingDown)
{
    if(reqId == mRequest->getRequestId())
    {
        mDownloadSession->workerChunkCompleted(this);
        if (!mRequest->removeWorker(this))
            mRequest->deleteLater();    // only delete if the request has no other workers attached
        mRequest = NULL;
    
        if(!shuttingDown)
            setState(kStCompleted); // so it gets removed from active worker list otherwise it gets orphaned
    }
    else
    {
        ORIGIN_LOG_ERROR << WORKERLOG_PREFIX << "Request closed call on worker with different request id " << reqId << " - " << mRequest->getRequestId();
    }
}

void DownloadSingleRequestWorker::handleWorkerCanceled()
{
    if(mRequest)
    {
        QVector<Origin::Downloader::DownloadRequest *> requests;
        requests.push_back(mRequest);
        mRequest->markInProgress(false);
        mRequest->setErrorState(DownloadError::DownloadWorkerCancelled);
        mRequest->setWorker(NULL);
        mDownloadSession->workerError(requests, DownloadError::DownloadWorkerCancelled, 0);
    }
}

void DownloadSingleRequestWorker::handleWorkerPostponed()
{
    if(mRequest)
    {
        mRequest->markInProgress(false);
        mRequest->markPostponed(true);
        mRequest->removeWorker(this);
        mDownloadSession->workerPostponed(mRequest->getRequestId());
    }
}

void DownloadSingleRequestWorker::handleWorkerError()
{
    if(mRequest)
    {	
        QVector<Origin::Downloader::DownloadRequest *> requests;
        requests.push_back(mRequest);
        mRequest->markInProgress(false);
        mRequest->setErrorState(mCurrentErrorType, mCurrentErrorCode);
//        mRequest->setWorker(NULL);
        mDownloadSession->workerError(requests, mCurrentErrorType, mCurrentErrorCode, this);
    }
}

bool DownloadSingleRequestWorker::canRemoveOnError()
{
    bool retval = !(mRequest && !mRequest->canRemoveWorkers() && mRequest->containsWorker(this));
#ifdef DEBUG_DOWNLOAD_MULTI_WORKER
    if (retval)
    {
        ORIGIN_LOG_ERROR << "allow removal of errored worker for request: " << mRequest->getRequestId() << "  order index: " << mOrderIndex;
    }
#endif
    return retval;
}

void DownloadSingleRequestWorker::handleIncomingData(bool dataComplete)
{
    // Request was completed, waiting for it to be serviced by protocol
    if(mRequest == NULL || mRequest->isComplete())
    {
        return;
    }
    else if (mRequest->isCanceled())
    {
        if(getState() != kStCanceling && getState() != kStCanceled)
        {
            setState(kStCanceling);
        }
        return;
    }
    else if(dataComplete)
    {
        if (!mRequest->containsWorker(this))    // this means to cancel because one of the workers encountered an error
        {
            ORIGIN_LOG_ERROR << "Handling Worker Error setting state to completed - req: " << mRequest->getRequestId() << " order index: " << mOrderIndex;
            setState(kStCompleted);
            return;
        }

		if ((mOrderIndex > -1) && mRequest->isDataContainerAvailable(mOrderIndex) && !mRequest->isDownloadingComplete() && !mRequest->isHandlingWorkerError())
        {
            // Postponing isn't urgent, so we don't need to discard this data by returning early, but we will stop queuing further chunks
            bool postponed = false;
            if (mRequest->isPostponed())
            {
                ORIGIN_LOG_EVENT << "[Req:" << mRequest->getRequestId() << "] Worker postponed ; # Workers for this request: " << mRequest->numWorkers();
                postponed = true;
                if (getState() != kStPostponing && getState() != kStPostponed)
                {
                    setState(kStPostponing);
                }
            }

            mDownloadSession->addSingleFileChunkCount(1);

            ChunkDataContainerType* container = mRequest->getAvailableDataContainer(mOrderIndex);

            getData(&container->data_buffer);

            // If the request is marked for diagnostics, compute the CRC for the received data
            if (mRequest->isDiagnosticMode())
            {
                container->diagnostic_info = QSharedPointer<DownloadChunkDiagnosticInfo>(new DownloadChunkDiagnosticInfo());

                QUrl thisWorkerUrl(getUrl());
                QString host_ip(thisWorkerUrl.host());
                QString host_name(mDownloadSession->getHostName());
                QString file(thisWorkerUrl.path());
                quint32 chunkCRC = CRCMap::CalculateFinalCRC(CRCMap::CalculateCRC(CRCMap::GetCRCInitialValue(), (void*)container->data_buffer.GetBufferPtr(), container->data_buffer.TotalSize()));

                // If the IP and hostname are the same, we're probably not doing multi-session, so just grab the primary IP if we have it
                if (host_ip == host_name)
                {
                    QString primaryHostIP = mDownloadSession->getPrimaryHostIP();

                    if (!primaryHostIP.isEmpty())
                        host_ip = primaryHostIP;
                }

                container->diagnostic_info->hostname = host_name;
                container->diagnostic_info->ip = host_ip;
                container->diagnostic_info->file = file;
                container->diagnostic_info->chunkCRC.startOffset = getStartOffset();
                container->diagnostic_info->chunkCRC.endOffset = getEndOffset();
                container->diagnostic_info->chunkCRC.crc = chunkCRC;
                container->diagnostic_info->chunkCRC.timestamp = (QDateTime::currentMSecsSinceEpoch() / 1000); // Unix timestamp in seconds
            }

            mRequest->queueDataContainer(container);
#ifdef DEBUG_DOWNLOAD_MULTI_WORKER
			ORIGIN_LOG_DEBUG << "["  << mRequest->getRequestId() << "] Queueing DATA CONTAINER: " << mOrderIndex << "  (" << getStartOffset() << ", " << getEndOffset() << ") [" << (getEndOffset() - getStartOffset()) << "]";
#endif

            mOrderIndex = -1;   // mark as queued

            // Update the counters
            mBytesRead = mRequest->getTotalBytesRead();
            mBytesRead += mCurrentChunkSize;

#ifdef DEBUG_DOWNLOAD_MULTI_WORKER
			if (mBytesRead >= (quint64)mRequest->getTotalBytesRequested())
			{
				ORIGIN_LOG_DEBUG << "Request complete? BytesRead: " << mBytesRead << "  Bytes Requested: " << (quint64)mRequest->getTotalBytesRequested();
			}
#endif

            mRequest->setDownloadingComplete((mBytesRead == (quint64)mRequest->getTotalBytesRequested()));
            // usually close the request
            mRequest->setRequestState(mBytesRead, true, false, 0);

			if (mRequest->chunkIsAvailable() && !mRequest->isProcessingData())   // need to make thread-safe
			{
				// notify the downloadSession, that data is available
                mRequest->setProcessingDataFlag(true);// need to make thread-safe
				mDownloadSession->workerChunkAvailable(mRequest->getRequestId());
			}

            quint64 startOffset, endOffset;

            if(!mRequest->isDownloadingComplete())
            {              
                mDownloadSession->workerChunkCompleted(this);

                // If we're postponed, we don't want to queue up the next chunk
                if (postponed)
                    return;

                // Queue up the next chunk and reset the worker
                if(determineNextChunk(startOffset, endOffset))
                {
                    reset();
                    initializeWorker(startOffset, endOffset);
                }
            }
        }
    }
}

bool DownloadSingleRequestWorker::determineNextChunk(quint64& startOffset, quint64& endOffset)
{
    if(mRequest == NULL)
    {
        return false;
    }

    qint64 totalBytesRequested = mRequest->getTotalBytesRequested();
    mSessionRequestSize = mDownloadSession->getRequestSize();
    qint64 current_bytes_requested = mRequest->getCurrentBytesRequested();

    if(current_bytes_requested < totalBytesRequested)
    {
        // Make sure we can fit the data
        quint64 bytesRemaining = totalBytesRequested - current_bytes_requested;
        mCurrentChunkSize = std::min<quint64>(bytesRemaining, mSessionRequestSize);

        // Make sure we don't leave a sliver
        quint64 nextBytesRemaining = totalBytesRequested - current_bytes_requested - mCurrentChunkSize;

        if (nextBytesRemaining < (mSessionRequestSize / 4))
        {
            // The next read would just be a sliver, so just read everything now
            mCurrentChunkSize = bytesRemaining;
        }

        startOffset = mRequest->getStartOffset() + current_bytes_requested;
        endOffset = startOffset + mCurrentChunkSize;

		mRequest->setBytesRemaining(totalBytesRequested - (endOffset - mRequest->getStartOffset()));
        mRequest->setCurrentBytesRequested(current_bytes_requested + mCurrentChunkSize);
        mOrderIndex = mRequest->incrementOrderIndex();
#ifdef DEBUG_DOWNLOAD_MULTI_WORKER
		ORIGIN_LOG_DEBUG << "["  << mRequest->getRequestId() << "] Assigning DATA CONTAINER: " << mOrderIndex << "  (" << startOffset << ", " << endOffset << ")";
#endif

        return true;
    }
    else
    {
        return false;
    }
}

bool DownloadSingleRequestWorker::setupWorker()
{
    quint64 startOffset, endOffset;

    if(determineNextChunk(startOffset, endOffset))
    {
        return initializeWorker(startOffset, endOffset);
    }
    else
    {
        return false;
    }
}

} // namespace Downloader
} // namespace Origin

