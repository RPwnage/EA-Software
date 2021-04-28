#include "framework/blaze.h"
#include <WinSock2.h>
#include "EASTL/string.h"
#include "EAIO/EAFileUtil.h"

#include "tdf/blazeredis.h"
#include "framework/config/config_file.h"
#include "framework/config/config_map.h"
#include "framework/config/configdecoder.h"

// Empty stubs to satisfy the linker for quirkiness around Component DB.
namespace Blaze
{
    void BlazeRpcComponentDb::initialize(uint16_t componentId) {}
    size_t BlazeRpcComponentDb::getTotalComponentCount() { return 0; }
    size_t BlazeRpcComponentDb::getComponentCategoryIndex(ComponentId componentId) { return 0; }
    size_t BlazeRpcComponentDb::getComponentAllocGroups(size_t componentCategoryIndex) { return 0; }
} // Blaze

static const char8_t* REDIS_WORKING_DIR = "..\\..\\..\\init";
static const char8_t* REDIS_CFG_FILE = "..\\..\\..\\init\\redis.cfg";
static const char8_t* DEFAULT_CFG = "blazeredis.cfg";
static const uint32_t MAX_KILL_COUNT = 3;

static uint32_t sKillCount = 0;
static uint16_t sBasePort = 0;
static eastl::vector<HANDLE> sJobObjectHandles;

static void printMessage(const char8_t* msg)
{
    printf("[blazeredis]%s", msg);
#if defined EA_PLATFORM_WINDOWS
    OutputDebugString("[blazeredis]");
    OutputDebugString(msg);
#endif
}

static int32_t usage(const char* prg)
{
    eastl::string msg;
    msg.sprintf("Usage: %s [-c config] [--genconfig] \n", prg);
    printMessage(msg.c_str());
    return 1;
}

static BOOL WINAPI redisShutdownHandler(DWORD dwCtrlType)
{
    BOOL result = FALSE;

    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT)
    {
        sKillCount++;

        if (sKillCount >= MAX_KILL_COUNT)
        {
            // Force the shutdown
            abort();
        }
        else
        {
            eastl::string msg;
            msg.sprintf("[redisShutdownHandler] Stopping %u redis instances...\n", sJobObjectHandles.size());
            printMessage(msg.c_str());

            while (!sJobObjectHandles.empty())
            {
                if (sJobObjectHandles.back() != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(sJobObjectHandles.back());
                }
                sJobObjectHandles.pop_back();
            }
        }
        
        result = TRUE;
    }

    return result;
}

bool start_process(char8_t* commandLine)
{
    eastl::string msg;

    PROCESS_INFORMATION processInfo;
    memset(&processInfo, 0, sizeof(processInfo));

    STARTUPINFO startupInfo;
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    msg.sprintf("[start_process] Executing: %s\n", commandLine);
    printMessage(msg.c_str());
    if (CreateProcess(
        nullptr,
        commandLine,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW | CREATE_BREAKAWAY_FROM_JOB,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo))
    {
        sJobObjectHandles.push_back(CreateJobObject(nullptr, nullptr));

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobObjectInfo;
        jobObjectInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(sJobObjectHandles.back(), JobObjectExtendedLimitInformation, &jobObjectInfo, sizeof(jobObjectInfo)))
        {
            msg.sprintf("[start_process] Error (%lu) returned by SetInformationJobObject\n", GetLastError());
            printMessage(msg.c_str());
            return false;
        }

        if (!AssignProcessToJobObject(sJobObjectHandles.back(), processInfo.hProcess))
        {
            msg.sprintf("[start_process] Error (%lu) returned by AssignProcessToJobObject\n", GetLastError());
            printMessage(msg.c_str());
            return false;
        }

        WaitForInputIdle(processInfo.hProcess, 2000);
    }
    else
    {
        msg.sprintf("[start_process] Error (%lu) returned by CreateProcess\n", GetLastError());
        printMessage(msg.c_str());
        return false;
    }
    return true;
}

