#ifndef _CLOUDSAVES_CLOUDSAVEMGR_H
#define _CLOUDSAVES_CLOUDSAVEMGR_H

#include <QObject>
#include <QHash>

#include "RemoteSync.h"

#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

class QTimer;

namespace Origin
{
namespace Engine
{
namespace CloudSaves 
{
    class LocalStateSnapshot;

    class ORIGIN_PLUGIN_API RemoteSyncManager : public QObject
    {
        Q_OBJECT

        public:
            typedef QHash <QString, RemoteSync*> RemoteSyncList;

            RemoteSyncManager();

            static RemoteSyncManager *instance();

            RemoteSync* remoteSyncByEntitlement(Content::EntitlementRef Entitlement);

            RemoteSync* createPullRemoteSync(Content::EntitlementRef entitlement);
            RemoteSync* createPushRemoteSync(Content::EntitlementRef entitlement);

            RemoteSyncList *remoteSyncInProgressList () { return &mRemoteSyncInProgressHash; }

            void cancelRemoteSyncs(int timeout = -1);
            bool syncBeingPerformed();

        signals:
            void lastSyncFinished();
            void remoteSyncCreated(Origin::Engine::CloudSaves::RemoteSync*);

        private slots:
            void onRemoteSyncFinished(const QString&);
            void onCancelTimeout();

        private:
            RemoteSync* createRemoteSync(Content::EntitlementRef entitlement, RemoteSync::TransferDirection direction);

            RemoteSyncList mRemoteSyncInProgressHash;

            QTimer *mCancelTimeout;
    };
}
}
}

#endif // Header include guard
