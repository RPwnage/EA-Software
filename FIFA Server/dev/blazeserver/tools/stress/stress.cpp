/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Stress

    This is the main controller class for the stress module.  It handles overall configation,
    initialization, creation of modules and stress instances and the coordinates the startup and
    shutdown of the main stress loop.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "stress.h"
#include "stressmodule.h"
#include "stressconnection.h"
#include "stressinstance.h"
#include "instanceresolver.h"
#include "framework/connection/selector.h"
#include "framework/connection/nameresolver.h"
#include "framework/config/config_boot_util.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"
#include "framework/config/config_file.h"
#include "framework/config/configdecoder.h"
#include "framework/util/shared/blazestring.h"

#ifdef EA_PLATFORM_LINUX
#include <sys/inotify.h>
#include <errno.h>

/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof(struct inotify_event))

/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))

#endif

namespace Blaze
{
namespace Stress
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

extern ModuleCreationInfo gModuleCreators[];
extern ModuleCreationInfo gCustomModuleCreators[];

/*** Public Methods ******************************************************************************/

Stress::Stress(ConfigMap* config, const char8_t* configFileName, PredefineMap& predefineMap, const CmdLineArguments& arguments)
    : mConfigBootUtil(nullptr),
      mConfigFileName(configFileName),
      mPredefineMap(predefineMap),
      mServers(),
      mArguments(arguments)
{
    EA_ASSERT(config != nullptr);
    mConfigs.push_front(config);

#ifdef EA_PLATFORM_LINUX
    if (mConfigFileName == nullptr)
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].Stress: Config directory path is not set");
    }
    else
    {
        mConfigFileDir = mConfigFileName;
        mConfigFileDir = mConfigFileDir.substr(0, mConfigFileDir.find_last_of("/\\"));
        BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].Stress: Config directory path is: " << mConfigFileDir);
    }

    mConfigFileDirWatchRetry = 0;
    mConfigFileDirFd = -1;
    mConfigFileDirWatch = -1;
#endif
}

Stress::Stress(ConfigMap* config, const char8_t* configFileName, PredefineMap& predefineMap, const InetAddress& serverAddress, const CmdLineArguments& arguments)
    : Stress(config, configFileName, predefineMap, arguments)
{
    mServers.push_back(serverAddress);
}

Stress::~Stress()
{
#ifdef EA_PLATFORM_LINUX
    if (mConfigFileDirFd > 0 && !inotify_rm_watch(mConfigFileDirFd, mConfigFileDirWatch))
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].~Stress: Error removing watch: " << strerror(errno));
    }
#endif

    delete mConfigBootUtil;

    while (!mConfigs.empty())
    {
        ConfigMap* config = mConfigs.front();
        mConfigs.pop_front();
        delete config;
    }
}

