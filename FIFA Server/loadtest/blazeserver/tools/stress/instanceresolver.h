
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
};

class StaticInstanceResolver : public InstanceResolver
{
public:
    StaticInstanceResolver(const InetAddress& addr) : mInstanceAddress(addr) { }

    virtual bool resolve(ClientType clientType, InetAddress* addr) { *addr = mInstanceAddress; return true;}

private:
    InetAddress mInstanceAddress;
};

class RedirectorInstanceResolver : public InstanceResolver
{
public:
    RedirectorInstanceResolver(bool secure, const char8_t* redirectorAddress, const char8_t* serviceName, uint32_t frameSize);
    virtual ~RedirectorInstanceResolver();

    virtual bool resolve(ClientType clientType, InetAddress* addr);

private:
    bool mSecure;
    InetAddress mRedirectorAddress;
    eastl::string mServiceName;

    StressConnection* mConnection;
    Redirector::RedirectorSlave* mRdir;
    uint32_t mMaxFramesize;
};

} // namespace Stress
} // namespace Blaze

#endif // INSTANCERESOLVER_H

