#include "XMPPImplEventAdapter.h"

#include <QScopedPointer>
#include <QStringList>

namespace Origin
{
namespace Chat
{
    XMPPImplEventAdapter::XMPPImplEventAdapter(QObject *parent) 
        : QObject(parent)
    {
    }

    void XMPPImplEventAdapter::Connected()
    {
        emit connected();
    }

    void XMPPImplEventAdapter::StreamError(const buzz::XmlElement& error)
    {
        StreamErrorProxy errorProxy(error);
        emit streamError(errorProxy);
    }

    void XMPPImplEventAdapter::Close(buzz::XmppEngine::Error error)
    {
        JingleErrorProxy errorProxy(error);
        emit jingleClose(errorProxy);
    }

    void XMPPImplEventAdapter::MessageReceived(const JingleMessage &msg)
    {
        emit messageReceived(msg);
    }

    void XMPPImplEventAdapter::ChatStateReceived(const JingleMessage &msg)
    {
        emit chatStateReceived(msg);
    }

    void XMPPImplEventAdapter::AddRequest(const buzz::Jid& from)
    {
        emit addRequest(from);
    }

    void XMPPImplEventAdapter::RosterUpdate(buzz::XmppRosterModule& roster)
    {
        XmppRosterModuleProxy proxy(roster);
        emit rosterUpdate(proxy);
    }

    void XMPPImplEventAdapter::PresenceUpdate(const buzz::XmppPresence* presence)
    {
        XmppPresenceProxy proxy(*presence);
        emit presenceUpdate(proxy);
    }

    /// \brief Event triggered when a user's capabilities change (typically when they're first set)
    void XMPPImplEventAdapter::CapabilitiesChanged(
        const buzz::Jid from,
        const buzz::Capabilities capsFromJingle)
    {
        XmppCapabilitiesProxy proxy;
        proxy.from = from;
        QStringList caps(QString::fromStdString(capsFromJingle).split(','));
        proxy.capabilities = caps;
        emit capabilitiesChanged(proxy);
    }

    void XMPPImplEventAdapter::ContactLimitReached(const buzz::Jid& from)
    {
        emit contactLimitReached(from);
    }

    void XMPPImplEventAdapter::BlockListUpdate(const QList<buzz::Jid>& blockList)
    {
        emit blockListUpdate(blockList);
    }

    void XMPPImplEventAdapter::RoomEntered(buzz::XmppChatroomModule& room, const QString& role)
    {
        XmppChatroomModuleProxy roomProxy(room);
        emit roomEntered(roomProxy, role);
    }

    void XMPPImplEventAdapter::RoomEnterFailed(buzz::XmppChatroomModule& room, const buzz::XmppChatroomEnteredStatus& status)
    {
        XmppChatroomModuleProxy roomProxy(room);
        emit roomEnterFailed(roomProxy, status);
    }

    void XMPPImplEventAdapter::RoomDestroyed(buzz::XmppChatroomModule& room)
    {
        XmppChatroomModuleProxy roomProxy(room);
        emit roomDestroyed(roomProxy);
    }

    void XMPPImplEventAdapter::RoomKicked(buzz::XmppChatroomModule& room)
    {
        XmppChatroomModuleProxy roomProxy(room);
        emit roomKicked(roomProxy);
    }

    void XMPPImplEventAdapter::RoomPasswordIncorrect(buzz::XmppChatroomModule& room)
    {
        XmppChatroomModuleProxy roomProxy(room);
        emit roomPasswordIncorrect(roomProxy);
    }

    void XMPPImplEventAdapter::RoomInvitationReceived(const buzz::Jid& inviter, const buzz::Jid& room, const QString& reason)
    {
        emit roomInvitationReceived(inviter, room, reason);
    }

    void XMPPImplEventAdapter::RoomUserJoined(const buzz::XmppChatroomMember& member)
    {
        XmppChatroomMemberProxy memberProxy(member);
        emit roomUserJoined(memberProxy);
    }

    void XMPPImplEventAdapter::RoomUserLeft(const buzz::XmppChatroomMember& member)
    {
        XmppChatroomMemberProxy memberProxy(member);
        emit roomUserLeft(memberProxy);
    }

    void XMPPImplEventAdapter::RoomUserChanged(const buzz::XmppChatroomMember& member)
    {
        XmppChatroomMemberProxy memberProxy(member);
        emit roomUserChanged(memberProxy);
    }

    void XMPPImplEventAdapter::RoomMessageReceived(const buzz::XmlElement&, const buzz::Jid&, const buzz::Jid&)
    {
        // not used
    }

