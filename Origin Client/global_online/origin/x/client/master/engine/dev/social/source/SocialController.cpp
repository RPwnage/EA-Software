// Copyright 2012, Electronic Arts

#include <QString>
#include <QUrl>

#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/session/SessionService.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/voice/VoiceService.h"

#include "TelemetryAPIDLL.h"

#include "engine/dirtybits/DirtyBitsClient.h"
#include "engine/social/SocialController.h"
#include "engine/social/UserAvailabilityController.h"
#include "engine/social/UserNamesCache.h"
#include "engine/social/ConversationManager.h"
#include "engine/social/ConversationHistoryManager.h"
#include "engine/social/CommonTitlesController.h"
#include "engine/social/AvatarManager.h"
#include "engine/social/SdkPresenceController.h"
#include "engine/social/OnlineContactCounter.h"
#include "engine/voice/VoiceController.h"

#include "engine/multiplayer/InviteController.h"

#include "chat/ChatGroup.h"
#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"
#include "chat/Presence.h"
#include "chat/RemoteUser.h"
#include "chat/Roster.h"

#include "UserIdleWatcher.h"
#include "InGameStatusWatcher.h"
#include "ConnectedUserPresenceStack.h"
#include "BlockedUserMessageFilter.h"

#include "services/log/LogService.h"

namespace Origin
{
namespace Engine
{
namespace Social
{
    SocialController::SocialController(UserRef user) :
        mUser(user),
        mUserNames(new UserNamesCache(user, this)),
        mSetInitialVisibility(false)
    {
        using Services::Session::SessionService;

        // Parse out our connection string
        const QString chatUrlString(Services::readSetting(Origin::Services::SETTING_ChatURL, user->getSession()));
        const QUrl chatUrl(chatUrlString);

        // Create a configuration for our connection
        Chat::Configuration config;
        config.setIgnoreInvalidCert(Services::readSetting(Services::SETTING_IgnoreSSLCertError, user->getSession()));

        // Create our XMPP connection
        mChatConnection.reset(new Chat::OriginConnection(user->getSession(), QUrl::toAce(chatUrl.host()), chatUrl.port(5222), config));

        // Create our internal presence stack
        mPresenceStack.reset(new ConnectedUserPresenceStack(mChatConnection->currentUser()));

        // Create our external-facing UserAvailabilityController
        mUserAvailability.reset(new UserAvailabilityController(mPresenceStack.data()));

        // Automatically update our presence when we're idle
        if (!Services::readSetting(Services::SETTING_DisableIdling))
        {
            new UserIdleWatcher(mPresenceStack.data(), this);
        }

        // Automatically update our presence for game launches/exists
        new InGameStatusWatcher(mPresenceStack.data(), user, this);
        
        // Allows the SDK to set our presence
        mSdkPresence.reset(new SdkPresenceController(mPresenceStack.data()));

        // Create our multi-player invites controller
        mMultiplayerInvites.reset(new MultiplayerInvite::InviteController(user));
        mChatConnection->installIncomingMessageFilter(mMultiplayerInvites.data());

        // Create our conversation manager
        mConversations.reset(new ConversationManager(this));

        // Create our groups manager
        //mGroupManager = new
        
        mConversationHistory.reset(new ConversationHistoryManager(mConversations.data(), mChatConnection->nucleusIdToJabberId(user->userId())));
        mConversationHistory->setDiskRecordingEnabled(Services::readSetting(Services::SETTING_SAVECONVERSATIONHISTORY, user->getSession()));

        // Create our common titles controller
        mCommonTitles.reset(new CommonTitlesController(user, mChatConnection->currentUser()->roster()));

        // Create our avatar managers
        mSmallAvatarManager.reset(new AvatarManager(user->getSession(), Services::AvatarServiceClient::Size_40X40));
        mMediumAvatarManager.reset(new AvatarManager(user->getSession(), Services::AvatarServiceClient::Size_208X208));
        mLargeAvatarManager.reset(new AvatarManager(user->getSession(), Services::AvatarServiceClient::Size_416X416));

        // Subscribe to our own avatars
        mSmallAvatarManager->subscribe(user->userId());
        mMediumAvatarManager->subscribe(user->userId());

        mBlockedUserMessageFilter.reset(new BlockedUserMessageFilter(mChatConnection->currentUser()->blockList()));
        mChatConnection->installIncomingMessageFilter(mBlockedUserMessageFilter.data());

        // Create our online user counter
        mOnlineContactCounter.reset(new OnlineContactCounter(mChatConnection->currentUser()->roster()));

        // Hook up our signals
        ORIGIN_VERIFY_CONNECT(Services::Connection::ConnectionStatesService::instance(), SIGNAL(nowOfflineUser()),
                this, SLOT(onNowOfflineUser()));

        Chat::Roster *roster = mChatConnection->currentUser()->roster();

        ORIGIN_VERIFY_CONNECT(roster, SIGNAL(loaded()), this, SLOT(onRosterLoaded()));

        ORIGIN_VERIFY_CONNECT(roster, SIGNAL(contactAdded(Origin::Chat::RemoteUser *)),
                this, SLOT(onContactAdded(Origin::Chat::RemoteUser *)));

        ORIGIN_VERIFY_CONNECT(roster, SIGNAL(contactRemoved(Origin::Chat::RemoteUser *)),
            this, SLOT(onContactRemoved(Origin::Chat::RemoteUser *)));

        ORIGIN_VERIFY_CONNECT(
                Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString &, const Origin::Services::Variant &)),
                this, SLOT(onSettingChanged(const QString &, const Origin::Services::Variant &)));