bool Stress::initialize()
{
    //Kick off our boot config so modules can use it to read in the server configuration
    const ConfigMap* configCfg = mConfigs.front()->getMap("serverConfiguration");
    if (configCfg != nullptr)
    {
        const char8_t* bootFile = configCfg->getString("bootFile", "");
        const char8_t* overrideBootFile = configCfg->getString("overrideBootFile", "");

        if (bootFile != nullptr && *bootFile != '\0')
        {
            PredefineMap externalDefines;
            externalDefines.addBuiltInValues();
            const ConfigMap* predefinesCfg = configCfg->getMap("predefines");
            if (predefinesCfg != nullptr)
            {
                while (predefinesCfg->hasNext())
                {
                    const char8_t* nextKey = predefinesCfg->nextKey();
                    externalDefines[nextKey] = predefinesCfg->getString(nextKey, "");
                }
                
            }

            mConfigBootUtil = BLAZE_NEW ConfigBootUtil(bootFile, overrideBootFile, externalDefines, false);
            if (!mConfigBootUtil->initialize())
            {
                return false;
            }
        }
    }

    uint32_t numConnections = 0;
    if (!initializeConnectionInfo(&numConnections))
        return false;

    bool useRedirector = false;
    const ConfigMap* serversCfg = mConfigs.front()->getMap("servers");
    if (serversCfg != nullptr)
    {
        // Stress client uses redirector to find available instance
        const char8_t* serviceName = serversCfg->getString("serviceName", nullptr);
        const char8_t* rdirAddress = serversCfg->getString("redirectorAddress", nullptr);
        if (serviceName != nullptr && rdirAddress != nullptr)
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].initialize: Using redirector(" << rdirAddress << ") to resolve slave instances for " << serviceName);
            mServiceName = serviceName;
            mRedirectorAddress = rdirAddress;
            useRedirector = true;
        }
        else
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initialize: No serviceName or redirectorAddress specified in redirector config.");
        }
    }

    if (!useRedirector)
    {
        // If we have a server/port already supplied by command line, ignore config servers
        if (mServers.empty() && serversCfg != nullptr)
        {
            //  fill server list with specified values.
            uint32_t serverIndex = 0;
            char8_t fieldName[16];
            const char8_t* fieldNameFmt = mSecure ? "ssl-%u" : "tcp-%u";
            blaze_snzprintf(fieldName, sizeof(fieldName), fieldNameFmt, serverIndex);

            const char8_t* serverAdr = serversCfg->getString(fieldName, "null");
            while (serverAdr != nullptr && blaze_strcmp(serverAdr, "null") != 0)
            {
                mServers.push_back(InetAddress(serverAdr));
                ++serverIndex;
                blaze_snzprintf(fieldName, sizeof(fieldName), fieldNameFmt, serverIndex);
                serverAdr = serversCfg->getString(fieldName, "null");
            }
        }

        //  resolve each server, removing servers that are unresolvable.
        for (ServerList::iterator it = mServers.begin(); it != mServers.end();)
        {
            InetAddress addr = *it;
            if (!gNameResolver->blockingResolve(addr))
            {
                char8_t addrBuf[256];
                BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].initialize: Unable to resolve server address: " << addr.toString(addrBuf, sizeof(addrBuf)) << "\n");

                it = mServers.erase(it);
            }
            else
            {
                ++it;
            }
        }
        if (mServers.size() > 1)
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].initialize: Resolved " << mServers.size() << " servers for connection.\n");
        }
        else if (mServers.empty())
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initialize: No available servers.\n");
            return false;
        }
    }

    BLAZE_INFO(Log::SYSTEM, "[Stress].initialize: Using %s allocator.\n", Allocator::getTypeName());

    if (!initializeConnections(numConnections))
        return false;

    const ConfigMap* loginCfg = mConfigs.front()->getMap("account");
    if (loginCfg != nullptr)
    {
        if (!mLoginManager.initialize(*loginCfg, mArguments))
            return false;
    }

    if (!initializeModules())
        return false;

    if (!initializeInstances())
        return false;

    if (!initializeStartup())
        return false;

    return true;
}

bool Stress::updateConfig(ConfigMap* config)
{
    BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].updateConfig: Updating Config Values.");
    EA_ASSERT(config != nullptr);
    mConfigs.push_front(config); // the new config is added to the front, but keep the old config around in case something is still referencing it

    const ConfigMap* loggingMap = config->getMap("logging");

    if (loggingMap != nullptr)
    {
        LoggingConfig configTdf;
        ConfigDecoder decoder(*loggingMap);

        if (decoder.decode(&configTdf))
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].updateConfig: Updating Logging Values.");
            Logger::configure(configTdf, false);
        }
    }

    if (!initializeModules())
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].updateConfig: Error re-initializing modules.");
        return false;
    }
    return true;
}

void Stress::startInitialization(Selector& selector)
{
    selector.scheduleCall(this, &Stress::runInitialization);
}

void Stress::startStress(Selector& selector)
{
    selector.scheduleCall(this, &Stress::runStressTest);
}

void Stress::destroy()
{
    gSelector->cancelAllTimers(this);

    InstancesById::iterator instItr = mInstances.begin();
    for(; instItr != mInstances.end(); ++instItr)
    {
        instItr->second->shutdown();
        delete instItr->second;
    }
    mInstances.clear();

    ConnectionsById::iterator conItr = mConnections.begin();
    for(; conItr != mConnections.end(); ++conItr)
    {
        delete conItr->second;
    }
    mConnections.clear();

    ModulesByName::iterator modItr = mModules.begin();
    for(; modItr != mModules.end(); ++modItr)
    {
        modItr->second->dumpStats(false);
        delete modItr->second;
    }
    mModules.clear();
}

/*** Private Methods *****************************************************************************/

