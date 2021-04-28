#ifndef _CONVERSATIONMANAGER_H
#define _CONVERSATIONMANAGER_H

// Copyright 2012, Electronic Arts
// All rights reserved.

#include <QObject>
#include <QSet>
#include <QQueue>
#include <QSharedPointer>
#include <QMutex>
#include <QAtomicInt>

#include <chat/Message.h>
#include <chat/Conversable.h>
#include <chat/ConnectedUser.h>
#include <chat/Connection.h>

#include "engine/social/Conversation.h"
#include "services/plugin/PluginAPI.h"
#include "talk/xmpp/chatroommodule.h"

namespace Origin
{
namespace Chat
{
    class Connection;
    class JabberID;
}

namespace Engine
{
namespace MultiplayerInvite
{
    class InviteController;
    class Invite;
}
namespace Voice
{
    class VoiceCall;
}

namespace Social
{
    class SocialController;
    class ConversationEvent;

    /// \brief Manages tracking and creating active conversations
    class ORIGIN_PLUGIN_API ConversationManager : public QObject
    {
        friend class Conversation;

        Q_OBJECT
    public:
        ///
        /// \brief Creates a new conversation controller
        ///
        /// \param socialController  Social controller to listen for messages from. If a conversation matching an
        ///                          incoming message isn't found a new conversation will be created
        /// \param voiceController   Voice controller to listen for events from.
        ///
        ConversationManager(SocialController *socialController);

        ~ConversationManager();

        /// \brief Returns true if there is a conversation for the given conversable
        bool existsConversationForConversable(Chat::Conversable& conversable) const;

        ///
        /// \brief Creates a new conversation for the given conversable or returns an exisiting unfinished conversation.
        ///
        /// conversationCreated() will fire if the conversation has been newly created.
        ///
        /// \param conversable TBD.
        /// \param reason  Originating source of the conversation. Must be one of CreationReason enums.
        ///
        ///
        QSharedPointer<Conversation> conversationForConversable(Chat::Conversable *, Conversation::CreationReason);

        /// \brief Returns a list of all conversations
        QList<QSharedPointer<Conversation> > allConversations() const;
        
        ///
        /// \brief Returns if the implementation currently supports multi-user chat conversations
        ///
        /// The Origin XMPP server currently doesn't support multi-user conversations while invisible. Whenever the 
        /// connected user's visibility changes multiUserChatCapabilityChanged() will be emitted and this method will 
        /// will return the appropriate value
        ///
        bool multiUserChatCapable() const;

        /// \brief Returns true if there are any active conversations requiring multi-user features
        bool hasMultiUserDependentConversations();
        
        ///
        /// \brief Finishes all conversations requiring multi-user features
        ///
        /// This includes multi-user conversations, 1:1 chats undergoing upgrades and incoming pending multi-user chat
        /// invites.
        ///
        void finishMultiUserDependentConversations(Conversation::FinishReason reason);

        ///
        /// \brief Finishes all active conversations
        ///
        void finishAllConversations(Conversation::FinishReason reason);

    signals:
        ///
        /// \brief Emitted whenever a new conversation has been created
        ///
        /// The user interface is expected to present new conversations to the user. These may either be conversations
        /// manually requested with conversationForConversable or ones implicitly created on receiving a new message.
        ///
        /// Conversations finishing can be detected with Conversation::conversationFinished()
        ///
        /// \param CreationReason  Originating source of the conversation. Must be one of \ref CreationReason enums.
        ///
        ///
        void conversationCreated(QSharedPointer<Origin::Engine::Social::Conversation>);

        ///
        /// \brief Emitted whenever the value returned by multiUserChatCapable() changes
        ///
        /// This currently happens when transitioning the connected user's visibility
        ///
        void multiUserChatCapabilityChanged(bool capable);

        ///
        /// \brief Emitted when we receive an incoming message with a group and room name
        ///
        /// This currently happens in onMessageReceived after checking the conversation for names
        ///
        void groupChatMessageReceived(QSharedPointer<Origin::Engine::Social::Conversation>, const Origin::Chat::Message &);

        ///
        /// \brief Emitted when we receive a signal that a room the user is in has been destroyed
        ///
        /// This currently happens in onRoomDestroyedBy after checking the roster has loaded
        ///
        void groupRoomDestroyed(QSharedPointer<Origin::Engine::Social::Conversation>, const QString&);

        ///
        /// \brief Emitted when we receive a signal that the user was kicked from a room
        ///
        /// This currently happens in onKickedFromRoom after checking the roster has loaded
        ///
        void kickedFromRoom(QSharedPointer<Origin::Engine::Social::Conversation>, const QString&);

        void mutedByAdmin(QSharedPointer<Origin::Engine::Social::Conversation>, const Origin::Chat::JabberID&);

        void enterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus);
    private slots:
        void onConversationFinished();
        void onConversationConversableChanged(Origin::Chat::Conversable *newConversable, Origin::Chat::Conversable *previousConversable);

        void onMessageReceived(Origin::Chat::Conversable *, const Origin::Chat::Message &);
        void onRosterLoaded();
        void onMultiplayerInviteSent(const Origin::Engine::MultiplayerInvite::Invite &invite);

        void onJoiningMucRoom(Origin::Chat::MucRoom* room, const QString& roomId, const QString& password, const QString& roomName, const bool leaveRoomOnJoin, const bool failSilently);
        void onRejoiningMucRoom(Origin::Chat::MucRoom* room, const QString& roomId);
        void onRoomDestroyedBy(Origin::Chat::MucRoom*, const QString&);
        void onKickedFromRoom(Origin::Chat::MucRoom*, const QString&);
        void onEnteredCreatedMucRoom(Origin::Chat::MucRoom *);
        void onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus = Origin::Chat::CHATROOM_ENTERED_FAILURE_UNSPECIFIED);

        void onVisibilityChanged(Origin::Chat::ConnectedUser::Visibility);

        void onEventAdded(const Origin::Engine::Social::ConversationEvent*);

    private:
        // Friend functions
        int allocateMultiUserChatSequenceNumber(); 

    private:
        void connectConversationSignals(QSharedPointer<Conversation> conv);

        typedef QHash<Chat::Conversable *, QSharedPointer<Conversation> > ActiveConversationHash;

        SocialController *mSocialController;
        ActiveConversationHash mActiveConversations;
        QQueue<Chat::Message> mRosterLoadQueuedMessages;
        mutable QMutex mConversationMutex;

        QAtomicInt mMultiUserChatSequence;
    };

}
}
}

#endif