        ORIGIN_VERIFY_CONNECT(mMultiplayerInvites.data(), SIGNAL(inviteReceivedFrom(const Origin::Chat::JabberID &)), 
            roster, SLOT(adjustUserPresenceOnInvite(const Origin::Chat::JabberID &)))

        Origin::Engine::DirtyBitsClient::instance()->registerHandler(this, "onGroupsEvent", "group");
    }

    SocialController::~SocialController()
    {
        mConversations->finishAllConversations(Conversation::UserFinished);
 
    }

    SocialController* SocialController::create(UserRef user)
    {
        return new SocialController(user);
    }

    void SocialController::loginToChat()
    {
        UserRef user(mUser.toStrongRef());

        if (user)
        {
            // Login
            //check and see if singlelogin is disabled
            bool singleLoginEnabled = Origin::Services::readSetting(Origin::Services::SETTING_SingleLogin, user->getSession());
            mChatConnection->login(!singleLoginEnabled);
            
            // We only want to set our initial visibility once. loginToChat can be called multiple times in a session
            // and the user might have already overridden the initial visibility. We can't do this in our constructor
            // because we're constructed before SETTING_LOGIN_AS_INVISIBLE is set
            if (!mSetInitialVisibility)
            {
                if (Services::readSetting(Origin::Services::SETTING_LOGIN_AS_INVISIBLE, user->getSession()))
                {
                    // Become invisible before setting presence
                    mChatConnection->currentUser()->requestVisibility(Chat::ConnectedUser::Invisible);
                }

                mSetInitialVisibility = true;
            }

            // We haven't explicitly set anything yet so tell PresenceStack to update our Chat Model anyway
            mPresenceStack->applyToConnectedUser();
        }
    }

    Chat::OriginConnection* SocialController::chatConnection()
    {
        return mChatConnection.data();
    }

    void SocialController::onContactAdded(Chat::RemoteUser *c)
    {
        // Get their nucleus ID from their Jabber ID
        quint64 nucleusId = mChatConnection->jabberIdToNucleusId(c->jabberId());

        if (!c->originId().isNull())
        {
            // Populate the identity cache with their Origin ID
            // Most users don't need full identity so this prevents a server trip
            mUserNames->setOriginIdForNucleusId(nucleusId, c->originId());
        }

        // Request their small avatar
        mSmallAvatarManager->subscribe(nucleusId);

#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( isVoiceEnabled )
        {
            ORIGIN_VERIFY_CONNECT(
                c, SIGNAL(incomingVoiceCall(const QString&)),
                this, SLOT(onIncomingVoiceCall(const QString&)));
            ORIGIN_VERIFY_CONNECT(
                c, SIGNAL(leavingVoiceCall()),
                this, SLOT(onLeavingVoiceCall()));
            ORIGIN_VERIFY_CONNECT(
                c, SIGNAL(joiningGroupVoiceCall(const QString&)),
                this, SLOT(onJoiningGroupVoiceCall(const QString&)));
            ORIGIN_VERIFY_CONNECT(
                c, SIGNAL(leavingGroupVoiceCall()),
                this, SLOT(onLeavingGroupVoiceCall()));
        }
#endif
    }

