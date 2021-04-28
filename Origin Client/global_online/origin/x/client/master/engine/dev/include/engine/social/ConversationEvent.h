#ifndef _CONVERSATIONEVENT_H
#define _CONVERSATIONEVENT_H

#include <chat/Conversable.h>
#include <chat/Message.h>
#include <chat/JabberID.h>
#include <chat/Presence.h>
#include <chat/SubscriptionState.h>

#include <engine/multiplayer/Invite.h>
#include <engine/social/Conversation.h>
#include <engine/social/MucRoomParticipantSummary.h>
#include <services/plugin/PluginAPI.h>
#include <QStringList>

namespace Origin
{
namespace Engine
{
namespace Social
{
    ///
    /// \brief Represents an event in a Conversation
    ///
    /// These can either be events that just occurred or historical events
    /// 
    class ORIGIN_PLUGIN_API ConversationEvent
    {
    public:
        virtual ~ConversationEvent()
        {
        }

        ///
        /// \brief Returns the original time of the event
        ///
        /// In the case of delayed delivery (e.g. offline messages) incoming messages may have event times in the
        /// relatively distant past.
        ///
        QDateTime time() const
        {
            return mTime;
        }

    protected:
        ConversationEvent(const QDateTime &time) :
            mTime(time)
        {
        }

    private:
        QDateTime mTime;
    };

    /// \brief Represents a message either sent or received in a conversation
    class ORIGIN_PLUGIN_API MessageEvent : public ConversationEvent
    {
    public:
        /// \brief Creates a new message event
        MessageEvent(const QDateTime &time, const Chat::Message &message) :
            ConversationEvent(time),
            mMessage(message)
        {
        }

        /// \brief Returns the message that was sent or received
        Chat::Message message() const
        {
            return mMessage;
        }

    private:
        Chat::Message mMessage;
    };

    /// \brief Represents a chat state being received in a conversation
    class ORIGIN_PLUGIN_API ChatStateEvent : public ConversationEvent
    {
    public:
        /// \brief Creates a new chat state event
        ChatStateEvent(const QDateTime &time, const Chat::Message &message) :
            ConversationEvent(time),
            mMessage(message)
        {
        }

        /// \brief Returns the message that was received
        Chat::Message message() const
        {
            return mMessage;
        }

    private:
        Chat::Message mMessage;
    };

    /// \brief Represents a presence change by a conversation participant
    class ORIGIN_PLUGIN_API PresenceChangeEvent : public ConversationEvent
    {
    public:
        /// \brief Creates a new presence change event
        PresenceChangeEvent(const QDateTime &time, 
                const Chat::JabberID &jid, 
                const Chat::Presence &presence,
                const Chat::Presence &previousPresence,
                const Origin::Engine::Social::Conversation::ConversationType conversationType) :
            ConversationEvent(time),
            mJabberId(jid),
            mPresence(presence),
            mPreviousPresence(previousPresence),
            mConversationType(conversationType)
        {
        }

        /// \brief Returns the Jabber ID changing presence
        Chat::JabberID jabberId() const
        {
            return mJabberId;
        }

        /// \brief Returns the new presence for the JabberID
        Chat::Presence presence() const
        {
            return mPresence;
        };

        /// \brief Returns the previous presence for the JabberID
        Chat::Presence previousPresence() const
        {
            return mPreviousPresence;
        };

        Origin::Engine::Social::Conversation::ConversationType conversationType() const
        {
            return mConversationType;
        }

    private:

        Chat::JabberID mJabberId;
        Chat::Presence mPresence;
        Chat::Presence mPreviousPresence;
        Origin::Engine::Social::Conversation::ConversationType mConversationType;
    };
    
    /// \brief Represents a subscription state change by a conversation participant
    class ORIGIN_PLUGIN_API SubscriptionStateChangeEvent : public ConversationEvent
    {
    public:
        /// \brief Creates a new presence change event
        SubscriptionStateChangeEvent(const QDateTime &time, 
                const Chat::JabberID &jid, 
                const Chat::SubscriptionState &subscriptionState,
                const Chat::SubscriptionState &previousSubscriptionState) :
            ConversationEvent(time),
            mJabberId(jid),
            mSubscriptionState(subscriptionState),
            mPreviousSubscriptionState(previousSubscriptionState)
        {
        }

        /// \brief Returns the Jabber ID changing subscription state
        Chat::JabberID jabberId() const
        {
            return mJabberId;
        }

        /// \brief Returns the new subscription state for the JabberID
        Chat::SubscriptionState subscriptionState() const
        {
            return mSubscriptionState;
        };

        /// \brief Returns the previous subscription state for the JabberID
        Chat::SubscriptionState previousSubscriptionState() const
        {
            return mPreviousSubscriptionState;
        };

    private:
        Chat::JabberID mJabberId;
        Chat::SubscriptionState mSubscriptionState;
        Chat::SubscriptionState mPreviousSubscriptionState;
    };

