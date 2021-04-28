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
    uint32_t startIndex;
	uint32_t psu;
    uint32_t blockSize;
    uint32_t blockDelay;
    uint32_t instanceDelay;

    CmdLineArguments() :
        numberOfConnections(0),
        startIndex(0),
		psu(0),
        blockSize(0),
        blockDelay(0),
        instanceDelay(0)
    {
    }

    CmdLineArguments(const CmdLineArguments& ref)
    {
        numberOfConnections = ref.numberOfConnections;
        startIndex = ref.startIndex;
		psu = ref.psu;
        blockSize = ref.blockSize;
        blockDelay = ref.blockDelay;
        instanceDelay = ref.instanceDelay;
    }
};

class StressInstance;

class Stress
{
    NON_COPYABLE(Stress);

public:
    Stress(const ConfigMap& config, const CmdLineArguments& arguments);
    Stress(const ConfigMap& config, const InetAddress& serverAddress, const CmdLineArguments& arguments);
    virtual ~Stress();

    bool initialize();

    void startInitialization(Selector& selector);
    void startStress(Selector& selector);
    void startAccountCreation(Selector& selector, uint32_t count);

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

    struct StartupConfig
    {
        uint32_t blockSize;
        uint32_t instanceDelay;
        uint32_t blockDelay;

        StartupConfig() : blockSize(0), instanceDelay(0), blockDelay(0) { }
    };

    ConfigBootUtil* mConfigBootUtil;
    const ConfigMap& mConfig;
    typedef eastl::vector<InetAddress> ServerList;
    ServerList mServers;
    eastl::string mRedirectorAddress;
    eastl::string mServiceName;

    char8_t mProtocolType[256];
    char8_t mEncoderType[256];
    char8_t mDecoderType[256];
    bool mSecure;
    StartupConfig mStartupConfig;
    ModulesByName mModules;
    ConnectionsById mConnections;
    InstancesById mInstances;
    LoginManager mLoginManager;
    StressConnection* mCreateAccountsConnection;

    CmdLineArguments mArguments;
    uint32_t mMaxFramesize;

    bool initializeModules();
    bool initializeConnections();
    bool initializeInstances();
    bool initializeStartup();
    void runInitialization();
    void runStressTest();
    void runAccountCreation(uint32_t count);
    void startInstance(StressInstance*);
    void onAccountsCreated(uint32_t count);
    void checkCpuBudget();
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_STRESS_H

