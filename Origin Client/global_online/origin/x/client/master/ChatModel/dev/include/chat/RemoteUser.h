#ifndef _CHATMODEL_REMOTEUSER_H
#define _CHATMODEL_REMOTEUSER_H

#include <QObject>
#include <QMutex>
#include <QPixmap>

#include "XMPPImplTypes.h"
#include "XMPPUser.h"
#include "Message.h"
#include "SubscriptionState.h"
#include "NucleusID.h"

#include "services/rest/GroupServiceResponse.h"


namespace Origin
{
namespace Chat
{
    class Connection;
    struct XmppPresenceProxy;
    struct XmppRosterModuleProxy;
    struct XmppRosterContactProxy;

    ///
    /// Represents a remote XMPP user
    ///
    /// Remote users may or may not exist on the user's roster. RemoteUser instances are created on demand by the
    /// chat model when new Jabber IDs are encountered over on the connected user's roster or from incoming messages.
    ///
    class RemoteUser : public XMPPUser
    {
        Q_OBJECT 

        // Connection creates us
        friend class Connection;
        // Roster can update our state
        friend class Roster;
    public:
        // Conversable implementation
        JabberID jabberId() const
        {
            return mJabberId;
        }
        
        QSet<ConversableParticipant> participants() const;

        ///
        /// Returns the roster nickname for this user if available; otherwise a null QString is returned
        ///
        QString rosterNickname() const;

        ///
        /// Returns the Origin ID for the user if available; otherwise a null QString is returned
        ///
        QString originId() const;

        ///
        /// Returns the Nucleus ID for the user if available; otherwise InvalidNucleusID
        ///
        NucleusID nucleusId() const;

        ///
        /// Indicates whether the user is in the legacy friends roster.
        ///
        bool legacyUser() const;
        
        ///
        /// Returns the Nucleus persona ID for the contact if available; otherwise InvalidNucleusID 
        ///
        NucleusID nucleusPersonaId() const;

        ///
        /// Sets the Nucleus persona ID for the contact
        ///
        void setNucleusPersonaId(NucleusID nucleusPersonaId);

        Presence presence() const;

        ///
        /// Returns the state of the user's presence subscription 
        ///
        SubscriptionState subscriptionState() const;
        
        ///
        /// Unsubscribes from the user's presence
        ///
        void unsubscribe();

        ///
        /// Returns the XEP-0115 chat capabilities for this user
        /// in the form of a comma-separated string (e.g. "MUC,VOIP"
        ///
        QString capabilities() const;

        ///
        /// This function notifies the user that their XEP-0115 capabilities have been set.
        ///
        void onCapabilitiesChanged();

        ///
        /// This ensures that this user has a non-null presence set.
        ///
        void initializeSettledPresence();

        ///
        /// This should never be used
        /// This is a temporary function to aid in loc testing for Groip
        /// This should be removed after A Groip View controller is created
        /// for dialogs currently in ClientViewController
        ///
        void debugSetupRemoteUser(const QString& name, qint32 caps);

    signals:
        ///
        /// Emitted when this user's presence subscription state changes
        ///
        /// \param newSubscriptionState       Contact's new subscription state
        /// \param previousSubscriptionState  Contact's previous subscription state
        ///
        void subscriptionStateChanged(Origin::Chat::SubscriptionState newSubscriptionState, Origin::Chat::SubscriptionState previousSubscriptionState);

        ///
        /// Emitted when this remote user has been added to the roster
        ///
        /// \sa Roster::contactAdded()
        ///
        void addedToRoster();
        
        ///
        /// Emitted when this remote user has been removed from the roster
        ///
        /// \sa Roster::contactRemoved()
        ///
        void removedFromRoster();

        void anyChange();

        /// Emitted when the capabilities of this user have changed.
        void capabilitiesChanged();

        void transferOwnershipSuccess(const QString&);
        void transferOwnershipFailure(const QString&);
        void promoteToAdminSuccess(QObject*);
        void promoteToAdminFailure(QObject*, Services::GroupResponse::GroupResponseError);
        void demoteToMemberSuccess(QObject*);
        void demoteToMemberFailure(QObject*);

    public slots:
        void promoteToAdmin(const QString& groupGuid);
        void transferOwnership(const QString& groupGuid);
        void demoteToMember(const QString& groupGuid);

    private slots:
        void disconnected();
        void onAssignOwnershipFinished();
        void onTransferOwnershipFinished();
        void onPromoteToAdminFinished();
        void onDemoteToMemberFinished();

    private: // Friend functions
        RemoteUser(const JabberID &jabberId, Connection *connection, QObject *parent);

        // Sets our metadata and subscription state
        void setRosterStateFromJingle(const XmppRosterContactProxy&, const QString& nickname, const QString& originId, bool legacyUser);
        // Sets our presence
        void setPresenceFromJingle(const XmppPresenceProxy& presence);

        // Called if the remote user requested a subscription
        void requestedSubscription();

        void wasAddedToRoster();
        void wasRemovedFromRoster();

        /// Updates the XEP-0115 chat capabilities from the capabilities cache.
        void refreshCapabilities();

    private:
        void possiblePresenceChange(const Presence &newPresence, const Presence &oldPresence);
        void possibleSubscriptionStateChange(const SubscriptionState &newSubscriptionState, const SubscriptionState &oldSubscriptionState);
        void setAsInviter( bool isInviter );
        JabberID mJabberId;

        // Protects all of our mutable state
        mutable QMutex mStateLock;
        
        QString mRosterNickname;
        mutable QString mOriginId;
        NucleusID mNucleusPersonaId;

        bool mLegacyUser;

        Presence mPresence;
        SubscriptionState mSubscriptionState;
        qint32 mCapabilities;
    };
}
}

#endif
