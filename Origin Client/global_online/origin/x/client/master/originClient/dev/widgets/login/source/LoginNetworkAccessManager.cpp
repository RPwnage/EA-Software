#include "LoginNetworkAccessManager.h"

#include "services/network/NetworkAccessManager.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/debug/DebugService.h"

#include "services/log/LogService.h"

namespace Origin
{
namespace Client
{

    LoginNetworkAccessManager::LoginNetworkAccessManager() : 
        Services::NetworkAccessManager()
        , mUseCache(false) 
    {
    }

    QNetworkReply* LoginNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
    {
        QNetworkRequest cachingReq(req);
        if (mUseCache)
        {
            cachingReq.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
        }

        return Services::NetworkAccessManager::createRequest(op, cachingReq, outgoingData);
    }

    void LoginNetworkAccessManager::setPreferNetwork()
    {
        mUseCache = false;
    }

    void LoginNetworkAccessManager::setAlwaysUseCache()
    {
        mUseCache = true;
    }    
}
}
