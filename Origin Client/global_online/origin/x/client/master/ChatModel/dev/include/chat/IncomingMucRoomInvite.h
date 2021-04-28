#ifndef _CHATMODEL_INCOMINGMUCROOMINVITE_H
#define _CHATMODEL_INCOMINGMUCROOMINVITE_H

#include <QString>

#include "XMPPImplTypes.h"
#include "JabberID.h"
#include "talk/xmpp/jid.h"

namespace Origin
{
namespace Chat
{
    class Connection;

    ///
    /// Represents a multi-user chat room invitation received from a remote user
    ///
    class IncomingMucRoomInvite
    {
        friend class Connection;
    public:
        ///
        /// Construct a null IncomingMucRoomInvite
        ///
        IncomingMucRoomInvite() : mNull(true)
        {
        }

        ///
        /// Returns true if this instance is null
        ///
        bool isNull() const
        {
            return mNull;
        }

        ///
        /// Returns the room ID the current user is being invited to
        ///
        QString roomId() const
        {
            return mRoomId;
        }

        ///
        /// Returns the Jabber ID of the user inviting the current user to enter the room
        ///
        JabberID inviter() const
        {
            return mInviter;
        }

        ///
        /// Returns the optional human-readable reason string for the invite
        ///
        QString reason() const
        {
            return mReason;
        }

        ///
        /// Returns the optional thread ID of the conversation being continued in the room
        ///
        /// This is used to upgrade one-on-one conversations to multi-user conversations
        ///
        QString threadId() const
        {
            return mThreadId;
        }

    private:
        IncomingMucRoomInvite(const QString &roomId, const JabberID &inviter, const QString &reason, const QString &threadId) :
            mNull(false),
            mRoomId(roomId),
            mInviter(inviter),
            mReason(reason),
            mThreadId(threadId)
        {
        }

        static IncomingMucRoomInvite fromJingle(const buzz::Jid& inviter, const buzz::Jid& room, const QString& reason);

        bool mNull;
        QString mRoomId;
        JabberID mInviter;
        QString mReason;
        QString mThreadId;
    };
}
}

#endif

