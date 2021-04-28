#include <QNetworkReply>
#include <QTimer>

#include "engine/cloudsaves/AuthorizedS3TransferBatch.h"
#include "engine/cloudsaves/S3Operation.h"
#include "services/log/LogService.h"
#include "services/rest/CloudSyncServiceClient.h"
#include "services/debug/DebugService.h"
#include "services/connection/ConnectionStatesService.h"
#include "engine/login/LoginController.h"

Q_DECLARE_METATYPE(Origin::Engine::CloudSaves::AuthorizedS3Transfer*)
    
namespace
{
    // Maximum number of concurrent S3 HTTP transactions
    const unsigned int MaxConcurrentOperations = 5;
    // Number of times a given transfer is tried before the batch is failed
    const unsigned int MaxTransferRetries = 3;
    // How long to wait between retries
    const unsigned int RetryIntervalMilliseconds = 5 * 1000;

    // The minimum number of signatures we attempt to get at once
    // This is also the low watermark for valid signatures before we make another
    // signing request
    const int MinimumSignatureBatchSize = 8;
    // How long we consider the signature valid for - the real server value is 15 minutes
    const unsigned int SignatureExpireMilliseconds = 10 * 60 * 1000;
    // How many progress updates we can defer before we send one synchronously
    const unsigned int MaxDeferredProgressUpdates = 24;
}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    bool TrackedAuthorizedS3Transfer::hasExpiredSignature() const
    {
        return m_signatureExpiryTimer.isValid() && m_signatureExpiryTimer.hasExpired(SignatureExpireMilliseconds);
    }
        
    bool TrackedAuthorizedS3Transfer::hasValidSignature() const
    {
        return m_signatureExpiryTimer.isValid() && !m_signatureExpiryTimer.hasExpired(SignatureExpireMilliseconds);
    }

    void TrackedAuthorizedS3Transfer::setSignature(const Origin::Services::CloudSyncServiceSignedTransfer &signature)
    {
        m_signature = signature;
        m_signatureExpiryTimer.restart();
    }
        
    AuthorizedS3TransferBatch::AuthorizedS3TransferBatch(RemoteSyncSession *session, QObject *parent) : 
        QObject(parent), 
        m_totalBytes(0),
        m_completedOperationBytes(0),
        m_signatureBatchSize(MinimumSignatureBatchSize),
        m_adjustingBatchSize(false),
        m_signInProgress(false),
        m_consecutiveSignFailures(0),
        m_syncSession(session),
        m_queuedProgressUpdates(0)
    {
    }
    
    AuthorizedS3TransferBatch::~AuthorizedS3TransferBatch()
    {
        // Free any pending transfers
        for(PendingTransfers::ConstIterator it = m_pendingTransfers.constBegin();
            it != m_pendingTransfers.constEnd();
            it++)
        {
            delete *it;
        }

        shutdownActiveOperations();
    }

    void AuthorizedS3TransferBatch::prepareRequests()
    {
        if (m_signInProgress)
        {
            // Wait for the current sign to finish or time out
            return;
        }
        
        // Count the valid signatures as we go
        int validSignatures = 0;
        QList<Origin::Services::CloudSyncServiceAuthorizationRequest> authRequests;

        for(PendingTransfers::const_iterator it = m_pendingTransfers.constBegin();
            it != m_pendingTransfers.constEnd();
            it++) 
        {
            TrackedAuthorizedS3Transfer *tracked = *it;

            if (tracked->hasValidSignature())
            {
                validSignatures++;
                if (validSignatures >= m_signatureBatchSize)
                {
                    // We have enough signatures
                    return;
                }
            }
            else
            {
                Origin::Services::CloudSyncServiceAuthorizationRequest authRequest;
                authRequest.setUserAttribute(QVariant::fromValue((void*)tracked));
                authRequest.setResource(m_syncSession->signedResourcePath(tracked->transfer()->remoteUrl()));
    
                switch(tracked->transfer()->verb())
                {
                case AuthorizedS3Transfer::HttpGet:
                    authRequest.setVerb(Origin::Services::HttpGet);
                    break;
                case AuthorizedS3Transfer::HttpPost:
                    authRequest.setVerb(Origin::Services::HttpPost);
                    break;
                case AuthorizedS3Transfer::HttpPut:
                    authRequest.setVerb(Origin::Services::HttpPut);
                    break;
                case AuthorizedS3Transfer::HttpDelete:
                    authRequest.setVerb(Origin::Services::HttpDelete);
                    break;
                default:
                    ORIGIN_ASSERT(0);
                }

                if (!tracked->transfer()->contentType().isNull())
                {
                    authRequest.setContentType(tracked->transfer()->contentType());
                }
            
                authRequests.append(authRequest);

                // At the same time ask the request to prepare itself
                // This entails gzipping in the background for PUT requests
                tracked->transfer()->prepare();

                if (authRequests.size() >= m_signatureBatchSize)
                {
                    // We have a big enough batch
                    break;
                }
            }
        }

        if (authRequests.isEmpty())
        {
            // Nothing to sign
            return;
        }

        // EBIBUGS-29226 Check for null user to prevent rare crash on exit.
        if (!LoginController::instance()->currentUser().isNull())
        {
            Origin::Services::CloudSyncServiceAuthenticationResponse *resp = Origin::Services::CloudSyncServiceClient::authResponse(Engine::LoginController::instance()->currentUser()->getSession(), authRequests, m_syncSession->syncServiceLock());
        
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(signSucceeded()));
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(SyncServiceAuthenticationError)), this, SLOT(signFailed()));

            m_signInProgress = true;
        }
    }

    void AuthorizedS3TransferBatch::signSucceeded()
    {
        Origin::Services::CloudSyncServiceAuthenticationResponse* resp = dynamic_cast<Origin::Services::CloudSyncServiceAuthenticationResponse*>(sender());
        ORIGIN_ASSERT(resp);
        resp->deleteLater();

        m_signInProgress = false;
        m_consecutiveSignFailures = 0;

        // Attach the signatures to our pending requests
        const QList<Origin::Services::CloudSyncServiceSignedTransfer> &signedTransfers = resp->signedTransfers();

        for(QList<Origin::Services::CloudSyncServiceSignedTransfer>::const_iterator it = signedTransfers.constBegin();
            it != signedTransfers.constEnd();
            it++) 
        {
            const Origin::Services::CloudSyncServiceSignedTransfer &signedTransfer = *it;

            // XXX: Is there any way for this to go away?
            void *voidTracked = signedTransfer.authorizationRequest().userAttribute().value<void*>();
            TrackedAuthorizedS3Transfer* tracked = reinterpret_cast<TrackedAuthorizedS3Transfer*>(voidTracked);

            ORIGIN_ASSERT(tracked);

            tracked->setSignature(signedTransfer);
        }
        
        if (signedTransfers.size() == m_signatureBatchSize)
        {
            // Our new signature batch size has taken effect - we can continue adjusting
            // the batch size based on the performance of this size
            m_adjustingBatchSize = false;
        }

        // We might've unblocked some ops
        issueOperations();
    }

    void AuthorizedS3TransferBatch::signFailed()
    {
        Origin::Services::CloudSyncServiceAuthenticationResponse* resp = dynamic_cast<Origin::Services::CloudSyncServiceAuthenticationResponse*>(sender());
        ORIGIN_ASSERT(resp);
        resp->deleteLater();

        m_signInProgress = false;
        m_consecutiveSignFailures++;

        if (m_consecutiveSignFailures >= MaxTransferRetries)
        {
            // Give up
            emit batchFailed();
            return;
        }

        // Try again in RetryIntervalMilliseconds
        // Note something else might call prepareRequests() to retry prematurely
        // I don't think that is important enough to complicate the logic with
        QTimer::singleShot(RetryIntervalMilliseconds, this, SLOT(prepareRequests()));
    }

    void AuthorizedS3TransferBatch::start()
    {
        // We need to sign/compress some requests before we can start
        prepareRequests();
    }
        
    void AuthorizedS3TransferBatch::issueOperations()
    {
        // We shouldn't issue operations in offline mode
        if (LoginController::instance()->currentUser().isNull() || !Origin::Services::Connection::ConnectionStatesService::isUserOnline (LoginController::instance()->currentUser()->getSession()))
        {
            batchFailed();
            return;
        }

        while(m_activeOperations.size() < MaxConcurrentOperations)
        {
            if (m_pendingTransfers.isEmpty())
            {    
                if (m_activeOperations.isEmpty() && m_retryQueue.isEmpty())
                {
                    // We're done!
                    batchSucceeded();
                }

                // No pending transfers to start
                return;
            }

            if (!m_pendingTransfers.head()->hasValidSignature())
            {
                // We need more presigning to happen before we can continue
                ORIGIN_LOG_EVENT << "Cloud Save: S3 batch blocked waiting for signatures";

                if (m_pendingTransfers.head()->hasExpiredSignature())
                {
                    // Signature expired while we were waiting for a slot!
                    // We're likely fetching too many at once
                    decreaseSignatureBatchSize();
                }
                else
                {
                    // We haven't got a signature yet - increase our batch size
                    increaseSignatureBatchSize();
                }

                break;
            }
            
            TrackedAuthorizedS3Transfer *nextTransfer = m_pendingTransfers.dequeue();

            nextTransfer->resetCompletedBytes();
            S3Operation *newOp = nextTransfer->transfer()->start(nextTransfer->signature()); 

            if (newOp->hasLocalFailure())
            {
                // Unable to create the operation due to some local mixup
                delete newOp;
                batchFailed();
                return;
            }

            ORIGIN_VERIFY_CONNECT(newOp, SIGNAL(succeeded()), this, SLOT(suboperationSucceeded()));
            ORIGIN_VERIFY_CONNECT(newOp, SIGNAL(failed()), this, SLOT(suboperationFailed()));
            ORIGIN_VERIFY_CONNECT(newOp, SIGNAL(progress(qint64, qint64)), this, SLOT(suboperationProgress(qint64, qint64)));

            m_activeOperations.insert(newOp, nextTransfer);
        }

        // See if we need to sign/compress some more requests
        prepareRequests();
    }
        
    void AuthorizedS3TransferBatch::suboperationSucceeded()
    {
        S3Operation *operation = dynamic_cast<S3Operation*>(sender());
        ORIGIN_ASSERT(operation);

        // Look up the transfer
        ORIGIN_ASSERT(m_activeOperations.contains(operation));
        TrackedAuthorizedS3Transfer *tracked = m_activeOperations[operation];
        m_completedOperationBytes += tracked->transfer()->expectedSize();

        // Notify any listeners
        emit transferSucceeded(tracked->transfer(), operation);
        
        // Delete the tracked transfer
        delete tracked;
        
        // And the operation
        m_activeOperations.remove(operation);
        operation->deleteLater();

        // We've made progress
        wantProgressUpdate();

        // Now we have a free slot
        issueOperations();
    }

    void AuthorizedS3TransferBatch::suboperationFailed()
    {
        S3Operation *operation = dynamic_cast<S3Operation*>(sender());
        ORIGIN_ASSERT(operation);

        // Replies that fail on file open can not have a reply associated
        if (operation->reply())
        {
            ORIGIN_LOG_WARNING << "Cloud Saves: request to " << qPrintable(operation->reply()->url().toString()) << " failed with " << qPrintable(operation->reply()->errorString());
        }

        operation->deleteLater();

        if (operation->optional())
        {
            // Delete the transfer
            ORIGIN_ASSERT(m_activeOperations.contains(operation));
            delete m_activeOperations[operation];
        
            m_activeOperations.remove(operation);

            // Pretend nothing happened
            issueOperations();
        }
        else
        {
            // Look up the transfer
            TrackedAuthorizedS3Transfer *transfer = m_activeOperations[operation];
                
            transfer->tryFailed();

            if (operation->hasLocalFailure() || (transfer->tries() > MaxTransferRetries))
            {
                // Give up
                delete transfer;
                // Fail the batch
                batchFailed();
            }
            else
            {
                // Don't trust the signature
                transfer->invalidateSignature();

                // Queue for retry
                m_retryQueue.enqueue(transfer);

                QTimer::singleShot(RetryIntervalMilliseconds, this, SLOT(retryNext()));
            }

            m_activeOperations.remove(operation);
        }

        // If the transfer was partially complete any progress has been lost
        // Note this means progress can regress
        wantProgressUpdate();
    }
        
    void AuthorizedS3TransferBatch::suboperationProgress(qint64 bytesCompleted, qint64)
    {
        S3Operation *operation = dynamic_cast<S3Operation*>(sender());
        ORIGIN_ASSERT(operation);

        // Look up our tracked transfer
        ORIGIN_ASSERT(m_activeOperations.contains(operation));
        TrackedAuthorizedS3Transfer *tracked = m_activeOperations[operation];

        // It's possible that our expected size is wrong
        // This is usually bad but it's possible for eg DELETE to have a body
        tracked->setCompletedBytes(qMin(tracked->transfer()->expectedSize(), bytesCompleted));

        wantProgressUpdate();
    }
        
    void AuthorizedS3TransferBatch::retryNext()
    {
        m_pendingTransfers.enqueue(m_retryQueue.dequeue());
        issueOperations();
    }
        
    void AuthorizedS3TransferBatch::batchSucceeded()
    {
        // Hook for subclasses to do post-success actions
        emit(succeeded());
    }    
    
    void AuthorizedS3TransferBatch::batchFailed()
    {
        for(ActiveOperations::iterator it = m_activeOperations.begin();
            it != m_activeOperations.end();
            it++)
        {
            // We don't care about any future failures
            disconnect(it.key(), SIGNAL(failed()), this, SLOT(suboperationFailed()));

            // Abort any concurrent operations
            it.key()->reply()->abort();
        }

        emit(failed());
    }

    unsigned int AuthorizedS3TransferBatch::remainingTransfers() const
    {
        return m_pendingTransfers.count() + m_activeOperations.count() + m_retryQueue.count();
    }

    void AuthorizedS3TransferBatch::sendProgressUpdate()
    {
        qint64 completedBytes = m_completedOperationBytes;

        for(ActiveOperations::const_iterator it = m_activeOperations.constBegin();
            it != m_activeOperations.constEnd();
            it++)
        {
            completedBytes += (*it)->completedBytes();
        }

        emit progress(completedBytes, m_totalBytes);
        
        m_queuedProgressUpdates = 0;
    }
        
    void AuthorizedS3TransferBatch::increaseSignatureBatchSize()
    {
        if (!m_adjustingBatchSize)
        {
            // There's no upper limit for this
            m_signatureBatchSize *= 2;
    
            // Prep some more requests
            prepareRequests();

            // Suppress adjustments until we get some feedback
            m_adjustingBatchSize = true;
        }
    }

    void AuthorizedS3TransferBatch::decreaseSignatureBatchSize()
    {
        if (!m_adjustingBatchSize)
        {
            m_signatureBatchSize = qMax(MinimumSignatureBatchSize, m_signatureBatchSize / 2);
            m_adjustingBatchSize = true;
        }
    }

    void AuthorizedS3TransferBatch::wantProgressUpdate()
    {
        m_queuedProgressUpdates++;

        if (m_queuedProgressUpdates == 1)
        {
            // Queue a call to update our progress once we've serviced all events
            QMetaObject::invokeMethod(this, "sendProgressUpdate", Qt::QueuedConnection); 
        }
        else if (m_queuedProgressUpdates > MaxDeferredProgressUpdates)
        {
            // We've compressed too many events; do this synchronously
            sendProgressUpdate();
        }
    }

    void AuthorizedS3TransferBatch::shutdownActiveOperations()
    {
        // Free any active operations
        for(ActiveOperations::ConstIterator it = m_activeOperations.constBegin();
            it != m_activeOperations.constEnd();
            it++)
        {
            delete it.key();
            delete it.value();
        }

        m_activeOperations.clear();
    }
}
}
}
