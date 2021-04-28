#ifndef _CHATMODEL_ROSTER_H
#define _CHATMODEL_ROSTER_H

// We need to include this before QHash to work around GCC bug #29131
#include "JabberID.h"

#include <QObject>
#include <QSet>
#include <QMutex>
#include <QAtomicInt>
#include <QAtomicPointer>
#include "RemoteUser.h"
#include "XMPPImplTypes.h"

namespace Origin
{
namespace Chat
{
    class Connection;
    struct XmppRosterModuleProxy;

    ///
    /// Represents a Chat::ConnectedUser's XMPP roster
    ///
    /// An XMPP roster is a server stored list of all of a user's contacts. This class mirrors and modifies the server
    /// state by providing a container for Chat::RemoteUser instances and functions for managing contact subscriptions
    ///
    class Roster : public QObject
    {
        Q_OBJECT

        friend class ConnectedUser;
    public:

        typedef QSet<RemoteUser*> RosterSet;

        ///
        /// Returns if this roster has been loaded from the server
        ///
        /// loaded() will be emitted once the initial load completes
        ///
        bool hasLoaded() const
        {
            return mFirstRosterUpdate.load() == 0;
        }

        ///
        /// Returns the set of remote users that are on the roster
        ///
        RosterSet contacts() const;

        ///
        /// Requests a presence subscription to the given Jabber ID
        ///
        /// Once completed a Chat::RemoteUser is added to the roster with a pending Chat::SubscriptionState
        ///
        /// \param user    User to request subscription to
        /// \param source  Optional Origin-specific string used for tracking the source of the subscription
        ///
        /// \sa cancelSubscriptionRequest()
        ///
        void requestSubscription(const JabberID &user, const QString &source = QString());

        ///
        /// Removes a contact from the roster and cancels any subscriptions with the contact
        ///
        void removeContact(const RemoteUser *);

        ///
        /// Determines if the roster has any fully subscribed users
        /// (Effectively we're asking, "Do we have any friends?"
        ///
        bool hasFullySubscribedContacts();
        
    public slots:
        ///
        /// Approves a presence subscription request from another user
        ///
        /// \sa denySubscriptionRequest()
        /// \sa subscriptionRequested()
        ///
        void approveSubscriptionRequest(const Chat::JabberID &jid);

        ///
        /// Denies a presence subscription request from another user
        ///
        /// \sa approveSubscriptionRequest()
        /// \sa subscriptionRequested()
        ///
        void denySubscriptionRequest(const Chat::JabberID &jid);

        ///
        /// Cancels a subscription request sent to another user
        ///
        /// \sa requestSubscription()
        ///
        void cancelSubscriptionRequest(const Chat::JabberID &jid);


        ///
        /// For invite only joinable the visible state will go from ingame to joinable.
        ///
        void adjustUserPresenceOnInvite(const Origin::Chat::JabberID &);

    signals:
        ///
        /// Emitted once the roster is loaded from the server the first time
        ///
        void loaded();

        ///
        /// Emitted once the roster is updated
        ///
        void updated();

        ///
        /// Emitted after a Chat::RemoteUser is added to the roster
        ///
        /// This is emitted for both the initial roster load and for roster updates
        ///
        /// \sa contactRemoved()
        ///
        void contactAdded(Origin::Chat::RemoteUser*);

        ///
        /// Emitted aftter a Chat::RemoteUser is removed from the roster
        ///
        /// \sa contactAdded()
        ///
        void contactRemoved(Origin::Chat::RemoteUser*);

        ///
        /// Emitted when a user requests presence subscription to the connected user
        ///
        /// The server will periodically re-request this until the user responds. The inteval is server defined.
        ///
        /// \sa approveSubscriptionRequest()
        /// \sa denySubscriptionRequest()
        ///
        void subscriptionRequested(const Origin::Chat::JabberID &);

        void anyUserChange();

    private: // Friend functions
        Roster(Connection *connection, QObject *parent = NULL);

    private slots:
        void rosterUpdate(Origin::Chat::XmppRosterModuleProxy roster);
        void addRequest(const buzz::Jid& from);
        void onRosterLoaded();
        void onChatSettled();

    private:
        Connection *mConnection;

        QAtomicInt mFirstRosterUpdate;

        RosterSet mRosterSet;
        mutable QMutex mRosterLock;
    };
}
}

#endif
