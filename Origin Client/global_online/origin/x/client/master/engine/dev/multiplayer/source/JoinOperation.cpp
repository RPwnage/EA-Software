#include "engine/multiplayer/JoinOperation.h"

#include <QTimer>

#include "services/debug/DebugService.h"

#include "chat/OriginConnection.h"
#include "chat/ConnectedUser.h"

#include "engine/multiplayer/Invite.h"
#include "engine/multiplayer/InviteController.h"

#include "engine/content/ContentController.h"
#include "engine/social/SocialController.h"
#include "engine/content/PlayFlow.h"


namespace
{
    using namespace Origin::Engine;

    // The number of milliseconds before we time out an invite solicitation
    const int InviteSolicitationTimeoutMs = 60 * 1000;

    bool isMultiplayerCompatible(Content::EntitlementRef entRef, const MultiplayerInvite::SessionInfo &session)
    {
        if (entRef->contentConfiguration()->productId() == session.productId())
        {
            // Exact match
            return true;
        }

        if (entRef->contentConfiguration()->multiplayerId() == session.multiplayerId())
        {
            // Multiplayer compatible
            return true;
        }

        // Not compatible
        return false;
    }
}

namespace Origin
{
namespace Engine
{
namespace MultiplayerInvite
{

JoinOperation::JoinOperation(UserRef user, const Chat::JabberID &jabberId) :
    mUser(user),
    mJabberId(jabberId),
    mWaitingForPlayingGameExit(false)
{
    Chat::Connection *chatConnection = user->socialControllerInstance()->chatConnection();

    // Create the solicitation message
    Chat::Message solicitation = createInviteSolicitationMessage(chatConnection->currentUser()->jabberId(), jabberId);

    // Send it out
    chatConnection->sendMessage(solicitation);

    // Track our thread ID
    mThreadId = solicitation.threadId();

    // Time ourselves out if we don't get a response in time
    mSolicitationTimeoutTimer = new QTimer(this);
    ORIGIN_VERIFY_CONNECT(mSolicitationTimeoutTimer, SIGNAL(timeout()), this, SLOT(remoteTimeout()));

    mSolicitationTimeoutTimer->start(InviteSolicitationTimeoutMs);
}

void JoinOperation::remoteTimeout()
{
    abort(RemoteTimeoutFailure);
}

void JoinOperation::abort()
{
    abort(AbortedFailure);
}
    
void JoinOperation::abort(FailureReason reason)
{
    emit failed(reason, mInvite.sessionInfo().productId());
    emit finished();

    disconnect();
}
    
void JoinOperation::sendSessionDataAndSucceed(bool initial)
{
    Engine::UserRef user = mUser.toStrongRef();

    if (!user)
    {
        abort();
    }
    else
    {
        emit user->socialControllerInstance()->multiplayerInvites()->sendSessionToConnectedGame(mInvite, initial);

        emit succeeded();
        emit finished();
    }
}

void JoinOperation::receivedInvite(const Invite &invite)
{
    // We can't time out now
    delete mSolicitationTimeoutTimer;
    mSolicitationTimeoutTimer = NULL;

    mInvite = invite;

    findEligibleEntitlement();
}

void JoinOperation::findEligibleEntitlement()
{
    // Make sure we have the user
    UserRef user = mUser.toStrongRef(); 

    if (!user)
    {
        // User went away!
        abort();
        return;
    }

    const QList<Content::EntitlementRef> entitlements = user->contentControllerInstance()->entitlements();

    Content::EntitlementRef playableCompatEnt;
    bool playingIncompatibleEntitlement = false;
    bool ownUnplayableCompatEntitlement = false;

    for(QList<Content::EntitlementRef>::ConstIterator it = entitlements.constBegin();
        it != entitlements.constEnd();
        it++)
    {
        Content::EntitlementRef entRef = *it;
        const bool multiplayerCompatible = isMultiplayerCompatible(entRef, mInvite.sessionInfo()); 

        if (entRef->localContent()->playing())
        {
            if (multiplayerCompatible)
            {
                // We have a compatible instance running!
                // :TODO: need to update this condition to also check for alternate launcher software ID
                QString launchUrl = entRef->contentConfiguration()->gameLauncherUrl();
                // normal launch needs to send a message to through the SDK
                if (launchUrl.isEmpty())
                {
                    sendSessionDataAndSucceed(false);
                    return;
                }

                // Alternate launches will always launch the URL
                else
                {
                    startAltLaunch(entRef);
                    return;
                }
            }
            else
            {
                playingIncompatibleEntitlement = true;
                // Keep checking to see if a compatible entitlement is playing
            }
        }
        else if (multiplayerCompatible)
        {
            if (entRef->localContent()->playable())
            {
                playableCompatEnt = entRef;
            }
            else
            {
                ownUnplayableCompatEntitlement = true;
            }
        }
    }

    // Figure out if we have the correct entitlements
    if (!playableCompatEnt.isNull())
    {
        if (playingIncompatibleEntitlement)
        {
            if (!mWaitingForPlayingGameExit)
            {
                // Notify any curious parties that we're waiting for a game exit
                emit waitingForPlayingGameExit(playableCompatEnt);

                // A incompatible entitlement is playing
                // Start from the top the next time an entitlement finishes playing
                ORIGIN_VERIFY_CONNECT(
                        user->contentControllerInstance(), SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)),
                        this, SLOT(findEligibleEntitlement()));

                mWaitingForPlayingGameExit = true;
            }
        }
        else
        {
            // Launch the game
            // :TODO: need to update this condition to also check for alternate launcher software ID
            QString launchUrl = playableCompatEnt->contentConfiguration()->gameLauncherUrl();

            // normal launch needs to call this play directly
            if (launchUrl.isEmpty())
                playableCompatEnt->localContent()->play();
            // Alternate launches need to go through the PlayFlow()
            else
            {
                startAltLaunch(playableCompatEnt);
            }

            // Wait for it to connect to the SDK
            ORIGIN_VERIFY_CONNECT(
                    user->contentControllerInstance(), SIGNAL(sdkConnected(Origin::Engine::Content::EntitlementRef)),
                    this, SLOT(sdkConnected(Origin::Engine::Content::EntitlementRef)));
        }
    }
    else
    {
        if (ownUnplayableCompatEntitlement)
        {
            // We have the correct entitlement but it's not playable
            abort(UnplayableEntitlementFailure);
        }
        else
        {
            // We don't own the correct entitlement
            abort(NoOwnedEntitlementFailure);
        }
    }
}
    
void JoinOperation::sdkConnected(Content::EntitlementRef entRef)
{
    if (!isMultiplayerCompatible(entRef, mInvite.sessionInfo()))
    {
        // Not the right multiplayer ID
        return;
    }

    // If we send this too quickly the SDK might not actually get it
    QTimer::singleShot(5000, this, SLOT(sendInitialInvite()));    
}

void JoinOperation::sendInitialInvite()
{
    sendSessionDataAndSucceed(true);
}


void JoinOperation::startAltLaunch(Content::EntitlementRef entRef)
{
    if (!entRef)
        return;

    QSharedPointer<Content::PlayFlow> playFlow = entRef->localContent()->playFlow();
    if (playFlow)
    {
        // alt launch only works if the lauch parameter are empty
        playFlow->clearLaunchParameters();
        // find inviter nuclues ID to add as alt launch parameter
        QString altParams = QString("invitefrom="+ mInvite.from().node());
        playFlow->setAlternateLaunchInviteParameters(altParams);
        // We want to be able to always launch the URL and not be blocked
        // by the .exe playing at the time of the joinOperation.
        playFlow->beginFromJoin();

        // When playFlow launched the URL consider this joinOperation finished
        ORIGIN_VERIFY_CONNECT(playFlow.data(), SIGNAL(launchedAlternate(const QString&, const QString&, const QString&)), this, SIGNAL(succeeded()));
        ORIGIN_VERIFY_CONNECT(playFlow.data(), SIGNAL(launchedAlternate(const QString&, const QString&, const QString&)), this, SIGNAL(finished()));
    }
}
}
}
}