    void SocialController::onContactRemoved(Chat::RemoteUser *c)
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( isVoiceEnabled )
        {
            ORIGIN_VERIFY_DISCONNECT(
                c, SIGNAL(incomingVoiceCall(const QString&)),
                this, SLOT(onIncomingVoiceCall(const QString&)));
            ORIGIN_VERIFY_DISCONNECT(
                c, SIGNAL(leavingVoiceCall()),
                this, SLOT(onLeavingVoiceCall()));
            ORIGIN_VERIFY_DISCONNECT(
                c, SIGNAL(joiningGroupVoiceCall(const QString&)),
                this, SLOT(onJoiningGroupVoiceCall(const QString&)));
            ORIGIN_VERIFY_DISCONNECT(
                c, SIGNAL(leavingGroupVoiceCall()),
                this, SLOT(onLeavingGroupVoiceCall()));
        }
#endif
    }

    void SocialController::onIncomingVoiceCall(const QString& incomingVoiceChannel)
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( isVoiceEnabled )
        {
            if (Origin::Services::readSetting(Origin::Services::SETTING_DeclineIncomingVoiceChatRequests))
                return;
        }
#endif
        Origin::Chat::Conversable* caller = dynamic_cast<Origin::Chat::Conversable*>(sender());
        if (!caller)
            return;

        bool isNewConversation = !conversations()->existsConversationForConversable(*caller);

        // create a new conversation, if necessary, before the notification
        QSharedPointer<Conversation> conversation = conversations()->conversationForConversable(caller, Origin::Engine::Social::Conversation::InternalRequest);
        if (!conversation)
            return;

#if ENABLE_VOICE_CHAT
        if( isVoiceEnabled )
        {
            QString callerId = caller->jabberId().node();
            Voice::VoiceController *voiceController = user()->voiceControllerInstance();
            if (voiceController && (voiceController->outgoingVoiceCallId() == callerId))
            {
                voiceController->resetOutgoingVoiceCallId();
                return;
            }
        }
