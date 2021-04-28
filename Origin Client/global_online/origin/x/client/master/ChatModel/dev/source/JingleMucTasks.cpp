#include "JingleMucTasks.h"
#include "ChatModelConstants.h"
#include "talk/xmpp/constants.h"
#include "talk/xmpp/xmppclient.h"
#include <QtCore/QScopedPointer>

bool MucInviteRecvTask::HandleStanza(const buzz::XmlElement* stanza)
{
    const buzz::XmlElement* xstanza;
    const buzz::XmlElement* invite;
    const buzz::XmlElement* reason;
    if (stanza->Name() != buzz::QN_MESSAGE) return false;
    xstanza = stanza->FirstNamed(buzz::QN_MUC_USER_X);
    if (!xstanza) return false;
        invite = xstanza->FirstNamed(buzz::QN_MUC_USER_INVITE);
    if (!invite) return false;
        // Else it's an invite and we definitely want to handle it.

    QString reasonStr;
    reason = invite->FirstNamed(buzz::QN_MUC_USER_REASON);
    if (reason)
        reasonStr = QString::fromUtf8(reason->BodyText().c_str());

    SignalInviteReceived(buzz::Jid(invite->Attr(buzz::QN_FROM)), buzz::Jid(stanza->Attr(buzz::QN_FROM)), reasonStr);
    return true;
}

int MucInviteRecvTask::ProcessStart()
{
    // We never queue anything so we are always blocked.
    return STATE_BLOCKED;
}

struct MucInviteSendData : public talk_base::MessageData
{
    MucInviteSendData(
        const buzz::Jid& to_,
        const buzz::Jid& room_,
        const QString& reason_,
        const QString& threadId_)
    : to(to_)
    , room(room_)
    , reason(reason_)
    , threadId(threadId_)
    {
    }
    
    const buzz::Jid to;
    const buzz::Jid room;
    const QString reason;
    const QString threadId;
};

void MucInviteSendTask::Send(
    const buzz::Jid& to,
    const buzz::Jid& room,
    const QString& reason,
    const QString& threadId,
    buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCINVITESEND, new MucInviteSendData(to, room, reason, threadId));
}

int MucInviteSendTask::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void MucInviteSendTask::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCINVITESEND)
    {
        const MucInviteSendData& data = *reinterpret_cast<const MucInviteSendData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> message(new buzz::XmlElement(buzz::QN_MESSAGE));
        message->AddAttr(buzz::QN_TO, data.room.Str());
        buzz::XmlElement* xstanza = new buzz::XmlElement(buzz::QN_MUC_USER_X);
        buzz::XmlElement* invite = new buzz::XmlElement(buzz::QN_MUC_USER_INVITE);
        buzz::XmlElement* reason = new buzz::XmlElement(buzz::QN_MUC_USER_REASON);
        buzz::XmlElement* cont = new buzz::XmlElement(buzz::QN_MUC_USER_CONTINUE);
        invite->AddAttr(buzz::QN_TO, data.to.Str());
        reason->AddText(data.reason.toStdString());
        cont->AddAttr(buzz::QN_THREAD, data.threadId.toStdString());
        invite->AddElement(reason);
        invite->AddElement(cont);
        xstanza->AddElement(invite);
        message->AddElement(xstanza);

        SendStanza(message.data());
    }
}


bool MucDeclineRecvTask::HandleStanza(const buzz::XmlElement* stanza)
{
    const buzz::XmlElement* xstanza;
    const buzz::XmlElement* decline;
    const buzz::XmlElement* reason;
    if (stanza->Name() != buzz::QN_MESSAGE) 
        return false;

    xstanza = stanza->FirstNamed(buzz::QN_MUC_USER_X);
    if (!xstanza) 
        return false;

    decline = xstanza->FirstNamed(buzz::QN_MUC_USER_DECLINE);
    if (!decline) 
        return false;

    reason = decline->FirstNamed(buzz::QN_MUC_USER_REASON);
    QString reasonStr;
    if (reason)
    {
        reasonStr = QString::fromUtf8(reason->BodyText().c_str());
    }    

    SignalDeclineReceived(buzz::Jid(decline->Attr(buzz::QN_FROM)), buzz::Jid(stanza->Attr(buzz::QN_FROM)), reasonStr);
    return true;
}

