#include <limits>
#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QDataStream>
#include <QDir>
#include <QtConcurrentRun>

#include "engine/cloudsaves/LocalStateBackup.h"
#include "TelemetryAPIDLL.h"

#include "engine/cloudsaves/SaveFileCrawler.h"
#include "engine/cloudsaves/FilesystemSupport.h"
#include "engine/cloudsaves/LastSyncStore.h"
#include "engine/cloudsaves/ChangeSet.h"
#include "engine/cloudsaves/PathSubstituter.h"

#include "engine/content/Entitlement.h"
#include "engine/content/CloudContent.h"
#include "engine/content/LocalContent.h"

namespace
{
    using Origin::Engine::Content::EntitlementRef;
    using namespace Origin::Engine::CloudSaves;

    // Random magic number
    const quint32 BackupMagicNumber = 0x0d7654c6;
    // We can only read or write our current backup version
    const quint32 BackupCurrentVersion = 3;
    // The Qt data stream format we use
    const int BackupDataStreamVersion = QDataStream::Qt_4_7;
    
    bool readBackupSignature(QDataStream &backupStream)
    {
        // Do some basic sanity checks
        quint32 magicNumber = 0;
        quint32 backupVersion = 0;
        backupStream >> magicNumber;
        backupStream >> backupVersion;

        if ((backupStream.status() != QDataStream::Ok) ||
            (magicNumber != BackupMagicNumber) ||
            (backupVersion != BackupCurrentVersion))
        {
            // Something is wrong 
            return false;
        }

        return true;
    }

    bool restoreBackupStream(QDataStream &backupStream)
    {
        // The beginning of this file should be a local state snapshot:w
        LocalStateSnapshot restoredState;

        if (!restoredState.load(backupStream))
        {
            return false;
        }

        // Sort the syncable files in to an known order
        QList<FileFingerprint> fingerprints = restoredState.fileHash().keys();
        qSort(fingerprints);
        
        for(QList<FileFingerprint>::const_iterator fingerIt = fingerprints.constBegin();
            fingerIt != fingerprints.constEnd();
            fingerIt++)
        {
            const SyncableFile &syncable = restoredState.fileHash()[*fingerIt];
            const QSet<LocalInstance> &localInstances = syncable.localInstances();

            // Read in the data for this file.  Place it into a QTempFile(OFM-7717) rather than a QByteArray
            // in order to handle very large files.
            QTemporaryFile syncableDataFile;
            syncableDataFile.open();

            quint32 numBytes;
            backupStream >> numBytes;
            
            // Manual buffer into temp file.  The files were added to the backup archive as QByteArrays, serialized by QDataStream.
            // The size of the QByteArray(save file) prepends the data.  0xffffffff is a sentinel value indicating an empty/uninitialized QByteArray.
            if (numBytes != 0xffffffff && numBytes != 0)
            {
                const quint32 blockSize = 1024*1024;
                char *buffer = new char[blockSize];

                while (numBytes > 0)
                {
                    bool endOfFile = numBytes <= blockSize;
                    quint32 bytesToRead = endOfFile? numBytes: blockSize;
                    int bytesRead = backupStream.readRawData(buffer, bytesToRead);
                    numBytes -= bytesRead;
                    syncableDataFile.write(buffer, bytesRead);
                }

                delete[] buffer;
                buffer = nullptr;
            }

            if (backupStream.status() != QDataStream::Ok)
            {
                return false;
            }

            if (syncableDataFile.size() == 0 && (fingerIt->size() > 0))
            {
                // We weren't able to save this for some reason while backing up:w
                continue;
            }

            syncableDataFile.close();

            // Write our to each of the instances
            foreach(const LocalInstance &instance, localInstances)
            {
                // Create the containing directory and Write the file to the target folder.
                QFileInfo targetFileName(instance.path());
                QDir("/").mkpath(targetFileName.absolutePath());

                // Write our the data
                syncableDataFile.open();
                syncableDataFile.copy(instance.path());

                // Reset our file properties
                FilesystemSupport::setFileMetadata(instance);
            }
        }

        return true;
    }

