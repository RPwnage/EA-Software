// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#include <eadp/foundation/Callback.h>
#include <eadp/foundation/Hub.h>
#include <eadp/foundation/Error.h>
#include <eadp/foundation/LoggingService.h>
#include "Serialization.h"

namespace eadp
{
namespace foundation
{
namespace Internal
{

class EADPSDK_API IRpcResponseDelegator
{
public:
    virtual ~IRpcResponseDelegator() = default;
    virtual Callback& getCallback() = 0;
    virtual void respond(RawBuffer data, const ErrorPtr& error) = 0;
};

}
}
}

