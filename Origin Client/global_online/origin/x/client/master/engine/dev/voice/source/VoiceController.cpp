// Copyright (C) 2013 Electronic Arts. All rights reserved.

#include "engine/voice/VoiceController.h"
#include "engine/voice/VoiceClient.h"
#include "engine/content/ContentController.h"
#include "engine/login/User.h"
#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "engine/social/Conversation.h"

#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"
#include <chat/MucRoom.h>
#include <chat/Roster.h>

#include "services/debug/DebugService.h"
#include "services/settings/InGameHotKey.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"
#include "services/voice/VoiceService.h"

#include <QApplication>
#include <QThread>
#ifdef ORIGIN_MAC
#include <QtMultimedia/QSound>
#endif
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QSoundEffect>
#include <QJsonObject>

#if ENABLE_VOICE_CHAT

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
        namespace Voice
        {
            namespace
            {
                const int VOICE_CALL_TIMEOUT = 30000;
                const int VOICE_OUTGOING_CALL_TIMEOUT = 35000;
                // TODO: fix this to be based on environment
                // Use Connection::host() ?
                const QString jabberDomainForChat = "integration.chat.dm.origin.com";
            }

            VoiceController::VoiceController(UserRef user)
                : mUser(user)
                , mVoiceClient(NULL)
                , mVoiceChannel("")
                , mVoiceCallState()
                , mNotifyParticipants(NOTIFY_PARTICIPANTS)
                , mOutgoingVoiceCallId("")
                , mReasonType("")
                , mReasonDescription("")
            {
                mVoiceClient = new VoiceClient(user);
                ORIGIN_ASSERT(mVoiceClient);

                mSocialController = user->socialControllerInstance();
                ORIGIN_ASSERT(mSocialController);

                ORIGIN_VERIFY_CONNECT(
                    mSocialController, SIGNAL(incomingVoiceCall(Origin::Chat::Conversable&, const QString&, bool)),
                    this, SLOT(onIncomingVoiceCall(Origin::Chat::Conversable&, const QString&, bool)));

                ORIGIN_VERIFY_CONNECT(
                    mSocialController, SIGNAL(leavingVoiceCall(Origin::Chat::Conversable&)),
                    this, SLOT(onLeavingVoiceCall(Origin::Chat::Conversable&)));

                ORIGIN_VERIFY_CONNECT(Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)),
                    this, SLOT(onSettingChanged(const QString&, const Origin::Services::Variant&)));
            }

            VoiceController::~VoiceController()
            {
                if (mVoiceClient)
                {
                    delete mVoiceClient;
                    mVoiceClient = NULL;
                }
            }

            VoiceController* VoiceController::create(UserRef user)
            {
                return new VoiceController(user);
            }

            VoiceClient* VoiceController::getVoiceClient()
            {
                return mVoiceClient;
            }

            void VoiceController::onSettingChanged(const QString& setting, const Origin::Services::Variant& value)
            {
                if (setting == Origin::Services::SETTING_EnablePushToTalk.name())
                {
                    bool pushToTalk = value;
                    bool enableAutoGain = Services::readSetting(Services::SETTING_AutoGain);
                    int activationThreshold = Services::readSetting(Services::SETTING_VoiceActivationThreshold);
                    mVoiceClient->setCaptureMode(pushToTalk, enableAutoGain, activationThreshold);
                }
                else if (setting == Origin::Services::SETTING_AutoGain.name())
                {
                    bool autoGain = value;
                    mVoiceClient->setAutoGain(autoGain);
                }
                else if (setting == Origin::Services::SETTING_PushToTalkKeyString.name())
                {
                    qulonglong pushToTalkKeySetting = Services::readSetting(Services::SETTING_PushToTalkKey);
                    if (pushToTalkKeySetting)
                    {
                        const int kVK_KEY_STROKE_ARRAY_SIZE = 8;
                        int vkkey[kVK_KEY_STROKE_ARRAY_SIZE];
                        int size = kVK_KEY_STROKE_ARRAY_SIZE;
                        Origin::Services::InGameHotKey settingHotKeys = Origin::Services::InGameHotKey::fromULongLong(pushToTalkKeySetting);
                        settingHotKeys.getVirtualKeyArray(vkkey, size);
                        if (size == 0)
                        {
                            ORIGIN_LOG_ERROR << "Push to Talk setting for virtual key array is zero-length";
                        }
                        else
                            mVoiceClient->setPushToTalkKey(vkkey, size);
                    }
                    else
                    {   // if zero then it is a mouse press
                        QString pushToTalkMouseString = Services::readSetting(Services::SETTING_PushToTalkMouse);
                        mVoiceClient->setPushToTalkKey(pushToTalkMouseString);
                    }
                }
                else if (setting == Origin::Services::SETTING_SpeakerGain.name())
                {
                    int gain = value;
                    mVoiceClient->setSpeakerGain(gain);
                }
                else if (setting == Origin::Services::SETTING_MicrophoneGain.name())
                {
                    int gain = value;
                    mVoiceClient->setMicrophoneGain(gain);
                }
                else if (setting == Origin::Services::SETTING_VoiceActivationThreshold.name())
                {
                    int threshold = value;
                    mVoiceClient->setVoiceActivationThreshold(threshold);
                }
                else if (setting == Origin::Services::SETTING_EchoQueuedDelay.name())
                {
                    int echo = value;
                    mVoiceClient->onEchoQueuedDelayChanged(echo);
                }
                else if (setting == Origin::Services::SETTING_EchoTailMultiplier.name())
                {
                    int echo = value;
                    mVoiceClient->onEchoTailMultiplierChanged(echo);
                }
                else if (setting == Origin::Services::SETTING_EchoCancellation.name())
                {
                    bool cancel = value;
                    mVoiceClient->onEnableEchoCancellation(cancel);
                }
            }

            /////////////////////////////////////////
            // Origin X - from Conversation.cpp
            /////////////////////////////////////////
            bool VoiceController::filterMessage(const Chat::Message &msg)
            {
                const Chat::JabberID currentUserJid = mSocialController->chatConnection()->currentUser()->jabberId();
                if ((msg.type() == "mute") && (msg.from().asBare() != currentUserJid.asBare()))
                {
                    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                    if (isVoiceEnabled)
                        emit muteByAdminRequest(msg.from());

                    return true;
                }
                return false;
            }

            void VoiceController::joinVoice(const QString& id, const QStringList& participants)
            {
                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "joinVoice: t=" << QDateTime::currentMSecsSinceEpoch();

                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

                // If we're being called from JS (via ConversationProxy), channel will be empty and we'll need to use the stashed mVoiceChannel
                bool isIncomingCall = (getVoiceCallState(id) & VOICECALLSTATE_INCOMINGCALL);
                QString channelToJoin = isIncomingCall ? mIncomingVoiceChannel : "";

                bool noWarnConflict = true; //Origin::Services::readSetting(Origin::Services::SETTING_NoWarnAboutVoiceChatConflict);
                if (!noWarnConflict && mVoiceClient->isInVoice() && !mVoiceClient->isConnectedToSettings())
                {
                    emit(voiceChatConflict(channelToJoin));
                }
                else
                {
                    if (isIncomingCall)
                    {
                        mStagedParticipants = mIncomingParticipants;
                        mStagedConversationId = mIncomingConversationId;

                        // clean up
                        mIncomingConversationId.clear();
                        mIncomingParticipants.clear();
                        mIncomingVoiceChannel.clear();
                    }
                    else
                    {
                        mStagedParticipants = participants;
                        mStagedConversationId = id;
                    }

                    joiningVoice(channelToJoin);
                }
            }

            void VoiceController::joiningVoice(const QString& channel)
            {
                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "joiningVoice: t=" << QDateTime::currentMSecsSinceEpoch();

                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "*** joiningVoice " << channel;
                ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

                // stop and cleanup any existing connection
                mVoiceClient->stopVoiceConnection("User disconnection", "joiningVoice", true);
                mReasonType = "";
                mReasonDescription = "";

                mParticipants = mStagedParticipants;
                mConversationId = mStagedConversationId;

                // override the saved voice channel, if any, with the incoming one
                if (!channel.isEmpty())
                    mVoiceChannel = channel;

                // TODO: fix for Group Voice Chat
                //if (mType == OneToOneType)
                {
                    if (mVoiceChannel.isEmpty())
                    {
                        // starting a new call
                        mOutgoingVoiceCallId = mConversationId;
                        mVoiceChannel = Chat::MucRoom::generateUniqueMucRoomId();

                        // channel name used for zabbix
                        QString groupName = "1on1Call";
                        UserRef user = Engine::LoginController::currentUser();
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
#if 0 // TODO: Group Voice Chat
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
#endif
            }

            bool VoiceController::isVoiceCallDisconnected(const QString& id)
            {
                bool isVoiceEnabled = Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled )
                    return true;

                if (mVoiceCallState.contains(id))
                {
                    return (mVoiceCallState[id] == VOICECALLSTATE_DISCONNECTED);
                }

                return true;
            }

            bool VoiceController::isVoiceCallInProgress(const QString& id)
            {
                bool isVoiceEnabled = Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled )
                    return false;

                if (mVoiceCallState.contains(id))
                {
                    bool isConnected = (mVoiceCallState[id] & VOICECALLSTATE_CONNECTED);
                    bool isIncomingCall = (mVoiceCallState[id] & VOICECALLSTATE_INCOMINGCALL);
                    bool isOutgoingCall = (mVoiceCallState[id] & VOICECALLSTATE_OUTGOINGCALL);
                    return isConnected || isIncomingCall || isOutgoingCall;
                }

                return false;
            }

            void VoiceController::leaveVoice(const QString& id, ParticipantNotification notifyParticipants)
            {
                bool isVoiceEnabled = Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                if (!isVoiceCallInProgress(id))
                    return;

                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "*** leaveVoice " << mVoiceChannel;

                if (mReasonType.isEmpty() && mReasonDescription.isEmpty())
                {
                    // user clicked on 'red telephone' icon to end the call
                    mReasonType = "User disconnection";
                    mReasonDescription = "leaveVoice";
                }

#if 0 // TODO: fix
                if (mConversable && mConversable->connection())
                    mConversable->connection()->removeIncomingMessageFilter(this);
#endif

                // TODO: Fix this 'if' section. 'mVoiceClient' is not NULL'd out anymore
                //       Try to remove 'mVoiceClient' from this section.
                if (!mVoiceClient || !mVoiceClient->isInVoice())
                {
                    // EBIBUGS-29024: When user immediately closes the chat window after starting voice,
                    //                we can get here in the middle of trying to connect to a voice channel,
                    //                i.e., the Connection is in the 'Connecting' state. If so, do not call
                    //                'onVoiceDisconnected()', which NULLs out mVoiceClient and prevents
                    //                the connection from being disconnected via 'stopVoiceConnection()'.
                    if (!mVoiceClient || !mVoiceClient->isConnectingToVoice())
                        onVoiceDisconnected("HANGING_UP", "Rejected incoming call.");

                    if (!isVoiceCallDisconnected(id))
                    {
                        mVoiceClient->stopVoiceConnection(mReasonType, mReasonDescription);
                        //mVoiceClient = NULL;
                    }
                    return;
                }

                mNotifyParticipants = notifyParticipants;

                if (getVoiceCallState(id) & VOICECALLSTATE_INCOMINGCALL)
                {
                    if (mNotifyParticipants == NOTIFY_PARTICIPANTS)
                    {
                        // Send directed presence to the participant that we're leaving the call
                        Chat::OriginConnection* connection = mSocialController->chatConnection();
                        foreach(QString participant, mIncomingParticipants)
                        {
                            QStringList jid = participant.split("@");
                            Chat::JabberID jabberId(jid[0], jabberDomainForChat);
                            connection->sendDirectedPresence(Origin::Chat::JINGLE_PRESENCE_VOIP_HANGINGUP, currentlyPlayingGame(), jabberId, mIncomingVoiceChannel);
                        }
                    }
                    QString nucleusId = mIncomingParticipants.at(0);
                    emit voiceCallEvent(voiceCallEventObject("ENDED_INCOMING", nucleusId).toVariantMap());

                    // clean up
                    mIncomingConversationId.clear();
                    mIncomingParticipants.clear();
                    mIncomingVoiceChannel.clear();
                }
                else
                {
                    mVoiceClient->stopVoiceConnection(mReasonType, mReasonDescription);
                }
                //mVoiceClient = NULL;

                // play sound
                // TODO - move all sound effects into the front-end JS layer
                QUrl pathUrl = QUrl::fromLocalFile(Origin::Services::PlatformService::resourcesDirectory() + "/sounds/CallEnded.wav");
                if (mSound.isNull())
                    mSound.reset(new QMediaPlayer());
                mSound->setMedia(pathUrl);
                mSound->setVolume(Origin::Services::PlatformService::GetCurrentApplicationVolume() * 100); //set expects an int from 0-100
                mSound->play();
            }

            void VoiceController::onJoinIncomingChannelSuccess()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

                if (mAddChannelUserResponse != NULL)
                {
                    mGetChannelTokenResponse.reset(Services::VoiceServiceClient::getChannelToken(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel));
                    ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(success()), this, SLOT(answerCall()));
                    ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onGetVoiceTokenFail(Origin::Services::HttpStatusCode)));
                }
            }

            void VoiceController::onJoinIncomingChannelFail(Origin::Services::HttpStatusCode errorCode)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

                Chat::OriginConnection *connection = mSocialController->chatConnection();
                ORIGIN_ASSERT(connection);
                if (!connection)
                {
                    return;
                }

                bool underMaintenance = false;
                if (mAddChannelUserResponse)
                {
                    if (mAddChannelUserResponse->error() == Services::restErrorVoiceServerUnderMaintenance)
                        underMaintenance = true;
                }

                QString user = Services::Session::SessionService::nucleusUser(Services::Session::SessionService::currentSession());
                const Chat::JabberID jid(connection->nucleusIdToJabberId(user.toULongLong()));
                mReasonType = underMaintenance ? "Start Fail, Under Maintenance" : "Start Fail";
                mReasonDescription = "onJoinIncomingChannelFail: " + user;
                leaveVoice(mConversationId);
                //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, underMaintenance ? VOICECALLEVENT_START_FAIL_UNDER_MAINTENANCE : VOICECALLEVENT_START_FAIL, type()));
                // TODO: emit voiceCallEvent(voiceCallEventObject("UNDER_MAINTENANCE").toVariantMap());
            }

            void VoiceController::onCreateOutgoingChannelSuccess()
            {
                bool isVoiceEnabled = Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

                if (mCreateChannelResponse != NULL)
                {
                    mGetChannelTokenResponse.reset(Services::VoiceServiceClient::getChannelToken(Services::Session::SessionService::currentSession(), "Origin", mVoiceChannel));
                    ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(success()), this, SLOT(startVoiceAndCall()));
                    ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onGetVoiceTokenFail(Origin::Services::HttpStatusCode)));
                }
            }

            void VoiceController::onCreateOutgoingChannelFail(Services::HttpStatusCode errorCode)
            {
                bool isVoiceEnabled = Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                ORIGIN_ASSERT(QThread::currentThread() == QApplication::instance()->thread());

                mOutgoingVoiceCallId.clear();

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
                leaveVoice(mConversationId);
                //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_START_FAIL, type()));
                // TODO: emit voiceCallEvent(voiceCallEventObject("START_FAIL").toVariantMap());
            }

            void VoiceController::startVoiceAndCall()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                Services::GetChannelTokenResponse* response = dynamic_cast<Services::GetChannelTokenResponse*>(sender());
                if (!response)
                    return;

