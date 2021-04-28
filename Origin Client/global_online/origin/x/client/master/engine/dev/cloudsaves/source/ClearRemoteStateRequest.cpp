#include "engine/cloudsaves/ClearRemoteStateRequest.h"

#include "services/debug/DebugService.h"
#include "engine/cloudsaves/RemoteStateSnapshot.h"
#include "engine/cloudsaves/AuthorizedS3Transfer.h"
#include "engine/cloudsaves/AuthorizedS3TransferBatch.h"
#include "services/rest/CloudSyncServiceClient.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    ClearRemoteStateRequest::ClearRemoteStateRequest(Content::EntitlementRef entitlement, const bool& clearOldPath) :
        m_entitlement(entitlement)
    {
        m_syncSession = new RemoteSyncSession(entitlement, QStringList(), clearOldPath);

        ORIGIN_VERIFY_CONNECT(m_syncSession, SIGNAL(remoteStateDiscovered(Origin::Engine::CloudSaves::RemoteStateSnapshot*)), 
            this, SLOT(remoteStateDiscovered(Origin::Engine::CloudSaves::RemoteStateSnapshot*)));

        ORIGIN_VERIFY_CONNECT(m_syncSession, SIGNAL(manifestRequestFailed(Origin::Engine::CloudSaves::RemoteSyncSession::ManifestRequestError)), 
            this, SIGNAL(failed()));
    }

    ClearRemoteStateRequest::~ClearRemoteStateRequest()
    {
        m_syncSession->releaseServiceLock(false);
        delete m_syncSession;
    }

    void ClearRemoteStateRequest::start()
    {
        m_syncSession->requestSaveManifest(RemoteSyncSession::WriteLock);
    }

    void ClearRemoteStateRequest::remoteStateDiscovered(RemoteStateSnapshot *)
    {
        AuthorizedS3TransferBatch *batch = new AuthorizedS3TransferBatch(m_syncSession, this);
        batch->addTransfer(new AuthorizedS3Delete(m_syncSession->saveManifestUrl()));
        batch->start();

        ORIGIN_VERIFY_CONNECT(batch, SIGNAL(succeeded()), this, SIGNAL(succeeded()));
        ORIGIN_VERIFY_CONNECT(batch, SIGNAL(failed()), this, SIGNAL(failed()));
    }
}
}
}
