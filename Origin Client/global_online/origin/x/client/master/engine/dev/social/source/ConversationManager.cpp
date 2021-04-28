#include <chat/ChatChannel.h>
#include <chat/ChatGroup.h>
#include <chat/Connection.h>
#include <chat/OriginConnection.h>
#include <chat/ConnectedUser.h>
#include <chat/Roster.h>
#include <chat/MucRoom.h>
#include <chat/NucleusID.h>
#include <chat/JabberID.h>

#include <services/debug/DebugService.h>
#include <services/log/LogService.h>
#include "services/voice/VoiceService.h"

#include <QMutexLocker>

#include "TelemetryAPIDLL.h"

#include "engine/igo/IGOController.h"
#include "engine/social/ConversationManager.h"
#include "engine/social/SocialController.h"
#include "engine/social/UserNamesCache.h"
#include "engine/multiplayer/Invite.h"
#include "engine/multiplayer/InviteController.h"
#include "engine/social/ConversationEvent.h"

#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Engine
{
namespace Social
{
    ConversationManager::ConversationManager(SocialController *socialController) :
        mSocialController(socialController),
        mMultiUserChatSequence(1)
    {
        Chat::Connection *connection = socialController->chatConnection();

        // REMOVE FOR ORIGIN X
        //ORIGIN_VERIFY_CONNECT(
        //        connection, SIGNAL(messageReceived(Origin::Chat::Conversable *, const Origin::Chat::Message &)),
        //        this, SLOT(onMessageReceived(Origin::Chat::Conversable *, const Origin::Chat::Message &)));

        ORIGIN_VERIFY_CONNECT(
                connection, SIGNAL(joiningMucRoom(Origin::Chat::MucRoom*, const QString&, const QString&, const QString&, const bool, const bool)),
                this, SLOT(onJoiningMucRoom(Origin::Chat::MucRoom*, const QString&, const QString&, const QString&, const bool, const bool)));

        ORIGIN_VERIFY_CONNECT(
            connection, SIGNAL(rejoiningMucRoom(Origin::Chat::MucRoom*, const QString&)),
            this, SLOT(onRejoiningMucRoom(Origin::Chat::MucRoom*, const QString&)));

        ORIGIN_VERIFY_CONNECT(
                connection, SIGNAL(roomDestroyedBy(Origin::Chat::MucRoom*, const QString&)),
                this, SLOT(onRoomDestroyedBy(Origin::Chat::MucRoom*, const QString&)));

        ORIGIN_VERIFY_CONNECT(
                connection, SIGNAL(kickedFromRoom(Origin::Chat::MucRoom*, const QString&)),
                this, SLOT(onKickedFromRoom(Origin::Chat::MucRoom*, const QString&)));

        // Queue this so initial presence won't show up as a presence change notification in the new conversations
        // created by replaying mRosterLoadQueuedMessages 
        ORIGIN_VERIFY_CONNECT_EX(
                connection->currentUser()->roster(), SIGNAL(loaded()),
                this, SLOT(onRosterLoaded()), Qt::QueuedConnection);

        ORIGIN_VERIFY_CONNECT(
                socialController->multiplayerInvites(), SIGNAL(invitedToRemoteSession(const Origin::Engine::MultiplayerInvite::Invite &)),
                this, SLOT(onMultiplayerInviteSent(const Origin::Engine::MultiplayerInvite::Invite &)));

        ORIGIN_VERIFY_CONNECT(
                connection->currentUser(), SIGNAL(visibilityChanged(Origin::Chat::ConnectedUser::Visibility)),
                this, SLOT(onVisibilityChanged(Origin::Chat::ConnectedUser::Visibility)));
        
        // Don't hook up InviteController::sentInviteToLocalSession
        // JavaScript doesn't render outgoing invites so we'll end up opening a conversation window with no content
    }

    ConversationManager::~ConversationManager()
    {
        // If finishAllConversations hasn't been explicitly called assume this is forcible
        finishAllConversations(Conversation::ForciblyFinished);
    }

    bool ConversationManager::existsConversationForConversable(Chat::Conversable& conversable) const
    {
        QMutexLocker locker(&mConversationMutex);
        return mActiveConversations.contains(&conversable);
    }

    QSharedPointer<Conversation> ConversationManager::conversationForConversable(Chat::Conversable *conversable, Conversation::CreationReason reason)
    {
        QMutexLocker locker(&mConversationMutex);

        // Do we have an existing conversation?
        if (mActiveConversations.contains(conversable))
        {
#if ENABLE_VOICE_CHAT
            bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
            if( isVoiceEnabled )
            {
                // Check to see if we should be starting a voice call
                if (reason == Conversation::StartVoice)
                {
                    mActiveConversations[conversable]->joinVoice();
                }
            }
#endif // ENABLE_VOICE_CHAT
            return mActiveConversations[conversable];
        }

        // Create the conversation
        QSharedPointer<Conversation> conversation(new Conversation(mSocialController, conversable, reason), &Conversation::deleteLater);

        ORIGIN_VERIFY_CONNECT(conversation.data(), SIGNAL(conversationFinished(Origin::Engine::Social::Conversation::FinishReason)),
                this, SLOT(onConversationFinished()));
        
        ORIGIN_VERIFY_CONNECT(
                conversation.data(), SIGNAL(conversableChanged(Origin::Chat::Conversable *, Origin::Chat::Conversable *)),
                this, SLOT(onConversationConversableChanged(Origin::Chat::Conversable *, Origin::Chat::Conversable *)));

        ORIGIN_VERIFY_CONNECT(conversation.data(), SIGNAL(eventAdded(const Origin::Engine::Social::ConversationEvent*)),
            this, SLOT(onEventAdded(const Origin::Engine::Social::ConversationEvent*)));

        mActiveConversations.insert(conversable, conversation);
        locker.unlock();

        emit conversationCreated(conversation);

        // EBIBUGS-24062: Do not send a 'Chat Start' message when starting a Group Chat
        if (!dynamic_cast<Chat::MucRoom*>(conversable))
        {
            uint64_t nucleusId = 0;
            if(reason == Conversation::IncomingMessage)
            {
                Chat::Connection *connection = NULL;
                Chat::RemoteUser *user = NULL;
                connection = mSocialController->chatConnection();
                if(connection != NULL)
                    user = connection->remoteUserForJabberId(conversable->jabberId());
                if(user != NULL)
                    nucleusId = user->nucleusId();
            }
            bool isInIgo = IGOController::instance()->isActive();
            GetTelemetryInterface()->Metric_CHAT_SESSION_START(isInIgo, nucleusId);
        }

#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( isVoiceEnabled )
        {
            if (reason == Conversation::StartVoice)
            {
                conversation->joinVoice();
            }
        }
#endif // ENABLE_VOICE_CHAT

        return conversation; 
    }
        
    QList<QSharedPointer<Conversation> > ConversationManager::allConversations() const
    {
        QMutexLocker locker(&mConversationMutex);
        return mActiveConversations.values();
    }

    void ConversationManager::onConversationFinished()
    {
        Conversation *conversation = dynamic_cast<Conversation*>(sender());

        if (conversation)
        {
            // Remove this from the list of active conversations and invites
            // This also releases our shared pointer reference
            QMutexLocker locker(&mConversationMutex);
            mActiveConversations.remove(conversation->conversable());
        
			bool isInIgo = IGOController::instance()->isActive();
			if (conversation->type() == Conversation::GroupType)
				GetTelemetryInterface()->Metric_CHATROOM_EXIT_ROOM(conversation->getChannelId().toLatin1(), conversation->getGroupGuid().toLocal8Bit(), isInIgo);
			else
				GetTelemetryInterface()->Metric_CHAT_SESSION_END(isInIgo);
        }
    }
        
    void ConversationManager::onConversationConversableChanged(Chat::Conversable *newConversable, Chat::Conversable *previousConversable)
    {
        // Re-index the conversation under the new conversable
        QMutexLocker locker(&mConversationMutex);
        QSharedPointer<Conversation> conv = mActiveConversations.take(previousConversable);

        if (conv)
        {
            mActiveConversations.insert(newConversable, conv);
        }
        else
        {
            ORIGIN_LOG_DEBUG << "Conversable changed for a conversation we didn't know about";
        }
    }
        
    void ConversationManager::onMessageReceived(Origin::Chat::Conversable *conversable, const Origin::Chat::Message &message)
    {
        Chat::Connection *connection = mSocialController->chatConnection();

        if (!connection->currentUser()->roster()->hasLoaded())
        {
            if (message.type() == "chat")
            {
                // Our RemoteUser instances aren't stable until the roster has loaded. To avoid our callers seeing
                // RemoteUser participants until they're stable wait until the roster is loaded.
                mRosterLoadQueuedMessages.enqueue(message);
            }
            else
            {
                ORIGIN_LOG_WARNING << "Received a message of type " << message.type() << " while loading; ignoring";
            }

            return;
        }

        // Create a new conversation if it doesn't exist
        QSharedPointer<Conversation> conversation = conversationForConversable(conversable, Conversation::IncomingMessage);
        if (message.type() == "groupchat")
        {
            emit groupChatMessageReceived(conversation, message);
        }

        // The Conversable::messageReceived() signal will be emitted next which will insert the message in to the
        // conversation
    }
        
    void ConversationManager::onMultiplayerInviteSent(const Origin::Engine::MultiplayerInvite::Invite &invite)
    {
        //make sure that jingle client exists
        Chat::Connection *connection = NULL;
        
        if (mSocialController)
            connection = mSocialController->chatConnection();

        if (!connection)
        {
            return;
        }

        if (!connection->established())
        {
            return;
        }

        if (!connection->currentUser()->roster()->hasLoaded())
        {
            // We haven't loaded our roster yet
            // This is likely due to offline delivery. Unlike messages, invites are ephemeral and are useless on the
            // scale of minutes. Instead of trying to inject them in to the conversation and interleave them properly
            // with messages just drop them on the floor.
            return;
        }

        QSharedPointer<Conversation> conversation;
        Chat::JabberID convLookupJid;

        if (invite.to().asBare() == connection->currentUser()->jabberId().asBare())
        {
            // This is to us - find the matching conversation using the from JID
            convLookupJid = invite.from().asBare();
        }
        else
        {
            // This is from us - find the matching conversation using the to JID
            convLookupJid = invite.to().asBare();
        }
            
        if (Chat::RemoteUser *user = connection->remoteUserForJabberId(convLookupJid))
        {
            conversation = conversationForConversable(user, Conversation::IncomingMessage);
            
            if (conversation)
            {
                conversation->injectMultiplayerInvite(invite);
            }
        }
    }

    void ConversationManager::onRosterLoaded()
    {
        // Replay any chat messages we queued waiting for a roster load
        while(!mRosterLoadQueuedMessages.isEmpty())
        {
            const Chat::Message message = mRosterLoadQueuedMessages.dequeue();

            Chat::Connection *connection = mSocialController->chatConnection();
            Chat::RemoteUser *user = connection->remoteUserForJabberId(message.from());

            QSharedPointer<Conversation> conversation = conversationForConversable(user, Conversation::IncomingMessage);

            if (!conversation.isNull())
            {
                conversation->injectMessage(message);
            }
        }
    }

    void ConversationManager::onJoiningMucRoom(Origin::Chat::MucRoom* room, const QString& roomId, const QString& password, const QString& roomName, const bool leaveRoomOnJoin, const bool failSilently)
    {
        QMutexLocker locker(&mConversationMutex);

        // Iterate over our conversations looking for an existing conversation related to this invite
        for(ActiveConversationHash::ConstIterator it = mActiveConversations.constBegin();
            it != mActiveConversations.constEnd();
            it++)
        {
            QSharedPointer<Conversation> conv = *it;

            // Is this a conversation we're already in?
            if (roomId == conv->roomId())
            {
                emit conversationCreated(conv);
                return;
            }
        }

        const QString originId = mSocialController->userNames()->namesForCurrentUser().originId();
        Chat::Connection *conn = mSocialController->chatConnection();
        Chat::EnterMucRoomOperation *enterOp = conn->enterMucRoom(room, roomId, originId, password, roomName, failSilently);

        // Set up a time out connection for our request in case the server never responds
        ORIGIN_VERIFY_CONNECT(enterOp, SIGNAL(enterRoomRequestTimedout(Origin::Chat::ChatroomEnteredStatus)), this, SLOT(onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus)));

        if (leaveRoomOnJoin) 
        {
            ORIGIN_VERIFY_CONNECT(
                enterOp, SIGNAL(succeeded(Origin::Chat::MucRoom *)),
                conn, SLOT(leaveMucRoom(Origin::Chat::MucRoom *)));
        }
        else
        {
            ORIGIN_VERIFY_CONNECT(
                enterOp, SIGNAL(succeeded(Origin::Chat::MucRoom *)),
                this, SLOT(onEnteredCreatedMucRoom(Origin::Chat::MucRoom *)));
        }
        
        // We need to always track these failure so we can properly debug possible issues
        ORIGIN_VERIFY_CONNECT(enterOp, SIGNAL(failed(Origin::Chat::ChatroomEnteredStatus)), this, SLOT(onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus)));

    }

    void ConversationManager::onRejoiningMucRoom(Origin::Chat::MucRoom* room, const QString& roomId)
    {
        QMutexLocker locker(&mConversationMutex);

        // Iterate over our conversations looking for an existing conversation related join
        for(ActiveConversationHash::ConstIterator it = mActiveConversations.constBegin();
            it != mActiveConversations.constEnd();
            it++)
        {
            QSharedPointer<Conversation> conv = *it;
            Chat::ChatGroup* group = conv->getGroup();

            // Is this a conversation we're already in?
            if (roomId == conv->roomId())
            {

                const QString originId = mSocialController->userNames()->namesForCurrentUser().originId();
                Chat::Connection *conn = mSocialController->chatConnection();
                Chat::EnterMucRoomOperation *enterOp = conn->enterMucRoom(room, roomId, originId);

                ORIGIN_VERIFY_CONNECT(enterOp, SIGNAL(succeeded(Origin::Chat::MucRoom *)), (QObject*)group, SIGNAL(rejoinMucRoomSuccess()));
                ORIGIN_VERIFY_CONNECT(enterOp, SIGNAL(failed(Origin::Chat::ChatroomEnteredStatus)), (QObject*)group, SLOT(onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus)));
            }
        }
    }

    void ConversationManager::onRoomDestroyedBy(Origin::Chat::MucRoom* mucRoom, const QString& owner)
    {
        Chat::Connection *connection = mSocialController->chatConnection();

        if (!connection->currentUser()->roster()->hasLoaded())
        {
            return;
        }

        // Grab the existing conversation
        QSharedPointer<Conversation> conversation = conversationForConversable(mucRoom, Conversation::InternalRequest);

        emit groupRoomDestroyed(conversation, owner);
    }

    void ConversationManager::onKickedFromRoom(Origin::Chat::MucRoom* mucRoom, const QString& owner)
    {
        Chat::Connection *connection = mSocialController->chatConnection();

        if (!connection->currentUser()->roster()->hasLoaded())
        {
            return;
        }

        // Grab the existing conversation
        QSharedPointer<Conversation> conversation = conversationForConversable(mucRoom, Conversation::InternalRequest);

        emit kickedFromRoom(conversation, owner);
    }

    void ConversationManager::onEnteredCreatedMucRoom(Origin::Chat::MucRoom *room)
    {
        Origin::Chat::ChatChannel* channel = dynamic_cast<Origin::Chat::ChatChannel*>(room);
        QString const& groupId = (channel && channel->getChatGroup())
            ? channel->getChatGroup()->getGroupGuid()
            : "";
        QString const& roomId = room->jabberId().node();
        bool isInIgo = IGOController::instance()->isActive();
        GetTelemetryInterface()->Metric_CHATROOM_ENTER_ROOM(roomId.toLatin1(), groupId.toLatin1(), isInIgo);

        // Create the matching conversation
        conversationForConversable(room, Conversation::InternalRequest);
    }

    void ConversationManager::onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus status)
    {
        Chat::EnterMucRoomOperation *enterOp = dynamic_cast<Origin::Chat::EnterMucRoomOperation*>(sender());
        if (enterOp)
        {
            // If this is not a silent fail trigger the proper dialog
            if (!enterOp->isSilentFailure())
            {
                emit enterMucRoomFailed(status);
            }
            Chat::JabberID jid = enterOp->jabberId();
            Q_UNUSED(jid);
            ORIGIN_LOG_ERROR << "Failed to enter room: "<<jid.node() << " with status: " << status << " connected to node: " << enterOp->xmppNode();
            GetTelemetryInterface()->Metric_CHATROOM_ENTER_ROOM_FAIL(enterOp->xmppNode().toLocal8Bit(), jid.node().toLocal8Bit(), status);
        }
    }

    bool ConversationManager::multiUserChatCapable() const
    {
        return mSocialController->chatConnection()->currentUser()->visibility() == Chat::ConnectedUser::Visible;
    }
        
    void ConversationManager::onVisibilityChanged(Origin::Chat::ConnectedUser::Visibility visibility)
    {
        emit multiUserChatCapabilityChanged(visibility == Chat::ConnectedUser::Visible);
    }

    void ConversationManager::onEventAdded(const ConversationEvent* evt)
    {
        if (!evt)
            return;

        Conversation* conv = dynamic_cast<Conversation*>(sender());
        if (conv)
        {
            const VoiceCallEvent* voiceEvent = dynamic_cast< const VoiceCallEvent*>(evt);
            if (voiceEvent)
            {
                if (voiceEvent->type() == Conversation::VOICECALLEVENT_USER_MUTED_BY_ADMIN)
                {
                    for(ActiveConversationHash::ConstIterator it = mActiveConversations.constBegin();
                        it != mActiveConversations.constEnd();
                        it++)
                    {
                        if ((*it).data() == conv)
                        {
                            emit mutedByAdmin((*it), voiceEvent->from());
                            return;
                        }
                    }
                    ORIGIN_ASSERT_MESSAGE(0, "We were muted in a converation we didn't find!");
                }
            }
        }
    }

    bool ConversationManager::hasMultiUserDependentConversations()
    {
        QMutexLocker locker(&mConversationMutex);
        
        for(ActiveConversationHash::ConstIterator it = mActiveConversations.constBegin();
            it != mActiveConversations.constEnd();
            it++)
        {
            if ((*it)->isMultiUserChatDependent())
            {
                return true;
            }
        }

        return false;
    }
        
    void ConversationManager::finishMultiUserDependentConversations(Conversation::FinishReason reason)
    {
        // Finishing can modify the hash as we iterate over it. Be safe and work on a copy of the hash. 
        ActiveConversationHash activeCopy;
        
        {
            QMutexLocker locker(&mConversationMutex);
            activeCopy = mActiveConversations;
        }

        for(ActiveConversationHash::ConstIterator it = activeCopy.constBegin();
            it != activeCopy.constEnd();
            it++)
        {
            QSharedPointer<Conversation> conv = *it;

            if (conv->isMultiUserChatDependent())
            {
                conv->finish(reason);
            }
        }
    }

    int ConversationManager::allocateMultiUserChatSequenceNumber()
    {
        return mMultiUserChatSequence.fetchAndAddOrdered(1);
    }

    void ConversationManager::finishAllConversations(Conversation::FinishReason reason)
    {
        ActiveConversationHash activeCopy;
        
        {
            QMutexLocker locker(&mConversationMutex);
            activeCopy = mActiveConversations;
        }

        for(ActiveConversationHash::ConstIterator it = activeCopy.constBegin();
            it != activeCopy.constEnd();
            it++)
        {
            (*it)->finish(reason);
        }
    }
}
}
}
