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
#include "framework/config/config_map.h"
#include "framework/config/config_file.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{
namespace Stress
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

extern ModuleCreationInfo gModuleCreators[];

/*** Public Methods ******************************************************************************/

Stress::Stress(const ConfigMap& config, const CmdLineArguments& arguments)
    : mConfigBootUtil(NULL),
      mConfig(config),
      mServers(),
      mCreateAccountsConnection(NULL),
      mArguments(arguments),
	  mMaxFramesize(262144)
{    
}

Stress::Stress(const ConfigMap& config, const InetAddress& serverAddress, const CmdLineArguments& arguments)
    : mConfigBootUtil(NULL),
      mConfig(config),
      mServers(),
      mCreateAccountsConnection(NULL),
      mArguments(arguments),
	  mMaxFramesize(262144)
{
    mServers.push_back(serverAddress);
}

Stress::~Stress()
{
    delete mConfigBootUtil;
}

bool Stress::initialize()
{
    //Kick off our boot config so modules can use it to read in the server configuration
    const ConfigMap* configCfg = mConfig.getMap("serverConfiguration");
    if (configCfg != NULL)
    {
        const char8_t* bootFile = configCfg->getString("bootFile", "");
        const char8_t* overrideBootFile = configCfg->getString("overrideBootFile", "");

        if (bootFile != NULL && *bootFile != '\0')
        {
            PredefineMap externalDefines;
            externalDefines.addBuiltInValues();
            const ConfigMap* predefinesCfg = configCfg->getMap("predefines");
            if (predefinesCfg != NULL)
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
    
    bool useRedirector = false;
    const ConfigMap* serversCfg = mConfig.getMap("servers");
    if (serversCfg != NULL)
    {
        // Stress client uses redirector to find available instance
        const char8_t* serviceName = serversCfg->getString("serviceName", NULL);
        const char8_t* rdirAddress = serversCfg->getString("redirectorAddress", NULL);
        if (serviceName != NULL && rdirAddress != NULL)
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "Stress.initialize: using redirector(" << rdirAddress << ") to resolve slave instances for " << serviceName);
            mServiceName = serviceName;
            mRedirectorAddress = rdirAddress;
            useRedirector = true;
        }
        else
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "No serviceName or redirectorAddress specified in redirector config.");
        }
    }

    if (!useRedirector)
    {
        // If we have a server/port already supplied by command line, ignore config servers
        if(mServers.empty())
        {
            const ConfigMap* serversCfg2 = mConfig.getMap("servers");
            if (serversCfg2 != NULL)
            {
                //  fill server list with specified values.
                uint32_t serverIndex = 0;
                char8_t fieldName[16];
                blaze_snzprintf(fieldName, sizeof(fieldName), "server-%u", serverIndex);
                const char8_t* serverAdr = serversCfg2->getString(fieldName, "null");
                while (serverAdr != NULL && blaze_strcmp(serverAdr, "null") != 0)
                {
                    mServers.push_back(InetAddress(serverAdr));
                    ++serverIndex;
                    blaze_snzprintf(fieldName, sizeof(fieldName), "server-%u", serverIndex);
                    serverAdr = serversCfg2->getString(fieldName, "null");
                }
            }
        }

        //  resolve each server, removing servers that are unresolvable.
        for (ServerList::iterator it = mServers.begin(); it != mServers.end();)
        {
            InetAddress addr = *it;
            if (!gNameResolver->blockingResolve(addr))
            {
                char8_t addrBuf[256];
                BLAZE_WARN_LOG(Log::SYSTEM, "Unable to resolve server address: " << addr.toString(addrBuf, sizeof(addrBuf)) << "\n");

                it = mServers.erase(it);
            }
            else
            {
                ++it;
            }
        }
        if (mServers.size() > 1)
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "Stress.initialize: Resolved " << mServers.size() << " servers for connection.\n");
        }
        else if (mServers.empty())
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "No available servers.\n");
            return false;
        }
    }


    BLAZE_INFO(Log::SYSTEM, "Stress.initialize: using %s allocator.\n", Allocator::getTypeName());

    if (!initializeConnections())
        return false;

    const ConfigMap* loginCfg = mConfig.getMap("account");
    if (loginCfg != NULL)
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

void Stress::startInitialization(Selector& selector)
{
    selector.scheduleCall(this, &Stress::runInitialization);
}

void Stress::startStress(Selector& selector)
{
    selector.scheduleCall(this, &Stress::runStressTest);
}

void Stress::startAccountCreation(Selector& selector, uint32_t count)
{
    selector.scheduleCall(this, &Stress::runAccountCreation, count);
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

    delete mCreateAccountsConnection;
}

/*** Private Methods *****************************************************************************/

