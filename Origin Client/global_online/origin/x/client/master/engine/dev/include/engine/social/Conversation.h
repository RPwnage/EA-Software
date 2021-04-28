// Copyright 2012, Electronic Arts
// All rights reserved.

#ifndef _CONVERSATION_H
#define _CONVERSATION_H

#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QTcpSocket>

#include <chat/Presence.h>
#include <chat/OutgoingMucRoomInvite.h>
#include <chat/IncomingMucRoomInvite.h>
#include <chat/Conversable.h>
#include <chat/MucRoom.h>
#include <chat/SubscriptionState.h>
#include <chat/MessageFilter.h>

#include "ConversationRecord.h"
#include "services/plugin/PluginAPI.h"

#include <services/rest/VoiceServiceClient.h>

#include "Qt/originwindow.h"
#include "Qt/originwindowmanager.h"
#include "Qt/originmsgbox.h"
#include "Qt/originpushbutton.h"

class QSoundEffect;
class QMediaPlayer;

namespace Origin
{
namespace UIToolkit
{
    class OriginWindow;
}
namespace Engine
{
namespace Social
{
    class ConversationEvent;
}
}
}

namespace Origin
{
namespace Chat
{
    class ChatChannel;
    class Connection;
    class Conversable;
    class ConversableParticipant;
    class ChatGroup;
    class Message;
    class RemoteUser;
    class MucRoom;
    class EnterMucRoomOperation;
    class XMPPOperation;
    class XMPPUser;
}

namespace Engine
{
namespace MultiplayerInvite
{
    class Invite;
}

namespace Voice
{
    class VoiceCall;
    class VoiceController;
    class VoiceHangup;
    class VoiceClient;
}

namespace Social
{
    class ConversationManager;
    class SocialController;
    class Conversation;

    ///
    /// \brief Tracks the result of asynchronously inviting a remote user to a conversation 
    ///
    /// ConversationManager has ownership of InviteRemoteUserOperation and will delete them immediately after finished()
    /// is emitted
    ///
    class ORIGIN_PLUGIN_API InviteRemoteUserOperation : public QObject
    {
        friend class Conversation;

        Q_OBJECT
    public:
        /// \brief Returns the invite being sent
        Chat::OutgoingMucRoomInvite invite() const
        {
            return mInvite;
        }

    signals:
        ///
        /// \brief Emitted when a remote user has been successfully invited to the conversation
        ///
        /// The remote user still must either accept or decline the invitation
        ///
        /// \sa finished()
        ///
        void succeeded();

        ///
        /// \brief Emitted if we are unable to invite the remote user
        ///
        /// \sa finished()
        ///
        void failed();

        ///
        /// \brief Emitted after either succeeded() or failed() have been emitted
        ///
        void finished();

    private slots:
        void triggerSuccess();
        void triggerFailure();

    private:
        InviteRemoteUserOperation(const Chat::OutgoingMucRoomInvite &invite) :
            mInvite(invite)
        {
        }

        Chat::OutgoingMucRoomInvite mInvite;
    };

    ///
    /// \brief Represents a conversation with zero or more other users
    ///
    /// Conversations are ephemeral entities that only exist while being displayed to a user. Once the user explicitly
    /// closes a view of a conversation finsih() should be called. That finishes the conversation and prevents it from
    /// receiving new events. A new conversation would need to be started to continue chatting with the conversable.
    ///
    class ORIGIN_PLUGIN_API Conversation : public QObject, public ConversationRecord, public Chat::MessageFilter 
    {
        Q_OBJECT

        friend class ConversationManager;
    public:
        /// \brief Internal state information
        enum ConversationType
        {
            /// \brief In a one-to-one conversation
            OneToOneType,
            /// \brief In a group conversation
            GroupType,
            /// \brief Conversation has been destroyed
            DestroyedType,
            /// \brief We were kicked from this conversation
            KickedType,
            /// \brief We left the group
            LeftGroupType
        };

