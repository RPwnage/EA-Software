#include "EABase/eabase.h"  //include eabase.h before any std lib includes for VS2012 (see EABase/2.02.00/doc/FAQ.html#Info.17 for details)

#include <stdlib.h>

#include "framework/blaze.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/util/shared/rawbufferistream.h"
#include "framework/logger.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/tdf/eventtypes_server.h"
#include "framework/rpc/blazecontrollerslave.h"
#include "framework/controller/metricsexporter.h"
#include "framework/connection/socketutil.h"
#include "EAStdC/EAString.h"

#ifdef EA_PLATFORM_WINDOWS
#include <io.h>
#include <fcntl.h>
# define SET_BINARY_MODE(handle) _setmode(_fileno(handle), _O_BINARY)
#else
# define SET_BINARY_MODE(handle) ((void)0)
#endif

#define DEFAULT_FORMAT Blaze::MetricsExportConfig::GRAPHITE

namespace metricsinjector
{

typedef eastl::vector<eastl::string> FileList;
static FileList gFiles;
static EA::TDF::TimeValue gStartTime = 0;
static EA::TDF::TimeValue gEndTime = EA::TDF::TimeValue::getTimeOfDay();
static const char8_t* gPrefix = nullptr;
static const char8_t* gServiceName = nullptr;
static eastl::string gInstanceName = "";
static Blaze::MetricsExportConfig::Format gFormat = DEFAULT_FORMAT;
static Blaze::MetricsExporter* gExporter = nullptr;
static const char8_t* gHostname = nullptr;
static uint16_t gPort = 0;

static eastl::string gBuildLocation = "";

class StdoutInjector : public Blaze::MetricsExporter::Injector
{
public:
    StdoutInjector(const char8_t* hostname, uint16_t port)
        : Blaze::MetricsExporter::Injector(hostname, port)
    {
    }

    bool connect() override { return true; }
    void disconnect() override { }
    
    void inject(const char8_t* metric, size_t len) override
    {
        printf("%s", metric);
    }
};

class BlockingTcpInjector : public Blaze::MetricsExporter::Injector
{
public:
    BlockingTcpInjector(const char8_t* hostname, uint16_t port)
        : Blaze::MetricsExporter::Injector(hostname, port),
          mAddr(nullptr),
          mSocket(INVALID_SOCKET)
    {
    }

    ~BlockingTcpInjector() override
    {
        disconnect();
        delete mAddr;
        mAddr = nullptr;
    }

