// Copyright (C) 2013 Electronic Arts. All rights reserved.

#ifndef VOICE_CONTROLLER_H
#define VOICE_CONTROLLER_H

#include <QObject>
#include <QPointer>

#include "engine/login/User.h"
#include "services/settings/SettingsManager.h"
#include "services/rest/VoiceServiceClient.h"

class QSoundEffect;
class QMediaPlayer;

namespace Origin
{
    namespace Chat
    {
        class JabberID;
        class Message;
        class Conversable; // TODO: remove
    }

    namespace Engine
    {
        namespace Social
        {
            class SocialController;
        }

        namespace Voice
        {
            class VoiceClient;

            class VoiceController : public QObject
            {
                Q_OBJECT
#if ENABLE_VOICE_CHAT
            public:
                virtual ~VoiceController();

                /// \brief Creates a new voice controller.
                ///
                /// \param  user  The user to create the voice controller for
                static VoiceController* create(UserRef user);

                VoiceClient* getVoiceClient();

                ////////////////////////////////////////
                // Origin X - from Conversation.h
                ////////////////////////////////////////
                /// \brief State of incoming/outgoing call progression.
                enum /*VoiceCallState*/
                {
                    VOICECALLSTATE_DISCONNECTED = 0x0000,
                    VOICECALLSTATE_CONNECTED = 0x0001,
                    VOICECALLSTATE_INCOMINGCALL = 0x0002,
                    VOICECALLSTATE_OUTGOINGCALL = 0x0004,
                    VOICECALLSTATE_MUTED = 0x0008,
                    VOICECALLSTATE_ADMIN_MUTED = 0x0010,
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
                    VOICECALLEVENT_START_FAIL_UNDER_MAINTENANCE
                };

                void joinVoice(const QString& id, const QStringList& participants = QStringList());

                enum ParticipantNotification
                {
                    DONT_NOTIFY_PARTICIPANTS,
                    NOTIFY_PARTICIPANTS
                };
                void leaveVoice(const QString& id, ParticipantNotification notifyParticipants = NOTIFY_PARTICIPANTS);

                bool isVoiceCallDisconnected(const QString& id);
                bool isVoiceCallInProgress(const QString& id);
                QString outgoingVoiceCallId() const { return mOutgoingVoiceCallId; }
                void resetOutgoingVoiceCallId() { mOutgoingVoiceCallId.clear(); }

                void muteSelf();
                void unmuteSelf(bool startingVoice = false);
                void muteConnection();
                void unmuteConnection(bool startingVoice = false);

                QString const& voiceChannelId() const { return mVoiceChannel; }

                // Origin X
                void showToast(const QString& event, const QString& originId, const QString& conversationId);
                void showSurvey(const QString& voiceChannelId);

                /// \brief Returns true if the participant identified by participantNucleusId is a participant in
                /// current conversation and support voice chat.
                bool doesFriendSupportVoice(QString const& participantNucleusId) const;

            signals:
                /// \brief Emitted when we start a voice chat while in another voice chat
                /// \param  userConfirmed Returns true if the user confirms to proceed
                void voiceChatConflict(const QString& channel);

                /// \brief Emitted when the voice call state changes
                void voiceCallStateChanged(const QString& id, int newVoiceCallState);

                /// \brief Emitted when a voice chat starts
                void voiceChatConnected(Origin::Engine::Voice::VoiceClient* voiceClient);

                /// \brief Emitted when a voice chat ends
                void voiceChatDisconnected();

                /// \brief This event is fired when we receive a request to mute from the room.
                void muteByAdminRequest(const Origin::Chat::JabberID &jabberId);

                /// \brief This event is sent out if the incoming call was never answered
                void callTimeout();

                ////////////////////////////////////////
                // Origin X
                ////////////////////////////////////////
                /// \brief Emitted when a voice call event occurs
                void voiceCallEvent(QVariantMap voiceCallEventObject);

                /// \brief Emitted when a toast notification should be shown
                void showToast_IncomingVoiceCall(const QString& originId, const QString& conversationId);

                /// \brief Emitted when a voice survey should be shown
                void showSurvey_Voice(const QString& voiceChannelId);