        ///
        /// \brief Indicates the originating source of the conversation:
        ///                    JoinMucRoom - We are joining a muc room
        ///                    IncomingMessage - Another user has messaged us.
        ///                    InternalRequest - User has manually started a conversation.
        ///                    Observe - Used interally by OriginChatProxy. No UI action is taken.
        ///                    SDK - The SDK has requested to create this conversation 
        ///                    StartVoice - We are joining a conversation to start voice
        ///
        enum CreationReason
        {
            CreationReasonUnset,
            JoinMucRoom,
            IncomingMessage,
            InternalRequest,
            Observe,
            SDK,
            StartVoice
        };

        /// \brief Reason for finishing a conversation
        enum FinishReason
        {
            /// \brief Conversation not finished
            NotFinished,
            /// \brief User voluntarily left the conversation
            UserFinished,
            /// \brief User voluntarily left the conversation by setting their presence to invisible
            UserFinished_Invisible,
            /// \brief Conversation was forcibly finished for a reason out of the user's control
            ForciblyFinished
        };

        /// \brief State of incoming/outgoing call progression.
        enum /*VoiceCallState*/
        {
            VOICECALLSTATE_DISCONNECTED  = 0x0000,
            VOICECALLSTATE_CONNECTED     = 0x0001,
            VOICECALLSTATE_INCOMINGCALL  = 0x0002,
            VOICECALLSTATE_OUTGOINGCALL  = 0x0004,
            VOICECALLSTATE_MUTED         = 0x0008,
            VOICECALLSTATE_ADMIN_MUTED   = 0x0010,
            VOICECALLSTATE_ANSWERINGCALL = 0x0020,
        };

        enum /*RemoveVoiceState*/
        {
            REMOTEVOICESTATE_NONE = 0x0,
            REMOTEVOICESTATE_VOICE = 0x1,
            REMOTEVOICESTATE_LOCAL_MUTE = 0x2,
            REMOTEVOICESTATE_REMOTE_MUTE = 0x4,
        };

        enum VoiceCallEventType
        {
            // General events
            VOICECALLEVENT_REMOTE_CALLING,
            VOICECALLEVENT_LOCAL_CALLING,
            VOICECALLEVENT_STARTED,
            VOICECALLEVENT_ENDED,
            VOICECALLEVENT_NOANSWER,
            VOICECALLEVENT_MISSED_CALL,
            VOICECALLEVENT_INACTIVITY,

            // User-specific events
            VOICECALLEVENT_USERJOINED,
            VOICECALLEVENT_USERLEFT,
            VOICECALLEVENT_USER_LOCAL_MUTED,
            VOICECALLEVENT_USER_LOCAL_UNMUTED,
            VOICECALLEVENT_USER_REMOTE_MUTED,
            VOICECALLEVENT_USER_REMOTE_UNMUTED,
            VOICECALLEVENT_USER_MUTED_BY_ADMIN,
            VOICECALLEVENT_USER_ADMIN_MUTED,
            VOICECALLEVENT_USERSTARTEDTALKING,
            VOICECALLEVENT_USERSTOPPEDTALKING,

            // group call
            VOICECALLEVENT_GROUP_CALL_CONNECTING,

            // Failure events
            VOICECALLEVENT_START_FAIL,
            VOICECALLEVENT_START_FAIL_UNDER_MAINTENANCE,
            VOICECALLEVENT_START_FAIL_VERSION_MISMATCH
        };

        ///
        /// \brief Returns the conversable for the conversation
        ///
        /// This may be NULL for finished conversations or conversations of type PendingMucRoomInvite
        ///
        Chat::Conversable *conversable() const
        {
            return mConversable;
        }

        /// This should replace state() I think
        ConversationType type() const
        {
            return mType;
        }        

