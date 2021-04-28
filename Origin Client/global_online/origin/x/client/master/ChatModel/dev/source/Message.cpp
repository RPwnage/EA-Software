#include <QUuid>
#include "JingleClient.h"

#include "Message.h"

namespace Origin
{
namespace Chat
{
    Message::Message() : 
        mNull(true)
    {
    }

    Message::Message(const QString &type, const JabberID &from, const JabberID &to, ChatState state, const QString &threadId, const QString &subject) :
        mNull(false),
        mType(type),
        mFrom(from),
        mTo(to),
        mState(state),
        mThreadId(threadId),
        mSubject(subject)
    {
    }
        
    QString Message::generateUniqueThreadId()
    {
        QString randomString = QUuid::createUuid().toString();

        // Remove some of punctuation so it looks nicer
        return randomString.replace('{', "").replace('}', "").replace('-', "");
    }
    
    Message Message::createReply() const
    {
        // Swap our to and from on the reply
        Message newReply(type(), to(), from(), ChatStateActive, threadId());
        
        return newReply;
    }
    
    Message Message::createFollowUp() const
    {
        // Swap our to and from on the reply
        Message newFollowUp(type(), from(), to(), ChatStateActive, threadId());
        
        return newFollowUp;
    }

    JingleMessage Message::toJingle() const
    {
        JingleMessage ret;

        ret.from = from().toJingle();
        ret.to = to().toJingle();
        ret.state = mState;
        ret.thread = threadId().toUtf8().constData();
        ret.subject = subject().toUtf8().constData();
        ret.type = type().toUtf8().constData();

        ret.body = body().toUtf8().constData();

        return ret;
    }

    Message Message::fromJingle(const JingleMessage& msg)
    {
        Message ret(
            QString::fromUtf8(msg.type.c_str()),
            JabberID::fromJingle(msg.from),
            JabberID::fromJingle(msg.to),
            msg.state,
            QString::fromUtf8(msg.thread.c_str()),
            QString());

        ret.setBody(QString::fromUtf8(msg.body.c_str()));
        
        if (!msg.subject.empty())
        {
            ret.setSubject(QString::fromUtf8(msg.subject.c_str()));
        }

        // TODO JINGLE: Add timestamp
        
        return ret;
    }
        
    Message Message::withFrom(const JabberID &from) const
    {
        Message rewritten(*this);
        rewritten.mFrom = from;
        return rewritten;
    }
}
}
