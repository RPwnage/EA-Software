#ifndef _MULTIPLAYER_INVITE_H
#define _MULTIPLAYER_INVITE_H

#include <QByteArray>
#include <QString>

#include "chat/Message.h"

#include "engine/multiplayer/SessionInfo.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Chat
{
    class OriginGameActivity;
}

namespace Engine
{
namespace MultiplayerInvite
{
    class ORIGIN_PLUGIN_API Invite
    {
    public:
        ///
        /// \brief Constructs a multiplayer invite from component fields
        ///
        /// \param  from           Sender of the multiplayer invite with an active session
        /// \param  to             Receiver of the multiplayer invite
        /// \param  sessionInfo    Game session information
        /// \param  message        Optional human-readable invite message
        ///
        Invite(const Chat::JabberID &from, const Chat::JabberID &to, const SessionInfo &sessionInfo, const QString &message = QString());

        /// \brief Constructs a multiplayer invite from a chat message
        explicit Invite(const Chat::Message &);

        /// \brief Constructs an invalid multiplayer invite
        Invite();

        /// \brief Returns true if this multiplayer invite is valid
        bool isValid() const { return mValid; }
        
        /// \brief Returns the Jabber ID of the invite sender
        Chat::JabberID from() const { return mFrom; }

        /// \brief Returns the Jabber ID of the invite receiver
        Chat::JabberID to() const { return mTo; }

        /// \brief Returns the game session information
        SessionInfo sessionInfo() const { return mSessionInfo; }

        /// \brief Returns the human-readable invite message or null if none was set
        QString message() const { return mMessage; }

        ///
        /// \brief Creates an XMPP message encoding the invite
        ///
        /// \param threadId  Thread ID for the message. See XEP-0201 for threading best practices
        ///
        Chat::Message toXmppMessage(const QString &threadId = Chat::Message::generateUniqueThreadId());

    private:
        bool mValid;

        Chat::JabberID mFrom;
        Chat::JabberID mTo;

        SessionInfo mSessionInfo;
        QString mMessage;
    };

    ///
    /// \brief Returns true if the passed message is an invite solicitation
    ///
    /// Invite solicitations are requests for an invite from another user
    ///
    ORIGIN_PLUGIN_API bool isInviteSolicitationMessage(const Chat::Message &);

    /// \brief Returns true if the passed message is an invite solicitation decline
    ORIGIN_PLUGIN_API bool isInviteSolicitationDeclineMessage(const Chat::Message &);

    /// \brief Returns true if the passed message is an invite message
    ORIGIN_PLUGIN_API bool isInviteMessage(const Chat::Message &);

    ///
    /// \brief Creates an invite solicitation message
    ///
    /// A new thread ID will be created to track the invite transaction
    ///
    /// \param from  User sending the solicitation
    /// \param to    User to solicit the invite from
    ///
    ORIGIN_PLUGIN_API Chat::Message createInviteSolicitationMessage(const Chat::JabberID &from, const Chat::JabberID &to);

    ///
    /// \brief Creates an invite solicitation decline message
    ///
    /// \param  solicitation  Original solicitation to decline
    ///
    ORIGIN_PLUGIN_API Chat::Message createInviteSolicitationDeclineMessage(const Chat::Message &solicitation);
}
}
}

#endif

