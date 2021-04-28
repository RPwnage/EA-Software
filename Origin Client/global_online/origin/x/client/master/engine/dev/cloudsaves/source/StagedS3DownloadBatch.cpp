#include <QDir>

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "engine/cloudsaves/StagedS3DownloadBatch.h"
#include "engine/cloudsaves/FilesystemSupport.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    static QString containingPathForFile(const QString &filePath)
    {
        QStringList filePathParts = filePath.split('/');
        filePathParts.takeLast();
        return filePathParts.join("/");
    }

    StagedS3DownloadBatch::~StagedS3DownloadBatch()
    {
        rollback();
    }
    
    void StagedS3DownloadBatch::rollback()
    {
        // Make sure all of our operations are deleted so we can clear out the temporary files
        shutdownActiveOperations();

        while(!m_pendingRenames.isEmpty())
        {
            delete m_pendingRenames.takeFirst().tempFile;
        }
    }

    bool StagedS3DownloadBatch::addStagedDownload(const QUrl &remoteUrl, const QSet<LocalInstance> &localInstances, qint64 expectedSize)
    {
        QList<LocalInstance> localInstanceList = localInstances.toList();
        PendingRename pending;

        // Create all of our containing directories
        // Note we don't try to roll this back
        for(QSet<LocalInstance>::const_iterator it = localInstances.constBegin();
            it != localInstances.constEnd();
            it++)
        {
            QString containingPath = containingPathForFile((*it).path());
            
            if (!m_createdDirectories.contains(containingPath))
            {
                QDir("/").mkpath(containingPath);
                m_createdDirectories << containingPath;
            }
        }


        // Try to place this in the same dir as one of the files
        QString firstLocalFile = localInstances.toList().takeFirst().path();
        QString pathTemplate = containingPathForFile(firstLocalFile) + "/cloudsavesXXXXXX.tmp";

        pending.tempFile = new QTemporaryFile(pathTemplate);
        pending.primaryDestInstance = localInstanceList.takeFirst();
        pending.alternateDestInstances = localInstanceList;

        // Open our file
        if (!pending.tempFile->open())
        {
            ORIGIN_LOG_WARNING << "Cloud Saves: Unable to open temporary file";

            delete pending.tempFile;
            return false;
        }

        // Create our transfer
        AuthorizedS3Download *download = new AuthorizedS3Download(remoteUrl, pending.tempFile->fileName(), expectedSize);
        addTransfer(download);

        m_pendingRenames.append(pending);

        return true;
    }

    void StagedS3DownloadBatch::batchSucceeded()
    {
        // StagedS3DownloadBatch object should live in the same thread where this signal is connected to
        emit(aboutToUnstage());

        // Rename everything in to place
        while(!m_pendingRenames.isEmpty())
        {
            const PendingRename &pending = m_pendingRenames.takeFirst();    

            // We can only rename the first file
            QFile::remove(pending.primaryDestInstance.path());
            if (!pending.tempFile->rename(pending.primaryDestInstance.path()))
            {
                ORIGIN_LOG_ERROR << "Cloud Saves: Couldn't rename to " << pending.primaryDestInstance.path();
                delete pending.tempFile;
                batchFailed();
                return;
            }

            // Take our security from our new parent directory and set our file
            // timestamps
            FilesystemSupport::setFileMetadata(pending.primaryDestInstance);

            // Copy the duplicates
            for(QList<LocalInstance>::const_iterator it = pending.alternateDestInstances.constBegin();
                it != pending.alternateDestInstances.constEnd();
                it++)
            {
                const LocalInstance &alternateInstance = *it;

                QFile::remove(alternateInstance.path());
                QFile::copy(pending.primaryDestInstance.path(), alternateInstance.path());

                // Reset the permissions on the destination in case its in
                // a different dir
                FilesystemSupport::setFileMetadata(alternateInstance);
            }
        }

        AuthorizedS3TransferBatch::batchSucceeded();
    }
    
    void StagedS3DownloadBatch::batchFailed()
    {
        rollback();
        AuthorizedS3TransferBatch::batchFailed();
    }
}
}
}