bool validate_cluster(const char8_t* clusterName, uint16_t totalNodes, uint16_t numReplicas)
{
    if ( (totalNodes / (numReplicas + 1)) < 3 )
    {
        // Although it's possible to launch redis clusters with fewer than 3 master nodes (or 
        // an uneven balance of replica nodes), the redis-cli --cluster functionality used by blazeredis does not support this.
        eastl::string msg;
        msg.sprintf("[validate_cluster] Invalid configuration for redis cluster '%s'. Clusters must include at least 3 master nodes -- this is not possible with %" PRIu16 " replicas per master node and %" PRIu16 " total nodes\n",
            clusterName, numReplicas, totalNodes);
        printMessage(msg.c_str());
        return false;
    }
    return true;
}

bool start_cluster(const char8_t* clusterName, uint16_t totalNodes, uint16_t numReplicas)
{
    static uint16_t basePort = sBasePort;
    uint16_t lastPort = basePort + totalNodes - 1;
    eastl::string msg;
    msg.sprintf("[start_cluster] Starting redis cluster '%s' on ports %" PRIu16 "-%" PRIu16 ", with %" PRIu16 " replicas per master:\n", clusterName, basePort, lastPort, numReplicas);
    printMessage(msg.c_str());

    eastl::string allHosts;

    for (; basePort<=lastPort; ++basePort)
    {
        eastl::string instanceName;
        instanceName.append_sprintf("%s-%" PRIu16, clusterName, basePort);
        allHosts.append_sprintf("127.0.0.1:%" PRIu16 " ", basePort);

        char8_t commandLine[1024] = {0};
        blaze_snzprintf(commandLine, sizeof(commandLine),
            "redis-server.exe redis.conf --cluster-config-file %s.conf --dbfilename %s.rdb --dir %s --port %" PRIu16,
            instanceName.c_str(),
            instanceName.c_str(),
            REDIS_WORKING_DIR,
            basePort);

        if (!start_process(commandLine))
            return false;
    }

    char8_t commandLine[1024] = { 0 };
    blaze_snzprintf(commandLine, sizeof(commandLine),
        "redis-cli.exe --cluster create %s --cluster-replicas %" PRIu16 " --cluster-yes",
        allHosts.c_str(),
        numReplicas);
    return start_process(commandLine);
}

void gen_cluster_config(eastl::string& cfgcontent, const char8_t* clusterName, uint16_t totalNodes, uint16_t numReplicas)
{
    static uint16_t basePort = sBasePort;
    uint16_t lastPort = basePort + totalNodes - 1;
    cfgcontent.append_sprintf("%s = { nodes = [\n", clusterName);

    for (; basePort <= lastPort; ++basePort)
        cfgcontent.append_sprintf("    \"127.0.0.1:%" PRIu16 "\"\n", basePort);

    cfgcontent.append("] }\n");
}



