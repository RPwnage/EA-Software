#ifndef JINGLE_CHAT_TASK_H
#define JINGLE_CHAT_TASK_H

#include "ChatModelConstants.h"

#include "talk/base/sigslot.h"
#include "talk/base/messagehandler.h"
#include "talk/xmpp/xmppengine.h"
#include "talk/xmpp/xmpptask.h"
#include "talk/xmpp/xmppthread.h"
#include "talk/xmpp/jid.h"

#include <QString>
#include <QMap>

struct JingleMessage
{
    buzz::Jid from;
    buzz::Jid to;
    std::string subject;
    std::string thread;
    std::string body;
    std::string type;
    Origin::Chat::ChatState state;
};

class ChatReceiveTask : public buzz::XmppTask
{
public:
    explicit ChatReceiveTask(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE) {}
    virtual int ProcessStart();

    sigslot::signal1<const JingleMessage&> SignalMessageReceived;
    sigslot::signal1<const JingleMessage&> SignalChatStateReceived;

protected:
    virtual bool HandleStanza(const buzz::XmlElement* stanza);
};

class ChatSendTask : public buzz::XmppTask, public talk_base::MessageHandler
{
public:

    explicit ChatSendTask(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE)
        {
        }
    virtual ~ChatSendTask() {}

    virtual int ProcessStart();

    void Send(const JingleMessage& message, buzz::XmppThread& xmppThread);

    void OnMessage(talk_base::Message* msg) override;
};

class CarbonsRequest : public buzz::XmppTask, public talk_base::MessageHandler
{
public:

    explicit CarbonsRequest(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE)
    {
    }
    virtual ~CarbonsRequest() {}

    virtual int ProcessStart();

    void Send(bool enable, buzz::XmppThread& xmppThread);

    void OnMessage(talk_base::Message* msg) override;
};

class CarbonsResponse : public buzz::XmppTask
{
public:
    explicit CarbonsResponse(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE) {}
    virtual int ProcessStart();

    sigslot::signal1<bool> SignalCarbonsResponded;

protected:
    virtual bool HandleStanza(const buzz::XmlElement* stanza);
};

#endif // JINGLE_CHAT_TASK_H
