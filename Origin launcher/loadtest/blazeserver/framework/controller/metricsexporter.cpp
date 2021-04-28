/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/controller/metricsexporter.h"
#include "framework/controller/processcontroller.h"
#include "framework/connection/socketutil.h"

namespace Blaze
{

#define METRICS_EXPORT_IS_ENABLED(config) ((config).getComponentMetrics() || (config).getFiberTimings() || (config).getStatus() || (config).getDbMetrics())

class MetricsFormatter
{
public:
    virtual ~MetricsFormatter() { }

    virtual int32_t formatMetric(const char8_t* key, int64_t value, TimeValue& timestamp, char8_t* outbuf, size_t outlen) = 0;
    virtual int32_t formatMetric(const char8_t* key, uint64_t value, TimeValue& timestamp, char8_t* outbuf, size_t outlen) = 0;
    virtual int32_t formatMetric(const char8_t* key, double value, TimeValue& timestamp, char8_t* outbuf, size_t outlen) = 0;
    virtual int32_t formatMetric(const char8_t* key, const char8_t* value, TimeValue& timestamp, char8_t* outbuf, size_t outlen) = 0;

    int32_t formatMetric(const char8_t* key, int32_t value, TimeValue& timestamp, char8_t* outbuf, size_t outlen)
    {
        return formatMetric(key, (int64_t)value, timestamp, outbuf, outlen);
    }

    int32_t formatMetric(const char8_t* key, uint32_t value, TimeValue& timestamp, char8_t* outbuf, size_t outlen)
    {
        return formatMetric(key, (uint64_t)value, timestamp, outbuf, outlen);
    }
};

class GraphiteMetricsFormatter : public MetricsFormatter
{
public:
    int32_t formatMetric(const char8_t* key, int64_t value, TimeValue& timestamp, char8_t* outbuf, size_t outlen) override
    {
        return blaze_snzprintf(outbuf, outlen, "%s %" PRId64 " %" PRId64 "\n", key, value, timestamp.getSec());
    }

    int32_t formatMetric(const char8_t* key, uint64_t value, TimeValue& timestamp, char8_t* outbuf, size_t outlen) override
    {
        return blaze_snzprintf(outbuf, outlen, "%s %" PRIu64 " %" PRId64 "\n", key, value, timestamp.getSec());
    }

    int32_t formatMetric(const char8_t* key, double value, TimeValue& timestamp, char8_t* outbuf, size_t outlen) override
    {
        return blaze_snzprintf(outbuf, outlen, "%s %g %" PRId64 "\n", key, value, timestamp.getSec());
    }

    int32_t formatMetric(const char8_t* key, const char8_t* value, TimeValue& timestamp, char8_t* outbuf, size_t outlen) override
    {
        return blaze_snzprintf(outbuf, outlen, "%s %s %" PRId64 "\n", key, value, timestamp.getSec());
    }
};

class StatsdMetricsFormatter : public MetricsFormatter
{
public:
    int32_t formatMetric(const char8_t* key, int64_t value, TimeValue& timestamp, char8_t* outbuf, size_t outlen) override
    {
        return blaze_snzprintf(outbuf, outlen, "%s:%" PRId64 "|g\n", key, value);
    }

    int32_t formatMetric(const char8_t* key, uint64_t value, TimeValue& timestamp, char8_t* outbuf, size_t outlen) override
    {
        return blaze_snzprintf(outbuf, outlen, "%s:%" PRIu64 "|g\n", key, value);
    }

    int32_t formatMetric(const char8_t* key, double value, TimeValue& timestamp, char8_t* outbuf, size_t outlen) override
    {
        return blaze_snzprintf(outbuf, outlen, "%s:%g|g\n", key, value);
    }

