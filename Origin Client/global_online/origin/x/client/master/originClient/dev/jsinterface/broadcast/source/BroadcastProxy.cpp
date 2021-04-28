#include "BroadcastProxy.h"
#include "engine/igo/IGOController.h"
#include "engine/igo/IGOTwitch.h"
#include "services/debug/DebugService.h"
#if defined (ORIGIN_PC)
#include "services/platform/PlatformService.h"
#endif

namespace Origin
{
namespace Client
{
namespace JsInterface
{
#ifdef ORIGIN_PC
BroadcastProxy::BroadcastProxy(QObject* parent)
: QObject(parent)
, mIgoTwitch(NULL)
{
    mIgoTwitch = Engine::IGOController::instance()->twitch();
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastStartPending()), this, SIGNAL(broadcastStartPending()));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastErrorOccurred(int, int)), this, SIGNAL(broadcastErrorOccurred(int, int)));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastStarted(const QString&)), this, SIGNAL(broadcastStarted()));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastStopped()), this, SIGNAL(broadcastStopped()));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastStartStopError()), this, SIGNAL(broadcastStartStopError()));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastPermitted(Origin::Engine::IIGOCommandController::CallOrigin)), this, SIGNAL(broadcastPermitted()));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastUserNameChanged(const QString&)), this, SIGNAL(broadcastUserNameChanged(const QString&)));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastGameNameChanged(const QString&)), this, SIGNAL(broadcastGameNameChanged(const QString&)));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastChannelChanged(const QString&)), this, SIGNAL(broadcastChannelChanged(const QString&)));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastDurationChanged(int)), this, SIGNAL(broadcastDurationChanged(const int&)));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastStreamInfoChanged(int)), this, SIGNAL(broadcastStreamInfoChanged(const int&)));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastOptEncoderAvailable(bool)), this, SIGNAL(broadcastOptEncoderAvailable(const bool&)));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastStatusChanged(bool, bool, const QString&, const QString&)), this, SIGNAL(broadcastStatusChanged(bool, bool, const QString&, const QString&)));
    ORIGIN_VERIFY_CONNECT(mIgoTwitch, SIGNAL(broadcastingStateChanged(bool, bool)), this, SIGNAL(broadcastingStateChanged(bool, bool)));
}


bool BroadcastProxy::enabled() const
{
    bool broadcastingEnabled = false;
    // hide the broadcast button on mac
#if defined(ORIGIN_MAC)
    broadcastingEnabled = false;
#else // ORIGIN_PC
    if (Services::PlatformService::OSMajorVersion() > 5)
        broadcastingEnabled = true;
#endif
    return broadcastingEnabled;
}


void BroadcastProxy::disconnectAccount()
{
    if(mIgoTwitch)
    {
        mIgoTwitch->disconnectAccount();
    }
}


void BroadcastProxy::showLogin()
{
    if(mIgoTwitch)
    {
        // I hate myself for this
        Engine::IIGOCommandController::CallOrigin callOrigin = Engine::IGOController::instance()->twitchStartedFrom();

        mIgoTwitch->broadcastAccountLinkDialogOpen(callOrigin);
    }
}


void BroadcastProxy::connectAccount()
{

}


bool BroadcastProxy::attemptBroadcastStart()
{
    // I hate myself for this
    Engine::IIGOCommandController::CallOrigin callOrigin = Engine::IGOController::instance()->twitchStartedFrom();

    return mIgoTwitch ? mIgoTwitch->attemptBroadcastStart(callOrigin) : false;
}


bool BroadcastProxy::attemptBroadcastStop()
{
    // I hate myself for this
    Engine::IIGOCommandController::CallOrigin callOrigin = Engine::IGOController::instance()->twitchStartedFrom();

    return mIgoTwitch ? mIgoTwitch->attemptBroadcastStop(callOrigin) : false;
}


bool BroadcastProxy::isUserConnected() const
{
    const QString token = Services::readSetting(Services::SETTING_BROADCAST_TOKEN, Services::Session::SessionService::currentSession());
    return token.count();
}


bool BroadcastProxy::isBroadcasting() const
{
    return mIgoTwitch ? mIgoTwitch->isBroadcasting() : false;
}


bool BroadcastProxy::isBroadcastingPending() const
{
    return mIgoTwitch ? mIgoTwitch->isBroadcastingPending() : false;
}


bool BroadcastProxy::isBroadcastTokenValid() const
{
    return mIgoTwitch ? mIgoTwitch->isBroadcastTokenValid() : false;
}


bool BroadcastProxy::isBroadcastingBlocked(uint32_t id) const
{
    return mIgoTwitch ? mIgoTwitch->isBroadcastingBlocked(id) : false;
}


int BroadcastProxy::broadcastDuration() const
{
    return mIgoTwitch ? mIgoTwitch->broadcastDuration() : -1;
}


int BroadcastProxy::broadcastViewers() const
{
    return mIgoTwitch ? mIgoTwitch->broadcastViewers() : -1;
}


const QString BroadcastProxy::broadcastUserName() const
{
    return mIgoTwitch ? mIgoTwitch->broadcastUserName() : "";
}


const QString BroadcastProxy::broadcastGameName() const
{
    return mIgoTwitch ? mIgoTwitch->broadcastGameName() : "";
}


const QString BroadcastProxy::broadcastChannel() const
{
    return mIgoTwitch ? mIgoTwitch->broadcastChannel() : "";
}


bool BroadcastProxy::isBroadcastOptEncoderAvailable() const
{
    return mIgoTwitch ? mIgoTwitch->isBroadcastOptEncoderAvailable() : false;
}


const QString BroadcastProxy::currentBroadcastedProductId() const
{
    return mIgoTwitch ? mIgoTwitch->currentBroadcastedProductId() : "";
}
#endif
}
}
}