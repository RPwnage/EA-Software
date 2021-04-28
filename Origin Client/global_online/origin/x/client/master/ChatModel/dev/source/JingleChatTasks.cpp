#include "JingleChatTasks.h"
#include "talk/xmpp/constants.h"
#include "talk/xmpp/xmppclient.h"
#include <QtCore/QScopedPointer>

bool ChatReceiveTask::HandleStanza(const buzz::XmlElement* stanza)
{
    const buzz::XmlElement* thread;
    const buzz::XmlElement* body;
    const buzz::XmlElement* subject;

    const buzz::XmlElement* sent;
    const buzz::XmlElement* forwarded;
    const buzz::XmlElement* message;


    if (stanza->Name() != buzz::QN_MESSAGE) 
        return false;

    thread = stanza->FirstNamed(buzz::QN_THREAD);

    // check for forwarded message
    buzz::QName QN_SENT("urn:xmpp:carbons:2", "sent");
    sent = stanza->FirstNamed(QN_SENT);
    if(sent)
    {
        buzz::QName QN_FORWARDED("urn:xmpp:forward:0", "forwarded");
        forwarded = sent->FirstNamed(QN_FORWARDED);
        if(forwarded)
        {
            message = forwarded->FirstNamed(buzz::QN_MESSAGE);
            HandleStanza(message);
        }
    }

    body = stanza->FirstNamed(buzz::QN_BODY);
    if (!body)
        return false;

    std::string threadStr;
    if (thread)
        threadStr = thread->BodyText();

    std::string bodyStr = stanza->FirstNamed(buzz::QN_BODY)->BodyText();

    const buzz::Jid from(stanza->Attr(buzz::QN_FROM));
    const buzz::Jid to(stanza->Attr(buzz::QN_TO));

    std::string subjectStr;
    subject = stanza->FirstNamed(buzz::QN_SUBJECT);
    if (subject)
        subjectStr = subject->BodyText();

    std::string type = stanza->Attr(buzz::QN_TYPE);

    JingleMessage jingleMsg;
    jingleMsg.from = from;
    jingleMsg.to = to;
    jingleMsg.body = bodyStr;
    jingleMsg.thread = threadStr;
    jingleMsg.subject = subjectStr;
    jingleMsg.type = type;
    jingleMsg.state = Origin::Chat::ChatStateNone;

    const buzz::XmlElement* chatState = stanza->FirstWithNamespace(buzz::NS_CHATSTATE);
    if (chatState)
    {
        if (chatState->Name().Compare(buzz::QN_CS_ACTIVE) == 0)
        {
            jingleMsg.state = Origin::Chat::ChatStateActive;
        }
        else if (chatState->Name().Compare(buzz::QN_CS_INACTIVE) == 0)
        {
            jingleMsg.state = Origin::Chat::ChatStateInactive;
        }
        else if (chatState->Name().Compare(buzz::QN_CS_GONE) == 0)
        {
            jingleMsg.state = Origin::Chat::ChatStateGone;
        }
        else if (chatState->Name().Compare(buzz::QN_CS_COMPOSING) == 0)
        {
            jingleMsg.state = Origin::Chat::ChatStateComposing;
        }
        else if (chatState->Name().Compare(buzz::QN_CS_PAUSED) == 0)
        {
            jingleMsg.state = Origin::Chat::ChatStatePaused;
        }
    }

    // Don't treat chat state errors as chat state notifications
    if ((type.compare("error") == 0) || (jingleMsg.state == Origin::Chat::ChatStateNone) || (jingleMsg.state == Origin::Chat::ChatStateActive && jingleMsg.body.compare("") != 0))
    {
        SignalMessageReceived(jingleMsg);
    }
    else
    {
        SignalChatStateReceived(jingleMsg);
    }

    return true;
}

int ChatReceiveTask::ProcessStart()
{
    // We never queue anything so we are always blocked.
    return STATE_BLOCKED;
}

struct ChatSendData : public talk_base::MessageData
{
    ChatSendData(const JingleMessage& msg_)
    : msg(msg_)
    {
    }
    
    const JingleMessage msg;
};

int ChatSendTask::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void ChatSendTask::Send(const JingleMessage& message, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_CHATSEND, new ChatSendData(message));
}

