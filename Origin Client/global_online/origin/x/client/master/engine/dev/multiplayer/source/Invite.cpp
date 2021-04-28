#include "engine/multiplayer/Invite.h"

#include <QCoreApplication>
#include <QList>

#include "chat/Message.h"
#include "chat/OriginGameActivity.h"

namespace
{
    const QString InviteSolicitationType("ebisu-request-invite");
    const QString InviteSolicitationDeclineType("ebisu-request-invite-decline");
    const QString InviteType("ebisu-invite");

    QString defaultMessageBody()
    {
        return QString("This message requires %1 to view").arg(QCoreApplication::applicationName());
    }
}

namespace Origin
{
namespace Engine
{
namespace MultiplayerInvite
{
    Invite::Invite(const Chat::JabberID &from, const Chat::JabberID &to, const SessionInfo &sessionInfo, const QString &message) :
        mValid(true),
        mFrom(from),
        mTo(to),
        mSessionInfo(sessionInfo),
        mMessage(message)
    {
    }

    Invite::Invite(const Chat::Message &message) :
        mValid(false)
    {
        if (!isInviteMessage(message))
        {
            // Nope
            return;
        }

        mFrom = message.from();
        mTo = message.to();

        QList<QByteArray> subjectParts = message.subject().toLatin1().split(';');

        if (subjectParts.length() < 3)
        {
            // Not valid
            return;
        }

        const QString productId = QString::fromLatin1(subjectParts.takeFirst());
        const QString multiplayerId = QString::fromLatin1(subjectParts.takeFirst());
        QByteArray sessionInfo = subjectParts.takeFirst();

        // The session can contain ; so add on all the additional parts we make have split out
        while(!subjectParts.isEmpty())
        {
            sessionInfo += ";" + subjectParts.takeFirst();
        }
        
        mSessionInfo = SessionInfo(productId, multiplayerId, sessionInfo);

        if (message.subject() != defaultMessageBody())
        {
            mMessage = message.subject();
        }

        mValid = true;
    }

    Invite::Invite() : mValid(false)
    {
    }

    Chat::Message Invite::toXmppMessage(const QString &threadId)
    {
        const QByteArray inviteSubject = mSessionInfo.productId().toLatin1() + ";" + mSessionInfo.multiplayerId().toLatin1() + ";" + mSessionInfo.data();

        Chat::Message ret(InviteType, from(), to(), Chat::ChatStateActive, threadId); 

        ret.setSubject(inviteSubject);

        if (message().isNull())
        {
            ret.setBody(defaultMessageBody());
        }
        else
        {
            ret.setBody(message());
        }

        return ret;
    }

    bool isInviteSolicitationMessage(const Chat::Message &message)
    {
        return message.type() == InviteSolicitationType;
    }

    bool isInviteSolicitationDeclineMessage(const Chat::Message &message)
    {
        return message.type() == InviteSolicitationDeclineType;
    }

    bool isInviteMessage(const Chat::Message &message)
    {
        return message.type() == InviteType;
    }
    
    Chat::Message createInviteSolicitationMessage(const Chat::JabberID &from, const Chat::JabberID &to)
    {
        Chat::Message ret(InviteSolicitationType, from, to, Origin::Chat::ChatStateActive, Chat::Message::generateUniqueThreadId());

        ret.setBody(defaultMessageBody());

        return ret;
    }

    Chat::Message createInviteSolicitationDeclineMessage(const Chat::Message &solicitation)
    {
        Chat::Message ret = solicitation.createReply();

        ret.setType(InviteSolicitationDeclineType);
        ret.setBody(defaultMessageBody());

        return ret;
    }
}
}
}