    int32_t formatMetric(const char8_t* key, const char8_t* value, TimeValue& timestamp, char8_t* outbuf, size_t outlen) override
    {
        return blaze_snzprintf(outbuf, outlen, "%s:%s|g\n", key, value);
    }
};

// static
bool MetricsExporter::validateConfig(const MetricsExportConfig& config, ConfigureValidationErrors& validationErrors)
{
    if (METRICS_EXPORT_IS_ENABLED(config))
    {
        // At least one of the metrics types is configured to be exported so ensure that
        // we have a valid injection target
        const char8_t* hostname = config.getHostname();
        if ((hostname != nullptr) && (hostname[0] != '\0'))
        {
            // A hostname is configured to check to make sure we can resolve it
            InetAddress addr(config.getHostname(), (uint16_t)config.getPort());
            NameResolver::LookupJobId resolveId;
            BlazeRpcError sleepErr = gNameResolver->resolve(addr, resolveId);
            if (sleepErr != ERR_OK)
            {
                eastl::string msg;
                msg.sprintf("[MetricsExporter].validateConfig: Unable to resolve hostname '%s': %s",
                        config.getHostname(), ErrorHelp::getErrorName(sleepErr));
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
                return false;
            }
        }
    }
    return true;
}

void MetricsExporter::exportMetric(const char8_t* key, int64_t value, TimeValue& timestamp)
{
    char8_t buffer[4096];
    int32_t bytes = mFormatter->formatMetric(key, value, timestamp, buffer, sizeof(buffer));
    if (bytes > 0)
        mInjector->inject(buffer, bytes);
}

void MetricsExporter::exportMetric(const char8_t* key, uint64_t value, TimeValue& timestamp)
{
    char8_t buffer[4096];
    int32_t bytes = mFormatter->formatMetric(key, value, timestamp, buffer, sizeof(buffer));
    if (bytes > 0)
        mInjector->inject(buffer, bytes);
}

void MetricsExporter::exportMetric(const char8_t* key, double value, TimeValue& timestamp)
{
    char8_t buffer[4096];
    int32_t bytes = mFormatter->formatMetric(key, value, timestamp, buffer, sizeof(buffer));
    if (bytes > 0)
        mInjector->inject(buffer, bytes);
}

void MetricsExporter::exportMetric(const char8_t* key, const char8_t* value, TimeValue& timestamp)
{
    char8_t buffer[4096];
    int32_t bytes = mFormatter->formatMetric(key, value, timestamp, buffer, sizeof(buffer));
    if (bytes > 0)
        mInjector->inject(buffer, bytes);
}

MetricsExporter::MetricsExporter(const char8_t* serviceName, const char8_t* instanceName, const char8_t* buildLocation, const MetricsExportConfig& config)
    : mConfig(config),
      mServiceName(serviceName),
      mInstanceName(instanceName),
      mBuildLocation(buildLocation),
      mFormatter(nullptr),
      mInjector(nullptr)
{
    setInstanceName(instanceName);

    switch (config.getFormat())
    {
        case MetricsExportConfig::STATSD:
            mFormatter = BLAZE_NEW StatsdMetricsFormatter;
            break;

        case MetricsExportConfig::GRAPHITE:
        default:
            mFormatter = BLAZE_NEW GraphiteMetricsFormatter;
            break;
    }

    switch (config.getInjector())
    {
        case MetricsExportConfig::UDP:
            mInjector = BLAZE_NEW UdpMetricsInjector(config.getHostname(), config.getPort());
            break;
        case MetricsExportConfig::TCP:
        default:
            mInjector = BLAZE_NEW TcpMetricsInjector(config.getHostname(), config.getPort());
            break;
    }
}

MetricsExporter::~MetricsExporter()
{
    delete mFormatter;
    delete mInjector;
}

void MetricsExporter::setInstanceName(const char8_t* instanceName)
{
    mInstanceName = instanceName;
    mPrefix = mConfig.getPrefix();
    if (mPrefix.length() > 0)
        mPrefix += ".";
    mPrefix.append_sprintf("%s.%s", mServiceName.c_str(), mInstanceName.c_str());
}


bool MetricsExporter::isExportEnabled() const
{
    return METRICS_EXPORT_IS_ENABLED(mConfig);
}

void MetricsExporter::setInjector(Injector* injector)
{
    delete mInjector;
    mInjector = injector;
}

void MetricsExporter::beginExport()
{
    if (!isExportEnabled())
        return;

    if (!mInjector->connect())
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "MetricsExporter.beginExport: unable to connect to metrics injection target '" << mConfig.getHostname() << ":" << mConfig.getPort() << "'");
    }
}

void MetricsExporter::endExport()
{
    mInjector->disconnect();
}

const char8_t* MetricsExporter::buildKey(char8_t* buf, size_t buflen, const char8_t* format, ...)
{
    va_list args;
    va_start(args, format);
    EA::StdC::Vsnprintf(buf, buflen, format, args);
    va_end(args);

    for(char8_t* b = buf; *b != '\0'; ++b)
    {
        if ((b[0] == ':') || (b[0] == '|') || (b[0] == '@') || (b[0] == '/'))
            b[0] = '_';
    }
    return buf;
}

