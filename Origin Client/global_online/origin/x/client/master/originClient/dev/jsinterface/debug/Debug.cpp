// Copyright 2014 Electronic Arts Inc.

#include <jsinterface/debug/Debug.h>

#include <services/debug/DebugService.h>
#include <services/log/LogService.h>
#include <services/platform/PlatformService.h>

namespace Origin
{
namespace Client
{
namespace JsInterface
{

void Console::log_action(const QString& msg) const
{
    ORIGIN_LOG_ACTION << msg;
}

void Console::log_debug(const QString& msg) const
{
    ORIGIN_LOG_DEBUG << msg;
}

void Console::log_error(const QString& msg) const
{
    ORIGIN_LOG_ERROR << msg;
}

void Console::log_event(const QString& msg) const
{
    ORIGIN_LOG_EVENT << msg;
}

void Console::log_warning(const QString& msg) const
{
    ORIGIN_LOG_WARNING << msg;
}

}
}
}
