/*************************************************************************************************/
/*!
    \file
        usersessionsmetrics.h

    \attention
            (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_USERSESSIONS_METRICS_H
#define BLAZE_USERSESSIONS_METRICS_H

#include "framework/tdf/userdefines.h"
#include "framework/metrics/metrics.h"
#include "EASTL/string.h"

namespace Blaze
{
    /*
        For a full description of the metrics defined for the UserManager health checks, see:
        https://docs.developer.ea.com/display/blaze/Blaze+Health+Check+Definitions
    */
    struct UserSessionsGlobalMetrics
    {
        UserSessionsGlobalMetrics(Metrics::MetricsCollection& collection)
            : mSessionsRemoved(collection, "sessionsRemoved", Metrics::Tag::disconnect_type)
        {
        }

        Metrics::TaggedCounter<UserSessionDisconnectType> mSessionsRemoved;
    };

    /*
        For a full description of the metrics defined for the UserManager health checks, see:
        https://docs.developer.ea.com/display/blaze/Blaze+Health+Check+Definitions
    */
    struct UserSessionsManagerMetrics
    {
        UserSessionsManagerMetrics(Metrics::MetricsCollection& collection);
        static void addPerPlatformStatusInfo(ComponentStatus::InfoMap& infoMap, const Metrics::TagPairList& tagList, const char8_t* metricName, const uint64_t value);

        Metrics::Counter mUserLookupsByBlazeId;
        Metrics::TaggedCounter<ClientPlatformType> mUserLookupsByPersonaName;
        Metrics::Counter mUserLookupsByPersonaNameMultiNamespace;
        Metrics::TaggedCounter<ClientPlatformType> mUserLookupsByExternalStringOrId;
        Metrics::TaggedCounter<ClientPlatformType> mUserLookupsByAccountId;
        Metrics::TaggedCounter<ClientPlatformType> mUserLookupsByOriginPersonaId;
        Metrics::Counter mUserLookupsByOriginPersonaName;

        Metrics::Counter mUserLookupsByBlazeIdDbHits;
        Metrics::TaggedCounter<ClientPlatformType> mUserLookupsByBlazeIdActualDbHits;
        Metrics::TaggedCounter<ClientPlatformType> mUserLookupsByPersonaNameDbHits;
        Metrics::TaggedCounter<ClientPlatformType> mUserLookupsByExternalStringOrIdDbHits;
        Metrics::TaggedCounter<ClientPlatformType> mUserLookupsByAccountIdDbHits;
        Metrics::TaggedCounter<ClientPlatformType> mUserLookupsByOriginPersonaIdDbHits;
        Metrics::TaggedCounter<ClientPlatformType> mUserLookupsByOriginPersonaNameUserInfoDbHits;
        Metrics::Counter mUserLookupsByOriginPersonaNameAccountInfoDbHits;

        Metrics::Gauge mGaugeUserSessionSubscribers;
        Metrics::Gauge mGaugeUserExtendedDataProviders;

        Metrics::TaggedCounter<ClientType, UserSessionDisconnectType, ClientState::Status, ClientState::Mode> mDisconnections;
        Metrics::Counter mBlazeObjectIdListUpdateCount;
        Metrics::Counter mUEDMessagesCoallesced;
        Metrics::Counter mReputationLookups;
        Metrics::Counter mReputationLookupsWithValueChange;
        Metrics::Counter mGeoIpLookupsFailed;

        Metrics::Counter mPinEventsSent;
        Metrics::Counter mPinEventBytesSent;

        Metrics::PolledGauge mGaugeUsers;

        Metrics::PolledGauge mGaugeUserSessionsOverall;
        Metrics::PolledGauge mGaugeOwnedUserSessions;
    };

    typedef eastl::hash_map<int32_t, uint64_t> CountByErrorType;
    typedef eastl::hash_map<eastl::string, CountByErrorType, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> LatencyOrErrorCountByPingSiteAlias;
    typedef eastl::hash_map<uint16_t, uint64_t> CountByNumberOfFailures;

    struct QosMetrics
    {
        QosMetrics() :
            mNatErrorCountByBestPingSiteAlias(BlazeStlAllocator("QosMetrics::mNatErrorCountByBestPingSiteAlias")), 
            mBandwidthErrorCountByBestPingSiteAlias(BlazeStlAllocator("QosMetrics::mBandwidthErrorCountByBestPingSiteAlias")), 
            mFailuresPerQosTestCount(BlazeStlAllocator("QosMetrics::mFailuresPerQosTestCount")), 
            mLatencyByBestPingSiteAlias(BlazeStlAllocator("QosMetrics::mLatencyByBestPingSiteAlias")), 
            mErrorCountByPingSiteAlias(BlazeStlAllocator("QosMetrics::mErrorCountByPingSiteAlias"))
        {
            memset(mNatTypeCount, 0, sizeof(mNatTypeCount));
            memset(mUpnpStatusCount, 0, sizeof(mUpnpStatusCount));
        }

        uint64_t mNatTypeCount[Blaze::Util::NAT_TYPE_PENDING+1];
        uint64_t mUpnpStatusCount[UPNP_ENABLED+1];
        LatencyOrErrorCountByPingSiteAlias mNatErrorCountByBestPingSiteAlias;
        LatencyOrErrorCountByPingSiteAlias mBandwidthErrorCountByBestPingSiteAlias;
        CountByNumberOfFailures mFailuresPerQosTestCount;
        LatencyOrErrorCountByPingSiteAlias mLatencyByBestPingSiteAlias;
        LatencyOrErrorCountByPingSiteAlias mErrorCountByPingSiteAlias;
    };

    typedef eastl::hash_map<eastl::string, QosMetrics, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> QosMetricsByCountry;
    typedef eastl::hash_map<eastl::string, QosMetricsByCountry, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> QosMetricsByCountryByISP;

    struct QosMetricsByGeoIPData
    {
        QosMetricsByGeoIPData() : mByCountry(BlazeStlAllocator("QosMetricsByGeoIPData::mByCountry")), 
                                  mByCountryByISP(BlazeStlAllocator("QosMetricsByGeoIPData::mByCountryByISP")) {}

        QosMetricsByCountry mByCountry;
        QosMetricsByCountryByISP mByCountryByISP;
    };


} // Blaze

#endif // BLAZE_USERSESSIONS_METRICS_H
