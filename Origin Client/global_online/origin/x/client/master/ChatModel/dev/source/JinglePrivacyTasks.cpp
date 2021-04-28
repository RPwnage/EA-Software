#include "JinglePrivacyTasks.h"
#include "ChatModelConstants.h"
#include "talk/xmpp/constants.h"
#include "talk/xmpp/xmppclient.h"
#include <QtCore/QScopedPointer>

bool BlockListRecv::HandleStanza(const buzz::XmlElement* stanza)
{
    if (stanza->Name() != buzz::QN_PRIVACY_IQ)
        return false;

    const buzz::XmlElement* query = stanza->FirstNamed(buzz::QN_PRIVACY_QUERY);
    if (!query)
        return false;

    const buzz::XmlElement* list = query->FirstNamed(buzz::QN_PRIVACY_LIST);
    if (!list)
        return false;

    QList<buzz::Jid> blockList;

    const buzz::XmlElement* item = list->FirstNamed(buzz::QN_PRIVACY_ITEM);
    if (!item)
    {
        // No item means that the list is empty
        SignalBlockListReceived(blockList);
        return true;
    }

    do {
        buzz::Jid jid(item->Attr(buzz::QN_VALUE));
        blockList.push_back(jid);
        item = item->NextNamed(buzz::QN_PRIVACY_ITEM);
    } while (item != NULL);

    SignalBlockListReceived(blockList);
    return true;
}

int BlockListRecv::ProcessStart()
{
    // We never queue anything so we are always blocked.
    return STATE_BLOCKED;
}

bool BlockListPush::HandleStanza(const buzz::XmlElement* stanza)
{
    if (stanza->Name() != buzz::QN_IQ)
        return false;

    if (stanza->Attr(buzz::QN_TYPE).compare("set") != 0)
        return false;

    const buzz::XmlElement* query = stanza->FirstNamed(buzz::QN_PRIVACY_QUERY);
    if (!query)
        return false;

    const buzz::XmlElement* list = query->FirstNamed(buzz::QN_PRIVACY_LIST);
    if (!list)
        return false;

    if (list->Attr(buzz::QN_NAME).compare("global") != 0)
        return false;

    SignalBlockListPushReceived();
    return true;
}

int BlockListPush::ProcessStart()
{
    // We never queue anything so we are always blocked.
    return STATE_BLOCKED;
}

struct BlockListSendData : public talk_base::MessageData
{
    BlockListSendData(const QList<buzz::Jid>& blockList_)
    : blockList(blockList_)
    {
    }
    
    QList<buzz::Jid> blockList;
};

void BlockListSend::Send(const QList<buzz::Jid>& blockList, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_PRIVACYBLOCKLISTSEND, new BlockListSendData(blockList));
}

int BlockListSend::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void BlockListSend::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_PRIVACYBLOCKLISTSEND)
    {
        BlockListSendData& data = *reinterpret_cast<BlockListSendData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> iq (new buzz::XmlElement(buzz::QN_PRIVACY_IQ));
        iq->AddAttr(buzz::QN_ID, "Origin");
        iq->AddAttr(buzz::QN_TYPE, "set");
        buzz::XmlElement* query = new buzz::XmlElement(buzz::QN_PRIVACY_QUERY);
        buzz::XmlElement* list = new buzz::XmlElement(buzz::QN_PRIVACY_LIST);
        list->AddAttr(buzz::QN_NAME, "global");

        int order = 1;
        for (QList<buzz::Jid>::const_iterator it = data.blockList.constBegin();
            it != data.blockList.constEnd();
            it++)
        {
            buzz::XmlElement* item = new buzz::XmlElement(buzz::QN_PRIVACY_ITEM);
            item->AddAttr(buzz::QN_TYPE, "jid");
            item->AddAttr(buzz::QN_VALUE, (*it).Str());
            item->AddAttr(buzz::QN_ACTION, "deny");
            item->AddAttr(buzz::QN_ORDER, QString::number(order).toStdString());

            list->AddElement(item);
        }

        query->AddElement(list);
        iq->AddElement(query);

        SendStanza(iq.data());
    }
    delete msg->pdata;
    msg->pdata = NULL;
}

struct BlockListRequestData : public talk_base::MessageData
{
    BlockListRequestData()
    {
    }
};

void BlockListRequest::Send(buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_PRIVACYBLOCKLISTREQUEST, new BlockListRequestData());
}

int BlockListRequest::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void BlockListRequest::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_PRIVACYBLOCKLISTREQUEST)
    {
        QScopedPointer<buzz::XmlElement> iq (new buzz::XmlElement(buzz::QN_PRIVACY_IQ));
        iq->AddAttr(buzz::QN_TYPE, "get");
        iq->AddAttr(buzz::QN_ID, talk_base::CreateRandomString(16));
        buzz::XmlElement* query = new buzz::XmlElement(buzz::QN_PRIVACY_QUERY);
        buzz::XmlElement* list = new buzz::XmlElement(buzz::QN_PRIVACY_LIST);
        list->AddAttr(buzz::QN_NAME, "global");
        query->AddElement(list);
        iq->AddElement(query);

        SendStanza(iq.data());
    }
    delete msg->pdata;
    msg->pdata = NULL;
}

struct SetPrivacyListData : public talk_base::MessageData
{
    SetPrivacyListData(const QString& name_)
    : name(name_)
    , id("")
    {
    }
    
    const QString name;
    std::string id;
};

void SetPrivacyList::Send(const QString& name, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_PRIVACYSETPRIVACYLIST, new SetPrivacyListData(name));
}

int SetPrivacyList::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void SetPrivacyList::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;

    if (msg->message_id == Origin::Chat::MSG_PRIVACYSETPRIVACYLIST)
    {
        SetPrivacyListData& data = *reinterpret_cast<SetPrivacyListData*>(msg->pdata);
        QScopedPointer<buzz::XmlElement> iq (new buzz::XmlElement(buzz::QN_PRIVACY_IQ));
        iq->AddAttr(buzz::QN_TYPE, "set");
        id = talk_base::CreateRandomString(16);
        iq->AddAttr(buzz::QN_ID, id);
        buzz::XmlElement* query = new buzz::XmlElement(buzz::QN_PRIVACY_QUERY);
        buzz::XmlElement* active = new buzz::XmlElement(buzz::QN_PRIVACY_ACTIVE);
        active->AddAttr(buzz::QN_NAME, data.name.toStdString());
        query->AddElement(active);
        iq->AddElement(query);

        SendStanza(iq.data());
    }
    delete msg->pdata;
    msg->pdata = NULL;
   
}

bool SetPrivacyList::HandleStanza(const buzz::XmlElement* stanza)
{
    if (!MatchResponsePrivacy(stanza, id))
    {
        return false;
    }

    SignalPrivacyReceived();
    return true;
}
