#include "JingleChatGroupTasks.h"
#include "ChatModelConstants.h"
#include "talk/xmpp/constants.h"
#include "talk/xmpp/xmppclient.h"
#include <QtCore/QScopedPointer>

#include "qdebug.h"

bool ChatGroupRoomInviteRecvTask::HandleStanza(const buzz::XmlElement* stanza)
{
    const buzz::XmlElement* xstanza;
    const buzz::XmlElement* invite;
    if (stanza->Name() != buzz::QN_MESSAGE) return false;
    xstanza = stanza->FirstNamed(buzz::QN_MUC_USER_X);
    if (!xstanza) return false;
        invite = xstanza->FirstNamed(buzz::QN_GROUP_CHANNEL_USER_INVITE);
    if (!invite) return false;
        // Else it's a group room invite and we definitely want to handle it.

    SignalInviteReceived(buzz::Jid(stanza->Attr(buzz::QN_FROM)),
                        QString(invite->Attr(buzz::QN_GROUPID).c_str()),
                        QString(invite->Attr(buzz::QN_CHANNELID).c_str()));
    return true;
}

int ChatGroupRoomInviteRecvTask::ProcessStart()
{
    // We never queue anything so we are always blocked.
    return STATE_BLOCKED;
}

struct ChatGroupInviteSendData : public talk_base::MessageData
{
    ChatGroupInviteSendData(
        const buzz::Jid& to_,
        const QString& roomId_,
        const QString& channelId_)
    : to(to_)
    , groupId(roomId_)
    , channelId(channelId_)
    {
    }
    
    const buzz::Jid to;
    const QString groupId;
    const QString channelId;
};

void ChatGroupRoomInviteSendTask::Send(const buzz::Jid& toJid, const QString& groupId, const QString& channelId, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_CHATGROUPROOMINVITE, new ChatGroupInviteSendData(toJid, groupId, channelId));
}

int ChatGroupRoomInviteSendTask::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void ChatGroupRoomInviteSendTask::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_CHATGROUPROOMINVITE)
    {
        const ChatGroupInviteSendData& data = *reinterpret_cast<const ChatGroupInviteSendData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> message(new buzz::XmlElement(buzz::QN_MESSAGE));
        message->AddAttr(buzz::QN_TO, data.to.Str());
        buzz::XmlElement* xstanza = new buzz::XmlElement(buzz::QN_MUC_USER_X);
        buzz::XmlElement* invite = new buzz::XmlElement(buzz::QN_GROUP_CHANNEL_USER_INVITE);
        buzz::XmlElement* cont = new buzz::XmlElement(buzz::QN_MUC_USER_CONTINUE); // DO I NEED THIS????
        invite->AddAttr(buzz::QN_TO, data.to.Str());
        invite->AddAttr(buzz::QN_GROUPID, data.groupId.toStdString());
        invite->AddAttr(buzz::QN_CHANNELID, data.channelId.toStdString());
        invite->AddElement(cont);
        xstanza->AddElement(invite);
        message->AddElement(xstanza);

        SendStanza(message.data());
    }
}

