#ifndef BLAZE_AUTHENTICATION_DECREMENTENTITLEMENTUSECOUNTUTIL_H
#define BLAZE_AUTHENTICATION_DECREMENTENTITLEMENTUSECOUNTUTIL_H

#include "authentication/util/hasentitlementutil.h"

namespace Blaze
{

class InboundRpcConnection;

namespace Authentication
{

class AuthenticationSlaveImpl;

class DecrementEntitlementUseCountUtil
{
public:
    DecrementEntitlementUseCountUtil(AuthenticationSlaveImpl &componentImpl, const PeerInfo* peerInfo, const ClientType &clientType)
      : mComponent(componentImpl),
        mPeerInfo(peerInfo),
        mGetEntitlementUtil(componentImpl, clientType)
    {
    }

    virtual ~DecrementEntitlementUseCountUtil() {}
    BlazeRpcError decrementEntitlementUseCount(const AccountId accountId, const DecrementUseCountRequest& request, uint32_t& consumedCount, uint32_t& remainedCount);

private:
    // memory owned by creator, don't free
    AuthenticationSlaveImpl &mComponent;

    const PeerInfo* mPeerInfo;

    // Owned memory
    HasEntitlementUtil mGetEntitlementUtil;
};

}
}
#endif //BLAZE_AUTHENTICATION_DECREMENTENTITLEMENTUSECOUNTUTIL_H

