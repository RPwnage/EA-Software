#include "engine/cloudsaves/UsageQuery.h"
#include "services/debug/DebugService.h"
#include "engine/cloudsaves/RemoteStateSnapshot.h"
#include "services/rest/CloudSyncServiceClient.h"
#include "services/settings/SettingsManager.h"
#include "services/network/NetworkAccessManager.h"
#include "engine/cloudsaves/PathSubstituter.h"

using namespace Origin::Services;

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    UsageQuery::UsageQuery(Content::EntitlementRef entitlement, const QByteArray &lastEtag) 
        : mEntitlement(entitlement),
        mLastEtag(lastEtag)
    {
    }

    UsageQuery::~UsageQuery()
    {
    }
        
    void UsageQuery::start()
    {
        const QString cloudSavesId(entitlement()->contentConfiguration()->cloudSavesId());
        Services::CloudSyncManifestDiscoveryResponse *resp = Services::CloudSyncServiceClient::discoverManifest(Engine::LoginController::instance()->currentUser()->getSession(), cloudSavesId);
        resp->setDeleteOnFinish(true);

        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(manifestDiscoverySucceeded()));
        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(CloudSyncManifestDiscoveryResponse::ManifestDiscoveryError)), this, SIGNAL(failed()));
    }
        
    void UsageQuery::manifestDiscoverySucceeded()
    {
        Origin::Services::CloudSyncManifestDiscoveryResponse *resp = dynamic_cast<Origin::Services::CloudSyncManifestDiscoveryResponse*>(sender());

        ORIGIN_ASSERT(resp);

        // Make a request for the manifest URL
        QNetworkRequest req(resp->manifestUrl());
        // We expect XML
        req.setRawHeader("Accept", "text/xml, application/xml");
        
        if (!mLastEtag.isNull())
        {
            req.setRawHeader("If-None-Match", mLastEtag);
        }

        QNetworkReply *reply = Origin::Services::NetworkAccessManager::threadDefaultInstance()->get(req);
        ORIGIN_VERIFY_CONNECT(reply, SIGNAL(finished()), this, SLOT(manifestRequestFinished()));
    }

    void UsageQuery::manifestRequestFinished()
    {
        QNetworkReply *reply = dynamic_cast<QNetworkReply*>(sender());

        ORIGIN_ASSERT(reply);

        reply->deleteLater();

        QNetworkReply::NetworkError error = reply->error();

        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 304)
        {
            // Nothing changed
            emit unchanged();
        }
        else if (error == QNetworkReply::NoError) 
        {
            QDomDocument replyDoc;

            if (!replyDoc.setContent(reply))
            {
                // Invalid XML 
                emit failed();
                return;
            }

            RemoteStateSnapshot *remoteState = new RemoteStateSnapshot(replyDoc, PathSubstituter(mEntitlement));

            if (!remoteState->isValid())
            {
                // Didn't like the content for some reason
                delete remoteState;
                emit failed();
                return;
            }

            emit succeeded(remoteState->localSize(), reply->rawHeader("ETag"));
            delete remoteState;
        }
        else if ((error == QNetworkReply::ContentNotFoundError) ||
            (error == QNetworkReply::ContentOperationNotPermittedError))
        {
            emit succeeded(0, QByteArray());
        }
        else 
        {
            // Unexpected error
            emit failed();
        }
    }
}
}
}
