// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Hub.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

class EADPSDK_API IHttpClient 
{
public:
    virtual bool update() = 0;
    virtual void cleanup() = 0;

    virtual ~IHttpClient() = default;

    EA_NON_COPYABLE(IHttpClient);

protected:
    IHttpClient() = default;
};

}
}
}
