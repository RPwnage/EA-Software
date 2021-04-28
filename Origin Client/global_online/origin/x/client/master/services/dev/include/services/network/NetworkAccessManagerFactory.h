#ifndef NETWORKACCESSMANAGERFACTORY_H
#define NETWORKACCESSMANAGERFACTORY_H

#include <QNetworkDiskCache>
#include <QList>
#include <QSslCertificate>
#include <QObject>
#include <QFuture>

#include "services/network/NetworkCookieJar.h"
#include "services/network/NetworkAccessManager.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class NetworkProxyFactory;
        ///
        /// Utility class for building thread-specific EbisuNetworkAccessManager instances
        ///
        class ORIGIN_PLUGIN_API NetworkAccessManagerFactory : public QObject 
        {
            Q_OBJECT
        public:
            static NetworkAccessManagerFactory* instance();

            NetworkAccessManager *createNetworkAccessManager();

            NetworkCookieJar *sharedCookieJar() const { return m_sharedCookieJar; }

            void clearCache();

        signals:
            void networkAccessManagerCreated(Origin::Services::NetworkAccessManager *);

        protected:
            NetworkAccessManagerFactory();

            QString cacheDirectory();
            QNetworkDiskCache *networkDiskCache();

            void initializeSslConfig();
            void trustCertificatesFromFile(const QString &filename);

            NetworkCookieJar *m_sharedCookieJar;
            QNetworkDiskCache *m_sharedCache;
            NetworkProxyFactory *m_sharedProxyFactory;

            QList<QSslCertificate> m_trustedCerts;
            QFuture<void> m_asyncSslInit;

            friend class NetworkAccessManager;

			QMutex m_sharedCacheMutex;
			QMutex m_sharedCookieJarMutex;
        };
    }
}

#endif
