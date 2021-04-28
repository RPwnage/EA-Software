#include "LocalHost/LocalHostOriginConfig.h"

#include <QString>

#include "LocalHost/LocalHostServiceHandler.h"

#include "LocalHost/LocalHost.h"
#include "LocalHost/LocalHostServices/PingService.h"
#include "LocalHost/LocalHostServices/VersionService.h"

#include "LocalHostServices/AuthenticationRequestStartService.h"
#include "LocalHostServices/AuthenticationRequestStartAuthcodeService.h"
#include "LocalHostServices/AuthenticationRequestStatusService.h"
#include "LocalHostServices/AuthenticationStatusService.h"
#include "LocalHostServices/OfferProgressService.h"
#include "LocalHostServices/OfferLaunchGameService.h"
#include "LocalHostServices/Origin2PassThroughService.h"
#include "LocalHostServices/Origin2StatusService.h"
#include "LocalHostServices/GameLaunchStatusService.h"
#include "LocalHostServices/PingServerService.h"

#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
namespace Sdk
{
namespace LocalHost
{
    LocalHostOriginConfig::~LocalHostOriginConfig()
    {

    }

    quint16 LocalHostOriginConfig::getUnsecurePort()
    {
        return 3215;
    }

    quint16 LocalHostOriginConfig::getSecurePort()
    {
        return 3214;
    }

    void LocalHostOriginConfig::onStart()
    {
        // If we are being stopped because the local host was disabled, we need to stop the service...
        bool localHostResponderEnabled = Origin::Services::readSetting(Origin::Services::SETTING_LOCALHOSTRESPONDERENABLED);
        if (localHostResponderEnabled)
        {
            ORIGIN_LOG_EVENT << "Localhost responder enabled.  Starting Web Helper service.";

            LocalHost::startResponderService();
        }
#if defined(ORIGIN_MAC)
        else
        {
            ORIGIN_LOG_EVENT << "Localhost responder disabled.  Stopping Web Helper service if running.";
            
            LocalHost::stopResponderService();
        }
#endif
    }

    void LocalHostOriginConfig::onStop()
    {
        // If we are being stopped because the local host was disabled, we need to stop the service...
        bool localHostResponderEnabled = Origin::Services::readSetting(Origin::Services::SETTING_LOCALHOSTRESPONDERENABLED);
        if (!localHostResponderEnabled)
        {
            ORIGIN_LOG_EVENT << "Localhost responder disabled.  Stopping Web Helper service if running.";
            
            LocalHost::stopResponderService();
        }
    }

    void LocalHostOriginConfig::registerServices(LocalHostServiceHandler* handler)
    {
        //register all the services available for the local host

        //deprecated services
        handler->registerService("/ping", new PingService(handler));
        handler->registerService("/authentication/requeststart/authcode/([^/]+)", new AuthenticationRequestStartAuthcodeService(handler));
        handler->registerService("/authentication/requeststart/authtoken/([^/]+)", new AuthenticationRequestStartService(handler));
        handler->registerService("/authentication/requeststatus/([^/]+)", new AuthenticationRequestStatusService(handler));
        handler->registerService("/authentication/status", new AuthenticationStatusService(handler));
        handler->registerService("/offer/progress", new OfferProgressService(handler));
        handler->registerService("/offer/launchgame", new OfferLaunchGameService(handler));

        //new services that will pass through to Origin2://
        handler->registerService("/store/open", new Origin2PassThroughService(handler));
        handler->registerService("/library/open", new Origin2PassThroughService(handler));
        handler->registerService("/game/launch", new Origin2PassThroughService(handler));
        handler->registerService("/game/download", new Origin2PassThroughService(handler));
        handler->registerService("/currenttab", new Origin2PassThroughService(handler));

        //new servers that are used only by local host for status
        handler->registerService("/currenttab/status/([^/]+)", new Origin2StatusService(handler));
        handler->registerService("/store/open/status/([^/]+)", new Origin2StatusService(handler));
        handler->registerService("/library/open/status/([^/]+)", new Origin2StatusService(handler));
        handler->registerService("/game/launch/status/([^/]+)", new GameLaunchStatusService(handler));
        handler->registerService("/game/status", new OfferProgressService(handler));
        handler->registerService("/ping/gameservers", new PingServerService(handler));
        handler->registerService("/version", new VersionService(handler));
    }

    void LocalHostOriginConfig::deregisterServices(LocalHostServiceHandler* handler)
    {
        // Do nothing
    }
}
}
}