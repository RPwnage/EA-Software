#include "engine/social/SdkPresenceController.h"

#include <QMutexLocker>

#include "chat/Presence.h"
#include "engine/multiplayer/SessionInfo.h"
#include "engine/igo/IGOController.h"
#include "services/debug/DebugService.h"
#include "services/downloader/Common.h"

#include "ConnectedUserPresenceStack.h"

namespace Origin
{
namespace Engine
{
namespace Social
{
    SdkPresenceController::SdkPresenceController(ConnectedUserPresenceStack *presenceStack) :
        mPresenceStack(presenceStack)
    {
        QMetaObject::invokeMethod(this, "connectBroadcastSignals", Qt::QueuedConnection);
    }

    void SdkPresenceController::set(QString productId, Origin::Chat::OriginGameActivity gameActivity, QString status, QByteArray sessionData)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QString, productId), Q_ARG(Origin::Chat::OriginGameActivity, gameActivity), Q_ARG(QString, status), Q_ARG(QByteArray, sessionData));

        QMutexLocker locker(&mStateLock);
        
        mProductId = productId;
        mGameActivity = gameActivity.withBroadcastUrl(mBroadcastUrl);
        mSessionData = sessionData;
        mStatus = status;

        const Chat::Presence newPresence(Chat::Presence::Online, status, mGameActivity);
        mPresenceStack->setPresenceForPriority(ConnectedUserPresenceStack::SdkGamePriority, newPresence);
    }

    void SdkPresenceController::clearIfSetBy(QString setterProductId)
    {
        ASYNC_INVOKE_GUARD_ARGS(Q_ARG(QString, setterProductId));

        QMutexLocker locker(&mStateLock);

        if (mProductId != setterProductId)
        {
            return;
        }

        mProductId = QString();
        mGameActivity = Chat::OriginGameActivity();
        mSessionData = QByteArray();

        mPresenceStack->clearPresenceForPriority(ConnectedUserPresenceStack::SdkGamePriority);
    }
        
    MultiplayerInvite::SessionInfo SdkPresenceController::multiplayerSessionInfo() const
    {
        QMutexLocker locker(&mStateLock);

        if (mProductId.isNull() || (!mGameActivity.joinable() && !mGameActivity.joinable_invite_only()))
        {
            return MultiplayerInvite::SessionInfo();
        }

        return MultiplayerInvite::SessionInfo(mGameActivity.productId(), mGameActivity.multiplayerId(), mSessionData);
    }

    void SdkPresenceController::onBroadcastStarted(const QString& broadcastUrl)
    {
        if (mGameActivity.isNull())
            return;
        mBroadcastUrl = broadcastUrl;

        const Chat::OriginGameActivity newActivity = mPresenceStack->presenceForPriority(ConnectedUserPresenceStack::SdkGamePriority).gameActivity().withBroadcastUrl(mBroadcastUrl);
        const Chat::Presence inGamePresence(Chat::Presence::Online, newActivity.gameTitle(), newActivity);
        mPresenceStack->setPresenceForPriority(ConnectedUserPresenceStack::SdkGamePriority, inGamePresence);
    }

    void SdkPresenceController::onBroadcastStopped()
    {
        if (mGameActivity.isNull())
            return;
        mBroadcastUrl = "";

        const Chat::OriginGameActivity newActivity = mPresenceStack->presenceForPriority(ConnectedUserPresenceStack::SdkGamePriority).gameActivity().withBroadcastUrl(mBroadcastUrl);
        const Chat::Presence inGamePresence(Chat::Presence::Online, newActivity.gameTitle(), newActivity);
        mPresenceStack->setPresenceForPriority(ConnectedUserPresenceStack::SdkGamePriority, inGamePresence);
    }
        
    void SdkPresenceController::connectBroadcastSignals()
    {
        IGOController* igoController = IGOController::instance();
        ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(broadcastStarted(const QString&)), this, SLOT(onBroadcastStarted(const QString&)));
        ORIGIN_VERIFY_CONNECT(igoController, SIGNAL(broadcastStopped()), this, SLOT(onBroadcastStopped()));
    }
}
}
}