#if 0 // TODO: remove
                if (!mConversable)
                    return;

                QSet<Chat::ConversableParticipant> participants = mConversable->participants();

                if (participants.count() != 1)
                {
                    ORIGIN_ASSERT(0);
                    return;
                }
#endif

                setVoiceCallState(mConversationId, VOICECALLSTATE_OUTGOINGCALL);
                //Origin::Chat::JabberID jid = (mConversable) ? mConversable->jabberId() : Origin::Chat::JabberID();
                //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_LOCAL_CALLING, type()));
                QString nucleusId = mParticipants.at(0);
                emit voiceCallEvent(voiceCallEventObject("OUTGOING", nucleusId).toVariantMap());

                if (!startVoice(response->getToken()))
                {
                    setVoiceCallState(mConversationId, VOICECALLSTATE_DISCONNECTED);
                    //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_ENDED, type()));
                    emit voiceCallEvent(voiceCallEventObject("ENDED", nucleusId).toVariantMap());
                    return;
                }

                Chat::OriginConnection* connection = mSocialController->chatConnection();

                // Start the outgoing call timer here in case the called user has the 
                // decline incoming calls setting enabled we have a way to stop the call.
                mOutgoingVoiceCallTimer.stop();

                mOutgoingVoiceCallTimer.setSingleShot(true);
                //mOutgoingVoiceCallTimer.setProperty("caller", QVariant::fromValue(mConversable.data()));
                mOutgoingVoiceCallTimer.start(VOICE_OUTGOING_CALL_TIMEOUT);

                ORIGIN_VERIFY_CONNECT(&mOutgoingVoiceCallTimer, SIGNAL(timeout()), this, SLOT(onOutgoingVoiceCallTimeout()));

