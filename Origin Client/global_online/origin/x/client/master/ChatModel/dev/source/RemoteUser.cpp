#include "RemoteUser.h"
#include <QMutexLocker>

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/rest/GroupServiceClient.h"
#include "services/settings/SettingsManager.h"
#include "services/voice/VoiceService.h"

#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "engine/social/UserNamesCache.h"

#include "Connection.h"
#include "OriginConnection.h"
#include "ConnectedUser.h"
#include "XMPPImplAvailabilityConversion.h"
#include "XMPPImplEventAdapter.h"

namespace Origin
{
namespace Chat
{
    ///
    enum Capabilities
    {
        NoCapabilities  = 0x00,
        CAPS            = 0x01,
        CHATSTATES      = 0x02,
        DISCOINFO       = 0x04,
        MUC             = 0x08,
        VOIP            = 0x10
    };


    RemoteUser::RemoteUser(const JabberID &jabberId, Connection *connection, QObject *parent) :
        XMPPUser(connection, parent),
        mJabberId(jabberId),
        mStateLock(QMutex::Recursive),
        mSubscriptionState(SubscriptionState::DirectionNone, false, false),
        mCapabilities(NoCapabilities)
    {
        refreshCapabilities();

        ORIGIN_VERIFY_CONNECT(
                connection, SIGNAL(disconnected(Origin::Chat::Connection::DisconnectReason)),
                this, SLOT(disconnected()));
    }
    
    QSet<ConversableParticipant> RemoteUser::participants() const
    {
        QSet<ConversableParticipant> participants;
        participants << ConversableParticipant(rosterNickname(), const_cast<RemoteUser*>(this));
        return participants;
    }

    Presence RemoteUser::presence() const
    {
        QMutexLocker locker(&mStateLock);
        return mPresence;
    }

    SubscriptionState RemoteUser::subscriptionState() const
    {
        QMutexLocker locker(&mStateLock);
        return mSubscriptionState;
    }

    QString RemoteUser::capabilities() const
    {
        QString caps;

        if (mCapabilities & CAPS)
            caps.append("CAPS");
        if (mCapabilities & CHATSTATES)
        {
            if (!caps.isEmpty())
                caps.append(",");
            caps.append("CHATSTATES");
        }
        if (mCapabilities & DISCOINFO)
        {
            if (!caps.isEmpty())
                caps.append(",");
            caps.append("DISCOINFO");
        }
        if (mCapabilities & MUC)
        {
            if (!caps.isEmpty())
                caps.append(",");
            caps.append("MUC");
        }
        if (mCapabilities & VOIP)
        {
            if (!caps.isEmpty())
                caps.append(",");
            caps.append("VOIP");
        }

        return caps;
    }

    void RemoteUser::possiblePresenceChange(const Presence &newPresence, const Presence &oldPresence)
    {
        if (newPresence != oldPresence)
        {
#if ENABLE_VOICE_CHAT
            bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
            if( isVoiceEnabled )
            {
                // look for voice chat directed presence
                if (newPresence.userActivity().general == Origin::Chat::DP_CALLING)
                {
                    // if incoming call and there's nobody listening
                    bool isConversationListening = receivers(SIGNAL(incomingVoiceCall(const QString&)));
                    if (isConversationListening)
                    {
                        ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip))
                            << "Conversation::onIncomingVoiceCall: VOIP "
                            << newPresence.userActivity().general
                            << ", "
                            << newPresence.userActivity().specific;

                        emit incomingVoiceCall(QString::fromUtf8(newPresence.userActivity().description.c_str()));
                    }
                    else
                    {
                        // present a user notification
                    }
                }
                else if (newPresence.userActivity().general == Origin::Chat::DP_HANGINGUP)
                {
                    ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip))
                        << "RemoteUser:leavingVoiceCall: VOIP "
                        << newPresence.userActivity().general
                        << ", "
                        << newPresence.userActivity().specific;

