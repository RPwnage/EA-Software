/*************************************************************************************************/
/*!
    \file hasentitlementutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_HASENTITLEMENTUTIL_H
#define BLAZE_AUTHENTICATION_HASENTITLEMENTUTIL_H

#include "authentication/tdf/authentication.h"
#include "proxycomponent/nucleusidentity/tdf/nucleusidentity.h"

namespace Blaze
{
namespace Authentication
{
class AuthenticationSlaveImpl;

class HasEntitlementUtil
{
public:
    HasEntitlementUtil(AuthenticationSlaveImpl &componentImpl, const ClientType &clientType)
        : mComponent(componentImpl), mIsSuccess(false)
    {}

    virtual ~HasEntitlementUtil() {}

    BlazeRpcError hasEntitlement(AccountId accountId, const char8_t* productId, const char8_t* entitlementTag, const GroupNameList& groupNameList, const char8_t* projectId, bool autoGrantEnabled,
        Blaze::Nucleus::EntitlementType::Code entitlementType = Blaze::Nucleus::EntitlementType::UNKNOWN, Blaze::Nucleus::EntitlementStatus::Code entitlementStatus = Blaze::Nucleus::EntitlementStatus::ACTIVE,
        Blaze::Nucleus::EntitlementSearchType::Type searchType = Blaze::Nucleus::EntitlementSearchType::USER, PersonaId personaId = 0, const Command* callingCommand = nullptr,
        const char8_t* accessToken = nullptr, const char8_t* clientId = nullptr);

    BlazeRpcError getUseCount(AccountId accountId, const char8_t* productId, const char8_t* entitlementTag, const char8_t* groupName, const char8_t* projectId,
        uint32_t& useCount,Blaze::Nucleus::EntitlementType::Code entitlementType =Blaze::Nucleus::EntitlementType::UNKNOWN, Blaze::Nucleus::EntitlementStatus::Code entitlementStatus = Blaze::Nucleus::EntitlementStatus::ACTIVE);

    const Entitlements::EntitlementList& getEntitlementsList() { return mEntitlementsList; }

    bool isSuccess() const { return mIsSuccess; }
    const NucleusIdentity::ErrorDetails& getError() const { return mError.getError(); }

protected:
    BlazeRpcError getEntitlements(AccountId accountId, const char8_t* productId, const char8_t* entitlementTag, const GroupNameList& groupNameList, const char8_t* projectId,
       Blaze::Nucleus::EntitlementType::Code entitlementType, Blaze::Nucleus::EntitlementStatus::Code entitlementStatus, Blaze::Nucleus::EntitlementSearchType::Type searchType, PersonaId personaId,
        NucleusIdentity::GetEntitlementsResponse &response, const Command* callingCommand = nullptr, const char8_t* userAccessToken = nullptr, const char8_t* clientId = nullptr);

private:
    // memory owned by creator, don't free
    AuthenticationSlaveImpl &mComponent;

    // Owned memory
    bool mIsSuccess;
    NucleusIdentity::IdentityError mError;

    Entitlements::EntitlementList mEntitlementsList;
};

} // Authentication
} // Blaze

#endif // BLAZE_AUTHENTICATION_HASENTITLEMENTUTIL_H

