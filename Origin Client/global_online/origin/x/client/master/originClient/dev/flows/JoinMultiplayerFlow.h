#ifndef _JOINMULTIPLAYERFLOW_H
#define _JOINMULTIPLAYERFLOW_H

#include <QObject>

#include "chat/JabberID.h" 
#include "chat/XMPPUser.h" 
#include "engine/multiplayer/JoinOperation.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
    class OriginWindow;
}

namespace Engine
{
namespace Social
{
    class SocialController;
}
}

namespace Client
{

///
/// \brief Handles high-level logic and user feedback for joining a multiplayer game session
///
class ORIGIN_PLUGIN_API JoinMultiplayerFlow : public QObject
{
    Q_OBJECT
public:
	JoinMultiplayerFlow();
    ///
    /// Starts the flow
    ///
    /// \param  socialController Social controller to use for the join operation
    /// \param  jabberId         Jabber ID with the multiplayer session to join
    ///
    void start(Engine::Social::SocialController *socialController, const Chat::JabberID &jabberId);

signals:
    /// \brief Emitted once this flow finished
    void finished(bool success);

private slots:
    void attemptJoin();
    void finishWithFailure();

    void clearRetryDialog();
    void updateRetryDialogWithPresence(const Origin::Chat::Presence &);

    void operationFailed(Origin::Engine::MultiplayerInvite::JoinOperation::FailureReason reason, const QString &productId);
    void operationSucceeded();

    void waitingForPlayingGameExit(Origin::Engine::Content::EntitlementRef toLaunch);

private:
    void startRetryDialogPresenceChecks();

    Engine::Social::SocialController *mSocialController;
    Chat::XMPPUser *mXmppUser;

    UIToolkit::OriginWindow *mRetryDialog;
};


}
}

#endif

