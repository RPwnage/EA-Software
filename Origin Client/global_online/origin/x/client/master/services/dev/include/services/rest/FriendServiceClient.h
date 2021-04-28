///////////////////////////////////////////////////////////////////////////////
// FriendServiceClient.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _FRIENDSERVICECLIENT_H
#define _FRIENDSERVICECLIENT_H

#include "OriginAuthServiceClient.h"
#include "FriendServiceResponses.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Services
	{
		///
		/// HTTP service client for the friends component of the Friends GCS API
		///
		class ORIGIN_PLUGIN_API FriendServiceClient : public OriginAuthServiceClient
		{
		public:
			friend class OriginClientFactory<FriendServiceClient>;

			///
			/// \brief Sends a friend invitation based on Nucleus ID
			///
			/// \param session        TBD.
			/// \param friendId       Nucleus ID of the invitee.
			/// \param comment        Optional invitation comment.
			/// \param emailTemplate  Optional name of the email template to use.
			/// \param source         Optional string idenitifying the source of the invite.
			///
			static OriginServiceResponse *inviteFriend(Session::SessionRef session, quint64 friendId, const QString &comment = QString(), const QString &emailTemplate = QString(), const QString &source = QString())
			{
				return OriginClientFactory<FriendServiceClient>::instance()->inviteFriendPriv(session, friendId,comment, emailTemplate, source);
			}

			///
			/// \brief Sends a friend invitation based on email
			///
			/// \param session        TBD.
			/// \param email          Email of the invitee
			/// \param comment        Optional invitation comment
			/// \param emailTemplate  Optional name of the email template to use
			/// \param source         Optional string idenitifying the source of the invite
			///
			static OriginServiceResponse *inviteFriendByEmail(Session::SessionRef session, const QString &email, const QString &comment = QString(), const QString &emailTemplate = QString(), const QString &source = QString())
			{
				return OriginClientFactory<FriendServiceClient>::instance()->inviteFriendByEmailPriv(session, email,comment,emailTemplate,source);
			}

			///
			/// \brief Confirms an invitation request
			///
			/// \param session        TBD.
			/// \param friendId       Nucleus ID of the inviter
			/// \param emailTemplate  Optional name of the email template to use
			///
			static UserActionResponse *confirmInvitation(Session::SessionRef session, quint64 friendId, const QString &emailTemplate = QString())
			{
				return OriginClientFactory<FriendServiceClient>::instance()->confirmInvitationPriv(session, friendId, emailTemplate);
			}

			///
			/// \brief Rejects an invitation request
			///
			/// \param session        TBD.
			/// \param friendId       Nucleus ID of the inviter
			/// \param emailTemplate  Optional name of the email template to use
			///
			static UserActionResponse *rejectInvitation(Session::SessionRef session, quint64 friendId, const QString &emailTemplate = QString())
			{
				return OriginClientFactory<FriendServiceClient>::instance()->rejectInvitationPriv(session,friendId,emailTemplate);
			}

			///
			/// \brief Retrieves a list of invitation requests 
			///
			/// \param session        TBD.
			static RetrieveInvitationsResponse *retrieveInvitations(Session::SessionRef session)
			{
				return OriginClientFactory<FriendServiceClient>::instance()->retrieveInvitationsPriv(session);
			}


			///
			/// \brief Retrieves a list of pending friends
			///
			/// \param session        TBD.
			static PendingFriendsResponse *retrievePendingFriends(Session::SessionRef session)
			{
				return OriginClientFactory<FriendServiceClient>::instance()->retrievePendingFriendsPriv(session);
			}

			///
			/// \brief Deletes a friend
			///
			/// \param session        TBD.
			/// \param friendId       Nucleus ID of the friend to be deleted
			///
			static UserActionResponse *deleteFriend(Session::SessionRef session, quint64 friendId)
			{
				return OriginClientFactory<FriendServiceClient>::instance()->deleteFriendPriv(session, friendId);
			}

			///
			/// \brief Blocks another user
			///
			/// \param session        TBD.
			/// \param userId         Nucleus ID of the user to block
			///
			static UserActionResponse *blockUser(Session::SessionRef session, quint64 userId)
			{
				return OriginClientFactory<FriendServiceClient>::instance()->blockUserPriv(session,userId);
			}

			///
			/// \brief Unblocks a previously blocked user
			///
			/// \param session        TBD.
			/// \param userId         Nucleus ID of the user to unblock
			///
			static UserActionResponse *unblockUser(Session::SessionRef session, quint64 userId)
			{
				return OriginClientFactory<FriendServiceClient>::instance()->unblockUserPriv(session, userId);
			}

			///
			/// \brief Retrieves a list of blocked users
			///
			/// \param session        TBD.
			static BlockedUsersResponse *blockedUsers(Session::SessionRef session)
			{
				return OriginClientFactory<FriendServiceClient>::instance()->blockedUsersPriv(session);
			}

			static OriginServiceResponse *inviteFriendFromUrl(Session::SessionRef session, QUrl url, const QString &comment, const QString &emailTemplate, const QString &source)
			{
				return OriginClientFactory<FriendServiceClient>::instance()->inviteFriendFromUrlPriv(session, url, comment, emailTemplate, source);
			}


		private:
			/// 
			/// \brief Creates a new friend service client
			///
			/// \param baseUrl       Base URL for the friends service API
			/// \param nam           QNetworkAccessManager instance to send requests through
			///
			explicit FriendServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

			///
			/// \brief Sends a friend invitation based on Nucleus ID
			///
			/// \param session        TBD.
			/// \param friendId       Nucleus ID of the invitee
			/// \param comment        Optional invitation comment
			/// \param emailTemplate  Optional name of the email template to use
			/// \param source         Optional string idenitifying the source of the invite
			///
			OriginServiceResponse *inviteFriendPriv(Session::SessionRef, quint64 friendId, const QString &comment = QString(), const QString &emailTemplate = QString(), const QString &source = QString());

			///
			/// \brief Sends a friend invitation based on email
			///
			/// \param session        TBD.
			/// \param email          Email of the invitee
			/// \param comment        Optional invitation comment
			/// \param emailTemplate  Optional name of the email template to use
			/// \param source         Optional string idenitifying the source of the invite
			///
			OriginServiceResponse *inviteFriendByEmailPriv(Session::SessionRef, const QString &email, const QString &comment = QString(), const QString &emailTemplate = QString(), const QString &source = QString());

			///
			/// \brief Confirms an invitation request
			///
			/// \param session        TBD.
			/// \param friendId       Nucleus ID of the inviter
			/// \param emailTemplate  Optional name of the email template to use
			///
			UserActionResponse *confirmInvitationPriv(Session::SessionRef session, quint64 friendId, const QString &emailTemplate = QString());

			///
			/// \brief Rejects an invitation request
			///
			/// \param session        TBD.
			/// \param friendId       Nucleus ID of the inviter
			/// \param emailTemplate  Optional name of the email template to use
			///
			UserActionResponse *rejectInvitationPriv(Session::SessionRef session, quint64 friendId, const QString &emailTemplate = QString());

			///
			/// \brief Retrieves a list of invitation requests 
			///
			/// \param session        TBD.
			RetrieveInvitationsResponse *retrieveInvitationsPriv(Session::SessionRef session);

			///
			/// \brief Retrieves a list of pending friends
			///
			/// \param session        TBD.
			PendingFriendsResponse *retrievePendingFriendsPriv(Session::SessionRef session);

			///
			/// \brief Deletes a friend
			///
			/// \param session        TBD.
			/// \param friendId       Nucleus ID of the friend to be deleted
			///
			UserActionResponse *deleteFriendPriv(Session::SessionRef session, quint64 friendId);

			///
			/// \brief Blocks another user
			///
			/// \param session        TBD.
			/// \param userId  Nucleus ID of the user to block
			///
			UserActionResponse *blockUserPriv(Session::SessionRef session, quint64 userId);

			///
			/// \brief Unblocks a previously blocked user
			///
			/// \param session        TBD.
			/// \param userId         Nucleus ID of the user to unblock
			///
			UserActionResponse *unblockUserPriv(Session::SessionRef session, quint64 userId);

			///
			/// \brief Retrieves a list of blocked users
			///
			/// \param session        TBD.
			BlockedUsersResponse *blockedUsersPriv(Session::SessionRef session);

			///
			/// \brief Invites friend from URL
			///
			/// \param session        TBD.
			/// \param url            TBD.
			/// \param comment        TBD.
			/// \param emailTemplate  TBD.
			/// \param source         TBD.
			OriginAuthServiceResponse *inviteFriendFromUrlPriv(Session::SessionRef session,QUrl url, const QString &comment, const QString &emailTemplate, const QString &source);
		};
	}
}

#endif
