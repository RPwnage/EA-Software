/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_RATELIMITER_H
#define BLAZE_RATELIMITER_H

/*** Include files *******************************************************************************/

#include "framework/blazedefines.h"
#include "EATDF/time.h"
#include "EATDF/tdfobjectid.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/inetfilter.h"
#include "framework/tdf/frameworkconfigtypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define RATE_LIMIT_RPC_MAXIMUM         ( 32 ) // the maximum number of rate-limited RPC's that can be configured

#define RATE_LIMIT_COMMAND_WINDOW      ( 60 ) // the number of seconds the command rate-limit counters will increase before being reset

// number of seconds that a closed connection will remain in the RateLimiter's list of active connections.
// setting this to be the same as RATE_LIMIT_COMMAND_WINDOW to prevent rate-limit counters from being
// prematurely removed and reset for non-persistent connections.
#define RATE_LIMITER_INACTIVITY_PERIOD ( RATE_LIMIT_COMMAND_WINDOW ) 

// the number of seconds the connection rate-limit counters will increase before being reset. 
#define RATE_LIMIT_CONNECTION_WINDOW   (  20 ) 

// number of seconds between each handled RPC command AFTER the rate-limit has been exceeded
#define RATE_LIMIT_GRACE_PERIOD        (  5 ) 

namespace Blaze
{
class Component;
class InetAddress;

struct IpAddressToRatePerIpMapNode     : public eastl::intrusive_hash_node { };
struct IpAddressPendingRemovalListNode : public eastl::intrusive_list_node { };

struct RatesPerIp
    : IpAddressToRatePerIpMapNode,
      IpAddressPendingRemovalListNode
{
        RatesPerIp(uint32_t addr)
            : mCurrentConnectionInterval(0)
            , mCurrentCommandInterval(0)
            , mAddr(addr)
            , mLastConnectionActivity(0)
            , mCommandRate(0)
            , mConnectionAttemptRate(0)
            , mConnectionCount(0)
            , mLoggedConnectionLimitExceeded(false)
        {
            IpAddressPendingRemovalListNode::mpNext = nullptr;
            IpAddressPendingRemovalListNode::mpPrev = nullptr;
            resetCommandCounters();
        }

        void resetCommandCounters()
        {
            mCommandRate = 0;
            memset(&mSpecificCommandRate[0], 0, sizeof(mSpecificCommandRate));
        }

        // returns the absolute time of the next rate limit evaluation period, which is used to defer message processing
        EA::TDF::TimeValue getNextGracePeriod()
        {
            return EA::TDF::TimeValue::getTimeOfDay() + (RATE_LIMIT_GRACE_PERIOD * 1000 * 1000);
        }

        EA::TDF::TimeValue mCurrentConnectionInterval;
        EA::TDF::TimeValue mCurrentCommandInterval;
        uint32_t mAddr;

        EA::TDF::TimeValue mLastConnectionActivity;

        uint16_t mCommandRate;
        uint16_t mConnectionAttemptRate;
        uint16_t mConnectionCount;

        uint16_t mSpecificCommandRate[RATE_LIMIT_RPC_MAXIMUM]; // array of per command specific rates

        // this flag is used to limit the number of log entries made to the log file
        bool mLoggedConnectionLimitExceeded;

        // checks if the current RatesPerIp node has been scheduled for removal
        bool isPendingRemoval() const 
        { 
            return mpPrev != nullptr; 
        } 
};  

class RateLimiter
{
    NON_COPYABLE(RateLimiter);

public:
    RateLimiter();
    ~RateLimiter();

    // initializes and enables this RateLimiter with the given name and config
    bool initialize(const char8_t* name, const RateConfig::RateLimiter &config);
    // finalizes and disables this RateLimiter
    void finalize();

    bool isEnabled() { return mConfigured; }

    const char8_t* getName() const { return (mRateLimiterName != nullptr ? mRateLimiterName : "NOT_CONFIGURED"); }
    uint16_t getCommandRateLimit() { return mConfig.getTotalRpcRateLimitPerIpPerMinute(); }
    uint16_t getConnectionAttemptRateLimit() { return mConfig.getConnectionAttemptRateLimitPerIpPerMinute(); }
    uint16_t getConnectionCountLimit() { return mConfig.getConnectionLimitPerIp(); }

