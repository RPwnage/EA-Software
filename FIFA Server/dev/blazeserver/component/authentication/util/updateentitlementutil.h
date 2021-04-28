/*************************************************************************************************/
/*!
    \file updateentitlementutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_UPDATEENTITLEMENTUTIL_H
#define BLAZE_AUTHENTICATION_UPDATEENTITLEMENTUTIL_H

#include "framework/tdf/nucleuscodes.h"

namespace Blaze
{

class InboundRpcConnection;

namespace Authentication
{

class AuthenticationSlaveImpl;

class UpdateEntitlementUtil
{
    NON_COPYABLE(UpdateEntitlementUtil);
public:
    UpdateEntitlementUtil(AuthenticationSlaveImpl &componentImpl, const PeerInfo* peerInfo, const ClientType &clientType);
    virtual ~UpdateEntitlementUtil() {};

    // This function can be used by one user to modify another users entitlements.  As such, the caller of this
    // function is responsible for checking the correct Authorizations of the caller.
    // If the caller has permission to modify another users entitlements, the caller must grant superuser privilege
    // to the usersession using UserSession::SuperUserPermissionAutoPtr
    BlazeRpcError updateEntitlement(AccountId accountId, int64_t entitlementId, const char8_t* productName,
        const char8_t* useCount, const char8_t* terminationDate, uint32_t version,
        const Blaze::Nucleus::EntitlementStatus::Code status = Blaze::Nucleus::EntitlementStatus::UNKNOWN,
        const Blaze::Nucleus::StatusReason::Code statusReason = Blaze::Nucleus::StatusReason::UNKNOWN);

private:
    // memory owned by creator, don't free
    AuthenticationSlaveImpl &mComponent;
    const PeerInfo* mPeerInfo;
};

} // Authentication
} // Blaze

#endif // BLAZE_AUTHENTICATION_UPDATEENTITLEMENTUTIL_H

