#ifndef LOCALHOST_PINGSERVERSERVICE_H
#define LOCALHOST_PINGSERVERSERVICE_H

#include "LocalHost/LocalHostServices/IService.h"
#include <QTimer>
#include <QStringList>
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protoping.h"

#define MAX_PING_PER_REQUEST 20 // set by DICE in accordance with SRM

namespace Origin
{
	namespace Sdk
	{
		namespace LocalHost
		{
			class PingServerService : public IService
			{
				Q_OBJECT

			public:
				PingServerService(QObject *parent) :IService(parent), mAllowedRequests(0), mRequestActive(false) { mMethod = "POST"; };
				~PingServerService() {};

				virtual ResponseInfo processRequest(QUrl requestUrl);

				// treat as asynchronous because ping service needs to be asynchronous
				virtual bool isAsynchronousService() { return true; }

				// treat as secure because we need to prevent ping service from being used for malicious purposes
				virtual bool isSecureService() { return true; }

				// getSecurityBitPosition returns bit position index of this service's security mask
				virtual int getSecurityBitPosition() { return SECURE_SERVICE_BIT_PINGSERVER; }

				protected slots:
				void onTimerInterval();
			private:

				void executePingRequest();

				QTimer mPollingTimer;

				ProtoPingRefT *mProtoPingRef;

				QStringList mUrlQueue;  // to queue up requests - may need to add a cap

				int mAllowedRequests;   // counter to countdown number of pings allowed before new auth token is sent.

				int mNumIPs;
				int mIPResultsReturned;	// counter to make sure all IP ping requests either returned a time or timed out - Protoping may write to memory after ProtoPingDestroy() is called
				bool mRequestActive;	// flag to say the current request is still active - queue up any new requests.
				bool mExtraInterval;	// flag to say, we are going to wait an extra second to clear out (time out) any remaining requests so we don't get wild writes.
				uint32_t mIPAddresses[MAX_PING_PER_REQUEST];    // 111.222.333.444 -> 111 << 24 | 222 << 16 | 333 << 8 | 444
				QString mIPAddrStrings[MAX_PING_PER_REQUEST];
				int mIPPingResults[MAX_PING_PER_REQUEST];   // -1 means timed out
			};
		}
	}
}

#endif