bool Stress::initializeModules()
{
    BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].initializeModules: Initializing modules.");

    const ConfigSequence* moduleList = mConfigs.front()->getSequence("modules");
    if (moduleList == nullptr)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initializeModules: No modules configured.");
        return false;
    }

    size_t totalConnections = mConnections.size();
    const ConfigMap* distributionMap = mConfigs.front()->getMap("distribution");

    const ConfigMap* platformDistroMap = mConfigs.front()->getMap("platform-distribution");
    if (platformDistroMap != nullptr)
    {
        // Build the distribution map from config
        double sum = 0;
        while (platformDistroMap->hasNext())
        {
            const char8_t* platformName = platformDistroMap->nextKey();
            double percentage = platformDistroMap->getDouble(platformName, 0);
            Blaze::ClientPlatformType platformType;
            if (!ParseClientPlatformType(platformName, platformType))
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initializeModules: Invalid platform type listed: " << platformName << "\n");
                return false;
            }
            mPlatformDist[platformType] = percentage; // member variable to store distribution data
            sum += percentage;
        }
        if (sum != 1.0)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initializeModules: Invalid distribution sum: " << sum << " != 1.0\n");
            return false;
        }
    }

    const ConfigMap* platformSpecificationMap = mConfigs.front()->getMap("platform-specification");
    if (platformSpecificationMap != nullptr)
    {
        if (platformSpecificationMap->hasNext())
        {
            const char8_t* platform = platformSpecificationMap->getString("platform", "");
            if (!ParseClientPlatformType(platform, mPlatformSpecification))
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initializeModules: Invalid platform type listed: " << platform << "\n");
                return false;
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initializeModules: Missing platform config in platform-specification\n");
            return false;
        }
    }

    while (moduleList->hasNext())
    {
        const char8_t* name = moduleList->nextString(nullptr);
        StressModule* module = nullptr;

        bool found = false;
        for(ModuleCreationInfo* info = gModuleCreators; info->name != nullptr; ++info)
        {
            if (blaze_stricmp(name, info->name) == 0)
            {
                found = true;
                module = (*info->func)();
                mModules[name] = module;
                break;
            }
        }
        if(!found)
        {
            for(ModuleCreationInfo* info = gCustomModuleCreators; info->name != nullptr; ++info)
            {
                if (blaze_stricmp(name, info->name) == 0)
                {
                    found = true;
                    module = (*info->func)();
                    mModules[name] = module;
                    break;
                }
            }
        }

        if (!found)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initializeModules: Unrecognized module name: '" << name << "'");
            return false;
        }
    }

    bool sumsWereZero = false;
    if (distributionMap != nullptr)
    {
        // Build the distribution map from config
        double sum = 0;
        eastl::map<eastl::string, double> modulePercent;
        while (distributionMap->hasNext())
        {
            const char8_t* moduleName = distributionMap->nextKey();
            ModulesByName::iterator i;
            if ((i = mModules.find(moduleName)) == mModules.end())
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initializeModules: Unknown module '" << moduleName << "' specified in module distribution.");
                return false;
            }
            double percentage = distributionMap->getDouble(moduleName, 0);
            modulePercent[moduleName] = percentage;
            sum += percentage;
        }

        if (sum <= 0.0 + FLT_EPSILON)
        {
            sumsWereZero = true;
        }
        else
        {
            size_t connsRemaining = totalConnections;
            ModulesByName::iterator i = mModules.begin();
            ModulesByName::iterator e = mModules.end();
            for(; i != e; ++i)
            {
                double percentage = modulePercent[i->first] / sum;
                size_t connsToUse = (size_t)(totalConnections * percentage);
                i->second->setConnectionDistribution((uint32_t) (i+1 != e ? connsToUse : connsRemaining));
                connsRemaining -= connsToUse;
            }
        }
    }

    if (distributionMap == nullptr || sumsWereZero == true)
    {
        // No distribution map is provided so allocate equally amongst all declared modules
        size_t connsRemaining = totalConnections;
        size_t instancesPerModule = totalConnections / mModules.size();
        ModulesByName::iterator i = mModules.begin();
        ModulesByName::iterator e = mModules.end();
        for(; i != e; ++i)
        {
            // The last module gets what ever connections are remaining, so there are no leftovers:
            i->second->setConnectionDistribution((uint32_t) (i+1 != e ? instancesPerModule : connsRemaining));
            connsRemaining -= instancesPerModule;
        }
    }

    ModulesByName::iterator i = mModules.begin();
    ModulesByName::iterator e = mModules.end();
    for(; i != e; ++i)
    {
        const ConfigMap* moduleConfig = mConfigs.front()->getMap(i->first.c_str());
        if (moduleConfig != nullptr)
        {
            if (!i->second->initialize(*moduleConfig, mConfigBootUtil))
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initializeModules: Failed to initialize module: '" << i->first.c_str() << "'");
                return false;
            }   
        }
    }
    return true;
}

