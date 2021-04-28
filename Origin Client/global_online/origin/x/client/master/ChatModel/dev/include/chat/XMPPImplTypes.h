#ifndef _CHATMODEL_XMPPIMPLTYPES
#define _CHATMODEL_XMPPIMPLTYPES

#include <vector>

#include <QStringList>

// Internal use only!
// Try not to include implementation headers
struct JingleMessage;
class JingleClient;

namespace buzz
{
    class XmppRosterModule;
    class XmppChatroomMember;
    class XmppChatroomModule;
    class XmppThread;
    class XmppClientSettings;
    class XmppPresence;
    class XmlElement;
    class Jid;
    class XmppRosterContact;
}


namespace Origin
{
namespace Chat
{
    struct Jid
    {
        Jid() {}
        explicit Jid(buzz::Jid const& jid);
        Jid& operator=(buzz::Jid const& jid);
        
        std::string node;
        std::string domain;
        std::string resource;
    };
    
    /// \brief Proxy for the stream error in the Jingle response.
    struct StreamErrorProxy
    {
        StreamErrorProxy()
            : resourceConflict(false)
        {
        }

        explicit StreamErrorProxy(buzz::XmlElement const& element);
        
        std::string errorBody;
        bool resourceConflict;
    };
    
    /// \brief Proxy for the XMPP presence in Jingle.
    enum XmppPresenceShow
    {
        XMPP_PRESENCE_CHAT = 0,
        XMPP_PRESENCE_DEFAULT = 1,
        XMPP_PRESENCE_AWAY = 2,
        XMPP_PRESENCE_XA = 3,
        XMPP_PRESENCE_DND = 4,
    };
    
    enum XmppPresenceAvailable {
        XMPP_PRESENCE_UNAVAILABLE = 0,
        XMPP_PRESENCE_AVAILABLE   = 1,
        XMPP_PRESENCE_ERROR       = 2,
    };
    
    struct XmppPresenceProxy
    {
        XmppPresenceProxy() {}
        explicit XmppPresenceProxy(buzz::XmppPresence const& presence);
    
        Jid jid;
        std::string status;
        XmppPresenceShow show;
        XmppPresenceAvailable available;
        std::string activityCategory;
        std::string activityInstance;
        std::string activityBody;
        std::string capsHash;
        std::string capsNode;
        std::string capsVerification;
    };
    
    /// \brief Proxy for the XMPP Roster Contact in Jingle.
    enum XmppSubscriptionState
    {
        XMPP_SUBSCRIPTION_NONE = 0,
        XMPP_SUBSCRIPTION_NONE_ASKED = 1,
        XMPP_SUBSCRIPTION_TO = 2,
        XMPP_SUBSCRIPTION_FROM = 3,
        XMPP_SUBSCRIPTION_FROM_ASKED = 4,
        XMPP_SUBSCRIPTION_BOTH = 5,
    };

    struct XmppRosterContactProxy
    {
        Jid jid;
        std::string personaId;
        std::string eaid;
        bool legacyUser;
        XmppSubscriptionState subscriptionState;
    };
    
    /// \brief Proxy for the XMPP Roster Module in Jingle.
    struct XmppRosterModuleProxy
    {
        XmppRosterModuleProxy() {}
        XmppRosterModuleProxy(buzz::XmppRosterModule& rosterModule);
        
        std::vector<XmppRosterContactProxy> rosterContacts;
    };
    
    /// \brief Proxy for the XMPP Chatroom Member in Jingle.
    struct XmppChatroomMemberProxy
    {
        XmppChatroomMemberProxy() {};
        XmppChatroomMemberProxy(buzz::XmppChatroomMember const& chatroomMember);

        Jid member_jid;
        Jid full_jid;
        XmppPresenceProxy presence;
        std::string name;
        std::string role;
    };
    
    /// \brief Proxy for the XMPP Chatroom Module in Jingle.
    struct XmppChatroomModuleProxy
    {
        XmppChatroomModuleProxy() {};
        XmppChatroomModuleProxy(buzz::XmppChatroomModule& chatroomModule);
        
        Jid chatroom_jid;
        std::string nickname;
        std::string reason;
        bool destroyRoom;
        bool destroyRoomReceiver;
        std::vector<XmppChatroomMemberProxy> members;
    };

    /// \brief Proxy for the XMPP XEP-0115 user capabilities in Jingle.
    typedef QStringList UserChatCapabilities;
    struct XmppCapabilitiesProxy
    {
        Jid from;
        UserChatCapabilities capabilities;
    };

    // This matches buzz::XmppEngine::Error
    enum JingleError
    {
        ERROR_NONE = 0,         //!< No error
        ERROR_XML,              //!< Malformed XML or encoding error
        ERROR_STREAM,           //!< XMPP stream error - see GetStreamError()
        ERROR_VERSION,          //!< XMPP version error
        ERROR_UNAUTHORIZED,     //!< User is not authorized (rejected credentials)
        ERROR_TLS,              //!< TLS could not be negotiated
        ERROR_AUTH,             //!< Authentication could not be negotiated
        ERROR_BIND,             //!< Resource or session binding could not be negotiated
        ERROR_CONNECTION_CLOSED,//!< Connection closed by output handler.
        ERROR_DOCUMENT_CLOSED,  //!< Closed by </stream:stream>
        ERROR_SOCKET,           //!< Socket error
        ERROR_NETWORK_TIMEOUT,  //!< Some sort of timeout (eg., we never got the roster)
        ERROR_MISSING_USERNAME  //!< User has a Google Account but no nickname
    };


    // Extended presence
    enum JingleExtendedPresence
    {
        // base presences from buzz
        JINGLE_PRESENCE_CHAT = 0,
        JINGLE_PRESENCE_DEFAULT = 1,
        JINGLE_PRESENCE_AWAY = 2,
        JINGLE_PRESENCE_XA = 3,
        JINGLE_PRESENCE_DND = 4,

        // added presences - currently used for Directed Presence / VOIP
        JINGLE_PRESENCE_VOIP_CALLING,
        JINGLE_PRESENCE_VOIP_HANGINGUP,
        JINGLE_PRESENCE_VOIP_JOIN,
        JINGLE_PRESENCE_VOIP_LEAVE
    };

    struct JingleErrorProxy
    {
        JingleErrorProxy() {};
        JingleErrorProxy(int err);

        JingleError error;
    };


    // VOIP Directed Presence Categories
    static const std::string DP_CALLING("calling");
    static const std::string DP_HANGINGUP("hanging-up");
    static const std::string DP_JOIN("join");
    static const std::string DP_LEAVE("left");

    // VOIP Directed Presence Instances
    static const std::string DP_VOICECHAT("voip");
}
}

#endif
