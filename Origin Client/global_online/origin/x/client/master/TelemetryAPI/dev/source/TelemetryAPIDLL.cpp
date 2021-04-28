// TelemetryAPIDLL.cpp : Defines the entry point for the DLL application.
//

#include "TelemetryAPIDLL.h"
#include "EbisuTelemetryAPIConcrete.h"
#include "EbisuMetrics.h"
#include "GenericTelemetryElements.h"
#include "EAPackageDefines.h"
#include "TelemetryAPIManager.h"
#include "TelemetryConfig.h"

// EA core tech support.
#include <EATrace/EALog_imp.h>
#include <EATrace/EALogConfig.h>

#include "netconn.h"

#if defined(WIN32)
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#endif

namespace OriginTelemetry
{
    void init(const char* appPrefix)
    {
        TelemetryConfig::preemptErrorFileName(appPrefix);

        // Just get the instance will create the singleton
        // Not entirely necessary but good to get it setup early
        OriginTelemetry::TelemetryAPIManager::getInstance();
    }

    void release()
    {
        OriginTelemetry::TelemetryAPIManager::getInstance().shutdown();
    }
}


EbisuTelemetryAPI *GetTelemetryInterface()
{
    return OriginTelemetry::TelemetryAPIManager::getInstance().getTelemetryAPI();
}



