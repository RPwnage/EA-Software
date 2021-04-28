/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_GAMEGROUPSMODULE_H
#define BLAZE_STRESS_GAMEGROUPSMODULE_H

/*** Include files *******************************************************************************/

#include "stressmodule.h"
#include "stressinstance.h"

#include "gamemanagerutil.h"
#include "gamegroupsutil.h"

#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/rpc/gamemanagerslave.h"

#include "EATDF/time.h"
#include "EASTL/set.h"


using namespace Blaze::GameManager;

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Stress
{

class StressInstance;
class StressConnection;
class GamegroupsInstance;
class Login;


class GamegroupsModule : public StressModule
{
    NON_COPYABLE(GamegroupsModule);

public:
    static StressModule* create();

    enum Action
    {
        ACTION_INVALID = -1,
        ACTION_NOOP,
        ACTION_GET_GAME_DATA
    };

    Action getAction() const {
        return mAction;
    }

    TimeValue getModuleTimestamp() const {
        return mInitTimestamp;
    }

    enum Metric
    {
        METRIC_NOOP,
        METRIC_GET_GAME_DATA,
        NUM_METRICS
    };

    void addMetric(Metric metric, const TimeValue& ms);

public:
    // Perform any necessary configuration prior to starting the stress test
    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;

    // Called by the core system to create stress instances for this module
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    const char8_t* getModuleName() override {return "gamegroups";}
    void releaseInstance(StressInstance *instance);

    ~GamegroupsModule() override;

    // Called when the stress tester determines how many connections the module will use out of the total pool
    void setConnectionDistribution(uint32_t conns) override;

    bool saveStats() override;

    //  determine what action to execute based on configuration parameters.
    float queryDoKickFreq() const {
        return mDoKickFreq;
    }
    float queryDoKickHostFreq() const {
        return mDoKickHostFreq;
    }
    float queryDoKickAdminFreq() const {
        return mDoKickAdminFreq;
    }
    float queryDoSetAttrFreq() const {
        return mSetAttrFreq;
    }

    GameManagerUtil* getGameManagerUtil() {
        return mGameUtil;
    }
    GamegroupsUtil* getGamegroupsUtil() {
        return mGamegroupsUtil;
    }

protected:
    GamegroupsModule();

    bool parseConfig(const ConfigMap& config);

private:
    const ConfigMap* mConfigFile;

    Action mAction;
    TimeValue mInitTimestamp;

    uint64_t mMetricTime[NUM_METRICS];
    uint64_t mMetricCount[NUM_METRICS];

    float mDoKickFreq;
    float mDoKickHostFreq;
    float mDoKickAdminFreq;
    float mSetAttrFreq;

    typedef eastl::hash_map<int32_t, GamegroupsInstance*> InstanceMap;
    InstanceMap mInstances;

    GameManagerUtil *mGameUtil;
    GamegroupsUtil *mGamegroupsUtil;

    uint32_t mRemainingLeaderCount;
    uint32_t mConnsRemaining;
    uint32_t mTotalConns;
};


class GamegroupsInstance : public StressInstance
{
    NON_COPYABLE(GamegroupsInstance);
    friend class GamegroupsModule;

public:
    ~GamegroupsInstance() override;

    // This is the entry point for a stress instance to start running
    void start() override;

    void onLogin(BlazeRpcError result) override;

    BlazeId getBlazeId() const;
    BlazeId getBlazeUserId() const { return getBlazeId(); }  // DEPRECATED

protected:
    //Override this method to do your task
    BlazeRpcError execute() override;
    const char8_t *getName() const override;

    int32_t getSeed() const {
        return mTrialIndex + StressInstance::getIdent();
    }

    void setRole(GamegroupsUtilInstance::Role role) {
        mGamegroupInst.setRole(role);
    }

protected:
    GamegroupsInstance(GamegroupsModule *owner, StressConnection* connection, Login* login);
   
private:

    GamegroupsModule *mOwner;
    const char8_t *mName;
    GameManagerSlave *mProxy;
    int32_t mTrialIndex;

    GameManagerUtilInstance mGameInst;
    GamegroupsUtilInstance mGamegroupInst;
    eastl::set<BlazeId> mPendingKicks;
};


} // Stress
} // Blaze

#endif // BLAZE_STRESS_GAMEGROUPSMODULE_H