    // returns/creates item of mCurrentRatesPerIp
    RatesPerIp* getCurrentRatesForIp(uint32_t addr);
    void addMtlsVerifiedIp(uint32_t addr);

    // general command rate check
    bool tickAndCheckOverallCommandRate(RatesPerIp &currentRates);

    // specific command rate check
    bool tickAndCheckCommandRate(RatesPerIp &currentRates, EA::TDF::ComponentId componentId, CommandId commandId);

    // total connection limit check
    bool tickAndCheckConnectionLimit(RatesPerIp &currentRates);

    // increment number of connections for an ip
    void incrementConnectionCount(RatesPerIp &currentRates) const;
    // decrement number of connections for an ip
    void decrementConnectionCount(RatesPerIp &currentRates);

    bool isExcludedAddress(uint32_t addr) const
    {
        if (mExcludedAddresses == nullptr)
            return false;
        return mExcludedAddresses->match(addr);
    }

    bool isIncludedAddress(uint32_t addr) const
    {
        if (mIncludedAddresses == nullptr)
            return false;
        return mIncludedAddresses->match(addr);
    }

    /* This is an intrusive link list, to which RatesPerIp nodes are added when
       RatesPerIp::mConnectionCount == 0.  At which time, when clearStaleIpAddresses()
       runs periodically, the RatesPerIp::mLastConnectionActivity is checked against
       RATE_LIMITER_INACTIVITY_PERIOD to determine if the RatesPerIp instance should be removed and deleted. */
    typedef eastl::intrusive_list<IpAddressPendingRemovalListNode> IpAddressPendingRemovalList;
    IpAddressPendingRemovalList &getIpAddressPendingRemovalList() { return mIpAddressPendingRemovalList; }

private:
    friend class Endpoint;

    char8_t* mRateLimiterName;
    RateConfig::RateLimiter mConfig;
    bool mConfigured;

    typedef eastl::hash_map<uint32_t, size_t> CommandHashToRateTableIndexMap;
    CommandHashToRateTableIndexMap mCommandIndexLookupMap;

    typedef eastl::intrusive_hash_map<uint32_t, IpAddressToRatePerIpMapNode, 67> IpAddressToRatePerIpMap;
    IpAddressToRatePerIpMap mIpAddressToRatePerIpMap;

    IpAddressPendingRemovalList mIpAddressPendingRemovalList;

    RatesPerIp mConfigurationLimits;   // contains the configured limits, if any

    InetFilter* mExcludedAddresses;    // ips for which there is no rate limits
    InetFilter* mIncludedAddresses;    // ips for which there is no rate limits

    TimerId mClearStaleIpAddressesTimerId;
    void clearStaleIpAddresses();      // clears out any stale connections based on last activity time

    bool checkCommandIntervalElapsed(RatesPerIp& ratesPerIp) const;    // checks if current command interval is elapsed
    bool checkConnectionIntervalElapsed(RatesPerIp& ratesPerIp) const; // checks if current connection interval is elapsed

    static const int32_t MAX_CACHED_ADDRESSES = 5 * 1000; // 5 thousand IPs (Fifa 21 PS4 has approximately 1 thousand IPs)
    static const int32_t MAX_CACHED_ADDRESS_AGE_MS = 60 * 1000; // cache mTLS-ed ip for 60 seconds

    typedef LruCache<uint32_t, EA::TDF::TimeValue> MtlsLruCache;

    MtlsLruCache mMtlsCache;
};

} // Blaze

namespace eastl
{
    template <>
    struct use_intrusive_key<Blaze::IpAddressToRatePerIpMapNode, uint32_t>
    {
        const uint32_t operator()(const Blaze::IpAddressToRatePerIpMapNode &x) const
        { 
            return static_cast<const Blaze::RatesPerIp &>(x).mAddr;
        }
    };
}

#endif // BLAZE_RATELIMITER_H