int MucDeclineRecvTask::ProcessStart()
{
    // We never queue anything so we are always blocked.
    return STATE_BLOCKED;
}

struct MucDeclineSendData : public talk_base::MessageData
{
    MucDeclineSendData(const buzz::Jid& to_, const buzz::Jid& room_, const QString& reason_)
    : to(to_)
    , room(room_)
    , reason(reason_)
    {
    }
    
    const buzz::Jid to;
    const buzz::Jid room;
    const QString reason;
};

void MucDeclineSendTask::Send(const buzz::Jid& to, const buzz::Jid& room, const QString& reason, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCDECLINESEND, new MucDeclineSendData(to, room, reason));
}

int MucDeclineSendTask::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void MucDeclineSendTask::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCDECLINESEND)
    {
        const MucDeclineSendData& data = *reinterpret_cast<const MucDeclineSendData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> message(new buzz::XmlElement(buzz::QN_MESSAGE));
        message->AddAttr(buzz::QN_TO, data.room.Str());
        buzz::XmlElement* xstanza = new buzz::XmlElement(buzz::QN_MUC_USER_X);
        buzz::XmlElement* decline = new buzz::XmlElement(buzz::QN_MUC_USER_DECLINE);
        buzz::XmlElement* reason = new buzz::XmlElement(buzz::QN_MUC_USER_REASON);
        decline->AddAttr(buzz::QN_TO, data.to.Str());
        reason->AddText(data.reason.toStdString());
        decline->AddElement(reason);
        xstanza->AddElement(decline);
        message->AddElement(xstanza);

        SendStanza(message.data());
    }
}

struct MucInstantRoomRequestData : public talk_base::MessageData
{
    MucInstantRoomRequestData(const buzz::Jid &room_)
    : room(room_)
    {
    }
    
    const buzz::Jid room;
};

void MucInstantRoomRequest::Send(const buzz::Jid &room, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCINSTANTROOMREQUEST, new MucInstantRoomRequestData(room));
}

int MucInstantRoomRequest::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void MucInstantRoomRequest::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCINSTANTROOMREQUEST)
    {
        const MucInstantRoomRequestData& data = *reinterpret_cast<const MucInstantRoomRequestData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> config(new buzz::XmlElement(buzz::QN_IQ));
        config->AddAttr(buzz::QN_ID, "OriginRoomSubmission");
        config->AddAttr(buzz::QN_TO, data.room.Str());
        config->AddAttr(buzz::QN_TYPE, "set");

        buzz::XmlElement* owner_query = new buzz::XmlElement(buzz::QN_MUC_OWNER_QUERY, true);
        buzz::XmlElement* x = new buzz::XmlElement(buzz::QN_XDATA_X, true);
        x->AddAttr(buzz::QN_TYPE, "submit");
        owner_query->AddElement(x);
        config->AddElement(owner_query);

        SendStanza(config.data());
    }
}

struct MucConfigRequestData : public talk_base::MessageData
{
    MucConfigRequestData(const buzz::Jid &room_)
    : room(room_)
    {
    }
    
    const buzz::Jid room;
};

void MucConfigRequest::Send(const buzz::Jid &room, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCCONFIGREQUEST, new MucConfigRequestData(room));
}

int MucConfigRequest::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void MucConfigRequest::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCCONFIGREQUEST)
    {
        const MucConfigRequestData& data = *reinterpret_cast<const MucConfigRequestData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> config(new buzz::XmlElement(buzz::QN_IQ));
        config->AddAttr(buzz::QN_ID, "Origin");
        config->AddAttr(buzz::QN_TO, data.room.Str());
        config->AddAttr(buzz::QN_TYPE, "get");

        buzz::XmlElement* owner_query = new
            buzz::XmlElement(buzz::QN_MUC_OWNER_QUERY, true);
        config->AddElement(owner_query);

        SendStanza(config.data());
    }
}

