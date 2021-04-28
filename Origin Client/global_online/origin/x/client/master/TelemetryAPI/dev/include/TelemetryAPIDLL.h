#ifndef TELEMETRYAPI_H
#define TELEMETRYAPI_H

#include "TelemetryPluginAPI.h"
#include "EbisuTelemetryAPI.h"


extern TELEMETRY_PLUGIN_API EbisuTelemetryAPI* GetTelemetryInterface();

namespace OriginTelemetry
{
    extern TELEMETRY_PLUGIN_API void init(const char*);
    extern TELEMETRY_PLUGIN_API void release();
}

#endif