        ///
        /// \brief Returns true if this conversation is active
        ///
        /// Only active conversations can send or receive messages or send multi-user chat room invites. This is
        /// wholly dependent on the return value of type() and may change whenever typeChanged() is emitted.
        ///
        bool active() const;

        ///
        /// \brief Invites an additional participant to this conversation
        ///
        /// \param  invitee  User to invite to the conversation. If this user is unavailable this method will return
        ///                  NULL
        ///
        InviteRemoteUserOperation* inviteRemoteUser(Chat::RemoteUser *invitee);

        /// \brief Finishes the conversation if it is not already finished
        void finish(FinishReason reason);

        ~Conversation();

        ///
        /// \brief Sets reason why conversation was created
        ///
        void setCreationReason(const CreationReason& reason)
        {
            mCreationReason = reason;
        }

        ///
        /// \brief Returns the reason the conversation was created
        ///
        CreationReason creationReason() const
        {
            return mCreationReason;
        }

        ///
        /// \brief Returns the reason the conversation was finished
        ///
        /// If the conversation wasn't finished then NotFinished will be returned
        ///
        FinishReason finishReason() const
        {
            return mFinishReason;
        }

        QString getChannelName() const;

        QString getChannelId() const;

        Chat::ChatChannel* getChatChannel() const;

        QString getGroupName() const;

        QString getGroupGuid() const;        

        Chat::ChatGroup* getGroup() const;

#if ENABLE_VOICE_CHAT

        void joinVoice(const QString& channel = QString());

        enum ParticipantNotification
        {
            DONT_NOTIFY_PARTICIPANTS,
            NOTIFY_PARTICIPANTS
        };
        void leaveVoice(ParticipantNotification notifyParticipants = NOTIFY_PARTICIPANTS);

        bool isVoiceCallDisconnected();
        bool isVoiceCallInProgress();
        bool isStarting1on1Call() const { return mStarting1on1Call; }
        void resetStarting1on1Call() { mStarting1on1Call = false; }

        void muteUser(const QString& userId);
        void unmuteUser(const QString& userId);

        void muteUserByAdmin(const QString& userId);

        void muteSelf();
        void unmuteSelf();

        void muteConnection();
        void unmuteConnection();

		QString const& voiceChannel() const { return mVoiceChannel; }

        void debugEvents();

    public slots:
        void joiningVoice(const QString& channel);
        void cancelingVoice();

#endif // ENABLE_VOICE_CHAT
    public:
        int voiceCallState() const
        {
            return mVoiceCallState;
        }

        int remoteUserVoiceCallState(const QString& userId) const
        {
            return mRemoteUserVoiceCallState[userId];
        }

    signals:
        /// \brief Emitted when a new event is added to the conversation
        void eventAdded(const Origin::Engine::Social::ConversationEvent *);
        
        ///
        /// \brief Emitted when the conversable for this conversation changes
        ///
        /// \param  newConversable       New conversable instance for the conversation or NULL if there is no conversable
        /// \param  previousConversable  Previous conversable instance for the conversation or NULL if there wasn't
        ///                              a conversable
        ///
        void conversableChanged(Origin::Chat::Conversable *newConversable, Origin::Chat::Conversable *previousConversable);

        /// \brief Emitted whenever the conversation's type changes
        void typeChanged(Origin::Engine::Social::Conversation::ConversationType type);

        /// \brief Emitted whenever the conversation's state changes to FinishedState
        void conversationFinished(Origin::Engine::Social::Conversation::FinishReason);

        /// \brief Reflected signals of voice call events
        void voiceLevel(float level);

        /// \brief Emitted when we start a voice chat while in another voice chat
        /// \param  userConfirmed Returns true if the user confirms to proceed
        void voiceChatConflict(const QString& channel);

        /// \brief Emitted when the voice call state changes
        void voiceCallStateChanged(int newVoiceCallState);

        /// \brief Emitted when a voice chat starts
        void voiceChatConnected(Origin::Engine::Voice::VoiceClient* voiceClient);