    void XMPPImplEventAdapter::RoomDeclineReceived(const buzz::Jid& decliner, const buzz::Jid& room, const QString& reason)
    {
        emit roomDeclineReceived(decliner, room, reason);
    }
    
    void XMPPImplEventAdapter::PrivacyReceived()
    {
        emit privacyReceived();
    }
    
    void XMPPImplEventAdapter::ChatGroupRoomInviteReceived(const buzz::Jid& fromJid, const QString& groupId, const QString& channelId)
    {
        emit chatGroupRoomInviteReceived(fromJid, groupId, channelId);
    }

    StreamErrorProxy::StreamErrorProxy(buzz::XmlElement const& element)
        : resourceConflict(false)
    {
        const buzz::XmlElement* other = element.FirstNamed(buzz::QN_XSTREAM_SEE_OTHER_HOST);
        if (other)
        {
            errorBody = other->BodyText();
        }

        const buzz::XmlElement* conflict = element.FirstNamed(buzz::QN_XSTREAM_CONFLICT);
        if (conflict)
        {
            resourceConflict = true;
        }
    }
    
    Jid::Jid(buzz::Jid const& jid)
    :   node(jid.node()),
        domain(jid.domain()),
        resource(jid.resource())
    {
    }

    Jid& Jid::operator=(buzz::Jid const& jid)
    {
        node = jid.node();
        domain = jid.domain();
        resource = jid.resource();
        return *this;
    }
    
    XmppPresenceProxy::XmppPresenceProxy(buzz::XmppPresence const& presence)
    : jid(presence.jid())
    , status()
    , show(XMPP_PRESENCE_DEFAULT)
    , activityCategory()
    , activityInstance()
    , activityBody()
    {
        available = (XmppPresenceAvailable)presence.available();
        
        if (available != XMPP_PRESENCE_UNAVAILABLE)
        {
            status = presence.status();
            show = (XmppPresenceShow)presence.presence_show();
            activityCategory = presence.activity().getCategory();
            activityInstance = presence.activity().getInstance();
            activityBody = presence.activity().getBody();
            capsHash = presence.caps_hash();
            capsNode = presence.caps_node();
            capsVerification = presence.caps_verification();
        }
    }
    
    XmppRosterModuleProxy::XmppRosterModuleProxy(buzz::XmppRosterModule& rosterModule)
    {
        size_t numContacts = rosterModule.GetRosterContactCount();
        for (size_t i = 0; i < numContacts; ++i)
        {
            buzz::XmppRosterContact const* contact = rosterModule.GetRosterContact(i);
            if (contact)
            {
                XmppRosterContactProxy contactProxy = {
                    Jid(contact->jid()),
                    contact->personaId(),
                    contact->eaid(),
                    contact->legacyUser(),
                    (XmppSubscriptionState)contact->subscription_state()
                };
                rosterContacts.push_back(contactProxy);
            }
        }
    }
    
    XmppChatroomModuleProxy::XmppChatroomModuleProxy(buzz::XmppChatroomModule& chatroomModule)
    : chatroom_jid(chatroomModule.chatroom_jid())
    , nickname(chatroomModule.nickname())
    , reason(chatroomModule.reason())
    , destroyRoom(chatroomModule.getDestroyRoom())
    , destroyRoomReceiver(chatroomModule.getDestroyRoomReceiver())
    {
        buzz::XmppChatroomMemberEnumerator* enumerator;
        buzz::XmppReturnStatus status = chatroomModule.CreateMemberEnumerator(&enumerator);
        if (status != buzz::XMPP_RETURN_OK)
            return;
        QScopedPointer<buzz::XmppChatroomMemberEnumerator> autoDestructor(enumerator);

        // Move from the beginning
        for (enumerator->Next(); !enumerator->IsAfterEnd(); enumerator->Next())
        {
            buzz::XmppChatroomMember* member = enumerator->current();
            
            XmppChatroomMemberProxy memberProxy(*member);
            members.push_back(memberProxy);
        }
    }

    XmppChatroomMemberProxy::XmppChatroomMemberProxy(buzz::XmppChatroomMember const& chatroomMember)
        : member_jid(chatroomMember.member_jid())
        , full_jid(chatroomMember.full_jid())
        , presence(*chatroomMember.presence())
        , name(chatroomMember.name())
        , role(chatroomMember.presence()->role())
    {

    }

    JingleErrorProxy::JingleErrorProxy(int err)
    {
        error = static_cast<JingleError>(err);
    }

}
}