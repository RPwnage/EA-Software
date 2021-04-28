/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_METRICSEXPORTER_H
#define BLAZE_METRICSEXPORTER_H

#include "framework/connection/blockingsocket.h"
#include "EATDF/time.h"
#ifdef EA_PLATFORM_LINUX
#include <sys/uio.h>
using iovec_blaze = iovec;
#else
struct iovec_blaze
{
    void* iov_base;
    size_t iov_len;
};
#endif

namespace Blaze
{

class MetricsExportConfig;
class MetricsFormatter;

// This is the legacy statsd/graphite metrics export implementation - https://developer.ea.com/display/blaze/Exporting+Metrics+To+Graphite 
// It was never really put to any substantial use.

class MetricsExporter
{
    NON_COPYABLE(MetricsExporter)

public:
    class Injector
    {
        NON_COPYABLE(Injector)

    public:
        Injector(const char8_t* hostname, uint16_t port)
            : mHostname(hostname)
            , mPort(port)
        {
        }

        virtual ~Injector() { }

        virtual bool connect() = 0;
        virtual void disconnect() = 0;

        virtual void inject(const char8_t* metric, size_t len) = 0;

    protected:
        eastl::string mHostname;
        uint16_t mPort;
    };

public:
    MetricsExporter(const char8_t* serviceName, const char8_t* instanceName, const char8_t* buildLocation, const MetricsExportConfig& config);
    ~MetricsExporter();

    static bool validateConfig(const MetricsExportConfig& config, ConfigureValidationErrors& validationErrors);

    void beginExport();
    void exportComponentMetrics(EA::TDF::TimeValue timestamp, ComponentMetricsResponse& metrics);
    void exportFiberTimings(EA::TDF::TimeValue timestamp, FiberTimings& fiberTimings);
    void exportStatus(EA::TDF::TimeValue timestamp, ServerStatus& status);
    void exportOutboundMetrics(EA::TDF::TimeValue timestamp, OutboundMetrics& metrics);
    void exportStorageManagerMetrics(EA::TDF::TimeValue timestamp, GetStorageMetricsResponse& metrics);
    void exportDbMetrics(EA::TDF::TimeValue timestamp, DbQueryMetrics& metrics);
    void endExport();

    bool isComponentMetricsEnabled() const { return mConfig.getComponentMetrics(); }
    bool isFiberTimingsEnabled() const { return mConfig.getFiberTimings(); }
    bool isStatusEnabled() const { return mConfig.getStatus(); }
    bool isDbMetricsEnabled() const { return mConfig.getDbMetrics(); }
    bool isOutboundMetricsEnabled() const { return mConfig.getOutboundMetrics(); }
    bool isStorageManagerMetricsEnabled() const { return mConfig.getStorageManagerMetrics(); }

    bool isExportEnabled() const;

    void setInjector(Injector* injector);
    void setBuildLocation(const char8_t* buildLocation) { mBuildLocation = buildLocation; }
    void setInstanceName(const char8_t* instanceName);

private:
    const char8_t* buildKey(char8_t* buf, size_t buflen, const char8_t* format, ...);

    void exportDbInstancePoolStatus(EA::TDF::TimeValue timestamp, const char8_t* database, const char8_t* name, DbInstancePoolStatus& status);
    const char8_t* replaceSeparator(const char8_t* src, char8_t* dst, size_t dstlen);
    void exportComponentStatusInfoMap(EA::TDF::TimeValue timestamp, const char8_t* componentName, const char8_t* serviceName, ComponentStatus::InfoMap& info);
    bool isNumber(const char8_t* value);

    void exportMetric(const char8_t* key, int64_t value, EA::TDF::TimeValue& timestamp);
    void exportMetric(const char8_t* key, uint64_t value, EA::TDF::TimeValue& timestamp);
    void exportMetric(const char8_t* key, double value, EA::TDF::TimeValue& timestamp);
    void exportMetric(const char8_t* key, const char8_t* value, EA::TDF::TimeValue& timestamp);
    void exportMetric(const char8_t* key, uint32_t value, EA::TDF::TimeValue& timestamp)
    {
        exportMetric(key, (uint64_t)value, timestamp);
    }
    void exportMetric(const char8_t* key, int32_t value, EA::TDF::TimeValue& timestamp)
    {
        exportMetric(key, (int64_t)value, timestamp);
    }

private:
    const MetricsExportConfig& mConfig;
    eastl::string mPrefix;
    eastl::string mServiceName;
    eastl::string mInstanceName;
    eastl::string mBuildLocation;
    MetricsFormatter* mFormatter;
    Injector* mInjector;
};

class UdpMetricsInjector : public MetricsExporter::Injector
{
    NON_COPYABLE(UdpMetricsInjector)

public:
    UdpMetricsInjector(const char8_t* hostname, uint16_t port);
    UdpMetricsInjector(const InetAddress& agentAddress);
    ~UdpMetricsInjector() override;

    bool connect() override;
    void disconnect() override;
    void inject(const char8_t* metric, size_t len) override;
    void inject(const iovec_blaze* iov, int iovcnt);

private:
    InetAddress* mAddr;
    SOCKET mSok;
    struct sockaddr_in mSockAddr;
};

class TcpMetricsInjector : public MetricsExporter::Injector
{
    NON_COPYABLE(TcpMetricsInjector)

public:
    TcpMetricsInjector(const char8_t* hostname, uint16_t port);
    ~TcpMetricsInjector() override;

    bool connect() override;
    void disconnect() override;
    void inject(const char8_t* metric, size_t len) override;

private:
    InetAddress* mAddr;
    BlockingSocket* mSocket;
};

} // namespace Blaze

#endif // BLAZE_METRICSEXPORTER_H