void MetricsExporter::exportComponentMetrics(TimeValue timestamp, ComponentMetricsResponse& metrics)
{
    char8_t keybuf[2048];
    for(auto itr = metrics.getMetrics().begin(), end = metrics.getMetrics().end(); itr != end; ++itr)
    {
        CommandMetricInfo& m = **itr;
        if (m.getCount() > 0)
        {
            char keyRoot[1024];
            blaze_snzprintf(keyRoot, sizeof(keyRoot), "%s.component.%s.rpc.%s", mPrefix.c_str(), m.getComponentName(), m.getCommandName());

            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.count", keyRoot), m.getCount(), timestamp);
            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.successcount", keyRoot), m.getSuccessCount(), timestamp);
            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.failcount", keyRoot), m.getFailCount(), timestamp);
            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.authfailcount", keyRoot), m.getAuthorizationFailureCount(), timestamp);
            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totaltime", keyRoot), m.getTotalTime(), timestamp);

            if (m.getGetConnTime() > 0)
                exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.conntime", keyRoot), m.getGetConnTime(), timestamp);
            if (m.getTxnCount() > 0)
                exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.txncount", keyRoot), m.getTxnCount(), timestamp);
            if (m.getQueryCount() > 0)
                exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.querycount", keyRoot), m.getQueryCount(), timestamp);
            if (m.getMultiQueryCount() > 0)
                exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.multiquerycount", keyRoot), m.getMultiQueryCount(), timestamp);
            if (m.getPreparedStatementCount() > 0)
                exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.preparedstatementcount", keyRoot), m.getPreparedStatementCount(), timestamp);
            if (m.getQueryTime() > 0)
                exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.querytime", keyRoot), m.getQueryTime(), timestamp);
            if (m.getQuerySetupTimeOnThread() > 0)
                exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.querysetuptimeonthread", keyRoot), m.getQuerySetupTimeOnThread(), timestamp);
            if (m.getQueryExecutionTimeOnThread() > 0)
                exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.queryexecutiontimeonthread", keyRoot), m.getQueryExecutionTimeOnThread(), timestamp);
            if (m.getQueryCleanupTimeOnThread() > 0)
                exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.querycleanuptimeonthread", keyRoot), m.getQueryCleanupTimeOnThread(), timestamp);

            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.fibertime", keyRoot), m.getFiberTime(), timestamp);
        }
    }

    for(auto itr = metrics.getMetrics().begin(), end = metrics.getMetrics().end(); itr != end; ++itr)
    {
        CommandMetricInfo& m = **itr;
        if (m.getCount() > 0)
        {
            for (auto i = m.getErrorCountMap().begin(), e = m.getErrorCountMap().end(); i != e; ++i)
            {
                if (i->second > 0)
                {
                    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.component.%s.rpc.%s.errors.%s", mPrefix.c_str(), m.getComponentName(), m.getCommandName(), i->first.c_str()), i->second, timestamp);
                }
            }
        }
    }


}

void MetricsExporter::exportFiberTimings(TimeValue timestamp, FiberTimings& fiberTimings)
{
    char8_t keybuf[2048];
    for(auto itr = fiberTimings.getFiberMetrics().begin(), end = fiberTimings.getFiberMetrics().end(); itr != end; ++itr)
    {
        FiberAccountingEntry* entry = itr->second;

        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.fibers.%s.time", mPrefix.c_str(), itr->first.c_str()), entry->getFiberTimeMicroSecs(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.fibers.%s.count", mPrefix.c_str(), itr->first.c_str()), entry->getCount(), timestamp);
    }

}

bool MetricsExporter::isNumber(const char8_t* value)
{
    if (value == nullptr)
        return false;

    for(const char8_t* v = value; *v != '\0'; ++v)
    {
        if (!isdigit(*v))
            return false;
    }
    return true;
}

void MetricsExporter::exportComponentStatusInfoMap(TimeValue timestamp, const char8_t* componentName, const char8_t* serviceName, ComponentStatus::InfoMap& info)
{
    char8_t keybuf[2048];
    for(auto itr = info.begin(), end = info.end(); itr != end; ++itr)
    {
        if (isNumber(itr->second.c_str()))
        {
            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.component.%s.status.%s%s%s", mPrefix.c_str(), componentName,
                    serviceName != nullptr ? serviceName : "", serviceName != nullptr ? "." : "",
                    itr->first.c_str()), itr->second.c_str(), timestamp);
        }
    }
}

const char8_t* MetricsExporter::replaceSeparator(const char8_t* src, char8_t* dst, size_t dstlen)
{
    dst[dstlen-1] = '\0';
    size_t len = 0;
    for(; (src[len] != '\0') && (len < dstlen); ++len)
    {
        if (src[len] == '.')
            dst[len] = '_';
        else
            dst[len] = src[len];
    }
    if (len < dstlen)
        dst[len] = '\0';
    return dst;
}

void MetricsExporter::exportDbInstancePoolStatus(TimeValue timestamp, const char8_t* database, const char8_t* name, DbInstancePoolStatus& status)
{
    char8_t keybuf[2048];
    char8_t host[1024];
    replaceSeparator(status.getHost(), host, sizeof(host));

    char keyRoot[1024];
    blaze_snzprintf(keyRoot, sizeof(keyRoot), "%s.databases.connectionpools.%s.%s.%s", mPrefix.c_str(), database, name, host);

    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.total_conns", keyRoot), status.getTotalConnections(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.active_conns", keyRoot), status.getActiveConnections(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.peak_conns", keyRoot), status.getPeakConnections(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.total_available_conns", keyRoot), status.getTotalAvailableConnections(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.total_errors", keyRoot), status.getTotalErrors(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.total_queries", keyRoot), status.getTotalQueries(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.total_multiqueries", keyRoot), status.getTotalMultiQueries(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.total_preparedstatement_queries", keyRoot), status.getTotalPreparedStatementQueries(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.total_commits", keyRoot), status.getTotalCommits(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.total_rollbacks", keyRoot), status.getTotalRollbacks(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.querytime_avg_ms", keyRoot), status.getQueryTimeAverageMs(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.querytime_dev_ms", keyRoot), status.getQueryTimeDeviationMs(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.querytime_baseline_avg_ms", keyRoot), status.getQueryTimeBaselineAverageMs(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.querytime_baseline_dev_ms", keyRoot), status.getQueryTimeBaselineDeviationMs(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.total_querytime_thresholdcrosses", keyRoot), status.getTotalQueryTimeThresholdCrosses(), timestamp);
}

