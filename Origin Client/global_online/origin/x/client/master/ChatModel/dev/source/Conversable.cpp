#include "Conversable.h"

#include "Connection.h"
#include "ConnectedUser.h"
#include "JingleClient.h"
#include "QRegularExpression.h"

namespace Origin
{
namespace Chat
{
    void Conversable::sendMessage(const QString &body, const QString &threadId)
    {
        Message message(mMessageType, mConnection->currentUser()->jabberId(), jabberId(), ChatStateActive, threadId);

        //we want to remove all non-printable chars from the message (including ESC).
        QString filteredBody = body;
        filteredBody.remove(QRegularExpression("[\\x00-\\x09\\x0B\\x0C\\x0E-\\x1F\\x7f]"));        

        if(!filteredBody.isEmpty())
        {
            message.setBody(filteredBody);
            return mConnection->sendMessage(message);
        }
    }

    void Conversable::sendComposing()
    {
        if (!enableChatStates())
        {
            return;
        }

        Message message(mMessageType, mConnection->currentUser()->jabberId(), jabberId(), ChatStateComposing);
    
        return mConnection->sendMessage(message);
    }

    void Conversable::sendStopComposing()
    {
        if (!enableChatStates())
        {
            return;
        }

        Message message(mMessageType, mConnection->currentUser()->jabberId(), jabberId(), ChatStateActive);

        return mConnection->sendMessage(message);
    }

    void Conversable::sendPaused()
    {
        if (!enableChatStates())
        {
            return;
        }

        Message message(mMessageType, mConnection->currentUser()->jabberId(), jabberId(), ChatStatePaused);

        return mConnection->sendMessage(message);
    }

    void Conversable::sendGone()
    {
        if (!enableChatStates())
        {
            return;
        }

        Message message(mMessageType, mConnection->currentUser()->jabberId(), jabberId(), ChatStateGone);

        return mConnection->sendMessage(message);
    }
    
    void Conversable::receivedMessage(const Message &msg)
    {
        mMessagesReceived++;
        emit messageReceived(msg);

        // If this message has any chat state in it then we know we can send them back
        if (msg.state() != ChatStateNone)
        {
            mChatStatesEnabled = true;
        }
    }

    void Conversable::receivedChatState(const Message &msg)
    {
        emit chatStateReceived(msg);
    }
    
    void Conversable::sentMessage(const Message &msg)
    {
        mMessagesSent++;
        emit messageSent(msg);
    }

}
}
