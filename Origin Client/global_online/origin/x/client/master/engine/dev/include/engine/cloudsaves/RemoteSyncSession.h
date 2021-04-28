#ifndef _CLOUDSAVES_REMOTESYNCSESSION_H
#define _CLOUDSAVES_REMOTESYNCSESSION_H

#include <limits>
#include <QDateTime>
#include <QObject>
#include <QByteArray>
#include <QUrl> 
#include <QString>
#include <QMutex>

#include "services/rest/CloudSyncServiceResponses.h"
#include "services/plugin/PluginAPI.h"
#include "engine/login/LoginController.h"
#include "engine/content/Entitlement.h"

namespace Origin
{
    namespace Services
    {
        class CloudSyncServiceClient;
    }
}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    class RemoteStateSnapshot;

    class ORIGIN_PLUGIN_API RemoteSyncSession : public QObject
    {
        Q_OBJECT

    public:
        enum ManifestRequestError
        {
            ManifestLockedError,
            SyncServiceError,
            ManifestFileRetrievalError
        };
        
        enum ServiceLockType
        {
            ReadLock,
            WriteLock
        };

        RemoteSyncSession(Content::EntitlementRef entitlement, const QStringList &fallbackIds, const bool& clearOldStoragePath = false);
        virtual ~RemoteSyncSession();
        
        const QUrl &saveManifestUrl() const
        {
            return mSaveManifest;
        }

        QUrl manifestRelativeUrl(const QString &path) const;
        QUrl signatureRoot() const { return mSignatureRoot; }
        QString signedResourcePath(const QUrl &) const;
        
        QDateTime manifestLastModified() const { return mRemoteLastModified; }
        const QByteArray &manifestEtag() const { return mSaveManifestnifestEtag; }

        bool hasFallbackIds() const { return !mFallbackIds.isEmpty(); }

        Origin::Services::CloudSyncServiceLock syncServiceLock() const;

    signals:
        void lockPromoted();
        void lockPromotionFailed();
        void serviceLockReleased();
        void remoteStateDiscovered(Origin::Engine::CloudSaves::RemoteStateSnapshot *remote);
        void manifestRequestFailed(Origin::Engine::CloudSaves::RemoteSyncSession::ManifestRequestError);
        void migrating(const bool&);

    public slots:
        void promoteLock();
        void releaseServiceLock(bool success = true);
        void requestSaveManifest(ServiceLockType lockType);
    
    private slots:
        void syncInitiationFailed();
        void syncInitiationSucceeded();
        void manifestRequestFinished();

        void serviceLockReceived();
        void refreshServiceLock();
        void processFallbackIds();
        void lockPromotedInternal();

    private:
        void requestSaveManifest(ServiceLockType lockType, const QString &cloudSavesId);
        bool requestNextFallbackManifest();

        // Our current lock with the sync service
        Origin::Services::CloudSyncServiceLock mSyncServiceLock;
        mutable QMutex mSyncServiceLockMutex;

        // This is path the auth service implicitly roots everything to
        QUrl mSignatureRoot;

        // Information about the remote state
        QDateTime mRemoteLastModified;
        QByteArray mSaveManifestnifestEtag;
        
        // This is the final location of the save manifest
        QUrl mSaveManifest;

        Content::EntitlementRef mEntitlement;
        QStringList mFallbackIds;

        // pointer to network reply for the manifest - must manage it to prevent a late reply from crashing cloud saves if the session is deleted
        QNetworkReply *mManifestRequestReply;

        ServiceLockType m_lockType;

        bool m_isProcessingFallbackID;
        
        // On cloud pull, if we do not find a save manifest in the new(masterTitleID_multiPlayerID) cloud server path, or using the fallback ID's, should we check 
        // to see if the saves need to be migrated from the old(contentID) path.  If we find saves there, we will migrate them.
        bool m_shouldCheckOldPath;

        bool m_isCheckingOldPath;
        
        // Is this session being created for the purpose of clearing out the old cloud storage path?
        bool m_clearingOldPath;
    };
}
}
}

#endif