void MetricsExporter::exportStatus(TimeValue timestamp, ServerStatus& status)
{
    char8_t keybuf[2048];
    char8_t keyRoot[1024];

    // Component status
    for(auto itr = status.getComponents().begin(), end = status.getComponents().end(); itr != end; ++itr)
    {
        ComponentStatus* compStatus = *itr;
        exportComponentStatusInfoMap(timestamp, compStatus->getComponentName(), nullptr, compStatus->getInfo());

        for (auto i = compStatus->getInfoByProductName().begin(), e = compStatus->getInfoByProductName().end(); i != e; ++i)
        {
            exportComponentStatusInfoMap(timestamp, compStatus->getComponentName(), i->first.c_str(), *i->second);
        }
        for (auto i = compStatus->getInfoByServiceName().begin(), e = compStatus->getInfoByServiceName().end(); i != e; ++i)
        {
            exportComponentStatusInfoMap(timestamp, compStatus->getComponentName(), i->first.c_str(), *i->second);
        }
    }

    // Logger metrics
    for(auto itr = status.getLoggerMetrics().getCategories().begin(), end = status.getLoggerMetrics().getCategories().end(); itr != end; ++itr)
    {
        for(auto i = itr->second->begin(), e = itr->second->end(); i != e; ++i)
        {
            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.logger.%s.%s", mPrefix.c_str(), itr->first.c_str(), i->first.c_str()), i->second, timestamp);
        }
    }

    // Database metrics
    DatabaseStatus dbStatus = status.getDatabase();
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.databases.activeworkerthreads", mPrefix.c_str()), dbStatus.getActiveWorkerThreads(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.databases.totalworkerthreads", mPrefix.c_str()), dbStatus.getTotalWorkerThreads(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.databases.peakworkerthreads", mPrefix.c_str()), dbStatus.getPeakWorkerThreads(), timestamp);
    for(auto itr = dbStatus.getConnectionPools().begin(), end = dbStatus.getConnectionPools().end(); itr != end; ++itr)
    {
        DbConnPoolStatus& dbConnStatus = *itr->second;
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.databases.connectionpools.%s.%s.queuedfibersmaster", mPrefix.c_str(), dbConnStatus.getDatabase(), dbConnStatus.getName()), dbConnStatus.getQueuedFibersMaster(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.databases.connectionpools.%s.%s.queuedfibersslave", mPrefix.c_str(), dbConnStatus.getDatabase(), dbConnStatus.getName()), dbConnStatus.getQueuedFibersSlaves(), timestamp);

        exportDbInstancePoolStatus(timestamp, dbConnStatus.getDatabase(), dbConnStatus.getName(), dbConnStatus.getMaster());

        for(auto i = dbConnStatus.getSlaves().begin(), e = dbConnStatus.getSlaves().end(); i != e; ++i)
        {
            exportDbInstancePoolStatus(timestamp, dbConnStatus.getDatabase(), dbConnStatus.getName(), **i);
        }
    }

    // ConnectionManager status
    ConnectionManagerStatus& connMgrStatus = status.getConnectionManager();
    blaze_snzprintf(keyRoot, sizeof(keyRoot), "%s.connmanager", mPrefix.c_str());

    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.inboundtotalrpcfiberlimitexceededdropped", keyRoot), connMgrStatus.getInboundTotalRpcFiberLimitExceededDropped(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.inboundtotalrpcfiberlimitexceededpermitted", keyRoot), connMgrStatus.getInboundTotalRpcFiberLimitExceededPermitted(), timestamp);

    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalrejectmaxinboundconnections", keyRoot), connMgrStatus.getTotalRejectMaxInboundConnections(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalrejectmaxratelimitinboundconnections", keyRoot), connMgrStatus.getTotalRejectMaxRateLimitInboundConnections(), timestamp);

    for(auto itr = connMgrStatus.getEndpointDrainStatus().begin(), end = connMgrStatus.getEndpointDrainStatus().end(); itr != end; ++itr)
    {
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.endpointdrainstatus.%s", keyRoot, itr->first.c_str()), itr->second, timestamp);
    }

    for(auto itr = connMgrStatus.getEndpointsStatus().begin(), end = connMgrStatus.getEndpointsStatus().end(); itr != end; ++itr)
    {
        blaze_snzprintf(keyRoot, sizeof(keyRoot), "%s.connmanager.endpointstatus.%s", mPrefix.c_str(), (*itr)->getName());
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.connectionnum", keyRoot), (*itr)->getConnectionNum(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.maxconnectionnum", keyRoot), (*itr)->getMaxConnectionNum(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.highwatermarks", keyRoot), (*itr)->getHighWatermarks(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalhighwatermarks", keyRoot), (*itr)->getTotalHighWatermarks(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalhighwatermarkdisconnects", keyRoot), (*itr)->getTotalHighWatermarkDisconnects(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalresumedconnections", keyRoot), (*itr)->getTotalResumedConnections(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalresumedconnectionsfailed", keyRoot), (*itr)->getTotalResumedConnectionsFailed(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalrpcauthorizationfailurecount", keyRoot), (*itr)->getTotalRpcAuthorizationFailureCount(), timestamp);
    }

    // InboundGrpcManager status
    GrpcManagerStatus& grpcMgrStatus = status.getGrpcManagerStatus();
    blaze_snzprintf(keyRoot, sizeof(keyRoot), "%s.grpcmanager", mPrefix.c_str());

    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.inboundtotalrpcfiberlimitexceededdropped", keyRoot), grpcMgrStatus.getInboundTotalRpcFiberLimitExceededDropped(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.inboundtotalrpcfiberlimitexceededpermitted", keyRoot), grpcMgrStatus.getInboundTotalRpcFiberLimitExceededPermitted(), timestamp);

    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalrejectmaxinboundconnections", keyRoot), grpcMgrStatus.getTotalRejectMaxInboundConnections(), timestamp);

    for (auto& grpcEndpointStatus : grpcMgrStatus.getGrpcEndpointsStatus())
    {
        blaze_snzprintf(keyRoot, sizeof(keyRoot), "%s.grpcmanager.grpcendpointstatus.%s", mPrefix.c_str(), grpcEndpointStatus->getName());
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.connectionnum", keyRoot), grpcEndpointStatus->getConnectionNum(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.maxconnectionnum", keyRoot), grpcEndpointStatus->getMaxConnectionNum(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalrpcauthorizationfailurecount", keyRoot), grpcEndpointStatus->getTotalRpcAuthorizationFailureCount(), timestamp);
    }

    // Redis status
    RedisDatabaseStatus redisStatus = status.getRedisDatabase();
    for(auto itr = redisStatus.getClusterInfos().begin(), end = redisStatus.getClusterInfos().end(); itr != end; ++itr)
    {
        blaze_snzprintf(keyRoot, sizeof(keyRoot), "%s.redis.%s", mPrefix.c_str(), itr->first.c_str());
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.all_nodes", keyRoot), itr->second->getAllNodes(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.commands", keyRoot), itr->second->getCommands(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.bytes_sent", keyRoot), itr->second->getBytesSent(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.bytes_recvd", keyRoot), itr->second->getBytesRecv(), timestamp);
        for (auto errIt = itr->second->getErrorCounts().begin(); errIt != itr->second->getErrorCounts().end(); ++errIt)
            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.error_counts.%s", keyRoot, errIt->first.c_str()), errIt->second, timestamp);
    }

    // Memory stats
    MemoryStatus& memStatus = status.getMemory();
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.mem_resident_bytes", mPrefix.c_str()), status.getMemory().getProcessResidentMemory(), timestamp);

    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.memory.totals.used_memory", mPrefix.c_str()), status.getMemory().getTotals().getUsedMemory(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.memory.totals.max_used_memory", mPrefix.c_str()), status.getMemory().getTotals().getMaxUsedMemory(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.memory.totals.allocated_core_memory", mPrefix.c_str()), status.getMemory().getTotals().getAllocatedCoreMemory(), timestamp);

    for(auto itr = memStatus.getPerAllocatorStats().begin(), end = memStatus.getPerAllocatorStats().end(); itr != end; ++itr)
    {
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.memory.%s.used_memory", mPrefix.c_str(), (*itr)->getName()), (*itr)->getStats().getUsedMemory(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.memory.%s.max_used_memory", mPrefix.c_str(), (*itr)->getName()), (*itr)->getStats().getMaxUsedMemory(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.memory.%s.allocated_core_memory", mPrefix.c_str(), (*itr)->getName()), (*itr)->getStats().getAllocatedCoreMemory(), timestamp);
    }

    // Fiber and CPU utilization
    FiberManagerStatus& fiberStatus = status.getFiberManagerStatus();
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.cpu_pct", mPrefix.c_str()), fiberStatus.getCpuUsageForProcessPercent(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.fibermgr.semaphores.used", mPrefix.c_str()), fiberStatus.getUsedSemaphoreMapSize(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.fibermgr.semaphores.unused", mPrefix.c_str()), fiberStatus.getUnusedSemaphoreListSize(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.fibermgr.semaphores.idcounter", mPrefix.c_str()), fiberStatus.getSemaphoreIdCounter(), timestamp);
    for(auto itr = fiberStatus.getFiberPoolInfoList().begin(), end = fiberStatus.getFiberPoolInfoList().end(); itr != end; ++itr)
    {
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.fibermgr.pool.%s.used", mPrefix.c_str(), Fiber::StackSizeToString((Fiber::StackSize)(*itr)->getStackSize())), (*itr)->getUsedFiberListSize(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.fibermgr.pool.%s.pooled", mPrefix.c_str(), Fiber::StackSizeToString((Fiber::StackSize)(*itr)->getStackSize())), (*itr)->getNumPooledFibers(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.fibermgr.pool.%s.maxpooled", mPrefix.c_str(), Fiber::StackSizeToString((Fiber::StackSize)(*itr)->getStackSize())), (*itr)->getMaxPooledFibers(), timestamp);
    }

    // Uptime
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.uptime", mPrefix.c_str()), status.getUptime(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.config_uptime", mPrefix.c_str()), status.getConfigUptime(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.utilization.inservice", mPrefix.c_str()), (int32_t)status.getInService(), timestamp);

    // Selector status
    const SelectorStatus& selector = status.getSelector();
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.timerqueuetasks", mPrefix.c_str()), selector.getTimerQueueTasks(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.timerqueuetime", mPrefix.c_str()), selector.getTimerQueueTime(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.timerqueuesize", mPrefix.c_str()), selector.getTimerQueueSize(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.jobqueuetasks", mPrefix.c_str()), selector.getJobQueueTasks(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.jobqueuetime", mPrefix.c_str()), selector.getJobQueueTime(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.networkpolls", mPrefix.c_str()), selector.getNetworkPolls(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.networktasks", mPrefix.c_str()), selector.getNetworkTasks(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.networktime", mPrefix.c_str()), selector.getNetworkTime(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.networkprioritypolls", mPrefix.c_str()), selector.getNetworkPriorityPolls(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.networkprioritytasks", mPrefix.c_str()), selector.getNetworkPriorityTasks(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.networkprioritytime", mPrefix.c_str()), selector.getNetworkPriorityTime(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.selects", mPrefix.c_str()), selector.getSelects(), timestamp);
    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.selector.wakes", mPrefix.c_str()), selector.getWakes(), timestamp);

    // Http service status
    HttpServiceStatusMap& httpStatusMap = status.getHttpServiceStatus();
    for(auto itr = httpStatusMap.begin(), end = httpStatusMap.end(); itr != end; ++itr)
    {
        HttpServiceStatus* httpStatus = itr->second;
        OutboundConnMgrStatus& connMgr = httpStatus->getConnMgrStatus();
        char keybufTemp[2048];
        char keyRootTemp[1024];
        blaze_snzprintf(keyRootTemp, sizeof(keyRootTemp), "%s.httpsvc.%s", mPrefix.c_str(), itr->first.c_str());
        exportMetric(buildKey(keybufTemp, sizeof(keybufTemp), "%s.totalconns", keyRootTemp), connMgr.getTotalConnections(), timestamp);
        exportMetric(buildKey(keybufTemp, sizeof(keybufTemp), "%s.activeconns", keyRootTemp), connMgr.getActiveConnections(), timestamp);
        exportMetric(buildKey(keybufTemp, sizeof(keybufTemp), "%s.peakconns", keyRootTemp), connMgr.getPeakConnections(), timestamp);
        exportMetric(buildKey(keybufTemp, sizeof(keybufTemp), "%s.maxconns", keyRootTemp), connMgr.getMaxConnections(), timestamp);
        exportMetric(buildKey(keybufTemp, sizeof(keybufTemp), "%s.totaltimeouts", keyRootTemp), connMgr.getTotalTimeouts(), timestamp);

        for(auto errCodeItr = httpStatus->getCommandErrorCodeList().begin(), errCodeEnd = httpStatus->getCommandErrorCodeList().end(); errCodeItr != errCodeEnd; ++errCodeItr)
        {
            HttpServiceStatus::CommandErrorCodes* codes = *errCodeItr;
            for(auto errCodeCountItr = codes->getErrorCodeCountMap().begin(), errCodeCountEnd = codes->getErrorCodeCountMap().end(); errCodeCountItr != errCodeCountEnd; ++errCodeCountItr)
            {
                exportMetric(buildKey(keybufTemp, sizeof(keybufTemp), "%s.errors.%s.%d", keyRootTemp, codes->getCommand(), errCodeCountItr->first), errCodeCountItr->second, timestamp);
            }
        }
    }
}

