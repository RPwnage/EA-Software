
#include "framework/blaze.h"
#include "tools/stress/instanceresolver.h"
#include "tools/stress/stressconnection.h"
#include "redirector/rpc/redirectorslave.h"
#include "redirector/tdf/redirectortypes.h"

namespace Blaze
{
namespace Stress
{

RedirectorInstanceResolver::RedirectorInstanceResolver(bool secure, const char8_t* redirectorAddress, const char8_t* serviceName, uint32_t frameSize)
    : mSecure(secure),
      mRedirectorAddress(redirectorAddress),
      mServiceName(serviceName),
      mConnection(NULL),
      mRdir(NULL),
	  mMaxFramesize(frameSize)
{
}

RedirectorInstanceResolver::~RedirectorInstanceResolver()
{
    delete mRdir;
    delete mConnection;
}

bool RedirectorInstanceResolver::resolve(ClientType clientType, InetAddress* addr)
{
    NameResolver::LookupJobId jobId;
    BlazeRpcError err =  gNameResolver->resolve(mRedirectorAddress, jobId);
    jobId = Blaze::NameResolver::INVALID_JOB_ID;
    if (err != ERR_OK)
    {
        char8_t rdirAddr[256];
        BLAZE_ERR_LOG(Log::CONTROLLER, "[OutboundConnectionManager:" << this << "].resolve: could not resolve "
            << "redirector server address: "<< mRedirectorAddress.toString(rdirAddr, sizeof(rdirAddr)));
        return false;
    }
    StaticInstanceResolver* resolver = BLAZE_NEW StaticInstanceResolver(mRedirectorAddress);
    if (mConnection == NULL)
    {
        mConnection = BLAZE_NEW StressConnection(-1, false, *resolver);
        mConnection->initialize("fire", "heat2", "heat2", mMaxFramesize);
        mRdir = BLAZE_NEW Redirector::RedirectorSlave(*mConnection);
    }

    if (!mConnection->connect(true))
    {
        delete mConnection;
        mConnection = NULL;
        delete mRdir;
        mRdir = NULL;
        BLAZE_ERR_LOG(Log::SYSTEM, "[RedirectorInstanceResolver:" << this << "]: failed to connect to " <<  mRedirectorAddress);
        return false;
    }

    Redirector::ServerInstanceRequest request;
    request.setName(mServiceName.c_str());
    request.setClientType(clientType);
    if (mSecure)
        request.setConnectionProfile("standardSecure_v4");
    else
        request.setConnectionProfile("standardInsecure_v4");

    request.setBlazeSDKVersion("N/A");
    request.setBlazeSDKBuildDate("N/A");
    request.setClientName("STRESS_Client");
    request.setClientPlatform(Blaze::pc);
    request.setClientSkuId("STRESS_Id_1");
    request.setClientVersion("STRESS_Version_1_1");
    request.setClientSkuId("STRESS_Id_1");
    request.setDirtySDKVersion("N/A");
    request.setEnvironment("stest");
    request.setClientLocale('enUS');
    request.setPlatform(EA_PLATFORM_NAME);

    Redirector::ServerInstanceInfo response;
    Redirector::ServerInstanceError error;
    BlazeRpcError rc = mRdir->getServerInstance(request, response, error);
    bool result = false;
    if (rc != Blaze::ERR_OK)
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[RedirectorInstanceResolver:" << this << "]: Failed to get instance from redirector( " <<  mRedirectorAddress << ").");
    }

    const Redirector::IpAddress* ipAddr = response.getAddress().getIpAddress();
    if (ipAddr != NULL)
    {
        InetAddress inetAddr(ipAddr->getIp(), ipAddr->getPort(), InetAddress::HOST);
        *addr = inetAddr;
        result = true;
    }
    
    mConnection->disconnect();
    return result;
}

} // namespace Stress
} // namespace Blaze