bool MucConfigResponse::HandleStanza(const buzz::XmlElement* stanza)
{
    // Figuring out that we want to handle this is a lot of the work of
    // actually handling it, so we handle it right here instead of queueing it.
    const buzz::XmlElement* query;
    const buzz::XmlElement* xstanza;
    const buzz::XmlElement* field;
    if (stanza->Name() != buzz::QN_IQ)
        return false;

    query = stanza->FirstNamed(buzz::QN_MUC_OWNER_QUERY);
    if (!query)
        return false;

    xstanza = query->FirstNamed(buzz::QN_XDATA_X);
    if (!xstanza)
        return false;

    QMap<QString, QString> config;

    field = xstanza->FirstNamed(buzz::QN_XDATA_FIELD);
    do {
        QString var(field->Attr(buzz::QN_VAR).c_str());

        const buzz::XmlElement* value = field->FirstNamed(buzz::QN_XDATA_VALUE);
        if (value)
        {
            QString valText = value->BodyText().c_str();
            config[var] = valText;
        }
        field = field->NextNamed(buzz::QN_XDATA_FIELD);
    } while (field != NULL);

    const buzz::Jid room(stanza->Attr(buzz::QN_FROM));

    SignalConfigReceived(config, room);
    return true;
}

int MucConfigResponse::ProcessStart()
{
    // We never queue anything so we are always blocked.
    return STATE_BLOCKED;
}

struct MucConfigSubmissionData : public talk_base::MessageData
{
    MucConfigSubmissionData(const buzz::Jid &room_, const QMap<QString, QString>& configMap_)
    : room(room_)
    , configMap(configMap_)
    {
    }
    
    const buzz::Jid room;
    QMap<QString, QString> configMap;
};

void MucConfigSubmission::Send(const buzz::Jid& room, const QMap<QString, QString>& configMap, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCCONFIGSUBMISSION, new MucConfigSubmissionData(room, configMap));
}

int MucConfigSubmission::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void MucConfigSubmission::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCCONFIGSUBMISSION)
    {
        const MucConfigSubmissionData& data = *reinterpret_cast<const MucConfigSubmissionData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> config(new buzz::XmlElement(buzz::QN_IQ));
        config->AddAttr(buzz::QN_ID, "OriginRoomSubmission");
        config->AddAttr(buzz::QN_TO, data.room.Str());
        config->AddAttr(buzz::QN_TYPE, "set");

        buzz::XmlElement* owner_query = new buzz::XmlElement(buzz::QN_MUC_OWNER_QUERY, true);
        buzz::XmlElement* x = new buzz::XmlElement(buzz::QN_XDATA_X);
        x->AddAttr(buzz::QN_TYPE, "submit");

        buzz::XmlElement* field = new buzz::XmlElement(buzz::QN_XDATA_FIELD);
        field->SetAttr(buzz::QN_VAR, "FORM_TYPE");

        buzz::XmlElement* value = new buzz::XmlElement(buzz::QN_XDATA_VALUE);
        value->SetBodyText(buzz::STR_MUC_ROOMCONFIG);

        field->AddElement(value);
        x->AddElement(field);

        QMapIterator<QString, QString> iter(data.configMap);
        while (iter.hasNext())
        {
            iter.next();
            field = new buzz::XmlElement(buzz::QN_XDATA_FIELD);
            field->SetAttr(buzz::QN_VAR, iter.key().toStdString());

            value = new buzz::XmlElement(buzz::QN_XDATA_VALUE);
            value->SetBodyText(iter.value().toStdString());

            field->AddElement(value);
            x->AddElement(field);
        }

        owner_query->AddElement(x);
        config->AddElement(owner_query);

        QString test(config->Str().c_str());

        SendStanza(config.data());
    }
}

bool MucRoomUnlocked::HandleStanza(const buzz::XmlElement* stanza)
{
    if (stanza->Name() != buzz::QN_IQ)
        return false;

    if (stanza->Attr(buzz::QN_ID).compare("OriginRoomSubmission") != 0)
        return false;

    if (stanza->Attr(buzz::QN_TYPE).compare("result") != 0)
        return false;

    const buzz::Jid room(stanza->Attr(buzz::QN_FROM));

    SignalRoomUnlocked(room);
    return true;
}

int MucRoomUnlocked::ProcessStart()
{
    // We never queue anything so we are always blocked.
    return STATE_BLOCKED;
}

const char NS_MUC_ADMIN[] = "http://jabber.org/protocol/muc#admin";
namespace buzz
{
const buzz::StaticQName QN_MUC_ADMIN_QUERY = { NS_MUC_ADMIN, "query" };
const buzz::StaticQName QN_ITEM = { STR_EMPTY, "item" };
const buzz::StaticQName QN_REASON = { STR_EMPTY, "reason" };
}

