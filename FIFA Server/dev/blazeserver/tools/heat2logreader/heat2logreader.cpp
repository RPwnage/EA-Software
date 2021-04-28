#include "EABase/eabase.h"  //include eabase.h before any std lib includes for VS2012 (see EABase/2.02.00/doc/FAQ.html#Info.17 for details)

#include <stdlib.h>
#include <iostream>

#include "framework/blaze.h"
#include "framework/connection/socketutil.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/logger.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/protocol/eventxmlencoder.h"
#include "framework/protocol/shared/xml2encoder.h"
#include "component/gamemanager/tdf/gamemanagerevents_server.h"
#include "component/gamemanager/tdf/gamemanagermetrics_server.h"
#include "component/gamemanager/rpc/gamemanagermetricsmaster.h"
#include "framework/rpc/usersessionsslave.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/tdf/eventtypes_server.h"
#include "framework/tdf/userevents_server.h"
#include "framework/rpc/blazecontrollerslave.h"

#include "EAStdC/EAString.h"
#include "EAStdC/EASprintf.h"
#include "EAIO/PathString.h"
#include "EAIO/EAFileDirectory.h"
#include "gamereporting/gamereportingcustom.h"

#ifdef EA_PLATFORM_WINDOWS
#include <io.h>
#include <fcntl.h>
# define SET_BINARY_MODE(handle) _setmode(_fileno(handle), _O_BINARY)
#else
# define SET_BINARY_MODE(handle) ((void)0)
#endif
namespace heat2logreader
{

    using namespace Blaze;

    typedef enum { ENCODE_XML, ENCODE_PRINT, ENCODE_TERSE } EncodeType;

    static char8_t* gFilePath = nullptr;
    static const char8_t* gOutputPrefix = "";
    static EncodeType gEncodeType = ENCODE_PRINT;
    static bool gPrintAsList = true;
    static bool gPrintEnvelope = true;
    static TimeValue gStartTime = 0;
    static TimeValue gEndTime = TimeValue::getTimeOfDay();

    static const uint32_t PRINT_COMPONENT_METRICS = 1;
    static const uint32_t PRINT_FIBER_TIMINGS = 2;
    static const uint32_t PRINT_STATUS = 4;
    static const uint32_t PRINT_DBQUERY_METRICS = 8;
    static const uint32_t PRINT_RPC_RATE_SUMMARY = 16;
    static uint32_t gPrintControllerMetrics = 0;
    static bool gPrintConnectionMetrics = false;
    static bool gPrintUserMetrics = false;
    static char8_t gDbQueryNamePrefix[1024] = "";
    typedef eastl::map<eastl::string, ComponentMetricsResponse*> InstanceToComponentMetricsMap;
    static InstanceToComponentMetricsMap gPrevComponentMetricsMap;
    typedef eastl::map<eastl::string, FiberTimings*> InstanceToFiberTimingsMap;
    static InstanceToFiberTimingsMap gPrevFiberTimingsMap;

    struct RpcRateMetrics
    {
        double minimum;
        double maximum;
        double average;
        uint64_t count;
        RpcRateMetrics()
            : minimum(0.0), maximum(0.0), average(0.0), count(0)
        {}
        void set(double rate)
        {
            if (rate < minimum || count == 0)
                minimum = rate;
            if (rate > maximum)
                maximum = rate;
            // we compute the factors separately here to avoid roundoff errors
            // in case of large magnitude average values and counts
            double newFactor = 1.0 / (count + 1.0);
            double oldFactor = count * newFactor;
            average = average * oldFactor + rate * newFactor;
            ++count;
        }
        void add(const RpcRateMetrics& other)
        {
            minimum += other.minimum;
            maximum += other.maximum;
            average += other.average;
        }
    };

    typedef eastl::hash_map<uint16_t, RpcRateMetrics> RpcMetricsMap;
    typedef eastl::hash_map<uint16_t, RpcMetricsMap> ComponentRpcMetricsMap;
    typedef eastl::map<eastl::string, ComponentRpcMetricsMap> ServerComponentRpcMetricsMap;

    static double gRpcRateScale = 1.0;
    static ServerComponentRpcMetricsMap gServerRpcMetrics;


    static void diffCommandMetrics(const CommandMetricInfo& prev, CommandMetricInfo& curr)
    {
        curr.setCount(curr.getCount() - prev.getCount());
        curr.setSuccessCount(curr.getSuccessCount() - prev.getSuccessCount());
        curr.setFailCount(curr.getFailCount() - prev.getFailCount());
        curr.setAuthorizationFailureCount(curr.getAuthorizationFailureCount() - prev.getAuthorizationFailureCount());
        curr.setTotalTime(curr.getTotalTime() - prev.getTotalTime());
        curr.setGetConnTime(curr.getGetConnTime() - prev.getGetConnTime());
        curr.setTxnCount(curr.getTxnCount() - prev.getTxnCount());
        curr.setQueryCount(curr.getQueryCount() - prev.getQueryCount());
        curr.setMultiQueryCount(curr.getMultiQueryCount() - prev.getMultiQueryCount());
        curr.setPreparedStatementCount(curr.getPreparedStatementCount() - prev.getPreparedStatementCount());
        curr.setQueryTime(curr.getQueryTime() - prev.getQueryTime());
        curr.setQuerySetupTimeOnThread(curr.getQuerySetupTimeOnThread() - prev.getQuerySetupTimeOnThread());
        curr.setQueryExecutionTimeOnThread(curr.getQueryExecutionTimeOnThread() - prev.getQueryExecutionTimeOnThread());
        curr.setQueryCleanupTimeOnThread(curr.getQueryCleanupTimeOnThread() - prev.getQueryCleanupTimeOnThread());
        curr.setFiberTime(curr.getFiberTime() - prev.getFiberTime());

        Blaze::ErrorCountMap& errorCountMap = curr.getErrorCountMap();
        for (auto it = errorCountMap.begin(), itEnd = errorCountMap.end(); it != itEnd; ++it)
        {
            uint64_t count = it->second;
            auto prevItr = prev.getErrorCountMap().find(it->first);
            if (prevItr != prev.getErrorCountMap().end())
                count -= prevItr->second;
            errorCountMap[it->first] = count;
        }
    }