        /// \brief Emitted when a voice chat ends
        void voiceChatDisconnected(const QString reasonType, const QString reasonDescription);

        /// \brief This event is fired when we receive a request to mute from the room.
        void muteByAdminRequest(const Origin::Chat::JabberID &jabberId);

        /// \brief This event is sent out if the incoming call was never answered
        void callTimeout();        

        /// \brief This signal is fired when a chat group is renamed
        void updateGroupName(const QString&, const QString&);

        /// \brief Signal fired when voice quality survey should be display
        void displaySurvey();

    // Friend slots
    protected slots:
        void injectIncomingMessage(const Origin::Chat::Message &);
        void injectIncomingChatState(const Origin::Chat::Message &);
        void injectMessage(const Origin::Chat::Message &);

    // Friend methods
    protected:
        /// \brief Internal state information
        enum ConversationPhase
        {
            /// \brief Pending multi-user chat room invite
            PendingMucRoomInvitePhase,
            /// \brief In a one-to-one conversation with no upgrade in progress
            StableOneToOnePhase,
            /// \brief Waiting for room creation during a MUC upgrade
            WaitingRoomCreationPhase,
            /// \brief Waiting for the previous chat partner to enter during a MUC upgrade
            WaitingPartnerEnterPhase,
            /// \brief In a fully created MUC room
            StableMultiUserPhase,
            /// \brief Finished conversation
            FinishedPhase
        };

        /// \brief Returns true if this conversation is dependent on multi-user chat support
        bool isMultiUserChatDependent() const;

        /// \brief Injects a multiplayer invite in to the conversation
        /// \param voiceController  VoiceController instance to user
        /// \param conversable      Initial conversable to monitor for conversation events
        ///
        Conversation(
            SocialController *socialController,
            Chat::Conversable *conversable,
            const CreationReason& creationReason = CreationReasonUnset);

        /// \brief Injects a multiplayer invite in to the conversation
        void injectMultiplayerInvite(const MultiplayerInvite::Invite &invite);

        /// \brief Returns all thread IDs seen in this conversation
        QSet<QString> allThreadIds() const
        {
            return mAllThreadIds;
        }

        /// \brief Returns the most recent thread ID in the conversation
        QString lastThreadId() const
        {
            return mLastThreadId;
        }

        ///
        /// \brief Returns the room ID associated with this conversation or null if there is none
        ///
        /// This can return a value in WaitingPartnerEnterPhase and StableMultiUserPhase
        ///
        QString roomId() const;

        ConversationPhase phase() const
        {
            return mPhase;
        }        

    private slots:
        void presenceChanged(const Origin::Chat::Presence &presence, const Origin::Chat::Presence &previousPresence);
        void subscriptionStateChanged(const Origin::Chat::SubscriptionState &subscriptionState, const Origin::Chat::SubscriptionState &previousSubscriptionState);

        void participantAdded(const Origin::Chat::ConversableParticipant &);
        void participantRemoved(const Origin::Chat::ConversableParticipant &);
        void participantChanged(const Origin::Chat::ConversableParticipant &);

        void onDeactivated(const Origin::Chat::MucRoom::DeactivationType type, const QString &reason);
        void onUpdateGroupName(const QString&, const QString&);

#if ENABLE_VOICE_CHAT

        void onIncomingVoiceCall(Origin::Chat::Conversable& caller, const QString& channel, bool isNewConversation);
        void onLeavingVoiceCall(Origin::Chat::Conversable& caller);
        void onJoiningGroupVoiceCall(Origin::Chat::Conversable& caller, const QString& description);
        void onLeavingGroupVoiceCall(Origin::Chat::Conversable& caller);

        void startGroupVoice();
        void startVoiceAndCall();
        void answerCall();

