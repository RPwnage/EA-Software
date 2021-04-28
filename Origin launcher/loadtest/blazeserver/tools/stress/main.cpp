/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "stress.h"
#include "framework/system/blazethread.h"
#include "framework/system/consolesignalhandler.h"
#include "framework/connection/sslcontext.h"
#include "framework/connection/socketutil.h"
#include "framework/config/config_file.h"
#include "framework/config/config_map.h"
#include "framework/config/configdecoder.h"
#include "framework/connection/nameresolver.h"
#include "framework/tdf/frameworkconfigtypes_server.h"

#include "EAStdC/EAString.h"

#ifdef EA_PLATFORM_LINUX
#include <signal.h>
extern const char _BlazeBuildTime[];
extern const char _BlazeBuildLocation[];
extern const char _BlazeP4DepotLocation[];
extern const char _BlazeChangelist[];
extern const char _BlazeBuildTarget[];
typedef struct
{
    const char* filename;
    const char* source;
} BlazeFileList;
extern const BlazeFileList _BlazeEditedFiles[];
#else
const char _BlazeBuildTime[] = "unknown";
const char _BlazeBuildLocation[] = "unknown";
const char _BlazeP4DepotLocation[] = "unknown";
const char _BlazeChangelist[] = "unknown";
const char _BlazeBuildTarget[] = "unknown";
extern struct
{
    const char* filename;
    const char* source;
} _BlazeEditedFiles[] = { { 0, 0 } };
#endif

namespace Blaze
{
namespace Stress
{

static Metrics::MetricsCollection sMetricsCollection;

class StressRunner : public BlazeThread
{
public:
    StressRunner(const ConfigFile& config, const CmdLineArguments& arguments)
        : BlazeThread("stressselector"),
        mNameResolver("stressselector"),
        mSelector(*Selector::createSelector()),
        mIsRunning(false),
        mStress(config, arguments)
    {
    }

    StressRunner(const ConfigFile& config, const char8_t* serverAddress, const CmdLineArguments& arguments)
        : BlazeThread("stressselector"),
        mNameResolver("stressselector"),
        mSelector(*Selector::createSelector()),
        mIsRunning(false),
        mStress(config, InetAddress(serverAddress), arguments)
    {
    }
    
    bool isRunning() const { return mIsRunning; }
    Selector& getSelector() { return mSelector; }

    void startStress()
    {
        mStress.startInitialization(getSelector());
        mStress.startStress(getSelector());
    }

    void startAccountCreation(uint32_t count)
    {
        mStress.startInitialization(getSelector());
        mStress.startAccountCreation(getSelector(), count);
    }

    void stop()
    {
        mSelector.shutdown();
    }

