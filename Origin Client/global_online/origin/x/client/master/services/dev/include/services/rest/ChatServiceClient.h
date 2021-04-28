///////////////////////////////////////////////////////////////////////////////
// ChatServiceClient.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _CHATSERVICECLIENT_H
#define _CHATSERVICECLIENT_H

#include "OriginAuthServiceClient.h"
#include "ChatServiceResponse.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Services
	{
		///
		/// \brief HTTP service client for the Chat component of the Chat GCS API
		///
		class ORIGIN_PLUGIN_API ChatServiceClient : public OriginAuthServiceClient
		{
		public:
			friend class OriginClientFactory<ChatServiceClient>;

			///
			/// \brief Checks if a user is online
			///
			static UserOnlineResponse *isUserOnline(Session::SessionRef session)
			{
				return OriginClientFactory<ChatServiceClient>::instance()->isUserOnlinePriv(session);
			}

		private:
			/// 
			/// \brief Creates a new chat service client
			///
			/// \param baseUrl       Base URL for the friends service API.
			/// \param nam           QNetworkAccessManager instance to send requests through.
			///
			explicit ChatServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

			///
			/// \brief Checks if a user is online
			///
			UserOnlineResponse *isUserOnlinePriv(Session::SessionRef);
		};
	}
}

#endif //_CHATSERVICECLIENT_H