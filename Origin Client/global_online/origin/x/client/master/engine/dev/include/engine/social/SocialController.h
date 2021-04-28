#ifndef _SOCIALCONTROLLER_H
#define _SOCIALCONTROLLER_H

// Copyright 2012, Electronic Arts
// All rights reserved.

#include <QObject>
#include <QScopedPointer>

#include "engine/login/User.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Chat
{
    class Conversable;
    class OriginConnection;
    class RemoteUser;
    class Presence;
}

namespace Services
{
    class Variant;
}

namespace Engine
{

namespace MultiplayerInvite
{
    class InviteController;
}


namespace Social
{
    class ConversationManager;
    class ConversationHistoryManager;
    class ConnectedUserPresenceStack;
    class UserAvailabilityController;
    class CommonTitlesController;
    class AvatarManager;
    class SdkPresenceController;
    class UserNamesCache;
    class BlockedUserMessageFilter;
    class OnlineContactCounter;

    /// \brief Controller for a user's social-related functionality
    ///
    /// Currently this includes chat
    class ORIGIN_PLUGIN_API SocialController : public QObject
    {
        Q_OBJECT
    public:
        virtual ~SocialController();

        /// \brief Creates a new social controller.
        ///
        /// \param  user  The user to create the social controller for
        static SocialController *create(UserRef user);

        /// \brief Logs in to the XMPP chat server
        void loginToChat();

        /// \brief Returns the XMPP chat connection
        Chat::OriginConnection *chatConnection();

        /// \brief Returns the user that this controller is associated with
        UserRef user() { return mUser; }

        /// \brief Returns our user identity cache
        UserNamesCache *userNames();

        /// \brief Returns the conversation manager
        ConversationManager *conversations();
        
        ///
        /// \brief Returns the conversation history manager
        ///
        /// This will return NULL if conversation history is disabled
        ///
        ConversationHistoryManager *conversationHistory();

        /// \brief Returns the common titles controller
        CommonTitlesController *commonTitles();

        /// \brief Returns the multiplayer game invites controller
        MultiplayerInvite::InviteController *multiplayerInvites();

        /// \brief register client to get its avatar refreshed with avatar manager
        void suscribeToRefreshAvatars(quint64 userId);

        ///
        /// \brief Returns the avatar manager for 40x40 avatars
        ///
        /// By default this manager is subscribed to the current user and of their all contacts
        ///
        AvatarManager *smallAvatarManager();
        
        ///
        /// \brief Returns the avatar manager for 208x208 avatars
        ///
        /// By default this manager is only subscribed to the current user
        ///
        AvatarManager *mediumAvatarManager();
        
        ///
        /// \brief Returns the avatar manager for 416x416 avatars
        ///
        /// By default this manager is not subscribed to any users
        ///
        AvatarManager *largeAvatarManager();

        ///
        /// \brief Returns our user-level availability controller
        ///
        /// This handles transitions between user visible availability states
        ///
        UserAvailabilityController *userAvailability();

        ///
        /// \brief Returns our SDK presence controller
        ///
        /// This allows games to influence our presence and set our multiplayer session via the Origin SDK
        ///
        SdkPresenceController *sdkPresence();
        
        ///
        /// \brief Returns the online contact counter
        ///
        /// This gives a cached value for the number of online contacts the current user has
        ///
        OnlineContactCounter *onlineContactCounter();

    signals:

        void incomingVoiceCall(Origin::Chat::Conversable& caller, const QString& channel, bool isNewConversation);
        void leavingVoiceCall(Origin::Chat::Conversable& caller);
        void joiningGroupVoiceCall(Origin::Chat::Conversable& caller, const QString& description);
        void leavingGroupVoiceCall(Origin::Chat::Conversable& caller);
        void groupDestroyed(const QString&, qint64, const QString&);
        void kickedFromGroup(const QString&, const QString&, qint64);
        void updateGroupName(const QString&, const QString&);
        void failedToEnterRoom();
        void inviteToChatGroup(const QString& from, const QString& groupGuid);

    private slots:

        void onContactAdded(Origin::Chat::RemoteUser *);
        void onContactRemoved(Origin::Chat::RemoteUser *);
        void onNowOfflineUser();
        void onRosterLoaded();
        void onSettingChanged(const QString& settingName, const Origin::Services::Variant& value);

        void onGroupsEvent(QByteArray payload);

        void onIncomingVoiceCall(const QString& incomingVoiceChannel);
        void onLeavingVoiceCall();
        void onJoiningGroupVoiceCall(const QString& description);
        void onLeavingGroupVoiceCall();

    private:
        SocialController(UserRef user);

        UserWRef mUser;

        QScopedPointer<Chat::OriginConnection> mChatConnection;
        QScopedPointer<UserNamesCache> mUserNames;
        QScopedPointer<ConversationManager> mConversations;
        QScopedPointer<ConversationHistoryManager> mConversationHistory;
        QScopedPointer<CommonTitlesController> mCommonTitles;
        QScopedPointer<MultiplayerInvite::InviteController> mMultiplayerInvites;

        QScopedPointer<AvatarManager> mSmallAvatarManager;
        QScopedPointer<AvatarManager> mMediumAvatarManager;
        QScopedPointer<AvatarManager> mLargeAvatarManager;

        QScopedPointer<ConnectedUserPresenceStack> mPresenceStack;
        QScopedPointer<UserAvailabilityController> mUserAvailability;

        QScopedPointer<SdkPresenceController> mSdkPresence;

        QScopedPointer<BlockedUserMessageFilter> mBlockedUserMessageFilter;

        QScopedPointer<OnlineContactCounter> mOnlineContactCounter;

        bool mSetInitialVisibility;
    };
}
}
}

#endif
