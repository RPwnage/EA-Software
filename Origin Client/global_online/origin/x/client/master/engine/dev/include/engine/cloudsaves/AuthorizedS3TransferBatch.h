#ifndef _CLOUDSAVES_AUTHORIZEDS3TRANSFERBATCH_H
#define _CLOUDSAVES_AUTHORIZEDS3TRANSFERBATCH_H

#include <QList>
#include <QObject>
#include <QSet>
#include <QHash>
#include <QQueue>
#include <QElapsedTimer>

#include "engine/cloudsaves/AuthorizedS3Transfer.h"
#include "services/rest/CloudSyncServiceResponses.h"
#include "services/plugin/PluginAPI.h"
#include "engine/login/LoginController.h"
#include "engine/cloudsaves/RemoteSyncSession.h"

namespace Origin
{
namespace Engine
{ 

namespace CloudSaves
{
    class S3Operation;

    class ORIGIN_PLUGIN_API TrackedAuthorizedS3Transfer
    {
    public:
        // Takes owner of transfer
        explicit TrackedAuthorizedS3Transfer(AuthorizedS3Transfer *transfer) : 
            m_transfer(transfer), m_tries(0)
        {
            invalidateSignature();
            resetCompletedBytes();
        }

        ~TrackedAuthorizedS3Transfer()
        {
            delete m_transfer;
        }

        AuthorizedS3Transfer *transfer() { return m_transfer; }
        const AuthorizedS3Transfer *transfer() const { return m_transfer; }

        unsigned int tries() const { return m_tries; }
        void tryFailed() { m_tries++; }

        // Note if we have an invalidated signature both these return false
        bool hasExpiredSignature() const;
        bool hasValidSignature() const;

        void invalidateSignature() { m_signatureExpiryTimer.invalidate(); }

        const Origin::Services::CloudSyncServiceSignedTransfer &signature() const { return m_signature; }
        void setSignature(const Origin::Services::CloudSyncServiceSignedTransfer &);

        void resetCompletedBytes() { m_completedBytes = 0; }
        void setCompletedBytes(qint64 bytes) { m_completedBytes = bytes; }
        qint64 completedBytes() const { return m_completedBytes; }

    private:
        AuthorizedS3Transfer *m_transfer;
        unsigned int m_tries;
        QElapsedTimer m_signatureExpiryTimer;
        Origin::Services::CloudSyncServiceSignedTransfer m_signature;
        qint64 m_completedBytes;
    };

    class ORIGIN_PLUGIN_API AuthorizedS3TransferBatch : public QObject
    {
        Q_OBJECT
    public:
        AuthorizedS3TransferBatch(RemoteSyncSession *session, QObject *parent);  
        virtual ~AuthorizedS3TransferBatch();

        // We take ownership of transfer
        void addTransfer(AuthorizedS3Transfer *transfer)
        {
            m_pendingTransfers.enqueue(new TrackedAuthorizedS3Transfer(transfer));

            // This is safe because there's no way to remove transfers
            m_totalBytes += transfer->expectedSize();
        }

        void start();
        
        unsigned int remainingTransfers() const;

    signals:
        void succeeded();
        void failed();
        void progress(qint64 completedBytes, qint64 totalBytes);

        void transferSucceeded(AuthorizedS3Transfer *transfer, S3Operation *operation);

    protected slots:
        void suboperationSucceeded();
        void suboperationFailed();
        void suboperationProgress(qint64, qint64);
        void retryNext();
    
        void prepareRequests();
        
        void signSucceeded();
        void signFailed();

        void sendProgressUpdate();

    protected:
        virtual void batchSucceeded();
        virtual void batchFailed();
        void shutdownActiveOperations();

    private:
        void issueOperations(); 

        // Used to tune our signature batch size
        void increaseSignatureBatchSize();
        void decreaseSignatureBatchSize();

        void wantProgressUpdate();

        // Transfers pending to be started
        typedef QQueue<TrackedAuthorizedS3Transfer*> PendingTransfers;
        PendingTransfers m_pendingTransfers;        
        // Transfers in progress
        typedef QHash<S3Operation*, TrackedAuthorizedS3Transfer*> ActiveOperations;
        ActiveOperations m_activeOperations;
        // Transfers pending to be restarted
        QQueue<TrackedAuthorizedS3Transfer*> m_retryQueue;

        // Total number of bytes required to be transfers 
        qint64 m_totalBytes;
        // Total number of bytes from completed operations (active operations are tracked separately)
        qint64 m_completedOperationBytes;

        // Number of signatures we attempt to get at once
        int m_signatureBatchSize;
        bool m_adjustingBatchSize;

        // Determines if there's a signing request in flight
        bool m_signInProgress;
        unsigned int m_consecutiveSignFailures;

        // We use this for its sync service client and lock
        RemoteSyncSession *m_syncSession;

        // Determines if we have a pending progress update
        unsigned int m_queuedProgressUpdates;
    };
}

}
}

#endif
