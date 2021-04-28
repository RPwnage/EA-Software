/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Endpoint

    Encapsulates all protocol, encoder and decoder objects for a single endpoint.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/component/componentmanager.h"
#include "framework/controller/controller.h"
#include "framework/connection/ratelimiter.h"
#include "framework/connection/socketutil.h"
#include "framework/component/component.h"
#include "framework/util/shared/blazestring.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/selector.h"
#include "framework/tdf/frameworkconfigtypes_server.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/
RateLimiter::RateLimiter()
    : mRateLimiterName(nullptr),
      mConfigured(false),
      mCommandIndexLookupMap(BlazeStlAllocator("RateLimiter::mCommandIndexLookupMap")),
      mConfigurationLimits(0),
      mExcludedAddresses(nullptr),
      mIncludedAddresses(nullptr),
      mClearStaleIpAddressesTimerId(INVALID_TIMER_ID),
      mMtlsCache(RateLimiter::MAX_CACHED_ADDRESSES, BlazeStlAllocator("RateLimiter::mMtlsCache"))
{
}

RateLimiter::~RateLimiter()
{
    finalize();

    // Note: mIpAddressPendingRemovalList does not own the list members, mIpAddressToRatePerIpMap does,
    //       so we can just call clear() here since no deallocation occurs. (delete happens in the next loop)
    mIpAddressPendingRemovalList.clear();

    // delete all owned RatesPerIp objects
    while (!mIpAddressToRatePerIpMap.empty())
    {
        RatesPerIp *ratesPerIp = (RatesPerIp *)(&(*mIpAddressToRatePerIpMap.begin()));

        mIpAddressToRatePerIpMap.erase(ratesPerIp->mAddr);
        delete ratesPerIp;
    }
}

bool RateLimiter::initialize(const char8_t* name, const RateConfig::RateLimiter &config)
{
    if (mConfigured)
    {
        // incase this is being called during a reconfigure, we need to reset every RatesPerIp object
        // because the RatesPerIp::mSpecificCommandRate indexes may have changed
        // NOTE: This only modifies values in existing RatesPerIp objects, it does not delete/re-create them.
        finalize();
    }

    mRateLimiterName = blaze_strdup(name);

    config.copyInto(mConfig);
    mConfigured = true;

    // pre-calculate the actual rate per eval period to be used during the tickAndCheck*() methods
    mConfigurationLimits.mCommandRate = (uint16_t)((mConfig.getTotalRpcRateLimitPerIpPerMinute() / 60.0f) * RATE_LIMIT_COMMAND_WINDOW);
    if (mConfigurationLimits.mCommandRate == 0)
        mConfigurationLimits.mCommandRate = 1;

    // pre-calculate the actual rate per grace period to be used during the tickAndCheckConnectionLimit() method
    mConfigurationLimits.mConnectionAttemptRate = (uint16_t)((mConfig.getConnectionAttemptRateLimitPerIpPerMinute() / 60.0f) * RATE_LIMIT_CONNECTION_WINDOW);
    if (mConfigurationLimits.mConnectionAttemptRate == 0)
        mConfigurationLimits.mConnectionAttemptRate = 1;

    mConfigurationLimits.mConnectionCount = mConfig.getConnectionLimitPerIp();

    mCommandIndexLookupMap.clear();
    RateConfig::RateLimiter::RpcRateLimitList::const_iterator rpcRateLimitIt = mConfig.getSpecificRpcRateLimitPerIpPerMinute().begin();
    RateConfig::RateLimiter::RpcRateLimitList::const_iterator rpcRateLimitItend = mConfig.getSpecificRpcRateLimitPerIpPerMinute().end();
    for (size_t currentCommandIndex = 0; rpcRateLimitIt != rpcRateLimitItend; ++rpcRateLimitIt, ++currentCommandIndex)
    {
        if (currentCommandIndex == RATE_LIMIT_RPC_MAXIMUM)
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[RateLimiter].initialize: Too many RPCs are configured for rate limiting. The maximum is " << RATE_LIMIT_RPC_MAXIMUM);
            return false;
        }

        const RateConfig::RpcRateLimit *rpcRateLimit = *rpcRateLimitIt;

        ComponentId componentId = BlazeRpcComponentDb::getComponentIdByName(rpcRateLimit->getComponentName());
        CommandId commandId = BlazeRpcComponentDb::getCommandIdByName(componentId, rpcRateLimit->getCommandName());

        if (componentId != Component::INVALID_COMPONENT_ID && commandId != Component::INVALID_COMMAND_ID)
        {
            //Set the index in our lookup map
            mCommandIndexLookupMap[(componentId << 16) | commandId] = currentCommandIndex;

            // pre-calculate the actual rate per eval period
            mConfigurationLimits.mSpecificCommandRate[currentCommandIndex] = (uint16_t)((rpcRateLimit->getRateLimitPerIpPerMinute() / 60.0f) * RATE_LIMIT_COMMAND_WINDOW);
        }
        else
        {
            BLAZE_INFO_LOG(Log::CONNECTION, "[RateLimiter].initialize: Invalid componentName '" << rpcRateLimit->getComponentName() << "' or commandName '" << rpcRateLimit->getCommandName() << "' is configured for rate limiting. No action is taken.");
        }
    }

    mExcludedAddresses = BLAZE_NEW InetFilter();
    if (!mExcludedAddresses->initialize(
            (mConfig.getIncludedIpList().size() > 0), // everything is excluded if the excludedIpList is empty AND the includedIpList is not
            mConfig.getExcludedIpList()))
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[RateLimiter].initialize: invalid ExpludedIps configuration for rate limiter '" << getName() << "'");
        return false;
    }

    mIncludedAddresses = BLAZE_NEW InetFilter();
    if (!mIncludedAddresses->initialize(
            !(mConfig.getExcludedIpList().size() > 0), // nothing is included if the includeIpList is empty AND the excludeIpList is not
            mConfig.getIncludedIpList()))
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[RateLimiter].initialize: invalid IncludedIps configuration for rate limiter '" << getName() << "'");
        return false;
    }

    // Schedule a timer to expire inactive connections
    mClearStaleIpAddressesTimerId = gSelector->scheduleTimerCall(
            TimeValue::getTimeOfDay() + (RATE_LIMITER_INACTIVITY_PERIOD * 1000 * 1000),
            this, &RateLimiter::clearStaleIpAddresses,
            "RateLiminter::clearStaleIpAddresses");

    return true;
}