bool Stress::initializeModules()
{
    const ConfigSequence* moduleList = mConfig.getSequence("modules");
    if (moduleList == NULL)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "No modules configured.");
        return false;
    }

    size_t totalConnections = mConnections.size();
    const ConfigMap* distributionMap = mConfig.getMap("distribution");

    while (moduleList->hasNext())
    {
        const char8_t* name = moduleList->nextString(NULL);
        StressModule* module = NULL;

        bool found = false;
        for(ModuleCreationInfo* info = gModuleCreators; info->name != NULL; ++info)
        {
            if (blaze_stricmp(name, info->name) == 0)
            {
                found = true;
                module = (*info->func)();
                mModules[name] = module;
                break;
            }
        }

        if (!found)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "Unrecognized module name: '" << name << "'");
            return false;
        }
    }

    bool sumsWereZero = false;
    if (distributionMap != NULL)
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
                BLAZE_ERR_LOG(Log::SYSTEM, "Unknown module '" << moduleName << "' specified in module distribution.");
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

    if (distributionMap == NULL || sumsWereZero == true)
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
        const ConfigMap* moduleConfig = mConfig.getMap(i->first.c_str());
        if (moduleConfig != NULL)
        {
            if (!i->second->initialize(*moduleConfig, mConfigBootUtil))
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "Failed to initialize module: '" << i->first.c_str() << "'");
                return false;
            }   
        }
    }
    return true;
}

bool Stress::initializeConnections()
{
    const ConfigMap* connInfo = mConfig.getMap("connection-info");
    if (connInfo == NULL)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "No connection-info specified in config file.");
        return false;
    }

    const char8_t* protocolType = connInfo->getString("protocol", "fire2");
    blaze_strnzcpy(mProtocolType, protocolType, sizeof(mProtocolType));
    const char8_t* encoderType = connInfo->getString("encoder", "heat2");
    blaze_strnzcpy(mEncoderType, encoderType, sizeof(mEncoderType));
    const char8_t* decoderType = connInfo->getString("decoder", "heat2");
    blaze_strnzcpy(mDecoderType, decoderType, sizeof(mDecoderType));
    mSecure = connInfo->getBool("secure", false);
	mMaxFramesize = connInfo->getUInt32("maxFrameSize", 307200);
    uint32_t numConnections = mArguments.numberOfConnections != 0 ? mArguments.numberOfConnections : connInfo->getUInt32("num-connections", 100);

    // Create instance objects for each connection
    ServerList::const_iterator serverIt = mServers.begin();
    for(uint32_t conn = 0; conn < numConnections; ++conn)
    {
        InstanceResolver* resolver;
        if (!mRedirectorAddress.empty() && !mServiceName.empty())
        {
            resolver = BLAZE_NEW RedirectorInstanceResolver(mSecure, mRedirectorAddress.c_str(), mServiceName.c_str(), mMaxFramesize);
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
        if (!connection->initialize(mProtocolType, mEncoderType, mDecoderType, mMaxFramesize))
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "Unable to initialize connection.");
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
    for(; modItr != modEnd; ++modItr)
    {
        for(size_t i = 0; i < modItr->second->getTotalConnections(); ++i)
        {
            StressModule* module = modItr->second;
            if (connItr == mConnections.end())
                break;
            StressConnection* connection = connItr->second;
            connItr++;
            Login* login = mLoginManager.getLogin(connection, module->isStressLogin());
            if (login == NULL)
            {
                BLAZE_ERR_LOG(Log::SYSTEM, "Unable to create login instance for " << i);
            }
            else
            {
                StressInstance* instance = module->createInstance(connection, login);
                module->incTotalInstances();
                BLAZE_TRACE_LOG(Log::SYSTEM, "stressInst:" << instance << ", conn:" << connection << "/" << i << ", login:" << login);
                mInstances[connection->getIdent()] = instance;
                ++totalConns;
            }
        }
    }

    if (totalConns > 0)
        return true;

    BLAZE_INFO_LOG(Log::SYSTEM, "No instances started, shutting down.");

    return false;
}

bool Stress::initializeStartup()
{
    const ConfigMap* startup = mConfig.getMap("startup");
    if (startup == NULL)
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
            BLAZE_ERR_LOG(Log::SYSTEM, "Unable to schedule instance timer.");
            continue;
        }
        BLAZE_INFO_LOG(Log::SYSTEM, "[" << it->second->getIdent() << "] " << "Starting instance with timer id(" << timerId << ")");
        instanceDelay += mStartupConfig.instanceDelay * 1000;
        if(0 == (i % mStartupConfig.blockSize))
        {
            instanceDelay += mStartupConfig.blockDelay * 1000;
        }
    }
}

void Stress::runAccountCreation(uint32_t count)
{
    InstanceResolver* resolver = BLAZE_NEW StaticInstanceResolver(mServers[0]);
    mCreateAccountsConnection = BLAZE_NEW StressConnection(0xffffffff, mSecure, *resolver);
    if (!mCreateAccountsConnection->initialize(mProtocolType, mEncoderType, mDecoderType, mMaxFramesize))
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "Unable to initialize connection.");
        delete mCreateAccountsConnection;
        gSelector->shutdown();
        return;
    }

    if (!mLoginManager.createAccounts(mCreateAccountsConnection, count,
                LoginManager::CreateAccountsCb(this, &Stress::onAccountsCreated)))
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "Unable to start account creation process.");
        gSelector->shutdown();
        return;
    }
}

void Stress::onAccountsCreated(uint32_t count)
{
    BLAZE_INFO_LOG(Log::SYSTEM, "Created " << count << " accounts.");

    gSelector->shutdown();
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

} // Stress
} // Blaze

