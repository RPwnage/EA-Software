#include "BuddyListJsHelper.h"

#include <QHash>
#include <QMetaObject>

#include "services/debug/DebugService.h"
#include "chat/OriginConnection.h"
#include "chat/Roster.h"
#include "chat/RemoteUser.h"
#include "chat/ConnectedUser.h"

namespace Origin
{
    namespace Client
    {

        BuddyListJsHelper::BuddyListJsHelper(Chat::OriginConnection *conn, QObject *parent)
            : QObject(parent)
            , mConn(conn)
        {
            Chat::ConnectedUser *currentUser = conn->currentUser();
            Chat::Roster *roster = currentUser->roster();

            // Subscribe to all existing contacts
            QSet<Chat::RemoteUser*> contacts = roster->contacts();

            for(QSet<Chat::RemoteUser*>::ConstIterator it = contacts.constBegin();
                it != contacts.constEnd();
                it++)
            {
                subscribeToContact(*it);
            }

            // Listen for new contacts
            ORIGIN_VERIFY_CONNECT(roster, SIGNAL(contactAdded(Origin::Chat::RemoteUser*)),
                this, SLOT(subscribeToContact(Origin::Chat::RemoteUser *)));

            // Listen for changes to the connected user
            ORIGIN_VERIFY_CONNECT(currentUser, SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)),
                this, SLOT(onConnectedUserChanged()));
        }

        void BuddyListJsHelper::subscribeToContact(Origin::Chat::RemoteUser *contact)
        {
            ORIGIN_VERIFY_CONNECT(contact, SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)),
                this, SLOT(onContactChanged()));
        }

        void BuddyListJsHelper::onContactChanged()
        {
            Chat::RemoteUser *contact = dynamic_cast<Chat::RemoteUser*>(sender());

            if (contact)
            {
                emit presenceChanged(new BuddyPresenceJs(mConn, contact));
            }
        }

        void BuddyListJsHelper::queryFriends()
        {
            // Get a list of all of our buddy's presence
            // Do this asynchronously to match legacy social madness
            QMetaObject::invokeMethod(this, "queryFriendsAsync", Qt::QueuedConnection);
        }

        void BuddyListJsHelper::queryFriendsAsync()
        {
            QSet<Chat::RemoteUser*> contacts = mConn->currentUser()->roster()->contacts();

            // Emit a presence changed signal for each contact
            for(QSet<Chat::RemoteUser*>::ConstIterator it = contacts.constBegin();
                it != contacts.constEnd();
                it++)
            {
                emit presenceChanged(new BuddyPresenceJs(mConn, *it));
            }
        }

        void BuddyListJsHelper::getUserPresence()
        {
            // Get our own presence
            // Do this asynchronously to match legacy social madness
            QMetaObject::invokeMethod(this, "getUserPresenceAsync", Qt::QueuedConnection);
        }

        void BuddyListJsHelper::getUserPresenceAsync()
        {
            emit SelfPresenceChanged(MyCurrentPresence()); 
        }

        QObject* BuddyListJsHelper::MyCurrentPresence()
        {
            return new BuddyPresenceJs(mConn, mConn->currentUser());
        }

        void BuddyListJsHelper::onConnectedUserChanged()
        {
            emit SelfPresenceChanged(MyCurrentPresence()); 
        }

        BuddyPresenceJs::BuddyPresenceJs(Chat::OriginConnection *conn, Chat::XMPPUser *user) :
            ScriptOwnedObject(NULL)
        {
            const Chat::Presence &presence = user->presence();

            if (presence.isNull())
            {
                mPresence = QString();
            }
            else if (!presence.gameActivity().isNull())
            {
                // Fold our game activity in to our presence
                mPresence =  presence.gameActivity().joinable() ? "joinable" : "ingame";
            }
            else
            {
                switch(presence.availability())
                {
                case Chat::Presence::Chat:
                case Chat::Presence::Online:
                    mPresence = "online";
                    break;
                case Chat::Presence::Dnd:
                    mPresence = "busy";
                    break;
                case Chat::Presence::Away:
                case Chat::Presence::Xa:
                    mPresence = "away";
                    break;
                case Chat::Presence::Unavailable:
                    mPresence = "offline";
                    break;
                }
            }

            mUserId = QString::number(conn->nucleusIdForUser(user));
            mRichPresence = presence.statusText();
        }

    }
}