bool Stress::initializeConnectionInfo(uint32_t* numConnections)
{
    const ConfigMap* connInfo = mConfigs.front()->getMap("connection-info");
    if (connInfo == nullptr)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initializeConnectionInfo: No connection-info specified in config file.");
        return false;
    }

    const char8_t* protocolType = connInfo->getString("protocol", "fire2");
    blaze_strnzcpy(mProtocolType, protocolType, sizeof(mProtocolType));
    const char8_t* encoderType = connInfo->getString("encoder", "heat2");
    blaze_strnzcpy(mEncoderType, encoderType, sizeof(mEncoderType));
    const char8_t* decoderType = connInfo->getString("decoder", "heat2");
    blaze_strnzcpy(mDecoderType, decoderType, sizeof(mDecoderType));
    mSecure = connInfo->getBool("secure", false);

    *numConnections = mArguments.numberOfConnections != 0 ? mArguments.numberOfConnections : connInfo->getUInt32("num-connections", 100);
    return true;
}

bool Stress::initializeConnections(uint32_t numConnections)
{
    // Create instance objects for each connection
    ServerList::const_iterator serverIt = mServers.begin();
    for(uint32_t conn = 0; conn < numConnections; ++conn)
    {
        InstanceResolver* resolver;
        if (!mRedirectorAddress.empty() && !mServiceName.empty())
        {
            resolver = BLAZE_NEW RedirectorInstanceResolver(conn, mSecure, mRedirectorAddress.c_str(), mServiceName.c_str());
        }
        else
        {
            resolver = BLAZE_NEW StaticInstanceResolver(*serverIt);
            ++serverIt;
            if (serverIt == mServers.end())
            {
                serverIt = mServers.begin();
            }
        }

        StressConnection* connection = BLAZE_NEW StressConnection(conn, mSecure, *resolver);
        if (!connection->initialize(mProtocolType, mEncoderType, mDecoderType))
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initializeConnections: Unable to initialize connection.");
            return false;
        }
        mConnections[conn] = connection;
    }
    
    return true;
}

bool Stress::initializeInstances()
{
    // Create the instances
    uint32_t totalConns = 0;
    ConnectionsById::iterator connItr = mConnections.begin();
    ModulesByName::iterator modItr = mModules.begin();
    ModulesByName::iterator modEnd = mModules.end();
    for (; modItr != modEnd; ++modItr)
    {
        eastl::vector<Blaze::ClientPlatformType> randClientPlatformOrder;
        if (!mPlatformDist.empty())
        {            
            for (auto platType : mPlatformDist)
            {
                uint32_t consoleCount = (uint32_t)(platType.second * modItr->second->getTotalConnections() + 1);    //always round up 1 more to prevent index exceeds range
                randClientPlatformOrder.insert(randClientPlatformOrder.end(), consoleCount, platType.first);
            }
            struct Rand { eastl_size_t operator()(eastl_size_t n) { return (eastl_size_t)(rand() % n); } }; // Note: The C rand function is poor and slow.
            Rand randInstance;
            eastl::random_shuffle(randClientPlatformOrder.begin(), randClientPlatformOrder.end(), randInstance);
        }
        else
        {
            randClientPlatformOrder.insert(randClientPlatformOrder.end(), modItr->second->getTotalConnections(), mPlatformSpecification);
        }

        for(size_t i = 0; i < modItr->second->getTotalConnections(); ++i)
        {
            StressModule* module = modItr->second;
            if (connItr == mConnections.end())
                break;
            StressConnection* connection = connItr->second;
            connItr++;
            ClientPlatformType platformType = randClientPlatformOrder[i];
            bool connectToCommonServer = !mPlatformDist.empty();
            Login* login = mLoginManager.getLogin(connection, platformType, connectToCommonServer);
            if (login == nullptr)
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initializeInstances: Unable to create login instance for " << i);
            }
            else
            {
                StressInstance* instance = module->createInstance(connection, login);
                if (instance != nullptr)
                {
                    module->incTotalInstances();
                    BLAZE_TRACE_LOG(Log::SYSTEM, "[Stress].initializeInstances: stressInst:" << instance << ", conn:" << connection << "/" << i << ", login:" << login);
                    mInstances[connection->getIdent()] = instance;
                    ++totalConns;
                }
                else
                {
                    BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].initializeInstances: Unable to create stress instance for " << i);
                    return false;
                }
            }
        }
    }

    if (totalConns > 0)
        return true;

    BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].initializeInstances: No instances started, shutting down.");

    return false;
}