#if 1 // TODO: fix
                // Need to send directed presence at this person for them to take the call
                foreach(QString participant, mParticipants)
                {
                    QStringList jid = participant.split("@");
                    Chat::JabberID jabberId(jid[0], jabberDomainForChat);
                    connection->sendDirectedPresence(Origin::Chat::JINGLE_PRESENCE_VOIP_CALLING, currentlyPlayingGame(), jabberId, mVoiceChannel);
                }
#endif
            }

            void VoiceController::onGetVoiceTokenFail(Origin::Services::HttpStatusCode errorCode)
            {
                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "onGetVoiceTokenFail: t=" << QDateTime::currentMSecsSinceEpoch();

                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                mOutgoingVoiceCallId.clear();

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
                leaveVoice(mConversationId);

                //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_START_FAIL, type()));
                // TODO: emit voiceCallEvent(voiceCallEventObject("START_FAIL").toVariantMap());
            }

            void VoiceController::answerCall()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                Services::GetChannelTokenResponse* response = dynamic_cast<Services::GetChannelTokenResponse*>(sender());
                if (!response)
                    return;

                if (mParticipants.count() != 1)
                {
                    ORIGIN_ASSERT(0);
                    return;
                }

                setVoiceCallState(mConversationId, VOICECALLSTATE_CONNECTED);
                QString nucluesId = mParticipants.at(0);
                emit voiceCallEvent(voiceCallEventObject("STARTED", nucluesId).toVariantMap());

                if (!startVoice(response->getToken()))
                {
                    setVoiceCallState(mConversationId, VOICECALLSTATE_DISCONNECTED);
                    emit voiceCallEvent(voiceCallEventObject("ENDED", nucluesId).toVariantMap());
                    return;
                }
            }

            bool VoiceController::startVoice(const QString& token)
            {
                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip)) << "startVoice: t=" << QDateTime::currentMSecsSinceEpoch();

                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return false;

                if (token.isEmpty())
                    return false;

                // Keep the call from timing out
                mIncomingVoiceCallTimer.stop();
                mOutgoingVoiceCallTimer.stop();
                ORIGIN_VERIFY_DISCONNECT(&mIncomingVoiceCallTimer, SIGNAL(timeout()), this, SLOT(onIncomingVoiceCallTimeout()));
                ORIGIN_VERIFY_DISCONNECT(&mOutgoingVoiceCallTimer, SIGNAL(timeout()), this, SLOT(onOutgoingVoiceCallTimeout()));

                UserRef user = Engine::LoginController::currentUser();
                if (!user.isNull() && mVoiceClient)
                {
                    ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceUserMuted(const QString&)), this, SLOT(onVoiceUserMuted(const QString&)));
                    ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceUserUnMuted(const QString&)), this, SLOT(onVoiceUserUnMuted(const QString&)));
                    ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceUserTalking(const QString&)), this, SLOT(onVoiceUserTalking(const QString&)));
                    ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceUserStoppedTalking(const QString&)), this, SLOT(onVoiceUserStoppedTalking(const QString&)));
                    ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceUserJoined(const QString&)), this, SLOT(onVoiceUserJoined(const QString&)));
                    ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceUserLeft(const QString&)), this, SLOT(onVoiceUserLeft(const QString&)));
                    ORIGIN_VERIFY_CONNECT(mVoiceClient, &Voice::VoiceClient::voiceConnected, this, &VoiceController::onVoiceConnected);
                    ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceDisconnected(const QString&, const QString&)), this, SLOT(onVoiceDisconnected(const QString&, const QString&)));
                    ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voicePushToTalkActive(bool)), this, SLOT(onVoicePushToTalkActive(bool)));
                    ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceChannelInactivity()), this, SLOT(onVoiceInactivity()));
                    ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceConnectError(int)), this, SLOT(onVoiceConnectError(int)));

                    // Make sure we are not in a muted state when we start voice
                    if (mVoiceClient->isMuted())
                    {
                        unmuteSelf(true /*startingVoice*/);
                    }

                    if (!mVoiceClient->createVoiceConnection(token, Engine::Voice::VoiceSettings(), false))
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

            void VoiceController::onIncomingVoiceCall(Origin::Chat::Conversable& caller, const QString& channel, bool /*isNewConversation*/)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled )
                    return;