struct MucGrantModeratorData : public talk_base::MessageData
{
    MucGrantModeratorData(const buzz::Jid &room_, const buzz::Jid &user_)
    : room(room_)
    , user(user_)
    {
    }
    
    const buzz::Jid room;
    const buzz::Jid user;
};

void MucGrantModerator::Send(const buzz::Jid &room, const buzz::Jid &user, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCGRANTMODERATOR, new MucGrantModeratorData(room, user));
}

int MucGrantModerator::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void MucGrantModerator::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCGRANTMODERATOR)
    {
        const MucGrantModeratorData& data = *reinterpret_cast<const MucGrantModeratorData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> iq(new buzz::XmlElement(buzz::QN_IQ));
        iq->AddAttr(buzz::QN_ID, "Origin");
        iq->AddAttr(buzz::QN_TO, data.room.Str());
        iq->AddAttr(buzz::QN_TYPE, "set");

        buzz::XmlElement* admin_query = new buzz::XmlElement(buzz::QN_MUC_ADMIN_QUERY, true);
        buzz::XmlElement* item = new buzz::XmlElement(buzz::QN_ITEM);
        item->AddAttr(buzz::QN_ROLE, "moderator");
        item->AddAttr(buzz::QN_JID, data.user.Str());

        admin_query->AddElement(item);
        iq->AddElement(admin_query);

        SendStanza(iq.data());
    }
}

struct MucRevokeModeratorData : public talk_base::MessageData
{
    MucRevokeModeratorData(const buzz::Jid &room_, const buzz::Jid &user_)
    : room(room_)
    , user(user_)
    {
    }
    
    const buzz::Jid room;
    const buzz::Jid user;
};

void MucRevokeModerator::Send(const buzz::Jid &room, const buzz::Jid &user, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCREVOKEMODERATOR, new MucRevokeModeratorData(room, user));
}

int MucRevokeModerator::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void MucRevokeModerator::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCREVOKEMODERATOR)
    {
        const MucRevokeModeratorData& data = *reinterpret_cast<const MucRevokeModeratorData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> iq(new buzz::XmlElement(buzz::QN_IQ));
        iq->AddAttr(buzz::QN_ID, "Origin");
        iq->AddAttr(buzz::QN_TO, data.room.Str());
        iq->AddAttr(buzz::QN_TYPE, "set");

        buzz::XmlElement* admin_query = new buzz::XmlElement(buzz::QN_MUC_ADMIN_QUERY, true);
        buzz::XmlElement* item = new buzz::XmlElement(buzz::QN_ITEM);
        item->AddAttr(buzz::QN_ROLE, "participant");
        item->AddAttr(buzz::QN_JID, data.user.Str());

        admin_query->AddElement(item);
        iq->AddElement(admin_query);

        SendStanza(iq.data());
    }
}

struct MucGrantAdminData : public talk_base::MessageData
{
    MucGrantAdminData(const buzz::Jid &room_, const buzz::Jid &user_)
    : room(room_)
    , user(user_)
    {
    }
    
    const buzz::Jid room;
    const buzz::Jid user;
};

void MucGrantAdmin::Send(const buzz::Jid &room, const buzz::Jid &user, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCGRANTADMIN, new MucGrantAdminData(room, user));
}

int MucGrantAdmin::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void MucGrantAdmin::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCGRANTADMIN)
    {
        const MucGrantAdminData& data = *reinterpret_cast<const MucGrantAdminData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> iq(new buzz::XmlElement(buzz::QN_IQ));
        iq->AddAttr(buzz::QN_ID, talk_base::CreateRandomString(16));
        iq->AddAttr(buzz::QN_TO, data.room.Str());
        iq->AddAttr(buzz::QN_TYPE, "set");

        buzz::XmlElement* admin_query = new buzz::XmlElement(buzz::QN_MUC_ADMIN_QUERY, true);
        buzz::XmlElement* item = new buzz::XmlElement(buzz::QN_ITEM);
        item->AddAttr(buzz::QN_AFFILIATION, "admin");
        item->AddAttr(buzz::QN_JID, data.user.Str());

        admin_query->AddElement(item);
        iq->AddElement(admin_query);

        SendStanza(iq.data());
    }
}

struct MucRevokeAdminData : public talk_base::MessageData
{
    MucRevokeAdminData(const buzz::Jid &room_, const buzz::Jid &user_)
    : room(room_)
    , user(user_)
    {
    }
    