    bool restoreBackupFile(QString backupPath, EntitlementRef entitlement)
    {
        QFile backupFile(backupPath);
        if (!backupFile.open(QIODevice::ReadOnly))
        {
            qDebug("Failed to open backup file %s in read mode", qPrintable(backupPath));
            return false;
        }

        QDataStream backupStream(&backupFile);

        if (!readBackupSignature(backupStream))
        {
            return false;
        }

        // We need these to help crawl save files
        PathSubstituter substituter(entitlement);

        QList<EligibleFile> eligibleFiles = SaveFileCrawler::findEligibleFiles(entitlement, substituter);

        for(QList<EligibleFile>::const_iterator it = eligibleFiles.constBegin();
            it != eligibleFiles.constEnd();
            ++it)
        {
            const QFileInfo &info = (*it).info();
            QFile::remove(info.absoluteFilePath());
        }

        if (!restoreBackupStream(backupStream))
        {
            return false;
        }
            
        // We restored from the cloud
        const QString offerId(entitlement->contentConfiguration()->productId());
        GetTelemetryInterface()->Metric_CLOUD_AUTOMATIC_RECOVERY_SUCCESS(offerId.toUtf8().data());

        return true;
    }
    
    bool performBackup(EntitlementRef entitlement, const LocalStateSnapshot *currentState, const QString &backupPath)
    {
        QFile backupFile(backupPath);

        if (!backupFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            qDebug("Failed to open backup file %s in write mode", qPrintable(backupPath));
            return false;
        }

        // Remove the version 2 backup files
        QFile::remove(backupPath + "upmeta");
        QFile::remove(backupPath + "up");

        QDataStream backupStream(&backupFile);
        backupStream.setVersion(BackupDataStreamVersion);
        backupStream << BackupMagicNumber;
        backupStream << BackupCurrentVersion;
        
        // Embed the state this represents
        currentState->save(backupStream);
        
        // Sort the syncable files in to an known order
        QList<FileFingerprint> fingerprints = currentState->fileHash().keys();
        qSort(fingerprints);

        for(QList<FileFingerprint>::const_iterator fingerIt = fingerprints.constBegin();
            fingerIt != fingerprints.constEnd();
            fingerIt++)
        {
            const SyncableFile &syncable = currentState->fileHash()[*fingerIt];
            const QSet<LocalInstance> &localInstances = syncable.localInstances();
            
            // Read in one of the instances
            QString referencePath = localInstances.toList().first().path();
            QFile file(referencePath);
            
            if (!file.open(QIODevice::ReadOnly))
            {
                // Include a placeholder bytearray so our ordering is preserved
                backupStream << QByteArray();
            }
            else
            {
                // Write out the data 
                backupStream << file.readAll();
            }
        }

        return true;
    }
}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    LocalStateBackup::LocalStateBackup(Content::EntitlementRef entitlement, BackupReason reason) :
        mEntitlement(entitlement)
    {
        mBackupPath = backupFileName(entitlement, reason);
    }
    
    QDateTime LocalStateBackup::created() const
    {
        QFileInfo localPathInfo(mBackupPath);
        return localPathInfo.lastModified();
    }

    bool LocalStateBackup::isValid() const
    {
        return QFile::exists(mBackupPath);
    }
        
    LocalStateSnapshot *LocalStateBackup::stateSnapshot() const
    {
        if (mStateSnapshot.isNull())
        {
            QFile backupFile(mBackupPath);

            if (!backupFile.open(QIODevice::ReadOnly))
            {
                // Can't load the backup file
                return NULL;
            }

            QDataStream backupStream(&backupFile);

            if (!readBackupSignature(backupStream))
            {
                // Bad signature
                return NULL;
            }

            LocalStateSnapshot *snapshot = new LocalStateSnapshot;

            if (!snapshot->load(backupStream))
            {
                // Can't load the state snapshot
                delete snapshot;
                return NULL;
            }

            mStateSnapshot = QSharedPointer<LocalStateSnapshot>(snapshot);
        }
        
        return mStateSnapshot.data();
    }

    QString LocalStateBackup::backupFileName(EntitlementRef entitlement, LocalStateBackup::BackupReason reason)
    {
        using namespace FilesystemSupport;

        const QString fileName(QString("%1.back").arg(entitlement->contentConfiguration()->cloudSavesId()));
        QDir cloudSavesRoot = cloudSavesDirectory(LocalRoot);
        return cloudSavesRoot.absoluteFilePath(fileName);
    }

    QFuture<bool> LocalStateBackup::restore()
    {
        return QtConcurrent::run(restoreBackupFile, mBackupPath, mEntitlement);
    }
        
    QFuture<bool> LocalStateBackup::createBackup(const LocalStateSnapshot *currentState)
    {
        return QtConcurrent::run(performBackup, mEntitlement, currentState, mBackupPath);
    }
        
    LocalStateBackup LocalStateBackup::lastBackupForEntitlement(Content::EntitlementRef entitlement)
    {
        return LocalStateBackup(entitlement, BackupForRollback);
    }
} // namespace CloudSaves
} // namespace Engine
} // namespace Origin