void MetricsExporter::exportOutboundMetrics(TimeValue timestamp, OutboundMetrics& metrics)
{
    char8_t keybuf[2048];
    char8_t keyRoot[1024];

    for(auto itr = metrics.getOutboundMetricsMap().begin(), end = metrics.getOutboundMetricsMap().end(); itr != end; ++itr)
    {
        const char8_t* typeStr;
        switch (itr->first)
        {
            case OutboundTransactionType::HTTP:
                typeStr = "http";
                break;
            case OutboundTransactionType::REDIS:
                typeStr = "redis";
                break;
            default:
                continue;
        }
        OutboundMetricsList* l = itr->second;
        for(auto il = l->begin(), el = l->end(); il != el; ++il)
        {
            OutboundMetricsEntry* entry = *il;
            char8_t pattern[1024];
            replaceSeparator(entry->getResourcePattern(), pattern, sizeof(pattern));
            blaze_snzprintf(keyRoot, sizeof(keyRoot), "%s.outboundmetrics.%s.%s", mPrefix.c_str(), typeStr, pattern);
            auto citr = entry->getCounts().begin();
            auto cend = entry->getCounts().end();
            for(int32_t idx = 1; citr != cend; ++citr, ++idx)
            {
                uint64_t count = *citr;
                if (count > 0)
                {
                    exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.count_%dms", keyRoot, idx * entry->getBucketSize().getMillis()), *citr, timestamp);
                }
            }
        }
    }
}

