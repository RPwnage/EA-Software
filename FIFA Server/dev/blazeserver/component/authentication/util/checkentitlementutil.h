#ifndef BLAZE_AUTHENTICATION_CHECKENTITLEMENTUTIL_H
#define BLAZE_AUTHENTICATION_CHECKENTITLEMENTUTIL_H

#include "authentication/tdf/authentication_server.h"

#include "authentication/util/hasentitlementutil.h"

namespace Blaze
{
namespace Authentication
{

class AuthenticationSlaveImpl;

class CheckEntitlementUtil
{
public:
    CheckEntitlementUtil(AuthenticationSlaveImpl &componentImpl, const ClientType &clientType)
      : mComponent(componentImpl),
        mHasEntitlementUtil(componentImpl, clientType),
        mSoftCheck(false)
    {}

    virtual ~CheckEntitlementUtil() {};

    BlazeRpcError checkEntitlement(AccountId accountId, const char8_t* productId, const char8_t* entitlementTag,
        const GroupNameList& groupNameList, Blaze::Nucleus::EntitlementStatus::Code* entitlementStatus,
        const char8_t* projectId, const TimeValue& time, 
        bool autoGrantEnabled,
        Blaze::Nucleus::EntitlementSearchType::Type searchType,
        Blaze::Nucleus::EntitlementType::Code entitlementType = Blaze::Nucleus::EntitlementType::DEFAULT, 
        EntitlementCheckType type = STANDARD, bool useManagedLifecycle = false,
        const char8_t* startDate = nullptr, const char8_t* endDate = nullptr, const Command* callingCommand = nullptr, const char8_t* accessToken = nullptr);

    void setSoftCheck(bool softCheck) { mSoftCheck = softCheck; }
    bool getSoftCheck() const { return mSoftCheck; }
private:
    // memory owned by creator, don't free
    AuthenticationSlaveImpl &mComponent;

    HasEntitlementUtil mHasEntitlementUtil;
    bool mSoftCheck;
};

}
}
#endif //BLAZE_AUTHENTICATION_CHECKENTITLEMENTUTIL_H