#if 0 // TODO: fix
                // check to ensure that the call is meant for us
                if (mConversable.isNull() || (mConversable->jabberId() != caller.jabberId()))
                    return;
#endif

                // Ignore multiple incoming calls
                if (!mIncomingConversationId.isEmpty())
                {
                    return;
                }

                mIncomingVoiceCallTimer.stop();

                mIncomingVoiceCallTimer.setSingleShot(true);
                mIncomingVoiceCallTimer.setProperty("caller", QVariant::fromValue(&caller));
                mIncomingVoiceCallTimer.start(VOICE_CALL_TIMEOUT);

                ORIGIN_VERIFY_CONNECT(&mIncomingVoiceCallTimer, SIGNAL(timeout()), this, SLOT(onIncomingVoiceCallTimeout()));

                mIncomingVoiceChannel = channel;
                mIncomingParticipants = QStringList(caller.jabberId().node());
                mIncomingConversationId = caller.jabberId().node();
                setVoiceCallState(mIncomingConversationId, VOICECALLSTATE_INCOMINGCALL);
                //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), caller.jabberId(), VOICECALLEVENT_REMOTE_CALLING, type()));
                emit voiceCallEvent(voiceCallEventObject("INCOMING", mIncomingConversationId).toVariantMap());
            }

            void VoiceController::onLeavingVoiceCall(Origin::Chat::Conversable& caller)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