void MetricsExporter::exportStorageManagerMetrics(TimeValue timestamp, GetStorageMetricsResponse& metrics)
{
    char8_t keybuf[2048];
    char8_t keyRoot[1024];

    for (const auto& storageTable : metrics.getTableMetrics())
    {
        const char8_t* sliverNamespace = storageTable->getSliverNamespaceStr();
        blaze_snzprintf(keyRoot, sizeof(keyRoot), "%s.storagetable.%s", mPrefix.c_str(), sliverNamespace);

        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalfieldchanges", keyRoot), storageTable->getTotalFieldChanges(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalfieldignores", keyRoot), storageTable->getTotalFieldIgnores(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalfieldupdates", keyRoot), storageTable->getTotalFieldUpdates(), timestamp);

        for (const auto& fiberChanges : storageTable->getChangesByFiber())
        {
            const char8_t* fiberName = fiberChanges.first;
            const uint64_t count = fiberChanges.second;

            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.changesbyfiber.%s", keyRoot, fiberName), count, timestamp);
        }

        for (const auto& fieldChanges : storageTable->getChangesByField())
        {
            const char8_t* fieldName = fieldChanges.first;
            const uint64_t count = fieldChanges.second;

            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.changesbyfeild.%s", keyRoot, fieldName), count, timestamp);
        }

        for (const auto& fieldUpdates : storageTable->getUpdatesByField())
        {
            const char8_t* fieldName = fieldUpdates.first;
            const uint64_t count = fieldUpdates.second;

            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.updatesbyfield.%s", keyRoot, fieldName), count, timestamp);
        }

        for (const auto& fieldIgnores : storageTable->getIgnoresByField())
        {
            const char8_t* fieldName = fieldIgnores.first;
            const uint64_t count = fieldIgnores.second;

            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.ignoresbyfield.%s", keyRoot, fieldName), count, timestamp);
        }
    }
}