void RateLimiter::finalize()
{
    if ((gSelector != nullptr) && (mClearStaleIpAddressesTimerId != INVALID_TIMER_ID))
    {
        gSelector->cancelTimer(mClearStaleIpAddressesTimerId);
    }
    mClearStaleIpAddressesTimerId = INVALID_TIMER_ID;

    // reset any owned RatesPerIp objects
    for (IpAddressToRatePerIpMap::iterator
        it = mIpAddressToRatePerIpMap.begin(), itend = mIpAddressToRatePerIpMap.end(); it != itend; ++it)
    {
        ((RatesPerIp *)(&(*it)))->resetCommandCounters();
    }

    // free the old name
    BLAZE_FREE(mRateLimiterName);
    mRateLimiterName = nullptr;
    delete mIncludedAddresses;
    delete mExcludedAddresses;
    mIncludedAddresses = mExcludedAddresses = nullptr;

    mConfigured = false;
}


RatesPerIp* RateLimiter::getCurrentRatesForIp(uint32_t addr)
{
    if (!mConfigured || (isExcludedAddress(addr) && !isIncludedAddress(addr)))
        return nullptr;

    EA::TDF::TimeValue added;
    if (mMtlsCache.get(addr, added))
    {
        if ((TimeValue::getTimeOfDay() - added) < RateLimiter::MAX_CACHED_ADDRESS_AGE_MS * 1000)
        {
            BLAZE_TRACE_LOG(Log::CONNECTION, "[RateLimiter].getCurrentRatesForIp: skip rate limiting (" << InetAddress(addr, 0, InetAddress::NET).getIpAsString()
                << ") as it was added to cache very recently at " << added << ".");
            return nullptr; 
        }
        else
        {
            BLAZE_TRACE_LOG(Log::CONNECTION, "[RateLimiter].getCurrentRatesForIp: it's more than our cache interval (" << RateLimiter::MAX_CACHED_ADDRESS_AGE_MS / 1000 << "s) since we saw (" << InetAddress(addr, 0, InetAddress::NET).getIpAsString()
                << ") as mTLS peer at " << added << ". Enable rate limiting until the trust is established.");
            mMtlsCache.remove(addr); 
        }
    }

    RatesPerIp *node;

    IpAddressToRatePerIpMap::iterator it = mIpAddressToRatePerIpMap.find(addr);
    if (it == mIpAddressToRatePerIpMap.end())
    {
        // this is a new ip address, add it to the indicies
        node = BLAZE_NEW RatesPerIp(addr);
        mIpAddressToRatePerIpMap.insert(*node);
    }
    else
    {
        node = (RatesPerIp *)(&(*it));
    }

    return node;
}