bool Stress::initializeStartup()
{
    const ConfigMap* startup = mConfigs.front()->getMap("startup");
    if (startup == nullptr)
        return true;

    mStartupConfig.blockSize = (mArguments.blockSize != 0) ? mArguments.blockSize : startup->getUInt32("block-size", 0);
    mStartupConfig.blockDelay = (mArguments.blockDelay != 0) ? mArguments.blockDelay : startup->getUInt32("block-delay", 0);
    mStartupConfig.instanceDelay = (mArguments.instanceDelay != 0) ? mArguments.instanceDelay : startup->getUInt32("instance-delay", 0);
    return true;
}

void Stress::runInitialization()
{
    if (!initialize())
    {
        gSelector->shutdown();
        return;
    }
}

void Stress::runStressTest()
{
    // Startup the the instances in a controlled manner as per the startup config
    if (mStartupConfig.blockSize == 0)
        mStartupConfig.blockSize = (uint32_t)mInstances.size();
    
    uint64_t instanceDelay = 0;
    InstancesById::const_iterator it = mInstances.begin();
    for(uint32_t i = 1; it != mInstances.end(); ++it, ++i)
    {
        TimerId timerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + instanceDelay, 
            this, &Stress::startInstance, it->second, "Stress::startInstance");
        if(INVALID_TIMER_ID == timerId)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[Stress].runStressTest: Unable to schedule instance timer.");
            continue;
        }
        BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].runStressTest: " << "Starting Stress Instance with timer id(" << timerId << "), index(" << it->second->getIdent() << ")");
        instanceDelay += mStartupConfig.instanceDelay * 1000;

        // Suppressed block delay - there are too many knobs to adjust to control the rate of user ramp up,
        // rather than an unrealistic scenario where we ramp up users quickly half the time, and sleep the other half,
        // it is recommended to adjust the 'iltRampUpMin' config setting when running ILT
        //if(0 == (i % mStartupConfig.blockSize))
        //{
        //    instanceDelay += mStartupConfig.blockDelay * 1000;
        //}
    }

#ifdef EA_PLATFORM_LINUX
    if (mConfigFileDir.empty())
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].runStressTest: Nowhere to add watch -- config reloading is disabled");
    }
    else
    {
        // start the config reload checking
        char8_t stringBuf[64];
        TimeValue interval = instanceDelay;
        BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].runStressTest: Start checking in " << interval.toIntervalString(stringBuf, sizeof(stringBuf)));
        gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + instanceDelay, this, &Stress::checkConfigReload, "Stress::checkConfigReload");
    }
#endif
}

void Stress::startInstance(StressInstance* instance)
{
    instance->start();
}

int32_t Stress::getConnectedCount() const
{
    int32_t count = 0;
    InstancesById::const_iterator itr = mInstances.begin();
    InstancesById::const_iterator end = mInstances.end();
    for(; itr != end; ++itr)
    {
        StressInstance* inst = itr->second;
        if (inst->getConnection()->connected())
            ++count;
    }
    return count;
}

#ifdef EA_PLATFORM_LINUX
bool Stress::addOrUpdateConfigWatch()
{
    if (mConfigFileDirFd < 0)
    {
        mConfigFileDirFd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
        if (mConfigFileDirFd < 0)
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].getConnectedCount: Error initializing inotify file descriptor: " << strerror(errno));
        }
        else
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].getConnectedCount: Initialized inotify file descriptor: " << mConfigFileDirFd);
        }
    }

    if (mConfigFileDirFd <= 0)
    {
        // technically, 0 is not an invalid fd; but it's a standard stream (stdin) that we don't want to use
        BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].getConnectedCount: Unable to add watch for inotify file descriptor: " << mConfigFileDirFd);
        return false;
    }

    mConfigFileDirWatch = inotify_add_watch(mConfigFileDirFd, mConfigFileDir.c_str(), IN_MODIFY);
    BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].getConnectedCount: Adding watch (" << mConfigFileDirWatch << ") on " << mConfigFileDir.c_str());
    if (mConfigFileDirWatch < 0)
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].getConnectedCount: Error adding watch: " << strerror(errno));
        return false;
    }

    return true;
}