void MetricsExporter::exportDbMetrics(TimeValue timestamp, DbQueryMetrics& metrics)
{
    char8_t keybuf[2048];
    char8_t keyRoot[1024];

    char8_t queryPrefix[1024];
    int32_t queryPrefixLen = 0;
    const char8_t* buildLocation = mBuildLocation.c_str();
    const char8_t* s = strchr(buildLocation, ':');
    if (s == nullptr)
        queryPrefix[0] = '\0';
    else
        queryPrefixLen = blaze_snzprintf(queryPrefix, sizeof(queryPrefix), "%s%c", s + 1, '/');

    for(auto itr = metrics.getDbQueryAccountingInfos().begin(), end = metrics.getDbQueryAccountingInfos().end(); itr != end; ++itr)
    {
        DbQueryAccountingInfo* info = itr->second;
        const char8_t* name = itr->first.c_str();

        // Strip off the build location if present to keep the query strings short
        if ((queryPrefix[0] != '\0') && (strstr(name, queryPrefix) == name))
            name += queryPrefixLen;

        char8_t queryKey[512];
        replaceSeparator(name, queryKey, sizeof(queryKey));
        blaze_snzprintf(keyRoot, sizeof(keyRoot), "%s.dbqueries.%s", mPrefix.c_str(), queryKey);

        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totalcount", keyRoot), info->getTotalCount(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.successcount", keyRoot), info->getSuccessCount(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.slowquerycount", keyRoot), info->getSlowQueryCount(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.failcount", keyRoot), info->getFailCount(), timestamp);
        exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.totaltime", keyRoot), info->getTotalTime(), timestamp);

        for(auto ctxItr = info->getFiberCallCounts().begin(), ctxEnd = info->getFiberCallCounts().end(); ctxItr != ctxEnd; ++ctxItr)
        {
            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.callers.%s", keyRoot, ctxItr->first.c_str()), ctxItr->second, timestamp);
        }
        for(auto errItr = info->getErrorCounts().begin(), errEnd = info->getErrorCounts().end(); errItr != errEnd; ++errItr)
        {
            exportMetric(buildKey(keybuf, sizeof(keybuf), "%s.errors.%s", keyRoot, errItr->first.c_str()), errItr->second, timestamp);
        }
    }
}