#endif

        emit incomingVoiceCall(*caller, incomingVoiceChannel, isNewConversation);
    }

    void SocialController::onLeavingVoiceCall()
    {
        Origin::Chat::Conversable* caller = dynamic_cast<Origin::Chat::Conversable*>(sender());
        if (!caller)
            return;

#if ENABLE_VOICE_CHAT
        QString callerId = caller->jabberId().node();
        Voice::VoiceController *voiceController = user()->voiceControllerInstance();
        if (voiceController && (voiceController->outgoingVoiceCallId() == callerId))
        {
            voiceController->resetOutgoingVoiceCallId();
        }
#endif

        emit leavingVoiceCall(*caller);
    }

    void SocialController::onJoiningGroupVoiceCall(const QString& description)
    {
        Origin::Chat::Conversable* caller = dynamic_cast<Origin::Chat::Conversable*>(sender());
        if (!caller)
            return;

        emit joiningGroupVoiceCall(*caller, description);
    }

    void SocialController::onLeavingGroupVoiceCall()
    {
        Origin::Chat::Conversable* caller = dynamic_cast<Origin::Chat::Conversable*>(sender());
        if (!caller)
            return;

        emit leavingGroupVoiceCall(*caller);
    }

    void SocialController::onNowOfflineUser()
    {
        // Our XMPP connection can happily live on without the rest of the client being online
        // However, single login is current implemented by checking for XMPP conflicts
        // If the client is displaying itself as offline we should resign our logged in status on the XMPP server so
        // other clients have a chance to connect
        mChatConnection->close();
    }

    void SocialController::onRosterLoaded()
    {
        int numContacts = mChatConnection->currentUser()->roster()->contacts().size();
        GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_END(EbisuTelemetryAPI::LoginToFriendsList, numContacts);
        GetTelemetryInterface()->Metric_FRIEND_COUNT(numContacts);
    }

    AvatarManager* SocialController::smallAvatarManager()
    {
        return mSmallAvatarManager.data();
    }

    AvatarManager* SocialController::mediumAvatarManager()
    {
        return mMediumAvatarManager.data();
    }
    
    AvatarManager* SocialController::largeAvatarManager()
    {
        return mLargeAvatarManager.data();
    }

    UserAvailabilityController *SocialController::userAvailability()
    {
        return mUserAvailability.data();
    }

    UserNamesCache *SocialController::userNames()
    {
        return mUserNames.data();
    }
    
    ConversationManager *SocialController::conversations()
    {
        return mConversations.data();
    }
    
    ConversationHistoryManager *SocialController::conversationHistory()
    {
        return mConversationHistory.data();
    }

    CommonTitlesController *SocialController::commonTitles()
    {
        return mCommonTitles.data();
    }

    MultiplayerInvite::InviteController *SocialController::multiplayerInvites()
    {
        return mMultiplayerInvites.data();
    }

    SdkPresenceController *SocialController::sdkPresence()
    {
        return mSdkPresence.data();
    }
        
    void SocialController::onSettingChanged(const QString& settingName, const Origin::Services::Variant& value)
    {
        if (settingName == Services::SETTING_SAVECONVERSATIONHISTORY.name())
        {
            mConversationHistory->setDiskRecordingEnabled(value);
        }
    }
        
    OnlineContactCounter* SocialController::onlineContactCounter()
    {
        return mOnlineContactCounter.data();
    }

    void SocialController::suscribeToRefreshAvatars(quint64 userId)
    {
        smallAvatarManager()->subscribe(userId);
        mediumAvatarManager()->subscribe(userId);
        largeAvatarManager()->subscribe(userId);
    }

    void SocialController::onGroupsEvent(QByteArray payload)
    {
        QJsonParseError jsonResult;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &jsonResult);

        //check if its a valid JSON response
        if(jsonResult.error == QJsonParseError::NoError)
        {
            QJsonObject obj = jsonDoc.object();
            QJsonObject data;

            data = obj["data"].toObject();

            QString type = data["type"].toString();

            if (type.compare("INVITATION_SENT") == 0)
            {
                QString groupName = data["groupName"].toString();
                QString groupGuid = data["groupGuid"].toString();
                qint64 by = data["by"].toDouble();
                QString invitedBy = QString("%1").arg(by);

                Chat::ChatGroups* groups = chatConnection()->currentUser()->chatGroups();
                if (groups)
                {
                    groups->addGroupInvite(groupGuid, groupName, by);
                    emit inviteToChatGroup(invitedBy, groupGuid);
                }
            }
            else if (type.compare("GROUP_NAME_CHANGE") == 0)
            {
                QString groupGuid = data["groupGuid"].toString();
                QString groupName = data["newName"].toString();

                Chat::ChatGroups* groups = chatConnection()->currentUser()->chatGroups();
                if (groups)
                {
                    groups->editGroupName(groupGuid, groupName);
                    emit updateGroupName(groupGuid, groupName);
                }
            }
            else if (type.compare("MEMBER_REMOVED")==0)
            {
                QString groupGuid = data["groupGuid"].toString();
                QJsonArray members = data["members"].toArray();
                qint64 removedBy = data["by"].toDouble();

                for (int i = 0; i < members.size(); ++i)
                {
                    quint64 userId = members[i].toDouble();
                    Chat::ChatGroups* groups = chatConnection()->currentUser()->chatGroups();
                    if (groups)
                    {
                        groups->removeMemberFromGroup(groupGuid, userId);

                        Chat::JabberID jid = chatConnection()->currentUser()->jabberId();
                        quint64 currentUserId = mChatConnection->jabberIdToNucleusId(jid);
                        if (userId == currentUserId && removedBy != currentUserId)
                        {
                            Chat::ChatGroup* group = groups->chatGroupForGroupGuid(groupGuid);
                            if (group)
                            {
                                QString groupName = group->getName();

                                emit kickedFromGroup(groupName, groupGuid, removedBy);

                                groups->removeGroupUI(groupGuid);
                                // Send Telemetry that we were kicked from a room
                                GetTelemetryInterface()->Metric_CHATROOM_KICKED_FROM_GROUP(groupGuid.toLatin1(), groups->count());
                            }
                        }
                    }
                }
            }
            else if (type.compare("MEMBER_ROLE_CHANGED") == 0)
            {
                QString groupGuid = data["groupGuid"].toString();
                QString role = data["r"].toString();
                quint64 userId = data["id"].toDouble();

                Chat::ChatGroups* groups = chatConnection()->currentUser()->chatGroups();
                if (groups)
                {
                    UserRef user(mUser.toStrongRef());
                    if (user->userId() == userId)
                    {
                        groups->editCurrentUserGroupRole(groupGuid, userId, role);
                    } else {
                        groups->editGroupMemberRole(groupGuid, userId, role);
                    }
                }
            }
            else if (type.compare("MUC_GROUP_ROOM_DESTROYED") == 0)
            {
                QString groupGuid = data["groupGuid"].toString();
                QString channelId = data["roomId"].toString();
                QString channelName = data["roomName"].toString();

                Chat::ChatGroups* groups = chatConnection()->currentUser()->chatGroups();
                if (groups)
                {
                    groups->handleRoomDestroyed(groupGuid, channelId);
                }
            }
            else if (type.compare("GROUP_DESTROYED") == 0)
            {
                QString groupGuid = data["groupGuid"].toString();
                quint64 removedBy = data["by"].toDouble();

                Chat::ChatGroups* groups = chatConnection()->currentUser()->chatGroups();
                if (groups)
                {
                    Chat::ChatGroup* group = groups->chatGroupForGroupGuid(groupGuid);
                    if( group )
                    {
                        QString groupName = group->getName();

                        // Only want to emit this if the user didn't remove themselves
                        Chat::JabberID jid = chatConnection()->currentUser()->jabberId();
                        quint64 currentUserId = mChatConnection->jabberIdToNucleusId(jid);
                        if (removedBy != currentUserId)
                        {
                            emit groupDestroyed(groupName, removedBy, groupGuid);
                            
                            // Only remove this grop and send telemetry if this was not a current User deleted group
                            // which is handled in the DeleteGroupViewController::onDeleteGroupSuccess()
                            groups->removeGroupUI(groupGuid);
                            // Send Telemetry that our group was deleted
                            GetTelemetryInterface()->Metric_CHATROOM_GROUP_WAS_DELETED(groupGuid.toLatin1(), groups->count());
                        }
                    }
                }
            }
            // Since the room create message doesn't actually have information in it yet since the room
            // config is still being parsed, listen to rename and password change for room creation
            else if (type.compare("MUC_GROUP_ROOM_PWD_REQUIREMENT_CHANGED") == 0)
            {
                QString groupGuid = data["groupGuid"].toString();
                QString channelId = data["roomId"].toString();
                // Always getting false for required here, currently a bug
                bool requiredPassword = data["requiredPWD"].toBool();
                Q_UNUSED(requiredPassword);
            }
            else if (type.compare("MUC_GROUP_ROOM_RENAMED") == 0)
            {
                QString groupGuid = data["groupGuid"].toString();
                QString channelId = data["roomId"].toString();
                QString channelName = data["roomName"].toString();

                QString mucHost = "@muc.";
                mucHost.append(chatConnection()->host());

                QString channelJid(channelId + mucHost);

                // Only way we can get this message is for a new room since rooms can't be renamed currently
                Chat::ChatGroups* groups = chatConnection()->currentUser()->chatGroups();
                if (groups)
                {
                    groups->addChannelToGroup(groupGuid, channelName, false, channelJid);
                }
            }
        }
    }
}
}
}
