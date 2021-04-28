///////////////////////////////////////////////////////////////////////////////
// PrivacyServiceClientResponses.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _PRIVACYSERVICESRESPONSES_H
#define _PRIVACYSERVICESRESPONSES_H

#include "OriginAuthServiceResponse.h"
#include "PrivacyServiceClient.h"
#include "PropertiesMap.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Services
	{
		///
		/// Represents the result from querying profile visibility
		///
		class ORIGIN_PLUGIN_API PrivacyVisibilityResponse : public OriginAuthServiceResponse
		{
		public:
			explicit PrivacyVisibilityResponse(AuthNetworkRequest);

			///
			/// Retrieve current visibility setting
			///
			visibility visibilitySetting() const { return m_visibilitySetting; }

		private:
			visibility m_visibilitySetting;
			bool parseSuccessBody(QIODevice *body);
		};

		///
		/// Represents the visibility if the user's multiple friends
		///
		class ORIGIN_PLUGIN_API PrivacyFriendsVisibilityResponse : public OriginAuthServiceResponse
		{
		public:
			explicit PrivacyFriendsVisibilityResponse(AuthNetworkRequest);
		private:
			bool parseSuccessBody(QIODevice *body);
		};

		class ORIGIN_PLUGIN_API PrivacyGetUserId  : public OriginServiceResponse
		{
		public:
			explicit PrivacyGetUserId(QNetworkReply* reply);
			const quint64 userId() const { return m_userId; }
		private:
			bool parseSuccessBody(QIODevice *body);
			quint64 m_userId;
		};

		///
		/// Represents the result from querying friend's profile visibility
		///
		class ORIGIN_PLUGIN_API PrivacyFriendVisibilityResponse : public OriginAuthServiceResponse
		{
		public:
			explicit PrivacyFriendVisibilityResponse(AuthNetworkRequest);

			///
			/// Retrieve whether friend is visible
			/// 
			bool visibilityFriend() const { return m_visibilityFriend; }

		private:
			bool m_visibilityFriend;
			bool parseSuccessBody(QIODevice *body);
		};
	}
}

#endif
