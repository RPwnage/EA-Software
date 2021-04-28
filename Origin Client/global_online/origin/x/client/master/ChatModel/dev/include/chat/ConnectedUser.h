#ifndef _CHATMODEL_CONNECTEDUSER_H
#define _CHATMODEL_CONNECTEDUSER_H

#include <QObject>
#include <QMutex>
#include <QPixmap>

#include "XMPPUser.h"

namespace Origin
{
namespace Chat
{
    class Connection;
    class Roster;
    class ChatGroups;
    class BlockList;

    /// 
    /// Represents the connected XMPP user
    ///
    /// This class has a few major functions:
    /// - Discovering the state of the current user using functions reimplemented from Chat::XMPPUser
    /// - Setting the user's XMPP presence and status
    /// - Accessing the user's Chat::BlockList and Chat::Roster
    ///
    class ConnectedUser : public XMPPUser
    {
        Q_OBJECT

        friend class Connection;
    public:
        ///
        /// Represents the connect user's presence visibility
        ///
        enum Visibility
        {
            ///
            /// Connected user's actual presence is visible to all other users
            ///
            Visible,

            ///
            /// Connected user appears unavailable to all other users
            ///
            Invisible
        };

        ///
        /// Returns the currently requested presence for the user
        ///
        /// This can differ from presence() while a presence change operation is in progress or during server
        /// disconnections
        ///
        Presence requestedPresence() const;
        
        Presence presence() const;

        ///
        /// Asynchronously sets the presence for the connected user
        ///
        /// \param  presence  Presence to set. If this presence is null or has an availability of Presence::Unavailable
        ///                   this function will return NULL without performing any action. requestVisibility() can be
        ///                   used to appear unavailable instead.
		/// \return True if presence was successfully requested, false otherwise
        ///
        bool requestPresence(const Presence &presence);
        
        ///
        /// Returns the current requested visibility for the user
        ///
        /// This can differ from visibility() while a visibility change is in progress or during server disconnections
        ///
        Visibility requestedVisibility() const;

        ///
        /// Returns the current user's visibility
        ///
        Visibility visibility() const;

        ///
        /// Asynchronously sets the visibility for the connected user
		///
		/// \return True if presence was successfully requested, false otherwise
        ///
        bool requestVisibility(Visibility);

        // Conversable implementation
        JabberID jabberId() const
        {
            return mJabberId;
        }

        QSet<ConversableParticipant> participants() const;

        ///
        /// Returns the user's roster
        ///
        Roster *roster()
        {
            return mRoster;
        }
        
        ///
        /// Returns the user's roster
        ///
        const Roster *roster() const
        {
            return mRoster;
        }

        ///
        /// Returns the user's chat groups
        ///
        ChatGroups *chatGroups()
        {
            return mChatGroups;
        }

        ///
        /// Returns the user's chat groups
        ///
        const ChatGroups *chatGroups() const
        {
            return mChatGroups;
        }

        /// 
        /// Returns the user's block list
        ///
        BlockList *blockList();

        /// 
        /// Checks if the user is logged in on a standalone browser
        ///
        void checkRemotePresence(RemoteUser * remoteUser);

    signals:
        ///
        /// Emitted when the user's visibility changes
        ///
        void visibilityChanged(Origin::Chat::ConnectedUser::Visibility);


        ///
        /// Emitted when the user's visibility changes to invisible
        ///
        void userIsInvisible();

        ///
        /// Emitted when an account from the browser has connected/disconnected
        ///
        void remoteStateChanged(bool);

    protected:
        ///
        /// Creates a new ConnectedUser instance
        ///
        /// \param connection  Chat::Connection the user is connected on
        ///
        explicit ConnectedUser(Connection *connection);
        virtual ~ConnectedUser();
        
        ///
        /// Sets the Jabber ID of the connected user
        ///
        void setJabberId(const JabberID &);

    private slots:
        void setActualPresence(const Origin::Chat::Presence &newState);
        void setActualVisibility(Origin::Chat::ConnectedUser::Visibility);

        void disconnected();
        void applyConnectionState();
        void onPrivacyReceived();

    private:
        JabberID mJabberId;

        bool performPresenceChangeOperation();
        bool performVisibilityChangeOperation();

        // Protects all of our mutable state
        mutable QMutex mStateLock;

        Presence mWantedPresence;
        Presence mActualPresence;

        Visibility mWantedVisibility;
        Visibility mActualVisibility;

        Roster *mRoster;

        ChatGroups *mChatGroups;

        BlockList *mBlockList;
    };
}
}

#endif