    bool connect() override
    {
        if (mSocket == INVALID_SOCKET)
        {
            mSocket = Blaze::SocketUtil::createSocket(true);
            if (mSocket == INVALID_SOCKET)
            {
                printf("Unable to create new socket for connection to %s:%d: errno=%d\n", mHostname.c_str(), mPort, WSAGetLastError());
                return false;
            }
            sockaddr_in addr;
            mAddr->getSockAddr(addr);
            if (::connect(mSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
            {
                printf("Unable to connect to %s:%d: errno=%d\n", mHostname.c_str(), mPort, WSAGetLastError());
                disconnect();
                return false;
            }
        }
        return true;
    }

    void disconnect() override
    {
        if (mSocket != INVALID_SOCKET)
        {
            closesocket(mSocket);
            mSocket = INVALID_SOCKET;
        }
    }

    void inject(const char8_t* metric, size_t len) override
    {
        int32_t bytesWritten = 0;
        while (bytesWritten < (int32_t)len)
        {
            int w = send(mSocket, metric + bytesWritten, len - bytesWritten, 0);
            if (w > 0)
            {
                bytesWritten += w;
            }
            else
            {
                disconnect();
                connect();
                break;
            }
        }
    }

    bool configure()
    {
        mAddr = BLAZE_NEW Blaze::InetAddress(mHostname.c_str(), mPort);
        if (!Blaze::NameResolver::blockingResolve(*mAddr))
        {
            delete mAddr;
            mAddr = nullptr;
            printf("Unable to resolve '%s'\n", mHostname.c_str());
            return false;
        }

        if (!connect())
            return false;

        return true;
    }

private:
    Blaze::InetAddress* mAddr;
    SOCKET mSocket;
};

static int usage(const char8_t* prg)
{
    printf("Usage: %s [-start startDate] [-end endDate] [-f format] [-tcp hostname port] [-i instanceName] [-p prefix] -s serviceName file ... \n", prg);
    printf("    -start  : Only print events newer than the date provided.\n");
    printf("    -end    : Only print events younger than the date provided.\n");
    printf("    -p      : Specify graphite prefix.\n");
    printf("    -s      : Blaze service name associated with file(s).\n");
    printf("    -i      : Blaze instance name associated with file(s).\n");
    printf("              If not provided, instance name from EventEnvelope will be used.\n");
    printf("    -f      : Select output format ('%s' or '%s').  Default: %s\n",
            Blaze::MetricsExportConfig::FormatToString(Blaze::MetricsExportConfig::GRAPHITE),
            Blaze::MetricsExportConfig::FormatToString(Blaze::MetricsExportConfig::STATSD),
            Blaze::MetricsExportConfig::FormatToString(DEFAULT_FORMAT));
    printf("    -tcp    : Write metrics to a socket connected to the provided host and port.\n");
    return 1;
}

static void processArgs(int32_t argc, char** argv)
{
    for(int32_t idx = 1; idx < argc; ++idx)
    {
        if (argv[idx][0] == '-')
        {
            if (strcmp(argv[idx], "-s") == 0)
            {
                ++idx;
                if (idx == argc)
                    exit(usage(argv[0]));
                gServiceName = argv[idx];
            }
            else if (strcmp(argv[idx], "-p") == 0)
            {
                ++idx;
                if (idx == argc)
                    exit(usage(argv[0]));
                gPrefix = argv[idx];
            }
            else if (strcmp(argv[idx], "-i") == 0)
            {
                ++idx;
                if (idx == argc)
                    exit(usage(argv[0]));
                gInstanceName = argv[idx];
            }
            else if (strcmp(argv[idx], "-f") == 0)
            {
                ++idx;
                if (idx == argc)
                    exit(usage(argv[0]));
                if (!Blaze::MetricsExportConfig::ParseFormat(argv[idx], gFormat))
                {
                    fprintf(stderr, "Unrecognized format '%s'.\n", argv[idx]);
                    exit(1);
                }
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
            else if (strcmp(argv[idx], "-tcp") == 0)
            {
                ++idx;
                if (idx == argc)
                    exit(usage(argv[0]));
                gHostname = argv[idx];
                ++idx;
                if (idx == argc)
                    exit(usage(argv[0]));
                gPort = (uint16_t) EA::StdC::AtoI32(argv[idx]);
            }
            else
            {
                exit(usage(argv[0]));
            }
        }
        else
        {
            gFiles.push_back(argv[idx]);
        }
    }

    if (gFiles.empty() || (gServiceName == nullptr))
        exit(usage(argv[0]));

}


static void processEvent(EA::TDF::TimeValue eventTime, const Blaze::Event::EventEnvelope& eventEnvelope)
{
    if (eventEnvelope.getComponentId() != Blaze::BlazeControllerSlave::COMPONENT_ID)
    {
        // Only care about blazecontroller events
        return;
    }

    if (gInstanceName.length() == 0)
    {
        const char8_t* instanceName = eventEnvelope.getInstanceName();
        if ((instanceName == nullptr) || (instanceName[0] == '\0'))
        {
            printf("EventEnvelope does not contain instance name information.  Please provide -i on command line.\n");
            exit(1);
        }
        gExporter->setInstanceName(instanceName);
    }
    else
    {
        gExporter->setInstanceName(gInstanceName.c_str());
    }

    uint32_t eventId = (Blaze::BlazeControllerSlave::COMPONENT_ID << 16) | eventEnvelope.getEventType();
    switch (eventId)
    {
        case Blaze::BlazeControllerSlave::EVENT_COMPONENTMETRICSEVENT:
        {
            Blaze::ComponentMetricsResponse* tdf = (Blaze::ComponentMetricsResponse*)eventEnvelope.getEventData();
            if (tdf != nullptr)
                gExporter->exportComponentMetrics(eventTime, *tdf);
            break;
        }
        case Blaze::BlazeControllerSlave::EVENT_FIBERTIMINGSEVENT:
        {
            Blaze::FiberTimings* tdf = (Blaze::FiberTimings*)eventEnvelope.getEventData();
            if (tdf != nullptr)
                gExporter->exportFiberTimings(eventTime, *tdf);
            break;
        }
        case Blaze::BlazeControllerSlave::EVENT_STATUSEVENT:
        {
            Blaze::ServerStatus* tdf = (Blaze::ServerStatus*)eventEnvelope.getEventData();
            if (tdf != nullptr)
                gExporter->exportStatus(eventTime, *tdf);

            // Override the build location in the injector in case the exe running here is
            // different than the one used to build the server.
            gBuildLocation = tdf->getVersion().getBuildLocation();
            break;
        }

        case Blaze::BlazeControllerSlave::EVENT_OUTBOUNDMETRICSEVENT:
        {
            Blaze::OutboundMetrics* tdf = (Blaze::OutboundMetrics*)eventEnvelope.getEventData();
            if (tdf != nullptr)
                gExporter->exportOutboundMetrics(eventTime, *tdf);
            break;
        }

        case Blaze::BlazeControllerSlave::EVENT_DBQUERYMETRICSEVENT:
        {
            // Make sure the build location is set to ensure the common leading prefix is
            // stripped of DB query locations to keep things manageable in graphite.
            gExporter->setBuildLocation(gBuildLocation.c_str());

            Blaze::DbQueryMetrics* tdf = (Blaze::DbQueryMetrics*)eventEnvelope.getEventData();
            if (tdf != nullptr)
                gExporter->exportDbMetrics(eventTime, *tdf);
            break;
        }

        case Blaze::BlazeControllerSlave::EVENT_STARTUPEVENT:
        case Blaze::BlazeControllerSlave::EVENT_SHUTDOWNEVENT:
        default:
            break;
    }
}

static int processFile(const char8_t* filename)
{
    FILE* fp = fopen(filename, "r");
    if (fp == nullptr)
    {
        fprintf(stderr, "Unable to open '%s'\n", filename);
        exit(1);
    }
    SET_BINARY_MODE(fp);

    Blaze::Event::EventEnvelopeTdfList eventEnvelopeList;

    size_t bufLen = 1024*1024*10; // 10 MB buffer
    uint8_t* buf = (uint8_t*) BLAZE_ALLOC(bufLen);

    while (!feof(fp))
    {
        uint32_t seq = 0;
        uint32_t size = 0;

        // extract the sequence counter
        fread(reinterpret_cast<char8_t*>(&seq), 1, sizeof(uint32_t), fp);
        seq = ntohl(seq);

        // extract the size of the event envelope
        fread(reinterpret_cast<char8_t*>(&size), 1, sizeof(uint32_t), fp);
        size = ntohl(size);

        if (seq == 0 || seq == UINT32_MAX || size == 0 || size == UINT32_MAX)
            continue;

        if (bufLen < size)
        {
            BLAZE_FREE(buf);
            buf = (uint8_t*) BLAZE_ALLOC(size);
            bufLen = size;
        }
        
        Blaze::RawBuffer raw(buf, bufLen);
        raw.put(size);

        // use the size to extract the event envelope data
        size_t rSize = fread(reinterpret_cast<char8_t*>(raw.data()), 1, size, fp);

        if (rSize != size)
        {
            fprintf(stderr, "Attempted to read %d but only read %d\n", size, static_cast<int32_t>(rSize));
        }

        Blaze::Event::EventEnvelope eventEnvelope;
        Blaze::Heat2Decoder decoder;
        Blaze::BlazeRpcError err = Blaze::ERR_OK;
        err = decoder.decode(raw, eventEnvelope);

        EA::TDF::TimeValue eventTime = eventEnvelope.getTimestamp();
        if ((eventTime < gStartTime) || (eventTime > gEndTime))
        {
            continue;
        }

        if (err != Blaze::ERR_OK)
        {
            fprintf(stderr, "Failed to decode event (sequence=%d)\n", seq);
        }
        else
        {
            processEvent(eventTime, eventEnvelope);
        }
    }
    fclose(fp);
    
    BLAZE_FREE(buf);

    return 0;
}

int metricsinjector_main(int argc, char** argv)
{
    Blaze::BlazeRpcComponentDb::initialize();

    processArgs(argc, argv);

    Blaze::MetricsExportConfig config;
    config.setComponentMetrics(true);
    config.setFiberTimings(true);
    config.setStatus(true);
    config.setOutboundMetrics(true);
    config.setDbMetrics(true);
    config.setFormat(gFormat);
    config.setHostname(gHostname);
    config.setPort(gPort);
    if (gPrefix != nullptr)
        config.setPrefix(gPrefix);

    Blaze::MetricsExporter::Injector* injector = nullptr;
    if (gHostname != nullptr)
    {
        BlockingTcpInjector* tcpInjector = BLAZE_NEW BlockingTcpInjector(config.getHostname(), config.getPort());
        if (!tcpInjector->configure())
            exit(1);
        injector = tcpInjector;
    }
    else
    {
        injector = BLAZE_NEW StdoutInjector("", 0);
    }

    gExporter = BLAZE_NEW Blaze::MetricsExporter(gServiceName, gInstanceName.c_str(), "", config);
    // Override the injector to use our local one
    gExporter->setInjector(injector);

    for(FileList::iterator itr = gFiles.begin(), end = gFiles.end(); itr != end; ++itr)
        processFile((*itr).c_str());

    delete gExporter;

    return 0;
}

}
