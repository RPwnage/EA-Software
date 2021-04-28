#include "services/rest/CloudSyncServiceClient.h"
#include "services/rest/OriginServiceMaps.h"
#include "services/network/NetworkAccessManager.h"
#include "services/settings/SettingsManager.h"
#include <QDomDocument>

template <typename T, typename U, typename V>
QHash<U, V> PropertiesMap<T, U, V>::m_Hash;

namespace Origin
{
	namespace Services
	{
        /// 
        /// Helper to construct required XML elements for ORS
        /// \param body QDomDocument to build elements upon
        /// \param name option's name
        /// \param val options value
        /// 
        inline QDomElement element(QDomDocument& body, const QString& name, const QString& val)
        {
            QDomElement el = body.createElement(name);
            QDomText elemlvText = body.createTextNode(val);
            el.appendChild( elemlvText );
            return el;
        }

		CloudSyncServiceClient::CloudSyncServiceClient(const QUrl &baseUrl, NetworkAccessManager *nam)
			: OriginAuthServiceClient(nam)
		{
			if (!baseUrl.isEmpty())
				setBaseUrl(baseUrl);
			else
			{
				QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_CloudSyncAPIURL);
				setBaseUrl(QUrl(strUrl));
			}
		}

		CloudSyncLockTypeResponse *CloudSyncServiceClient::getLockTypePriv(Session::SessionRef session)
		{
			QUrl url(urlForServicePath("locktype/" + nucleusId(session)));

			QNetworkRequest req(url);
			req.setRawHeader("X-Origin-AuthToken", sessionToken(session).toUtf8());
			return new CloudSyncLockTypeResponse(getAuth(session,  req));
		}

		CloudSyncSyncInitiationResponse *CloudSyncServiceClient::initiateSyncPriv(Session::SessionRef session, const QString &cloudSavesId, ManifestLockType lockType)
		{
			QString urlLockPart = QString();

			if (lockType == ReadLock)
			{
				urlLockPart = "readlock";
			}
			else if (lockType == WriteLock)
			{
				urlLockPart = "writelock";
			}

			QString urlstring = urlLockPart + "/" + escapedNucleusId(session) + "/" + QUrl::toPercentEncoding(cloudSavesId);
			QUrl url(urlForServicePath(urlstring));

			QNetworkRequest req(url);
			req.setRawHeader(SyncAuthtoken, sessionToken(session).toLatin1());
			return new CloudSyncSyncInitiationResponse(postAuth(session,  req, QByteArray()));
		}


		CloudSyncManifestDiscoveryResponse *CloudSyncServiceClient::discoverManifestPriv(Session::SessionRef session, const QString &cloudSavesId)
		{
			QUrl url(urlForServicePath("manifest/" + escapedNucleusId(session) + "/" + QUrl::toPercentEncoding(cloudSavesId)));

			QNetworkRequest req(url);
			req.setRawHeader(SyncAuthtoken, sessionToken(session).toLatin1());
			return new CloudSyncManifestDiscoveryResponse(getAuth(session,  req));
		}

		CloudSyncLockExtensionResponse *CloudSyncServiceClient::extendLockPriv(Session::SessionRef session, const CloudSyncServiceLock &existing)
		{
			QUrl url(urlForServicePath("extendlock/" + escapedNucleusId(session)));
			QNetworkRequest req(url);
			req.setRawHeader(SyncAuthtoken, sessionToken(session).toLatin1());
			req.setRawHeader(SyncLockNameHeader, existing.lockName);
			return new CloudSyncLockExtensionResponse(putAuth(session,  req, QByteArray()), existing.lockName);
		}

		CloudSyncLockPromotionResponse *CloudSyncServiceClient::promoteLockPriv(Session::SessionRef session, const CloudSyncServiceLock &existing)
		{
			QUrl url(urlForServicePath("promotetowritelock/" + escapedNucleusId(session)));
			QNetworkRequest req(url);
			req.setRawHeader(SyncAuthtoken, sessionToken(session).toLatin1());
			req.setRawHeader(SyncLockNameHeader, existing.lockName);
			return new CloudSyncLockPromotionResponse(putAuth(session,  req, QByteArray()), existing.lockName);
		}

		CloudSyncUnlockResponse *CloudSyncServiceClient::unlockPriv(Session::SessionRef session, const CloudSyncServiceLock &toUnlock, OperationResult result)
		{
			QUrl serviceUrl(urlForServicePath("lock/" + escapedNucleusId(session)));
            QUrlQuery serviceUrlQuery(serviceUrl);

			if (result == SuccessfulResult)
			{
				serviceUrlQuery.addQueryItem("status", "commit");
			}
			else
			{
				serviceUrlQuery.addQueryItem("status", "rollback");
			}

            serviceUrl.setQuery(serviceUrlQuery);
            QNetworkRequest req(serviceUrl);
			req.setRawHeader(SyncAuthtoken, sessionToken(session).toLatin1());
			req.setRawHeader(SyncLockNameHeader, toUnlock.lockName);
			req.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
			return new CloudSyncUnlockResponse(deleteResourceAuth(session, req));
		}

		CloudSyncServiceAuthenticationResponse* CloudSyncServiceClient::authResponsePriv(Session::SessionRef session, const QList<CloudSyncServiceAuthorizationRequest>& authReqList, const CloudSyncServiceLock & lock )
		{
			QDomDocument body;
			QDomElement requests = body.createElement("requests");

			unsigned int index = 0;
			foreach(CloudSyncServiceAuthorizationRequest ar, authReqList)
			{
				QDomElement req = body.createElement("request");
				req.setAttribute("id", index++);
				req.appendChild(element(body, "verb", httpVerbMap().name(ar.verb()) ));

				if (!ar.resource().isEmpty())
				{
					req.appendChild(element(body, "resource", ar.resource()));
				}

				if (!ar.md5().isEmpty())
				{
					req.appendChild(element(body, "md5", ar.md5().toBase64()));
				}

				if (!ar.contentType().isEmpty())
				{
					req.appendChild(element(body, "content-type", ar.contentType()));
				}

				requests.appendChild(req);
			}
			body.appendChild(requests);

			QUrl serviceUrl(urlForServicePath("authorize/" + escapedNucleusId(session)));

			QNetworkRequest req(serviceUrl);
			req.setRawHeader(SyncAuthtoken, sessionToken(session).toLatin1());
			req.setRawHeader(SyncLockNameHeader, lock.lockName);
			req.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");

			return new CloudSyncServiceAuthenticationResponse(putAuth(session,  req, body.toByteArray()), authReqList, lock);
		}
	}
}

