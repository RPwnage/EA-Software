/*************************************************************************************************/
/*!
    \file listentitlementsutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_GETENTITLEMENTLISTUTIL_H
#define BLAZE_AUTHENTICATION_GETENTITLEMENTLISTUTIL_H

#include "authentication/authenticationimpl.h"
#include "framework/oauth/nucleus/error_codes.h"
#include "authentication/tdf/authentication.h"

namespace Blaze
{

namespace NucleusIdentity
{
    class NucleusIdentitySlave;
}

namespace Authentication
{

class ListEntitlementsUtil
{
public:
    ListEntitlementsUtil();
    virtual ~ListEntitlementsUtil() {};

    BlazeRpcError listEntitlements2ForPersona(PersonaId personaId, ListPersonaEntitlements2Request& request, Entitlements::EntitlementList& resultList, const Command* callingCommand = nullptr);
    BlazeRpcError listEntitlements2ForUser(AccountId accountId, ListUserEntitlements2Request& request, Entitlements::EntitlementList& resultList, const Command* callingCommand = nullptr);

    BlazeRpcError listEntitlementsForPersona(PersonaId personaId, const GroupNameList& groupNameList, uint16_t pageSize, uint16_t pageNo,
        Entitlements::EntitlementList& resultList, const Command* callingCommand = nullptr);
    BlazeRpcError listEntitlementsForUser(AccountId accountId, const GroupNameList& groupNameList, uint16_t pageSize, uint16_t pageNo,
        Entitlements::EntitlementList& resultList, const Command* callingCommand = nullptr, bool hasAuthorizedPersona = true);

private:
    // helper
    BlazeRpcError fetchAuthToken(NucleusIdentity::GetEntitlementsRequest& req, PersonaId personaId, AccountId accountId) const;

    // Not owned memory
    NucleusIdentity::NucleusIdentitySlave *mNucleusIdentityComp;
};

} // Authentication
} // Blaze

#endif // BLAZE_AUTHENTICATION_GETENTITLEMENTLISTUTIL_H

