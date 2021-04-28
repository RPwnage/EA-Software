#ifndef _CLOUDSAVES_REMOTEPULL
#define _CLOUDSAVES_REMOTEPULL

#include "LocalInstance.h"
#include "AbstractStateTransfer.h"
#include "RemoteSync.h"

#include "services/plugin/PluginAPI.h"

#include <QObject>
#include <QString>
#include <QSet>
#include <QFuture>

namespace Origin
{
namespace Engine
{
namespace CloudSaves 
{
    class RemoteSync;
    class BackupRestore;

    class ORIGIN_PLUGIN_API RemotePull : public AbstractStateTransfer 
    {
        Q_OBJECT

    public:
        RemotePull(RemoteSync *sync) : AbstractStateTransfer(sync), m_finalState(NULL), m_changeSet(NULL)
        {
        }

        ~RemotePull();

    signals:
        void succeeded();
        void failed(Origin::Engine::CloudSaves::SyncFailure);
        void progress(qint64 completedBytes, qint64 totalBytes);

    private slots:
        void addedFilesPlaced();
        void rollbackPull();
        void aboutToUnstage();

    private:
        void run();

        void updateLastSyncFromRemote();
        bool checkFreeSpace(ChangeSet *changeSet);

        RemoteStateSnapshot *m_finalState;
        ChangeSet *m_changeSet;
        QSet<QString> m_filesToKeep;

        QFuture<bool> m_backup;
    };
}
}
}

#endif