    const buzz::Jid room;
    const buzz::Jid user;
};

void MucRevokeAdmin::Send(const buzz::Jid &room, const buzz::Jid &user, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCREVOKEADMIN, new MucRevokeAdminData(room, user));
}

int MucRevokeAdmin::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}


void MucRevokeAdmin::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCREVOKEADMIN)
    {
        const MucRevokeAdminData& data = *reinterpret_cast<const MucRevokeAdminData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> iq(new buzz::XmlElement(buzz::QN_IQ));
        iq->AddAttr(buzz::QN_ID, talk_base::CreateRandomString(16));
        iq->AddAttr(buzz::QN_TO, data.room.Str());
        iq->AddAttr(buzz::QN_TYPE, "set");

        buzz::XmlElement* admin_query = new buzz::XmlElement(buzz::QN_MUC_ADMIN_QUERY, true);
        buzz::XmlElement* item = new buzz::XmlElement(buzz::QN_ITEM);
        item->AddAttr(buzz::QN_AFFILIATION, "member");
        item->AddAttr(buzz::QN_JID, data.user.Str());

        admin_query->AddElement(item);
        iq->AddElement(admin_query);

        SendStanza(iq.data());
    }
}

struct MucGrantMembershipData : public talk_base::MessageData
{
    MucGrantMembershipData(const buzz::Jid &room_, const buzz::Jid &user_)
    : room(room_)
    , user(user_)
    {
    }
    
    const buzz::Jid room;
    const buzz::Jid user;
};

void MucGrantMembership::Send(const buzz::Jid &room, const buzz::Jid &user, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCGRANTMEMBERSHIP, new MucGrantMembershipData(room, user));
}

int MucGrantMembership::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}


void MucGrantMembership::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCGRANTMEMBERSHIP)
    {
        const MucGrantMembershipData& data = *reinterpret_cast<const MucGrantMembershipData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> iq(new buzz::XmlElement(buzz::QN_IQ));
        iq->AddAttr(buzz::QN_ID, talk_base::CreateRandomString(16));
        iq->AddAttr(buzz::QN_TO, data.room.Str());
        iq->AddAttr(buzz::QN_TYPE, "set");

        buzz::XmlElement* admin_query = new buzz::XmlElement(buzz::QN_MUC_ADMIN_QUERY, true);
        buzz::XmlElement* item = new buzz::XmlElement(buzz::QN_ITEM);
        item->AddAttr(buzz::QN_AFFILIATION, "member");
        item->AddAttr(buzz::QN_JID, data.user.Str());

        admin_query->AddElement(item);
        iq->AddElement(admin_query);

        SendStanza(iq.data());
    }
}

struct MucRevokeMembershipData : public talk_base::MessageData
{
    MucRevokeMembershipData(const buzz::Jid &room_, const buzz::Jid &user_)
    : room(room_)
    , user(user_)
    {
    }
    
    const buzz::Jid room;
    const buzz::Jid user;
};

void MucRevokeMembership::Send(const buzz::Jid &room, const buzz::Jid &user, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCREVOKEMEMBERSHIP, new MucRevokeMembershipData(room, user));
}

int MucRevokeMembership::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void MucRevokeMembership::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCREVOKEMEMBERSHIP)
    {
        const MucRevokeMembershipData& data = *reinterpret_cast<const MucRevokeMembershipData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> iq(new buzz::XmlElement(buzz::QN_IQ));
        iq->AddAttr(buzz::QN_ID, "Origin");
        iq->AddAttr(buzz::QN_TO, data.room.Str());
        iq->AddAttr(buzz::QN_TYPE, "set");

        buzz::XmlElement* admin_query = new buzz::XmlElement(buzz::QN_MUC_ADMIN_QUERY, true);
        buzz::XmlElement* item = new buzz::XmlElement(buzz::QN_ITEM);
        item->AddAttr(buzz::QN_AFFILIATION, "none");
        item->AddAttr(buzz::QN_JID, data.user.Str());

        admin_query->AddElement(item);
        iq->AddElement(admin_query);

        SendStanza(iq.data());
    }
}

struct MucGrantVoiceData : public talk_base::MessageData
{
    MucGrantVoiceData(const buzz::Jid &room_, const buzz::Jid &user_)
    : room(room_)
    , user(user_)
    {
    }
    
