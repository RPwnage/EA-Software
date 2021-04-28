#include <QTimer>

#include "engine/cloudsaves/RemoteSyncManager.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"

namespace
{
    Origin::Engine::CloudSaves::RemoteSyncManager *RemoteSyncManagerInstance = NULL;
}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{

    RemoteSyncManager::RemoteSyncManager() : mCancelTimeout(NULL)
    {
    }

    RemoteSyncManager* RemoteSyncManager::instance()
    {
        if (RemoteSyncManagerInstance == NULL)
        {
            RemoteSyncManagerInstance = new RemoteSyncManager();
        }

        return RemoteSyncManagerInstance;
    }

    RemoteSync* RemoteSyncManager::remoteSyncByEntitlement(Content::EntitlementRef entitlement)
    {
        const QString offerID(entitlement->contentConfiguration()->productId());

        return mRemoteSyncInProgressHash.contains(offerID) ? mRemoteSyncInProgressHash[offerID] : NULL;
    }

    RemoteSync* RemoteSyncManager::createRemoteSync(Content::EntitlementRef entitlement, RemoteSync::TransferDirection direction)
    {
        const QString offerID(entitlement->contentConfiguration()->productId());

        if (!mRemoteSyncInProgressHash.contains(offerID))
        {
            RemoteSync* remoteSync = new RemoteSync(entitlement, direction);
            ORIGIN_VERIFY_CONNECT(remoteSync, SIGNAL(finished(const QString&)), this, SLOT(onRemoteSyncFinished(const QString&)));
            mRemoteSyncInProgressHash.insert(offerID, remoteSync);
            emit(remoteSyncCreated(remoteSync));
            return remoteSync;
        }

        return mRemoteSyncInProgressHash[offerID];
    }

    RemoteSync* RemoteSyncManager::createPullRemoteSync(Content::EntitlementRef entitlement)
    {
        return createRemoteSync(entitlement, RemoteSync::PullFromCloud);
    }

    RemoteSync* RemoteSyncManager::createPushRemoteSync(Content::EntitlementRef entitlement)
    {
        return createRemoteSync(entitlement, RemoteSync::PushToCloud);
    }

    void RemoteSyncManager::onRemoteSyncFinished(const QString& offerID)
    {
        Origin::Engine::Content::ContentController* contentController = Origin::Engine::Content::ContentController::currentUserContentController();
        if(contentController)
        {
            ORIGIN_LOG_EVENT << "Finishing sync for " <<  contentController->getDisplayNameFromId(offerID) << ", offerID: " << offerID;
        }
        else
            ORIGIN_LOG_EVENT << "Finishing sync for " << ", offerID: " << offerID;
        if(mRemoteSyncInProgressHash.contains(offerID))
        {
            RemoteSync* sync = mRemoteSyncInProgressHash[offerID];
            mRemoteSyncInProgressHash.remove(offerID);

            sync->releasedByManager();
            sync->deleteLater();
        }

        if (mRemoteSyncInProgressHash.isEmpty())
        {
            emit lastSyncFinished();
            // We cancelled on our own
            delete mCancelTimeout;
            mCancelTimeout = NULL;
        }
        if(contentController)
            ORIGIN_LOG_EVENT << "Finished: " << contentController->getDisplayNameFromId(offerID) << ", offerID: " << offerID;
        else
            ORIGIN_LOG_EVENT << "Finished: " << ", offerID: " << offerID;
    }

    void RemoteSyncManager::onCancelTimeout()
    {
        foreach(RemoteSync* remoteSync, mRemoteSyncInProgressHash)
        {
            delete remoteSync;
        }

        mRemoteSyncInProgressHash.clear();
        emit lastSyncFinished();

        mCancelTimeout->deleteLater();
        mCancelTimeout = NULL;
    }

    void RemoteSyncManager::cancelRemoteSyncs(int timeout)
    {
        if (mRemoteSyncInProgressHash.isEmpty())
        {
            emit lastSyncFinished();
            return;
        }

        foreach(RemoteSync* remoteSync, mRemoteSyncInProgressHash)
        {
            remoteSync->cancel();
        }

        if (timeout >= 0)
        {
            delete mCancelTimeout;

            mCancelTimeout = new QTimer();
            mCancelTimeout->setInterval(timeout);
            mCancelTimeout->setSingleShot(true);
            ORIGIN_VERIFY_CONNECT(mCancelTimeout, SIGNAL(timeout()), this, SLOT(onCancelTimeout()));

            mCancelTimeout->start();
        }
    }

    bool RemoteSyncManager::syncBeingPerformed()
    {
        return !mRemoteSyncInProgressHash.isEmpty();
    }
}
}
}
