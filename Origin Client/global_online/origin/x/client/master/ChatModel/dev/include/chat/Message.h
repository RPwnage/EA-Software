#ifndef _CHATMODEL_MESSAGE_H
#define _CHATMODEL_MESSAGE_H

#include <QString>
#include <QMetaType>
#include <QDateTime>

#include "JabberID.h"
#include "XMPPImplTypes.h"
#include "ChatModelConstants.h"

namespace Origin
{
namespace Chat
{
    ///
    /// Represents an incoming or outgoing XMPP message
    /// 
    class Message
    {
        friend class Connection;
    public:
        ///
        /// Creates a null message
        ///
        Message();

        ///
        /// Creates a message with the specified addressing
        ///
        /// \param type      Type of this message. User-to-user IM messages should always be of type "chat"
        /// \param from      Jabber ID sending this message. Typically will be the Jabber ID of the connected user. 
        /// \param to        Jabber ID this message is addressed to. May be bare to send a message to any connected resource.
        /// \param state     Chat Type state for XEP-0085
        /// \param threadId  Thread this message is associated with. See XEP-0201 for information on threading best 
        ///                  practices. If this parameter is null a new thread ID is generated
        /// \param subject   Human-readable subject of the message
        ///
        /// \sa Chat::ConnectedUser::jabberId
        /// \sa Chat::Message::generateUniqueThreadId
        ///
        Message(const QString &type, const JabberID &from, const JabberID &to, ChatState state, const QString &threadId = generateUniqueThreadId(), const QString &subject = QString());

        ///
        /// Returns if this is a null message
        ///
        /// \sa Message()
        ///
        bool isNull() const
        {
            return mNull;
        }

        ///
        /// Returns the Jabber ID this message originated from
        ///
        JabberID from() const
        {
            return mFrom;
        }

        ///
        /// Returns the Jabber ID this message is addressed to
        ///
        JabberID to() const
        {
            return mTo;
        }

        ///
        /// Returns the thread ID for this message
        ///
        /// See XEP-0201 for messge threading best practices
        ///
        QString threadId() const
        {
            return mThreadId;
        }    

        ///
        /// Sets the body of this message
        ///
        void setBody(const QString &body)
        {
            mBody = body;
        }

        ///
        /// Returns the body of this message
        ///
        QString body() const
        {
            return mBody;
        }
        
        ///
        /// Sets the subject of this message
        ///
        void setSubject(const QString &subject)
        {
            mSubject = subject;
        }

        ///
        /// Returns the subject of this message
        ///
        QString subject() const
        {
            return mSubject;
        }

        ///
        /// Sets the type of this message
        ///
        void setType(const QString &type)
        {
            mType = type;
        }

        ///
        /// Returns the type of this message
        ///
        QString type() const
        {
            return mType;
        }

        ///
        /// Returns the chat state associated with this message
        ///
        ChatState state() const
        {
            return mState;
        }

        ///
        /// Creates an empty message in the same thread with to and from addresses swapped
        ///
        Message createReply() const;

        ///
        /// Creates an empty message in the same thread with the same addressing
        ///
        Message createFollowUp() const;

        ///
        /// Returns the original send time of a message
        ///
        /// For messages that have been delayed (e.g. for offline delivery) this time can be much earlier than the
        /// receive time.
        ///
        /// The send time will be null for outgoing messages or if  the original send time cannot be determined. It
        /// should be safe to assume such messages are recent. 
        ///
        /// The timestamp will be in an arbitrary timezone. See the QDateTime documentation for a discussion of
        /// timezones as they relate to QDateTime.
        ///
        QDateTime sendTime() const
        {
            return mSendTime;
        }
        
        ///
        /// Generates a new thread ID
        ///
        /// Thread IDs don't have a particular format as long as they're unique. This is just a helper function.
        ///
        static QString generateUniqueThreadId();
    
    private: // Friend functions
        ///
        /// Creates an implementation message based on this one
        ///
        JingleMessage toJingle() const;

        ///
        /// Creates a new Message with the from field rewritten
        ///
        Message withFrom(const JabberID &from) const;

        ///
        /// Creates a Message instance based on an implementation message
        ///
        static Message fromJingle(const JingleMessage &jingle);

    private:
        void setSendTime(const QDateTime &sendTime)
        {
            mSendTime = sendTime;
        }

        bool mNull;
        
        QString mType;

        JabberID mFrom;
        JabberID mTo;

        ChatState mState;

        QString mBody;
        QString mThreadId;
        QString mSubject;

        QDateTime mSendTime;
    };
}
}

#endif