int main(int argc, char **argv)
{
    if (argc > 3)
        return usage(argv[0]);

    eastl::string msg;

    const char8_t* configFileName = nullptr;
    bool genconfig = false;
    for (int32_t a = 1; a < argc; a++)
    {
        if (strcmp(argv[a], "-c") == 0)
        {
            a++;
            if (a == argc)
                return usage(argv[0]);
            configFileName = argv[a];
        }
        else if (strcmp(argv[a], "--genconfig") == 0)
        {
            genconfig = true;
        }
        else
        {
            return usage(argv[0]);
        }
    }

    if (genconfig)
    {
        if (EA::IO::Directory::Exists(REDIS_WORKING_DIR))
        {
            if (!EA::IO::Directory::Remove(REDIS_WORKING_DIR))
            {
                msg.sprintf("[main] Failed to delete existing directory '%s'\n", REDIS_WORKING_DIR);
                printMessage(msg.c_str());
                return 1;
            }
        }

        if (!EA::IO::Directory::Create(REDIS_WORKING_DIR))
        {
            msg.sprintf("[main] Failed to create directory '%s'\n", REDIS_WORKING_DIR);
            printMessage(msg.c_str());
            return 1;
        }
    }
    else if (!EA::IO::File::Exists(REDIS_CFG_FILE))
    {
        msg.sprintf("[main] Missing file '%s'\n", REDIS_CFG_FILE);
        printMessage(msg.c_str());
        return 1;
    }

    if (configFileName == nullptr)
        configFileName = DEFAULT_CFG;

    Blaze::PredefineMap predefineMap;
    predefineMap.addBuiltInValues();
    Blaze::ConfigFile* config = Blaze::ConfigFile::createFromFile("./", configFileName, true, predefineMap);
    if (config == nullptr)
    {
        msg.sprintf("[main] Unable to read config file '%s'\n", configFileName);
        printMessage(msg.c_str());
        return 1;
    }

    const Blaze::ConfigMap* configMap = config->getMap("blazeRedisConfig");
    if (configMap == nullptr)
    {
        msg.sprintf("[main] Unable to load BlazeRedisConfig from config file '%s'\n", configFileName);
        printMessage(msg.c_str());
        return 1;
    }

    BlazeRedis::BlazeRedisConfig configTdf;
    Blaze::ConfigDecoder decoder(*configMap);
    if (!decoder.decode(&configTdf))
    {
        msg.sprintf("[main] Unable to decode config file '%s'\n", configFileName);
        printMessage(msg.c_str());
        return 1;
    }
    sBasePort = configTdf.getBasePort();

    if (genconfig)
    {
        eastl::string cfgcontent;
        for (BlazeRedis::BlazeRedisConfig::ClusterMap::const_iterator itr = configTdf.getClusters().begin(); itr != configTdf.getClusters().end(); ++itr)
        {
            if (!validate_cluster(itr->first.c_str(), itr->second->getTotalNodes(), itr->second->getNumReplicas()))
                return 1;

            gen_cluster_config(cfgcontent, itr->first.c_str(), itr->second->getTotalNodes(), itr->second->getNumReplicas());
        }

        FILE* file = fopen(REDIS_CFG_FILE, "w");
        if (file == nullptr)
        {
            msg.sprintf("[main] Error opening '%s': (%i) %s\n", REDIS_CFG_FILE, errno, strerror(errno));
            printMessage(msg.c_str());
            return 1;
        }
        fwrite(cfgcontent.c_str(), cfgcontent.size(), 1, file);
        fclose(file);
        return 0;
    }


    // if fail, try again.
    if (SetConsoleCtrlHandler(redisShutdownHandler, TRUE) == 0)
    {
        if (SetConsoleCtrlHandler(redisShutdownHandler, TRUE) == 0)
        {
            msg.sprintf("[main] Error (%lu) returned by SetConsoleCtrlHandler\n", GetLastError());
            printMessage(msg.c_str());
            return 1;
        }
    }

    for (BlazeRedis::BlazeRedisConfig::ClusterMap::const_iterator itr = configTdf.getClusters().begin(); itr != configTdf.getClusters().end(); ++itr)
    {
        if (!start_cluster(itr->first.c_str(), itr->second->getTotalNodes(), itr->second->getNumReplicas()))
            return 1;
    }

    eastl::vector<HANDLE> copyOfHandles = sJobObjectHandles;
    HANDLE* allHandles = new HANDLE[copyOfHandles.size()];
    size_t index = 0;
    for (eastl::vector<HANDLE>::iterator itr = copyOfHandles.begin(); itr != copyOfHandles.end(); ++itr)
    {
        allHandles[index] = *itr;
        ++index;
    }
    WaitForMultipleObjects(copyOfHandles.size(), allHandles, TRUE, INFINITE);

    return 0;
}