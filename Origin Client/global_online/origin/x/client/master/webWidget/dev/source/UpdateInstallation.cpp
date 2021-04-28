#include <QNetworkReply>
#include <QByteArray> 
#include <QtConcurrentRun>
#include <QUrl>

#include "UpdateInstallation.h"
#include "UpdateCache.h"

namespace WebWidget
{
    UpdateInstallation::UpdateInstallation(UpdateCache *cache, const WebWidget::UpdateIdentifier &identifier, QNetworkReply *source) :
        mCache(cache),
        mUpdateIdentifier(identifier),
        mSource(source),
        mTemporaryPackage(),
        mFinished(false)
    {
        // Open our temporary file
        mTemporaryPackage.open();

        connect(mSource, SIGNAL(finished()), this, SLOT(sourceFinished()));
        connect(mSource, SIGNAL(readyRead()), this, SLOT(sourceReadyRead()));
        
        // Notify when our unpacking is done
        connect(&mUnpackWatcher, SIGNAL(finished()), this, SLOT(unpackFinished()));

        // Catch up with the state of the source
        if (mSource->bytesAvailable() > 0)
        {
            sourceReadyRead();
        }

        if (mSource->isFinished())
        {
            // Need to queue this so callers have a chance to connect to our finished() signal
            QMetaObject::invokeMethod(this, "sourceFinished", Qt::QueuedConnection);
        }
    }

    UpdateInstallation::~UpdateInstallation()
    {
        delete mSource;
    }

    void UpdateInstallation::sourceReadyRead()
    {
        QByteArray sourceData(mSource->readAll());

        if (mTemporaryPackage.write(sourceData) != sourceData.size())
        {
            // Can't write to our temporary file
            finish(UpdateDownloadFailedLocal);
        }
    }
    
    void UpdateInstallation::sourceFinished()
    {
        // Close our temporary file. It will still exist on disk until its destructor is called
        mTemporaryPackage.close();

        if (mSource->error() != QNetworkReply::NoError)
        {
            finish(UpdateDownloadFailedRemote);
            return;
        }

        // Unpack in the background
        QString packageFileName(mTemporaryPackage.fileName());

        mUnpackWatcher.setFuture(
            QtConcurrent::run(mCache, &UpdateCache::unpackAndRegisterUpdate, packageFileName, mUpdateIdentifier));
    }
    
    void UpdateInstallation::unpackFinished()
    {
        finish(mUnpackWatcher.result());
    }
        
    void UpdateInstallation::finish(UpdateError error)
    {
        disconnect(mSource, NULL, this, NULL);
        mFinished = true;
        emit finished(error);
    }
}
