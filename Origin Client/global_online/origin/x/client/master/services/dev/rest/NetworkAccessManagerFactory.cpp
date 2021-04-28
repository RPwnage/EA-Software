//  NetworkAccessManagerFactory.h
//  Copyright 2012-2013 Electronic Arts Inc. All rights reserved.

#include <limits>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QNetworkReply>
#include <QNetworkCookie>
#include <QNetworkProxyFactory>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QtConcurrentRun>
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/network/NetworkProxyFactory.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/network/SslSafeNetworkDiskCache.h"
#include "services/platform/PlatformService.h"

#if defined (Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h> 
#include <strsafe.h>
#endif


namespace Origin
{
    namespace Services
    {
        static NetworkAccessManagerFactory *enamfInstance = NULL;

        // This is to get around multiple NetworkAccessManagers sharing one cookie ar
        class QNetworkCookieJarProxy : public QNetworkCookieJar
        {
        public:
            QNetworkCookieJarProxy(QNetworkCookieJar *backend, QMutex *sharedMutex) : m_accessLock(sharedMutex), m_backend(backend)
            {
            }

            QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const
            {
                QMutexLocker locker(m_accessLock);
                if (m_backend)
                    return m_backend->cookiesForUrl(url);
                return QList<QNetworkCookie>();
            }

            bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
            {
                QMutexLocker locker(m_accessLock);
                if (m_backend)
                    return m_backend->setCookiesFromUrl(cookieList, url);
                return 0;
            }

        protected:
            QMutex *m_accessLock;
            QNetworkCookieJar *m_backend;
        };

        class QNetworkCacheProxy : public QAbstractNetworkCache
        {
        public:
            QNetworkCacheProxy(QAbstractNetworkCache *backend, QMutex *sharedCacheMutex) : m_accessLock(sharedCacheMutex), m_backend(backend)
            {
            }

            virtual qint64 cacheSize() const 
            {
                QMutexLocker locker(m_accessLock);
                if (m_backend)
                    return m_backend->cacheSize();
                return 0;
            }

            virtual QIODevice *data(const QUrl &url) 
            {
                QMutexLocker locker(m_accessLock);
                if (m_backend)
                    return m_backend->data(url);
                return 0;
            }

            virtual void insert(QIODevice *device)  
            {
                QMutexLocker locker(m_accessLock);
                if (m_backend)
                    return m_backend->insert(device);
            }

            virtual QNetworkCacheMetaData metaData(const QUrl &url) 
            {
                QMutexLocker locker(m_accessLock);
                if (m_backend)
                    return m_backend->metaData(url);
                return QNetworkCacheMetaData();
            }

            virtual QIODevice *prepare(const QNetworkCacheMetaData &metaData)  
            {
                QMutexLocker locker(m_accessLock);
                if (m_backend)
                    return m_backend->prepare(metaData);
                return 0;
            }

            virtual bool remove(const QUrl &url) 
            {
                QMutexLocker locker(m_accessLock);
                if (m_backend)
                    return m_backend->remove(url);
                return 0;
            }

            virtual void updateMetaData(const QNetworkCacheMetaData &metaData) 
            {
                QMutexLocker locker(m_accessLock);
                if (m_backend)
                    return m_backend->updateMetaData(metaData);
            }

            virtual void clear() 
            {
                QMutexLocker locker(m_accessLock);
                if (m_backend)
                    m_backend->clear();
            }

        protected:
            QMutex *m_accessLock;
            QAbstractNetworkCache *m_backend;
        };

        // This class name isn't a typo - this is a proxy class for QNetworkProxyFactory
        class QNetworkProxyFactoryProxy : public QNetworkProxyFactory
        {
        public:
            QNetworkProxyFactoryProxy(QNetworkProxyFactory *backend) : m_backend(backend)
            {
            }


            virtual QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query) 
            {
                if (m_backend)
                    return m_backend->queryProxy(query);
                
                return QList<QNetworkProxy>();
            }

        protected:
            // QNetworkProxyFactory appears to be thread safe and proxy requests can
            // block for unreasonable amounts of time. Don't lock requests
            QNetworkProxyFactory *m_backend;
        };

        NetworkAccessManagerFactory *NetworkAccessManagerFactory::instance()
        {
            static QMutex instanceMutex;

            if (enamfInstance == NULL)
            {
                QMutexLocker locker(&instanceMutex);

                if (enamfInstance == NULL)
                {
                    enamfInstance = new NetworkAccessManagerFactory;
                }
            }

            return enamfInstance;
        }

        NetworkAccessManagerFactory::NetworkAccessManagerFactory()
        {
            m_sharedCache = networkDiskCache();

            // Use the OS' proxy settings
            QNetworkProxyFactory::setUseSystemConfiguration(true);

            // Automatically clears itself on logout
            m_sharedCookieJar = new NetworkCookieJar();

            // Does some crazy sutff to get around proxy configurations that take a while to query
            m_sharedProxyFactory = new NetworkProxyFactory;

            // Start initializing SSL in the background
            // This can be extremely slow (>1 second)
            m_asyncSslInit = QtConcurrent::run(this, &NetworkAccessManagerFactory::initializeSslConfig);
        }