            private slots:
                void onSettingChanged(const QString& setting, const Origin::Services::Variant& value);

                void onIncomingVoiceCall(Origin::Chat::Conversable& caller, const QString& channel, bool isNewConversation);
                void onLeavingVoiceCall(Origin::Chat::Conversable& caller);
                //void onJoiningGroupVoiceCall(Origin::Chat::Conversable& caller, const QString& description);
                //void onLeavingGroupVoiceCall(Origin::Chat::Conversable& caller);

                //void startGroupVoice();
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
                //void onCheckChannelExistsSuccess();
                //void onCheckChannelExistsFail(Origin::Services::HttpStatusCode errorCode);
                void onCreateOutgoingChannelSuccess();
                void onCreateOutgoingChannelFail(Origin::Services::HttpStatusCode errorCode);
                //void onCreateGroupChannelSuccess();
                //void onCreateGroupChannelFail(Origin::Services::HttpStatusCode errorCode);
                //void onJoinGroupChannelSuccess();
                //void onJoinGroupChannelFail(Origin::Services::HttpStatusCode errorCode);
                void onGetVoiceTokenFail(Origin::Services::HttpStatusCode errorCode);

                //void muteByAdmin(const Origin::Chat::JabberID &jabberId);

                void onIncomingVoiceCallTimeout();
                void onOutgoingVoiceCallTimeout();
                void onVoiceInactivity();
                void onVoiceConnectError(int errorCode);

            private:
                bool filterMessage(const Chat::Message &);

                void joiningVoice(const QString& channel);
                bool startVoice(const QString& token);

                void setVoiceCallState(const QString& id, int newVoiceCallState);
                int  getVoiceCallState(const QString& id);
                void addVoiceCallState(const QString& id, int newVoiceCallState);
                void clearVoiceCallState(const QString& id, int newVoiceCallState);

                void setRemoteUserVoiceCallState(const QString& userId, int newRemoteUserVoiceCallState);
                void addRemoteUserVoiceCallState(const QString& userId, int newRemoteUserVoiceCallState);
                void clearRemoteUserVoiceCallState(const QString& userId, int newRemoteUserVoiceCallState);

                QJsonObject voiceCallEventObject(const QString& event, const QString& nucleusId = QString(), const QString& errorCode = QString());

            private:
                VoiceController(UserRef user);

                UserWRef mUser;
                VoiceClient* mVoiceClient;

                /////////////////////////////////////////
                // Origin X - from Conversation.h
                /////////////////////////////////////////
                QPointer<Social::SocialController> mSocialController;

                QScopedPointer<Services::ChannelExistsResponse> mChannelExistsResponse;
                QScopedPointer<Services::CreateChannelResponse> mCreateChannelResponse;
                QScopedPointer<Services::AddChannelUserResponse> mAddChannelUserResponse;
                QScopedPointer<Services::GetChannelTokenResponse> mGetChannelTokenResponse;

                QString mVoiceChannel;
                QString mIncomingVoiceChannel;
                QMap<QString, int> mRemoteUserVoiceCallState;
                QMap<QString, int> mVoiceCallState;
                QScopedPointer<QSoundEffect> mPushToTalkOn;
                QScopedPointer<QSoundEffect> mPushToTalkOff;
                QScopedPointer<QMediaPlayer> mSound;

                ParticipantNotification mNotifyParticipants;
                QStringList mParticipants; // active
                QString mConversationId;
                QStringList mIncomingParticipants;
                QString mIncomingConversationId;
                QStringList mStagedParticipants;
                QString mStagedConversationId;
                QTimer mIncomingVoiceCallTimer;
                QTimer mOutgoingVoiceCallTimer;
                QString mOutgoingVoiceCallId;
                QString mReasonType;
                QString mReasonDescription;
#endif // ENABLE_VOICE_CHAT
            };

            // inline functions follow

#if ENABLE_VOICE_CHAT

            inline void VoiceController::onVoiceConnected(int /*chid*/, const QString& /*channelId*/, const QString& /*channelDescription*/) {
                emit voiceChatConnected(mVoiceClient);
            }

#endif // ENABLE_VOICE_CHAT
        }
    }
}

#endif //VOICE_CONTROLLER_H