
#ifndef INSTANCERESOLVER_H
#define INSTANCERESOLVER_H

#include "framework/tdf/userdefines.h"

namespace Blaze
{

namespace Redirector
{
    class RedirectorSlave;
}

namespace Stress
{

class StressConnection;

class InstanceResolver
{
public:
    virtual ~InstanceResolver() { }

    virtual bool resolve(ClientType clientType, InetAddress* addr) = 0;
    virtual const char8_t* getName() = 0;
};

class StaticInstanceResolver : public InstanceResolver
{
public:
    StaticInstanceResolver(const InetAddress& addr) : mInstanceAddress(addr) { }

    bool resolve(ClientType clientType, InetAddress* addr) override { *addr = mInstanceAddress; return true;}
    const char8_t* getName() override { return "StaticInstanceResolver"; }

private:
    InetAddress mInstanceAddress;
};

class RedirectorInstanceResolver : public InstanceResolver
{
public:
    RedirectorInstanceResolver(uint32_t id, bool secure, const char8_t* redirectorAddress, const char8_t* serviceName);
    ~RedirectorInstanceResolver() override;

    bool resolve(ClientType clientType, InetAddress* addr) override;
    const char8_t* getName() override { return "RedirectorInstanceResolver"; }

private:
    uint32_t mId;
    bool mSecure;
    InetAddress mRedirectorAddress;
    eastl::string mServiceName;

    StressConnection* mConnection;
    Redirector::RedirectorSlave* mRdir;
};

} // namespace Stress
} // namespace Blaze

#endif // INSTANCERESOLVER_H

