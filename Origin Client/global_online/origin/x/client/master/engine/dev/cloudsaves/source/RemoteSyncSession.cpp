#include <limits>
#include <QDateTime>
#include <QTimer>
#include <QDomDocument>
#include <QMutexLocker>

#include "TelemetryAPIDLL.h"

#include "services/debug/DebugService.h"
#include "services/rest/CloudSyncServiceClient.h"
#include "services/rest/AvatarServiceClient.h"
#include "services/network/NetworkAccessManager.h"
#include "services/settings/SettingsManager.h"
#include "services/log/LogService.h"
#include "engine/login/LoginController.h"

#include "engine/cloudsaves/RemoteSyncSession.h"
#include "engine/cloudsaves/RemoteStateSnapshot.h"
#include "engine/cloudsaves/PathSubstituter.h"

using namespace Origin::Services;

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    const unsigned int LockRefreshLeadMilliseconds = 30000;

    RemoteSyncSession::RemoteSyncSession(Content::EntitlementRef entitlement, const QStringList &fallbackIds, const bool& clearOldStoragePath /* = false */) :
        mEntitlement(entitlement),
        mFallbackIds(fallbackIds),
        mManifestRequestReply(NULL),
        m_lockType(ReadLock),
        m_isProcessingFallbackID(false),
        m_shouldCheckOldPath(true),
        m_isCheckingOldPath(false),
        m_clearingOldPath(clearOldStoragePath)
    {
    }

    RemoteSyncSession::~RemoteSyncSession()
    {
        if (mManifestRequestReply)
        {
            ORIGIN_VERIFY_DISCONNECT(mManifestRequestReply, SIGNAL(finished()), this, SLOT(manifestRequestFinished()));
            mManifestRequestReply->deleteLater();
            mManifestRequestReply = NULL;
        }
    }
    
    void RemoteSyncSession::requestSaveManifest(ServiceLockType lockType)
    {
        // Request the primary manifest, unless we are clearing the old storage path
        if (m_clearingOldPath)
        {
            m_isCheckingOldPath = true;

            // The old cloud save location is based on contentID
            requestSaveManifest(lockType, mEntitlement->contentConfiguration()->contentId());
        }
        else
        {
            requestSaveManifest(lockType, mEntitlement->contentConfiguration()->cloudSavesId());
        }
    }

    void RemoteSyncSession::requestSaveManifest(ServiceLockType lockType, const QString &cloudSavesId)
    {
        m_lockType = lockType;

        if (lockType == WriteLock || m_isCheckingOldPath)
        {
            // We only fall back to the old storage path during cloud pull.
            // And if we're currently checking the old path, turn this flag off to avoid re-checking.
            m_shouldCheckOldPath = false;
        }

        // Reset our remote state
        mSaveManifestnifestEtag = QByteArray();
        mRemoteLastModified = QDateTime();
        mSaveManifest = QUrl();
        
        // Discover our manifest
        Origin::Services::CloudSyncServiceClient::ManifestLockType manifestLockType;

        ORIGIN_ASSERT(lockType == ReadLock || lockType == WriteLock);
        
        if (lockType == ReadLock)
        {
            manifestLockType = Origin::Services::CloudSyncServiceClient::ReadLock;
        }
        else //if (lockType == WriteLock)
        {
            manifestLockType = Origin::Services::CloudSyncServiceClient::WriteLock;
        }

        Origin::Services::CloudSyncSyncInitiationResponse *resp = Origin::Services::CloudSyncServiceClient::initiateSync(Engine::LoginController::instance()->currentUser()->getSession(), cloudSavesId, manifestLockType);
        // Time out after 15 seconds
        QTimer::singleShot(15 * 1000, resp, SLOT(abort()));

        resp->setDeleteOnFinish(true);

        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(syncInitiationSucceeded()));
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(CloudSyncManifestDiscoveryResponse::ManifestDiscoveryError)), this, SLOT(syncInitiationFailed()));
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(serviceLockReceived()));
    }

    void RemoteSyncSession::refreshServiceLock()
    {
        QMutexLocker locker(&mSyncServiceLockMutex);

        if (!mSyncServiceLock.isValid())
        {
            return;
        }

        Origin::Services::CloudSyncLockExtensionResponse *resp = Origin::Services::CloudSyncServiceClient::extendLock(Engine::LoginController::instance()->currentUser()->getSession(), mSyncServiceLock);
        resp->setDeleteOnFinish(true);

        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(serviceLockReceived()));
    }

    void RemoteSyncSession::serviceLockReceived()
    {
        QMutexLocker locker(&mSyncServiceLockMutex);

        Origin::Services::CloudSyncLockingResponse *lockingResponse = dynamic_cast<Origin::Services::CloudSyncLockingResponse*>(sender());
        ORIGIN_ASSERT(lockingResponse);

        mSyncServiceLock = lockingResponse->lock();

        // Give the lock refresh some lead time
        const unsigned int refreshMilliseconds = (mSyncServiceLock.timeToLiveSeconds * 1000) - LockRefreshLeadMilliseconds;
        QTimer::singleShot(refreshMilliseconds, this, SLOT(refreshServiceLock()));
    }
    
    void RemoteSyncSession::syncInitiationFailed()
    {
        Origin::Services::CloudSyncSyncInitiationResponse *resp = dynamic_cast<Origin::Services::CloudSyncSyncInitiationResponse*>(sender());
        ORIGIN_ASSERT(resp);

        if (resp->error() == Origin::Services::CloudSyncManifestDiscoveryResponse::ManifestLockedError)
        {
            emit manifestRequestFailed(ManifestLockedError);
        }
        else
        {
            emit manifestRequestFailed(SyncServiceError);
        }
         
        //we failed on the URL/lock request lets clear out the fallback content ids so it doesn't try.
        // Also, no need to check the old storage location.
        mFallbackIds.clear();
        m_shouldCheckOldPath = false;

        GetTelemetryInterface()->Metric_CLOUD_ERROR_SYNC_LOCKING_UNSUCCESFUL(mEntitlement->contentConfiguration()->productId().toUtf8().data());
    }

    void RemoteSyncSession::syncInitiationSucceeded()
    {
        Origin::Services::CloudSyncSyncInitiationResponse *resp = dynamic_cast<Origin::Services::CloudSyncSyncInitiationResponse*>(sender());

        ORIGIN_ASSERT(resp);

        // Make a request for the manifest URL
        QNetworkRequest req(resp->manifestUrl());
        // We need to do an end-to-end reload to get the server timestamp
        req.setRawHeader("Cache-Control", "no-cache");
        // We expect XML
        req.setRawHeader("Accept", "text/xml, application/xml");

        mSignatureRoot = resp->signatureRoot();
        mManifestRequestReply = Origin::Services::NetworkAccessManager::threadDefaultInstance()->get(req);
        ORIGIN_VERIFY_CONNECT(mManifestRequestReply, SIGNAL(finished()), this, SLOT(manifestRequestFinished()));
    }
    
    QUrl RemoteSyncSession::manifestRelativeUrl(const QString &path) const
    {
        return mSaveManifest.resolved(QUrl(path));
    }
        
    QString RemoteSyncSession::signedResourcePath(const QUrl &url) const
    {
        QString signRootString = mSignatureRoot.toString();
        QString urlString = url.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
        ORIGIN_ASSERT(urlString.startsWith(signRootString));

        return urlString.mid(signRootString.length());
    }
        
    void RemoteSyncSession::releaseServiceLock(bool success)
    {
        using namespace Origin::Engine;
        QMutexLocker locker(&mSyncServiceLockMutex);

        if (mSyncServiceLock.isValid() &&
            !LoginController::instance()->currentUser().isNull() &&
            Origin::Services::Connection::ConnectionStatesService::isUserOnline (LoginController::instance()->currentUser()->getSession()))
        {
            Origin::Services::CloudSyncServiceClient::OperationResult result = success ? Origin::Services::CloudSyncServiceClient::SuccessfulResult : Origin::Services::CloudSyncServiceClient::FailedResult;
            Origin::Services::CloudSyncUnlockResponse *resp = Origin::Services::CloudSyncServiceClient::unlock(Engine::LoginController::instance()->currentUser()->getSession(), mSyncServiceLock, result);

            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(processFallbackIds()));

            // We don't care about the result
            resp->setDeleteOnFinish(true);

            mSyncServiceLock = Origin::Services::CloudSyncServiceLock();
        }
        else
        {
            //If there is no valid sync lock, we are done at this point so mark as service lock released in order to finish the sync operation
            processFallbackIds();
        }
    }

    void RemoteSyncSession::manifestRequestFinished()
    {
        QNetworkReply *reply = dynamic_cast<QNetworkReply*>(sender());

        ORIGIN_ASSERT(reply);
        ORIGIN_ASSERT(reply == mManifestRequestReply);

        if (reply != mManifestRequestReply)
        {
            if (mManifestRequestReply)
                ORIGIN_LOG_ERROR << "reply != mManifestRequestReply";
            else
                ORIGIN_LOG_ERROR << "reply != mManifestRequestReply : mManifestRequestReply is NULL";
            reply->deleteLater();
            return;	// exit out as an unexpected reply may cause a crash
        }
        else
        {
            ORIGIN_VERIFY_DISCONNECT(mManifestRequestReply, SIGNAL(finished()), this, SLOT(manifestRequestFinished()));
            mManifestRequestReply = NULL;
        }

        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) 
        {
            QDomDocument replyDoc;

            if (!replyDoc.setContent(reply))
            {
                // Invalid XML 
                mFallbackIds.clear();
                m_shouldCheckOldPath = false;
                manifestRequestFailed(ManifestFileRetrievalError);
                return;
            }

            PathSubstituter substituter(mEntitlement);
            RemoteStateSnapshot *remoteState = new RemoteStateSnapshot(replyDoc, substituter);

            if (!remoteState->isValid())
            {
                // Didn't like the content for some reason
                mFallbackIds.clear();
                m_shouldCheckOldPath = false;
                manifestRequestFailed(ManifestFileRetrievalError);
                return;
            }

            // Remember some facts about this manifest for conflict and concurrent
            // upload detection
            mRemoteLastModified = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime();
            mSaveManifestnifestEtag = reply->rawHeader("ETag");
            mSaveManifest = reply->url();

            //we got a save with content so don't try the fallbacks, or the old storage path
            mFallbackIds.clear();
            m_shouldCheckOldPath = false;

            if (m_isCheckingOldPath)
            {
                // We found cloud saves in the old storage path.  We need to migrate them.
                emit migrating(true);
            }

            emit remoteStateDiscovered(remoteState);
        }
        else if ((reply->error() == QNetworkReply::ContentNotFoundError) ||
            (reply->error() == QNetworkReply::ContentOperationNotPermittedError))
        {
            // Success, no save manifest
            // Still record our location so we know where the to upload our
            // save files and manifest if we were 3xx'ed here
            mSaveManifest = reply->url();

            if( m_lockType == ReadLock && hasFallbackIds() )
            {
                //release the service lock.  This will cause us to process the fallback Ids. 
                m_isProcessingFallbackID = true;
                releaseServiceLock();
            }
            else if( m_lockType == ReadLock && m_shouldCheckOldPath )
            {
                // releasing the service lock will lead to checking the old storage path.
                releaseServiceLock();
            }
            else
            {
                emit remoteStateDiscovered(NULL);
            }
        }
        else 
        {
            //we errored so don't try the fallbacks, or the old storage path
            mFallbackIds.clear();
            m_shouldCheckOldPath = false;

            // Unexpected error
            emit manifestRequestFailed(ManifestFileRetrievalError);
        }
    }

    void RemoteSyncSession::promoteLock()
    {
        QMutexLocker locker(&mSyncServiceLockMutex);

        Origin::Services::CloudSyncLockPromotionResponse *resp = Origin::Services::CloudSyncServiceClient::promoteLock(Engine::LoginController::instance()->currentUser()->getSession(), mSyncServiceLock);

        resp->setDeleteOnFinish(true);
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(lockPromotedInternal()));
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error()), this, SIGNAL(lockPromotionFailed()));
    }
        
    Origin::Services::CloudSyncServiceLock RemoteSyncSession::syncServiceLock() const
    {
        QMutexLocker locker(&mSyncServiceLockMutex);
        return mSyncServiceLock;
    }

    void RemoteSyncSession::processFallbackIds()
    {
        bool isProcessingFallbackIds = false;

        if( m_isProcessingFallbackID )
        {
             isProcessingFallbackIds = requestNextFallbackManifest();
        }

        m_isProcessingFallbackID = false;

        if(!isProcessingFallbackIds)
        {
            if (m_shouldCheckOldPath)
            {
                m_isCheckingOldPath = true;

                // The old cloud save location is the contentId
                requestSaveManifest(RemoteSyncSession::ReadLock, mEntitlement->contentConfiguration()->contentId());
            }
            else
            {
                //Only emit serviceLockReleased if we aren't processing fallback ids, or checking the old storage path
                emit serviceLockReleased();
            }
        }
    }

    void RemoteSyncSession::lockPromotedInternal() 
    {
        m_lockType = WriteLock;
        emit lockPromoted();
    }

    bool RemoteSyncSession::requestNextFallbackManifest()
    {
        if(!mFallbackIds.isEmpty())
        {
            QString nextFallbackId = mFallbackIds.takeFirst();
            requestSaveManifest(RemoteSyncSession::ReadLock, nextFallbackId);

            return true;
        }

        return false;
    }

}
}
}