    const buzz::Jid room;
    const buzz::Jid user;
};

void MucGrantVoice::Send(const buzz::Jid &room, const buzz::Jid &user, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCGRANTVOICE, new MucGrantVoiceData(room, user));
}

int MucGrantVoice::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void MucGrantVoice::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;
    
    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCGRANTVOICE)
    {
        const MucGrantVoiceData& data = *reinterpret_cast<const MucGrantVoiceData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> iq(new buzz::XmlElement(buzz::QN_IQ));
        iq->AddAttr(buzz::QN_ID, "Origin");
        iq->AddAttr(buzz::QN_TO, data.room.Str());
        iq->AddAttr(buzz::QN_TYPE, "set");

        buzz::XmlElement* admin_query = new buzz::XmlElement(buzz::QN_MUC_ADMIN_QUERY, true);
        buzz::XmlElement* item = new buzz::XmlElement(buzz::QN_ITEM);
        item->AddAttr(buzz::QN_ROLE, "participant");
        item->AddAttr(buzz::QN_JID, data.user.Str());

        admin_query->AddElement(item);
        iq->AddElement(admin_query);

        SendStanza(iq.data());
    }
}

struct MucRevokeVoiceData : public talk_base::MessageData
{
    MucRevokeVoiceData(const buzz::Jid &room_, const buzz::Jid &user_)
    : room(room_)
    , user(user_)
    {
    }
    
    const buzz::Jid room;
    const buzz::Jid user;
};

void MucRevokeVoice::Send(const buzz::Jid &room, const buzz::Jid &user, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCREVOKEVOICE, new MucRevokeVoiceData(room, user));
}

int MucRevokeVoice::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void MucRevokeVoice::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_MUCREVOKEVOICE)
    {
        const MucRevokeVoiceData& data = *reinterpret_cast<const MucRevokeVoiceData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> iq(new buzz::XmlElement(buzz::QN_IQ));
        iq->AddAttr(buzz::QN_ID, "Origin");
        iq->AddAttr(buzz::QN_TO, data.room.Str());
        iq->AddAttr(buzz::QN_TYPE, "set");

        buzz::XmlElement* admin_query = new buzz::XmlElement(buzz::QN_MUC_ADMIN_QUERY, true);
        buzz::XmlElement* item = new buzz::XmlElement(buzz::QN_ITEM);
        item->AddAttr(buzz::QN_ROLE, "visitor");
        item->AddAttr(buzz::QN_JID, data.user.Str());

        admin_query->AddElement(item);
        iq->AddElement(admin_query);

        SendStanza(iq.data());
    }
}

struct MucKickOccupantData : public talk_base::MessageData
{
    MucKickOccupantData(const buzz::Jid &room_, const QString &userNickname_, const QString &reason_)
        : room(room_)
        , userNickname(userNickname_)
        , reason(reason_)
    {
    }

    const buzz::Jid room;
    const QString userNickname;
    const QString reason;
};

void MucKickOccupant::Send(const buzz::Jid &room, const QString& userNickname, const QString& reason, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_MUCKICKOCCUPANT, new MucKickOccupantData(room, userNickname, reason));
}

int MucKickOccupant::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void MucKickOccupant::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;

    if (msg->message_id == Origin::Chat::MSG_MUCKICKOCCUPANT)
    {
        const MucKickOccupantData& data = *reinterpret_cast<const MucKickOccupantData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> iq(new buzz::XmlElement(buzz::QN_IQ));

        iq->AddAttr(buzz::QN_ID, "OriginKick");
        iq->AddAttr(buzz::QN_TO, data.room.Str());
        iq->AddAttr(buzz::QN_TYPE, "set");

        buzz::XmlElement* admin_query = new buzz::XmlElement(buzz::QN_MUC_ADMIN_QUERY, true);
        buzz::XmlElement* item = new buzz::XmlElement(buzz::QN_ITEM);
        buzz::XmlElement* reason = new buzz::XmlElement(buzz::QN_REASON);

        item->AddAttr(buzz::QN_ROLE, "none");
        item->AddAttr(buzz::QN_NICK, data.userNickname.toStdString());

        reason->AddText(data.reason.toStdString());
        item->AddElement(reason);

        admin_query->AddElement(item);
        iq->AddElement(admin_query);

        SendStanza(iq.data());
    }
}
