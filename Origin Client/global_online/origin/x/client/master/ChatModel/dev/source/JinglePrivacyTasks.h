#ifndef JINGLE_PRIVACY_TASK_H
#define JINGLE_PRIVACY_TASK_H

#include "talk/base/sigslot.h"
#include "talk/base/messagehandler.h"
#include "talk/xmpp/xmppengine.h"
#include "talk/xmpp/xmpptask.h"
#include "talk/xmpp/xmppthread.h"
#include "talk/xmpp/jid.h"

#include <QString>
#include <QMap>

class BlockListRecv : public buzz::XmppTask
{
public:
    explicit BlockListRecv(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE) {}
    virtual int ProcessStart();

    sigslot::signal1<const QList<buzz::Jid>&> SignalBlockListReceived;

protected:
    virtual bool HandleStanza(const buzz::XmlElement* stanza);
};

class BlockListPush : public buzz::XmppTask
{
public:
    explicit BlockListPush(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE) {}
    virtual int ProcessStart();

    sigslot::signal0<> SignalBlockListPushReceived;

protected:
    virtual bool HandleStanza(const buzz::XmlElement* stanza);
};

class BlockListSend : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit BlockListSend(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent) {}
    virtual ~BlockListSend() {}

    void Send(const QList<buzz::Jid>& blockList, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class BlockListRequest : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit BlockListRequest(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent) {}
    virtual ~BlockListRequest() {}

    void Send(buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class SetPrivacyList : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit SetPrivacyList(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent, buzz::XmppEngine::HL_PEEK) {}
    virtual ~SetPrivacyList() {}

    void Send(const QString& list, buzz::XmppThread& xmppThread);
    sigslot::signal0<> SignalPrivacyReceived;
    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
protected:
    virtual bool HandleStanza(const buzz::XmlElement* stanza);
    std::string id;
};

#endif // JINGLE_PRIVACY_TASK_H
