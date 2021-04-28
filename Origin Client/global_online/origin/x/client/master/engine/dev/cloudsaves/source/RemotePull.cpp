#include "engine/cloudsaves/RemotePull.h"
#include "engine/cloudsaves/ChangeSet.h"
#include "engine/cloudsaves/StagedS3DownloadBatch.h"
#include "services/debug/DebugService.h"
#include "engine/cloudsaves/LastSyncStore.h"
#include "engine/cloudsaves/RemoteUsageMonitor.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"
#include "engine/cloudsaves/PathSubstituter.h"
#include "engine/cloudsaves/FilesystemSupport.h"
#include "engine/cloudsaves/LocalStateBackup.h"

#ifdef ORIGIN_PC
#include <windows.h>
#endif

#include <QSet> 
#include <QThread>

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    RemotePull::~RemotePull()
    {
        m_backup.waitForFinished();
        delete m_changeSet;
    }

    void RemotePull::run()
    {
        // If there was no remote state there's nothing to do 
        if (!remoteSync()->remoteState())
        {
            RemoteUsageMonitor::instance()->setUsageForEntitlement(remoteSync()->entitlement(), 0);
            emit succeeded();
            return;
        }
            
        // Update our remote usage information since we just queried it for free
        RemoteUsageMonitor::instance()->setUsageForEntitlement(remoteSync()->entitlement(),
                                                               remoteSync()->remoteState()->localSize(),
                                                               remoteSync()->session()->manifestEtag());

        // Calculate our changeset
        m_changeSet = new ChangeSet(*remoteSync()->currentLocalState(), *remoteSync()->remoteState());

        if (m_changeSet->isEmpty())
        {
            // Save our last sync snapshot for two reasons:
            // 1) If there was a conflict we want to save the remote lineage as our 
            //    local even though the contents were identical to prevent future 
            //    lineage-triggered conflicts
            // 2) If files had their instance ID change while keeping their MD5 we want
            //    to avoid recalculating them in the future
            updateLastSyncFromRemote();

            // Nothing to do
            emit succeeded();
            return;
        }

        // Check for sufficient disk space
        if (!checkFreeSpace(m_changeSet))
        {
            // checkFreeSpace emits a space error
            return;
        }

        // Backup all local content
        LocalStateBackup localStateBackup(remoteSync()->entitlement(), LocalStateBackup::BackupForRollback);
        m_backup = localStateBackup.createBackup(remoteSync()->currentLocalState());

        if (!m_changeSet->added().isEmpty())
        {
            const QList<SyncableFile> &toDownload = m_changeSet->added();
            StagedS3DownloadBatch* batch = new StagedS3DownloadBatch(remoteSync()->session(), this);

            for(QList<SyncableFile>::const_iterator it = toDownload.constBegin();
                it != toDownload.constEnd();
                it++)
            {
                const SyncableFile &syncable = *it;
                QUrl remoteUrl = remoteSync()->session()->manifestRelativeUrl(syncable.remotePath());
                const QSet<LocalInstance> &localInstances = syncable.localInstances();

                if (batch->addStagedDownload(remoteUrl, localInstances, syncable.fingerprint().size()))
                {
                    ORIGIN_LOG_EVENT << "Cloud Save: File " << qPrintable(syncable.remotePath()) << " added to be pulled from the cloud";
                }
                else
                {
                    ORIGIN_ASSERT(0);
                    ORIGIN_LOG_WARNING << "Cloud Save: Unable to add staged download";
                    emit failed(SyncFailure(UnknownFailure));
                    return;
                }
            }

            batch->start();
            ORIGIN_VERIFY_CONNECT(batch, SIGNAL(succeeded()), this, SLOT(addedFilesPlaced()));
            ORIGIN_VERIFY_CONNECT(batch, SIGNAL(failed()), this, SLOT(rollbackPull()));
            ORIGIN_VERIFY_CONNECT(batch, SIGNAL(progress(qint64, qint64)), this, SIGNAL(progress(qint64, qint64)));
            ORIGIN_VERIFY_CONNECT(batch, SIGNAL(aboutToUnstage()), this, SLOT(aboutToUnstage()));
        }
        else
        {
            // Nothing to do - continue on
            aboutToUnstage();
            addedFilesPlaced();
        }

        exec();
    }

    bool RemotePull::checkFreeSpace(ChangeSet *changeSet)
    {
        const qint64 currentSize = remoteSync()->currentLocalState()->localSize();
        const qint64 targetSize = remoteSync()->remoteState()->localSize();
        const qint64 backupSize = currentSize;

        /*
         * During a pull we:
         * 1) Create a backup
         * 2) Download any new or modified files to a temporary location
         * 3) Rename the modified files over
         * 4) Delete any old files
         *
         * This means we need space for the backup, the old state and the new state.
         * The old state is already on disk and does not need to be accounted for
         * Add 10% more for filesystem overhead
         */
        const quint64 requiredBytes = (backupSize + targetSize) * 1.10;

        PathSubstituter substituter(remoteSync()->entitlement());
        
        // Start with all the files we're adding
        QList<SyncableFile> createdFiles = changeSet->added();

        // Include all of our changed paths
        const QList<LocalInstanceChange> &localInstancesChanged = changeSet->localInstancesChanged();

        for(QList<LocalInstanceChange>::ConstIterator it = localInstancesChanged.constBegin();
            it != localInstancesChanged.constEnd();
            it++)
        {
            createdFiles.append((*it).to);
        }

        // Figure out which prefixes were used    
        // This is to avoid calling GetDiskFreeSpace for every file
        // Just using the drive letters doesn't work with reparse points and UNC paths
        QSet<QString> usedPrefixes;
        for(QList<SyncableFile>::ConstIterator syncableIt = createdFiles.constBegin();
            syncableIt != createdFiles.constEnd();
            syncableIt++)
        {
            const SyncableFile &syncable = *syncableIt;

            const QSet<LocalInstance> &localInstances = syncable.localInstances();
            for(QSet<LocalInstance>::ConstIterator instanceIt = localInstances.constBegin();
                instanceIt != localInstances.constEnd();
                instanceIt++)
            {
                QString matchedPrefix;

                if (!substituter.templatizePath((*instanceIt).path(), &matchedPrefix).isNull())
                {
                    usedPrefixes << matchedPrefix;
                }
            }
        }

        for(QSet<QString>::ConstIterator it = usedPrefixes.constBegin();
            it != usedPrefixes.constEnd();
            it++)
        {
            QString dirName = *it;
            qint64 freeBytes = Services::PlatformService::GetFreeDiskSpace(dirName);

            if (freeBytes >= 0 && (quint64)freeBytes < requiredBytes)
            {
                emit failed(SyncFailure(NoSpaceFailure, requiredBytes, freeBytes, dirName));
                return false;
            }
        }

        return true;
    }

    void RemotePull::rollbackPull()
    {
        emit failed(SyncFailure(NetworkFailure));
        ORIGIN_LOG_EVENT << "Cloud Save: Rolling back pull";

        // TELEMETRY
        GetTelemetryInterface()->Metric_CLOUD_ERROR_PULL_FAILED(remoteSync()->offerId().toUtf8().data());
    }

    void RemotePull::aboutToUnstage()
    {
        // Before we touch the filesystem wait for our backup to complete
        m_backup.waitForFinished();
        
        // Telemetry
        size_t numObjDeleted(0);
        
        // Delete all files that have disappeared completely
        // We know these won't be needed as there are no remaining copies with this 
        // file fingerprint. If we do this later then its possible we'll delete paths
        // occupied by a new version
        const QList<SyncableFile> &deletedFiles = m_changeSet->deleted();
        
        for(QList<SyncableFile>::const_iterator deletedFileIt = deletedFiles.constBegin();
            deletedFileIt != deletedFiles.constEnd();
            deletedFileIt++)
        {
            const QSet<LocalInstance> &deletedInstances = (*deletedFileIt).localInstances();

            for(QSet<LocalInstance>::const_iterator deletedInstanceIt = deletedInstances.constBegin();
                deletedInstanceIt != deletedInstances.constEnd();
                deletedInstanceIt++)
            {
                const QString &deletedPath = (*deletedInstanceIt).path();

                ORIGIN_LOG_EVENT << "Cloud Save: File " << qPrintable(deletedPath) << "being deleted";

                QFile deleteFile(deletedPath);
                if (deleteFile.remove() == false)
                {
                    GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("RemotePull_aboutToUnstage1", deletedPath.toUtf8().data(), deleteFile.error());
                }
                
                // Telemetry
                ++numObjDeleted;
            }
        }
        
        // This handles renames or creation/delete when an existing copy
        // exists
        const QList<LocalInstanceChange> &localInstancesChanged = m_changeSet->localInstancesChanged();
        for(QList<LocalInstanceChange>::const_iterator it = localInstancesChanged.constBegin(); 
            it != localInstancesChanged.constEnd();
            it++)
        {
            const LocalInstanceChange &localInstanceChange = *it;

            // Use a random path in the old set as our reference file
            LocalInstance referenceInstance = localInstanceChange.from.localInstances().toList().takeFirst();
            
            QList<LocalInstance> onlyOld = (localInstanceChange.from.localInstances() - 
                                            localInstanceChange.to.localInstances()).toList();

            QList<LocalInstance> onlyNew = (localInstanceChange.to.localInstances() - 
                                            localInstanceChange.from.localInstances()).toList();

            // Do as many renames as we can
            while(!onlyNew.isEmpty() && !onlyOld.isEmpty())
            {
                LocalInstance oldInstance = onlyOld.takeFirst();
                LocalInstance newInstance = onlyNew.takeFirst();

                if (QFile(oldInstance.path()).rename(newInstance.path()))
                {
                    FilesystemSupport::setFileMetadata(newInstance);

                    // In case we just renamed the reference file just use the new file as
                    // it's guaranteed to exist
                    referenceInstance = newInstance;
                }
                else
                {
                    // Bad!
                    ORIGIN_ASSERT(false);
                }
            }

            // Copy any files only in the new state
            for(QList<LocalInstance>::const_iterator copyIt = onlyNew.constBegin();
                copyIt != onlyNew.constEnd();
                copyIt++)
            {
                QFile(referenceInstance.path()).copy((*copyIt).path());
                FilesystemSupport::setFileMetadata(*copyIt);
                ORIGIN_LOG_EVENT << "Cloud Save: File " << qPrintable(referenceInstance.path()) << " being copied";
            }

            // Delete any old filenames
            for(QList<LocalInstance>::const_iterator removeIt = onlyOld.constBegin();
                removeIt != onlyOld.constEnd();
                removeIt++)
            {
                ORIGIN_LOG_EVENT << "Cloud Save: File " << qPrintable(referenceInstance.path()) << " being removed";

                QFile deleteFile((*removeIt).path());
                if (deleteFile.remove() == false)
                {
                    GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("RemotePull_aboutToUnstage2", (*removeIt).path().toUtf8().data(), deleteFile.error());
                }

                // Telemetry
                ++numObjDeleted;
            }
        }


        // TELEMETRY
        if (numObjDeleted > 0)
            GetTelemetryInterface()->Metric_CLOUD_ACTION_OBJECT_DELETION(remoteSync()->offerId().toUtf8().data(), numObjDeleted);
    }

    void RemotePull::addedFilesPlaced()
    {
        updateLastSyncFromRemote();
        emit succeeded();
    }
        
    void RemotePull::updateLastSyncFromRemote()
    {
        // Update our local state
        LocalStateSnapshot *newLocalState = new LocalStateSnapshot(*remoteSync()->remoteState());
        
        // Commit the last sync store disk
        LastSyncStore::setLastSyncForCloudSavesId(remoteSync()->cloudSavesId(), *newLocalState);

        delete newLocalState;

        // TELEMETRY
        GetTelemetryInterface()->Metric_CLOUD_ACTION_GAME_NEW_CONTENT_DOWNLOADED(remoteSync()->offerId().toUtf8().data());
    }
}
}
}
