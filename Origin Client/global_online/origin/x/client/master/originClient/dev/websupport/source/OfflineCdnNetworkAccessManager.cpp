#include "OfflineCdnNetworkAccessManager.h"

#include "services/network/NetworkAccessManager.h"
#include "services/debug/DebugService.h"
#include "services/connection/ConnectionStatesService.h"
#include "engine/login/LoginController.h"
#include "engine/login/User.h"


namespace
{
    QNetworkReply* sendRequestToNetworkAccessManager(QNetworkAccessManager *manager, QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
    {
        switch (op)
        {
        case QNetworkAccessManager::GetOperation:
            return manager->get(req);
        case QNetworkAccessManager::PostOperation:
            return manager->post(req, outgoingData);
        case QNetworkAccessManager::PutOperation:
            return manager->put(req, outgoingData);
        case QNetworkAccessManager::DeleteOperation:
            return manager->deleteResource(req);
        case QNetworkAccessManager::HeadOperation:
            return manager->head(req);
        case QNetworkAccessManager::CustomOperation:
            return manager->sendCustomRequest(req, req.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray(), outgoingData);
        default:
            break;
        }

        return NULL;
    }

}

namespace Origin
{
    namespace Client
    {

    OfflineCdnNetworkAccessManager::OfflineCdnNetworkAccessManager(QObject *parent) : 
    QNetworkAccessManager(parent)
    {
    }

    QNetworkReply* OfflineCdnNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
    {
        using namespace Origin::Services;
        QNetworkRequest cachingReq(req);

        QString reqUrlStr = req.url().toString();

        Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();

        //Since this is used by the SPA, it's ok to assume that we'll want to always used the cached version when the network requests are coming from the SPA
        bool online = (user != NULL && Origin::Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()));
        if(!online)
        {
            cachingReq.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
            return sendRequestToNetworkAccessManager(
                NetworkAccessManager::threadDefaultInstance(),
                op,
                cachingReq,
                outgoingData);
        }
        else
        {
            cachingReq.setAttribute (QNetworkRequest::CacheSaveControlAttribute, QVariant::fromValue (true));

            return sendRequestToNetworkAccessManager(
                NetworkAccessManager::threadDefaultInstance(),
                op,
                cachingReq,
                outgoingData);
        }
    }
    }
}
