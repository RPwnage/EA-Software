#ifndef CHATGROUPTASK_H
#define CHATGROUPTASK_H

#include "talk/base/sigslot.h"
#include "talk/base/messagehandler.h"
#include "talk/xmpp/xmppengine.h"
#include "talk/xmpp/xmpptask.h"
#include "talk/xmpp/xmppthread.h"

#include <QString>
#include <QMap>

class ChatGroupRoomInviteRecvTask : public buzz::XmppTask
{
public:
    explicit ChatGroupRoomInviteRecvTask(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE) {}
    virtual int ProcessStart();

    sigslot::signal3<const buzz::Jid&, const QString&, const QString&> SignalInviteReceived;

protected:
    virtual bool HandleStanza(const buzz::XmlElement* stanza);
};

class ChatGroupRoomInviteSendTask : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit ChatGroupRoomInviteSendTask(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent)
      {
      }
    virtual ~ChatGroupRoomInviteSendTask() {}

    void Send(const buzz::Jid& toJid, const QString& groupId, const QString& channelId, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();

    void OnMessage(talk_base::Message* msg) override;
};


#endif // CHATGROUP_H
