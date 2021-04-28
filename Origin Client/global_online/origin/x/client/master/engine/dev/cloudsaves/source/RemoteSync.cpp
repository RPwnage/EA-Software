#include <limits>
#include <QDateTime>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply> 
#include <QTimer>

#include "services/debug/DebugService.h"

#include "engine/cloudsaves/ChangeSet.h"
#include "engine/cloudsaves/AbstractStateTransfer.h"
#include "engine/cloudsaves/LocalStateSnapshot.h"
#include "engine/cloudsaves/LastSyncStore.h"
#include "engine/cloudsaves/LocalStateCalculator.h"

#include "engine/cloudsaves/RemotePush.h"
#include "engine/cloudsaves/RemotePull.h"
#include "services/log/LogService.h"
#include "engine/login/LoginController.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentConfiguration.h"

#include "engine/cloudsaves/RemoteSync.h"

using namespace Origin::Engine::Content;

namespace
{
    using namespace Origin::Engine::CloudSaves;

    // Number of preparation steps 
    const unsigned int TotalPrepSteps = 2;
    // The amount of progress each prep step represents
    const float PrepStepProgress = 0.05f;

    void debugDumpChangeSet(const char *introduction, const ChangeSet &changeSet)
    {
        ORIGIN_LOG_DEBUG << introduction 
            << changeSet.added().count() << " syncables added, "
            << changeSet.deleted().count() << " syncables deleted, "
            << changeSet.localInstancesChanged().count() << " syncables have had their local instances changed"; 
    }
}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    RemoteSync::RemoteSync(Engine::Content::EntitlementRef entitlement, TransferDirection direction) :
        m_defaultDirection(direction),
        m_transfer(NULL),
        m_localStateCalculator(NULL),
        m_remoteState(NULL),
        m_previousSync(NULL),
        m_currentLocal(NULL),
        m_remoteStateDiscovered(false),
        m_prepStepsCompleted(0),
        m_entitlement(entitlement),
        m_cancelled(false),
        m_failed(false),
        m_currProgress (0.0f),
        m_migrating(false),
        m_transferSucceeded(false)
    {
        m_syncSession = new RemoteSyncSession(entitlement, m_entitlement->contentConfiguration()->cloudSaveContentIDFallback());
        
        ORIGIN_VERIFY_CONNECT(m_syncSession, SIGNAL(remoteStateDiscovered(Origin::Engine::CloudSaves::RemoteStateSnapshot*)), 
            this, SLOT(remoteStateDiscovered(Origin::Engine::CloudSaves::RemoteStateSnapshot*)));

        ORIGIN_VERIFY_CONNECT(m_syncSession, SIGNAL(manifestRequestFailed(Origin::Engine::CloudSaves::RemoteSyncSession::ManifestRequestError)), 
            this, SLOT(manifestRequestFailed(Origin::Engine::CloudSaves::RemoteSyncSession::ManifestRequestError)));
        
        ORIGIN_VERIFY_CONNECT(m_syncSession, SIGNAL(lockPromoted()), this, SLOT(lockPromoted()));
        ORIGIN_VERIFY_CONNECT(m_syncSession, SIGNAL(lockPromotionFailed()), this, SLOT(lockPromotionFailed()));
        ORIGIN_VERIFY_CONNECT(m_syncSession, SIGNAL(serviceLockReleased()), this, SLOT(serviceLockReleased()));
        ORIGIN_VERIFY_CONNECT(m_syncSession, SIGNAL(migrating(const bool&)), this, SLOT(setMigrating(const bool&)));
    }
    
    RemoteSync::~RemoteSync()
    {
        Origin::Engine::Content::ContentController* contentController = Origin::Engine::Content::ContentController::currentUserContentController();
        if(contentController)
        {
            ORIGIN_LOG_EVENT << "Destroying sync object for " << m_entitlement->contentConfiguration()->displayName() << ", offerId: " << offerId();
        }
        else
            ORIGIN_LOG_EVENT << "Destroying sync object for " << ", offerId: " << offerId();

        if (m_transfer) 
        {
            m_transfer->exit();
            m_transfer->wait();
            delete m_transfer;
        }

        if (m_localStateCalculator)
        {
            // Delete the local state calculator
            // Note this is still referencing m_previousSync
            m_localStateCalculator->wait();
            delete m_localStateCalculator;
        }
        
        // Clear out our states
        delete m_previousSync;
        delete m_currentLocal;
        delete m_remoteState;

        // Clean up our sync session
        delete m_syncSession;
        if(contentController)
            ORIGIN_LOG_EVENT << "Destruction for " << m_entitlement->contentConfiguration()->displayName() << ", offerId: " << offerId() << " completed.";
        else
            ORIGIN_LOG_EVENT << "Destruction for " <<  ", offerId: " << offerId() << " completed.";

    }
    
    void RemoteSync::start()
    {
        ORIGIN_LOG_EVENT << "Cloud Save: Remote sync started";

        if(m_defaultDirection == PullFromCloud)
        {
            m_syncSession->requestSaveManifest(RemoteSyncSession::ReadLock);
        }
        else
        {
            m_syncSession->requestSaveManifest(RemoteSyncSession::WriteLock);
        }


        determineLocalState();

        updateProgress(0.0f, -1, -1);
    }

    bool RemoteSync::determineLocalState()
    {
        // Get a snapshot of our last successful sync for this cloudSavesID
        m_previousSync = LastSyncStore::lastSyncForCloudSavesId(m_entitlement);
    
        // Calculate our local state using the last state as a cache to skip MD5s where possible
        m_localStateCalculator = new LocalStateCalculator(entitlement(), *m_previousSync);
        ORIGIN_VERIFY_CONNECT(m_localStateCalculator, SIGNAL(finished(Origin::Engine::CloudSaves::LocalStateSnapshot*)), 
                this, SLOT(localStateCalculated(Origin::Engine::CloudSaves::LocalStateSnapshot*)));
        
        m_localStateCalculator->start();

        return true;
    }
        
    void RemoteSync::localStateCalculated(LocalStateSnapshot *snapshot)
    {
        // Clean up our calculator thread
        m_localStateCalculator->wait();
        delete m_localStateCalculator;
        m_localStateCalculator = NULL;

        if (!snapshot->isValid())
        {
            cancel(LocalAccessDeniedFailure);
            delete snapshot;

            return;
        }

        m_currentLocal = snapshot;
        
        prepStepCompleted();

        if (m_remoteStateDiscovered && !m_cancelled)
        {
            // We have both states
            statesDiscovered();
        }
    }
        
    void RemoteSync::startTransfer(TransferDirection direction)
    {
        if (direction == PushToCloud)
        {
            m_transfer = new RemotePush(this);
            ORIGIN_LOG_EVENT << "Cloud Save: Push to cloud started";
        }
        else
        {
            m_transfer = new RemotePull(this);
            ORIGIN_LOG_EVENT << "Cloud Save: Pull from cloud started";
        }

        ORIGIN_VERIFY_CONNECT(m_transfer, SIGNAL(succeeded()), this, SLOT(transferSucceeded()));
        ORIGIN_VERIFY_CONNECT(m_transfer, SIGNAL(failed(Origin::Engine::CloudSaves::SyncFailure)), this, SLOT(transferFailed(Origin::Engine::CloudSaves::SyncFailure)));
        ORIGIN_VERIFY_CONNECT(m_transfer, SIGNAL(progress(qint64, qint64)), this, SLOT(transferProgress(qint64, qint64)));

        m_transfer->start();
    }
        
    void RemoteSync::transferSucceeded()
    {
        m_transferSucceeded = true;
        m_transfer->exit();
        // Set progress to 100%
        updateProgress(1.0f, -1, -1);
        m_syncSession->releaseServiceLock(true);
        emit succeeded();
        ORIGIN_LOG_EVENT << "Cloud Save: Transfer succeeded";
    }

    void RemoteSync::transferFailed(SyncFailure reason)
    {
        m_transfer->exit();

        m_failed = true;
        emit failed(reason);
        
        if (LoginController::instance()->currentUser().isNull() || !Origin::Services::Connection::ConnectionStatesService::isUserOnline (LoginController::instance()->currentUser()->getSession()))
        {
            m_failed = false;
            // We probably won't be able to release our lock
            emit finished(offerId());
        }
        else
        {
            m_syncSession->releaseServiceLock(false);
        }
    }
        
    void RemoteSync::remoteStateDiscovered(RemoteStateSnapshot *remote)
    {
        m_remoteState = remote;
        m_remoteStateDiscovered = true;

        prepStepCompleted();

        if (m_currentLocal && !m_cancelled)
        {
            // We have both states
            statesDiscovered();
        }
    }
        
    void RemoteSync::prepStepCompleted()
    {
        m_prepStepsCompleted++;
        // We don't know our transfer size yet
        updateProgress(PrepStepProgress * m_prepStepsCompleted, -1, -1);
    }

    void RemoteSync::statesDiscovered()
    {
        if (m_defaultDirection == PullFromCloud)
        {
            // Check for saves outside client
            LastStateComparison lastStateCompare = compareLastClientState();

            if (lastStateCompare == ChangedSinceLastSnapshot)
            {
                // The user definitely changed something since the last snapshot
                // Give them a warning
                emit externalChangesDetected();
            }

            if (lastStateCompare != UnchangedSinceLastSnapshot)
            {
                // There were either changes or we're missing the last snapshot

                // Warn about two cases
                // 1) Untrusted files that might have been placed on the filesystem in any situation
                //    This is done for security purposes
                // 2) There are any files on the filesystem and no files on the cloud
                //    This is to prevent inadvertently uploading someone else's files
                const bool extraFiles = !findAddedUntrustedFiles().isEmpty();
                const bool emptyCloud = !m_remoteState && !m_currentLocal->isEmpty();
    
                if (extraFiles)
                {
                    emit untrustedRemoteFilesDetected();

                    // Wait for user input
                    return;
                }

                if (emptyCloud)
                {
                    emit emptyCloudWarning();

                    // Wait for user input
                    return;
                }
            }

        }
        // Update client state to detect saves outside client
        else if (m_defaultDirection == PushToCloud)
        {
            LastSyncStore::setLocalStateForCloudSavesId(cloudSavesId(), LastSyncStore::ClientState, *m_currentLocal);
        }

        bool inConflict = (m_defaultDirection == PushToCloud) ? pushWouldConflict() : pullWouldConflict();

        if (!inConflict)
        {
            startTransfer(m_defaultDirection);
        }
        else
        {
            emit syncConflictDetected(m_currentLocal->lastModified(), m_syncSession->manifestLastModified());
        }
    }


    void RemoteSync::useRemoteState()
    {
        // If there isn't a remote state create one or else our pull will exit early
        if (!m_remoteState)
        {
            m_remoteState = new RemoteStateSnapshot();
        }

        startTransfer(PullFromCloud);
    }

    void RemoteSync::useLocalState()
    {
        if (m_defaultDirection == PushToCloud)
        {
            // We were already pushing
            startTransfer(PushToCloud);
        }
        else
        {
            // If this is a pull, and we're migrating cloud saves, then we don't want to push to the old storage location
            if (m_migrating)
            {
                // Finish this "pull" successfully to trigger the cloud migration push to the new cloud storage location
                m_transferSucceeded = true;
                m_syncSession->releaseServiceLock(true);
            }
            else 
            {
                // We were pulling; get a write lock
                m_syncSession->promoteLock();
            }
        }
    }

    void RemoteSync::manifestRequestFailed(RemoteSyncSession::ManifestRequestError error)
    {
        m_failed = true;

        if (error == RemoteSyncSession::ManifestFileRetrievalError)
        {
            emit failed(SyncFailure(NetworkFailure));
        }
        else if (error == RemoteSyncSession::ManifestLockedError)
        {
            emit failed(SyncFailure(ManifestLockedFailure));
        }
        else 
        {
            emit failed(SyncFailure(ServiceFailure));
        }

        m_syncSession->releaseServiceLock(false);

        ORIGIN_LOG_EVENT << "Cloud Save: Manifest request failed during sync";
    }
    
    RemoteSync::LastStateComparison RemoteSync::compareLastClientState()
    {
        bool loadedSnapshot = false;
        LastStateComparison result;
        
        // localStateForClientId will return an empty LocalStateSnapshot on failure
        // Other callers want this but we pass loadedSnapshot so we can detect if we succeeded
        LocalStateSnapshot *localState = LastSyncStore::localStateForCloudSavesId(m_entitlement, LastSyncStore::ClientState, &loadedSnapshot);

        if (!loadedSnapshot)
        {
            result = NoSnapshotFound;
        }
        else
        {
            if (ChangeSet(*localState, *m_currentLocal).isEmpty())
            {
                result = UnchangedSinceLastSnapshot;
            }
            else
            {
                result = ChangedSinceLastSnapshot;
            }
        }
        
        delete localState;
        return result;
    }

    bool RemoteSync::pullWouldConflict() const
    {
        if (!m_remoteState)
        {
            // Nothing on the remote
            return false;
        }
        
        if (m_currentLocal->isEmpty())
        {
            // No files on disk
            return false;
        }

        // Ignore lineage mismatches if we are migrating
        if (!m_migrating && m_remoteState->lineage() != m_currentLocal->lineage())
        {
            // Different lineages
            ORIGIN_LOG_DEBUG << "Detected pull conflict due to different lineages between local and remote";
            return true;
        }

        const ChangeSet remoteToLocalChanges(*m_remoteState, *m_currentLocal);

        if (!remoteToLocalChanges.isEmpty())
        {
            const ChangeSet localToPreviousSyncChanges(*m_currentLocal, *m_previousSync);

            if (!localToPreviousSyncChanges.isEmpty())
            {
                ORIGIN_LOG_DEBUG << "Detected pull conflict due to both local and remote having changes";
                debugDumpChangeSet("Delta from remote to local: ", remoteToLocalChanges);
                debugDumpChangeSet("Delta from local to previous sync: ", localToPreviousSyncChanges);

                return true;
            }
        }

        return false;
    }

    bool RemoteSync::pushWouldConflict() const
    {
        if (!m_remoteState)
        {
            return false;
        }
        
        if (m_remoteState->lineage() != m_currentLocal->lineage())
        {
            // Different lineages
            ORIGIN_LOG_DEBUG << "Detected push conflict due to different lineages between local and remote";
            return true;
        }

        // If the remote has changed in the meantime we're in trouble
        const ChangeSet previousSyncToRemoteChanges(*m_previousSync, *m_remoteState);

        if (!previousSyncToRemoteChanges.isEmpty())
        {
            ORIGIN_LOG_DEBUG << "Detected push conflict due to remote having changes";
            debugDumpChangeSet("Delta from previous sync and remote: ", previousSyncToRemoteChanges);

            return true;
        }

        return false;
    }

    QSet<QString> RemoteSync::findAddedUntrustedFiles() const
    {
        QSet<QString> untrustedPaths;

        foreach(const SyncableFile& syncable, m_currentLocal->fileHash())
        {
            foreach(const LocalInstance& instance, syncable.localInstances())
            {
                if (instance.trust() == LocalInstance::Untrusted)
                {
                    // This path is untrusted
                    untrustedPaths << instance.path();
                }
            }
        }

        // Remove any paths that already exist
        if (m_remoteState)
        {
            foreach(const SyncableFile& syncable, m_remoteState->fileHash())
            {
                foreach(const LocalInstance& instance, syncable.localInstances())
                {
                    untrustedPaths.remove(instance.path());
                }
            }
        }

        return untrustedPaths;
    }
        
    void RemoteSync::transferProgress(qint64 completedBytes, qint64 totalBytes)
    {
        // Add our prep step progress - they should technically be all done at this
        // point but we might have post-steps in the future
        float totalProgress = float(m_prepStepsCompleted) * PrepStepProgress;

        // Now the transfer progress
        // This has to be scaled down to exclude the prep steps
        const float transferProgressScale = 1.0f - (PrepStepProgress * float(TotalPrepSteps));
        totalProgress += (float(completedBytes) / float(totalBytes)) * transferProgressScale; 

        updateProgress(totalProgress, completedBytes, totalBytes);
    }

    void RemoteSync::updateProgress (float currProgress, qint64 completedBytes, qint64 totalBytes)
    {
        m_currProgress = currProgress;
        emit progress (currProgress, completedBytes, totalBytes);
    }
        
    void RemoteSync::lockPromoted()
    {
        // Start pushing now that we have a write lock
        startTransfer(PushToCloud);
    }

    void RemoteSync::lockPromotionFailed()
    {
        m_failed = true;
        emit failed(NetworkFailure);
        m_syncSession->releaseServiceLock(false);
    }
        
    void RemoteSync::serviceLockReleased()
    {
        m_failed = false;
        emit finished(offerId());
    }

    void RemoteSync::cancel()
    {
        cancel(SystemCancelledFailure);
    }

    void RemoteSync::cancel(FailureReason reason)
    {
        // if we have already failed but have not yet finished we are releasing the service lock
        // and re-emmiting a fail and trying to remove the lock again is in the best case redundant and 
        // in the worse case will cause whoever is canceling on fail to enter an infinite loop.
        if(!m_failed)
        {
            if (m_transfer)
            {
                // Start tearing down our transfer
                // Our destructor will actually wait for this
                m_transfer->exit();
            }

            m_cancelled = true;
            m_failed = true;
            emit failed(reason);

            // Release our lock
            m_syncSession->releaseServiceLock(false);
        }
    }
        
    void RemoteSync::handleDelegateResponse(const int& response)
    {
        switch(response)
        {
        case UseLocalState:
            useLocalState();
            break;
        case UseRemoteState:
            useRemoteState();
            break;
        case CancelSync:
            ORIGIN_LOG_EVENT << "Cloud Save: Delegate cancelled sync";
            cancel(DelegateCancelledFailure);
            break;
        }
    }
        
    QString RemoteSync::contentId() const
    {
        return m_entitlement->contentConfiguration()->contentId(); 
    }

    QString RemoteSync::offerId() const
    {
        return m_entitlement->contentConfiguration()->productId();
    }

    QString RemoteSync::cloudSavesId() const
    {
        return m_entitlement->contentConfiguration()->cloudSavesId();
    }    

    void RemoteSync::setMigrating(const bool& migrating)
    {
        m_migrating = migrating;
    }
}
}
}
