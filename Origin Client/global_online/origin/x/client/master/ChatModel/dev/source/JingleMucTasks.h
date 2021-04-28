#ifndef MUCTASK_H
#define MUCTASK_H

#include "talk/base/sigslot.h"
#include "talk/base/messagehandler.h"
#include "talk/xmpp/xmppengine.h"
#include "talk/xmpp/xmpptask.h"
#include "talk/xmpp/xmppthread.h"

#include <QString>
#include <QMap>

class MucInviteRecvTask : public buzz::XmppTask
{
public:
    explicit MucInviteRecvTask(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE) {}
    virtual int ProcessStart();

    sigslot::signal3<const buzz::Jid&, const buzz::Jid&, const QString&> SignalInviteReceived;

protected:
    virtual bool HandleStanza(const buzz::XmlElement* stanza);
};

class MucInviteSendTask : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucInviteSendTask(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent)
      {
      }
    virtual ~MucInviteSendTask() {}

    void Send(const buzz::Jid& to, const buzz::Jid& room, const QString& reason, const QString& threadId, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();

    void OnMessage(talk_base::Message* msg) override;
};

class MucDeclineRecvTask : public buzz::XmppTask
{
public:
    explicit MucDeclineRecvTask(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE) {}
    virtual int ProcessStart();

    sigslot::signal3<const buzz::Jid&, const buzz::Jid&, const QString&> SignalDeclineReceived;

protected:
    virtual bool HandleStanza(const buzz::XmlElement* stanza);
};

class MucDeclineSendTask : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucDeclineSendTask(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent) {}
    virtual ~MucDeclineSendTask() {}

    void Send(const buzz::Jid& to, const buzz::Jid& room, const QString& reason, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class MucInstantRoomRequest : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucInstantRoomRequest(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent) {}
    virtual ~MucInstantRoomRequest() {}

    void Send(const buzz::Jid& room, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class MucConfigRequest : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucConfigRequest(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent) {}
    virtual ~MucConfigRequest() {}

    void Send(const buzz::Jid& room, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class MucConfigResponse : public buzz::XmppTask
{
public:
    explicit MucConfigResponse(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE) {}
    virtual int ProcessStart();

    sigslot::signal2<QMap<QString, QString>, const buzz::Jid&> SignalConfigReceived;

protected:
    virtual bool HandleStanza(const buzz::XmlElement* stanza);
};

class MucConfigSubmission : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucConfigSubmission(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent) {}
    virtual ~MucConfigSubmission() {}

    void Send(const buzz::Jid& room, const QMap<QString, QString>& config, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class MucRoomUnlocked : public buzz::XmppTask
{
public:
    explicit MucRoomUnlocked(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE) {}
    virtual int ProcessStart();

    sigslot::signal1<const buzz::Jid&> SignalRoomUnlocked;

protected:
    virtual bool HandleStanza(const buzz::XmlElement* stanza);
};

class MucGrantModerator : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucGrantModerator(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent) {}
    virtual ~MucGrantModerator() {}

    void Send(const buzz::Jid& room, const buzz::Jid& user, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class MucRevokeModerator : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucRevokeModerator(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent) {}
    virtual ~MucRevokeModerator() {}

    void Send(const buzz::Jid& room, const buzz::Jid& user, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class MucGrantAdmin : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucGrantAdmin(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent) {}
    virtual ~MucGrantAdmin() {}

    void Send(const buzz::Jid& room, const buzz::Jid& user, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class MucRevokeAdmin : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucRevokeAdmin(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent) {}
    virtual ~MucRevokeAdmin() {}

    void Send(const buzz::Jid& room, const buzz::Jid& user, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class MucGrantMembership : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucGrantMembership(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent) {}
    virtual ~MucGrantMembership() {}

    void Send(const buzz::Jid& room, const buzz::Jid& user, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class MucRevokeMembership : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucRevokeMembership(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent) {}
    virtual ~MucRevokeMembership() {}

    void Send(const buzz::Jid& room, const buzz::Jid& user, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class MucGrantVoice : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucGrantVoice(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent) {}
    virtual ~MucGrantVoice() {}

    void Send(const buzz::Jid& room, const buzz::Jid& user, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class MucRevokeVoice : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucRevokeVoice(buzz::XmppTaskParentInterface* parent)
      : buzz::XmppTask(parent) {}
    virtual ~MucRevokeVoice() {}

    void Send(const buzz::Jid& room, const buzz::Jid& user, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

class MucKickOccupant : public buzz::XmppTask, public talk_base::MessageHandler
{
public:
    explicit MucKickOccupant(buzz::XmppTaskParentInterface* parent)
        : buzz::XmppTask(parent) {}
    virtual ~MucKickOccupant() {}

    void Send(const buzz::Jid& room, const QString& userNickname, const QString& reason, buzz::XmppThread& xmppThread);

    virtual int ProcessStart();
    void OnMessage(talk_base::Message* msg) override;
};

#endif // MUCTASK_H
