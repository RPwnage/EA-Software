#include "services/rest/HttpServiceClient.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/network/OfflineModeReply.h"
#include "services/network/NetworkCookieJar.h"


using namespace Origin::Services::Connection;

namespace Origin
{
    namespace Services
    {
        HttpServiceClient::HttpServiceClient(NetworkAccessManager *nam) :
        m_nam(nam)
        {
        }

        HttpServiceClient::~HttpServiceClient()
        {
        }

        QNetworkReply* HttpServiceClient::headNonAuth(const QNetworkRequest &request) const
        {
            if ( Connection::ConnectionStatesService::isGlobalStateOnline () )
                return networkAccessManager()->head(request);
            else
                return new OfflineModeReply(QNetworkAccessManager::HeadOperation, request);
        }

        QNetworkReply* HttpServiceClient::getNonAuth(const QNetworkRequest &request) const
        {
            if ( Connection::ConnectionStatesService::isGlobalStateOnline () )
                return networkAccessManager()->get(request);
            else
                return new OfflineModeReply(QNetworkAccessManager::GetOperation, request);
        }

        QNetworkReply* HttpServiceClient::postNonAuth(const QNetworkRequest &request, QIODevice *data) const
        {
            if ( Connection::ConnectionStatesService::isGlobalStateOnline () )
                return networkAccessManager()->post(request, data);
            else
                return new OfflineModeReply(QNetworkAccessManager::PostOperation, request);
        }

        QNetworkReply* HttpServiceClient::postNonAuth(const QNetworkRequest &request, const QByteArray &data) const
        {
            if ( Connection::ConnectionStatesService::isGlobalStateOnline () )
                return networkAccessManager()->post(request, data);
            else
                return new OfflineModeReply(QNetworkAccessManager::PostOperation, request);
        }

        QNetworkReply* HttpServiceClient::postNonAuth(const QNetworkRequest &request, QHttpMultiPart *multiPart) const
        {
            if ( Connection::ConnectionStatesService::isGlobalStateOnline () )
                return networkAccessManager()->post(request, multiPart);
            else
                return new OfflineModeReply(QNetworkAccessManager::PostOperation, request);
        }

        QNetworkReply* HttpServiceClient::putNonAuth(const QNetworkRequest &request, QIODevice *data) const
        {
            if ( Connection::ConnectionStatesService::isGlobalStateOnline () )
                return networkAccessManager()->put(request, data);
            else
                return new OfflineModeReply(QNetworkAccessManager::PutOperation, request);
        }

        QNetworkReply* HttpServiceClient::putNonAuth(const QNetworkRequest &request, const QByteArray &data) const
        {
            if ( Connection::ConnectionStatesService::isGlobalStateOnline () )
                return networkAccessManager()->put(request, data);
            else
                return new OfflineModeReply(QNetworkAccessManager::PutOperation, request);
        }

        QNetworkReply* HttpServiceClient::putNonAuth(const QNetworkRequest &request, QHttpMultiPart *multiPart) const
        {
            if ( Connection::ConnectionStatesService::isGlobalStateOnline () )
                return networkAccessManager()->put(request, multiPart);
            else              
                return new OfflineModeReply(QNetworkAccessManager::PutOperation, request);
        }

        QNetworkReply* HttpServiceClient::deleteNonAuth(const QNetworkRequest &request) const
        {
            if ( Connection::ConnectionStatesService::isGlobalStateOnline ())
                return networkAccessManager()->deleteResource(request);
            else
                return new OfflineModeReply(QNetworkAccessManager::DeleteOperation, request);
        }

        QNetworkReply* HttpServiceClient::sendCustomRequestNonAuth(const QNetworkRequest &request, const QByteArray &verb, QIODevice *data /*= NULL*/) const
        {
            if ( Connection::ConnectionStatesService::isGlobalStateOnline () )
                return networkAccessManager()->sendCustomRequest(request, verb, data);
            else
                return new OfflineModeReply(QNetworkAccessManager::CustomOperation, request);
        }

        QNetworkReply* HttpServiceClient::postNonAuthOffline(const QNetworkRequest &request, const QByteArray &data) const
        {
            return networkAccessManager()->post(request, data);
        }

        NetworkAccessManager* HttpServiceClient::networkAccessManager() const
        {
            if (m_nam)
                return m_nam;
            else
                return NetworkAccessManager::threadDefaultInstance();
        }

        void HttpServiceClient::setBaseUrl(const QUrl &baseUrl)
        {
            m_baseUrl = baseUrl;
        }

        QUrl HttpServiceClient::baseUrl() const
        {
            return m_baseUrl;
        }

        QString HttpServiceClient::urlStringForServicePath(const QString &myPath) const 
        {
            QString path(myPath);
            if (path.startsWith("/"))
            {
                path.remove(0,1);
            } 

            QString serviceUrlStr = baseUrl().toString();

            if (baseUrl().path().endsWith("/") )
            {
                serviceUrlStr = serviceUrlStr + path;
            } 
            else
            {
                serviceUrlStr = serviceUrlStr + QString("/") + path;
            }
            return serviceUrlStr;
        }

        QUrl HttpServiceClient::urlForServicePath(const QString &myPath) const 
        {
            QString path(myPath);
            if (path.startsWith("/"))
            {
                path.remove(0,1);
            } 

            QUrl serviceUrl = baseUrl();

            if (baseUrl().path().endsWith("/") )
            {
                serviceUrl.setPath(baseUrl().path() + path);
            } 
            else
            {
                serviceUrl.setPath(baseUrl().path() + QString("/") + path);
            }
            return serviceUrl;
        }

        void HttpServiceClient::setNetworkAccessManagerCookieJar( const QList<QNetworkCookie>& cookies, const QUrl& serviceUrl )
        {
            auto cookieJar = new Services::NetworkCookieJar(NULL);
            cookieJar->setCookiesFromUrl(cookies,serviceUrl);
            networkAccessManager()->setCookieJar(cookieJar);
        }

        QList<QNetworkCookie> HttpServiceClient::getMeMyCookies( const QByteArray& at, const QByteArray& name /*= QByteArray("CEM-login")*/ )
        {
            QNetworkCookie cookie;
            cookie.setDomain(".origin.com");
            cookie.setHttpOnly(false);
            cookie.setSecure(false);
            cookie.setName(name);
            cookie.setValue(at);
            cookie.setPath("/");

            QList<QNetworkCookie> cookies;
            cookies.append(cookie);
            return cookies;
        }

    }
}
