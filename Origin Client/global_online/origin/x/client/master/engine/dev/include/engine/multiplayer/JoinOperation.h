#ifndef _MULTIPLAYER_JOINOPERATION_H
#define _MULTIPLAYER_JOINOPERATION_H

#include "chat/JabberID.h"

#include "engine/login/User.h"
#include "engine/content/Entitlement.h"
#include "engine/multiplayer/Invite.h"
#include "services/plugin/PluginAPI.h"

class QTimer;

namespace Origin
{

namespace Chat
{
    class Connection;
}

namespace Engine
{
namespace MultiplayerInvite
{

class InviteController;

///
/// \brief Represents an asynchronous join operation 
///
/// Instances of JoinOperation can be deleted at any time from the main thread. From other threads only signal
/// connections to instances created from those threads are safe.
///
class ORIGIN_PLUGIN_API JoinOperation : public QObject
{
    Q_OBJECT
    friend class InviteController;
public:
    /// \brief Reason for a join operation to fail
    enum FailureReason
    {
        /// \brief Join operation was explicitly aborted locally
        AbortedFailure,
        /// \brief Join operation timed out waiting for the remote user to respond
        RemoteTimeoutFailure,
        /// \brief Remote user declined the join
        RemoteDeclinedFailure,
        /// \brief Local user doesn't own a compatible entitlement
        NoOwnedEntitlementFailure,
        /// \brief Local user has a compatible entitlement that isn't playing
        UnplayableEntitlementFailure
    };

    /// \brief Returns the jabberId with the remote game session we're attempting to join 
    Chat::JabberID jabberId() const
    {
        return mJabberId;
    }

public slots:
    ///
    /// \brief Aborts the join operation
    ///
    /// This will emit failed with AbortedFailure
    ///
    void abort();

signals:
    ///
    /// \brief Emitted when the join operation is waiting for the playing game to exit
    ///
    /// \param toLaunch  Entitlement to be launched once the currently playing game exits
    ///
    /// It is safe to call JoinOperation::abort() after this signal has been emitted to abort the join operation
    ///
    void waitingForPlayingGameExit(Origin::Engine::Content::EntitlementRef toLaunch);

    ///
    /// \brief Emitted if the join operation succeeds
    ///
    /// \sa finished()
    ///
    void succeeded();

    ///
    /// \brief Emitted if the join operation fails
    ///
    /// \param  reason     Reason the join operation failed
    /// \param  productId  Expected product ID for NoOwnedEntitlementFailure or UnplayableEntitlementFailure
    ///
    /// \sa finished()
    ///
    void failed(Origin::Engine::MultiplayerInvite::JoinOperation::FailureReason reason, const QString &productId);

    ///
    /// \brief Emitted if the join operation succeeds or fails 
    ///
    /// \sa succeeded()
    /// \sa failed()
    ///
    void finished();

protected slots:
    void remoteTimeout();
    
    void findEligibleEntitlement();
    void sdkConnected(Origin::Engine::Content::EntitlementRef);
    void sendInitialInvite();

protected:
    QString threadId()
    {
        return mThreadId;
    }

    void receivedInvite(const Invite &);

    void abort(FailureReason reason);
    void sendSessionDataAndSucceed(bool initial);

private:
    void startAltLaunch(Content::EntitlementRef ent);

    JoinOperation(UserRef user, const Chat::JabberID &jabberId); 

    QTimer *mSolicitationTimeoutTimer;

    UserWRef mUser;
    Chat::JabberID mJabberId;

    QString mThreadId;

    bool mWaitingForPlayingGameExit;
    Invite mInvite;
};

}
}
}

#endif

