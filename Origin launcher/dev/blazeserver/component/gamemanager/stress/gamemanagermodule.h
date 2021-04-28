/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_GAMEMANAGERMODULE_H
#define BLAZE_STRESS_GAMEMANAGERMODULE_H

/*** Include files *******************************************************************************/

#include "stressmodule.h"
#include "stressinstance.h"

#ifndef LOGGER_CATEGORY
  // several of the MM headers include matchmakingutil.h, which expects LOGGER_CATEGORY to be defined;
  //   this is usually the case in the blazeServer, but we need to 'fake' it for the stress tool
  //   Note: this fixes some compile-time issues, the logger methods are not used by the stress tool
#include "framework/logger.h"
#define LOGGER_CATEGORY Log::UTIL
#endif

#include "gamemanagerutil.h"
#include "gamegroupsutil.h"

#include "EATDF/time.h"
#ifdef TARGET_util
#include "util/rpc/utilslave.h"
#endif

#include "gamemanager/tdf/gamebrowser.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"

#ifdef TARGET_arson
#include "arson/rpc/arsonslave.h"
#endif

using namespace Blaze::GameManager;

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Stress
{

class StressInstance;
class StressConnection;
class GameManagerInstance;
class Login;


class GameManagerModule : public StressModule
{
    NON_COPYABLE(GameManagerModule);

public:
    static StressModule* create();

    enum Action
    {
        ACTION_INVALID = -1,
        ACTION_NOOP
    };

    Action getAction() const {
        return mAction;
    }

    TimeValue getModuleTimestamp() const {
        return mInitTimestamp;
    }

public:
    // Perform any necessary configuration prior to starting the stress test
    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;

    // Called by the core system to create stress instances for this module
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    void releaseInstance(GameManagerInstance *instance);
    const char8_t* getModuleName() override {return "gamemanager";}

    ~GameManagerModule() override;

    bool saveStats() override;
    void setConnectionDistribution(uint32_t conns) override;

    //  determine what action to execute based on configuration parameters.
    GameManagerUtil* getGameManagerUtil() {
        return mGameUtil;
    }
    GamegroupsUtil* getGamegroupsUtil() {
        return mGamegroupsUtil;
    }

    float getHostMigrationRate() const {
        return mHostMigrationRate;
    }
    float getKickPlayerRate() const {
        return mKickPlayerRate;
    }
    float getSetAttributeRate() const {
        return mSetAttributeRate;
    }
    float getSetPlayerAttributeRate() const {
        return mSetPlayerAttributeRate;
    }

    float getJoinByUserRate() const {
        return mJoinByUserRate;
    }

    uint32_t getBrowserListLifespan() const {
        return mBrowserListLife;
    }
    uint32_t getBrowserListSize() const {
        return mBrowserListSize;
    }
    bool getBrowserSubscriptonLists() const { 
        return mBrowserSubscriptionLists; 
    }
    
    bool getAdditionalUserSetSubscriptionLists() const {
        return mAdditionalUserSetSubscriptionLists;
    }

    float getJoinByBrowserRate() const {
        return mJoinByBrowserRate;
    }
    float getBrowserListGameResetRate() const {
        return mBrowserGameResetRate;
    }
    bool getSendLegacyGameReport() const {
        return mSendLegacyGameReport;
    }
    const char* getBrowserListName() const {
        return mBrowserListName.c_str();
    }

    uint16_t getGamegroupJoinerFreq() const {
        return mGamegroupJoinerFreq;
    }

    const GameBrowserListConfigMap& getListConfigMap() const {
          return mGameBrowserListConfigs;
    }

    void setListConfigMap(GameBrowserListConfigMap& map) {
       map.copyInto(mGameBrowserListConfigs);
    }

    typedef eastl::pair<Collections::AttributeValue, uint32_t> AttributeValueWeight;
    typedef eastl::vector< AttributeValueWeight > AttrValueVector;    // value + weight.
    typedef eastl::hash_map<const char8_t*, AttrValueVector, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > AttrValues;
    const AttrValues& getGameAttrValues() const {
        return mGameAttrValues;
    }
    const AttrValues& getPlayerAttrValues() const {
        return mPlayerAttrValues;
    }
    bool getBatchAttributeSetPreGame() const {
        return mBatchAttributeSetPreGame;
    }
    bool getBatchAttributeSetPostGame() const {
        return mBatchAttributeSetPostGame;
    }
    uint32_t getRoleIdleLifespan() const {
        return mRoleIdleLifespan;
    }    
   
    const char8_t *getGameSizeRuleMinFitThreshold() const {
        return mGameSizeRuleMinFitThreshold;
    }

    const char8_t* pickGameAttrValue(const char8_t* attr) const;
    const char8_t* pickPlayerAttrValue(const char8_t* attr) const;

    //  metrics are shown when calling dumpStats
    //  diagnostics for utility not meant to measure load.
    enum Metric
    {       
        METRIC_REPORTS_SENT,
        METRIC_REPORTS_PROCESSED,
        METRIC_BROWSER_SELECTIONS,
        METRIC_JOIN_BY_USER,
        METRIC_BROWSER_LISTS,
        NUM_METRICS
    };

    void addMetric(Metric metric);
    void subMetric(Metric metric);

protected:
    GameManagerModule();

    bool parseConfig(const ConfigMap& config, const ConfigBootUtil* bootUtil);

private:
    const ConfigMap* mConfigFile;
    //GameBrowserServerConfig mGameBrowserServerConfig;
    GameBrowserListConfigMap mGameBrowserListConfigs;

    AttrValues mGameAttrValues;
    AttrValues mPlayerAttrValues;

    Action mAction;
    TimeValue mInitTimestamp;

    typedef eastl::hash_map<int32_t, GameManagerInstance*> InstanceMap;
    InstanceMap mInstances;

    GameManagerUtil *mGameUtil;
    GamegroupsUtil *mGamegroupsUtil;

    uint32_t mTotalConns;
    uint32_t mRemainingCreatorCount;
    uint32_t mRemainingVirtualCreatorCount;

    //  configurations
    float mHostMigrationRate;
    float mKickPlayerRate;
    float mSetAttributeRate;
    float mSetPlayerAttributeRate;
        
    bool mBatchAttributeSetPreGame;
    bool mBatchAttributeSetPostGame;
    bool mSendLegacyGameReport;

    uint16_t mGamegroupJoinerFreq;

    //  join options.
    float mJoinByUserRate;
    float mJoinByBrowserRate;
    float mBrowserGameResetRate;
    eastl::string mBrowserListName;
    
    //  tweaks to per-instance behavior for joiners
    uint32_t mRoleIdleLifespan;
    uint32_t mBrowserListLife;
    uint32_t mBrowserListSize;
    bool mBrowserSubscriptionLists;
    bool mAdditionalUserSetSubscriptionLists;

    char8_t mGameSizeRuleMinFitThreshold[128];

    //  used to balance what types of reports are sent via the stress config
    typedef eastl::vector_map<const char8_t* , uint32_t> LegacyGameReportTypeWeights;
    LegacyGameReportTypeWeights mLegacyGameReportTypeWeights;

    //  used to display metrics during stat dump
    uint64_t mMetricCount[NUM_METRICS];
    char8_t mLogBuffer[512];
};


class GameManagerInstance : public StressInstance, public AsyncHandler
{
    NON_COPYABLE(GameManagerInstance);
    friend class GameManagerModule;

    typedef eastl::vector_map<GameId, GameBrowserGameDataPtr> GameBrowserGameMap;

public:
    ~GameManagerInstance() override;

    // This is the entry point for a stress instance to start running
    void start() override;

    BlazeId getBlazeId() const;
    BlazeId getBlazeUserId() const { return getBlazeId(); }  // DEPRECATED

protected:
    //Override this method to do your task
    BlazeRpcError execute() override;
    const char8_t *getName() const override;

    int32_t getSeed() const {
        return mTrialIndex + StressInstance::getIdent();
    }

    void setRole(GameManagerUtil::Role role) {
        mGameInst.setRole(role);
    }

    GameManagerUtil::Role getRole() { return mGameInst.getRole(); }

protected:
    GameManagerInstance(GameManagerModule *owner, StressConnection* connection, Login* login, bool joinByUser);

    void onDisconnected() override;
    void onLogout(BlazeRpcError result) override;
    void onLogin(BlazeRpcError result) override;
    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override;

    void fillPlayerAttributeMap(Collections::AttributeMap &attrMap);
    void fillGameAttributeMap(Collections::AttributeMap &attrMap);
        
    bool checkGameBrowserGameCapacity(SlotType slotType, const GameBrowserGameData *gbgame) const;
    void setOpenMode(const GameData& gameData, bool open);

private:
    BlazeRpcError createGameBrowserList();
    BlazeRpcError destroyGameBrowserList();
    BlazeRpcError getGameDataFromId(GameId gameId);
    void freeGameMap();

    void createGameRequestOverrideCb(CreateGameRequest *req);
    
    void onGameStateChange(EA::TDF::Tdf *tdf);

private:
    GameManagerModule *mOwner;
    const char8_t *mName;
    GameManagerSlave *mProxy;
#ifdef TARGET_arson
    Arson::ArsonSlave *mArsonProxy;
#endif

#ifdef TARGET_util
    Util::UtilSlave *mUtilSlave;
#endif
    int32_t mTrialIndex;

    GameManagerUtilInstance mGameInst;
    GamegroupsUtilInstance mGamegroupInst;

    uint32_t mStateCounter;
    uint32_t mBrowserListLifeCounter;

    GameBrowserListId mBrowserListId;
    GameBrowserListId mUserSetBrowserListId;

    GameBrowserGameMap mGameMap;
};


} // Stress
} // Blaze

#endif // BLAZE_STRESS_GAMEMANAGERMODULE_H

