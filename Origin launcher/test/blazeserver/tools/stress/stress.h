/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_STRESS_H
#define BLAZE_STRESS_STRESS_H

/*** Include files *******************************************************************************/

#include "stressinstance.h"
#include "loginmanager.h"
#include "framework/connection/inetaddress.h"
#include "EASTL/vector_map.h"
#include "EASTL/string.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConfigMap;
class Selector;
class ConfigBootUtil;
class PredefineMap;

namespace Stress
{

class StressModule;

typedef StressModule* (*ModuleCreatorFunc)();

struct ModuleCreationInfo
{
    const char8_t* name;
    ModuleCreatorFunc func;
};

struct CmdLineArguments
{
    uint32_t numberOfConnections;
    uint32_t blockSize;
    uint32_t blockDelay;
    uint32_t instanceDelay;
    int64_t startIndex;
    eastl::string useStressLogin;

    CmdLineArguments() :
        numberOfConnections(0),
        blockSize(0),
        blockDelay(0),
        instanceDelay(0),
        startIndex(-1),
        useStressLogin("")
    {
    }

    CmdLineArguments(const CmdLineArguments& ref)
    {
        numberOfConnections = ref.numberOfConnections;
        blockSize = ref.blockSize;
        blockDelay = ref.blockDelay;
        instanceDelay = ref.instanceDelay;
        startIndex = ref.startIndex;
        useStressLogin = ref.useStressLogin;
    }
};

class StressInstance;

class Stress
{
    NON_COPYABLE(Stress);

public:
    Stress(ConfigMap* config, const char8_t* configFileName, PredefineMap& predefineMap, const CmdLineArguments& arguments);
    Stress(ConfigMap* config, const char8_t* configFileName, PredefineMap& predefineMap, const InetAddress& serverAddress, const CmdLineArguments& arguments);
    virtual ~Stress();

    bool initialize();
    bool updateConfig(ConfigMap* config);

    void startInitialization(Selector& selector);
    void startStress(Selector& selector);

    virtual void destroy();

    StressModule* getModule(const char8_t* name) const;

    size_t getMaxConnections() const {
        return mConnections.size();
    }

    LoginManager& getLoginManager() { return mLoginManager; }
    int32_t getConnectedCount() const;
    size_t getInstanceCount() const { return mInstances.size(); }
    
    ConfigBootUtil* getConfigBootUtil() { return mConfigBootUtil; }
private:
    typedef eastl::vector_map<eastl::string,StressModule*> ModulesByName;
    typedef eastl::vector_map<int32_t,StressConnection*> ConnectionsById;
    typedef eastl::vector_map<int32_t,StressInstance*> InstancesById;
    typedef eastl::vector_map<TimerId,StressInstance*> InstancesByTimerId;
    typedef eastl::vector_map<Blaze::ClientPlatformType, double> PlatformDistribution;

    struct StartupConfig
    {
        uint32_t blockSize;
        uint32_t instanceDelay;
        uint32_t blockDelay;

        StartupConfig() : blockSize(0), instanceDelay(0), blockDelay(0) { }
    };

    ConfigBootUtil* mConfigBootUtil;
    typedef eastl::list<ConfigMap*> ConfigList;
    ConfigList mConfigs;
    const char8_t* mConfigFileName;
    PredefineMap& mPredefineMap;
    typedef eastl::vector<InetAddress> ServerList;
    ServerList mServers;
    eastl::string mRedirectorAddress;
    eastl::string mServiceName;
    ClientPlatformType mPlatformSpecification;

    char8_t mProtocolType[256];
    char8_t mEncoderType[256];
    char8_t mDecoderType[256];
    bool mSecure;
    StartupConfig mStartupConfig;
    ModulesByName mModules;
    ConnectionsById mConnections;
    InstancesById mInstances;
    LoginManager mLoginManager;
    PlatformDistribution mPlatformDist;

    CmdLineArguments mArguments;

    bool initializeModules();
    bool initializeConnectionInfo(uint32_t* numConnections);
    bool initializeConnections(uint32_t numConnections);
    bool initializeInstances();
    bool initializeStartup();
    void runInitialization();
    void runStressTest();
    void startInstance(StressInstance*);
    void checkCpuBudget();

#ifdef EA_PLATFORM_LINUX
    bool addOrUpdateConfigWatch();
    bool doConfigReload();
    void checkConfigReload();

    int32_t mConfigFileDirWatchRetry;
    int mConfigFileDirFd; // file descriptor
    int32_t mConfigFileDirWatch; // file watch
    eastl::string mConfigFileDir; // directory path
#endif
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_STRESS_H