UdpMetricsInjector::UdpMetricsInjector(const char8_t* hostname, uint16_t port)
    : MetricsExporter::Injector(hostname, port),
      mAddr(nullptr),
      mSok(INVALID_SOCKET)
{
    memset(&mSockAddr, 0, sizeof(mSockAddr));
}

UdpMetricsInjector::UdpMetricsInjector(const InetAddress& agentAddress)
    : MetricsExporter::Injector(agentAddress.getHostname(), agentAddress.getPort(InetAddress::Order::HOST)),
    mAddr(BLAZE_NEW InetAddress(agentAddress)),
    mSok(INVALID_SOCKET)
{
    memset(&mSockAddr, 0, sizeof(mSockAddr));
}

UdpMetricsInjector::~UdpMetricsInjector()
{
    disconnect();
    delete mAddr;
}

bool UdpMetricsInjector::connect()
{
    if (mSok != INVALID_SOCKET)
        disconnect();

    if (mAddr == nullptr)
    {
        mAddr = BLAZE_NEW InetAddress(mHostname.c_str(), mPort);
        NameResolver::LookupJobId resolveId;
        BlazeRpcError sleepErr = gNameResolver->resolve(*mAddr, resolveId);
        if (sleepErr != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "UdpMetricsInjector.connect: failed to resolve to '" << *mAddr << "': rc=" << ErrorHelp::getErrorName(sleepErr));
            delete mAddr;
            mAddr = nullptr;
            return false;
        }
    }

    mSok = SocketUtil::createSocket(false, SOCK_DGRAM, IPPROTO_UDP);
    mAddr->getSockAddr(mSockAddr);
    ::connect(mSok, (struct sockaddr*)&mSockAddr, sizeof(mSockAddr));

    return true;
}

void UdpMetricsInjector::disconnect()
{
    if (mSok != INVALID_SOCKET)
    {
        closesocket(mSok);
        mSok = INVALID_SOCKET;
    }
    memset(&mSockAddr, 0, sizeof(mSockAddr));
}

void UdpMetricsInjector::inject(const char8_t* metric, size_t len)
{
    if (mSok == INVALID_SOCKET)
    {
        if (!connect())
            return;
    }

    ssize_t bs = sendto(mSok, metric, len, 0, (struct sockaddr*)&mSockAddr, sizeof(mSockAddr));
    if (bs != (ssize_t)len)
    {
        disconnect();
    }
}

#ifdef EA_PLATFORM_LINUX

void UdpMetricsInjector::inject(const iovec_blaze* iov, int iovcnt)
{
    if (mSok == INVALID_SOCKET)
    {
        if (!connect())
            return;
    }

    if (writev(mSok, iov, iovcnt) == -1)
        disconnect();

}

#else 

void UdpMetricsInjector::inject(const iovec_blaze* iov, int32_t iovcnt)
{
    if (mSok == INVALID_SOCKET)
    {
        if (!connect())
            return;
    }

    // Windows doesn't have writev so simulate it.
    size_t totalSize = 0;
    for(int32_t i = 0; i < iovcnt; ++i)
        totalSize += iov[i].iov_len;

    char8_t* outbuf = BLAZE_NEW_ARRAY(char8_t, totalSize);
    size_t len = 0;
    for(int32_t i = 0; i < iovcnt; ++i)
    {
        memcpy(outbuf + len, iov[i].iov_base, iov[i].iov_len);
        len += iov[i].iov_len;
    }
    inject(outbuf, len);
    delete[] outbuf;
}

#endif // EA_PLATFORM_LINUX

TcpMetricsInjector::TcpMetricsInjector(const char8_t* hostname, uint16_t port)
    : MetricsExporter::Injector(hostname, port),
      mAddr(nullptr),
      mSocket(nullptr)
{
}

TcpMetricsInjector::~TcpMetricsInjector()
{
    disconnect();
    delete mAddr;
}

bool TcpMetricsInjector::connect()
{
    if (mSocket != nullptr)
        disconnect();

    if (mAddr == nullptr)
        mAddr = BLAZE_NEW InetAddress(mHostname.c_str(), mPort);

    SocketChannel* sok = BLAZE_NEW SocketChannel(*mAddr);
    mSocket = BLAZE_NEW BlockingSocket(*sok);

    if (mSocket->connect() == -1)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "TcpMetricsInjector.connect: failed to connect to '" << *mAddr << "': errno=" << mSocket->getLastError());
        disconnect();
        return false;
    }
    return true;
}

void TcpMetricsInjector::disconnect()
{
    if (mSocket != nullptr)
    {
        mSocket->close();
        delete mSocket;
        mSocket = nullptr;
    }
}

void TcpMetricsInjector::inject(const char8_t* metric, size_t len)
{
    if (mSocket == nullptr)
    {
        if (!connect())
            return;
    }

    int32_t bytesWritten = 0;
    while (bytesWritten < (int32_t)len)
    {
        int w = mSocket->send(metric + bytesWritten, len - bytesWritten, 0);
        if (w > 0)
        {
            bytesWritten += w;
        }
        else
        {
            disconnect();
            break;
        }
    }
}

} // namespace Blaze

