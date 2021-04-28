#include "engine/social/Conversation.h"

#include <chat/Conversable.h>
#include <chat/Message.h>
#include <chat/OriginConnection.h>
#include <chat/XMPPUser.h>
#include <chat/ConnectedUser.h>
#include <chat/RemoteUser.h>
#include <chat/MucRoom.h>
#include <chat/ChatChannel.h>
#include <chat/ChatGroup.h>

#include <services/debug/DebugService.h>
#include <services/log/LogService.h>
#include <services/platform/PlatformService.h>
#include <services/settings/SettingsManager.h>
#include <services/rest/OriginServiceValues.h>
#include "services/config/OriginConfigService.h"
#include "services/voice/VoiceService.h"

#include <engine/content/ContentController.h>
#include <engine/content/ContentTypes.h>
#include <engine/content/LocalContent.h>
#include "engine/igo/IGOController.h"
#include <engine/login/LoginController.h>
#include <engine/multiplayer/Invite.h>
#include <engine/social/SocialController.h>
#include <engine/social/UserNamesCache.h>
#include <engine/social/ConversationManager.h>
#include <engine/social/ConversationEvent.h>
#include <engine/social/AvatarManager.h>
#include <engine/social/MucRoomParticipantSummary.h>
#include <engine/voice/VoiceController.h>
#include <engine/voice/VoiceClient.h>
#include <engine/voice/VoiceSettings.h>

#include "TelemetryAPIDLL.h"

#include <QLocale>
#include <QApplication>
#include <QThread>
#ifdef ORIGIN_MAC
#include <QtMultimedia/QSound>
#endif
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QSoundEffect>

namespace {

    /// \brief Returns the name of the currently playing game; an empty string if there is no game currently playing.
    /// If multiple games are playing, only the name of the first game is returned.
    QString currentlyPlayingGame()
    {
        using namespace Origin::Engine::Content;

        ContentController* contentController = ContentController::currentUserContentController();
        if (!contentController)
            return QString();

        QList<EntitlementRef> playing = contentController->entitlementByState(0, LocalContent::Playing);
        if (playing.count() > 0)
            return playing[0]->contentConfiguration()->displayName();

        return QString();
    }
}

namespace Origin
{
namespace Engine
{
namespace Social
{

namespace
{
#if ENABLE_VOICE_CHAT
    const int VOICE_CALL_TIMEOUT = 60000;
    const int VOICE_OUTGOING_CALL_TIMEOUT = 65000;

#endif
}
    Conversation::Conversation(
        SocialController *socialController,
        Chat::Conversable *conversable,
        const CreationReason& creationReason) :
        ConversationRecord(QDateTime::currentDateTime()),
        mPhase(PendingMucRoomInvitePhase),
        mSocialController(socialController),
        mConversable(conversable),
        mRespondedToMucInvite(false),
        mCreationReason(creationReason),
        mFinishReason(NotFinished),
        mCreateChannelResponse(NULL),
        mAddChannelUserResponse(NULL),
        mGetChannelTokenResponse(NULL),
        mVoiceClient(NULL),
        mVoiceChannel(""),
        mVoiceCallState(Conversation::VOICECALLSTATE_DISCONNECTED)
#if ENABLE_VOICE_CHAT
        , mNotifyParticipants(NOTIFY_PARTICIPANTS)
        , mStarting1on1Call(false)
        , mReasonType("")
        , mReasonDescription("")
#endif
    {
        // Spy on our conversable to see what state we're in
        if (dynamic_cast<Chat::RemoteUser*>(mConversable.data()))
        {
            mType = OneToOneType;
        }
        else if (dynamic_cast<Chat::MucRoom*>(mConversable.data()))
        {
            mType = GroupType;
        }

        foreach(Chat::ConversableParticipant added, mConversable->participants())
        {
            participantAdded(added);
#if ENABLE_VOICE_CHAT
            bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
            if( isVoiceEnabled )
            {
                if (mType == GroupType)
                {
                    // Check to see the presence for any room users to see if they're already in voice
                    Chat::RemoteUser* user = added.remoteUser();
                    Chat::Presence presence = added.getRoomPresence();

                    if (presence.userActivity().general == Origin::Chat::DP_JOIN)
                    {
                        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), user->jabberId(), VOICECALLEVENT_USERJOINED, type()));
                        if (QString::fromUtf8(presence.userActivity().description.c_str()).compare("MUTE") == 0)
                        {
                            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), user->jabberId(), VOICECALLEVENT_USER_REMOTE_MUTED, type()));
                            setRemoteUserVoiceCallState(user->jabberId().node(), REMOTEVOICESTATE_REMOTE_MUTE);
                        }
                        else
                        {
                            setRemoteUserVoiceCallState(user->jabberId().node(), REMOTEVOICESTATE_VOICE);
                        }
                    }
                }
            }
#endif
        }

        connectConversableSignals(mConversable);

#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( isVoiceEnabled )
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(muteByAdminRequest(const Origin::Chat::JabberID &)), this, SLOT(muteByAdmin(const Origin::Chat::JabberID &)));
#endif
        ORIGIN_VERIFY_CONNECT(mSocialController, SIGNAL(updateGroupName(const QString&, const QString&)), this, SLOT(onUpdateGroupName(const QString&, const QString&)));


        mConversable->connection()->installIncomingMessageFilter(this);
    }

    Conversation::~Conversation()
    {
    }
        
    void Conversation::connectPartipantSignals(const Chat::RemoteUser *user)
    {
        if (user)
        {
            ORIGIN_VERIFY_CONNECT(
                    user, SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)),
                    this, SLOT(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)));

#if ENABLE_VOICE_CHAT
            bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
            if( isVoiceEnabled )
            {
                ORIGIN_VERIFY_CONNECT(
                    mSocialController, SIGNAL(joiningGroupVoiceCall(Origin::Chat::Conversable&, const QString&)),
                    this, SLOT(onJoiningGroupVoiceCall(Origin::Chat::Conversable&, const QString&)));

                ORIGIN_VERIFY_CONNECT(
                    mSocialController, SIGNAL(leavingGroupVoiceCall(Origin::Chat::Conversable&)),
                    this, SLOT(onLeavingGroupVoiceCall(Origin::Chat::Conversable&)));
            }
