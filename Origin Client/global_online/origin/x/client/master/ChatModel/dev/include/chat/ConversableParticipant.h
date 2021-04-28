#ifndef _CHATMODEL_CONVERSABLEPARTICIPANT_H
#define _CHATMODEL_CONVERSABLEPARTICIPANT_H

#include <QString>
#include "Presence.h"

namespace Origin
{
namespace Chat
{
    class RemoteUser;
    class MucRoom;

    ///
    /// Represents a remote user participating in a conversable
    ///
    class ConversableParticipant
    {
        friend class RemoteUser;
        friend class MucRoom;
    public:
        ///
        /// Creates an empty ConversableParticipant. This is here so that we can register this as a metatype
        /// and should not be used.
        ///
        ConversableParticipant() :
            mNickname(""),
            mRemoteUser(NULL)
        {
        }

        void setRoomPresence(const XmppPresenceProxy& presence);

        Presence getRoomPresence() const
        {
            return mPresence;
        }

        ///
        /// Returns the nickname for the user in the context of the conversable
        ///
        /// For one-to-one chats this will be the same as RemoteUser::rosterNickname(). In multi-user chats this will 
        /// be the user's room-specific nickname
        ///
        QString nickname() const
        {
            return mNickname;
        }

        ///
        /// Returns the RemoteUser instance of the participant
        ///
        /// One RemoteUser instance is shared between all conversables the user may be participating in
        ///
        RemoteUser *remoteUser() const
        {
            return mRemoteUser;
        }

        ///
        /// Compares two participants for equality
        ///
        /// Equality is entirely determined by the participant's full Jabber ID
        ///
        bool operator==(const ConversableParticipant &other) const;

        ///
        /// Compares two participant for inequality
        ///
        bool operator!=(const ConversableParticipant &other) const
        {
            return !(*this == other);
        }

    protected:
        ///
        /// Creates a new ConversableParticipant with the passed fields
        ///
        ConversableParticipant(const QString &nickname, RemoteUser *remoteUser) :
            mNickname(nickname),
            mRemoteUser(remoteUser)
        {
        }

    private:
        QString mNickname;
        RemoteUser *mRemoteUser;
        Presence mPresence;
    };
    
    uint qHash(const Origin::Chat::ConversableParticipant &participant);
}
}

#endif

