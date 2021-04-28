#ifndef _MULTIPLAYER_INVITECONTROLLER_H
#define _MULTIPLAYER_INVITECONTROLLER_H

#include <QObject>
#include <QHash>
#include <QMutex>
#include <QByteArray>

#include "chat/JabberID.h"
#include "chat/MessageFilter.h"

#include "engine/login/User.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Chat
{
    class Connection;
}

namespace Engine
{

namespace Social
{
    class SdkPresenceController;
}

namespace MultiplayerInvite
{

class Invite;
class JoinOperation;

///
/// \brief Handles sending and receiving invites to multiplayer game sessions
///
/// This class is thread safe
///
class ORIGIN_PLUGIN_API InviteController : public QObject, public Chat::MessageFilter
{
    Q_OBJECT
    friend class JoinOperation;
public:
    ///
    /// \brief Constructs a new InviteController
    ///
    /// Before InviteController is used it should be installed using Chat::Connection::installIncomingMessageFilter
    ///
    /// \param user            User to manage invites for
    ///
    explicit InviteController(UserRef user);
    ~InviteController();

    ///
    /// \brief Asynchronously attempts to join a user's remote multiplayer game session
    ///
    /// This will fail if there is an active join operation in progress. 
    ///
    /// \param  host  Jabber ID with a multiplayer game session to join
    /// \return JoinOperation representing the operation or NULL if there was already a join operation in progress. The
    ///         caller has ownership of this object.
    ///
    /// \sa activeJoinOperation()
    /// 
    JoinOperation *joinRemoteSession(const Chat::JabberID &host);

    ///
    /// \brief Invites a user to a local multiplayer game session
    ///
    /// \param guest  User to invite to the local multiplayer game session
    /// \return True if an invite was sent, false otherwise
    ///
    /// \sa activeJoinOperation()
    ///
    bool inviteToLocalSession(const Chat::JabberID &guest)
    {
        return inviteToLocalSession(guest, QString());
    }

    ///
    /// \brief Returns the active join operation of NULL if none is active
    ///
    JoinOperation* activeJoinOperation() const;

signals:
    ///
    /// \brief Emitted whenever the connected chat user has been invited to a remote user's multiplayer game session
    ///
    void invitedToRemoteSession(const Origin::Engine::MultiplayerInvite::Invite &);

    ///
    /// \brief Emitted whenever the connected chat user has been invited to a remote user's multiplayer game session
    ///
    void inviteReceivedFrom(const Origin::Chat::JabberID &);

    ///
    /// \brief Emitted whenever the connected chat user invites another user to the local multiplayer game session
    ///
    /// Hidden mutliplayer invites are sent whenever a remote user attempts to join the local multiplayer session. 
    /// These hidden invites will not cause this signal to be fired.
    ///
    void sentInviteToLocalSession(const Origin::Engine::MultiplayerInvite::Invite &);

    ///
    /// \brief Emitted when we want a game session sent to the currently running game via the Origin SDK
    ///
    /// This should only be used by XMPP_ServiceArea. This is only a signal for OriginLegacyApp layering purposes
    ///
    /// \param  invite   Invite containing the relevant session information
    /// \param  initial  True if the game was launched to accept this invite, false otherwise
    ///
    void sendSessionToConnectedGame(const Origin::Engine::MultiplayerInvite::Invite &invite, bool initial);

protected:
    bool filterMessage(const Chat::Message &);
    
    bool inviteToLocalSession(const Chat::JabberID &guest, const QString &solicitationThread);

private slots:
    void joinOperationFinished();

private:
    UserWRef mUser;
    
    mutable QMutex mActiveJoinOperationLock;
    JoinOperation *mActiveJoinOperation;
};

}
}
}

#endif

