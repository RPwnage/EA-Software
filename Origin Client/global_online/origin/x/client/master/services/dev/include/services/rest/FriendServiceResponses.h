///////////////////////////////////////////////////////////////////////////////
// FriendServiceResponses.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _FRIENDSERVICERESPONSES_H
#define _FRIENDSERVICERESPONSES_H

#include "OriginAuthServiceResponse.h"

#include "services/plugin/PluginAPI.h"

struct IncomingInvitation
{
	quint64 nucleusId;
	QString comment;
};

struct BlockedUser
{
	quint64 nucleusId;
	quint64 personaId;

	QString email;
	QString name;
};

namespace Origin
{
	namespace Services
	{

		///
		/// Encapsulate repsonses for actions on other users
		///
		class ORIGIN_PLUGIN_API UserActionResponse : public OriginAuthServiceResponse
		{
		public:
			UserActionResponse(quint64 userId, AuthNetworkRequest reply) : OriginAuthServiceResponse(reply), m_userId(userId)
			{
			}

			///
			/// Returns the user ID the request was acting upon
			///
			quint64 userId() const { return m_userId; }

		protected:
			quint64 m_userId;
		};

		class ORIGIN_PLUGIN_API RetrieveInvitationsResponse : public OriginAuthServiceResponse
		{
		public:
			RetrieveInvitationsResponse(AuthNetworkRequest);

			QList<IncomingInvitation> invitations() const { return m_invitations; }

		private:
			QList<IncomingInvitation> m_invitations;
			bool parseSuccessBody(QIODevice *body);
		};

		class ORIGIN_PLUGIN_API PendingFriendsResponse : public OriginAuthServiceResponse
		{
		public:
			PendingFriendsResponse(AuthNetworkRequest);
			QList<quint64> pendingFriends() const { return m_pendingFriends; }

		private:
			QList<quint64> m_pendingFriends;
			bool parseSuccessBody(QIODevice *body);
		};

		class ORIGIN_PLUGIN_API BlockedUsersResponse : public OriginAuthServiceResponse
		{
		public:
			BlockedUsersResponse(AuthNetworkRequest);
			const QList<BlockedUser>& blockedUsers() const { return m_blockedUsers; }

		private:
			QList<BlockedUser> m_blockedUsers;
			bool parseSuccessBody(QIODevice *body);
		};
	}
}


#endif
