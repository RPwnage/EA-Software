#ifndef _OUTGOINGMUCROOMINVITE_H
#define _OUTGOINGMUCROOMINVITE_H

#include <QString>

#include "XMPPImplTypes.h"
#include "JabberID.h"

namespace Origin
{
namespace Chat
{
    class Connection;

    ///
    /// Represents a multi-user chat room invitation received from a remote user
    ///
    class OutgoingMucRoomInvite
    {
        friend class Connection;
    public:
        ///
        /// Constructs a new OutgoingMucRoomInvite
        ///
        /// \param  invitee   Remote user to invite to the room
        /// \param  reason    Human-readable reason string for inviting the user to the room
        /// \param  threadId  Optional thread ID of the conversation being continued in the room 
        ///
        OutgoingMucRoomInvite(const JabberID &invitee, const QString &reason = QString(), const QString &threadId = QString()) :
            mInvitee(invitee),
            mReason(reason),
            mThreadId(threadId)
        {
        }

        ///
        /// Returns the Jabber ID of the user being invited
        ///
        JabberID invitee() const
        {
            return mInvitee;
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
        buzz::Jid toJingle() const;

        JabberID mInvitee;
        QString mReason;
        QString mThreadId;
    };
}
}

#endif

