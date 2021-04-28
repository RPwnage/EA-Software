// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/Callback.h>

namespace eadp
{
namespace foundation
{

class IHub;

/*!
 * @brief Response interface for reporting the result of job execution
 *
 * Normally, the response instance will host the result data and the user callback for receiving the result data.
 * Hub.invokeCallbacks() is for invoking callback hosted by each response object.
 */
class EADPSDK_API IResponse
{
public:
    virtual ~IResponse() = default;
    virtual Callback& getCallback() = 0;
    virtual void invoke(IHub* hub) = 0;
    virtual String toString(Allocator& allocator) = 0;
};

}
}

