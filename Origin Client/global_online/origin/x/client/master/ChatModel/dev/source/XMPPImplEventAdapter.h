#ifndef _CHATMODEL_XMPPIMPLEVENTADAPTER_H
#define _CHATMODEL_XMPPIMPLEVENTADAPTER_H

#include <QObject>
#include <QVector>

#include "XMPPImplTypes.h"
#include "JingleClient.h"
#include "JabberID.h"
#include "talk/xmllite/xmlelement.h"

namespace Origin
{
namespace Chat
{
    ///
    /// Proxies event callbacks from our implementation to Qt signals
    ///
    class XMPPImplEventAdapter : public QObject, public JingleEvents
    {
        Q_OBJECT
    public:
        XMPPImplEventAdapter(QObject *parent = NULL);

        virtual void Connected() override;
        virtual void StreamError(const buzz::XmlElement& error) override;
        virtual void Close(buzz::XmppEngine::Error error) override;

        virtual void MessageReceived(const JingleMessage &msg) override;
        virtual void ChatStateReceived(const JingleMessage &msg) override;
        virtual void AddRequest(const buzz::Jid& from) override;
        virtual void RosterUpdate(buzz::XmppRosterModule& roster) override;
        virtual void PresenceUpdate(const buzz::XmppPresence* presence) override;

        /// \brief Event triggered when a user's capabilities change (typically when they're first set)
        virtual void CapabilitiesChanged(
            const buzz::Jid from,
            const buzz::Capabilities caps);

        virtual void ContactLimitReached(const buzz::Jid& from) override;

        virtual void BlockListUpdate(const QList<buzz::Jid>& blockList) override;

        // MUC
        virtual void RoomEntered(buzz::XmppChatroomModule& room, const QString& role) override;
        virtual void RoomEnterFailed(buzz::XmppChatroomModule& room, const buzz::XmppChatroomEnteredStatus& status) override;
        virtual void RoomDestroyed(buzz::XmppChatroomModule& room) override;
        virtual void RoomKicked(buzz::XmppChatroomModule& room);
        virtual void RoomPasswordIncorrect(buzz::XmppChatroomModule& room) override;
        virtual void RoomInvitationReceived(const buzz::Jid& inviter, const buzz::Jid& room, const QString& reason) override;
        virtual void RoomUserJoined(const buzz::XmppChatroomMember& member) override;
        virtual void RoomUserLeft(const buzz::XmppChatroomMember& member) override;
        virtual void RoomUserChanged(const buzz::XmppChatroomMember& member) override;
        virtual void RoomMessageReceived(const buzz::XmlElement&, const buzz::Jid& room, const buzz::Jid& from);
        virtual void RoomDeclineReceived(const buzz::Jid& decliner, const buzz::Jid& room, const QString& reason) override;
        virtual void PrivacyReceived() override;

        // Group Chat
        virtual void ChatGroupRoomInviteReceived(const buzz::Jid& fromJid, const QString& groupId, const QString& channelId) override;

signals:
        void connected();
        void streamError(Origin::Chat::StreamErrorProxy errorProxy);
        void jingleClose(Origin::Chat::JingleErrorProxy error);

        void messageReceived(const JingleMessage &msg);
        void chatStateReceived(const JingleMessage &msg);
        void addRequest(buzz::Jid from);
        void rosterUpdate(Origin::Chat::XmppRosterModuleProxy roster);
        void presenceUpdate(Origin::Chat::XmppPresenceProxy presenceProxy);

        /// \brief Event triggered when a user's capabilities change (typically when they're first set)
        void capabilitiesChanged(Origin::Chat::XmppCapabilitiesProxy);

        void contactLimitReached(const buzz::Jid& from);

        void blockListUpdate(const QList<buzz::Jid>& blockList);

        void roomEntered(Origin::Chat::XmppChatroomModuleProxy room, const QString& role);
        void roomEnterFailed(Origin::Chat::XmppChatroomModuleProxy room, buzz::XmppChatroomEnteredStatus );
        void roomDestroyed(Origin::Chat::XmppChatroomModuleProxy room);
        void roomKicked(Origin::Chat::XmppChatroomModuleProxy room);
        void roomPasswordIncorrect(Origin::Chat::XmppChatroomModuleProxy room);
        void roomInvitationReceived(buzz::Jid inviter, buzz::Jid room, QString reason);
        void roomUserJoined(Origin::Chat::XmppChatroomMemberProxy member);
        void roomUserLeft(Origin::Chat::XmppChatroomMemberProxy member);
        void roomUserChanged(Origin::Chat::XmppChatroomMemberProxy member);
        void roomDeclineReceived(buzz::Jid decliner, buzz::Jid room, QString reason);
        void privacyReceived();

        void chatGroupRoomInviteReceived(buzz::Jid fromJid, QString groupId, QString channelId);

    };
}
}

#endif