#if 0 // TODO: fix
                // check to ensure that the call is meant for us
                if (mConversable.isNull() || (mConversable->jabberId() != caller.jabberId()))
                    return;
#endif

                mReasonType = "User disconnection";
                mReasonDescription = "onLeavingVoiceCall: " + caller.jabberId().node();
                leaveVoice(caller.jabberId().node(), DONT_NOTIFY_PARTICIPANTS);
            }

            void VoiceController::onVoiceUserMuted(const QString& userId)
            {
#if 0
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
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
#endif
            }

            void VoiceController::onVoiceUserUnMuted(const QString& userId)
            {
#if 0
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
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
#endif
            }

            void VoiceController::onVoiceUserJoined(const QString& userId)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                Chat::OriginConnection *conn = mSocialController->chatConnection();
                ORIGIN_ASSERT(conn);
                if (!conn)
                {
                    return;
                }

                const Chat::JabberID jid(conn->nucleusIdToJabberId(userId.toULongLong()));

                // TODO: fix for Group Voice Chat
                //if (mType == OneToOneType)
                {
                    mOutgoingVoiceCallId.clear();
                    mOutgoingVoiceCallTimer.stop();
                    setVoiceCallState(userId, VOICECALLSTATE_CONNECTED);
                    //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_STARTED, type()));
                    QString nucleusId = mParticipants.at(0);
                    emit voiceCallEvent(voiceCallEventObject("STARTED", nucleusId).toVariantMap());
                }
#if 0 // TODO: fix for Group Voice Chat
                else // group chat
                {
                    ORIGIN_ASSERT(false);
                    addRemoteUserVoiceCallState(userId, REMOTEVOICESTATE_VOICE);
                    //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_USERJOINED, type()));
                }
