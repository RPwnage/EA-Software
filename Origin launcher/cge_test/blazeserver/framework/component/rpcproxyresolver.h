/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_RPCPROXYRESOLVER_H
#define BLAZE_RPCPROXYRESOLVER_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class InetAddress;
class RpcProxySender;

class RpcProxyResolver
{
public:
    virtual ~RpcProxyResolver() { }

    virtual RpcProxySender* getProxySender(const InetAddress& addr) = 0;
    virtual RpcProxySender* getProxySender(InstanceId instanceId) = 0;
};

} // namespace Blaze

#endif // BLAZE_RPCPROXYRESOLVER_H