                    emit leavingVoiceCall();
                }
                // We only care about these messages if we're not in the voice channel as well.
                // If we're in the voice channel we get much more finely tuned information directly
                // from sonar
                else if (newPresence.userActivity().general == Origin::Chat::DP_JOIN)
                {
                    ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip))
                        << "RemoteUser::joiningGroupVoiceCall: VOIP "
                        << newPresence.userActivity().general
                        << ", "
                        << newPresence.userActivity().specific;

                    emit joiningGroupVoiceCall(QString::fromUtf8(newPresence.userActivity().description.c_str()));
                }
                else if (newPresence.userActivity().general == Origin::Chat::DP_LEAVE)
                {
                    ORIGIN_LOG_DEBUG_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoip))
                        << "RemoteUser::leavingGroupVoiceCall: VOIP "
                        << newPresence.userActivity().general
                        << ", "
                        << newPresence.userActivity().specific;

                    emit leavingGroupVoiceCall();
                }
                else
                {
                    presenceChanged(newPresence, oldPresence);
                }
            }
            else
#endif
            presenceChanged(newPresence, oldPresence);
        }
    }

    void RemoteUser::possibleSubscriptionStateChange(const SubscriptionState &newSubscriptionState, const SubscriptionState &oldSubscriptionState)
    {
        if (newSubscriptionState != oldSubscriptionState)
        {
            subscriptionStateChanged(newSubscriptionState, oldSubscriptionState);
        }
    }
    
    void RemoteUser::setRosterStateFromJingle(const XmppRosterContactProxy& user, const QString &nickname, const QString &originId, bool legacyUsr)
    {
        // Determine our subscription state
        SubscriptionState newSubscriptionState;

        switch (user.subscriptionState)
        {
        case buzz::XMPP_SUBSCRIPTION_NONE:
            newSubscriptionState = SubscriptionState(SubscriptionState::DirectionNone, false, false);
            break;
        case buzz::XMPP_SUBSCRIPTION_NONE_ASKED:
            newSubscriptionState = SubscriptionState(SubscriptionState::DirectionNone, true, false);
            break;
        case buzz::XMPP_SUBSCRIPTION_FROM:
            newSubscriptionState = SubscriptionState(SubscriptionState::DirectionFrom, false, false);
            break;
        case buzz::XMPP_SUBSCRIPTION_FROM_ASKED:
            newSubscriptionState = SubscriptionState(SubscriptionState::DirectionFrom, true, false);
            break;
        case buzz::XMPP_SUBSCRIPTION_TO:
            newSubscriptionState = SubscriptionState(SubscriptionState::DirectionTo, false, false);
            break;
        case buzz::XMPP_SUBSCRIPTION_BOTH:
            newSubscriptionState = SubscriptionState(SubscriptionState::DirectionBoth, false, false);
            break;
        }

        mStateLock.lock();

        mRosterNickname = nickname;
        mOriginId = originId;
        mLegacyUser = legacyUsr;

        const SubscriptionState oldSubscriptionState = mSubscriptionState;

        // Preserve our pending user approval flag
        // This only gets sets through RemoteUser::requestedSubscription(); we don't have this information in the roster
        if (oldSubscriptionState.isPendingCurrentUserApproval() && !newSubscriptionState.isContactSubscribedToCurrentUser())
        {
            newSubscriptionState = newSubscriptionState.withPendingCurrentUser(true);
        }

        mSubscriptionState = newSubscriptionState;

        // If we're subscribed to the contact we can assume we know their presence. Start with an unavailable presence
        // because we won't get explicit presence updates for unavailable contacts
        const Presence oldPresence = mPresence;
        Presence newPresence = oldPresence;

        if (newSubscriptionState.isCurrentUserSubscribedToContact() && oldPresence.isNull())
        {
            // Need to set this as a null presence of Unavailable
            newPresence = Presence();
            mPresence = newPresence;
        }

        // OFM-10965, OFM-11054: If we just accepted a friend invite, make sure the presence is valid
        // so that we don't early exit out of OriginSocialProxy::onPresenceChanged().
        // And thus, preventing a presenceChanged() signal from being emitted.
        if (newSubscriptionState.isCurrentUserSubscribedToContact() && oldSubscriptionState.isPendingCurrentUserApproval())
        {
            mPresence.setNull(false);
        }

        mStateLock.unlock();

        possiblePresenceChange(newPresence, oldPresence);
        possibleSubscriptionStateChange(newSubscriptionState, oldSubscriptionState);
    }
        
    void RemoteUser::setNucleusPersonaId(NucleusID nucleusPersonaId)
    {
        QMutexLocker locker(&mStateLock);
        mNucleusPersonaId = nucleusPersonaId;
    }

    void RemoteUser::setPresenceFromJingle(const XmppPresenceProxy& presence)
    {
        QString newStatus = QString::fromUtf8(presence.status.c_str());
        Presence::Availability newAvailability;
        OriginGameActivity newOriginGameActivity;

        newAvailability = jingleToAvailability(presence.show);
        if (presence.available == XMPP_PRESENCE_UNAVAILABLE)
            newAvailability = Presence::Unavailable;
        
        bool isVoip = presence.activityInstance == Origin::Chat::DP_VOICECHAT;
        
        if (isVoip)
        {
            // don't change/clear game activity if we get a voip presence
            newOriginGameActivity = mPresence.gameActivity();
        }
        else if (presence.activityBody.compare("") != 0)
        {
            //QString body = QString("%1;%2;%3;%4;%5;%6").arg(title)
            //.arg(productId).arg((joinable) ? "JOINABLE" : "INGAME").arg(twitchPresence)
            //.arg(gamePresence).arg(multiplayerId);
            QString activityString = QString::fromUtf8(presence.activityBody.c_str());
            QStringList activityElements = activityString.split(";");

            // Extract the OriginGameActivity from the various related impl accessors
            QString title;
            QString productId;
            QString gamePresence;
            QString multiplayerId;
            QString twitchPresence;
            bool joinable = false;
            bool joinable_invite_only = false;
            
            if (activityElements.count() > 0)
                title = activityElements[0];
            if (activityElements.count() > 1)
                productId = activityElements[1];
            if (activityElements.count() > 2)
            {
                joinable_invite_only = activityElements[2].compare("JOINABLE_INVITE_ONLY") == 0;
                joinable = activityElements[2].compare("JOINABLE") == 0;
            }
            if (activityElements.count() > 3)
                twitchPresence = activityElements[3];
            if (activityElements.count() > 4)
                gamePresence = activityElements[4];
            if (activityElements.count() > 5)
                multiplayerId = activityElements[5];
            
            ORIGIN_LOG_ERROR_IF(activityElements.count() < 6) << "Presence missing elements: " << activityElements.join(";");
            
            OriginGameActivity activity(false, productId, title, joinable, joinable_invite_only, gamePresence, multiplayerId);

            if (twitchPresence.contains("http"))
            {
                newOriginGameActivity = activity.withBroadcastUrl(twitchPresence);
            }
            else
            {
                newOriginGameActivity = activity;
            }            
        }

        // Our impl can't distinguish empty status from unset status
        // Assume empty means unset
        if (newStatus.isEmpty())
        {
            newStatus = QString();
        }

        // Build a Presence instance from all of our bits
        UserActivity userActivity(presence.activityCategory, presence.activityInstance, presence.activityBody);
        const Presence newPresence = Presence(
            newAvailability,
            newStatus,
            newOriginGameActivity,
            userActivity);

        mStateLock.lock();

        const Presence oldPresence = mPresence;
        mPresence = newPresence;

        mStateLock.unlock();

        possiblePresenceChange(newPresence, oldPresence);
    }

    void RemoteUser::refreshCapabilities()
    {
        // Allow the connection to look up our capabilities since we don't want to replicate
        // the storage for every user.
        const UserChatCapabilities caps = connection()->capabilities(*this);

        qint32 oldCaps = mCapabilities;
        mCapabilities = NoCapabilities;
        UserChatCapabilities::const_iterator lastCap(caps.cend());
        for (UserChatCapabilities::const_iterator i(caps.cbegin()); i != lastCap; ++i)
        {
            if (*i == Origin::Chat::CHATCAP_CAPS)
            {
                mCapabilities |= CAPS;
            }
            else if (*i == Origin::Chat::CHATCAP_CHATSTATES)
            {
                mCapabilities |= CHATSTATES;
            }
            else if (*i == Origin::Chat::CHATCAP_DISCOINFO)
            {
                mCapabilities |= DISCOINFO;
            }
            else if (*i == Origin::Chat::CHATCAP_MUC)
            {
                mCapabilities |= MUC;
            }
#if ENABLE_VOICE_CHAT
            else if (*i == Origin::Chat::CHATCAP_VOIP)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( isVoiceEnabled )
                    mCapabilities |= VOIP;
            }
#endif
        }

        if (mCapabilities != oldCaps)
            emit capabilitiesChanged();
    }

    // Sets our capabilities
    void RemoteUser::onCapabilitiesChanged()
    {
        refreshCapabilities();
    }

    void RemoteUser::initializeSettledPresence()
    {
        if (mPresence.isNull())
        {
            // if our old presence is still null we can assume at this point the user is offline
            QMutexLocker stateLocker(&mStateLock);

            const Presence oldPresence = mPresence;
            // Set the presence to a non-null unavailable
            mPresence = Presence(Presence::Unavailable);

            possiblePresenceChange(mPresence, oldPresence);
        }
    }

    // This is a temporary function to aid in loc testing for Groip
    // This should be removed after A Groip View controller is created
    // for dialogs currently in ClientViewController
    void RemoteUser::debugSetupRemoteUser(const QString& name,  qint32 caps)
    {
        mCapabilities = caps;
        mOriginId = name;
    }

    void RemoteUser::requestedSubscription()
    {
        mStateLock.lock();
        
        const SubscriptionState oldSubscriptionState = mSubscriptionState;
        const SubscriptionState newSubscriptionState = oldSubscriptionState.withPendingCurrentUser(true);

        mSubscriptionState = newSubscriptionState;

        mStateLock.unlock();

        possibleSubscriptionStateChange(newSubscriptionState, oldSubscriptionState);
    }

    void RemoteUser::wasAddedToRoster()
    {
        addedToRoster();
    }

    void RemoteUser::wasRemovedFromRoster()
    {
        // We no longer have a subscription or know presence
        mStateLock.lock();
        
        const SubscriptionState oldSubscriptionState = mSubscriptionState;
        const SubscriptionState newSubscriptionState = SubscriptionState(SubscriptionState::DirectionNone, false, false);

        mSubscriptionState = newSubscriptionState;

        const Presence oldPresence = mPresence;
        const Presence newPresence = Presence();

        mPresence = newPresence;

        mStateLock.unlock();

        removedFromRoster();

        possibleSubscriptionStateChange(newSubscriptionState, oldSubscriptionState);
        possiblePresenceChange(newPresence, oldPresence);
    }

    void RemoteUser::unsubscribe()
    {
        if (connection() && connection()->jingle())
        {
            connection()->jingle()->CancelFriendRequest(jabberId().toJingle());
        }
    }

    QString RemoteUser::rosterNickname() const
    {
        QMutexLocker locker(&mStateLock);
        return mRosterNickname;
    }
    
    QString RemoteUser::originId() const
    {
        QMutexLocker locker(&mStateLock);
        // If our Origin Id for this RemoteUser object is empty try to grab the last known name from our cache
        if (mOriginId.isEmpty())
        {
            QString cachedId;
            Engine::Social::SocialController *socialController = Engine::LoginController::currentUser()->socialControllerInstance();
            if (socialController)
                cachedId = socialController->userNames()->namesForNucleusId(nucleusId()).originId();
            // Double check our cached value is valid, if atom is down we will not get a cached list of friends
            if (!cachedId.isNull() && !cachedId.isEmpty())
                mOriginId = cachedId;
        }

        return mOriginId;
    }
        
    NucleusID RemoteUser::nucleusPersonaId() const
    {
        QMutexLocker locker(&mStateLock);
        return mNucleusPersonaId;
    }

    NucleusID RemoteUser::nucleusId() const
    {
        OriginConnection *originConnection = dynamic_cast<OriginConnection*>(connection());

        if (!originConnection)
        {
            // Not an Origin connection
            return InvalidNucleusID;
        }

        return originConnection->jabberIdToNucleusId(jabberId());
    }

    bool RemoteUser::legacyUser() const
    {
        QMutexLocker locker(&mStateLock);
        return mLegacyUser;
    }
        
    void RemoteUser::disconnected()
    {
        Presence unknownPresence;

        // We've been disconnected from the XMPP server; we no longer know our state
        mStateLock.lock();

        const Presence oldPresence = mPresence;
        mPresence = unknownPresence;

        mStateLock.unlock();

        presenceChanged(unknownPresence, oldPresence);
    }

    void RemoteUser::promoteToAdmin(const QString& groupGuid)
    {
        Services::Session::SessionRef session = Services::Session::SessionService::instance()->currentSession();
        Services::GroupUserUpdateRoleResponse* resp = Services::GroupServiceClient::updateGroupUserRole(session, groupGuid, QString::number(nucleusId()), "admin");
        
        if (!resp)
            return;
        
        resp->setDeleteOnFinish(true);

        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onPromoteToAdminFinished()));
    }

    void RemoteUser::transferOwnership(const QString& groupGuid)
    {

        Services::Session::SessionRef session = Services::Session::SessionService::instance()->currentSession();
        Services::GroupUserUpdateRoleResponse* resp = Services::GroupServiceClient::updateGroupUserRole(session, groupGuid, QString::number(nucleusId()), "superuser");
        resp->setDeleteOnFinish(true);

        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onAssignOwnershipFinished()));

    }

    void RemoteUser::demoteToMember(const QString& groupGuid)
    {
        Services::Session::SessionRef session = Services::Session::SessionService::instance()->currentSession();
        Services::GroupUserUpdateRoleResponse* resp = Services::GroupServiceClient::updateGroupUserRole(session, groupGuid, QString::number(nucleusId()), "member");
        
        if (!resp)
            return;
        
        resp->setDeleteOnFinish(true);

        ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onDemoteToMemberFinished()));
    }

    void RemoteUser::onPromoteToAdminFinished()
    {
        Origin::Services::GroupUserUpdateRoleResponse* resp = dynamic_cast<Origin::Services::GroupUserUpdateRoleResponse*>(sender());
        ORIGIN_ASSERT(resp);

        if (resp != NULL)
        {
            if (!resp->failed()) 
            {
                emit(promoteToAdminSuccess(this));
            } 
            else 
            {
                emit(promoteToAdminFailure(this, resp->getGroupResponseError()));
            }
        }
        ORIGIN_VERIFY_DISCONNECT(resp, SIGNAL(finished()), this, SLOT(onPromoteToAdminFinished()));
    }

    void RemoteUser::onAssignOwnershipFinished()
    {
        Origin::Services::GroupUserUpdateRoleResponse* resp = dynamic_cast<Origin::Services::GroupUserUpdateRoleResponse*>(sender());
        ORIGIN_ASSERT(resp);

        if (resp != NULL)
        {
            if (!resp->failed()) 
            {
                // Now demote current user to admin
                QString currentUserId;

                OriginConnection *originConnection = dynamic_cast<OriginConnection*>(connection());
                if (!originConnection)
                {
                    emit(transferOwnershipFailure(resp->getGroupGuid()));
                } else {
                    currentUserId = QString::number(originConnection->jabberIdToNucleusId(connection()->currentUser()->jabberId()) );

                    Services::Session::SessionRef session = Services::Session::SessionService::instance()->currentSession();
                    Services::GroupUserUpdateRoleResponse* demote = Services::GroupServiceClient::updateGroupUserRole(session, resp->getGroupGuid(), currentUserId, "admin");

                    if (!demote)
                        emit(transferOwnershipFailure(resp->getGroupGuid()));

                    demote->setDeleteOnFinish(true);

                    ORIGIN_VERIFY_CONNECT(demote, SIGNAL(finished()), this, SLOT(onTransferOwnershipFinished()));
                }
            } 
            else 
            {
                emit(transferOwnershipFailure(resp->getGroupGuid()));
            }
        }
        ORIGIN_VERIFY_DISCONNECT(resp, SIGNAL(finished()), this, SLOT(onAssignOwnershipFinished()));
    }
    
    void RemoteUser::onTransferOwnershipFinished()
    {
        Origin::Services::GroupUserUpdateRoleResponse* resp = dynamic_cast<Origin::Services::GroupUserUpdateRoleResponse*>(sender());
        ORIGIN_ASSERT(resp);

        emit(transferOwnershipSuccess(resp->getGroupGuid()));

        ORIGIN_VERIFY_DISCONNECT(resp, SIGNAL(finished()), this, SLOT(onTransferOwnershipFinished()));
    }

    void RemoteUser::onDemoteToMemberFinished()
    {
        Origin::Services::GroupUserUpdateRoleResponse* resp = dynamic_cast<Origin::Services::GroupUserUpdateRoleResponse*>(sender());
        ORIGIN_ASSERT(resp);

        if (resp != NULL)
        {
            if (!resp->failed()) 
            {
                emit(demoteToMemberSuccess(this));
            } 
            else 
            {
                emit(demoteToMemberFailure(this));
            }
        }

        ORIGIN_VERIFY_DISCONNECT(resp, SIGNAL(finished()), this, SLOT(onDemoteToMemberFinished()));
    }

    void RemoteUser::setAsInviter( bool isInviter )
    {
        Presence oldPresence = mPresence;
        {
            QMutexLocker locker(&mStateLock);

            OriginGameActivity activity = mPresence.gameActivity();
            activity.setInvited(isInviter);
            mPresence.setGameActivity(activity);
        }

        emit presenceChanged(mPresence, oldPresence);
    }

}
}