#endif
            }

            void VoiceController::onVoiceUserLeft(const QString& userId)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                // When someone leaves, if this is 1:1 then end the call
                if (mVoiceClient && mVoiceClient->getUsersInVoice() == 1 /*&& mType == OneToOneType*/)
                {
                    mReasonType = "User disconnection";
                    mReasonDescription = "onVoiceUserLeft: " + userId;
                    leaveVoice(userId);
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
                    //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_USERLEFT, type()));
                }
            }

            void VoiceController::onVoiceUserTalking(const QString& userId)
            {
#if 0
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
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
#endif
            }

            void VoiceController::onVoiceUserStoppedTalking(const QString& userId)
            {
#if 0
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
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
#endif
            }

            void VoiceController::onVoicePushToTalkActive(bool active)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                if (mVoiceClient && mVoiceClient->getCaptureMode() == Origin::Engine::Voice::VoiceClient::PushToTalk)
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

            void VoiceController::onVoiceDisconnected(const QString& reasonType, const QString& reasonDescription)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                // decide which conversation to clean up after disconnection
                bool isHangingUp = false;
                QString conversationId = mConversationId;
                QStringList participants = mParticipants;
                QString voiceChannel = mVoiceChannel;
                if (reasonType == "HANGING_UP") // rejecting incoming call
                {
                    isHangingUp = true;
                    conversationId = mIncomingConversationId;
                    participants = mIncomingParticipants;
                    voiceChannel = mIncomingVoiceChannel;
                }

                // early exit if conversation is already disconnected
                if (isVoiceCallDisconnected(conversationId))
                    return;

                ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip))
                    << "onVoiceDisconnected: voice chat terminated, id=" << voiceChannel << ", reason: " << reasonType << ", " << reasonDescription;

                bool isOutgoingCall = (getVoiceCallState(conversationId) & VOICECALLSTATE_OUTGOINGCALL);
                bool isIncomingCall = (getVoiceCallState(conversationId) & VOICECALLSTATE_INCOMINGCALL);
                bool isDisconnected = (getVoiceCallState(conversationId) == VOICECALLSTATE_DISCONNECTED);

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
                    //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), connection->currentUser()->jabberId(), VOICECALLEVENT_INACTIVITY, type()));
                    QString nucleusId = participants.at(0);
                    emit voiceCallEvent(voiceCallEventObject("INACTIVITY", nucleusId).toVariantMap());
                }

                QString eventType;
                bool isServerIncompatible;
                if (reasonType == "PROTOCOL_VERSION") {
                    eventType = "START_FAIL_VERSION_MISMATCH";
                    mNotifyParticipants = DONT_NOTIFY_PARTICIPANTS;
                    isServerIncompatible = true;
                }
                else {
                    bool forceUnexpectedEnd = Origin::Services::readSetting(Origin::Services::SETTING_VoipForceUnexpectedEnd);

                    // Show error for unexpected disconnect (ignore "normal" disconnections)
                    if (forceUnexpectedEnd || (reasonType != "User disconnection" && reasonType != "HANGING_UP" && reasonType != "PROTOCOL_VERSION"))
                    {
                        eventType = "ENDED_UNEXPECTEDLY";
                    }
                    else if (isIncomingCall && reasonType == "HANGING_UP")
                    {
                        eventType = "ENDED_INCOMING";
                    }
                    else
                    {
                        eventType = "ENDED";
                    }
                    isServerIncompatible = false;
                };

                // Only display survey prompt if: not disconnected, English locale, and has BOI enabled
                bool promptForSurvey = (isSupportedLanguage && !isDisconnected && isVoiceEnabled && randomSelected && !isServerIncompatible);

                //if (mType == OneToOneType)
                {
                    if (mNotifyParticipants == NOTIFY_PARTICIPANTS)
                    {
                        // Send directed presence to the participant that we're leaving the call
                        Chat::OriginConnection* connection = mSocialController->chatConnection();
                        foreach(QString participant, participants)
                        {
                            QStringList jid = participant.split("@");
                            Chat::JabberID jabberId(jid[0], jabberDomainForChat);
                            connection->sendDirectedPresence(Origin::Chat::JINGLE_PRESENCE_VOIP_HANGINGUP, currentlyPlayingGame(), jabberId, voiceChannel);
                        }
                    }

                    //if (mConversable)
                    {
                        //Origin::Chat::JabberID jid = (mConversable) ? mConversable->jabberId() : Origin::Chat::JabberID();
                        //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_ENDED, type()));
                        QString nucleusId = participants.at(0);
                        emit voiceCallEvent(voiceCallEventObject(eventType, nucleusId).toVariantMap());
                        if (isOutgoingCall)
                        {
                            //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_NOANSWER, type()));
                            emit voiceCallEvent(voiceCallEventObject("NOANSWER", nucleusId).toVariantMap());
                        }

#if 0 // TODO: fix
                        // 9.5-SPECIFIC GROIP SURVEY PROMPT
                        if (promptForSurvey)
                        {
                            // Open the dialog only if the user is not in OIG
                            if (!Engine::IGOController::instance()->isVisible())
                                emit displaySurvey();
                        }
#endif
                    }
                }
