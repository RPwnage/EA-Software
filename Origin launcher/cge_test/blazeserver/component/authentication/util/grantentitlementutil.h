/*************************************************************************************************/
/*!
    \file grantentitlementutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_GRANTENTITLEMENTUTIL_H
#define BLAZE_AUTHENTICATION_GRANTENTITLEMENTUTIL_H

#include "authentication/authenticationimpl.h"

namespace Blaze
{

class InboundRpcConnection;

namespace Authentication
{

class AuthenticationSlaveImpl;

class GrantEntitlementUtil
{
    NON_COPYABLE(GrantEntitlementUtil);
public:
    GrantEntitlementUtil(const PeerInfo* peerInfo,
        const AuthenticationConfig::NucleusBypassURIList* bypassList = nullptr, NucleusBypassType bypassType = BYPASS_NONE);
    virtual ~GrantEntitlementUtil() {};

    BlazeRpcError grantEntitlementLegacy(AccountId accountId, PersonaId personaId,
        const char8_t* productId, const char8_t* entitlementTag,
        const char8_t* groupName, const char8_t* projectId,
        const char8_t* entitlementSource, const char8_t* productName,
        Blaze::Nucleus::EntitlementType::Code entitlementType = Blaze::Nucleus::EntitlementType::DEFAULT,
        Blaze::Nucleus::EntitlementStatus::Code entitlementStatus = Blaze::Nucleus::EntitlementStatus::ACTIVE, bool withPersona = true);

    BlazeRpcError grantEntitlement2(AccountId accountId, PersonaId personaId,
        GrantEntitlement2Request& request, GrantEntitlement2Response &response,
        const char8_t* entitlementSource, const char8_t* productName);

private:
    // Not owned memory
    const PeerInfo* mPeerInfo;
};

} // Authentication
} // Blaze
#endif // BLAZE_AUTHENTICATION_GRANTENTITLEMENTUTIL_H
