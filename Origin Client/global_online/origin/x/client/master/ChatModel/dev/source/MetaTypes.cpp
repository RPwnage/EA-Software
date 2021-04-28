#include "MetaTypes.h"

#include <QMetaType> 
#include <QSet>
#include <QSharedPointer>

#include "XMPPImplTypes.h"
#include "JabberID.h"
#include "Presence.h"
#include "Message.h"
#include "ConnectedUser.h"
#include "Connection.h"
#include "SubscriptionState.h"
#include "XMPPImplEventAdapter.h"
#include "JingleClient.h"
#include "MucRoom.h"

namespace Origin
{
namespace Chat
{
    namespace MetaTypes
    {
        void registerMetaTypes()
        {
            qRegisterMetaType<JabberID>("Origin::Chat::JabberID");
            qRegisterMetaType<Presence>("Origin::Chat::Presence");
            qRegisterMetaType<Message>("Origin::Chat::Message");
            qRegisterMetaType<ConnectedUser::Visibility>("Origin::Chat::ConnectedUser::Visibility");
            qRegisterMetaType<Connection::DisconnectReason>("Origin::Chat::Connection::DisconnectReason");
            qRegisterMetaType<ConversableParticipant>("Origin::Chat::ConversableParticipant");
            qRegisterMetaType<SubscriptionState>("Origin::Chat::SubscriptionState");
            qRegisterMetaType<XmppPresenceProxy>("Origin::Chat::XmppPresenceProxy");
            qRegisterMetaType<XmppRosterModuleProxy>("Origin::Chat::XmppRosterModuleProxy");
            qRegisterMetaType<XmppRosterContactProxy>("Origin::Chat::XmppRosterContactProxy");
            qRegisterMetaType<XmppChatroomModuleProxy>("Origin::Chat::XmppChatroomModuleProxy");
            qRegisterMetaType<XmppChatroomMemberProxy>("Origin::Chat::XmppChatroomMemberProxy");
            qRegisterMetaType<StreamErrorProxy>("Origin::Chat::StreamErrorProxy");
            qRegisterMetaType<JingleErrorProxy>("Origin::Chat::JingleErrorProxy");
            qRegisterMetaType<XmppCapabilitiesProxy>("Origin::Chat::XmppCapabilitiesProxy");

            qRegisterMetaType<buzz::Jid>("buzz::Jid");
            qRegisterMetaType<JingleMessage>("JingleMessage");
            qRegisterMetaType<std::string>("std::string");
            qRegisterMetaType<MucRoom::DeactivationType>("Origin::Chat::MucRoom::DeactivationType");
            qRegisterMetaType<buzz::XmppChatroomEnteredStatus>("buzz::XmppChatroomEnteredStatus");

        }
    }
}
}
