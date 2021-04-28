#include <QMutexLocker> 
#include <QTimer>

#include "services/debug/DebugService.h"

#include "XMPPImplAvailabilityConversion.h"
#include "ConnectedUser.h"
#include "Connection.h"
#include "Roster.h"
#include "ChatGroup.h"
#include "BlockList.h"
#include "TelemetryAPIDLL.h"

namespace
{
    // We depend on these being pre-defined server side
    const char8_t *InvisiblePrivacyList = "invisible";
    const char8_t *VisiblePrivacyList = "visible";
}

namespace Origin
{
namespace Chat
{
    ConnectedUser::ConnectedUser(Connection *connection) :
        XMPPUser(connection),
        mWantedVisibility(Visible),
        mActualVisibility(Visible),
        mRoster(new Roster(connection, this)),
        mBlockList(NULL)
    {
        mBlockList = new BlockList(connection);
        mChatGroups = new ChatGroups(connection);

        ORIGIN_VERIFY_CONNECT(
                connection, SIGNAL(disconnected(Origin::Chat::Connection::DisconnectReason)),
                this, SLOT(disconnected()));

        // ORIGIN X - ORIGIN_VERIFY_CONNECT(connection, SIGNAL(connected()), this, SLOT(applyConnectionState()));
    }

    void ConnectedUser::setJabberId(const JabberID &jabberId)
    {
        mJabberId = jabberId;
    }

    ConnectedUser::~ConnectedUser()
    {
        delete mRoster;
        delete mBlockList;
        delete mChatGroups;
    }
        
    QSet<ConversableParticipant> ConnectedUser::participants() const
    {
        // The connected user is never considered to be a participant
        return QSet<ConversableParticipant>();
    }
    
    Presence ConnectedUser::requestedPresence() const
    {
        QMutexLocker locker(&mStateLock);
        return mWantedPresence;
    }
        
    Presence ConnectedUser::presence() const
    {
        QMutexLocker locker(&mStateLock);
        return mActualPresence;
    }
    
    bool ConnectedUser::requestPresence(const Presence &presence)
    {
        QMutexLocker locker(&mStateLock);

        if (presence.isNull() || (presence.availability() == Presence::Unavailable) || (mWantedPresence == presence))
        {
            return NULL;
        }

        mWantedPresence = presence;
        return performPresenceChangeOperation();
    }
        
    // Should be called inside of mStateLock
    void ConnectedUser::setActualPresence(const Presence &newPresence)
    {
        bool availabilityChanged = mActualPresence.availability() != newPresence.availability();
        bool exitingAGame = (mActualPresence.gameActivity().isNull() == false) && (newPresence.gameActivity().isNull());
        bool inGamePresenceChanged = mActualPresence.gameActivity().isNull() ^ newPresence.gameActivity().isNull();

        // send telemetry
        // Temporarily commenting out presense telemetry
        // This hook is generating unintentional duplicate data when a user repeatedly changes from
        // Online or Away to Invisible.
        // EBIBUGS-28986 must be fixed before re-enabling this hook
        /*
        if((mWantedVisibility == Invisible) && ((inGamePresenceChanged == false) || (exitingAGame)))
            GetTelemetryInterface()->Metric_USER_PRESENCE_SET("Invisible");
        else if((newPresence.availability() == Presence::Online) && (newPresence.gameActivity().isNull())
            && (availabilityChanged || exitingAGame))
            GetTelemetryInterface()->Metric_USER_PRESENCE_SET("Online");
        else if((newPresence.availability() == Presence::Away) && availabilityChanged)
            GetTelemetryInterface()->Metric_USER_PRESENCE_SET("Away");
        else if(newPresence.gameActivity().isNull() == false)
            GetTelemetryInterface()->Metric_USER_PRESENCE_SET("InGame");
        */

        // Update our state
        const Presence oldPresence = mActualPresence;
        mActualPresence = newPresence;

        // Emit a signal outside our lock
        if (oldPresence != newPresence)
        {
            mStateLock.unlock();
            emit presenceChanged(newPresence, oldPresence);
            mStateLock.lock();
        }
    }
        
