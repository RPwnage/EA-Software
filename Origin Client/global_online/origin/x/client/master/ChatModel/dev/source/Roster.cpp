#if defined(ORIGIN_PC)
#define NOMINMAX
#elif defined(ORIGIN_MAC)
#else
#error Must specialize for other platform.
#endif
// We need to include this before QHash to work around GCC 4.2 bug #29131
#include "JabberID.h"

#include <QMutexLocker>
#include <QDebug>
#include <QMutableHashIterator>

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "Roster.h"
#include "ConnectedUser.h"
#include "Connection.h"
#include "XMPPImplEventAdapter.h"
#include "BlockList.h"

#include "JingleClient.h"

#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Chat
{
    Roster::Roster(Connection *connection, QObject *parent) : 
        QObject(parent),
        mConnection(connection),
        mFirstRosterUpdate(1)
    {
        ORIGIN_VERIFY_CONNECT(
            connection->implEventAdapter(), SIGNAL(rosterUpdate(Origin::Chat::XmppRosterModuleProxy)),
            this, SLOT(rosterUpdate(Origin::Chat::XmppRosterModuleProxy)));
        
        ORIGIN_VERIFY_CONNECT(connection->implEventAdapter(), SIGNAL(addRequest(buzz::Jid)),
            this, SLOT(addRequest(buzz::Jid)));

        ORIGIN_VERIFY_CONNECT(this, SIGNAL(loaded()),
            this, SLOT(onRosterLoaded()));
    }

    QSet<RemoteUser*> Roster::contacts() const
    {
        QMutexLocker locker(&mRosterLock);
        return mRosterSet;
    }

    void Roster::requestSubscription(const JabberID &user, const QString &source)
    {
        buzz::Jid jid = user.toJingle();
        if (mConnection && mConnection->jingle())
        {
            mConnection->jingle()->SendFriendRequest(jid);
        }
    }
    
    void Roster::rosterUpdate(Origin::Chat::XmppRosterModuleProxy roster)
    {
        // Convert the impl roster in to a set of up-to-date RemoteUser instances
        QSet<RemoteUser*> seenContacts;
        int missingNames = 0;
        for (int i = 0; i < roster.rosterContacts.size(); i++)
        {
            const Origin::Chat::XmppRosterContactProxy& contact = roster.rosterContacts.at(i);

            // Fetch or create the appropriate RemoteUser instance
            JabberID contactJid = JabberID::fromJingle(contact.jid).asBare();
            RemoteUser* user = mConnection->remoteUserForJabberId(contactJid);

            // TODO: [Hans, Kevin] Figure out what is going on here... as below doesn't make a whole lot of sense.

            const QString nickname = QString::fromUtf8(contact.personaId.c_str());
            const QString eaid = QString::fromUtf8(contact.eaid.c_str());
            const bool legacyUser = contact.legacyUser;

            // Try to convert the Nucleus persona ID to a quint64
            bool parsedOk = false;
            const QString nucleusPersonaIdStr = QString::fromUtf8(contact.personaId.c_str());
            NucleusID nucleusPersonaId = nucleusPersonaIdStr.toULongLong(&parsedOk);

            if (parsedOk)
            {
                user->setNucleusPersonaId(nucleusPersonaId);
            }
            else
            {
                user->setNucleusPersonaId(-1);
            }

            // If we have any non-valid eaid's we need to ask for the roster again
            // This is part of the attempt to fix/limit this bug:
            // https://developer.origin.com/support/browse/EBIBUGS-27209
            // GOS will remove the name element from the xml if it's null and we should see ""
            if (eaid == "")
            {
                missingNames++;
                ORIGIN_LOG_DEBUG << "Failed to find eaid for contact: "<< nucleusPersonaIdStr;
            }

            user->setRosterStateFromJingle(contact, nickname, eaid, legacyUser);

            // We've seen this
            seenContacts << user;
        }

        if (missingNames)
            GetTelemetryInterface()->Metric_FRIEND_USERNAME_MISSING(missingNames);

        QSet<RemoteUser*> removedContacts;
        QSet<RemoteUser*> addedContacts;

        // Update our actual roster under its lock
        {
            QMutexLocker locker(&mRosterLock);

            removedContacts = mRosterSet - seenContacts;
            addedContacts = seenContacts - mRosterSet;

            mRosterSet = seenContacts;
        }
        
        // Emit all our signals outside of our lock now that our rosteris in a consistent state
        foreach(RemoteUser *removed, removedContacts)
        {
            removed->wasRemovedFromRoster();
            emit contactRemoved(removed);

            ORIGIN_VERIFY_DISCONNECT(removed, SIGNAL(anyChange()), this, SIGNAL(anyUserChange()));
        }
        
        foreach(RemoteUser *added, addedContacts)
        {
            added->wasAddedToRoster();
            emit contactAdded(added);

            ORIGIN_VERIFY_CONNECT(added, SIGNAL(anyChange()), this, SIGNAL(anyUserChange()));
        }

        // Make sure we only emit loaded once
        if (mFirstRosterUpdate.testAndSetRelaxed(1, 0))
        {
            emit loaded();
        }
            
        emit updated();
    }
        
    void Roster::addRequest(const buzz::Jid& from)
    {
        const JabberID jid = JabberID::fromJingle(from);
        bool needRosterRequest = false;

        RemoteUser* user = mConnection->remoteUserForJabberId(jid);

        {
            QMutexLocker locker(&mRosterLock);
            needRosterRequest = !mRosterSet.contains(user);
        }

        if (needRosterRequest)
        {
            // Subscription requests don't have the Origin ID attached
            // Refresh the roster to get the Origin ID and build a RemoteUser
            if (mConnection && mConnection->jingle())
            {
                mConnection->jingle()->RequestRoster();
            }
        }

        emit subscriptionRequested(JabberID::fromJingle(from));
    }
        
    void Roster::approveSubscriptionRequest(const JabberID &jid)
    {
        // Origin server seems to ignore the name but XMPPClient requires it
        if (mConnection && mConnection->jingle())
        {
            mConnection->jingle()->AcceptFriendRequest(jid.toJingle());
        }
    }

    void Roster::denySubscriptionRequest(const JabberID &jid)
    {
        if (mConnection && mConnection->jingle())
        {
            mConnection->jingle()->RejectFriendRequest(jid.toJingle());
        }
    }

    void Roster::cancelSubscriptionRequest(const JabberID &jid)
    {
        if (mConnection && mConnection->jingle())
        {
            mConnection->jingle()->CancelFriendRequest(jid.toJingle());
        }
    }

    void Roster::removeContact(const RemoteUser *user)
    {
        if ((!mConnection) || (!mConnection->currentUser()))
            return;

        BlockList *blockList = mConnection->currentUser()->blockList();

        const JabberID jid = user->jabberId();
        bool wasBlocked = false;
        
        if (blockList)
        {
            wasBlocked = blockList->jabberIDs().contains(jid);
        }

        if (wasBlocked)
        {
            // Temporarily unblock them so they know we're unsubscribing
            if (blockList)
            {
                blockList->removeJabberID(jid);
            }
        }

        if (mConnection && mConnection->jingle())
        {
            mConnection->jingle()->RemoveFriend(user->jabberId().toJingle());
        }

        if (wasBlocked)
        {
            blockList->addJabberID(jid);
        }
    }

    bool Roster::hasFullySubscribedContacts()
    {
        bool hasFriends = false;
        if (mRosterSet.size() > 0)
        {
            for(RosterSet::iterator i = mRosterSet.begin(); i != mRosterSet.end(); ++i)
            {
                RemoteUser* user = *i;
                if (user->subscriptionState().direction() == SubscriptionState::DirectionBoth)
                {
                    hasFriends = true;
                    break;
                }
            }
        }
        return hasFriends;
    }


    void Roster::onRosterLoaded()
    {
        // In 10000ms consider the connection "settled"
        QTimer::singleShot(10000, this, SLOT(onChatSettled()));        
    }

    void Roster::onChatSettled()
    {
        // Now that we are "settled" loop through all fully-subscribed friends and make
        // sure that everyone has a valid non-null presence.
        foreach(RemoteUser *contact, contacts())
        {
            if (contact->subscriptionState().direction() == Origin::Chat::SubscriptionState::DirectionBoth) 
            {
                contact->initializeSettledPresence();
            }
        }
    }

    void Roster::adjustUserPresenceOnInvite( const Origin::Chat::JabberID &user)
    {
         foreach(RemoteUser * remoteUser, contacts())
         {
             // For some reason the 
             if(remoteUser->jabberId().asBare() == user.asBare())
             {
                remoteUser->setAsInviter(true);
                break;
             }
         }
    }
}
}
