// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

class EADPSDK_API ServiceManager
{
public:
    explicit ServiceManager(const Allocator& allocator);
    ~ServiceManager() = default;

    void registerService(SharedPtr<IService> service, const char8_t* serviceId);
    void unregisterService(const char8_t* serviceId);

    SharedPtr<IService> getService(const char8_t* serviceId);

private:
    Allocator m_allocator;
    HashStringMap<WeakPtr<IService>> m_services;
};

}
}
}
