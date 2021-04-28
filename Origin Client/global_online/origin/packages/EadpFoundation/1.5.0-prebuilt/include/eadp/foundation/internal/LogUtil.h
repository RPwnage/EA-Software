// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#include <eadp/foundation/Config.h>
#include <eadp/foundation/Hub.h>
#include <eadp/foundation/internal/HttpResponse.h>

#pragma once

namespace eadp
{
namespace foundation
{
namespace Internal
{
    class EADPSDK_API LogUtil
    {
    public:
        static void LogHttpError(IHub* hub, const char* name, HttpResponse* httpResponse);
    };
}
}
}
