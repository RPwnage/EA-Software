#ifndef _CHATMODEL_CONVERSABLE_H
#define _CHATMODEL_CONVERSABLE_H

#include <QObject>
#include <QString>
#include <QSet>
#include <QPointer>

#include "JabberID.h"
#include "Message.h"
#include "ConversableParticipant.h"
#include "Connection.h"

namespace Origin
{
namespace Chat
{
    class Connection;

    ///
    /// Interface for entities that messages can be sent to or received from
    ///
    /// Example of Conversables include remote user and multiuser chats
    ///
    class Conversable : public QObject
    {
        friend class Connection;

        Q_OBJECT
    public:
        ///
        /// Returns the connection the conversable belongs to
        ///
        virtual Connection* connection() const
        {
            return mConnection;
        }

        ///
        /// Returns the Jabber ID for the conversable
        ///
        virtual JabberID jabberId() const = 0;
        
        ///
        /// Returns a list of remote Jabber IDs participating in the conversation. 
        ///
        /// These must all be bare Jabber IDs.
        ///
        /// The current user is also implicitly participating in the conversation
        ///
        virtual QSet<ConversableParticipant> participants() const = 0;
        
        ///
        /// Sends a message to the conversable asynchronously
        ///
        /// This is a convenience function; see Chat::Message and Chat::Connection::sendMessage() for the full message
        /// sending API
        ///
        /// \param  body      Body of the message to send
        /// \param  threadId  Thread ID of the message. See XEP-0201 for threading best practices.
        ///
        void sendMessage(const QString &body, const QString &threadId = Message::generateUniqueThreadId());

        ///
        /// Sends a composing message to the conversable asynchronously
        ///
        void sendComposing();

        ///
        /// Sends a stop composing message to the conversable asynchronously
        ///
        void sendStopComposing();

        ///
        /// Sends a paused message to the conversable asynchronously
        ///
        void sendPaused();

        ///
        /// Sends a gone message to the conversable asynchronously
        ///
        void sendGone();

        ///
        /// Returns the total number of messages sent to this conversable
        ///
        /// If a conversable has been left and re-joined this counter will be reset
        ///
        unsigned int messagesSent()
        {
            return mMessagesSent;
        }
        
        ///
        /// Returns the total number of messages receiving from this conversable
        ///
        /// If a conversable has been left and re-joined this counter will be reset
        ///
        unsigned int messagesReceived()
        {
            return mMessagesReceived;
        }

    signals:
        ///
        /// Emitted when a message is received from this conversable
        ///
        /// \sa Chat::Connection::messageReceived
        ///
        void messageReceived(const Origin::Chat::Message &);

        ///
        /// Emitted when a chat state is received from this conversable
        ///
        /// \sa Chat::Connection::chatStateReceived
        ///
        void chatStateReceived(const Origin::Chat::Message &);
        
        ///
        /// Emitted when a message is sent to this conversable
        ///
        /// \sa Chat::Connection::messageSent
        ///
        void messageSent(const Origin::Chat::Message &);

        ///
        /// Emitted when a participant joins the conversation
        ///
        void participantAdded(const Origin::Chat::ConversableParticipant &);

        ///
        /// Emitted when a participant leaves the conversation
        ///
        void participantRemoved(const Origin::Chat::ConversableParticipant &);

        ///
        /// Emitted when a participant is changed in the conversation
        ///
        void participantChanged(const Origin::Chat::ConversableParticipant &);

    protected:
        ///
        /// Creates a conversable with the given connection and QObject parent
        ///
        explicit Conversable(Connection *connection, const QString &messageType, QObject *parent = NULL) : 
            QObject(parent),
            mConnection(connection),
            mMessageType(messageType),
            mMessagesSent(0),
            mMessagesReceived(0),
            mChatStatesEnabled(false)
        {
        }

        virtual bool enableChatStates() const
        {
            return mChatStatesEnabled;
        }

    // Friend functions
    private:
        void receivedMessage(const Message &);
        void receivedChatState(const Message &);
        void sentMessage(const Message &);

    private:
        QPointer<Connection> mConnection;
        const QString mMessageType;
        unsigned int mMessagesSent;
        unsigned int mMessagesReceived;

        bool mChatStatesEnabled;
    };
}
}

#endif