void RateLimiter::addMtlsVerifiedIp(uint32_t addr)
{
    EA::TDF::TimeValue added = TimeValue::getTimeOfDay();
    
    BLAZE_TRACE_LOG(Log::CONNECTION, "[RateLimiter].addMtlsVerifiedIp: added mTLS verified IP (" << InetAddress(addr, 0, InetAddress::NET).getIpAsString()
        << ") to the cache at " << added << ".");
    
    mMtlsCache.add(addr, added);
}

bool RateLimiter::tickAndCheckOverallCommandRate(RatesPerIp &currentRates)
{
    if (!mConfigured)
        return true;

    if (checkCommandIntervalElapsed(currentRates) ||
        (mConfigurationLimits.mCommandRate > currentRates.mCommandRate))
    {
        currentRates.mCommandRate++;

        return true;
    }

    return false;
}

bool RateLimiter::tickAndCheckCommandRate(RatesPerIp &currentRates, ComponentId componentId, CommandId commandId)
{
    //we aren't setup, so no limits
    if (!mConfigured)
        return true;

    //check if this rpc is limited
    CommandHashToRateTableIndexMap::const_iterator itr = mCommandIndexLookupMap.find(componentId << 16 | commandId);
    if (itr == mCommandIndexLookupMap.end())
        return true;

    size_t cmdIndex = itr->second;
    if (checkCommandIntervalElapsed(currentRates) ||
        (mConfigurationLimits.mSpecificCommandRate[cmdIndex] > currentRates.mSpecificCommandRate[cmdIndex]))
    {
        currentRates.mSpecificCommandRate[cmdIndex]++;

        return true;
    }

    return false;
}

bool RateLimiter::tickAndCheckConnectionLimit(RatesPerIp &currentRates)
{
    if (!mConfigured)
        return true;

    char8_t addrBuf[256];
    blaze_snzprintf(addrBuf, sizeof(addrBuf), "%d.%d.%d.%d", NIPQUAD(currentRates.mAddr));

    if (checkConnectionIntervalElapsed(currentRates) ||
        (mConfigurationLimits.mConnectionAttemptRate > currentRates.mConnectionAttemptRate))
    {
        currentRates.mConnectionAttemptRate++;
        currentRates.mLoggedConnectionLimitExceeded = false;
    }
    else
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[RateLimiter].tickAndCheckConnectionLimit: rateCounter(" << &currentRates << ") connectionAttemptRate("
            << (uint32_t)currentRates.mConnectionAttemptRate << "), addr=" << addrBuf << " over limit");
        return false;
    }
            
    if (mConfigurationLimits.mConnectionCount > currentRates.mConnectionCount)
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[RateLimiter].tickAndCheckConnectionLimit: rateCounter(" << &currentRates << ") connectionCount("
            << (uint32_t)currentRates.mConnectionCount << "), addr=" << addrBuf << " within limit");
        return true;
    }

    BLAZE_TRACE_LOG(Log::CONNECTION, "[RateLimiter].tickAndCheckConnectionLimit: rateCounter(" << &currentRates << ") connectionCount("
        << (uint32_t)currentRates.mConnectionCount << "), addr=" << addrBuf << " over limit");

    return false;
}

void RateLimiter::decrementConnectionCount(RatesPerIp &currentRates)
{
    char8_t addrBuf[256];
    blaze_snzprintf(addrBuf, sizeof(addrBuf), "%d.%d.%d.%d", NIPQUAD(currentRates.mAddr));

    BLAZE_TRACE_LOG(Log::CONNECTION, "[RateLimiter].decrementConnectionCount: rateCounter(" << &currentRates << ") connectionCount("
                   << (uint32_t)currentRates.mConnectionCount <<"), addr=" << addrBuf);
    // it's possible that rate-limit counter was created but never passed
    // into tickAndCheckConnectionLimit() if a connection was created and then quickly closed
    // by the client.  Because of that, we're not always guarranteed that mConnectionCount
    // will have been incrememnted.
    if (currentRates.mConnectionCount > 0)
    {
        --currentRates.mConnectionCount;

        // Only add to the list if the connection count is 0 and currentRates isn't already in the list
        if (currentRates.mConnectionCount == 0 && !currentRates.isPendingRemoval())
            mIpAddressPendingRemovalList.push_back(currentRates);
    }
}