#if 0 // TODO: fix for GroupChat
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
                        handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_ENDED, type()));

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
#endif

                // reset the notification flag to default setting (there should be some less stateful way of passing in this value
                // but this will have to do for the time being.
                mNotifyParticipants = NOTIFY_PARTICIPANTS;

                setVoiceCallState(conversationId, VOICECALLSTATE_DISCONNECTED);

                // Notify for cleanup
                emit voiceChatDisconnected();

                Origin::Engine::UserRef user = Engine::LoginController::currentUser();
                if (!user.isNull() && mVoiceClient)
                {
                    ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceUserMuted(const QString&)), this, SLOT(onVoiceUserMuted(const QString&)));
                    ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceUserUnMuted(const QString&)), this, SLOT(onVoiceUserUnMuted(const QString&)));
                    ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceUserTalking(const QString&)), this, SLOT(onVoiceUserTalking(const QString&)));
                    ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceUserStoppedTalking(const QString&)), this, SLOT(onVoiceUserStoppedTalking(const QString&)));
                    ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceUserJoined(const QString&)), this, SLOT(onVoiceUserJoined(const QString&)));
                    ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceUserLeft(const QString&)), this, SLOT(onVoiceUserLeft(const QString&)));
                    ORIGIN_VERIFY_DISCONNECT(mVoiceClient, &Voice::VoiceClient::voiceConnected, this, &VoiceController::onVoiceConnected);
                    ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceDisconnected(const QString&, const QString&)), this, SLOT(onVoiceDisconnected(const QString&, const QString&)));
                    ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voicePushToTalkActive(bool)), this, SLOT(onVoicePushToTalkActive(bool)));
                    ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceChannelInactivity()), this, SLOT(onVoiceInactivity()));
                    ORIGIN_VERIFY_DISCONNECT(mVoiceClient, SIGNAL(voiceConnectError(int)), this, SLOT(onVoiceConnectError(int)));
                }

                //mVoiceClient = NULL;
                if (isHangingUp)
                {
                    mIncomingVoiceChannel.clear();
                    mIncomingParticipants.clear();
                    mIncomingConversationId.clear();
                }
                else
                {
                    mParticipants.clear();
                    mConversationId.clear();
                    mVoiceChannel.clear();
                }
            }

            void VoiceController::onIncomingVoiceCallTimeout()
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
                leaveVoice(mIncomingConversationId);
                //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), conversable->jabberId(), VOICECALLEVENT_MISSED_CALL, type()));
                QString nucleusId = conversable->jabberId().node();
                emit voiceCallEvent(voiceCallEventObject("MISSED", nucleusId).toVariantMap());
            }

            void VoiceController::onOutgoingVoiceCallTimeout()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                ORIGIN_VERIFY_DISCONNECT(&mOutgoingVoiceCallTimer, SIGNAL(timeout()), this, SLOT(onOutgoingVoiceCallTimeout()));

                //Origin::Chat::Conversable* conversable = mOutgoingVoiceCallTimer.property("caller").value<Origin::Chat::Conversable*>();
                //if (!conversable)
                //    return;
                emit callTimeout();
                mReasonType = "User disconnection";
                mReasonDescription = "onOutgoingVoiceCallTimeout";
                leaveVoice(mConversationId);
                // No need to handle the VOICECALLEVENT_NOANSWER here it will be handled in the onVoiceDisconnected call
            }

            void VoiceController::onVoiceInactivity()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                // Leave voice and spit out an error message
                // Don't send directed presence to other voice participants!
                // They will get the inactivity event and disconnect themselves.
                mReasonType = "User disconnection";
                mReasonDescription = "onVoiceInactivity";
                leaveVoice(mConversationId, DONT_NOTIFY_PARTICIPANTS);
            }

            void VoiceController::onVoiceConnectError(int errorCode)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                QString nucleusId = mParticipants.at(0);
                QString errorCodeString = QString::number(errorCode, 16).right(8);
                emit voiceCallEvent(voiceCallEventObject("CONNECT_ERROR", nucleusId, errorCodeString).toVariantMap());
            }

            void VoiceController::setVoiceCallState(const QString& id, int newVoiceCallState)
            {
                mVoiceCallState[id] = newVoiceCallState;
                emit voiceCallStateChanged(id, mVoiceCallState[id]);
            }

            int VoiceController::getVoiceCallState(const QString& id)
            {
                if (mVoiceCallState.contains(id))
                {
                    return mVoiceCallState[id];
                }

                return 0;
            }

            void VoiceController::addVoiceCallState(const QString& id, int newVoiceCallState)
            {
                mVoiceCallState[id] |= newVoiceCallState;
                emit voiceCallStateChanged(id, mVoiceCallState[id]);
            }

            void VoiceController::clearVoiceCallState(const QString& id, int newVoiceCallState)
            {
                mVoiceCallState[id] = mVoiceCallState[id] & ~newVoiceCallState;
                emit voiceCallStateChanged(id, mVoiceCallState[id]);
            }

            void VoiceController::setRemoteUserVoiceCallState(const QString& userId, int newVoiceCallState)
            {
                mRemoteUserVoiceCallState[userId] = newVoiceCallState;
            }

            void VoiceController::addRemoteUserVoiceCallState(const QString& userId, int newVoiceCallState)
            {
                mRemoteUserVoiceCallState[userId] |= newVoiceCallState;
            }

            void VoiceController::clearRemoteUserVoiceCallState(const QString& userId, int newVoiceCallState)
            {
                mRemoteUserVoiceCallState[userId] = mVoiceCallState[userId] & ~newVoiceCallState;
            }

            void VoiceController::muteSelf()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                if (!isVoiceCallInProgress(mConversationId))
                {
                    return;
                }

                addVoiceCallState(mConversationId, VOICECALLSTATE_MUTED);

                //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), mSocialController->chatConnection()->currentUser()->jabberId(), VOICECALLEVENT_USER_LOCAL_MUTED, type()));
                QString nucleusId = mParticipants.at(0);
                emit voiceCallEvent(voiceCallEventObject("LOCAL_MUTED", nucleusId).toVariantMap());

                muteConnection();
            }

            void VoiceController::unmuteSelf(bool startingVoice)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                unmuteConnection(startingVoice);
            }

            void VoiceController::muteConnection()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                if (mVoiceClient)
                    mVoiceClient->mute();

                addVoiceCallState(mConversationId, VOICECALLSTATE_MUTED);

                Chat::OriginConnection *connection = mSocialController->chatConnection();
                ORIGIN_ASSERT(connection);
                if (!connection)
                {
                    return;
                }

