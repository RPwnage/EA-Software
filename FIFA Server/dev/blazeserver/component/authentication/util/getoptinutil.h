/*************************************************************************************************/
/*!
    \file getoptinutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_GETOPTINUTIL_H
#define BLAZE_AUTHENTICATION_GETOPTINUTIL_H

namespace Blaze
{

class InboundRpcConnection;

namespace Authentication
{

class AuthenticationSlaveImpl;
class OptInValue;

class GetOptInUtil
{
    NON_COPYABLE(GetOptInUtil);
public:
    GetOptInUtil(AuthenticationSlaveImpl &componentImpl, const PeerInfo* peerInfo, const ClientType &clientType);
    virtual ~GetOptInUtil() {};

    BlazeRpcError getOptIn(AccountId accountId, const char8_t* optInName, OptInValue& optInValue);

private:
    // memory owned by creator, don't free
    AuthenticationSlaveImpl &mComponent;
    const PeerInfo* mPeerInfo;
};

} // Authentication
} // Blaze

#endif // BLAZE_AUTHENTICATION_GETOPTINUTIL_H

