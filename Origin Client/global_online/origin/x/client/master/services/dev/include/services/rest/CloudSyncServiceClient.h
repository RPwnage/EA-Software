#ifndef _CLOUDSYNCSERVICECLIENT_H
#define _CLOUDSYNCSERVICECLIENT_H

#include "OriginAuthServiceClient.h"
#include "CloudSyncServiceResponses.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Services
	{
		class ORIGIN_PLUGIN_API CloudSyncServiceClient : public OriginAuthServiceClient
		{
		public:
			friend class OriginClientFactory<CloudSyncServiceClient>;
			enum ManifestLockType
			{
				ReadLock,
				WriteLock,
			};

			enum OperationResult
			{
				SuccessfulResult,
				FailedResult
			};

			static CloudSyncManifestDiscoveryResponse *discoverManifest(Session::SessionRef session, const QString &cloudSavesId)
			{
				return OriginClientFactory<CloudSyncServiceClient>::instance()->discoverManifestPriv(session, cloudSavesId);
			}

			static CloudSyncSyncInitiationResponse *initiateSync(Session::SessionRef session, const QString &cloudSavesId, ManifestLockType lockType)
			{
				return OriginClientFactory<CloudSyncServiceClient>::instance()->initiateSyncPriv(session, cloudSavesId, lockType);
			}

			static CloudSyncLockExtensionResponse *extendLock(Session::SessionRef session, const CloudSyncServiceLock & lock)
			{
				return OriginClientFactory<CloudSyncServiceClient>::instance()->extendLockPriv(session, lock);
			}

			static CloudSyncLockPromotionResponse *promoteLock(Session::SessionRef session, const CloudSyncServiceLock & lock)
			{
				return OriginClientFactory<CloudSyncServiceClient>::instance()->promoteLockPriv(session, lock);
			}

			static CloudSyncUnlockResponse *unlock(Session::SessionRef session, const CloudSyncServiceLock & lock, OperationResult opresult)
			{
				return OriginClientFactory<CloudSyncServiceClient>::instance()->unlockPriv(session, lock, opresult);
			}

			static CloudSyncServiceAuthenticationResponse* authResponse(Session::SessionRef session, const QList<CloudSyncServiceAuthorizationRequest>& authReqList, const CloudSyncServiceLock & lock )
			{
				return OriginClientFactory<CloudSyncServiceClient>::instance()->authResponsePriv(session, authReqList, lock);
			}

			static CloudSyncLockTypeResponse *getLockType(Session::SessionRef session)
			{
				return OriginClientFactory<CloudSyncServiceClient>::instance()->getLockTypePriv(session);
			}

		private:
			explicit CloudSyncServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);


			CloudSyncManifestDiscoveryResponse *discoverManifestPriv(Session::SessionRef session, const QString &cloudSavesId);
			CloudSyncSyncInitiationResponse *initiateSyncPriv(Session::SessionRef session, const QString &cloudSavesId, ManifestLockType lockType);
			CloudSyncLockExtensionResponse *extendLockPriv(Session::SessionRef session, const CloudSyncServiceLock &);
			CloudSyncLockPromotionResponse *promoteLockPriv(Session::SessionRef session, const CloudSyncServiceLock &);
			CloudSyncUnlockResponse *unlockPriv(Session::SessionRef session, const CloudSyncServiceLock &, OperationResult);
			CloudSyncServiceAuthenticationResponse* authResponsePriv(Session::SessionRef session, const QList<CloudSyncServiceAuthorizationRequest>& authReqList, const CloudSyncServiceLock & lock );
			CloudSyncLockTypeResponse *getLockTypePriv(Session::SessionRef);
		};
	}
}

#endif