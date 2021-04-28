#ifndef _CLOUDSAVES_REMOTESYNC_H
#define _CLOUDSAVES_REMOTESYNC_H

#include <limits>
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QDateTime>

#include "engine/cloudsaves/RemoteSyncSession.h"
#include "engine/content/Entitlement.h"
#include "engine/cloudsaves/LocalStateSnapshot.h"
#include "engine/cloudsaves/RemoteStateSnapshot.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    class LocalStateCalculator;
    class AbstractStateTransfer;
    class RemoteSync;

    enum FailureReason
    {
        UnknownFailure,
        NetworkFailure,
        ServiceFailure,
        QuotaExceededFailure,
        DelegateCancelledFailure,
        NoSpaceFailure,
        SystemCancelledFailure,
        ManifestLockedFailure,
        LocalAccessDeniedFailure
    };

    enum DelegateResponse
    {
        UseLocalState,
        UseRemoteState,
        CancelSync
    };

    class ORIGIN_PLUGIN_API SyncFailure
    {
    public:
        SyncFailure(FailureReason newReason = UnknownFailure, qint64 wantedBytes = -1, qint64 limitBytes = -1, const QString &failedPath = QString()) :
            m_reason(newReason),
            m_wantedBytes(wantedBytes),
            m_limitBytes(limitBytes),
            m_failedPath(failedPath)
        {
        }

        FailureReason reason() const { return m_reason; }

        qint64 wantedBytes() const { return m_wantedBytes; }
        qint64 limitBytes() const { return m_limitBytes; }

        // Only for NoSpaceFailure
        QString failedPath() const { return m_failedPath; }

    private:
        FailureReason m_reason;

        qint64 m_wantedBytes;
        qint64 m_limitBytes;

        QString m_failedPath;
    };

    class ORIGIN_PLUGIN_API RemoteSync : public QObject
    {
        Q_OBJECT
    public:

        // The user can override this on the event of conflict
        enum TransferDirection
        {
            PushToCloud,
            PullFromCloud
        };

        RemoteSync(Content::EntitlementRef entitlement, TransferDirection defaultDirection);
        ~RemoteSync();
        
        const LocalStateSnapshot *currentLocalState() const { return m_currentLocal; }
        const LocalStateSnapshot *previousSyncState() const { return m_previousSync; }
        const RemoteStateSnapshot *remoteState() const { return m_remoteState; }
        RemoteSyncSession *session() const { return m_syncSession; }

        bool hasRemoteState() const { return (m_remoteState != NULL); }    

        Content::EntitlementRef entitlement() const { return m_entitlement; }
        QString contentId() const;
        QString offerId() const;
        QString cloudSavesId() const;

        float currentProgres () { return m_currProgress;}
        TransferDirection direction () { return m_defaultDirection; }

        void releasedByManager() { emit releasedByManager(offerId(), m_defaultDirection, m_transferSucceeded, m_migrating);}

    public slots:
        void start();
        void cancel();
        void handleDelegateResponse(const int&);
        void setMigrating(const bool&);
    
    signals:
        // Indicates a push or pull has succeeded
        // Wait for finished() to account for any background cleanup
        void succeeded();
        void failed(Origin::Engine::CloudSaves::SyncFailure);

        void progress(float progress, qint64 completedBytes, qint64 totalBytes);
        void externalChangesDetected();
        
        // We should only be deleted after this is emitted
        void finished(const QString&);
        void untrustedRemoteFilesDetected();
        void emptyCloudWarning();
        void syncConflictDetected(const QDateTime &localLastModified, const QDateTime &remoteLastModified);
        
        // Emitting this signal indicates that we have already emitted the finished signal *and* the RemoteSyncManager has already removed us
        // from its hash set of in-progress remote syncs.
        void releasedByManager(const QString&, Origin::Engine::CloudSaves::RemoteSync::TransferDirection defaultDirection, const bool& transferSucceeded, const bool& reversingMigration);

    protected slots:
        void remoteStateDiscovered(Origin::Engine::CloudSaves::RemoteStateSnapshot *remote);
        void manifestRequestFailed(Origin::Engine::CloudSaves::RemoteSyncSession::ManifestRequestError);
        
        void lockPromoted();
        void lockPromotionFailed();

        void serviceLockReleased(); 

    private slots:
        void localStateCalculated(Origin::Engine::CloudSaves::LocalStateSnapshot *);

        void transferSucceeded();
        void transferFailed(Origin::Engine::CloudSaves::SyncFailure reason);
        void transferProgress(qint64, qint64);

        void useRemoteState();
        void useLocalState();


    private:
        void cancel(FailureReason reason);

        enum LastStateComparison
        {
            NoSnapshotFound,
            ChangedSinceLastSnapshot,
            UnchangedSinceLastSnapshot
        };

        void shutdownTransfer();

        void startTransfer(TransferDirection direction);
        void statesDiscovered();
        void prepStepCompleted();

        bool determineLocalState();
        LastStateComparison compareLastClientState();
        bool pullWouldConflict() const;
        bool pushWouldConflict() const;
        QSet<QString> findAddedUntrustedFiles() const;

        void updateProgress (float currProgress, qint64 completedBytes, qint64 totalBytes);

        TransferDirection m_defaultDirection;
        AbstractStateTransfer *m_transfer;
    
        LocalStateCalculator *m_localStateCalculator;

        // The states we're working with
        RemoteStateSnapshot *m_remoteState;
        LocalStateSnapshot *m_previousSync;
        LocalStateSnapshot *m_currentLocal;        

        // We need a flag because m_remoteState can be NULL if there is no manifest
        bool m_remoteStateDiscovered;

        // Number of pre-sync preparation steps we've completed
        // This is only used for user feedback
        unsigned int m_prepStepsCompleted;

        // The entitlement we're syncing
        Origin::Engine::Content::EntitlementRef m_entitlement;

        // Our current remote sync session
        Origin::Engine::CloudSaves::RemoteSyncSession *m_syncSession;

        bool m_cancelled;
        bool m_failed;

        // Did the transfer(push or pull) succeed?  Used by CloudContent to determine if we need to proceed with the next step of a migration.
        bool m_transferSucceeded;

        float m_currProgress;

        // Are we in the process of migrating cloud saves?  True if we are pulling from the old storage path.
        bool m_migrating;
    };
}
}
}

#endif