    static void processComponentMetrics(const char8_t* eventInfo, const char8_t* instanceKey, ComponentMetricsResponse& metrics)
    {
        printf("%s\n", eventInfo);

        printf("%-32s %-48s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s\n",
            "Component", "Command", "Count", "Success", "Fail", "Time", "GetConn", "Txn",
            "Query", "MultiQ", "PrepStmt", "QTime", "QTimeSu", "QTimeEx", "QTimeCu", "FiberTime");
        printf("----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

        ComponentMetricsResponse* curr = metrics.clone();
        // Calculate a diff between these metrics and the previous interval
        ComponentMetricsResponse::CommandMetricInfoList::iterator itr, end;
        auto prevEntry = gPrevComponentMetricsMap.find(instanceKey);
        if (prevEntry != gPrevComponentMetricsMap.end())
        {
            ComponentMetricsResponse* prev = prevEntry->second;
            itr = metrics.getMetrics().begin();
            end = metrics.getMetrics().end();
            for (; itr != end; ++itr)
            {
                CommandMetricInfo* currCmd = *itr;
                auto prevItr = prev->getMetrics().begin();
                auto prevEnd = prev->getMetrics().end();
                for (; prevItr != prevEnd; ++prevItr)
                {
                    if ((currCmd->getComponent() == (*prevItr)->getComponent()) && (currCmd->getCommand() == (*prevItr)->getCommand()))
                    {
                        diffCommandMetrics(**prevItr, *currCmd);
                        break;
                    }
                }
            }
            delete prev;
        }
        gPrevComponentMetricsMap[instanceKey] = curr;

        itr = metrics.getMetrics().begin();
        end = metrics.getMetrics().end();
        for (; itr != end; ++itr)
        {
            CommandMetricInfo& m = **itr;
            if (m.getCount() > 0)
            {
                printf("%-32s %-48s %8" PRIu64 " %8" PRId64 " %8" PRId64 " %8" PRId64 " %8" PRId64 " %8" PRIu64 " %8" PRIu64 " %8" PRIu64 " %8" PRIu64 " %8" PRId64 " %8" PRId64  " %8" PRId64  " %8" PRId64  " %9" PRId64 "\n",
                    BlazeRpcComponentDb::getComponentNameById(m.getComponent()),
                    BlazeRpcComponentDb::getCommandNameById(m.getComponent(), m.getCommand()),
                    m.getCount(), m.getSuccessCount(), m.getFailCount(), m.getTotalTime(),
                    m.getGetConnTime(), m.getTxnCount(), m.getQueryCount(), m.getMultiQueryCount(),
                    m.getPreparedStatementCount(), m.getQueryTime(), m.getQuerySetupTimeOnThread(), m.getQueryExecutionTimeOnThread(), m.getQueryCleanupTimeOnThread(),
                    m.getFiberTime());
            }
        }

        printf("\n");

        itr = metrics.getMetrics().begin();
        end = metrics.getMetrics().end();
        for (; itr != end; ++itr)
        {
            CommandMetricInfo& m = **itr;
            if (m.getCount() > 0)
            {
                for (ErrorCountMap::const_iterator i = m.getErrorCountMap().begin(), e = m.getErrorCountMap().end(); i != e; ++i)
                {
                    if (i->second > 0)
                    {
                        printf("%s.%s.%s %" PRIu64 "\n",
                            BlazeRpcComponentDb::getComponentNameById(m.getComponent()),
                            BlazeRpcComponentDb::getCommandNameById(m.getComponent(), m.getCommand()),
                            i->first.c_str(),
                            i->second);
                    }
                }
            }
        }

        printf("\n");
    }

    static void processRpcRates(const char8_t* eventInfo, ComponentMetricsResponse& metrics)
    {
        const char8_t* serverName = gOutputPrefix;
        ComponentMetricsResponse::CommandMetricInfoList::iterator itr = metrics.getMetrics().begin();
        ComponentMetricsResponse::CommandMetricInfoList::iterator end = metrics.getMetrics().end();
        for (; itr != end; ++itr)
        {
            CommandMetricInfo& m = **itr;
            if (m.getCount() > 0)
            {
                ComponentRpcMetricsMap& cMap = gServerRpcMetrics[serverName];
                RpcMetricsMap& rMap = cMap[m.getComponent()];
                RpcRateMetrics& rateMetrics = rMap[m.getCommand()];
                double rate = (double)m.getCount();
                rateMetrics.set(rate);
            }
        }
    }

    static void diffFiberTimings(const char8_t* instanceKey, FiberTimings& timings)
    {
        FiberTimings* curr = timings.clone();

        // Calculate a diff between these timings and the previous interval
        auto prevEntry = gPrevFiberTimingsMap.find(instanceKey);
        if (prevEntry != gPrevFiberTimingsMap.end())
        {
            FiberTimings* prev = prevEntry->second;

            // Calculate the difference across the interval
            timings.setTotalCpuTime(timings.getTotalCpuTime() - prev->getTotalCpuTime());
            timings.setMainFiberCpuTime(timings.getMainFiberCpuTime() - prev->getMainFiberCpuTime());

            for (auto itr = timings.getFiberMetrics().begin(); itr != timings.getFiberMetrics().end(); )
            {
                FiberAccountingEntry* entry = itr->second;
                auto find = prev->getFiberMetrics().find(itr->first);
                if (find != prev->getFiberMetrics().end())
                {
                    entry->setFiberTimeMicroSecs(entry->getFiberTimeMicroSecs() - find->second->getFiberTimeMicroSecs());
                    entry->setCount(entry->getCount() - find->second->getCount());
                }
                if (entry->getCount() == 0)
                    itr = timings.getFiberMetrics().erase(itr);
                else
                    ++itr;
            }
            timings.setCpuBudgetCheckCount(timings.getCpuBudgetCheckCount() - prev->getCpuBudgetCheckCount());
            timings.setCpuBudgetExceededCount(timings.getCpuBudgetExceededCount() - prev->getCpuBudgetExceededCount());

            delete prev;
        }
        gPrevFiberTimingsMap[instanceKey] = curr;

    }


    static void processFiberTimings(const char8_t* eventInfo, const char8_t* instanceKey, FiberTimings& fiberTimings)
    {
        diffFiberTimings(instanceKey, fiberTimings);

        printf("%s totalCpuTime=%" PRIu64 " mainFiberCpuTime=%" PRIu64 " CpuBudgetCheckCount=%" PRIu64 " CpuBudgetExceededCount=%" PRIu64 "\n", eventInfo, fiberTimings.getTotalCpuTime(), fiberTimings.getMainFiberCpuTime(), fiberTimings.getCpuBudgetCheckCount(), fiberTimings.getCpuBudgetExceededCount());
        printf("%-64s %-12s %s\n", "Context", "Time(us)", "Count");
        printf("-------------------------------------------------------------------------------------\n");

        FiberTimings::FiberMetricsMap::iterator itr = fiberTimings.getFiberMetrics().begin();
        FiberTimings::FiberMetricsMap::iterator end = fiberTimings.getFiberMetrics().end();
        for (; itr != end; ++itr)
        {
            FiberAccountingEntry* entry = itr->second;
            printf("%-64s %-12" PRIu64 " %-12" PRIu64 "\n", itr->first.c_str(), entry->getFiberTimeMicroSecs(), entry->getCount());
        }
        printf("\n");
    }

    static void processStatus(const char8_t* eventInfo, ServerStatus& status)
    {
        printf("%s\n", eventInfo);

        StringBuilder s;
        printf("%s\n", (s << status).get());
    }

    static void processDbQueryMetrics(const char8_t* eventInfo, DbQueryMetrics& metrics)
    {
        printf("%s\n", eventInfo);

        printf("%-128s %-128s %-12s %-12s %-12s %-12s %-12s\n", "Name", "Context", "Count", "Success", "Slow", "Fail", "Total Time");
        printf("---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

        DbQueryAccountingInfos::iterator itr = metrics.getDbQueryAccountingInfos().begin();
        DbQueryAccountingInfos::iterator end = metrics.getDbQueryAccountingInfos().end();
        for (; itr != end; ++itr)
        {
            DbQueryAccountingInfo* info = itr->second;
            const char8_t* name = itr->first.c_str();
            if (gDbQueryNamePrefix[0] != '\0')
            {
                if (strstr(name, gDbQueryNamePrefix) == name)
                    name += strlen(gDbQueryNamePrefix);
            }
            DbQueryFiberCallCountMap& contextMap = info->getFiberCallCounts();
            DbQueryFiberCallCountMap::iterator ctxItr = contextMap.begin();
            DbQueryFiberCallCountMap::iterator ctxEnd = contextMap.end();
            for (; ctxItr != ctxEnd; ++ctxItr)
            {
                printf("%-128s %-128s %-12" PRIu64 "\n", name, ctxItr->first.c_str(), ctxItr->second);
            }
            char8_t contextBuf[64];
            blaze_snzprintf(contextBuf, sizeof(contextBuf), "TOTAL(%u)", (uint32_t)contextMap.size());
            printf("%-128s %-128s %-12" PRIu64 " %-12" PRIu64 " %-12" PRIu64 " %-12" PRIu64 " %-12" PRIu64 "\n", name, contextBuf, info->getTotalCount(), info->getSuccessCount(), info->getSlowQueryCount(), info->getFailCount(), info->getTotalTime());
        }
        printf("\n");
    }

    static char8_t* replaceCommas(const char8_t* input, char8_t* output, size_t outlen)
    {
        size_t i;
        for (i = 0; (i < outlen) && (input[i] != '\0'); ++i)
        {
            char8_t c = input[i];
            if (c == ',')
                c = '_';
            output[i] = c;
        }
        output[i] = '\0';
        return output;
    }

    static void printConnectionMetricsHeader(const GameManager::DisconnectedEndpointData& metrics)
    {
        static bool sPrintHeader = true;

        if (!sPrintHeader)
            return;
        sPrintHeader = false;

        EA::StdC::Printf("Topology,Ping Site,Account Country,Game Mode,Client Type, Client Initiated,QoS Rule Enabled,Local IP,Local Port");
        EA::StdC::Printf(",Remote IP,Remote Port");
        EA::StdC::Printf(",Disconnect Reason,Connection Status,Total Packet Loss,Average Latency,Max Latency,# Packet Loss Buckets,# Latency Buckets");
        for (size_t i = 0; i < metrics.getTotalPacketLossSample().size(); ++i)
            EA::StdC::Printf(",p%" PRIsize, i);
        for (size_t i = 0; i < metrics.getTotalLatencySample().size(); ++i)
            EA::StdC::Printf(",l%" PRIsize, i);
        EA::StdC::Printf("\n");
    }

    static void processConnectionMetrics(const char8_t* eventInfo, const GameManager::DisconnectedEndpointData& metrics)
    {
        printConnectionMetricsHeader(metrics);

        if (metrics.getTotalLatencyAvg() != -1)
        {
            uint32_t localIp = metrics.getLocalIp();
            uint32_t remoteIp = metrics.getRemoteIp();
            printf("%s,%s,%s,%s,%s,%d,%d,%d.%d.%d.%d,%d",
                GameNetworkTopologyToString(metrics.getGameNetworkTopology()),
                metrics.getPingSite(),
                metrics.getCountry(),
                metrics.getGameMode(),
                ClientTypeToString(metrics.getClientType()),
                metrics.getClientInitiatedConnection(),
                metrics.getQosRuleEnabled(),
                HIPQUAD(localIp),
                metrics.getLocalPort());
            printf(",%d.%d.%d.%d,%d", HIPQUAD(remoteIp), metrics.getRemotePort());
            printf(",%s,%s,%d,%" PRIi64 ",%" PRIi64,
                PlayerRemovedReasonToString(metrics.getDisconnectReason()),
                DemanglerConnectionHealthcheckToString(metrics.getConnectionStatus()),
                metrics.getTotalPacketLoss(),
                metrics.getTotalLatencyAvg(),
                metrics.getTotalLatencyMax());
            printf(",%d", (int32_t)metrics.getTotalPacketLossSample().size());
            printf(",%d", (int32_t)metrics.getTotalLatencySample().size());
            for (auto entry : metrics.getTotalPacketLossSample())
                printf(",%u", entry);
            for (auto entry : metrics.getTotalLatencySample())
                printf(",%u", entry);
            printf("\n");
        }
    }

    typedef eastl::vector<eastl::pair<eastl::string, eastl::string> > PingSiteData;

    static void parsePingSiteData(const char8_t* networkInfo, PingSiteData& pingSiteData)
    {
        int32_t lowestLatencyIndex = -1;
        int32_t lowestLatency = INT32_MAX;
        char8_t info[2048];
        blaze_strnzcpy(info, networkInfo, sizeof(info));
        char8_t* tokptr = nullptr;
        char8_t* tok = blaze_strtok(info, ",", &tokptr);
        int32_t idx = 0;
        while (tok != nullptr)
        {
            char* v = strchr(tok, '=');
            if (v != nullptr)
            {
                int32_t latency = -1;
                *v++ = '\0';
                if (*v != '\0')
                    latency = EA::StdC::AtoI32(v);
                if ((latency >= 0) && ((lowestLatencyIndex == -1) || (latency < lowestLatency)))
                {
                    lowestLatency = latency;
                    lowestLatencyIndex = idx;
                }
            }
            pingSiteData.push_back(eastl::make_pair(tok, v ? v : ""));
            tok = blaze_strtok(nullptr, ",", &tokptr);
            ++idx;
        }
        pingSiteData.push_back(eastl::make_pair("Best Ping Site", lowestLatencyIndex == -1 ? "" : pingSiteData[lowestLatencyIndex].first.c_str()));
    }

    static void printUserMetricsHeader(const Logout& event)
    {
        static bool sPrintHeader = true;

        if (!sPrintHeader)
            return;
        sPrintHeader = false;

        printf("Timestamp,Type,Blaze Id,Service Name,Project ID,Session ID,User Session Type,Persona,Client Type,Client Platform,Account ID,Origin ID,Origin Persona,PSN ID,XBL ID,Switch ID,IP,Error,Connection Locale,Locale,Geo Country,Geo State/Region,Geo City,Time Zone,Geo ISP,Latitude,Longitude,Time Online,NAT Type,NAT Error Code,Upstream bps,Downstream bps,Bandwidth Error Code,UPnP Last Rslt Code,UPnP Flags,UPnP Blaze Flags,UPnP NAT Type,UPnP Status,UPnP Device Info,TTS Events,STT Events,Close Reason,Socket Error,Client Status,Client Mode,Unique Device ID,Trial Content IDs");

        PingSiteData pingSiteData;
        parsePingSiteData(event.getNetworkInfo(), pingSiteData);
        for (auto site : pingSiteData)
            printf(",%s", site.first.c_str());
        printf("\n");
    }

    static const char8_t* platformInfoToCSV(const PlatformInfo& platformInfo, eastl::string& str)
    {
        str.sprintf("%s,%" PRId64 ",%" PRIu64 ",%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s",
            ClientPlatformTypeToString(platformInfo.getClientPlatform()),
            platformInfo.getEaIds().getNucleusAccountId(),
            platformInfo.getEaIds().getOriginPersonaId(),
            platformInfo.getEaIds().getOriginPersonaName(),
            platformInfo.getExternalIds().getPsnAccountId(),
            platformInfo.getExternalIds().getXblAccountId(),
            platformInfo.getExternalIds().getSteamAccountId(),
            platformInfo.getExternalIds().getSwitchId());
        return str.c_str();
    }

    static void printUserMetricsCommon(const char8_t* timestamp, const char8_t* eventType, BlazeId blazeId, const char8_t* serviceName, const char8_t* projectId, UserSessionId sessionId, UserSessionType sessionType,
        const char8_t* personaName, const char8_t* clientType, const PlatformInfo& platformInfo, const char8_t* ipAddress, const char8_t* error, const Login* login)
    {
        eastl::string platformInfoStr;
        printf("%s,%s,%" PRId64 ",%s,%s,%" PRIu64 ",%s,%s,%s,%s,%s,%s",
            timestamp,
            eventType,
            blazeId,
            serviceName,
            projectId,
            sessionId,
            UserSessionTypeToString(sessionType),
            personaName,
            clientType,
            platformInfoToCSV(platformInfo, platformInfoStr),
            ipAddress,
            error);

        if (login == nullptr)
        {
            printf(",,,,,,,,,");
        }
        else
        {
            printf(",%s,%s,%s,%s,%s,%s,%s,%f,%f",
                login->getConnectionLocale(),
                login->getLocale(),
                login->getGeoCountry(),
                login->getGeoStateRegion(),
                login->getGeoCity(),
                login->getTimeZone(),
                login->getISP(),
                login->getLatitude(),
                login->getLongitude());
        }
    }

    static void processLoginEvent(const char8_t* timestamp, const Login& event, bool isFailureEvent)
    {
        printUserMetricsCommon(
            timestamp,
            isFailureEvent ? "loginFailure" : "login",
            event.getBlazeId(),
            event.getServiceName(),
            event.getProjectId(),
            event.getSessionId(),
            event.getSessionType(),
            event.getPersonaName(),
            event.getClientType(),
            event.getPlatformInfo(),
            event.getIpAddress(),
            event.getError(),
            &event
        );

        printf("\n");
    }

    static eastl::vector<eastl::string> sPingSiteList;

    static const char8_t* trialContentIdsToString(const TrialContentIdList& ids, char8_t* buf, size_t len)
    {
        buf[0] = '\0';
        if (ids.size() == 0)
            return buf;

        int32_t dstSize = 0;
        for (auto id : ids)
        {
            int32_t written = blaze_snzprintf(buf + dstSize, len - dstSize, ":%s", id.c_str());
            if (written > 0)
            {
                dstSize += written;
                if (dstSize >= (int32_t)len)
                    break;
            }
        }
        return buf + 1;
    }

    static void processLogoutEvent(const char8_t* timestamp, const Logout& event, bool isDisconnectEvent)
    {
        printUserMetricsHeader(event);

        printUserMetricsCommon(
            timestamp,
            isDisconnectEvent ? "disconnect" : "logout",
            event.getBlazeId(),
            event.getServiceName(),
            event.getProjectId(),
            event.getSessionId(),
            event.getSessionType(),
            event.getPersonaName(),
            event.getClientType(),
            event.getPlatformInfo(),
            event.getIpAddress(),
            event.getError(),
            nullptr
        );

        char8_t trialBuf[8192];
        char8_t deviceInfo[2048];
        printf(",%u,%s,%u,%u,%u,%u,%d,%u,%d,%u,%s,%s,%u,%u,%s,%d,%s,%s,%s,%s",
            event.getTimeOnline(),
            NatTypeToString(event.getQosData().getNatType()),
            event.getQosData().getNatErrorCode(),
            event.getQosData().getUpstreamBitsPerSecond(),
            event.getQosData().getDownstreamBitsPerSecond(),
            event.getQosData().getBandwidthErrorCode(),
            event.getClientMetrics().getLastRsltCode(),
            event.getClientMetrics().getFlags(),
            event.getClientMetrics().getBlazeFlags().getBits(),
            event.getClientMetrics().getNatType(),
            UpnpStatusToString(event.getClientMetrics().getStatus()),
            replaceCommas(event.getClientMetrics().getDeviceInfo(), deviceInfo, sizeof(deviceInfo)),
            event.getClientUserMetrics().getTtsEventCount(),
            event.getClientUserMetrics().getSttEventCount(),
            event.getCloseReason(),
            event.getSocketError(),
            ClientState::StatusToString(event.getClientState().getStatus()),
            ClientState::ModeToString(event.getClientState().getMode()),
            event.getUniqueDeviceId(),
            trialContentIdsToString(event.getTrialContentIds(), trialBuf, sizeof(trialBuf))
        );

        PingSiteData pingSiteData;
        parsePingSiteData(event.getNetworkInfo(), pingSiteData);
        for (auto site : pingSiteData)
            printf(",%s", site.second.c_str());
        printf("\n");
    }

    static void processEvent(TimeValue eventTime, const Event::EventEnvelope& eventEnvelope)
    {
        char8_t timeBuf[256];
        char8_t eventInfo[1024];
        blaze_snzprintf(eventInfo, sizeof(eventInfo), "%s (%s/%s)", eventTime.toString(timeBuf, sizeof(timeBuf)), eventEnvelope.getServiceName(), eventEnvelope.getInstanceName());

        int32_t eventId = (int32_t)((eventEnvelope.getComponentId() << 16) | eventEnvelope.getEventType());
        switch (eventId)
        {
        case BlazeControllerSlave::EVENT_COMPONENTMETRICSEVENT:
        {
            if (gPrintControllerMetrics & (PRINT_COMPONENT_METRICS | PRINT_RPC_RATE_SUMMARY))
            {
                ComponentMetricsResponse* tdf = (ComponentMetricsResponse*)eventEnvelope.getEventData();
                if (tdf != nullptr)
                {
                    if (gPrintControllerMetrics & PRINT_RPC_RATE_SUMMARY)
                        processRpcRates(eventInfo, *tdf);
                    else
                        processComponentMetrics(eventInfo, eventEnvelope.getInstanceName(), *tdf);
                }
            }
            break;
        }
        case BlazeControllerSlave::EVENT_FIBERTIMINGSEVENT:
        {
            if (gPrintControllerMetrics & PRINT_FIBER_TIMINGS)
            {
                FiberTimings* tdf = (FiberTimings*)eventEnvelope.getEventData();
                if (tdf != nullptr)
                    processFiberTimings(eventInfo, eventEnvelope.getInstanceName(), *tdf);
            }
            break;
        }
        case BlazeControllerSlave::EVENT_STATUSEVENT:
        {
            if (gPrintControllerMetrics & PRINT_STATUS)
            {
                ServerStatus* tdf = (ServerStatus*)eventEnvelope.getEventData();
                if (tdf != nullptr)
                    processStatus(eventInfo, *tdf);
            }
            break;
        }

        case BlazeControllerSlave::EVENT_DBQUERYMETRICSEVENT:
        {
            if (gPrintControllerMetrics & PRINT_DBQUERY_METRICS)
            {
                DbQueryMetrics* tdf = (DbQueryMetrics*)eventEnvelope.getEventData();
                if (tdf != nullptr)
                    processDbQueryMetrics(eventInfo, *tdf);
            }
            break;
        }
        case GameManager::GameManagerMetricsMaster::EVENT_ENDPOINTDISCONNECTEDEVENT:
        {
            if (gPrintConnectionMetrics)
            {
                GameManager::DisconnectedEndpointData* tdf = (GameManager::DisconnectedEndpointData*)eventEnvelope.getEventData();
                if (tdf != nullptr)
                    processConnectionMetrics(eventInfo, *tdf);
            }
            break;
        }
        case UserSessionsSlave::EVENT_LOGINEVENT:
        case UserSessionsSlave::EVENT_LOGINFAILUREEVENT:
        {
            if (gPrintUserMetrics)
            {
                Login* tdf = (Login*)eventEnvelope.getEventData();
                if (tdf != nullptr)
                    processLoginEvent(timeBuf, *tdf, (eventId == UserSessionsSlave::EVENT_LOGINFAILUREEVENT));
            }
            break;
        }
        case UserSessionsSlave::EVENT_LOGOUTEVENT:
        case UserSessionsSlave::EVENT_DISCONNECTEVENT:
        {
            if (gPrintUserMetrics)
            {
                Logout* tdf = (Logout*)eventEnvelope.getEventData();
                if (tdf != nullptr)
                    processLogoutEvent(timeBuf, *tdf, (eventId == UserSessionsSlave::EVENT_DISCONNECTEVENT));
            }
            break;
        }
        case BlazeControllerSlave::EVENT_STARTUPEVENT:
        case BlazeControllerSlave::EVENT_SHUTDOWNEVENT:
        default:
            break;
        }
    }

    static int usage(const char8_t* prg)
    {
        printf("Usage: %s [-f inputFile] [-encode xml|print|terse] [-nolist] [-noenv] [-start startDate] [-end endDate] [-componentMetrics] [-userMetrics] [-fiberTimings]\n", prg);
        printf("    -f                  : Input file(s) to read (use system wildcards for processing multiple files).  If omitted then read from stdin\n");
        printf("    -encode             : Dictates what format to output in (xml, TDF print, or terse TDF print)\n");
        printf("    -nolist             : Don't print contents of file wrapped in an EventEnvelopeList\n");
        printf("    -noenv              : Mimmick output from the event manager by omitting the event envelope\n");
        printf("    -start              : Only print events newer than the date provided.\n");
        printf("    -end                : Only print events younger than the date provided.\n");
        printf("    -componentMetrics   : Print formatted component metrics events (from controller logs)\n");
        printf("    -rpcRateSummary     : Print formatted(CSV) summary of RPC rates (from controller logs) [use -f wildcards for combined summaries]\n");
        printf("    -rpcRateScale       : Used to scale the RPC rate values by a constant factor. e.g. -rpcRateScale 2.5\n");
        printf("    -fiberTimings       : Print formatted fiber timings events (from controller logs)\n");
        printf("    -status             : Only print status events (from controller logs)\n");
        printf("    -dbqueries[=prefix] : Only print DB query metrics events (from controller logs).  Strip given prefix from query name.\n");
        printf("    -connectionMetrics  : Only print game manager connection metrics\n");
        printf("    -userMetrics        : Only print user manager login/logout metrics\n");
        return 1;
    }

    static void processArgs(int32_t argc, char** argv)
    {
        for (int32_t idx = 1; idx < argc; ++idx)
        {
            if (strcmp(argv[idx], "-f") == 0)
            {
                ++idx;
                if (idx == argc)
                    exit(usage(argv[0]));
                gFilePath = argv[idx];
            }
            else if (strcmp(argv[idx], "-encode") == 0)
            {
                idx++;
                if (idx == argc)
                    exit(usage(argv[0]));
                if (strcmp(argv[idx], "xml") == 0)
                    gEncodeType = ENCODE_XML;
                else if (strcmp(argv[idx], "print") == 0)
                    gEncodeType = ENCODE_PRINT;
                else if (strcmp(argv[idx], "terse") == 0)
                    gEncodeType = ENCODE_TERSE;
                else
                {
                    fprintf(stderr, "Unrecognized encode type: '%s'\n", argv[idx]);
                    exit(1);
                }
            }
            else if (strcmp(argv[idx], "-nolist") == 0)
            {
                gPrintAsList = false;
            }
            else if (strcmp(argv[idx], "-noenv") == 0)
            {
                gPrintEnvelope = false;
                gPrintAsList = false;
            }
            else if (strcmp(argv[idx], "-start") == 0)
            {
                ++idx;
                if (idx == argc)
                    exit(usage(argv[0]));
                if (!gStartTime.parseGmDateTime(argv[idx]))
                {
                    fprintf(stderr, "Invalid date/time format for -start parameter.\n");
                    exit(1);
                }
            }
            else if (strcmp(argv[idx], "-end") == 0)
            {
                ++idx;
                if (idx == argc)
                    exit(usage(argv[0]));
                if (!gEndTime.parseGmDateTime(argv[idx]))
                {
                    fprintf(stderr, "Invalid date/time format for -end parameter.\n");
                    exit(1);
                }
            }
            else if (strcmp(argv[idx], "-componentMetrics") == 0)
            {
                gPrintControllerMetrics |= PRINT_COMPONENT_METRICS;
                gPrintAsList = false;
            }
            else if (strcmp(argv[idx], "-rpcRateSummary") == 0)
            {
                gPrintControllerMetrics |= PRINT_RPC_RATE_SUMMARY;
                gPrintAsList = false;
            }
            else if (strcmp(argv[idx], "-rpcRateScale") == 0)
            {
                ++idx;
                if (idx == argc || !(gPrintControllerMetrics & PRINT_RPC_RATE_SUMMARY))
                    exit(usage(argv[0]));
                gRpcRateScale = atof(argv[idx]);
            }
            else if (strcmp(argv[idx], "-fiberTimings") == 0)
            {
                gPrintControllerMetrics |= PRINT_FIBER_TIMINGS;
                gPrintAsList = false;
            }
            else if (strcmp(argv[idx], "-status") == 0)
            {
                gPrintControllerMetrics |= PRINT_STATUS;
                gPrintAsList = false;
            }
            else if (strstr(argv[idx], "-dbqueries") == argv[idx])
            {
                const char* s = argv[idx] + strlen("-dbqueries");
                if (*s == '=')
                {
                    ++s;
                    blaze_strnzcpy(gDbQueryNamePrefix, s, sizeof(gDbQueryNamePrefix));
                }
                gPrintControllerMetrics |= PRINT_DBQUERY_METRICS;
                gPrintAsList = false;
            }
            else if (strcmp(argv[idx], "-connectionMetrics") == 0)
            {
                gPrintConnectionMetrics = true;
                gPrintEnvelope = false;
                gPrintAsList = false;
            }
            else if (strcmp(argv[idx], "-userMetrics") == 0)
            {
                gPrintUserMetrics = true;
                gPrintEnvelope = false;
                gPrintAsList = false;
            }
            else
            {
                exit(usage(argv[0]));
            }
        }
    }

    static void printTdf(const EA::TDF::Tdf& tdf)
    {
        if (gEncodeType == ENCODE_XML)
        {
            RawBuffer printbuf(0);
            Xml2Encoder encoder;
            RpcProtocol::Frame frame;

            encoder.encode(printbuf, tdf, &frame);
            printf("%s", reinterpret_cast<char8_t*>(printbuf.data()));
        }
        else if (gEncodeType == ENCODE_PRINT)
        {
            StringBuilder s;
            printf("%s\n", (s << tdf).get());
        }
        else if (gEncodeType == ENCODE_TERSE)
        {
            StringBuilder s;
            printf("%s\n", (s << tdf).get());
        }
    }

    int processFile(const char8_t* fileName)
    {
        FILE* pFile = stdin;
        if (fileName != nullptr)
        {
            pFile = fopen(fileName, "r");
            if (pFile == nullptr)
            {
                fprintf(stderr, "Unable to open input file '%s': %s\n", fileName, strerror(errno));
                return 1;
            }
        }

        SET_BINARY_MODE(pFile);

        Event::EventEnvelopeTdfList eventEnvelopeList;

        size_t bufLen = 1024 * 1024 * 10; // 10 MB buffer
        uint8_t* buf = (uint8_t*)BLAZE_ALLOC(bufLen);

        while (!feof(pFile))
        {
            uint32_t seq = 0;
            uint32_t size = 0;

            // extract the sequence counter
            fread(reinterpret_cast<char8_t*>(&seq), 1, sizeof(uint32_t), pFile);
            seq = ntohl(seq);

            // extract the size of the event envelope
            fread(reinterpret_cast<char8_t*>(&size), 1, sizeof(uint32_t), pFile);
            size = ntohl(size);

            if (seq == 0 || seq == UINT32_MAX || size == 0 || size == UINT32_MAX)
                continue;

            if (bufLen < size)
            {
                BLAZE_FREE(buf);
                buf = (uint8_t*)BLAZE_ALLOC(size);
                bufLen = size;
            }

            RawBuffer raw(buf, bufLen);
            raw.put(size);

            // use the size to extract the event envelope data
            size_t rSize = fread(reinterpret_cast<char8_t*>(raw.data()), 1, size, pFile);

            if (rSize != size)
            {
                fprintf(stderr, "Attempted to read %d but only read %d\n", size, static_cast<int32_t>(rSize));
            }

            Event::EventEnvelope eventEnvelope;
            Heat2Decoder decoder;
            BlazeRpcError err = Blaze::ERR_OK;
            err = decoder.decode(raw, eventEnvelope);

            TimeValue eventTime = eventEnvelope.getTimestamp();
            if ((eventTime < gStartTime) || (eventTime > gEndTime))
            {
                continue;
            }

            if (err != Blaze::ERR_OK)
            {
                fprintf(stderr, "Failed to decode event (sequence=%d)\n", seq);
            }
            else if (gPrintAsList)
            {
                eventEnvelopeList.getEventEnvelopes().push_back(eventEnvelope.clone());
            }
            else if ((gPrintControllerMetrics != 0) || gPrintConnectionMetrics || gPrintUserMetrics)
            {
                processEvent(eventTime, eventEnvelope);
            }
            else
            {
                EA::TDF::Tdf* tdf = &eventEnvelope;
                if (!gPrintEnvelope)
                    tdf = eventEnvelope.getEventData();
                if (tdf != nullptr)
                    printTdf(*tdf);
            }
        }

        BLAZE_FREE(buf);

        fclose(pFile);

        if (gPrintAsList)
        {
            if (!eventEnvelopeList.getEventEnvelopes().empty())
                printTdf(eventEnvelopeList);
            eventEnvelopeList.getEventEnvelopes().release();
        }

        return 0;
    }

    static eastl::string parseServerNameFromBinLogName(const char8_t* logName)
    {
        eastl::string serverName(logName);
        const char8_t* begin = strchr(logName, '_');
        if (begin == nullptr)
            return serverName;
        ++begin;
        const char8_t* end = strchr(begin, '_');
        if (end == nullptr)
            return serverName;
        if (end == begin)
            return serverName;
        serverName.assign(begin, end);
        return serverName;
    }

    static void printRpcRateSummary()
    {
        if (!gServerRpcMetrics.empty())
        {
            typedef eastl::map<eastl::string, RpcRateMetrics> RpcMetricsByNameMap;
            RpcMetricsByNameMap componentRpcMetrics;
            RpcMetricsByNameMap commandRpcMetrics;
            RpcMetricsByNameMap serverRpcMetrics;
            eastl::string buf;

            // generate summary data
            for (ServerComponentRpcMetricsMap::const_iterator i = gServerRpcMetrics.begin(), e = gServerRpcMetrics.end(); i != e; ++i)
            {
                const char8_t* serverName = i->first.c_str();
                const ComponentRpcMetricsMap& cMap = i->second;
                for (ComponentRpcMetricsMap::const_iterator ci = cMap.begin(), ce = cMap.end(); ci != ce; ++ci)
                {
                    const uint16_t componentId = ci->first;
                    const char8_t* componentName = BlazeRpcComponentDb::getComponentNameById(componentId);
                    const RpcMetricsMap& rMap = ci->second;
                    for (RpcMetricsMap::const_iterator ri = rMap.begin(), re = rMap.end(); ri != re; ++ri)
                    {
                        const uint16_t commandId = ri->first;
                        const char8_t* commandName = BlazeRpcComponentDb::getCommandNameById(componentId, commandId);
                        const RpcRateMetrics& metrics = ri->second;
                        RpcRateMetrics& cMetrics = componentRpcMetrics[componentName];
                        buf.sprintf("%s,%s", componentName, commandName);
                        RpcRateMetrics& rMetrics = commandRpcMetrics[buf];
                        buf.sprintf("%s,%s,%s", serverName, componentName, commandName);
                        RpcRateMetrics& sMetrics = serverRpcMetrics[buf];
                        cMetrics.add(metrics);
                        rMetrics.add(metrics);
                        sMetrics.add(metrics);
                    }
                }
            }

            printf("[heat2logreader, sampled @ 1m, scaled x %f]\n", gRpcRateScale);
            printf("\n");

            printf("[per/component rpc rates]\n");
            printf("component,minrate,maxrate,avgrate\n");
            for (RpcMetricsByNameMap::const_iterator i = componentRpcMetrics.begin(), e = componentRpcMetrics.end(); i != e; ++i)
            {
                printf("%s,%f,%f,%f\n", i->first.c_str(), i->second.minimum*gRpcRateScale, i->second.maximum*gRpcRateScale, i->second.average*gRpcRateScale);
            }
            printf("\n");

            printf("[per/rpc rates]\n");
            printf("component,rpc,minrate,maxrate,avgrate\n");
            for (RpcMetricsByNameMap::const_iterator i = commandRpcMetrics.begin(), e = commandRpcMetrics.end(); i != e; ++i)
            {
                printf("%s,%f,%f,%f\n", i->first.c_str(), i->second.minimum*gRpcRateScale, i->second.maximum*gRpcRateScale, i->second.average*gRpcRateScale);
            }
            printf("\n");

            printf("[per/instance rates]\n");
            printf("server,component,rpc,minrate,maxrate,avgrate\n");
            for (RpcMetricsByNameMap::const_iterator i = serverRpcMetrics.begin(), e = serverRpcMetrics.end(); i != e; ++i)
            {
                printf("%s,%f,%f,%f\n", i->first.c_str(), i->second.minimum*gRpcRateScale, i->second.maximum*gRpcRateScale, i->second.average*gRpcRateScale);
            }
            printf("\n");
        }
    }

    int heat2logreader_main(int argc, char** argv)
    {
        // Initialize the game manager event objects in order to register the TDF implicitly
        GameManager::CreatedGameSession createdGameSession;
        GameManager::PlayerJoinedGameSession playerJoinedGameSession;
        GameManager::PlayerLeftGameSession playerLeftGameSession;
        GameManager::DestroyedGameSession destroyedGameSession;
        GameManager::GameSessionStateChanged GameSessionStateChanged;
#ifdef TARGET_gamereporting
        GameReporting::registerCustomReportTdfs();
#endif
        BlazeRpcComponentDb::initialize();

        processArgs(argc, argv);

        if (gFilePath != nullptr)
        {
            char16_t filePath[128];
            char16_t defaultDir[8];
            EA::StdC::Strlcpy(filePath, gFilePath, EAArrayCount(filePath));
            EA::StdC::Strlcpy(defaultDir, ".", EAArrayCount(defaultDir));
            EA::IO::Path::PathString16 dirstr(EA::IO::Path::GetDirectoryString(filePath));
            EA::IO::Path::PathString16 filestr(EA::IO::Path::GetFileNameString(filePath));
            EA::IO::EntryFindData efd;
            if (EA::IO::EntryFindFirst(dirstr.empty() ? defaultDir : dirstr.c_str(), filestr.empty() ? (const char16_t*) nullptr : filestr.c_str(), &efd))
            {
                do
                {
                    char8_t fileName[128];
                    char8_t fileDir[128];
                    EA::StdC::Strlcpy(fileName, efd.mName, EAArrayCount(fileName));
                    EA::StdC::Strlcpy(fileDir, efd.mDirectoryPath, EAArrayCount(fileDir));
                    EA::IO::Path::PathString8 filePath2(fileName);
                    EA::IO::Path::PathString8 fullPath(EA::IO::Path::GetDirectoryString(fileDir));
                    EA::IO::Path::Append(fullPath, filePath2);
                    eastl::string serverName = parseServerNameFromBinLogName(fileName);
                    gOutputPrefix = serverName.c_str();
                    processFile(fullPath.c_str());
                } while (EA::IO::EntryFindNext(&efd));
                EA::IO::EntryFindFinish(&efd);
            }
            else
            {
                fprintf(stderr, "Failed to open any files at '%s'\n", gFilePath);
                return 1;
            }
        }
        else
        {
            // process input from stdin
            processFile(nullptr);
        }

        if (gPrintControllerMetrics & PRINT_RPC_RATE_SUMMARY)
        {
            printRpcRateSummary();
        }

        return 0;
    }
}


