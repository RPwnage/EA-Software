/*************************************************************************************************/
/*!
\file   CheckEntitlementUtil.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"

#include "authentication/util/checkentitlementutil.h"

namespace Blaze
{
namespace Authentication
{

BlazeRpcError CheckEntitlementUtil::checkEntitlement(AccountId accountId,
    const char8_t* productId,
    const char8_t* entitlementTag,
    const GroupNameList& groupNameList,
    Blaze::Nucleus::EntitlementStatus::Code* entitlementStatus,
    const char8_t* projectId,
    const TimeValue& time,
    bool autoGrantEnabled,
    Blaze::Nucleus::EntitlementSearchType::Type searchType,
    Blaze::Nucleus::EntitlementType::Code entitlementType /*= Blaze::Nucleus::EntitlementType::DEFAULT*/, 
    EntitlementCheckType type /*= STANDARD*/,
    bool useManagedLifecycle /*= false*/,
    const char8_t* startDate /*= nullptr*/,
    const char8_t* endDate /*= nullptr*/,
    const Command* callingCommand /* = nullptr */,
    const char8_t* accessToken/* = nullptr*/)
{
    TRACE_LOG("[CheckEntitlementUtil] start - Fetch entitlement");

    // we want to fetch entitlements regardless of status
    // as entitlement status check is performed further down
    BlazeRpcError rpcError =  ERR_OK;

    rpcError = mHasEntitlementUtil.hasEntitlement(accountId, productId, entitlementTag, groupNameList, projectId, autoGrantEnabled,
        entitlementType, Blaze::Nucleus::EntitlementStatus::UNKNOWN, searchType, 0, callingCommand, accessToken);

    if (rpcError != ERR_OK)
    {
        if (!mHasEntitlementUtil.isSuccess())
        {
            WARN_LOG("[CheckEntitlementUtil].checkEntitlement(): Check entitlement result - "
                "Nucleus error: " << mHasEntitlementUtil.getError().getCode());
        }
        
        BLAZE_LOG((autoGrantEnabled ? Blaze::Logging::TRACE : Blaze::Logging::ERR), LOGGER_CATEGORY, "[CheckEntitlementUtil].checkEntitlement(): AccountId (" << accountId << ") Error (" << Blaze::ErrorHelp::getErrorName(rpcError) << ")");
        return rpcError;
    }

    // check if entitlement exists
    if (mHasEntitlementUtil.getEntitlementsList().empty())
    {
        BLAZE_LOG((autoGrantEnabled ? Blaze::Logging::TRACE : Blaze::Logging::ERR), LOGGER_CATEGORY, "[CheckEntitlementUtil].checkEntitlement(): No entitlements found for user[" << accountId << "], productId[" << productId
            << "], entitlementTag[" << entitlementTag << "], projectId[" << projectId << "], entitlementType[" << Blaze::Nucleus::EntitlementType::CodeToString(entitlementType) << "]");
        return Blaze::AUTH_ERR_NO_SUCH_ENTITLEMENT;
    }

    // check if entitlement exists and go through each of them to get use count
    Entitlements::EntitlementList::const_iterator it = mHasEntitlementUtil.getEntitlementsList().begin();
    Entitlements::EntitlementList::const_iterator itend = mHasEntitlementUtil.getEntitlementsList().end();
    TimeValue tv = TimeValue::getTimeOfDay();

    // Loop through all entitlements to ensure there are no BANNED status. 
    // If ACTIVE status entitlement is found, track it with mFoundEntitlementId but need to keep going to make sure there are no BANNED entitlements as that trumps the ACTIVE one
    bool activeFound = false;
    for (; it != itend; ++it)
    {
        const Entitlement* info = *it;

        switch (info->getStatus())
        {
        case Blaze::Nucleus::EntitlementStatus::BANNED:
            {
                if (entitlementStatus != nullptr)
                {
                    *entitlementStatus = info->getStatus();
                    TRACE_LOG("[CheckEntitlementUtil].checkEntitlement(): Entitlement(" << entitlementTag << ") Status(" << *entitlementStatus << ")");
                }
                return Blaze::ERR_OK;
            }
        case Blaze::Nucleus::EntitlementStatus::ACTIVE:
            {
                if (!activeFound)
                {
                    // check if entitlement expired
                    if (type == STANDARD || (type == PERIOD_TRIAL && useManagedLifecycle) || (type == TIMED_TRIAL && useManagedLifecycle))
                    {
                        if ((info->getTerminationDate()[0] != 0) &&
                            (TimeValue::parseAccountTime(info->getTerminationDate()) <= tv))
                        {
                            if (type != STANDARD)
                            {
                                rpcError = Blaze::AUTH_ERR_NO_SUCH_ENTITLEMENT;
                                BLAZE_LOG((autoGrantEnabled ? Blaze::Logging::TRACE : Blaze::Logging::ERR), LOGGER_CATEGORY, "[CheckEntitlementUtil].checkEntitlement(): 1. Entitlement(" << entitlementTag << ") of type (" << Blaze::Authentication::EntitlementCheckTypeToString(type) << ") expired. Continuing..");
                            }

                            continue;
                        }
                    }

                    if (type == PERIOD_TRIAL && !useManagedLifecycle)
                    {
                        if (TimeValue::parseAccountTime(startDate) > tv || TimeValue::parseAccountTime(endDate) <= tv || TimeValue::parseAccountTime(startDate) > TimeValue::parseAccountTime(info->getGrantDate()))
                        {
                            rpcError = Blaze::AUTH_ERR_NO_SUCH_ENTITLEMENT;
                            BLAZE_LOG((autoGrantEnabled ? Blaze::Logging::TRACE : Blaze::Logging::ERR), LOGGER_CATEGORY, "[CheckEntitlementUtil].checkEntitlement(): 2. Entitlement(" << entitlementTag << ") of type (" << Blaze::Authentication::EntitlementCheckTypeToString(type) << ") expired. Continuing..");
                            continue;
                        }
                    }

                    if (type == TIMED_TRIAL && !useManagedLifecycle)
                    {

                        if (TimeValue::parseAccountTime(info->getGrantDate()) + time <= tv)
                        {
                            BLAZE_LOG((autoGrantEnabled ? Blaze::Logging::TRACE : Blaze::Logging::ERR), LOGGER_CATEGORY, "[CheckEntitlementUtil].checkEntitlement(): 3. Entitlement(" << entitlementTag << ") of type (" << Blaze::Authentication::EntitlementCheckTypeToString(type) << ") expired. Continuing..");
                            rpcError = Blaze::AUTH_ERR_NO_SUCH_ENTITLEMENT;
                            continue;
                        }

                    }

                    activeFound = true;
                    if (entitlementStatus != nullptr) 
                    {
                        *entitlementStatus = info->getStatus();
                        TRACE_LOG("[CheckEntitlementUtil].checkEntitlement(): Entitlement(" << entitlementTag << ") Status(" << *entitlementStatus << ")");
                    }

                    rpcError = ERR_OK;
                }
                continue;
            }
        default: // Ignore all other entitlement status
            continue;
        }
    }

    // if no active entitlement is found, return error
    if (!activeFound)
    {
        BLAZE_LOG((autoGrantEnabled ? Blaze::Logging::TRACE : Blaze::Logging::ERR), LOGGER_CATEGORY, "[CheckEntitlementUtil].checkEntitlement(): No active entitlements found for user[" << accountId << "], productId[" << productId
            << "], entitlementTag[" << entitlementTag << "], projectId[" << projectId << "], entitlementType[" << Blaze::Nucleus::EntitlementType::CodeToString(entitlementType) << "]");
        rpcError =  Blaze::AUTH_ERR_NO_SUCH_ENTITLEMENT;
    }

    return rpcError;
}

}
}
