#ifndef HTTPSERVICECLIENT_H
#define HTTPSERVICECLIENT_H

#include "services/network/NetworkAccessManager.h"
#include "services/session/AbstractSession.h"
#include "services/plugin/PluginAPI.h"
#include <QUrl>
#include <QNetworkCookie>


namespace Origin
{
    namespace Services
    {
        ///
        /// Base class for HTTP services
        ///
        class ORIGIN_PLUGIN_API HttpServiceClient
        {
        public:
            ///
            /// Functions for making non-authenticated network requests. If you need to make
            /// authenticated request, derive from OriginAuthServiceClient and use the *Auth()
            /// functions.
            ///
            QNetworkReply* headNonAuth(const QNetworkRequest &request) const;
            QNetworkReply* getNonAuth(const QNetworkRequest &request) const;
            QNetworkReply* postNonAuth(const QNetworkRequest &request, QIODevice *data) const;
            QNetworkReply* postNonAuth(const QNetworkRequest &request, const QByteArray &data) const;
            QNetworkReply* postNonAuth(const QNetworkRequest &request, QHttpMultiPart *multiPart) const;
            QNetworkReply* putNonAuth(const QNetworkRequest &request, QIODevice *data) const;
            QNetworkReply* putNonAuth(const QNetworkRequest &request, const QByteArray &data) const;
            QNetworkReply* putNonAuth(const QNetworkRequest &request, QHttpMultiPart *multiPart) const;
            QNetworkReply* deleteNonAuth(const QNetworkRequest &request) const;
            QNetworkReply* sendCustomRequestNonAuth(const QNetworkRequest &request, const QByteArray &verb, QIODevice *data = 0) const;

            ///
            /// Make a post request regardless of current offline state, used to request token refresh
            /// token, which may happen as part of the process of going online.
            ///
            /// This function should be used only for VERY SPECIAL CASES!!!
            ///
            QNetworkReply* postNonAuthOffline(const QNetworkRequest &request, const QByteArray &data) const;

        protected:
            ///
            /// \brief Returns the base URL for this service
            ///
            virtual QUrl baseUrl() const;

            ///
            /// \brief Provides a URL to a service path
            ///
            /// \param path  Relative path to the service
            /// \return QUrl Full URL to the service
            ///
            virtual QUrl urlForServicePath(const QString &path) const;

            ///
            /// \brief Provides a URL string to a service path
            ///
            /// \param path  Relative path to the service
            /// \return QString Full URL string to the service
            ///
            virtual QString urlStringForServicePath(const QString &path) const;

            /// 
            /// \brief Sets the base URL for this service
            ///
            /// \param baseUrl  New base URL for the service
            ///
            void setBaseUrl(const QUrl &baseUrl);

            ///
            /// \brief Creates a new HTTP service.
            ///
            /// \param nam      Network access manager to use. Defaults to the EbisuNetworkAccessManager singleton.
            ///
            HttpServiceClient(NetworkAccessManager *nam = NULL);
            virtual ~HttpServiceClient();

            ///
            /// \brief Returns the network access manager this service is using
            ///
            /// \return NetworkAccessManager A pointer to the NetworkAccessManager.
            virtual NetworkAccessManager* networkAccessManager() const;


            /// \brief sets cookies for the networkAccessManager
            /// \param cookie list
            /// \param URL
            void setNetworkAccessManagerCookieJar(const QList<QNetworkCookie>& cookies, const QUrl& serviceUrl);

            /// \brief create cookies for the request
            /// returns list of cookies
            QList<QNetworkCookie> getMeMyCookies(const QByteArray& at, const QByteArray& name = QByteArray("CEM-login"));

        private:

            /// \brief Network Access Manager
            NetworkAccessManager *m_nam;

            /// \brief the client's base URL
            QUrl m_baseUrl;

        };
    }
}

#endif