#include "LocalHostSecurity.h"

#include <QNetworkRequest>
#include <QSslSocket>
#include <QTimer>
#include "LocalHost/LocalHostServices/IService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/network/NetworkAccessManager.h"
#include "services/settings/SettingsManager.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
	namespace Sdk
	{
		namespace LocalHost
		{
			static LocalHostSecurity *sLocalHostSecurityInstance = NULL;

#define SECURITY_SERVICE_TIME  (60*60)    // number of seconds services are open after https assisted validation (DICE requested 1 hour)
#define SECURITY_MAX_FILE_SIZE 1024     // max number of characters we will read in from security services list from server

			void LocalHostSecurity::registerServices()
			{   // add service text name to bitmask value
				mSecureServiceMap["pingserver"] = SECURE_SERVICE_BIT_PINGSERVER;
			}

			LocalHostSecurity *LocalHostSecurity::instance()
			{
				if (sLocalHostSecurityInstance == NULL)
					start();

				return sLocalHostSecurityInstance;
			}

			void LocalHostSecurity::start()
			{
				if (!sLocalHostSecurityInstance)
				{
					sLocalHostSecurityInstance = new LocalHostSecurity();
					ORIGIN_VERIFY_CONNECT(&sLocalHostSecurityInstance->mUpdateTimer, SIGNAL(timeout()), sLocalHostSecurityInstance, SLOT(onTimerInterval()));
					sLocalHostSecurityInstance->mUpdateTimer.setInterval(1000); // 1 second updates
					sLocalHostSecurityInstance->mNetworkAccessManager = Origin::Services::NetworkAccessManager::threadDefaultInstance();

					sLocalHostSecurityInstance->registerServices();

					// per DICE's request if not Production, turn off HTTPS assisted validation
					if (Services::readSetting(Services::SETTING_EnvironmentName).toString().compare(Services::env::production) == 0)
						sLocalHostSecurityInstance->mVerificationDisabled = false;
					else
						sLocalHostSecurityInstance->mVerificationDisabled = true;
				}
			}

			void LocalHostSecurity::stop()
			{
				if (sLocalHostSecurityInstance)
				{
					sLocalHostSecurityInstance->mUpdateTimer.stop();

					while (sLocalHostSecurityInstance->mRequestList.count())
					{
						sLocalHostSecurityInstance->onCloseRequest(sLocalHostSecurityInstance->mRequestList.takeFirst());
					}

					delete sLocalHostSecurityInstance;
					sLocalHostSecurityInstance = NULL;
				}
			}

			void LocalHostSecurity::onTimerInterval()
			{
				if (sLocalHostSecurityInstance == NULL)
					return;

				if (mSecurityStatusMap.count() == 0)
				{
					mUpdateTimer.stop();    // stop updating until something new is added
					return;
				}

				QMap<QString, SecurityStatusType>::iterator iter = mSecurityStatusMap.begin();

				while (iter != mSecurityStatusMap.end())
				{
					QMap<QString, SecurityStatusType>::iterator hold = iter;
					SecurityStatusType& status = iter.value();

					iter++;

					status.mTimeRemaining--;    // tick down time remaining
					if (status.mTimeRemaining <= 0)
						mSecurityStatusMap.remove(hold.key());
				}

				if (mSecurityStatusMap.count() == 0)     // now empty?
					mUpdateTimer.stop();    // stop updating until something new is added
			}

			void LocalHostSecurity::onCloseRequest(LocalHostSecurityRequest *request)
			{
				// close down request
				if (request)
				{
					mRequestList.removeAll(request);

					ORIGIN_VERIFY_DISCONNECT(request, SIGNAL(closeRequest(LocalHostSecurityRequest *)), this, SLOT(onCloseRequest(LocalHostSecurityRequest *)));

					request->mNetworkReply->abort();
					request->mNetworkReply->deleteLater();
					request->mNetworkReply = NULL;
					request->deleteLater();
				}
			}

			bool LocalHostSecurity::isCurrentlyUnlocked(QString domain, uint32_t service_bit)
			{
				if (!sLocalHostSecurityInstance)
					return false;

				if (sLocalHostSecurityInstance->mVerificationDisabled)
					return true;

				if (sLocalHostSecurityInstance->mSecurityStatusMap.isEmpty())
					return false;

				if (sLocalHostSecurityInstance->mSecurityStatusMap.contains(domain))
				{
					QMap<QString, SecurityStatusType>::iterator iter = sLocalHostSecurityInstance->mSecurityStatusMap.find(domain);
					SecurityStatusType& status = iter.value();

					if (status.mServiceMask & (1L << service_bit))
						return true;
				}

				return false;
			}

			void LocalHostSecurity::verifyServices(QString domain, QStringList service_list)
			{
				SecurityStatusType status;

				status.mServiceMask = 0;

				for (int i = 0; i < service_list.count(); i++)
				{
					if (mSecureServiceMap.contains(service_list[i]))
					{
						status.mServiceMask |= (1L << mSecureServiceMap[service_list[i]]);
					}
				}

				// temporary values
				status.mTimeRemaining = SECURITY_SERVICE_TIME;    // seconds
				mSecurityStatusMap[domain] = status;

				if (!mUpdateTimer.isActive())
					mUpdateTimer.start();   // start updates
			}

			LocalHostSecurityRequest *LocalHostSecurity::validateConnection(QString domain)
			{
				if (!sLocalHostSecurityInstance)
				{
					return NULL;
				}

				QString hostname = QString("https://") + domain + QString("/origin/validate");

				QUrl url = QUrl(hostname);
				QNetworkRequest request(url);
				QVariant alwaysNetwork(QNetworkRequest::AlwaysNetwork);

				LocalHostSecurityRequest *lh_request = new LocalHostSecurityRequest(sLocalHostSecurityInstance, domain);
				sLocalHostSecurityInstance->mRequestList.append(lh_request);
				ORIGIN_VERIFY_CONNECT(lh_request, SIGNAL(closeRequest(LocalHostSecurityRequest *)), sLocalHostSecurityInstance, SLOT(onCloseRequest(LocalHostSecurityRequest *)));

				lh_request->mNetworkReply = sLocalHostSecurityInstance->mNetworkAccessManager->get(request);
				ORIGIN_VERIFY_CONNECT_EX(lh_request->mNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)), lh_request, SLOT(onErrorReceived(QNetworkReply::NetworkError)), Qt::DirectConnection);
				ORIGIN_VERIFY_CONNECT(lh_request->mNetworkReply, SIGNAL(finished()), lh_request, SLOT(onEncrypted()));

				return lh_request;
			}

			void LocalHostSecurityRequest::onEncrypted()
			{
				ORIGIN_LOG_WARNING << "SSL Connected";

				ORIGIN_VERIFY_DISCONNECT(mNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onErrorReceived(QNetworkReply::NetworkError)));
				ORIGIN_VERIFY_DISCONNECT(mNetworkReply, SIGNAL(finished()), this, SLOT(onEncrypted()));

				// get certificate from server used to validate this connection
				QSslConfiguration config = mNetworkReply->sslConfiguration();
				QSslCertificate certificate = config.peerCertificate();
				QStringList org_name_list = certificate.issuerInfo(QSslCertificate::Organization);

				bool bypass_cert_validation = false;
				//if user has ODT and is in developer mode, allow "Origin" as the Issuer's name since we are using opqa-online.rws.ad.ea.com as our https server
				bool ODTenabled = Services::readSetting(Services::SETTING_OriginDeveloperToolEnabled).toQVariant().toBool();
				if (ODTenabled)
				{
					bypass_cert_validation = true;
					ORIGIN_LOG_WARNING << "ODT bypass used" << org_name_list.join('|') << " : " << certificate.issuerInfo(QSslCertificate::CommonName).join('|');
				}

				//validate the certificate of the calling site
				QList<QSslError> errors = QSslCertificate::verify(config.peerCertificateChain(), mDomain);
				qint32 numErrors = errors.length();
				qint32 i = 0;

				if (!bypass_cert_validation && numErrors)
				{
					//if we have errors lets loop through and record errors (QSslError enum)
					QString errorString;
					for (i = 0; i < numErrors; i++)
					{
						errorString += QString::number(errors.at(i).error());
						if (i < numErrors - 1)
						{
							errorString += ",";
						}
					}

					GetTelemetryInterface()->Metric_LOCALHOST_SECURITY_FAILURE(QString("Invalid certificate from %1 - Error(s):%2").arg(mDomain).arg(errorString).toLatin1());
					emit validationCompleted(false);
					emit closeRequest(this);
					return;
				}

				ulong bytes_available = static_cast<ulong>(mNetworkReply->bytesAvailable());
				char data[SECURITY_MAX_FILE_SIZE];
				if (bytes_available > (SECURITY_MAX_FILE_SIZE - 1))
					bytes_available = (SECURITY_MAX_FILE_SIZE - 1);
				mNetworkReply->read(data, bytes_available);
				data[bytes_available] = 0;

				QString json_data = QString(data).remove("[").remove("]").remove("\""); // remove [,],and " 

				QStringList feature_list = json_data.split(",");

				if (feature_list.count() == 0)
				{
					GetTelemetryInterface()->Metric_LOCALHOST_SECURITY_FAILURE(QString("No features secured from %1").arg(mDomain).toLatin1());
					emit validationCompleted(false);
					emit closeRequest(this);
					return;
				}

				sLocalHostSecurityInstance->verifyServices(mDomain, feature_list);

				emit validationCompleted(true);
				emit closeRequest(this);
			}

			void LocalHostSecurityRequest::onErrorReceived(QNetworkReply::NetworkError err)
			{
				ORIGIN_LOG_WARNING << "SSL error " << err;

				ORIGIN_VERIFY_DISCONNECT(mNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onErrorReceived(QNetworkReply::NetworkError)));
				ORIGIN_VERIFY_DISCONNECT(mNetworkReply, SIGNAL(finished()), this, SLOT(onEncrypted()));

				GetTelemetryInterface()->Metric_LOCALHOST_SECURITY_FAILURE(QString("Error %1 from %2").arg(err).arg(mDomain).toLatin1());

				emit validationCompleted(false);
				emit closeRequest(this);
			}

		}
	}
}
