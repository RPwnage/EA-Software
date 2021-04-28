#ifndef XMPP_SERVICEAREA_H__
#define XMPP_SERVICEAREA_H__

///////////////////////////////////////////////////////////////////////////////|
//
// Copyright (C) 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////|
/**
 * @file GCS_XMPP_EALS/XMPP_ServiceArea.h
 *
 * $Id$
 *
 * @author Carlos Mellano <CMellano@contractor.ea.com>
 *
 * @brief Header for XMPP_ServiceArea.
 */

/**
 * @page commands LSX Commands
 */

#include "Service/BaseService.h"
#include "engine/social/ConversationManager.h"
#include "engine/login/User.h"
#include "chat/ConnectedUser.h"

#include <QMap>
#include <QSet>

#include "lsx.h"

namespace Origin
{
    namespace Chat
    {
        class Message;
        class JabberID;
        class Conversable;
        class MucRoom;
    }
    namespace Engine
    {
        namespace MultiplayerInvite
        {
            class Invite;
        }
    }
}

namespace Origin
{
    namespace SDK
    {
        /// XMPP LSX facade.
        /**
         * Extends EALS_ServiceArea to send and receive LSX commands from a
         * client.
         */
        class XMPP_ServiceArea : public BaseService
        {
            Q_OBJECT
        public:
            virtual ~XMPP_ServiceArea();

            // Singleton functions
            static XMPP_ServiceArea* instance();
            static XMPP_ServiceArea* create(Lsx::LSX_Handler* handler);
            void destroy();

        private:
            XMPP_ServiceArea( Lsx::LSX_Handler* handler );
            XMPP_ServiceArea();

            // Handlers
            //void Request_SendChatMessage(Lsx::Request &request, Lsx::Response &response);
    
            void Request_SubscribePresence(Lsx::Request &request, Lsx::Response &response);	
            void Request_UnsubscribePresence(Lsx::Request &request, Lsx::Response &response);	

            void Request_GetPresence(Lsx::Request &request, Lsx::Response &response);
            void Request_SetPresence(Lsx::Request &request, Lsx::Response &response);

            void Request_GetPresenceVisibility(Lsx::Request &request, Lsx::Response &response);
            void Request_SetPresenceVisibility(Lsx::Request &request, Lsx::Response &response);

            void CreateErrorResponse( int code, const char * description, Lsx::Response &response );

            void Request_QueryPresence(Lsx::Request &request, Lsx::Response &response);
            void Request_QueryFriends(Lsx::Request &request, Lsx::Response &response);
            void Request_QueryAreFriends(Lsx::Request &request, Lsx::Response &response);

            void Request_SendInvite(Lsx::Request &request, Lsx::Response &responseMiddleWare);
            void Request_AcceptInvite(Lsx::Request &request, Lsx::Response &responseMiddleWare);

            void Request_SendChat(Lsx::Request &request, Lsx::Response &response);
            void Request_QueryChatInfo(Lsx::Request &request, Lsx::Response &response);
            void Request_QueryChatUsers(Lsx::Request &request, Lsx::Response &response);
            void Request_LeaveChat(Lsx::Request &request, Lsx::Response &response);
            void Request_JoinChat(Lsx::Request &request, Lsx::Response &response);
            void Request_CreateChat(Lsx::Request &request, Lsx::Response &response);
            void Request_AddUserToChat(Lsx::Request &request, Lsx::Response &response);

            void Request_BlockUser(Lsx::Request &request, Lsx::Response &response);
            void Request_UnblockUser(Lsx::Request &request, Lsx::Response &response);

			void Request_AcceptFriendInvite(Lsx::Request &request, Lsx::Response &response);
            void Request_RequestFriend(Lsx::Request &request, Lsx::Response &response);
            void Request_RemoveFriend(Lsx::Request &request, Lsx::Response &response);

            bool hasLegacyConnection() const;
            bool isLegacyConnection(const Lsx::Connection * connection) const;

        private slots:
            void connectionHungup(const QString &);
            void userLoggedIn(Origin::Engine::UserRef);
            void presenceChanged(const Origin::Chat::Presence &presence);
            void visibilityChanged(Origin::Chat::ConnectedUser::Visibility);
            void messageReceived(Origin::Chat::Conversable *, const Origin::Chat::Message &);
    
            void invitedToRemoteSession(const Origin::Engine::MultiplayerInvite::Invite &invite);
            void sendSessionToConnectedGame(const Origin::Engine::MultiplayerInvite::Invite &invite, bool initial);

           void onConversationCreated(QSharedPointer<Origin::Engine::Social::Conversation>);
           void onConversationFinished();
           void onConversationUpdated(const Origin::Engine::Social::ConversationEvent* event);

           void onRosterUpdated();

           void subscribeToContact(Origin::Chat::RemoteUser* contact);
           void unsubscribeToContact(Origin::Chat::RemoteUser* contact);
           void onContactPresenceChanged(Origin::Chat::Presence, Origin::Chat::Presence);

        private:
            QMap<QString, QWeakPointer<Engine::Social::Conversation> > mConversations;
        };
    }
}
#endif // XMPP_ServiceArea_h__
