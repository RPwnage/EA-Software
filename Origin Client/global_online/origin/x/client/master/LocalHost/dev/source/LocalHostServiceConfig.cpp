#include "LocalHost/LocalHostServiceConfig.h"

#include <QString>

#include "LocalHost/LocalHostServiceHandler.h"

#include "LocalHost/LocalHostServices/VersionService.h"
#include "LocalHost/LocalHostServices/PingService.h"

namespace Origin
{
namespace Sdk
{
namespace LocalHost
{
    LocalHostServiceConfig::~LocalHostServiceConfig()
    {

    }

    quint16 LocalHostServiceConfig::getUnsecurePort()
    {
        return 3213;
    }

    quint16 LocalHostServiceConfig::getSecurePort()
    {
        return 3212;
    }

    void LocalHostServiceConfig::onStart()
    {
        // Do nothing
    }

    void LocalHostServiceConfig::onStop()
    {
        // Do nothing
    }

    void LocalHostServiceConfig::registerServices(LocalHostServiceHandler* handler)
    {
        //register all the services available for the local host

        handler->registerService("/ping", new PingService(handler));
        handler->registerService("/version", new VersionService(handler));
    }

    void LocalHostServiceConfig::deregisterServices(LocalHostServiceHandler* handler)
    {
        // Do nothing
    }
}
}
}