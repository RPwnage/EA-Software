#ifndef _CLOUDSAVES_STAGEDS3DOWNLOADBATCH
#define _CLOUDSAVES_STAGEDS3DOWNLOADBATCH

#include "LocalInstance.h"

#include <QHash>
#include <QList>
#include <QTemporaryFile>
#include <QString>
#include <QSet>
#include <QUrl>
#include <QByteArray>

#include "AuthorizedS3Transfer.h"
#include "AuthorizedS3TransferBatch.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    class ORIGIN_PLUGIN_API StagedS3DownloadBatch : public AuthorizedS3TransferBatch
    {
        Q_OBJECT

    public:
        StagedS3DownloadBatch(RemoteSyncSession *session, QObject *parent) : AuthorizedS3TransferBatch(session, parent)
        {
        }

        ~StagedS3DownloadBatch();

        bool addStagedDownload(const QUrl &remoteUrl, const QSet<LocalInstance> &localInstances, qint64 expectedSize);

    signals:
        void aboutToUnstage();

    protected:
        virtual void batchSucceeded();
        virtual void batchFailed();

        void rollback();

    private:
        struct PendingRename
        {
            QTemporaryFile *tempFile;
            LocalInstance primaryDestInstance;
            QList<LocalInstance> alternateDestInstances;
        };

        QSet<QString> m_createdDirectories;
        QList<PendingRename> m_pendingRenames;    
    };
}
}
}

#endif