#endif
           
            ORIGIN_VERIFY_CONNECT(
                    user, SIGNAL(subscriptionStateChanged(const Origin::Chat::SubscriptionState &, const Origin::Chat::SubscriptionState &)),
                    this, SLOT(subscriptionStateChanged(const Origin::Chat::SubscriptionState &, const Origin::Chat::SubscriptionState &)));
        }
    }

    void Conversation::disconnectPartipantSignals(const Chat::RemoteUser *user)
    {
        if (user)
        {
            disconnect(user, NULL, this, NULL);
        }
    }

    void Conversation::connectConversableSignals(Chat::Conversable *conversable)
    {
        // Hook up to the conversable's signals
        ORIGIN_VERIFY_CONNECT(
                conversable, SIGNAL(messageReceived(const Origin::Chat::Message &)),
                this, SLOT(injectIncomingMessage(const Origin::Chat::Message &)));

        ORIGIN_VERIFY_CONNECT(
            conversable, SIGNAL(chatStateReceived(const Origin::Chat::Message &)),
            this, SLOT(injectIncomingChatState(const Origin::Chat::Message &)));
        
        if (mType == GroupType)
        {
            ORIGIN_VERIFY_CONNECT(
                conversable, SIGNAL(deactivated(const Origin::Chat::MucRoom::DeactivationType, const QString &)),
                this, SLOT(onDeactivated(const Origin::Chat::MucRoom::DeactivationType, const QString &)));
        }

        ORIGIN_VERIFY_CONNECT(
                conversable, SIGNAL(messageSent(const Origin::Chat::Message &)),
                this, SLOT(injectMessage(const Origin::Chat::Message &)));

        ORIGIN_VERIFY_CONNECT(
                conversable, SIGNAL(participantAdded(const Origin::Chat::ConversableParticipant &)),
                this, SLOT(participantAdded(const Origin::Chat::ConversableParticipant &)));
        
        ORIGIN_VERIFY_CONNECT(
                conversable, SIGNAL(participantRemoved(const Origin::Chat::ConversableParticipant &)),
                this, SLOT(participantRemoved(const Origin::Chat::ConversableParticipant &)));

        ORIGIN_VERIFY_CONNECT(
            conversable, SIGNAL(participantChanged(const Origin::Chat::ConversableParticipant &)),
            this, SLOT(participantChanged(const Origin::Chat::ConversableParticipant &)));
    }
    
    void Conversation::disconnectConversableSignals(Chat::Conversable *conversable)
    {
        ORIGIN_VERIFY_DISCONNECT(
                conversable, SIGNAL(messageReceived(const Origin::Chat::Message &)),
                this, SLOT(injectIncomingMessage(const Origin::Chat::Message &)));

        ORIGIN_VERIFY_DISCONNECT(
            conversable, SIGNAL(chatStateReceived(const Origin::Chat::Message &)),
            this, SLOT(injectIncomingChatState(const Origin::Chat::Message &)));

        if (mType == GroupType)
        {
            ORIGIN_VERIFY_DISCONNECT(
                conversable, SIGNAL(deactivated(const Origin::Chat::MucRoom::DeactivationType, const QString &)),
                this, SLOT(onDeactivated(const Origin::Chat::MucRoom::DeactivationType, const QString &)));
        }

        disconnect(
                conversable, SIGNAL(messageSent(const Origin::Chat::Message &)),
                this, SLOT(injectMessage(const Origin::Chat::Message &)));

        disconnect(
                conversable, SIGNAL(participantAdded(const Origin::Chat::ConversableParticipant &)),
                this, SLOT(participantAdded(const Origin::Chat::ConversableParticipant &)));
        
        disconnect(
                conversable, SIGNAL(participantRemoved(const Origin::Chat::ConversableParticipant &)),
                this, SLOT(participantRemoved(const Origin::Chat::ConversableParticipant &)));

        disconnect(
            conversable, SIGNAL(participantChanged(const Origin::Chat::ConversableParticipant &)),
            this, SLOT(participantChanged(const Origin::Chat::ConversableParticipant &)));
    }
    
    void Conversation::injectIncomingMessage(const Origin::Chat::Message &msg)
    {
        const Chat::JabberID currentUserJid = mSocialController->chatConnection()->currentUser()->jabberId();

        if ((msg.type() == "groupchat") && (msg.from().asBare() == currentUserJid.asBare()))
        {
            // This is a group chat message being reflected back to us
            return;
        }

        injectMessage(msg);
    }
        
    void Conversation::injectMessage(const Origin::Chat::Message &msg)
    {
        if ((msg.type() != "chat") && (msg.type() != "groupchat"))
        {
            // This isn't a chat-like message (eg a game invite)
            return;
        }

        if (!msg.threadId().isEmpty())
        {
            // Track our thread ID
            // This is need to convert 1:1 chats to multiuser chats
            mAllThreadIds << msg.threadId();
            mLastThreadId = msg.threadId();
        }

        handleNewEvent(new MessageEvent(QDateTime::currentDateTime(), msg));
    }
    
    void Conversation::injectMultiplayerInvite(const MultiplayerInvite::Invite &invite)
    {
        handleNewEvent(new MultiplayerInviteEvent(QDateTime::currentDateTime(), invite));
    }

    void Conversation::injectIncomingChatState(const Origin::Chat::Message &msg)
    {
        handleNewEvent(new ChatStateEvent(QDateTime::currentDateTime(), msg));
    }
    
    void Conversation::presenceChanged(const Origin::Chat::Presence &presence, const Origin::Chat::Presence &previousPresence)
    {
        // Find out who sent this
        const Chat::XMPPUser *changedUser = dynamic_cast<Chat::XMPPUser*>(sender());

        if (!changedUser)
        {
            // Didn't work
            return;
        }

        if (presence.userActivity().specific != Origin::Chat::DP_VOICECHAT)
        {
            handleNewEvent(new PresenceChangeEvent(QDateTime::currentDateTime(), changedUser->jabberId(), presence, previousPresence, type()));
        }

#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( isVoiceEnabled )
        {
            // If in a 1:1 call with someone and they bail on us, leave the call
            if (presence.availability() == Chat::Presence::Unavailable && mType == OneToOneType && 
                mVoiceCallState != VOICECALLSTATE_DISCONNECTED)
            {
                mReasonType = "User disconnection";
                mReasonDescription = "presenceChanged: " + changedUser->jabberId().node();
                leaveVoice();
            }
        }
#endif
    }

#if ENABLE_VOICE_CHAT

    void Conversation::onIncomingVoiceCall(Origin::Chat::Conversable& caller, const QString& channel, bool /*isNewConversation*/)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        // check to ensure that the call is meant for us
        if (mConversable.isNull() || (mConversable->jabberId() != caller.jabberId()))
            return;

        ORIGIN_ASSERT_MESSAGE(mVoiceChannel.isEmpty(), "Incoming VOIP call when already in existing VOIP call");
        mIncomingVoiceCallTimer.stop();

        mIncomingVoiceCallTimer.setSingleShot(true);
        mIncomingVoiceCallTimer.setProperty("caller", QVariant::fromValue(&caller));
        mIncomingVoiceCallTimer.start(VOICE_CALL_TIMEOUT);

        ORIGIN_VERIFY_CONNECT(&mIncomingVoiceCallTimer, SIGNAL(timeout()), this, SLOT(onIncomingVoiceCallTimeout()));

        mVoiceChannel = channel;
        setVoiceCallState(VOICECALLSTATE_INCOMINGCALL);
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), caller.jabberId(), VOICECALLEVENT_REMOTE_CALLING, type()));
    }

    void Conversation::onLeavingVoiceCall(Origin::Chat::Conversable& caller)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        // check to ensure that the call is meant for us
        if (mConversable.isNull() || (mConversable->jabberId() != caller.jabberId()))
            return;

        mReasonType = "User disconnection";
        mReasonDescription = "onLeavingVoiceCall: " + caller.jabberId().node();
        leaveVoice(DONT_NOTIFY_PARTICIPANTS);
    }

    void Conversation::onJoiningGroupVoiceCall(Origin::Chat::Conversable& caller, const QString& description)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        // check to ensure that the call is meant for us
        if (mConversable.isNull() || (mConversable->jabberId() != caller.jabberId()))
            return;

        if (!isVoiceCallInProgress())
        {
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), caller.jabberId(), VOICECALLEVENT_USERJOINED, type()));
        }

        if (description.compare("MUTE") == 0)
        {
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), caller.jabberId(), VOICECALLEVENT_USER_REMOTE_MUTED, type()));
            addRemoteUserVoiceCallState(caller.jabberId().node(), REMOTEVOICESTATE_REMOTE_MUTE);
        }
        else
        {
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), caller.jabberId(), VOICECALLEVENT_USER_REMOTE_UNMUTED, type()));
            clearRemoteUserVoiceCallState(caller.jabberId().node(), REMOTEVOICESTATE_VOICE);
        }
    }

    void Conversation::onLeavingGroupVoiceCall(Origin::Chat::Conversable& caller)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        // check to ensure that the call is meant for us
        if (mConversable.isNull() || (mConversable->jabberId() != caller.jabberId()))
            return;

        const Chat::XMPPUser* from = dynamic_cast<Chat::XMPPUser*>(sender());
        if (!from)
            return;

        if (!isVoiceCallInProgress())
        {
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), from->jabberId(), VOICECALLEVENT_USERLEFT, type()));
        }
    }

    void Conversation::onIncomingVoiceCallTimeout()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_VERIFY_DISCONNECT(&mIncomingVoiceCallTimer, SIGNAL(timeout()), this, SLOT(onIncomingVoiceCallTimeout()));

        Origin::Chat::Conversable* conversable = mIncomingVoiceCallTimer.property("caller").value<Origin::Chat::Conversable*>();
        if (!conversable)
            return;
        emit callTimeout();
        mReasonType = "User disconnection";
        mReasonDescription = "onIncomingVoiceCallTimeout";
        leaveVoice();
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), conversable->jabberId(), VOICECALLEVENT_MISSED_CALL, type()));
    }

    void Conversation::onOutgoingVoiceCallTimeout()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_VERIFY_DISCONNECT(&mOutgoingVoiceCallTimer, SIGNAL(timeout()), this, SLOT(onOutgoingVoiceCallTimeout()));

        Origin::Chat::Conversable* conversable = mOutgoingVoiceCallTimer.property("caller").value<Origin::Chat::Conversable*>();
        if (!conversable)
            return;
        emit callTimeout();
        mReasonType = "User disconnection";
        mReasonDescription = "onOutgoingVoiceCallTimeout";
        leaveVoice();
        // No need to handle the VOICECALLEVENT_NOANSWER here it will be handled in the onVoiceDisconnected call
    }

    void Conversation::onVoiceInactivity()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        // Leave voice and spit out an error message
        // Don't send directed presence to other voice participants!
        // They will get the inactivity event and disconnect themselves.
        mReasonType = "User disconnection";
        mReasonDescription = "onVoiceInactivity";
        leaveVoice(DONT_NOTIFY_PARTICIPANTS);
    }