        void onVoiceUserMuted(const QString& userId);
        void onVoiceUserUnMuted(const QString& userId);
        void onVoiceUserTalking(const QString& userId);
        void onVoiceUserStoppedTalking(const QString& userId);
        void onVoiceUserJoined(const QString& userId);
        void onVoiceUserLeft(const QString& userId);
        void onVoicePushToTalkActive(bool active);

        void onVoiceConnected(int chid, const QString& channelId, const QString& channelDescription);
        void onVoiceDisconnected(const QString& reasonType, const QString& reasonDescription);

        void onJoinIncomingChannelSuccess();
        void onJoinIncomingChannelFail(Origin::Services::HttpStatusCode errorCode);
        void onCheckChannelExistsSuccess();
        void onCheckChannelExistsFail(Origin::Services::HttpStatusCode errorCode);
        void onCreateOutgoingChannelSuccess();
        void onCreateOutgoingChannelFail(Origin::Services::HttpStatusCode errorCode);
        void onCreateGroupChannelSuccess();
        void onCreateGroupChannelFail(Origin::Services::HttpStatusCode errorCode);
        void onJoinGroupChannelSuccess();
        void onJoinGroupChannelFail(Origin::Services::HttpStatusCode errorCode);
        void onGetVoiceTokenFail(Origin::Services::HttpStatusCode errorCode);

        void muteByAdmin(const Origin::Chat::JabberID &jabberId);

        void onIncomingVoiceCallTimeout();
        void onOutgoingVoiceCallTimeout();
        void onVoiceInactivity();

#endif // ENABLE_VOICE_CHAT

    private:

        bool startVoice(const QString& token);

        void connectPartipantSignals(const Chat::RemoteUser *);
        void disconnectPartipantSignals(const Chat::RemoteUser *);

        void connectConversableSignals(Chat::Conversable *);
        void disconnectConversableSignals(Chat::Conversable *);

        void handleNewEvent(const ConversationEvent *); 

        void dispatchInviteOperation(InviteRemoteUserOperation *op);

        ConversationPhase mPhase;

        void setVoiceCallState(int newVoiceCallState);
        void addVoiceCallState(int newVoiceCallState);
        void clearVoiceCallState(int newVoiceCallState);

        void setRemoteUserVoiceCallState(const QString& userId, int newRemoteUserVoiceCallState);
        void addRemoteUserVoiceCallState(const QString& userId, int newRemoteUserVoiceCallState);
        void clearRemoteUserVoiceCallState(const QString& userId, int newRemoteUserVoiceCallState);

        bool filterMessage(const Chat::Message &);

        ConversationType mType;

        SocialController *mSocialController;

        QPointer<Chat::Conversable> mConversable;

        QString mMucRoomId;
        bool mRespondedToMucInvite;

        QSet<QString> mAllThreadIds;
        QString mLastThreadId;

        CreationReason mCreationReason;
        FinishReason mFinishReason;

        QScopedPointer<Services::ChannelExistsResponse> mChannelExistsResponse;
        QScopedPointer<Services::CreateChannelResponse> mCreateChannelResponse;
        QScopedPointer<Services::AddChannelUserResponse> mAddChannelUserResponse;
        QScopedPointer<Services::GetChannelTokenResponse> mGetChannelTokenResponse;

        Voice::VoiceClient* mVoiceClient;
        QString mVoiceChannel;
        QMap<QString, int> mRemoteUserVoiceCallState;
        int mVoiceCallState;
        QScopedPointer<QSoundEffect> mPushToTalkOn;
        QScopedPointer<QSoundEffect> mPushToTalkOff;
        QScopedPointer<QMediaPlayer> mSound;

#if ENABLE_VOICE_CHAT
        ParticipantNotification mNotifyParticipants;
        QTimer mIncomingVoiceCallTimer;
        QTimer mOutgoingVoiceCallTimer;
        bool mStarting1on1Call;
        QString mReasonType;
        QString mReasonDescription;
#endif // ENABLE_VOICE_CHAT
    };

}
}
}

#endif