bool Stress::doConfigReload()
{
    if (mConfigFileDirFd < 0 || mConfigFileDirWatch < 0)
    {
        return addOrUpdateConfigWatch();
    }

    struct pollfd pollFd = { mConfigFileDirFd, POLLIN, 0 };

    int changeObserved = poll(&pollFd, 1, 1000); // 1s (1000ms) timeout

    if (changeObserved < 0)
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].doConfigReload: Error polling: " << strerror(errno));
        return false;
    }
    else if (changeObserved == 0)
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].doConfigReload: No change detected");
    }
    else
    {
        // Process the event(s)
        int len = 0;
        char buf[BUF_LEN];
        BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].doConfigReload: Reading events...");
        len = read(mConfigFileDirFd, buf, BUF_LEN);

        if (len <= 0)
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].doConfigReload: Error reading events: " << strerror(errno));
            return false;
        }

        BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].doConfigReload: Read events buffer of length: " << len);

        struct inotify_event* event;

        int32_t numEvents = 0;
        int i = 0;
        while (i < len)
        {
            if (i + ((int)(EVENT_SIZE)) > len)
            {
                BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].doConfigReload: Incomplete event after " << numEvents << "event(s)");
                break;
            }

            event = (struct inotify_event*) &buf[i];
            i += EVENT_SIZE;

            BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].doConfigReload: Event[" << numEvents << "]->wd: " << event->wd << ", ->mask: " << SbFormats::HexUpper(event->mask) << ", ->cookie: " << event->cookie << ", ->len: " << event->len);

            if (event->len > 0)
            {
                if (i + ((int)(event->len)) > len)
                {
                    BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].doConfigReload: Incomplete name after " << numEvents << "event(s)");
                    break;
                }
                BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].doConfigReload: Event[" << numEvents << "] " << ((event->mask & IN_ISDIR) ? "dir" : "file") << " name: " << event->name);
                // Currently name of file that is modified is not required as there is only one config file that will be updated.
                // File name is required if there are multiple config files and reload of only modified file is required.
                i += event->len;
            }

            ++numEvents;
        }

        if (numEvents == 0)
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].doConfigReload: Unable to read a complete event");
            return false;
        }

        // for simplicity, we don't care what file was modified (even if it's a temp editing file) as long as the last event indicates a modification in the dir tree
        if (event->mask & IN_MODIFY)
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].doConfigReload: File modification detected -- starting config reload");

            // Read from config file
            EA_ASSERT(mConfigFileName != nullptr);
            ConfigFile* config = ConfigFile::createFromFile("./", mConfigFileName, true, mPredefineMap);
            if (config == nullptr)
            {
                BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].doConfigReload: Unable to read config file: " << mConfigFileName);
                return false;
            }

            close(mConfigFileDirFd);
            mConfigFileDirFd = -1;
            updateConfig(config);
            return addOrUpdateConfigWatch();
        }
        else if (event->mask & IN_IGNORED)
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].doConfigReload: Watch was removed by kernel -- adding new watch");
            return addOrUpdateConfigWatch();
        }
    }

    return true;
}

void Stress::checkConfigReload()
{
    BLAZE_INFO_LOG(Log::SYSTEM, "[Stress].checkConfigReload: Checking for config reload");
    if (doConfigReload())
    {
        mConfigFileDirWatchRetry = 0;
    }
    else
    {
        ++mConfigFileDirWatchRetry;
    }

    if (mConfigFileDirWatchRetry >= 5)
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].checkConfigReload: Disabling config reload checking after " << mConfigFileDirWatchRetry << " consecutive retries");
        return;
    }
    else if (mConfigFileDirWatchRetry > 0)
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[Stress].checkConfigReload: Config reload checking has failed " << mConfigFileDirWatchRetry << " time(s) in a row");
    }

    // + 1 minute
    gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + 60000000, this, &Stress::checkConfigReload, "StressRunner::checkConfigReload");
}
#endif

} // Stress
} // Blaze