#endif
    
    void Conversation::subscriptionStateChanged(const Origin::Chat::SubscriptionState &subscriptionState, const Origin::Chat::SubscriptionState &previousSubscriptionState)
    {
        // Find out who sent this
        const Chat::XMPPUser *changedUser = dynamic_cast<Chat::XMPPUser*>(sender());

        if (!changedUser)
        {
            // Didn't work
            return;
        }
        
        handleNewEvent(new SubscriptionStateChangeEvent(QDateTime::currentDateTime(), changedUser->jabberId(), subscriptionState, previousSubscriptionState));
    }
        
    void Conversation::participantAdded(const Chat::ConversableParticipant &participant)
    {
        connectPartipantSignals(participant.remoteUser());

        const QString nickname = participant.nickname();
        const Chat::JabberID jid(participant.remoteUser()->jabberId());
        const Chat::NucleusID nucleusId = mSocialController->chatConnection()->jabberIdToNucleusId(jid);

        if (nucleusId != Chat::InvalidNucleusID)
        {
            // Assume their room nickname is their Origin ID. This is safe since we must be in a room
            // This is an Origin convention
            mSocialController->userNames()->setOriginIdForNucleusId(nucleusId, nickname, false);

            mSocialController->smallAvatarManager()->subscribe(nucleusId);
        }
        else
        {
            ORIGIN_LOG_WARNING << "Unable to parse Nucleus ID for conversation participant: " << QString::fromUtf8(jid.toEncoded());
        }

        handleNewEvent(new ParticipantEnterEvent(QDateTime::currentDateTime(), jid, participant.nickname())); 
    }
    
    void Conversation::participantRemoved(const Chat::ConversableParticipant &participant)
    {
        disconnectPartipantSignals(participant.remoteUser());

        const Chat::JabberID jid(participant.remoteUser()->jabberId());
        handleNewEvent(new ParticipantExitEvent(QDateTime::currentDateTime(), jid));
    }

    void Conversation::participantChanged(const Chat::ConversableParticipant &participant)
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        Chat::RemoteUser* user = participant.remoteUser();
        Chat::Presence presence = participant.getRoomPresence();

        // We only care about these messages if we're not in the voice channel as well.
        // If we're in the voice channel we get much more finely tuned information directly
        // from sonar
        if (presence.userActivity().general == Origin::Chat::DP_JOIN)
        {
            if (!isVoiceCallInProgress())
            {
                handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), user->jabberId(), VOICECALLEVENT_USERJOINED, type()));
            }

            if (QString::fromUtf8(presence.userActivity().description.c_str()).compare("MUTE") == 0)
            {
                handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), user->jabberId(), VOICECALLEVENT_USER_REMOTE_MUTED, type()));
                addRemoteUserVoiceCallState(user->jabberId().node(), REMOTEVOICESTATE_REMOTE_MUTE);
            }
            else
            {
                handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), user->jabberId(), VOICECALLEVENT_USER_REMOTE_UNMUTED, type()));
                setRemoteUserVoiceCallState(user->jabberId().node(), REMOTEVOICESTATE_VOICE);
            }
        }
        else if (presence.userActivity().general == Origin::Chat::DP_LEAVE)
        {
            if (!isVoiceCallInProgress())
            {
                handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), user->jabberId(), VOICECALLEVENT_USERLEFT, type()));
            }
        }
#endif
    }

    void Conversation::onDeactivated(const Origin::Chat::MucRoom::DeactivationType type, const QString &reason)
    {
        if( !active() )
            return;

        qint64 nucleusId = reason.toDouble();
        QString by = Engine::LoginController::instance()->currentUser()->eaid(); // default to current user

        Chat::JabberID jid = mSocialController->chatConnection()->nucleusIdToJabberId(nucleusId);
        Chat::XMPPUser *user = mSocialController->chatConnection()->userForJabberId(jid);
 
        Chat::RemoteUser *remoteUser = dynamic_cast<Chat::RemoteUser*>(user); // Are we a remote user?
        if( remoteUser )
        {
            by = remoteUser->originId();
        }

        // Send telemetry about room being deactivated
        GetTelemetryInterface()->Metric_CHATROOM_DEACTIVATED(getGroupGuid().toLatin1(), roomId().toLatin1(), type, by.toLatin1());

        switch(type)
        {
        case Chat::MucRoom::RoomDestroyedType:
            {
                mType = DestroyedType;
                QString msg = tr("ebisu_client_this_chat_room_was_deleted_by").arg(by);
                handleNewEvent(new MucRoomDestroyedEvent(QDateTime::currentDateTime(), msg));
            }
            break;
        case Chat::MucRoom::GroupDestroyedType:
            {
                mType = DestroyedType;
                QString msg = tr("ebisu_client_this_chat_group_was_deleted_by").arg(by);
                handleNewEvent(new MucRoomDestroyedEvent(QDateTime::currentDateTime(), msg));
            }
            break;
        case Chat::MucRoom::GroupLeftType:
            {
                mType = LeftGroupType;
                QString msg = tr("ebisu_client_you_are_no_longer_a_member_of_this_chatgroup");
                handleNewEvent(new MucRoomDestroyedEvent(QDateTime::currentDateTime(), msg));
            }
            break;
        case Chat::MucRoom::RoomKickedType:
            {
                mType = KickedType;
                QString msg = tr("ebisu_client_you_were_removed_from_the_room_by").arg(by);
                handleNewEvent(new MucRoomDestroyedEvent(QDateTime::currentDateTime(), msg));
            }
            break;
        case Chat::MucRoom::GroupKickedType:
            {
                mType = KickedType;
                QString msg = tr("ebisu_client_you_were_removed_from_the_chatgroup_by").arg(by);
                handleNewEvent(new MucRoomDestroyedEvent(QDateTime::currentDateTime(), msg));
            }
            break;
        default:
            ORIGIN_ASSERT_MESSAGE(false,"Invalid DeactivationType");
        }
    }

    void Conversation::handleNewEvent(const ConversationEvent *evt)
    {
        addEvent(evt);
        emit eventAdded(evt);
    }
        
    void Conversation::setVoiceCallState(int newVoiceCallState)
    {
        mVoiceCallState = newVoiceCallState;
        emit voiceCallStateChanged(mVoiceCallState);
    }

    void Conversation::addVoiceCallState(int newVoiceCallState)
    {
        mVoiceCallState |= newVoiceCallState;
        emit voiceCallStateChanged(mVoiceCallState);
    }

    void Conversation::clearVoiceCallState(int newVoiceCallState)
    {
        mVoiceCallState = mVoiceCallState & ~newVoiceCallState;
        emit voiceCallStateChanged(mVoiceCallState);
    }

    void Conversation::setRemoteUserVoiceCallState(const QString& userId, int newVoiceCallState)
    {
        mRemoteUserVoiceCallState[userId] = newVoiceCallState;
    }

    void Conversation::addRemoteUserVoiceCallState(const QString& userId, int newVoiceCallState)
    {
        mRemoteUserVoiceCallState[userId] |= newVoiceCallState;
    }

    void Conversation::clearRemoteUserVoiceCallState(const QString& userId, int newVoiceCallState)
    {
        mRemoteUserVoiceCallState[userId] = mVoiceCallState & ~newVoiceCallState;
    }
    
    void Conversation::finish(FinishReason reason)
    {
        mFinishReason = reason;

        if (mConversable)
        {
            if(mConversable->connection())
                mConversable->connection()->removeIncomingMessageFilter(this);

            mConversable->sendGone();

            if (Chat::MucRoom* room = dynamic_cast<Chat::MucRoom*>(mConversable.data()))
            {
                // When we finish a chat group conversation, leave the room
                room->leaveRoom();
            }

#if ENABLE_VOICE_CHAT
            bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
            if( isVoiceEnabled )
            {
                if (mVoiceCallState != VOICECALLSTATE_DISCONNECTED)
                {
                    mReasonType = "User disconnection";
                    mReasonDescription = "finish";
                    leaveVoice();
                }
            }
#endif
            if(reason == UserFinished_Invisible)
            {
                handleNewEvent(new MucRoomDestroyedEvent(QDateTime::currentDateTime(), "UserFinished_Invisible"));
            }
        }

        emit conversationFinished(reason);
        mConversable = NULL;
    }

    InviteRemoteUserOperation* Conversation::inviteRemoteUser(Chat::RemoteUser *invitee)
    {
        if (mType != GroupType)
        {
            ORIGIN_ASSERT(0);
            return NULL;
        }

        // See ConversationManager::createMultiUserConversation for the rationale behind this check
        if (invitee->presence().availability() == Chat::Presence::Unavailable)
        {
            return NULL;
        }

        // Create an invite operation
        const QString reason = this->getGroupGuid();
        const Chat::OutgoingMucRoomInvite invitation(invitee->jabberId(), reason);

        InviteRemoteUserOperation *inviteOp = new InviteRemoteUserOperation(invitation);

        dispatchInviteOperation(inviteOp);
        
        return inviteOp;
    }

    void Conversation::dispatchInviteOperation(InviteRemoteUserOperation *op)
    {
        Chat::MucRoom *room = dynamic_cast<Chat::MucRoom*>(mConversable.data());

        if (!room)
        {
            ORIGIN_ASSERT(0);
            return;
        }

        room->invite(op->invite());
    }

    QString Conversation::roomId() const
    {
        if (auto room = dynamic_cast<Chat::MucRoom*>(mConversable.data()))
        {
            return room->id();
        }

        return QString();
    }

    bool Conversation::active() const
    {
        return (type() == OneToOneType) || (type() == GroupType);
    }

    QString Conversation::getChannelName() const
    {
        if (Origin::Chat::ChatChannel* channel = dynamic_cast<Origin::Chat::ChatChannel*>(mConversable.data()))
        {
            return channel->getChannelName();
        }
        return "";
    }

    QString Conversation::getChannelId() const
    {
        if (Origin::Chat::ChatChannel* channel = dynamic_cast<Origin::Chat::ChatChannel*>(mConversable.data()))
        {
            return channel->getChannelId();
        }
        return "";
    }

    Chat::ChatChannel* Conversation::getChatChannel() const
    {
        if (Origin::Chat::ChatChannel* channel = dynamic_cast<Origin::Chat::ChatChannel*>(mConversable.data()))
        {
            return channel;
        }
        return NULL;
    }

    QString Conversation::getGroupName() const
    {
        if (Origin::Chat::ChatChannel* channel = dynamic_cast<Origin::Chat::ChatChannel*>(mConversable.data()))
        {
            return channel->getChatGroup()->getName();
        }
        return "";
    }

    QString Conversation::getGroupGuid() const
    {
        if (Origin::Chat::ChatChannel* channel = dynamic_cast<Origin::Chat::ChatChannel*>(mConversable.data()))
        {
            return channel->getChatGroup()->getGroupGuid();
        }
        return "";
    }

    Chat::ChatGroup* Conversation::getGroup() const
    {
        if (Origin::Chat::ChatChannel* channel = dynamic_cast<Origin::Chat::ChatChannel*>(mConversable.data()))
        {
            return channel->getChatGroup();
        }
        return NULL;
    }

    void Conversation::onUpdateGroupName(const QString& groupGuid, const QString& groupName)
    {
        if (this->getGroupGuid() == groupGuid)
        {
            emit updateGroupName(groupGuid, groupName);
        }
    }

