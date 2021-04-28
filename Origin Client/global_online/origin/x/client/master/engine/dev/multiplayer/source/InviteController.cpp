#include "engine/multiplayer/InviteController.h"

#include <QMutexLocker>
#include <QMutableHashIterator>
#include <QTimer>

#include "services/debug/DebugService.h"

#include "chat/OriginConnection.h"
#include "chat/ConnectedUser.h"

#include "TelemetryAPIDLL.h"

#include "engine/multiplayer/Invite.h"
#include "engine/multiplayer/JoinOperation.h"

#include "engine/social/SocialController.h"
#include "engine/social/SdkPresenceController.h"

namespace Origin
{
namespace Engine
{
namespace MultiplayerInvite
{

InviteController::InviteController(UserRef user) :
    mUser(user),
    mActiveJoinOperation(NULL),
    mActiveJoinOperationLock(QMutex::Recursive)
{
}

InviteController::~InviteController()
{
    UserRef user = mUser.toStrongRef();

    if (mActiveJoinOperation)
    {
        mActiveJoinOperation->abort();
    }
}

JoinOperation *InviteController::joinRemoteSession(const Chat::JabberID &jabberId)
{
    QMutexLocker locker(&mActiveJoinOperationLock);

    if (mActiveJoinOperation)
    {
        // There's already an operation in progress
        return NULL;
    }

    UserRef user = mUser.toStrongRef();

    if (user.isNull())
    {
        return NULL;
    }
    
    // Create the join operation
    mActiveJoinOperation = new JoinOperation(user, jabberId);

    ORIGIN_VERIFY_CONNECT_EX(
            mActiveJoinOperation, SIGNAL(finished()),
            this, SLOT(joinOperationFinished()),
            Qt::DirectConnection);

    return mActiveJoinOperation;
}

bool InviteController::inviteToLocalSession(const Chat::JabberID &jabberId, const QString &solicitationThread)
{
    UserRef user = mUser.toStrongRef();

    if (!user)
    {
        return false;
    }

    Social::SocialController *socialController = user->socialControllerInstance();

    const MultiplayerInvite::SessionInfo sessionInfo = socialController->sdkPresence()->multiplayerSessionInfo();

    if (sessionInfo.isNull())
    {
        // No active multiplayer session
        return false;
    }

    Invite inviteMessage(socialController->chatConnection()->currentUser()->jabberId(), jabberId, sessionInfo);

    if (solicitationThread.isNull())
    {
        // This wasn't solicited - the user is sending out an invite themselves
        socialController->chatConnection()->sendMessage(inviteMessage.toXmppMessage());

        emit sentInviteToLocalSession(inviteMessage);
        QString productId = sessionInfo.productId();
        GetTelemetryInterface()->Metric_GAME_INVITE_SENT(productId.toUtf8().data());
    }
    else
    {
        // This is an automatic reply to an invite solicitation
        socialController->chatConnection()->sendMessage(inviteMessage.toXmppMessage(solicitationThread));
    }

    return true;
}
    
JoinOperation* InviteController::activeJoinOperation() const
{
    QMutexLocker locker(&mActiveJoinOperationLock);
    return mActiveJoinOperation;
}

bool InviteController::filterMessage(const Chat::Message &message)
{
    // It's safe to cache this because joinOps can only be deleted from the main thread which we're also
    // running on
    JoinOperation *activeJoinOp;
    {
        QMutexLocker locker(&mActiveJoinOperationLock);
        activeJoinOp = mActiveJoinOperation;
    }

    if (isInviteMessage(message))
    {
        // Parse the invite message
        Invite inviteMessage = Invite(message);

        if (activeJoinOp && (activeJoinOp->threadId() == message.threadId()))
        {
            // This was solicited, continue the join operation
            activeJoinOp->receivedInvite(inviteMessage);
            QString productId = inviteMessage.sessionInfo().productId();
            GetTelemetryInterface()->Metric_GAME_INVITE_ACCEPTED(productId.toUtf8().data());
        }
        else
        {
            // This was unsolicited. Allow the UI to act upon it
            emit invitedToRemoteSession(inviteMessage);
            emit inviteReceivedFrom(inviteMessage.from());
        }

        return true;
    }
    else if (isInviteSolicitationDeclineMessage(message))
    {
        if (activeJoinOp && (activeJoinOp->threadId() == message.threadId()))
        {
            activeJoinOp->abort(JoinOperation::RemoteDeclinedFailure);
            return true;
        }
    }
    else if (isInviteSolicitationMessage(message))
    {
        // Invite the user to our session if we have one
        if (!inviteToLocalSession(message.from(), message.threadId()))
        {
            UserRef user = mUser.toStrongRef();

            if (user)
            {
                // Couldn't invite; send a decline
                Chat::Connection *chatConnection = user->socialControllerInstance()->chatConnection();
                chatConnection->sendMessage(createInviteSolicitationDeclineMessage(message));
            }
        }
        
        return true;
    }

    return false;
}

void InviteController::joinOperationFinished()
{
    QMutexLocker locker(&mActiveJoinOperationLock);

    if (sender() == mActiveJoinOperation)
    {
        mActiveJoinOperation = NULL;
    }
}

}
}
}
