/*************************************************************************************************/
/*!
    \file setoptinutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_SETOPTINUTIL_H
#define BLAZE_AUTHENTICATION_SETOPTINUTIL_H

namespace Blaze
{

class InboundRpcConnection;

namespace Authentication
{

class AuthenticationSlaveImpl;

class SetOptInUtil
{
    NON_COPYABLE(SetOptInUtil);
public:
    SetOptInUtil(AuthenticationSlaveImpl &componentImpl, const PeerInfo* peerInfo, const ClientType &clientType);
    virtual ~SetOptInUtil() {};

    BlazeRpcError SetOptIn(AccountId accountId, const char8_t* optInName, const char8_t* nucleusSourceHeader, bool optInValue = true);

private:
    // memory owned by creator, don't free
    AuthenticationSlaveImpl &mComponent;
    const PeerInfo* mPeerInfo;
};

} // Authentication
} // Blaze

#endif // BLAZE_AUTHENTICATION_SETOPTINUTIL_H

