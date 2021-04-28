#ifndef _CHATMODEL_XMPPUSER_H
#define _CHATMODEL_XMPPUSER_H

#include <QObject>

#include "Conversable.h"

#include "Presence.h"
#include "JabberID.h"

namespace Origin
{
namespace Chat
{
    class Connection;

    ///
    /// Abstract XMPP user
    ///
    /// XMPPUser captures common functionality between Chat::ConnectedUser and Chat::RemoteUser. This is related
    /// to querying the users status, presence and being notified when they change
    ///
    class XMPPUser : public Conversable
    {
        Q_OBJECT

    public:
        ///
        /// Returns the user's presence
        ///
        /// This will be null if the user's presence cannot be determined
        ///
        virtual Presence presence() const = 0;
    
    signals:
        ///
        /// Emitted when the user's presence changes
        ///
        /// \param newPresence       User's new presence
        /// \param previousPresence  User's previous presence
        ///
        void presenceChanged(const Origin::Chat::Presence &newPresence, const Origin::Chat::Presence &previousPresence);

#if ENABLE_VOICE_CHAT
        void incomingVoiceCall(const QString& incomingVoiceChannel);
        void leavingVoiceCall();
        void joiningGroupVoiceCall(const QString& description);
        void leavingGroupVoiceCall();
#endif

    protected:
        ///
        /// Creates a new XMPP user
        ///
        /// \sa Conversable::Conversable
        ///
        XMPPUser(Connection *connection, QObject *parent = NULL) : 
            Conversable(connection, "chat", parent)
        {
        }
    };
}
}

#endif