    /// \brief Represents a multiplayer invite received in a conversation
    class ORIGIN_PLUGIN_API MultiplayerInviteEvent : public ConversationEvent
    {
    public:
        /// \brief Creates a new multiplayer invite event
        MultiplayerInviteEvent(const QDateTime &time, const MultiplayerInvite::Invite &invite) :
            ConversationEvent(time),
            mInvite(invite)
        {
        }

        /// \brief Returns the invite sent or received
        MultiplayerInvite::Invite invite() const
        {
            return mInvite;
        }

    private:
        MultiplayerInvite::Invite mInvite;
    };

    /// \brief Represents a participant entering a conversation
    class ORIGIN_PLUGIN_API ParticipantEnterEvent : public ConversationEvent
    {
    public:
        ParticipantEnterEvent(const QDateTime &time, const Chat::JabberID &jid, const QString &nickname) :
            ConversationEvent(time),
            mJabberId(jid),
            mNickname(nickname)
        {
        }

        /// \brief Returns the Jabber ID of the participant entering the conversation
        Chat::JabberID jabberId() const
        {
            return mJabberId;
        }

        /// \brief Returns the nickname of the participant entering the conversation
        QString nickname() const
        {
            return mNickname;
        }

    private:
        Chat::JabberID mJabberId;
        QString mNickname;
    };
    
    /// \brief Represents a participant exiting a conversation
    class ORIGIN_PLUGIN_API ParticipantExitEvent : public ConversationEvent
    {
    public:
        ParticipantExitEvent(const QDateTime &time, const Chat::JabberID &jid) :
            ConversationEvent(time),
            mJabberId(jid)
        {
        }

        /// \brief Returns the Jabber ID of the participant exiting the conversation
        Chat::JabberID jabberId() const
        {
            return mJabberId;
        }

    private:
        Chat::JabberID mJabberId;
    };

    /// \brief Represents an incoming multi-user chat room invite
    class ORIGIN_PLUGIN_API MucRoomInviteEvent : public ConversationEvent
    {
    public:
        /// \brief Creates a new multi-user chat room invite event
        MucRoomInviteEvent(const QDateTime &time, const Chat::JabberID &inviter, const Chat::JabberID &invitee, const MucRoomParticipantSummary &participantSummary) :
            ConversationEvent(time),
            mInviter(inviter),
            mInvitee(invitee),
            mParticipantSummary(participantSummary)
        {
        }

        /// \brief Returns the invitee user
        Chat::JabberID invitee() const
        {
            return mInvitee;
        }
        
        /// \brief Returns the inviter user
        Chat::JabberID inviter() const
        {
            return mInviter;
        }

        /// \brief Returns a summary of the participants in the room
        MucRoomParticipantSummary participantSummary() const
        {
            return mParticipantSummary;
        }

    private:
        Chat::JabberID mInviter;
        Chat::JabberID mInvitee;
        MucRoomParticipantSummary mParticipantSummary;
    };

    /// \brief Represents a failure to enter a multi-user chat room by the current user
    class ORIGIN_PLUGIN_API EnterMucRoomFailureEvent : public ConversationEvent
    {
    public:
        EnterMucRoomFailureEvent(const QDateTime &time) : ConversationEvent(time)
        {
        }
    };
    
    /// \brief Represents a decline of an incoming or outgoing multi-user chat room invite
    class ORIGIN_PLUGIN_API MucRoomInviteDeclineEvent : public ConversationEvent
    {
    public:
        MucRoomInviteDeclineEvent(const QDateTime &time, const Chat::JabberID &invitee, const QString &reason) :
            ConversationEvent(time),
            mInvitee(invitee),
            mReason(reason)
        {
        }

        /// \brief Returns the user declining the invite
        Chat::JabberID invitee() const
        {
            return mInvitee;
        }

        /// \brief Returns the optional human-readable string describing the reason for declining
        QString reason() const
        {
            return mReason;
        }

    private:
        Chat::JabberID mInvitee;
        QString mReason;
    };

    /// \brief Represents the destruction of a multi-user chat room
    class MucRoomDestroyedEvent : public ConversationEvent
    {
    public:
        MucRoomDestroyedEvent(const QDateTime &time, const QString &reason) :
            ConversationEvent(time),
            mReason(reason)
        {
        }

        /// \brief Returns human-readable string describing the reason for destruction
        QString reason() const
        {
            return mReason;
        }

    private:
        QString mReason;
    };

    /// \brief
    class VoiceCallEvent : public ConversationEvent
    {
    public:

        VoiceCallEvent(
            const QDateTime& time,
            const Chat::JabberID& from,
            const Conversation::VoiceCallEventType eventType,
            const Conversation::ConversationType conversationType)
        : ConversationEvent(time)
        , mFrom(from)
        , mEventType(eventType)
        , mConversationType(conversationType)
        {
        }

        Conversation::VoiceCallEventType type() const
        {
            return mEventType;
        }

        const Origin::Chat::JabberID& from() const
        {
            return mFrom;
        }

        Conversation::ConversationType ConversationType()
        {
            return mConversationType;
        }

    private:

        const Origin::Chat::JabberID mFrom;
        const Conversation::VoiceCallEventType mEventType;
        const Conversation::ConversationType mConversationType;
    };
}
}
}

#endif

