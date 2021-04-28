
#ifndef DIRTYSOCKDEV_H
#define DIRTYSOCKDEV_H

#include "Ignition/Ignition.h"

#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/shared/framework/locales.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/statsapi/lbapi.h"
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/netconn.h"

namespace Ignition
{

class DirtySockDev
    : public BlazeHubUiBuilder,
      public BlazeHubUiDriver::PrimaryLocalUserAuthenticationListener
{
    public:
        DirtySockDev();
        virtual ~DirtySockDev();
        
        virtual void onAuthenticatedPrimaryUser();
        virtual void onDeAuthenticatedPrimaryUser();

    private:

        PYRO_ACTION(DirtySockDev, RunFunc1);
        PYRO_ACTION(DirtySockDev, ConnectToTicker);
#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_XBSX)
        PYRO_ACTION(DirtySockDev, SpeechToText);
#endif
};

}

#endif