    // Should be called with mStateMutex held
    bool ConnectedUser::performPresenceChangeOperation()
    {
        if (mWantedPresence.isNull())
        {
            // Presence hasn't been set yet
            return false;
        }

        if (!connection())
        {
            return false;
        }

        if (!connection()->established())
        {
            // Our impl isn't running
            return false;
        }

        // Convert to our impl's presence
        buzz::XmppPresenceShow jingleAvailability = availabilityToJingle(mWantedPresence.availability());

        //do the check right before referencing since there's still a chance that jingle() can get deleted from underneath us....
        if (!connection()->jingle())
        {
            return false;
        }

        if (!mWantedPresence.gameActivity().isNull())
        {
            connection()->jingle()->SetPresence(
                jingleAvailability,
                buzz::XMPP_PRESENCE_AVAILABLE,
                &mWantedPresence.statusText(),
                &mWantedPresence.gameActivity().gameTitle(),
                &mWantedPresence.gameActivity().productId(),
                &mWantedPresence.gameActivity().multiplayerId(),
                &mWantedPresence.gameActivity().gamePresence(),
                mWantedPresence.gameActivity().joinable(),
                mWantedPresence.gameActivity().joinable_invite_only(),
                &mWantedPresence.gameActivity().broadcastUrl());
        }
        else
        {
            if (!connection()->jingle()->SetPresence(
                    jingleAvailability,
                    buzz::XMPP_PRESENCE_AVAILABLE, 
                    &mWantedPresence.statusText()))
            {
                return false;
            }
        }
        
        setActualPresence(mWantedPresence);
        return true;
    }
    
    ConnectedUser::Visibility ConnectedUser::requestedVisibility() const
    {
        QMutexLocker locker(&mStateLock);
        return mWantedVisibility;
    }
        
    ConnectedUser::Visibility ConnectedUser::visibility() const
    {
        QMutexLocker locker(&mStateLock);
        return mActualVisibility;
    }
    
    bool ConnectedUser::requestVisibility(Visibility visibility)
    {
        QMutexLocker locker(&mStateLock);

        if (mWantedVisibility == visibility)
        {
            return true;
        }

        mWantedVisibility = visibility;
        return performVisibilityChangeOperation();
    }
        
    // Should be called inside of mStateLock
    void ConnectedUser::setActualVisibility(const Visibility newVisibility)
    {
        Visibility oldVisibility = mActualVisibility;
    
        // Update our actual visibility first
        mActualVisibility = newVisibility;

        if (oldVisibility != newVisibility)
        {
            mStateLock.unlock();
            emit visibilityChanged(newVisibility);

            if (newVisibility == Invisible)
            {
                emit userIsInvisible();
            }
            mStateLock.lock();
        }
    }
    
    // Should be called with mStateMutex held
    bool ConnectedUser::performVisibilityChangeOperation()
    {
        if (!connection() || !connection()->established() || !connection()->jingle())
        {
            // Our impl isn't running
            return false;
        }

        if (mWantedVisibility == Visible)
        {
            // Set our privacy list to the visible one
            connection()->jingle()->SetActivePrivacyList(VisiblePrivacyList);
            // Re-assert our actual presence if its set
            performPresenceChangeOperation();

            setActualVisibility(mWantedVisibility);
        }
        else
        {
            // If we send Unavailable as our initial presence the server may mark us offline and stop sending us
            // presence. Be careful to only set it if we're not already unavailable
            if (mActualPresence.availability() != Presence::Unavailable)
            {
                // Send out unavailable presence so everyone thinks we're offline
                // Don't set mActualPresence here because this is hopefully a transient state. Sending out signals etc.
                // could confuse observers
                buzz::XmppPresenceShow jingleAvailability = availabilityToJingle(mWantedPresence.availability());
                connection()->jingle()->SetPresence(jingleAvailability, buzz::XMPP_PRESENCE_UNAVAILABLE);
            }

            // Set our privacy list to the invisible one
            connection()->jingle()->SetActivePrivacyList(InvisiblePrivacyList);
            // If we are invisible need to wait for the response before sending our presence
            ORIGIN_VERIFY_CONNECT(connection(), SIGNAL(privacyReceived()), this, SLOT(onPrivacyReceived()));
        }
        return true;
    }

    void ConnectedUser::disconnected()
    {
        // We're disconnected now
        Presence unavailablePresence(Presence::Unavailable);

        {
            QMutexLocker locker(&mStateLock);    

            // Preserve our status test
            unavailablePresence.setStatusText(mActualPresence.statusText());

            setActualPresence(unavailablePresence);
            setActualVisibility(Visible);
        }
    }
    
    void ConnectedUser::applyConnectionState()
    {
        // Reassert our state
        QMutexLocker locker(&mStateLock);

        // Make sure we have been given presence information to re-assert
        if (mWantedVisibility == Invisible)
        {
            // Going invisible requires re-asserting our presence (see XEP-0126 section 23)
            performVisibilityChangeOperation();
        }
        else
        {
            // Assert just our presence
            performPresenceChangeOperation();
        }
    }
    
    BlockList *ConnectedUser::blockList()
    {
        return mBlockList;
    }

    void ConnectedUser::onPrivacyReceived()
    {
        // Reassert our state
        QMutexLocker locker(&mStateLock);

        // Re-assert our actual presence if its set
        performPresenceChangeOperation();

        setActualVisibility(mWantedVisibility);
    }
    void ConnectedUser::checkRemotePresence(RemoteUser * remoteUser)
    {
        if(remoteUser->jabberId().asBare() == jabberId().asBare())
        {
            emit remoteStateChanged(true);
        }
    }
}
}