#if 0 // TODO: fix for rooms
                if (Chat::MucRoom* room = dynamic_cast<Chat::MucRoom*>(mConversable.data()))
                {
                    connection->sendDirectedPresence(Origin::Chat::JINGLE_PRESENCE_VOIP_JOIN, currentlyPlayingGame(), room->jabberId().withResource(room->nickname()), "MUTE");
                }
#endif
            }

            void VoiceController::unmuteConnection(bool startingVoice)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                if (!isVoiceCallInProgress(mConversationId))
                {
                    return;
                }

                if (mVoiceClient)
                    mVoiceClient->unmute();
                clearVoiceCallState(mConversationId, VOICECALLSTATE_MUTED | VOICECALLSTATE_ADMIN_MUTED);

                Chat::OriginConnection *connection = mSocialController->chatConnection();
                ORIGIN_ASSERT(connection);
                if (!connection)
                {
                    return;
                }

                if (!startingVoice)
                {
                    //QString user = Services::Session::SessionService::nucleusUser(Services::Session::SessionService::currentSession());
                    //const Chat::JabberID jid(connection->nucleusIdToJabberId(user.toULongLong()));
                    //handleNewEvent(new VoiceCallEvent(QDateTime::currentDateTime(), jid, VOICECALLEVENT_USER_LOCAL_UNMUTED, type()));
                    QString nucleusId = mParticipants.at(0);
                    emit voiceCallEvent(voiceCallEventObject("LOCAL_UNMUTED", nucleusId).toVariantMap());
                }

#if 0 // TODO: fix for rooms
                if (Chat::MucRoom* room = dynamic_cast<Chat::MucRoom*>(mConversable.data()))
                {
                    connection->sendDirectedPresence(Origin::Chat::JINGLE_PRESENCE_VOIP_JOIN, currentlyPlayingGame(), room->jabberId().withResource(room->nickname()), "");
                }
#endif
            }

            void VoiceController::showToast(const QString& event, const QString& originId, const QString& conversationId)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                if (event == "INCOMING")
                {
                    emit showToast_IncomingVoiceCall(originId, conversationId);
                }
            }

            void VoiceController::showSurvey(const QString& voiceChannelId)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                emit showSurvey_Voice(voiceChannelId);
            }

            QJsonObject VoiceController::voiceCallEventObject(const QString& event, const QString& nucleusId, const QString& errorCode)
            {
                QJsonObject obj;

                obj["event"] = event;
                obj["nucleusId"] = nucleusId;
                obj["errorCode"] = errorCode;

                return obj;
            }

            bool VoiceController::doesFriendSupportVoice(QString const& participantNucleusId) const {

                if (!mSocialController)
                    return false;

                Chat::OriginConnection* connection = mSocialController->chatConnection();
                if (!connection)
                    return false;

                Chat::ConnectedUser* currentUser = connection->currentUser();
                if (!currentUser)
                    return false;

                Chat::Roster* roster = currentUser->roster();
                if (!roster)
                    return false;

                QSet<Chat::RemoteUser*> contacts = roster->contacts();
                if (contacts.size() == 0)
                    return false;

                for (Chat::Roster::RosterSet::const_iterator i(contacts.cbegin()), last(contacts.cend()); i != last; ++i) {
                    if ((*i)->nucleusId() == participantNucleusId) {
                        return (*i)->capabilities().contains("VOIP");
                    }
                }

                return false;
            }
        }
    }
}

#endif //ENABLE_VOICE_CHAT