    void run()
    {            
		Metrics::gMetricsCollection = &sMetricsCollection;
		Metrics::gFrameworkCollection = &Metrics::gMetricsCollection->getTaglessCollection("framework");

        //Use a blank selector config.
        SelectorConfig dummyConfig;
        if (!mSelector.initialize(dummyConfig))
        {
            printf("Failed to initialize selector. Exiting\n");
            return;
        }
        mSelector.open();
        gSelector = &mSelector;    
        gFiberManager = &mSelector.getFiberManager();
		gSslContextManager = &mSslContextManager;

        if (!gSslContextManager->createContext("ssl/ca-certs", ""))
        {
            printf("Failed to initialize SSL context; it may not be possible to make secure "
                "connections.\n");
        }
        
        // Initialize the Random Seed
        srand((int)TimeValue::getTimeOfDay().getMicroSeconds());

        gNameResolver = &mNameResolver;
        if (mNameResolver.start() == EA::Thread::kThreadIdInvalid)
        {
            printf("Failed to start name resolver thread.  Exiting\n");
            return;
        }

        mIsRunning = true;
        mSelector.run();
        mIsRunning = false;

        mStress.destroy();

        mNameResolver.stop();
    }

private:
    NameResolver mNameResolver;
    Selector& mSelector;
    bool mIsRunning;
    Stress mStress;
	SslContextManager mSslContextManager;
};

static StressRunner* gStressRunner = NULL;


static void shutdownHandler(void* context)
{
    gStressRunner->stop();
}

#ifdef EA_PLATFORM_LINUX
static void sigprocSignalHandler(int sig)
{
    switch(sig)
    {
    case SIGPIPE:
        BLAZE_INFO_LOG(Log::SYSTEM, "Received SIGPIPE signal. Ignoring.");
        break;
    default:
        BLAZE_INFO_LOG(Log::SYSTEM, "Received signal number: '" << sig << "'");
    }
}

void SetupSignalHandler()
{
    sigset_t procmask;
    sigemptyset(&procmask);
    sigprocmask(SIG_SETMASK, &procmask, NULL);

    struct sigaction act;
    act.sa_handler = sigprocSignalHandler;
    act.sa_flags = 0;

    sigemptyset(&act.sa_mask);
    sigaction(SIGPIPE, &act, NULL);
}
#endif

static void printVersion(bool verbose)
{
    printf("====== Build Version Details ======\n");
    printf("Build CL#        : %s\n", _BlazeChangelist);
    printf("Build time        : %s\n", _BlazeBuildTime);
    printf("Build location    : %s\n", _BlazeBuildLocation);
    printf("Build target    : %s\n", _BlazeBuildTarget);
    printf("Depot location    : %s\n", _BlazeP4DepotLocation);

   if (_BlazeEditedFiles[0].filename != NULL)
    {
        printf("Modified source files:\n");
        for(int32_t i = 0; _BlazeEditedFiles[i].filename != NULL; ++i)
            printf("    %s\n", _BlazeEditedFiles[i].filename);

        if (verbose)
        {
            printf("\n");
            for(int32_t i = 0; _BlazeEditedFiles[i].filename != NULL; ++i)
            {
                printf("==== %s ====\n\n", _BlazeEditedFiles[i].filename);
                printf("%s\n", _BlazeEditedFiles[i].source);
            }
        }
    }
}

static int32_t usage(const char* prg)
{
    printf("Usage: %s [-c config] [-s hostname:port] [-create count] [--logdir dir] [--logname filename] [--testid uuid] [-num_conns count] [-start_index index] [-psu PSU] [-block_size size] [-block_delay delay] [-instance_delay delay]\n", prg);
    return 1;
}

extern "C" int main(int argc, char** argv)
{
    const char8_t* configFileName = NULL;
    const char8_t* server = NULL;
    const char8_t* logDir = NULL;
    const char8_t* logName = NULL;
    uint32_t createCount = 0;
    bool createAccounts = false;
    CmdLineArguments arguments;

    PredefineMap predefineMap;
    predefineMap.addBuiltInValues();
    EA::TDF::TdfFactory::fixupTypes();


    for(int32_t a = 1; a < argc; a++)
    {
        if (strcmp(argv[a], "-c") == 0)
        {
            a++;
            if (a == argc)
                return usage(argv[0]);
            configFileName = argv[a];
        }
        else if (strcmp(argv[a], "-s") == 0)
        {
            a++;
            if (a == argc)
                return usage(argv[0]);
            server = argv[a];
        }
        else if (strcmp(argv[a], "-create") == 0)
        {
            a++;
            if (a == argc)
                return usage(argv[0]);
            createAccounts = true;
            createCount = atoi(argv[a]);
        }
        else if ((argv[a][0] == '-') && (argv[a][1] == 'D'))
        {
            predefineMap.addFromKeyValuePair(&argv[a][2]);
        }
        else if (strcmp(argv[a], "--logdir") == 0)
        {
            a++;
            if (a == argc)
                return usage(argv[0]);
            logDir = argv[a];
        }
        else if (strcmp(argv[a], "--logname") == 0)
        {
            a++;
            if (a == argc)
                return usage(argv[0]);
            logName = argv[a];
        }
        else if (strcmp(argv[a], "--testid") == 0)
        {
            a++;
            if (a == argc)
                return usage(argv[0]);
        }
        else if (strcmp(argv[a], "-num_conns") == 0)
        {
            a++;
            if (a == argc)
                return usage(argv[0]);
            arguments.numberOfConnections = atoi(argv[a]);
        }
        else if (strcmp(argv[a], "-start_index") == 0)
        {
            a++;
            if (a == argc)
                return usage(argv[0]);
            arguments.startIndex = EA::StdC::AtoU32(argv[a]);
        }
		else if (strcmp(argv[a], "-psu") == 0)
		{
			a++;
            if (a == argc)
                return usage(argv[0]);
            arguments.psu = atoi(argv[a]);
		}
        else if (strcmp(argv[a], "-block_size") == 0)
        {
            a++;
            if (a == argc)
                return usage(argv[0]);
            arguments.blockSize = atoi(argv[a]);
        }
        else if (strcmp(argv[a], "-block_delay") == 0)
        {
            a++;
            if (a == argc)
                return usage(argv[0]);
            arguments.blockDelay = atoi(argv[a]);
        }
        else if (strcmp(argv[a], "-instance_delay") == 0)
        {
            a++;
            if (a == argc)
                return usage(argv[0]);
            arguments.instanceDelay = atoi(argv[a]);
        }
        else if (strcmp(argv[a], "-v") == 0)
        {
           printVersion(false);
           return 1;
        }
        else if (strcmp(argv[a], "-vv") == 0)
        {
            printVersion(true);
            return 1;
        }
        else
        {
            return usage(argv[0]);
        }
    }

    //  if server == NULL, uses server values in the config file.
    if (configFileName == NULL)
        return usage(argv[0]);
	
	Metrics::gMetricsCollection = &sMetricsCollection;
    Metrics::gFrameworkCollection = &Metrics::gMetricsCollection->getTaglessCollection("framework");
    // Must initialize fiber storage before Logger
    Fiber::FiberStorageInitializer fiberStorage;

    BlazeRpcError err = Logger::initialize(Logging::INFO, logDir, logName);
    if (err != Blaze::ERR_OK)
    {
        eastl::string errorLog;
        errorLog.sprintf("Failed to initialize logger(logDir=%s, logName=%s) \n", logDir, logName);
        EA::StdC::Printf(errorLog.c_str());
        exit(2);
    }

    BlazeRpcComponentDb::initialize();
    
    ConfigFile* config = ConfigFile::createFromFile("./", configFileName, true, predefineMap);
    if (config == NULL)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "Unable to read config file '" << configFileName << "'");
        Logger::destroy();
        exit(2);
    }
    