#if ENABLE_VOICE_CHAT

    void Conversation::joinVoice(const QString& channel)
    {
        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "joinVoice: t=" << QDateTime::currentMSecsSinceEpoch();

        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "*** joinVoice " << channel;
        ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

        Voice::VoiceController* voiceController = mSocialController->user()->voiceControllerInstance();
        ORIGIN_ASSERT(voiceController);
        if (!voiceController)
            return;

        // If we're being called from JS (via ConversationProxy), channel will be empty and we'll need to use the stashed mVoiceChannel
        QString channelToJoin = (channel.isEmpty()) ? mVoiceChannel : channel;

        // Do not cache the pointer locally yet incase this is a conflicting voice conversation
        if (voiceController->getVoiceClient()->isInVoice() && !voiceController->getVoiceClient()->isConnectedToSettings())
        {
            emit (voiceChatConflict(channelToJoin));
        }
        else
        {
            joiningVoice(channelToJoin);
        }
    }

    void Conversation::cancelingVoice()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        onVoiceDisconnected("HANGING_UP", "Rejected incoming call.");
    }

    void Conversation::joiningVoice(const QString& channel)
    {
        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "joiningVoice: t=" << QDateTime::currentMSecsSinceEpoch();

        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "*** joiningVoice " << channel;
        ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

        // stop and cleanup any existing connection
        // We need to grab the voiceController from the user since the pointer might not be valid yet
        if (!mVoiceClient)
        {
            Voice::VoiceController* voiceController = mSocialController->user()->voiceControllerInstance();
            ORIGIN_ASSERT(voiceController);
            if (!voiceController)
                return;

            mVoiceClient = voiceController->getVoiceClient();
        }

        mVoiceClient->stopVoiceConnection("User disconnection", "joiningVoice", true);
        mReasonType = "";
        mReasonDescription = "";

        // override the saved voice channel, if any, with the incoming one
        if (!channel.isEmpty())
            mVoiceChannel = channel;

        if (mType == OneToOneType)
        {
            if (mVoiceChannel.isEmpty())
            {
                // starting a new call
                mStarting1on1Call = true;
                mVoiceChannel = Chat::MucRoom::generateUniqueMucRoomId();

                // channel name used for zabbix
                QString groupName = "1on1Call";
                Origin::Engine::UserRef user = Engine::LoginController::currentUser();
                if (!user.isNull())
                {
                    groupName += "_";
                    groupName += user->eaid();
                }

                mCreateChannelResponse.reset(Services::VoiceServiceClient::createChannel(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel, groupName));
                ORIGIN_VERIFY_CONNECT(mCreateChannelResponse.data(), SIGNAL(success()), this, SLOT(onCreateOutgoingChannelSuccess()));
                ORIGIN_VERIFY_CONNECT(mCreateChannelResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onCreateOutgoingChannelFail(Origin::Services::HttpStatusCode)));

                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "JoinVoice: one-on-one voice chat initiated, id=" << mVoiceChannel;
            }
            else
            {
                // answering call
                mAddChannelUserResponse.reset(Services::VoiceServiceClient::addUserToChannel(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel));
                ORIGIN_VERIFY_CONNECT(mAddChannelUserResponse.data(), SIGNAL(success()), this, SLOT(onJoinIncomingChannelSuccess()));
                ORIGIN_VERIFY_CONNECT(mAddChannelUserResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onJoinIncomingChannelFail(Origin::Services::HttpStatusCode)));

                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "JoinVoice: one-on-one voice chat answered, id=" << mVoiceChannel;
            }
        }
        else if (mType == GroupType)
        {
            mVoiceChannel = roomId();

            ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "channelExists: t=" << QDateTime::currentMSecsSinceEpoch();

            // Check if channel exists
            mChannelExistsResponse.reset(Services::VoiceServiceClient::channelExists(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel));
            ORIGIN_VERIFY_CONNECT(mChannelExistsResponse.data(), SIGNAL(success()), this, SLOT(onCheckChannelExistsSuccess()));
            ORIGIN_VERIFY_CONNECT(mChannelExistsResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onCheckChannelExistsFail(Origin::Services::HttpStatusCode)));

            ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "JoinVoice: group voice chat joined, id=" << mVoiceChannel;

            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), mSocialController->chatConnection()->currentUser()->jabberId(), VOICECALLEVENT_GROUP_CALL_CONNECTING, type()));
        }
    }

    bool Conversation::isVoiceCallDisconnected()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return true;

        return (mVoiceCallState == VOICECALLSTATE_DISCONNECTED);
    }

    bool Conversation::isVoiceCallInProgress()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return false;

        bool isConnected = (mVoiceCallState & VOICECALLSTATE_CONNECTED);
        bool isIncomingCall = (mVoiceCallState & VOICECALLSTATE_INCOMINGCALL);
        bool isAnsweringCall = (mVoiceCallState & VOICECALLSTATE_ANSWERINGCALL);
        bool isOutgoingCall = (mVoiceCallState & VOICECALLSTATE_OUTGOINGCALL);
        return isConnected || isIncomingCall || isOutgoingCall || isAnsweringCall;
    }

    void Conversation::muteUser(const QString& userId)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        if (!isVoiceCallInProgress())
        {
            return;
        }

        if (mVoiceClient)
            mVoiceClient->muteUser(userId);
    }

    void Conversation::unmuteUser(const QString& userId)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        if (!isVoiceCallInProgress())
        {
            return;
        }

        if (mVoiceClient)
            mVoiceClient->unmuteUser(userId);
    }

    void Conversation::muteUserByAdmin(const QString& userId)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        if (!mConversable)
           return;

        const Chat::JabberID currentUserJid = mSocialController->chatConnection()->currentUser()->jabberId();

        QSet<Chat::ConversableParticipant> participants = mConversable->participants();
        Chat::OriginConnection* connection = mSocialController->chatConnection();

        // Send directed presence to the remote user to mute his/her mic.
        foreach(Chat::ConversableParticipant participant, participants)
        {            
            if ((participant.remoteUser()->nucleusId() == userId) && (participant.remoteUser()->jabberId() != currentUserJid))
            {
                handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), participant.remoteUser()->jabberId(), VOICECALLEVENT_USER_ADMIN_MUTED, type()));

                Origin::Chat::Message msg("mute", connection->currentUser()->jabberId(), participant.remoteUser()->jabberId(), Chat::ChatStateNone);
                connection->sendMessage(msg);
            }
        }
    }

    void Conversation::muteByAdmin(const Origin::Chat::JabberID &jabberId)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        if (!isVoiceCallInProgress())
        {
            return;
        }

        addVoiceCallState(VOICECALLSTATE_ADMIN_MUTED);

        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jabberId, VOICECALLEVENT_USER_MUTED_BY_ADMIN, type()));

        muteConnection();
    }

    void Conversation::muteSelf()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        if (!isVoiceCallInProgress())
        {
            return;
        }

        addVoiceCallState(VOICECALLSTATE_MUTED);

        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), mSocialController->chatConnection()->currentUser()->jabberId(), VOICECALLEVENT_USER_LOCAL_MUTED, type()));

        muteConnection();
    }

    void Conversation::unmuteSelf()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        unmuteConnection();
    }

    void Conversation::muteConnection()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        if (mVoiceClient)
            mVoiceClient->mute();

        addVoiceCallState(VOICECALLSTATE_MUTED);

        Chat::OriginConnection *connection = mSocialController->chatConnection();
        ORIGIN_ASSERT(connection);
        if (!connection)
        {
            return;
        }
    
        if (Chat::MucRoom* room = dynamic_cast<Chat::MucRoom*>(mConversable.data()))
        {
            connection->sendDirectedPresence(Origin::Chat::JINGLE_PRESENCE_VOIP_JOIN, currentlyPlayingGame(), room->jabberId().withResource(room->nickname()), "MUTE");
        }
    }

    void Conversation::unmuteConnection()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        if (!isVoiceCallInProgress())
        {
            return;
        }

        if (mVoiceClient)
            mVoiceClient->unmute();
        clearVoiceCallState(VOICECALLSTATE_MUTED | VOICECALLSTATE_ADMIN_MUTED);

        Chat::OriginConnection *connection = mSocialController->chatConnection();
        ORIGIN_ASSERT(connection);
        if (!connection)
        {
            return;
        }

        QString user = Services::Session::SessionService::nucleusUser(Services::Session::SessionService::currentSession());
        const Chat::JabberID jid(connection->nucleusIdToJabberId(user.toULongLong()));
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_USER_LOCAL_UNMUTED, type()));
    
        if (Chat::MucRoom* room = dynamic_cast<Chat::MucRoom*>(mConversable.data()))
        {
            connection->sendDirectedPresence(Origin::Chat::JINGLE_PRESENCE_VOIP_JOIN, currentlyPlayingGame(), room->jabberId().withResource(room->nickname()), "");
        }
    }


    void Conversation::leaveVoice(ParticipantNotification notifyParticipants)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "*** leaveVoice " << mVoiceChannel;

        if(mReasonType.isEmpty() && mReasonDescription.isEmpty())
        {
            // user clicked on 'red telephone' icon to end the call
            mReasonType = "User disconnection";
            mReasonDescription = "leaveVoice";
        }

        if(mConversable && mConversable->connection())
            mConversable->connection()->removeIncomingMessageFilter(this);

        if (!mVoiceClient || !mVoiceClient->isInVoice())
        {
            // EBIBUGS-29024: When user immediately closes the chat window after starting voice,
            //                we can get here in the middle of trying to connect to a voice channel,
            //                i.e., the Connection is in the 'Connecting' state. If so, do not call
            //                'onVoiceDisconnected()', which NULLs out mVoiceClient and prevents
            //                the connection from being disconnected via 'stopVoiceConnection()'.
            if(!mVoiceClient || !mVoiceClient->isConnectingToVoice() )
                onVoiceDisconnected("HANGING_UP", "Rejected incoming call.");

            if (mVoiceClient)
            {
                mVoiceClient->stopVoiceConnection(mReasonType, mReasonDescription);
                mVoiceClient = NULL;
            }
            return;
        }

        mNotifyParticipants = notifyParticipants;
        mVoiceClient->stopVoiceConnection(mReasonType, mReasonDescription);
        mVoiceClient = NULL;

        // play sound
        // TODO - move all sound effects into the front-end JS layer
        QUrl pathUrl = QUrl::fromLocalFile(Origin::Services::PlatformService::resourcesDirectory() + "/sounds/CallEnded.wav");
        if (mSound.isNull())
            mSound.reset(new QMediaPlayer());
        mSound->setMedia(pathUrl);
        mSound->setVolume(Origin::Services::PlatformService::GetCurrentApplicationVolume() * 100); //set expects an int from 0-100
        mSound->play();
        //Stop timers in case user called and hung up before answered.
        this->mIncomingVoiceCallTimer.stop();
        this->mOutgoingVoiceCallTimer.stop();
    }

    void Conversation::startVoiceAndCall()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        Services::GetChannelTokenResponse* response = dynamic_cast<Services::GetChannelTokenResponse*>(sender());
        if (!response)
            return;

        if (!mConversable)
            return;

        QSet<Chat::ConversableParticipant> participants = mConversable->participants();
        
        if (participants.count() != 1)
        {
            ORIGIN_ASSERT(0);
            return;
        }

        setVoiceCallState(VOICECALLSTATE_OUTGOINGCALL);
        Origin::Chat::JabberID jid = (mConversable) ? mConversable->jabberId() : Origin::Chat::JabberID();
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_LOCAL_CALLING, type()));
        
        if (!startVoice(response->getToken()))
        {
            setVoiceCallState(VOICECALLSTATE_DISCONNECTED);
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_ENDED, type()));
            return;
        }
    }

    void Conversation::startGroupVoice()
    {
        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "startGroupVoice: t=" << QDateTime::currentMSecsSinceEpoch();

        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        Services::GetChannelTokenResponse* response = dynamic_cast<Services::GetChannelTokenResponse*>(sender());
        if (!response)
            return;

        Chat::OriginConnection *connection = mSocialController->chatConnection();
        ORIGIN_ASSERT(connection);
        if (!connection)
        {
            return;
        }

        QString user = Services::Session::SessionService::nucleusUser(Services::Session::SessionService::currentSession());
        const Chat::JabberID jid(connection->nucleusIdToJabberId(user.toULongLong()));

        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_STARTED, type()));
        setVoiceCallState(VOICECALLSTATE_CONNECTED);
        if (!startVoice(response->getToken()))
        {
            setVoiceCallState(VOICECALLSTATE_DISCONNECTED);
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_ENDED, type()));
            return;
        }

        // Need to add the filter here so we always track adminMute
        if (mConversable && mConversable->connection())
            mConversable->connection()->installIncomingMessageFilter(this);

        if (Chat::MucRoom* room = dynamic_cast<Chat::MucRoom*>(mConversable.data()))
        {
            // Need to send presence to the room that we're in voice
            connection->sendDirectedPresence(Origin::Chat::JINGLE_PRESENCE_VOIP_JOIN, currentlyPlayingGame(), room->jabberId().withResource(room->nickname()), "");
        }
    }

    void Conversation::answerCall()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        Services::GetChannelTokenResponse* response = dynamic_cast<Services::GetChannelTokenResponse*>(sender());
        if (!response)
            return;

        QSet<Chat::ConversableParticipant> participants = mConversable->participants();

        if (participants.count() != 1)
        {
            ORIGIN_ASSERT(0);
            return;
        }

        setVoiceCallState(VOICECALLSTATE_ANSWERINGCALL);
        Origin::Chat::JabberID jid = (mConversable) ? mConversable->jabberId() : Origin::Chat::JabberID();
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_STARTED, type()));

        if (!startVoice(response->getToken()))
        {
            setVoiceCallState(VOICECALLSTATE_DISCONNECTED);
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_ENDED, type()));
            return;
        }
    }

    bool Conversation::startVoice(const QString& token)
    {
        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "startVoice: t=" << QDateTime::currentMSecsSinceEpoch();

        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return false;

        // We might not have a local pointer to the voice client yet so check and grab it here if needed.
        if (!mVoiceClient)
        {
            Voice::VoiceController* voiceController = mSocialController->user()->voiceControllerInstance();
            ORIGIN_ASSERT(voiceController);
            if (!voiceController)
                return false;

            mVoiceClient = voiceController->getVoiceClient();
        }

        if (token.isEmpty())
            return false;

        // Keep the call from timing out
        mIncomingVoiceCallTimer.stop();
        mOutgoingVoiceCallTimer.stop();
        ORIGIN_VERIFY_DISCONNECT(&mIncomingVoiceCallTimer, SIGNAL(timeout()), this, SLOT(onIncomingVoiceCallTimeout()));
        ORIGIN_VERIFY_DISCONNECT(&mOutgoingVoiceCallTimer, SIGNAL(timeout()), this, SLOT(onOutgoingVoiceCallTimeout()));

        Origin::Engine::UserRef user = Engine::LoginController::currentUser();
        if (!user.isNull() && mVoiceClient)
        {
            ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceUserMuted(const QString&)), this, SLOT(onVoiceUserMuted(const QString&)));
            ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceUserUnMuted(const QString&)), this, SLOT(onVoiceUserUnMuted(const QString&)));
            ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceUserTalking(const QString&)), this, SLOT(onVoiceUserTalking(const QString&)));
            ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceUserStoppedTalking(const QString&)), this, SLOT(onVoiceUserStoppedTalking(const QString&)));
            ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceUserJoined(const QString&)), this, SLOT(onVoiceUserJoined(const QString&)));
            ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceUserLeft(const QString&)), this, SLOT(onVoiceUserLeft(const QString&)));
            ORIGIN_VERIFY_CONNECT(mVoiceClient, &Voice::VoiceClient::voiceConnected, this, &Conversation::onVoiceConnected);
            ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceDisconnected(const QString&, const QString&)), this, SLOT(onVoiceDisconnected(const QString&, const QString&)));
            ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceLevel(float)), this, SIGNAL(voiceLevel(float)));
            ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voicePushToTalkActive(bool)), this, SLOT(onVoicePushToTalkActive(bool)));
            ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceChannelInactivity()), this, SLOT(onVoiceInactivity()));

            // Make sure we are not in a muted state when we start voice
            if (mVoiceClient->isMuted())
            {
                unmuteSelf();
            }

            bool enableRemoteEcho = Origin::Services::readSetting(Origin::Services::SETTING_SonarEnableRemoteEcho);
            if (!mVoiceClient->createVoiceConnection(token, Engine::Voice::VoiceSettings(), enableRemoteEcho))
            {
                return false;
            }

            // play sound
            // TODO - move all sound effects into the front-end JS layer
            QUrl pathUrl = QUrl::fromLocalFile(Origin::Services::PlatformService::resourcesDirectory() + "/sounds/CallEntered.wav");
            if (mSound.isNull())
                mSound.reset(new QMediaPlayer());
            mSound->setMedia(pathUrl);
            mSound->setVolume(Origin::Services::PlatformService::GetCurrentApplicationVolume() * 100); //set expects an int from 0-100
            mSound->play();
        }
    }

    void Conversation::onVoiceUserMuted(const QString& userId)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        Chat::OriginConnection *conn = mSocialController->chatConnection();
        ORIGIN_ASSERT(conn);
        if (!conn)
        {
            return;
        }

        addRemoteUserVoiceCallState(userId, REMOTEVOICESTATE_LOCAL_MUTE);

        const Chat::JabberID jid(conn->nucleusIdToJabberId(userId.toULongLong()));
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_USER_LOCAL_MUTED, type()));
    }

    void Conversation::onVoiceUserUnMuted(const QString& userId)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        Chat::OriginConnection *conn = mSocialController->chatConnection();
        ORIGIN_ASSERT(conn);
        if (!conn)
        {
            return;
        }

        clearRemoteUserVoiceCallState(userId, REMOTEVOICESTATE_LOCAL_MUTE);

        const Chat::JabberID jid(conn->nucleusIdToJabberId(userId.toULongLong()));
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_USER_LOCAL_UNMUTED, type()));
    }

    void Conversation::onVoiceUserJoined(const QString& userId)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        Chat::OriginConnection *conn = mSocialController->chatConnection();
        ORIGIN_ASSERT(conn);
        if (!conn)
        {
            return;
        }

        const Chat::JabberID jid(conn->nucleusIdToJabberId(userId.toULongLong()));

        if (mType == OneToOneType)
        {
            mStarting1on1Call = false;
            mOutgoingVoiceCallTimer.stop();
            setVoiceCallState(VOICECALLSTATE_CONNECTED);
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_STARTED, type()));
        }
        else // group chat
        {
            addRemoteUserVoiceCallState(userId, REMOTEVOICESTATE_VOICE);
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_USERJOINED, type()));
        }
    }

    void Conversation::onVoiceUserLeft(const QString& userId)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        // When someone leaves, if this is 1:1 then end the call
        if (mVoiceClient && mVoiceClient->getUsersInVoice() == 1 && mType == OneToOneType)
        {
            mReasonType = "User disconnection";
            mReasonDescription = "onVoiceUserLeft: " + userId;
            leaveVoice();
        }
        else
        {
            Chat::OriginConnection *conn = mSocialController->chatConnection();
            ORIGIN_ASSERT(conn);
            if (!conn)
            {
                return;
            }

            const Chat::JabberID jid(conn->nucleusIdToJabberId(userId.toULongLong()));
            clearRemoteUserVoiceCallState(userId, REMOTEVOICESTATE_VOICE);
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_USERLEFT, type()));
        }
    }

    void Conversation::onVoiceUserTalking(const QString& userId)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        Chat::OriginConnection *conn = mSocialController->chatConnection();
        ORIGIN_ASSERT(conn);
        if (!conn)
        {
            return;
        }

        const Chat::JabberID jid(conn->nucleusIdToJabberId(userId.toULongLong()));
        if ((jid.asBare() == mSocialController->chatConnection()->currentUser()->jabberId().asBare()) || (mType == GroupType))
        {
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_USERSTARTEDTALKING, type()));
        }
    }

    void Conversation::onVoiceUserStoppedTalking(const QString& userId)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        Chat::OriginConnection *conn = mSocialController->chatConnection();
        ORIGIN_ASSERT(conn);
        if (!conn)
        {
            return;
        }

        const Chat::JabberID jid(conn->nucleusIdToJabberId(userId.toULongLong()));
        if ((jid.asBare() == mSocialController->chatConnection()->currentUser()->jabberId().asBare()) || (mType == GroupType))
        {
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_USERSTOPPEDTALKING, type()));
        }
    }

    void Conversation::onVoicePushToTalkActive(bool active)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        if(mVoiceClient && mVoiceClient->getCaptureMode() == Origin::Engine::Voice::VoiceClient::PushToTalk )
        {

            // TODO - move all sound effects into the front-end JS layer

            static const QString PUSH_TO_TALK_ON(Origin::Services::PlatformService::resourcesDirectory() + "/sounds/PushToTalkOn.wav");
            static const QString PUSH_TO_TALK_OFF(Origin::Services::PlatformService::resourcesDirectory() + "/sounds/PushToTalkOff.wav");

            // Lazily load the sounds once
            if (mPushToTalkOn.isNull())
            {
                mPushToTalkOn.reset(new QSoundEffect());
                mPushToTalkOn->setSource(QUrl::fromLocalFile(PUSH_TO_TALK_ON));
                mPushToTalkOn->setVolume(1.0f);
            }
            if (mPushToTalkOff.isNull())
            {
                mPushToTalkOff.reset(new QSoundEffect());
                mPushToTalkOff->setSource(QUrl::fromLocalFile(PUSH_TO_TALK_OFF));
                mPushToTalkOff->setVolume(1.0f);
            }

            (active) ? mPushToTalkOn->play() : mPushToTalkOff->play();
        }
    }

    void Conversation::onVoiceConnected(int /*chid*/, const QString& /*channelId*/, const QString& /*channelDescription*/) {

        Chat::MucRoom* room = dynamic_cast<Chat::MucRoom*>(mConversable.data());

        bool isOneToOneCall = (room == NULL);
        bool isAnsweringCall = mVoiceCallState & VOICECALLSTATE_ANSWERINGCALL;
        setVoiceCallState(VOICECALLSTATE_CONNECTED);

        if (isOneToOneCall && !isAnsweringCall) {
            // Start the outgoing call timer here in case the called user has the 
            // decline incoming calls setting enabled we have a way to stop the call.
            mOutgoingVoiceCallTimer.stop();

            mOutgoingVoiceCallTimer.setSingleShot(true);
            mOutgoingVoiceCallTimer.setProperty("caller", QVariant::fromValue(mConversable.data()));
            mOutgoingVoiceCallTimer.start(VOICE_OUTGOING_CALL_TIMEOUT);

            ORIGIN_VERIFY_CONNECT(&mOutgoingVoiceCallTimer, SIGNAL(timeout()), this, SLOT(onOutgoingVoiceCallTimeout()));

            // Need to send directed presence at this person for them to take the call
            Chat::OriginConnection* connection = mSocialController->chatConnection();
            QSet<Chat::ConversableParticipant> participants = mConversable->participants();
            foreach(Chat::ConversableParticipant participant, participants)
            {
                connection->sendDirectedPresence(Origin::Chat::JINGLE_PRESENCE_VOIP_CALLING, currentlyPlayingGame(), participant.remoteUser()->jabberId(), mVoiceChannel);
            }
        }

        emit voiceChatConnected(mVoiceClient);
    }

    void Conversation::onVoiceDisconnected(const QString& reasonType, const QString& reasonDescription)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if (!isVoiceEnabled)
            return;

        // Checking this is important was causing a crash on PC when the desktop and IGO chat windows got out of sync
        if (!mConversable)
            return;
        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip))
            << "onVoiceDisconnected: voice chat terminated, id=" << mVoiceChannel << ", reason: " << reasonType << ", " << reasonDescription;

        bool isOutgoingCall = (mVoiceCallState & VOICECALLSTATE_OUTGOINGCALL);
        bool isDisconnected = (mVoiceCallState == VOICECALLSTATE_DISCONNECTED);

        // 9.5-SPECIFIC GROIP SURVEY PROMPT
        QString locale = QLocale().name();
        bool isSupportedLanguage =
            locale.startsWith("de", Qt::CaseInsensitive)
            || locale.startsWith("en", Qt::CaseInsensitive)
            || locale.startsWith("es", Qt::CaseInsensitive)
            || locale.startsWith("fr", Qt::CaseInsensitive)
            || locale.startsWith("it", Qt::CaseInsensitive)
            || locale.startsWith("ru", Qt::CaseInsensitive)
            || locale.startsWith("pl", Qt::CaseInsensitive)
            || locale.startsWith("pt", Qt::CaseInsensitive)
            || locale.startsWith("zh", Qt::CaseInsensitive);

        bool randomSelected;
        double surveyFrequency = Services::OriginConfigService::instance()->miscConfig().voipSurveyFrequency;

        // Check to see whether QA testing is forcing the survey
        if (Origin::Services::readSetting(Origin::Services::SETTING_VoipForceSurvey)) {
            surveyFrequency = 1;
        }

        if (surveyFrequency == 0)
        {
            randomSelected = false;
        }
        else if (surveyFrequency == 1)
        {
            randomSelected = true;
        }
        else
        {
            // Figure out the chance of occurrence
            randomSelected = (rand() % 100) < (surveyFrequency * 100);
        }

        if (reasonDescription == "onVoiceInactivity") {
            Chat::OriginConnection* connection = mSocialController->chatConnection();
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), connection->currentUser()->jabberId(), VOICECALLEVENT_INACTIVITY, type()));
        }

        Conversation::VoiceCallEventType eventType;
        bool isServerIncompatible;
        if (reasonType == "PROTOCOL_VERSION") {
            eventType = VOICECALLEVENT_START_FAIL_VERSION_MISMATCH;
            mNotifyParticipants = DONT_NOTIFY_PARTICIPANTS;
            isServerIncompatible = true;
        } else {
            eventType = VOICECALLEVENT_ENDED;
            isServerIncompatible = false;
        };

        // Only display survey prompt if: not disconnected, English locale, and has BOI enabled
        bool promptForSurvey = (isSupportedLanguage && !isDisconnected && isVoiceEnabled && randomSelected && !isServerIncompatible);

        if (mType == OneToOneType)
        {
            if (mNotifyParticipants == NOTIFY_PARTICIPANTS)
            {
                // Send directed presence to the participant that we're leaving the call
                QSet<Chat::ConversableParticipant> participants = mConversable->participants();
                Chat::OriginConnection* connection = mSocialController->chatConnection();
                foreach(Chat::ConversableParticipant participant, participants)
                {            
                    connection->sendDirectedPresence(Origin::Chat::JINGLE_PRESENCE_VOIP_HANGINGUP, currentlyPlayingGame(), participant.remoteUser()->jabberId(), mVoiceChannel);
                }
            }

            if (mConversable)
            {
                Origin::Chat::JabberID jid = (mConversable) ? mConversable->jabberId() : Origin::Chat::JabberID();
                handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, eventType, type()));
                if (isOutgoingCall && !isServerIncompatible)
                    handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_NOANSWER, type()));

                // 9.5-SPECIFIC GROIP SURVEY PROMPT
                if (promptForSurvey)
                {
                    // Open the dialog only if the user is not in OIG
                    if (!Engine::IGOController::instance()->isVisible())
                        emit displaySurvey();
                }
            }
        }
        else
        {            
            Chat::OriginConnection *connection = mSocialController->chatConnection();
            ORIGIN_ASSERT(connection);
            if (!connection)
            {
                return;
            }

            Services::Session::SessionRef currSession = Services::Session::SessionService::currentSession();
            if (mConversable && !currSession.isNull())
            {
                QString userID = Services::Session::SessionService::nucleusUser(currSession);
                const Chat::JabberID jid(connection->nucleusIdToJabberId(userID.toULongLong()));
                Origin::Chat::JabberID jidFrom = (mConversable) ? mConversable->jabberId() : Origin::Chat::JabberID();
                handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, eventType, type()));

                // 9.5-SPECIFIC GROIP SURVEY PROMPT
                if (promptForSurvey)
                {
                    // Open the dialog only if the user is not in OIG
                    if (!Engine::IGOController::instance()->isVisible())
                        emit displaySurvey();
                }
            }

            if (Chat::MucRoom* room = dynamic_cast<Chat::MucRoom*>(mConversable.data()))
            {
                if (!room->isDeactivated()) // do not send presence when user got kicked from the room (fixes https://developer.origin.com/support/browse/OFM-3574)
                {
                    // Need to send presence to the room that we're not in voice
                    connection->sendDirectedPresence(Origin::Chat::JINGLE_PRESENCE_VOIP_LEAVE, currentlyPlayingGame(), room->jabberId().withResource(room->nickname()), "");
                }
            }
        }

        // reset the notification flag to default setting (there should be some less stateful way of passing in this value
        // but this will have to do for the time being.
        mNotifyParticipants = NOTIFY_PARTICIPANTS;

        setVoiceCallState(VOICECALLSTATE_DISCONNECTED);

        // Notify in case of an error
        emit voiceChatDisconnected(reasonType, reasonDescription);

        mVoiceChannel.clear();

        Origin::Engine::UserRef user = Engine::LoginController::currentUser();
        if (!user.isNull() && mVoiceClient)
        {
            ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceUserMuted(const QString&)), this, SLOT(onVoiceUserMuted(const QString&)));
            ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceUserUnMuted(const QString&)), this, SLOT(onVoiceUserUnMuted(const QString&)));
            ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceUserTalking(const QString&)), this, SLOT(onVoiceUserTalking(const QString&)));
            ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceUserStoppedTalking(const QString&)), this, SLOT(onVoiceUserStoppedTalking(const QString&)));
            ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceUserJoined(const QString&)), this, SLOT(onVoiceUserJoined(const QString&)));
            ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceUserLeft(const QString&)), this, SLOT(onVoiceUserLeft(const QString&)));
            ORIGIN_VERIFY_DISCONNECT(mVoiceClient, &Voice::VoiceClient::voiceConnected, this, &Conversation::onVoiceConnected);
            ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceDisconnected(const QString&, const QString&)), this, SLOT(onVoiceDisconnected(const QString&, const QString&)));
            ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceLevel(float)), this, SIGNAL(voiceLevel(float)));
            ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voicePushToTalkActive(bool)), this, SLOT(onVoicePushToTalkActive(bool)));
            ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceChannelInactivity()), this, SLOT(onVoiceInactivity()));
        }

        mVoiceClient = NULL;
    }
    
    void Conversation::onCreateOutgoingChannelSuccess()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

        if(mCreateChannelResponse != NULL)
        {
            mGetChannelTokenResponse.reset(Services::VoiceServiceClient::getChannelToken(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel));
            ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(success()), this, SLOT(startVoiceAndCall()));
            ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onGetVoiceTokenFail(Origin::Services::HttpStatusCode)));
        }
    }

    void Conversation::onCreateOutgoingChannelFail(Origin::Services::HttpStatusCode errorCode)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

        mStarting1on1Call = false;

        Chat::OriginConnection *connection = mSocialController->chatConnection();
        ORIGIN_ASSERT(connection);
        if (!connection)
        {
            return;
        }

        QString user = Services::Session::SessionService::nucleusUser(Services::Session::SessionService::currentSession());
        const Chat::JabberID jid(connection->nucleusIdToJabberId(user.toULongLong()));
        mReasonType = "Start Fail";
        mReasonDescription = "onCreateOutgoingChannelFail: " + user;
        leaveVoice();
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_START_FAIL, type()));
    }

    void Conversation::onJoinIncomingChannelSuccess()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

        if(mAddChannelUserResponse != NULL)
        {
            mGetChannelTokenResponse.reset(Services::VoiceServiceClient::getChannelToken(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel));
            ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(success()), this, SLOT(answerCall()));
            ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onGetVoiceTokenFail(Origin::Services::HttpStatusCode)));
        }
    }

    void Conversation::onJoinIncomingChannelFail(Origin::Services::HttpStatusCode errorCode)
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

        Chat::OriginConnection *connection = mSocialController->chatConnection();
        ORIGIN_ASSERT(connection);
        if (!connection)
        {
            return;
        }

        bool underMaintenance = false;
        if( mAddChannelUserResponse )
        {
            if( mAddChannelUserResponse->error() == Services::restErrorVoiceServerUnderMaintenance )
                underMaintenance = true;
        }

        QString user = Services::Session::SessionService::nucleusUser(Services::Session::SessionService::currentSession());
        const Chat::JabberID jid(connection->nucleusIdToJabberId(user.toULongLong()));
        mReasonType = underMaintenance ? "Start Fail, Under Maintenance" : "Start Fail";
        mReasonDescription = "onJoinIncomingChannelFail: " + user;
        leaveVoice();
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, underMaintenance ? VOICECALLEVENT_START_FAIL_UNDER_MAINTENANCE : VOICECALLEVENT_START_FAIL, type()));
    }

    void Conversation::onCheckChannelExistsSuccess()
    {
        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "onCheckChannelExistsSuccess: t=" << QDateTime::currentMSecsSinceEpoch();

        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

        if(mChannelExistsResponse != NULL)
        {
            if( mChannelExistsResponse->channelExists() )
            {
                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "addUserToChannel: t=" << QDateTime::currentMSecsSinceEpoch();

                // Join the channel
                mAddChannelUserResponse.reset(Services::VoiceServiceClient::addUserToChannel(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel));
                ORIGIN_VERIFY_CONNECT(mAddChannelUserResponse.data(), SIGNAL(success()), this, SLOT(onJoinGroupChannelSuccess()));
                ORIGIN_VERIFY_CONNECT(mAddChannelUserResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onJoinGroupChannelFail(Origin::Services::HttpStatusCode)));
            }
            else
            {
                // channel name used for zabbix
                QString groupName = "GroupCall";
                Origin::Engine::UserRef user = Engine::LoginController::currentUser();
                if (!user.isNull())
                {
                    groupName += "_";
                    groupName += user->eaid();
                }

                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "createChannel: t=" << QDateTime::currentMSecsSinceEpoch();

                // Create the channel
                mCreateChannelResponse.reset(Services::VoiceServiceClient::createChannel(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel, groupName));
                ORIGIN_VERIFY_CONNECT(mCreateChannelResponse.data(), SIGNAL(success()), this, SLOT(onCreateGroupChannelSuccess()));
                ORIGIN_VERIFY_CONNECT(mCreateChannelResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onCreateGroupChannelFail(Origin::Services::HttpStatusCode)));
            }
        }
    }

    void Conversation::onCheckChannelExistsFail(Origin::Services::HttpStatusCode errorCode)
    {
        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "onCheckChannelExistsFail: t=" << QDateTime::currentMSecsSinceEpoch();

        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

        if(mChannelExistsResponse != NULL)
        {
            Chat::OriginConnection *connection = mSocialController->chatConnection();
            ORIGIN_ASSERT(connection);
            if (!connection)
            {
                return;
            }

            QString user = Services::Session::SessionService::nucleusUser(Services::Session::SessionService::currentSession());
            const Chat::JabberID jid(connection->nucleusIdToJabberId(user.toULongLong()));
            mReasonType = "Start Fail";
            mReasonDescription = "onCheckChannelExistsFail: " + user;
            leaveVoice();
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_START_FAIL, type()));
        }
    }

    void Conversation::onCreateGroupChannelSuccess()
    {
        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "onCreateGroupChannelSuccess: t=" << QDateTime::currentMSecsSinceEpoch();

        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

        if(mCreateChannelResponse != NULL)
        {
            ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "getChannelToken: t=" << QDateTime::currentMSecsSinceEpoch();

            mGetChannelTokenResponse.reset(Services::VoiceServiceClient::getChannelToken(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel));
            ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(success()), this, SLOT(startGroupVoice()));
            ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onGetVoiceTokenFail(Origin::Services::HttpStatusCode)));
        }
    }

    void Conversation::onCreateGroupChannelFail(Origin::Services::HttpStatusCode errorCode)
    {
        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "onCreateGroupChannelFail: t=" << QDateTime::currentMSecsSinceEpoch();

        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

        if(mCreateChannelResponse != NULL)
        {
            if (errorCode == Origin::Services::Http409ClientErrorConflict)
            {
                // Channel exists. Probably a race condition when 2 users try to create a channel simultaneously.
                // Channel was created by the other user, so we must add ourself to the channel.
                mAddChannelUserResponse.reset(Services::VoiceServiceClient::addUserToChannel(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel));
                ORIGIN_VERIFY_CONNECT(mAddChannelUserResponse.data(), SIGNAL(success()), this, SLOT(onJoinGroupChannelSuccess()));
                ORIGIN_VERIFY_CONNECT(mAddChannelUserResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onJoinGroupChannelFail(Origin::Services::HttpStatusCode)));
            }
            else
            {
                Chat::OriginConnection *connection = mSocialController->chatConnection();
                ORIGIN_ASSERT(connection);
                if (!connection)
                {
                    return;
                }

                QString user = Services::Session::SessionService::nucleusUser(Services::Session::SessionService::currentSession());
                const Chat::JabberID jid(connection->nucleusIdToJabberId(user.toULongLong()));
                mReasonType = "Start Fail";
                mReasonDescription = "onCreateGroupChannelFail: " + user;
                leaveVoice();
                handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_START_FAIL, type()));
            }
        }
    }

    void Conversation::onJoinGroupChannelSuccess()
    {
        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "onJoinGroupChannelSuccess: t=" << QDateTime::currentMSecsSinceEpoch();

        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

        if(mAddChannelUserResponse != NULL)
        {
            ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "getChannelToken: t=" << QDateTime::currentMSecsSinceEpoch();

            mGetChannelTokenResponse.reset(Services::VoiceServiceClient::getChannelToken(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel));
            ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(success()), this, SLOT(startGroupVoice()));
            ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onGetVoiceTokenFail(Origin::Services::HttpStatusCode)));
        }
    }

    void Conversation::onJoinGroupChannelFail(Origin::Services::HttpStatusCode errorCode)
    {
        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "onJoinGroupChannelFail: t=" << QDateTime::currentMSecsSinceEpoch();

        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

        if(mAddChannelUserResponse != NULL)
        {
            if (errorCode == Origin::Services::Http409ClientErrorConflict)
            {
                //Already in channel
                mGetChannelTokenResponse.reset(Services::VoiceServiceClient::getChannelToken(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel));
                ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(success()), this, SLOT(startGroupVoice()));
                ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onGetVoiceTokenFail(Origin::Services::HttpStatusCode)));
            }
            else // if we get any other error need to just tell the user
            {
                Chat::OriginConnection *connection = mSocialController->chatConnection();
                ORIGIN_ASSERT(connection);
                if (!connection)
                {
                    return;
                }

                bool underMaintenance = false;
                if( mAddChannelUserResponse->error() == Services::restErrorVoiceServerUnderMaintenance )
                    underMaintenance = true;

                QString user = Services::Session::SessionService::nucleusUser(Services::Session::SessionService::currentSession());
                const Chat::JabberID jid(connection->nucleusIdToJabberId(user.toULongLong()));
                mReasonType = underMaintenance ? "Start Fail, Under Maintenance" : "Start Fail";
                mReasonDescription = "onJoinGroupChannelFail: " + user;
                leaveVoice();
                handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, underMaintenance ? VOICECALLEVENT_START_FAIL_UNDER_MAINTENANCE : VOICECALLEVENT_START_FAIL, type()));
            }
        }
    }

    void Conversation::onGetVoiceTokenFail(Origin::Services::HttpStatusCode errorCode)
    {
        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "onGetVoiceTokenFail: t=" << QDateTime::currentMSecsSinceEpoch();

        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        mStarting1on1Call = false;

        Chat::OriginConnection *connection = mSocialController->chatConnection();
        ORIGIN_ASSERT(connection);
        if (!connection)
        {
            return;
        }

        QString user = Services::Session::SessionService::nucleusUser(Services::Session::SessionService::currentSession());
        const Chat::JabberID jid(connection->nucleusIdToJabberId(user.toULongLong()));
        mReasonType = "Start Fail";
        mReasonDescription = "onGetVoiceTokenFail: " + user;
        leaveVoice();
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_START_FAIL, type()));
    }

    void Conversation::debugEvents()
    {
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( !isVoiceEnabled )
            return;

        Chat::OriginConnection *connection = mSocialController->chatConnection();
        ORIGIN_ASSERT(connection);
        if (!connection)
        {
            return;
        }

        QString user = Services::Session::SessionService::nucleusUser(Services::Session::SessionService::currentSession());
        const Chat::JabberID jid(connection->nucleusIdToJabberId(user.toULongLong()));
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_START_FAIL, type()));
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), connection->currentUser()->jabberId(), VOICECALLEVENT_INACTIVITY, type()));
        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_START_FAIL_UNDER_MAINTENANCE, type()));

        if (mType != GroupType)
        {
            Origin::Chat::JabberID jidFrom = (mConversable) ? mConversable->jabberId() : Origin::Chat::JabberID();
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jidFrom, VOICECALLEVENT_NOANSWER, type()));
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jidFrom, VOICECALLEVENT_MISSED_CALL, type()));
            handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jidFrom, VOICECALLEVENT_START_FAIL_UNDER_MAINTENANCE, type()));
        }
    }

#endif //ENABLE_VOICE_CHAT

    bool Conversation::filterMessage( const Chat::Message &msg )
    {
        const Chat::JabberID currentUserJid = mSocialController->chatConnection()->currentUser()->jabberId();
        if ((msg.type() == "mute") && (msg.from().asBare() != currentUserJid.asBare()))
        {
#ifdef ENABLE_VOICE_CHAT
            bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
            if( isVoiceEnabled )
                emit muteByAdminRequest(msg.from());
#endif
            return true;
        }
        return false;
    }

    bool Conversation::isMultiUserChatDependent() const
    {
        return (type() == GroupType);
    }  

    void InviteRemoteUserOperation::triggerSuccess()
    {
        emit succeeded();
        emit finished();
        deleteLater();
    }

    void InviteRemoteUserOperation::triggerFailure()
    {
        emit failed();
        emit finished();
        deleteLater();
    }        

}
}
}
