///////////////////////////////////////////////////////////////////////////////
// PrivacyServiceClient.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _PRIVACYSERVICECLIENT_H
#define _PRIVACYSERVICECLIENT_H

#include "OriginAuthServiceClient.h"
#include "OriginServiceResponse.h"
#include "OriginServiceMaps.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Services
	{
		class PrivacyFriendsVisibilityResponse;
		class PrivacyFriendVisibilityResponse;
		class PrivacyVisibilityResponse;

		///
		/// HTTP service client for the privacy component of the Friends GCS API 
		///
		class ORIGIN_PLUGIN_API PrivacyServiceClient : public OriginAuthServiceClient
		{
		public:
			friend class OriginClientFactory<PrivacyServiceClient>;

			///
			/// \brief Checks that the specified friends profiles are visible to the specified user.
			/// \param session     TBD.
			/// \param friendsIds  The user's friends Ids, in a string, separated by semi-colons ';'.
			///
			static PrivacyFriendsVisibilityResponse* friendsProfileVisibility(Session::SessionRef session, const QList<quint64>& friendsIds)
			{
				return OriginClientFactory<PrivacyServiceClient>::instance()->friendsProfileVisibilityPriv(session, friendsIds);
			}

			///
			/// \brief Checks that the specified friend's profile is visible to the current user.
			/// Returns true if it is possible.
			///
			/// \param session   TBD.
			/// \param friendId  The id of the friend we want to query visibility from.
			///
			static PrivacyFriendVisibilityResponse *isFriendProfileVisibility(Session::SessionRef session, quint64 friendId)
			{
				return OriginClientFactory<PrivacyServiceClient>::instance()->isFriendProfileVisibilityPriv(session, friendId);
			}

			///
			/// \brief Gets the current user's friend's rich presence visibility setting
			///
			/// \param session   TBD.
			/// \param friendId  The id of the friend we want to query visibility from.
			///
			static PrivacyFriendVisibilityResponse * friendRichPresencePrivacy(Session::SessionRef session, quint64 friendId)
			{
				return OriginClientFactory<PrivacyServiceClient>::instance()->friendRichPresencePrivacyPriv(session, friendId);
			}

			///
			/// \brief Retrieves the user's rich presence privacy setting.
			///
			/// \param session  TBD
			///
			static PrivacyVisibilityResponse * richPresencePrivacy(Session::SessionRef session)
			{
				return OriginClientFactory<PrivacyServiceClient>::instance()->richPresencePrivacyPriv(session);
			}

			///
			/// \brief Gets the current user's profile privacy setting.
			///
			static PrivacyVisibilityResponse *profilePrivacy(Session::SessionRef session)
			{
				return OriginClientFactory<PrivacyServiceClient>::instance()->profilePrivacyPriv(session);
			}

			/// 
			/// \brief Updates the current user's profile privacy settings.
			/// \param session  TBD.
			/// \param MyVisibility The setting to update to.
			///
			static PrivacyVisibilityResponse *updateProfilePrivacy(Session::SessionRef session,visibility MyVisibility)
			{
				return OriginClientFactory<PrivacyServiceClient>::instance()->updateProfilePrivacyPriv(session, MyVisibility);
			}

			/// 
			/// \brief Updates the current user's rich presence privacy setting.
			/// \param session TBD.
			/// \param MyVisibility The setting to update to.
			///
			static PrivacyVisibilityResponse *updateRichPresence(Session::SessionRef session,visibility MyVisibility)
			{
				return OriginClientFactory<PrivacyServiceClient>::instance()->updateRichPresencePriv(session, MyVisibility);
			}

		private:
			/// 
			/// \brief Creates a new privacy service client.
			///
			/// \param baseUrl       Base URL for the friends service API.
			/// \param nam           QNetworkAccessManager instance to send requests through.
			///
			explicit PrivacyServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

			///
			/// \brief Checks that the specified friends profiles are visible to the specified user.
			/// \param session    TBD.
			/// \param friendsIds The user's friends Ids, in a string, separated by semi-colons ';'.
			///
			PrivacyFriendsVisibilityResponse* friendsProfileVisibilityPriv(Session::SessionRef session, const QList<quint64>& friendsIds);

			/// 
			/// \brief Docs go KA-BOOM! here
			///

			///
			/// \brief Checks that the specified friend's profile is visible to the current user.
			/// Returns true if it is possible.
			///
			/// \param session   TBD.
			/// \param friendId  The id of the friend we want to query visibility from.
			///
			PrivacyFriendVisibilityResponse *isFriendProfileVisibilityPriv(Session::SessionRef session, quint64 friendId);

			///
			/// \brief Gets the current user's friend's rich presence visibility setting
			///
			/// \param session   TBD.
			/// \param friendId  The id of the friend we want to query visibility from.
			///
			PrivacyFriendVisibilityResponse * friendRichPresencePrivacyPriv(Session::SessionRef session, quint64 friendId);

			///
			/// \brief Retrieves the user's rich presence privacy setting.
			///
			/// \param session  TBD.
			///
			PrivacyVisibilityResponse * richPresencePrivacyPriv(Session::SessionRef session);

			///
			/// \brief Gets the current user's profile privacy setting.
			///
			PrivacyVisibilityResponse *profilePrivacyPriv(Session::SessionRef session);

			/// 
			/// \brief Updates the current user's profile privacy settings.
			///
			/// \param session  TBD.
			/// \param MyVisibility The setting to update to.
			///
			PrivacyVisibilityResponse *updateProfilePrivacyPriv(Session::SessionRef session, visibility MyVisibility);

			/// 
			/// \brief Updates the current user's rich presence privacy setting.
			/// \param session TBD.
			/// \param MyVisibility The setting to update to.
			///
			PrivacyVisibilityResponse *updateRichPresencePriv(Session::SessionRef session, visibility MyVisibility);

			//////////////////////////////////////////////////////////////////////////
			/// HELPERS

			///
			/// Gets the current user's profile visibility setting value as string.
			///
			/// \param MyVisibility  The visibility setting we want to retrieve the string from.
			///
			static const QByteArray visibilitySetting(visibility MyVisibility);
		};

	}
}

#include "PrivacyServiceResponses.h"

#endif