    const ConfigMap* loggingMap = config->getMap("logging");
    if (loggingMap != NULL)
    {
        LoggingConfig configTdf;
        ConfigDecoder decoder(*loggingMap);
        if (decoder.decode(&configTdf))
        {
            Logger::configure(configTdf, false);
        }
    }

    SocketUtil::initializeNetworking();

#ifdef EA_PLATFORM_LINUX
    SetupSignalHandler();
#endif

    InstallConsoleSignalHandler(shutdownHandler, NULL, NULL, NULL, NULL);

    // Initialize and start selector thread
    if (server == NULL)
    {
        gStressRunner = BLAZE_NEW StressRunner(*config, arguments);
    }
    else
    {
        gStressRunner = BLAZE_NEW StressRunner(*config, server, arguments);
    }
    if (gStressRunner->start() == EA::Thread::kThreadIdInvalid)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "Unable to start StressRunner thread");
        exit(2);
    }

    // Wait for selector to get up and running
    while (!gStressRunner->isRunning())
        doSleep(10);

    if (createAccounts)
    {
        gStressRunner->startAccountCreation(createCount);
    }
    else
    {
        gStressRunner->startStress();
    }

    // wait for stress runner to exit
    gStressRunner->waitForEnd();
    BLAZE_INFO(Log::SYSTEM, "stress client exit normally.");

    delete gStressRunner;
    delete config;
    SocketUtil::shutdownNetworking();
    Logger::destroy();

    EA::TDF::TdfFactory::cleanupTypeAllocations();
    return 0;
}

} // namespace Stress
} // namespace Blaze