void ChatSendTask::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;
    
    if (msg->message_id == Origin::Chat::MSG_CHATSEND)
    {
        const ChatSendData& data = *reinterpret_cast<const ChatSendData*>(msg->pdata);
        
        QScopedPointer<buzz::XmlElement> stanza(new buzz::XmlElement(buzz::QN_MESSAGE));
        stanza->AddAttr(buzz::QN_TO, data.msg.to.Str());
        stanza->AddAttr(buzz::QN_ID, talk_base::CreateRandomString(16));
        stanza->AddAttr(buzz::QN_TYPE, data.msg.type);

        buzz::XmlElement* body = new buzz::XmlElement(buzz::QN_BODY);
        body->SetBodyText(data.msg.body);
        stanza->AddElement(body);

        if (data.msg.subject.compare("") != 0)
        {
            buzz::XmlElement* subject = new buzz::XmlElement(buzz::QN_SUBJECT);
            subject->SetBodyText(data.msg.subject);
            stanza->AddElement(subject);
        }

        if (data.msg.thread.compare("") != 0)
        {
            buzz::XmlElement* thread = new buzz::XmlElement(buzz::QN_THREAD);
            thread->SetBodyText(data.msg.thread);
            stanza->AddElement(thread);    
        }

        buzz::XmlElement* chatState = NULL;
        switch (data.msg.state)
        {
        case Origin::Chat::ChatStateActive:
            {
                chatState = new buzz::XmlElement(buzz::QN_CS_ACTIVE);
            }
            break;
        case Origin::Chat::ChatStateInactive:
            {
                chatState = new buzz::XmlElement(buzz::QN_CS_INACTIVE);
            }
            break;
        case Origin::Chat::ChatStateGone:
            {
                chatState = new buzz::XmlElement(buzz::QN_CS_GONE);
            }
            break;
        case Origin::Chat::ChatStateComposing:
            {
                chatState = new buzz::XmlElement(buzz::QN_CS_COMPOSING);
            }
            break;
        case Origin::Chat::ChatStatePaused:
            {
                chatState = new buzz::XmlElement(buzz::QN_CS_PAUSED);
            }
            break;
        case Origin::Chat::ChatStateNone:
        default:
            // Don't want to send something for none states
            break;
        }
        
        if (chatState)
            stanza->AddElement(chatState);

        SendStanza(stanza.data());
        
        delete msg->pdata;
        msg->pdata = NULL;
    }
}

struct CarbonsRequestData : public talk_base::MessageData
{
    CarbonsRequestData(const bool enable_)
        : enable(enable_)
    {
    }

    const bool enable;
};

int CarbonsRequest::ProcessStart()
{
    const buzz::XmlElement* stanza = NextStanza();
    if (stanza == NULL)
        return STATE_BLOCKED;

    if (SendStanza(stanza) != buzz::XMPP_RETURN_OK)
        return STATE_ERROR;

    return STATE_START;
}

void CarbonsRequest::Send(bool enable, buzz::XmppThread& xmppThread)
{
    xmppThread.Post(this, Origin::Chat::MSG_CARBONS_REQUEST, new CarbonsRequestData(enable));
}

void CarbonsRequest::OnMessage(talk_base::Message* msg)
{
    if (GetState() != STATE_INIT && GetState() != STATE_START)
        return;

    if (!msg || !msg->pdata)
        return;

    if (msg->message_id == Origin::Chat::MSG_CARBONS_REQUEST)
    {
        const CarbonsRequestData& data = *reinterpret_cast<const CarbonsRequestData*>(msg->pdata);

        QScopedPointer<buzz::XmlElement> carbons(new buzz::XmlElement(buzz::QN_IQ));
        carbons->AddAttr(buzz::QN_TYPE, "set");

        buzz::XmlElement* request = NULL;
        if( data.enable )
        {
            carbons->AddAttr(buzz::QN_ID, "OriginCarbonsEnableRequest");
            buzz::QName QN_ENABLE("urn:xmpp:carbons:2", "enable");  
            request = new buzz::XmlElement(QN_ENABLE, true);
        }
        else
        {
            carbons->AddAttr(buzz::QN_ID, "OriginCarbonsDisableRequest");
            buzz::QName QN_DISABLE("urn:xmpp:carbons:2", "disable");
            request = new buzz::XmlElement(QN_DISABLE, true);
        }

        carbons->AddElement(request);

        SendStanza(carbons.data());
    }
}

bool CarbonsResponse::HandleStanza(const buzz::XmlElement* stanza)
{
    if (stanza->Name() != buzz::QN_IQ)
        return false;

    bool isEnable = stanza->Attr(buzz::QN_ID).compare("OriginCarbonsEnableRequest") == 0;
    bool isDisable = stanza->Attr(buzz::QN_ID).compare("OriginCarbonsDisableRequest") == 0;

    if ( !isEnable && !isDisable )
        return false;

    if (stanza->Attr(buzz::QN_TYPE).compare("result") != 0)
        return false;

    // TODO: handle error responses

    // carbons enabled
    SignalCarbonsResponded(isEnable);

    return true;
}

int CarbonsResponse::ProcessStart()
{
    // We never queue anything so we are always blocked.
    return STATE_BLOCKED;
}