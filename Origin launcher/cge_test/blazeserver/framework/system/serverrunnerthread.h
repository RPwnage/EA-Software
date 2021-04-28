/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVERRUNNERTHREAD_H
#define BLAZE_SERVERRUNNERTHREAD_H

/*** Include files *******************************************************************************/

#include "framework/system/blazethread.h"
#include "framework/controller/controller.h"
#include "eathread/eathread_storage.h"
#include "framework/redis/redismanager.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Selector;
class NameResolver;
class ShellRunner;
class Controller;
class DbScheduler;
class RedisManager;
class Localization;
class ProfanityFilter;
class FiberManager;
class Replicator;
class CensusDataManager;
namespace Metrics { class MetricsCollection; }
class BootConfig;
class Random;
class BlockingSocketManager;
class OutboundHttpService;
class OutboundMetricsManager;
class VaultManager;
class SslContextManager;

class ServerRunnerThread : public BlazeThread
{
    NON_COPYABLE(ServerRunnerThread);

public:
    ServerRunnerThread(uint16_t basePort, const char8_t* baseName, const char8_t* instanceName, const char8_t* hostName);
    ~ServerRunnerThread() override;

    Selector& getSelector() const { return *mSelector; }
    
    void stopInstance();
    void startConfigReload();
    void setServiceState(bool inService);

    // pure virtual methods from BlazeThread
    void stop() override;
    void run() override;

    virtual void cleanup();
protected:
    virtual bool initialize();

private:
    uint16_t mBasePort;
    eastl::string mBaseName;
    eastl::string mInstanceName;
    volatile bool mStarted; 
    volatile bool mRunning;
    volatile bool mStopping;
    volatile bool mCleanedUp;

    EA::Thread::Mutex mThreadLock;

    Selector* mSelector;
    NameResolver* mNameResolver;
    ShellRunner* mShellRunner;
    Controller* mController;
    DbScheduler* mDbScheduler;
    RedisManager* mRedisManager;
    Localization* mLocalization;
    ProfanityFilter* mProfanityFilter;  
    Replicator* mReplicator;
    CensusDataManager* mCensusDataManager;
    Random* mRandom;
    BlockingSocketManager* mBlockingSocketManager;
    OutboundHttpService* mOutboundHttpService;
    OutboundMetricsManager* mOutboundMetricsManager;
    VaultManager* mVaultManager;
    Metrics::MetricsCollection mMetricsCollection;
    Metrics::MetricsCollection& mInstanceCollection;
    SslContextManager* mSslContextManager;

    void setThreadLocalVars();
    void clearThreadLocalVars() const;

    void setStarted() { mStarted = true;}
    
    void stopControllerInternal();
    void stopCommonInternal();
};

} // namespace Blaze

#endif // BLAZE_SERVERRUNNERTHREAD_H