void RateLimiter::incrementConnectionCount(RatesPerIp &currentRates) const
{
    char8_t addrBuf[256];
    blaze_snzprintf(addrBuf, sizeof(addrBuf), "%d.%d.%d.%d", NIPQUAD(currentRates.mAddr));

    BLAZE_TRACE_LOG(Log::CONNECTION, "[RateLimiter].incrementConnectionCount: rateCounter(" << &currentRates << ") connectionCount(" 
                   << (uint32_t)currentRates.mConnectionCount << "), addr=" << addrBuf);

    ++currentRates.mConnectionCount;
}
/*** Private Methods *****************************************************************************/

void RateLimiter::clearStaleIpAddresses()
{
    TimeValue now = TimeValue::getTimeOfDay();

    // walk the link list, which is sorted by least active ip addresses at the front of the list
    IpAddressPendingRemovalList::iterator it = mIpAddressPendingRemovalList.begin();
    IpAddressPendingRemovalList::iterator itend = mIpAddressPendingRemovalList.end();
    while (it != itend)
    {
        RatesPerIp *ratesPerIp = (RatesPerIp *)(&(*it++)); // advance the iterator to the next node in the list

        if (now - ratesPerIp->mLastConnectionActivity > RATE_LIMITER_INACTIVITY_PERIOD * 1000 * 1000)
        {
            IpAddressPendingRemovalList::remove(*ratesPerIp);
            ((IpAddressPendingRemovalListNode *)ratesPerIp)->mpPrev = ((IpAddressPendingRemovalListNode *)ratesPerIp)->mpNext = nullptr;

            if (ratesPerIp->mConnectionCount == 0)
            {
                char8_t addrBuf[256];
                blaze_snzprintf(addrBuf, sizeof(addrBuf), "%d.%d.%d.%d", NIPQUAD(ratesPerIp->mAddr));

                mIpAddressToRatePerIpMap.erase(ratesPerIp->mAddr);
                BLAZE_TRACE_LOG(Log::CONNECTION, "[RateLimiter].clearStaleIpAddresses: rateCounter(" << ratesPerIp << "), addr=" << addrBuf);
                delete ratesPerIp;
            }
        }
    }

    // Reschedule a timer to call this function
    mClearStaleIpAddressesTimerId = gSelector->scheduleTimerCall(
        TimeValue::getTimeOfDay() + (RATE_LIMITER_INACTIVITY_PERIOD * 1000 * 1000),
        this, &RateLimiter::clearStaleIpAddresses,
        "RateLimiter::clearStateIpAddresses");
}

bool RateLimiter::checkCommandIntervalElapsed(RatesPerIp& ratesPerIp) const
{
    TimeValue now = TimeValue::getTimeOfDay();

    ratesPerIp.mLastConnectionActivity = now;

    if ((((IpAddressPendingRemovalListNode &)ratesPerIp).mpPrev != nullptr) ||
        (((IpAddressPendingRemovalListNode &)ratesPerIp).mpNext != nullptr))
    {
        // If the intrusive node pointers are not nullptr, then the node is in the removable list.
        // So, remove it from that list because the ip address is showing some activity
        IpAddressPendingRemovalList::remove(ratesPerIp);
        ((IpAddressPendingRemovalListNode &)ratesPerIp).mpPrev = ((IpAddressPendingRemovalListNode &)ratesPerIp).mpNext = nullptr;
    }

    if ((now - ratesPerIp.mCurrentCommandInterval) > RATE_LIMIT_COMMAND_WINDOW * 1000 * 1000)
    {
        ratesPerIp.mCurrentCommandInterval = now;
        ratesPerIp.resetCommandCounters();

        return true;
    }

    return false;
}

bool RateLimiter::checkConnectionIntervalElapsed(RatesPerIp& ratesPerIp) const
{
    TimeValue now = TimeValue::getTimeOfDay();

    ratesPerIp.mLastConnectionActivity = now;

    if ((((IpAddressPendingRemovalListNode &)ratesPerIp).mpPrev != nullptr) ||
        (((IpAddressPendingRemovalListNode &)ratesPerIp).mpNext != nullptr))
    {
        // If the intrusive node pointers are not nullptr, then the node is in the removable list.
        // So, remove it from that list because the ip address is showing some activity
        IpAddressPendingRemovalList::remove(ratesPerIp);
        ((IpAddressPendingRemovalListNode &)ratesPerIp).mpPrev = ((IpAddressPendingRemovalListNode &)ratesPerIp).mpNext = nullptr;
    }

    if ((now - ratesPerIp.mCurrentConnectionInterval) > RATE_LIMIT_CONNECTION_WINDOW * 1000 * 1000)
    {
        ratesPerIp.mCurrentConnectionInterval = now;
        ratesPerIp.mConnectionAttemptRate = 0;

        return true;
    }

    return false;
}

} // namespace Blaze

