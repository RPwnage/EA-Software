// TODO move to \\global_online\origin\client\services\dev\network\source
///////////////////////////////////////////////////////////////////////////////
// NetworkAccessManager.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QNetworkReply>
#include <QApplication>
#include <QLocale>
#include <QThreadStorage>
#include <QMutexLocker>
#include <QStringList>
#include <QTimer>

#include "services/network/NetworkAccessManager.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/network/OfflineModeReply.h"
#include "services/rest/UserAgent.h"
#include "services/Services.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"
#include "services/settings/SettingsManager.h"
#include "services/debug/DebugService.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
    namespace Services
    {
        // Our custom attributes
        static const QNetworkRequest::Attribute EbisuRequestSourceAttribute = static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 0);

        NetworkAccessManager::NetworkAccessManager() 
            : m_requestSource(SystemRequestSourceHint)
            , m_shouldAddDebugHeaders(Services::readSetting(Services::SETTING_OverridesEnabled).toQVariant().toBool())
            , m_geoIp(Services::readSetting(Origin::Services::SETTING_GeolocationIPAddress).toString().toUtf8())
            , m_geoTestHeader(Services::readSetting(Services::SETTING_OriginGeolocationTest).toString().toUtf8())
        {
            // md5(eu.gcsip.nl)
            m_forceServerProtocol["\x82\xec\x9d\xe1\x62\x6a\x68\xa2\x0d\x17\xa0\x21\x4b\xd0\xc0\x28"] = QSsl::TlsV1_0;

            // md5(.econ.ne.jp)
            m_forceServerProtocol["\x46\x13\xbf\x8b\x7e\x56\xcb\x64\xae\x94\xdf\xd1\x05\xae\x9c\x21"] = QSsl::TlsV1_0;
        }

        NetworkAccessManager *NetworkAccessManager::threadDefaultInstance()
        {
            // QNetworkAccessManager isn't thread safe so we need to have one of us
            // per thread
            static QThreadStorage<NetworkAccessManager*> instances;

            if (instances.hasLocalData())
            {
                return instances.localData();
            }
            else
            {
                NetworkAccessManagerFactory *enaf = NetworkAccessManagerFactory::instance();
                NetworkAccessManager *newInstance = enaf->createNetworkAccessManager();

                instances.setLocalData(newInstance);

                return newInstance;
            }

        }

		QNetworkRequest NetworkAccessManager::networkRequest( const QNetworkRequest& req )
		{
			QNetworkRequest request(req);

			// Do not enable HTTP pipelining
			// Qt has problems associating requests with response bodies when we're
			// heavily pipelined
			request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, false);

            if (!request.hasRawHeader(UserAgent::title()))
			{
				// Include our user agent and version
				request.setRawHeader(UserAgent::title(), UserAgent::headerAsBA());
			}

            // If the X-Origin-UID header is not already set it.
            // NOTE: For 9.2 and later we use the machine ID only !!!!
            if (!request.hasRawHeader("X-Origin-UID"))
            {
                request.setRawHeader("X-Origin-UID", PlatformService::machineHashAsString().toUtf8());
            }

            // If the X-Origin-Platform header is not already set,
            if (!request.hasRawHeader("X-Origin-Platform"))
            {
                // Set the X-Origin-Platform header.
#ifdef Q_OS_WIN
                request.setRawHeader("X-Origin-Platform", "PCWIN");
#elif defined(ORIGIN_MAC)
                request.setRawHeader("X-Origin-Platform", "MAC");
#endif
            }
            
            // If an EACore.ini header is present, add this debug header to allow
            // server team to optimize by excluding algorithms for QA specific functionality
            // such as GeoIP and store preview entitlement checks that add additional load
            // server side...
            if(m_shouldAddDebugHeaders)
            {
                request.setRawHeader("X-Origin-Client-Debug", "true");
            }

			// Don't overwrite the locale if someone has already set it.
			if (!request.hasRawHeader("localeInfo"))
			{
				QString localeName = QLocale().name();
				// This is just different naming between Qt and server
				if(localeName == "nb_NO")
				{
					localeName = "no_NO";
				}
				request.setRawHeader("localeInfo", localeName.toLatin1());
                // Set the locale every time. It reverts back to en-us.
                request.setRawHeader("Accept-Language", localeName.replace("_", "-").toUtf8());
			}

            if (!m_geoIp.isEmpty())
            {
                if (!request.hasRawHeader("X-Forwarded-For"))
                {
                    request.setRawHeader("X-Forwarded-For", m_geoIp);
                }

                if (!request.hasRawHeader("Origin-ClientIp"))
                {
                    request.setRawHeader("Origin-ClientIp", m_geoIp);
                }

                if (!request.hasRawHeader("X-Origin-Geo-Test") && !m_geoTestHeader.isEmpty())
                {
                    request.setRawHeader("X-Origin-Geo-Test", m_geoTestHeader);
                }
            }

			// Record our request source hint
			request.setAttribute(EbisuRequestSourceAttribute, QVariant(static_cast<unsigned int>(m_requestSource)));

            QCryptographicHash hostHash(QCryptographicHash::Md5);
            const QString &host = request.url().host().toLower();

            hostHash.addData(host.toUtf8());
            if (!m_forceServerProtocol.contains(hostHash.result()))
            {
                hostHash.reset();
                hostHash.addData(host.mid(host.indexOf('.')).toUtf8());
            }

            if (m_forceServerProtocol.contains(hostHash.result()))
            {
                QSslConfiguration sslConfig = request.sslConfiguration();
                sslConfig.setProtocol(m_forceServerProtocol[hostHash.result()]);
                request.setSslConfiguration(sslConfig);
            }

			return request;

		}

		QNetworkRequest NetworkAccessManager::networkRequest( const QUrl& url )
		{
			QNetworkRequest request(url);
			return networkRequest(request);
		}

		QNetworkRequest NetworkAccessManager::networkRequest( const QString& url )
		{
			const QUrl URL(url);
			return networkRequest(URL);
		}

        QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest & req, QIODevice * outgoingData)
        {
            QNetworkRequest request = networkRequest(req);
            QNetworkReply* reply;

#ifdef _DEBUG
            // dumped this here to allow us to inspect the request URL
            QString requestUrl(request.url().toString());
#endif
            
#define ENABLE_URL_BLACKLIST 1

#if defined(ORIGIN_MAC) && ENABLE_URL_BLACKLIST
            // Configure a static blacklist.
            // NOTE: currently this is only used on the Mac, but it can be left in here for future use.
            static bool blacklistUninitialized = true;
            static QStringList blacklist;
            if (blacklistUninitialized)
            {
                // blacklist ru4.com due to slow or non-existent responses
                blacklist << "d.xp1.ru4.com";
                blacklistUninitialized = false;
            }
            
            // If this request host is blacklisted,
            if (blacklist.contains(req.url().host().toUtf8()))
            {
                
#ifdef _DEBUG
                ORIGIN_LOG_EVENT << "Blacklisted URL: " << req.url().toString();
#endif
                
                // Construct an access denied reply.
                NetworkReply *forbidden = new NetworkReply(op, req, this);
                forbidden->setHttpStatusCode( 403, "Forbidden" );
                forbidden->setContentType("text/html");
                forbidden->setBody( QString("<html><body><p>Resource forbidden.</p></body></html>") );
                reply = forbidden;
            }
            else
#endif
            {
                reply = QNetworkAccessManager::createRequest(op, request, outgoingData);

                // Connect reply to the ssl error handler in this nam.  We now listen for sslErrors from the
                // qNetworkReply directly rather than from the qNetworkAccessManager (they should both be emitting
                // the signal) because the qNam signal doesn not seem to be working properly on Mac, EBIBUGS-28658.
                ORIGIN_VERIFY_CONNECT(reply, SIGNAL(sslErrors(const QList<QSslError> &)),
                                      this, SLOT(handleSslErrors(const QList<QSslError> &)));
                
                //	Link the reply to the heartbeat monitor
                emit(requestCreated(request, reply));
            }
            return reply;
        }
        
        QString NetworkAccessManager::getCertificateSubjectString(const QSslCertificate& cert)
        {
            QString o = cert.subjectInfo(QSslCertificate::Organization).join(" - ");
            QString cn = cert.subjectInfo(QSslCertificate::CommonName).join(" - ");
            QString l = cert.subjectInfo(QSslCertificate::LocalityName).join(" - ");
            QString ou = cert.subjectInfo(QSslCertificate::OrganizationalUnitName).join(" - ");
            QString c = cert.subjectInfo(QSslCertificate::CountryName).join(" - ");
            QString st = cert.subjectInfo(QSslCertificate::StateOrProvinceName).join(" - ");
            
            return (o.length() > 0 ? "O=" + o + " ": "")
            + (cn.length() >0 ? "CN=" + cn + " ": "")
            + (l.length() >0 ? "L=" + l + " ": "")
            + (ou.length() >0 ? "OU=" + ou + " ": "")
            + (c.length() >0 ? "C=" + c + " ": "")
            + (st.length() >0 ? "ST=" + st + " ": "");
        }
        
        QString NetworkAccessManager::getCertificateIssuerString(const QSslCertificate& cert)
        {
            QString o = cert.issuerInfo(QSslCertificate::Organization).join(" - ");
            QString cn = cert.issuerInfo(QSslCertificate::CommonName).join(" - ");
            QString l = cert.issuerInfo(QSslCertificate::LocalityName).join(" - ");
            QString ou = cert.issuerInfo(QSslCertificate::OrganizationalUnitName).join(" - ");
            QString c = cert.issuerInfo(QSslCertificate::CountryName).join(" - ");
            QString st = cert.issuerInfo(QSslCertificate::StateOrProvinceName).join(" - ");
            
            return (o.length() > 0 ? "O=" + o + " ": "")
            + (cn.length() >0 ? "CN=" + cn + " " : "")
            + (l.length() >0 ? "L=" + l + " ": "")
            + (ou.length() >0 ? "OU=" + ou + " ": "")
            + (c.length() >0 ? "C=" + c + " ": "")
            + (st.length() >0 ? "ST=" + st + " ": "");
        }
        
        void NetworkAccessManager::handleSslErrors(const QList<QSslError> &errors)
        {
            QNetworkReply *reply = dynamic_cast<QNetworkReply*>(sender());
            
            if (readSetting(SETTING_IgnoreSSLCertError, Session::SessionRef()))
            {
                // SSL errors are being ignored at the application level
                reply->ignoreSslErrors();
                return;
            }
            
            if (sourceForRequest(reply->request()) != UserRequestSourceHint)
            {
                // Log this to telemetry and the log
                ORIGIN_LOG_WARNING << "Request to " << reply->url().toString() << " failed with " << errors.length() << " unsuppressed SSL errors";
                for(QList<QSslError>::ConstIterator it = errors.constBegin(); it != errors.constEnd(); it++)
                {
                    const QSslError &error = *it;
                    
                    ORIGIN_LOG_WARNING << "Certificate with "
                                       << "common name(s) '" << getCertificateSubjectString(error.certificate()) << "', "
                                       << "SHA-1 '" << error.certificate().digest(QCryptographicHash::Sha1).toHex() << "', "
                                       << "expiry '" << error.certificate().expiryDate().toString(Qt::ISODate) << "' "
                                       << "failed with error '" << error.errorString() << "'";
                    
                    GetTelemetryInterface()->Metric_NETWORK_SSL_ERROR(
                        reply->url().toString().toUtf8().data(),
                        static_cast<uint32_t>(error.error()),
                        error.certificate().digest(QCryptographicHash::Sha1).toHex().data(),
                        getCertificateSubjectString(error.certificate()).toUtf8().data(),
                        getCertificateIssuerString(error.certificate()).toUtf8().data() );
                }
            }
        }

        NetworkAccessManager::RequestSourceHint NetworkAccessManager::sourceForRequest(const QNetworkRequest &req)
        {
            QVariant attrValue = req.attribute(EbisuRequestSourceAttribute, QVariant(static_cast<unsigned int>(SystemRequestSourceHint)));
            return static_cast<RequestSourceHint>(attrValue.toInt());
        }
        
        
        NetworkReply::NetworkReply( QNetworkAccessManager::Operation op, const QNetworkRequest& req, QObject *parent ) : QNetworkReply(parent)
        {
            setOperation(op);
            setRequest(req);
        }
        
        void NetworkReply::setHttpStatusCode( int code, const QByteArray &statusText )
        {
            setAttribute( QNetworkRequest::HttpStatusCodeAttribute, code );
            if ( statusText.isNull() ) return;
            setAttribute( QNetworkRequest::HttpReasonPhraseAttribute, statusText );
        }
        
        void NetworkReply::setHeader( QNetworkRequest::KnownHeaders header, const QVariant &value )
        {
            QNetworkReply::setHeader( header, value );
        }
        
        void NetworkReply::setContentType( const QByteArray &contentType )
        {
            setHeader(QNetworkRequest::ContentTypeHeader, contentType);
        }
        
        void NetworkReply::setBody( const QString &body )
        {
            setBody(body.toUtf8());
        }
        
        void NetworkReply::setBody( const QByteArray &body )
        {
            mLocation = 0;
            mBody = body;
            
            open(ReadOnly | Unbuffered);
            setHeader(QNetworkRequest::ContentLengthHeader, QVariant(body.size()));
            
            // Queue up some IODevice signals.
            QTimer::singleShot( 0, this, SIGNAL(readyRead()) );
            QTimer::singleShot( 0, this, SIGNAL(finished()) );
        }
        
        void NetworkReply::abort() {} // do nothing
        
        qint64 NetworkReply::bytesAvailable() const
        {
            return mBody.size() - mLocation;
        }
        
        bool NetworkReply::isSequential() const
        {
            return true;
        }
        
        
        qint64 NetworkReply::readData(char *data, qint64 maxSize)
        {
            if (mLocation >= mBody.size())
                return -1;
            
            qint64 number = qMin(maxSize, mBody.size() - mLocation);
            memcpy(data, mBody.constData() + mLocation, number);
            mLocation += number;
            
            return number;
        }


	}
}