        void NetworkAccessManagerFactory::initializeSslConfig()
        {
            // Get our current SSL config
            QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();

            QList<QSslCertificate> certs = sslConfig.caCertificates();

            //
            // Additional certificate we require for authentication
            //

            // Add the GeoTrust cert used for Common Services
            trustCertificatesFromFile(":/certificates/GeoTrust_Global_CA.cer");
            // And the AddTrust cert used for Omniture
            trustCertificatesFromFile(":/certificates/AddTrust_External_CA.cer");
            // And the UserTrust cert used for Playstation Network
            trustCertificatesFromFile(":/certificates/UserTrust_CA.cer");
            // And the PositiveSSL cert also used by PSN (?)
            trustCertificatesFromFile(":/certificates/PositiveSSL.cer");
            // And DigiCert used by Facebook
            trustCertificatesFromFile(":/certificates/DigiCertHighAssuranceCA3.cer");
            trustCertificatesFromFile(":/certificates/DigiCertHighAssuranceEVRootCA.cer");
            // And a VeriSign cert for Live
            trustCertificatesFromFile(":/certificates/VeriSignG5.cer");
            // And Entrust Amazon S3/cloud saves
            trustCertificatesFromFile(":/certificates/Entrust.cer");
            // And a VeriSign cert for decryption
            trustCertificatesFromFile(":/certificates/PCA-3G2.pem");
            // And a TrustWave cert for paypal/globalcollect
            trustCertificatesFromFile(":/certificates/TrustWave_dvca.cer");
            // And a godaddy cert for twitch
            trustCertificatesFromFile(":/certificates/gd-class2-root.cer");
            // And a valicert cert for twitch
            trustCertificatesFromFile(":/certificates/valicert_class2_root.cer");
            // And a Equifax certificate used by Google
            trustCertificatesFromFile(":/certificates/Equifax.cer");
			// Add VeriSign G3 for www.origin.com
			trustCertificatesFromFile(":/certificates/VeriSignG3.cer");
            
#ifdef ORIGIN_MAC
			// Add VeriSign G5 for old intermediate certificate (no longer supported on OS X 10.11?)
			trustCertificatesFromFile(":/certificates/Class-3-Public-Primary-Certification-Authority.cer");
			// Add GTE CyberTrust Global Root (no longer supported on OS X 10.11?)
			trustCertificatesFromFile(":/certificates/GTE_CyberTrust_Global_Root.cer");
#endif

            // Set our new cert chain
            certs.append(m_trustedCerts);

#ifdef ORIGIN_MAC
            Origin::Services::PlatformService::readLocalCertificates(&certs);
#endif
            sslConfig.setCaCertificates(certs);

            // Apply our config
            QSslConfiguration::setDefaultConfiguration(sslConfig);
        }

        NetworkAccessManager *NetworkAccessManagerFactory::createNetworkAccessManager()
        {
            // Wait for our background SSL initialization to finish
            m_asyncSslInit.waitForFinished();

            NetworkAccessManager *enam = new NetworkAccessManager();
            enam->setCache(new QNetworkCacheProxy(m_sharedCache, &m_sharedCacheMutex));
            enam->setCookieJar(new QNetworkCookieJarProxy(m_sharedCookieJar, &m_sharedCookieJarMutex));
            enam->setProxyFactory(new QNetworkProxyFactoryProxy(m_sharedProxyFactory));

            emit networkAccessManagerCreated(enam);

            return enam;
        }


        QString NetworkAccessManagerFactory::cacheDirectory()
        {
#ifdef _WIN32
            wchar_t path[MAX_PATH + 1];
            HRESULT concatResult;

            SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path);
            concatResult = StringCbCatW(path, sizeof(path), L"\\Origin\\Web Cache");

            ORIGIN_ASSERT(concatResult == S_OK);

            return QString::fromUtf16(path);
#else
            return Origin::Services::PlatformService::getStorageLocation(QStandardPaths::CacheLocation) + "/Web Cache";
#endif
        }

        QNetworkDiskCache* NetworkAccessManagerFactory::networkDiskCache()
        {
            QNetworkDiskCache *cache = new SslSafeNetworkDiskCache();

            // Find out our cache dir (platform specific)
            QString cacheDir = cacheDirectory();
            // Make our cache dir if we need it
            QDir("/").mkpath(cacheDir);
            // Set our cache dir
            cache->setCacheDirectory(cacheDir);

            return cache;
        }

        void NetworkAccessManagerFactory::clearCache()
        {
            if (m_sharedCache)
            {
                QMutexLocker lock(&m_sharedCacheMutex);
                m_sharedCache->clear();
            }
        }

        void NetworkAccessManagerFactory::trustCertificatesFromFile(const QString &filename)
        {
            QFile certFile(filename);
            if (certFile.exists() && certFile.open(QIODevice::ReadOnly))
            {
                QList<QSslCertificate> toAdd = QSslCertificate::fromDevice(&certFile, QSsl::Pem); 

				// Make sure we can parse the cert
				ORIGIN_ASSERT(toAdd.length() >= 1);
                ORIGIN_ASSERT(toAdd[0].isBlacklisted() == false);

                m_trustedCerts.append(toAdd);
            }
            else
            {
                // Couldn't read the cert
                ORIGIN_ASSERT(0);
            }
        }
    }
}
