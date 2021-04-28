/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PAS_UTIL_H
#define BLAZE_PAS_UTIL_H

/*** Include files *******************************************************************************/
#include "proxycomponent/pas/tdf/pas.h"
#include "proxycomponent/pas/rpc/passlave.h"
#include "framework/component/rpcproxyresolver.h"
#include "framework/connection/outboundhttpconnectionmanager.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{       
    class InetAddress;
    class RpcProxyResolver;
    class OutboundHttpConnectionManager;

    class PasRpcProxyResolver : public RpcProxyResolver
    {
    public:
        PasRpcProxyResolver(const InetAddress& pasAddr, bool secure) :
            mHttpConnMgr(BLAZE_NEW OutboundHttpConnectionManager("PAS"))
        {
            mHttpConnMgr->initialize(pasAddr, 1, secure,
                true, OutboundHttpConnectionManager::SslVersion::SSLVERSION_DEFAULT, nullptr, nullptr,
                EA::TDF::TimeValue(), EA::TDF::TimeValue(), EA::TDF::TimeValue(), nullptr, Blaze::Component::INVALID_COMPONENT_ID, Blaze::HttpStatusCodeRangeList(),
                false);
        }

        virtual RpcProxySender* getProxySender(const InetAddress& addr)
        {
            return mHttpConnMgr.get();
        }

        virtual RpcProxySender* getProxySender(InstanceId instanceId) { return nullptr; } // Force send by address

    private:
        HttpConnectionManagerPtr mHttpConnMgr;
    };

    class PasUtil
    {
    NON_COPYABLE(PasUtil);
    public:
        PasUtil(); 
        ~PasUtil();
        bool initialize(const char8_t* serviceName, const char8_t* instanceName, const char8_t* pasAddr);
        bool getBasePort(uint32_t numberOfPorts, uint16_t& basePort);

    private:

        PasRpcProxyResolver* mResolver;
        PAS::PASSlave* mPasSlave;
        PAS::PortAssignment mRequest;
        eastl::string mFilename;

    }; 

} // Blaze

#endif // BLAZE_PAS_UTIL_H

