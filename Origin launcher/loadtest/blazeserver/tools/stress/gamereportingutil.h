/*************************************************************************************************/
/*!
    \file gamereportingutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_GAMEREPORTING_UTIL
#define BLAZE_STRESS_GAMEREPORTING_UTIL

#ifdef TARGET_gamereporting

#include "stressconnection.h"       // AsyncHandler
#include "stressinstance.h"
#include "loginmanager.h"

#include "framework/tdf/attributes.h"
#include "gamereporting/tdf/gamereporting.h"

#include "stats/statscommontypes.h"


namespace Blaze
{
class ConfigMap;
class ConfigBootUtil;

namespace GameReporting
{
    class GameTypeCollection;
    class GameReportingConfig;
    class GameReportingSlave;
}


namespace Stress
{

class StressModule;

class GameReportingUtilInstance;
class CustomGameReporter;

///////////////////////////////////////////////////////////////////////////////
//    class GameReportingUtil
//    
//    Constructs and manages a game report.  Called by stress modules that want to
//    support game reporting.
class GameReportingUtil
{
public:
    GameReportingUtil();
    virtual ~GameReportingUtil();

    //  parse game manager utility specific configuration for setup
    virtual bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil);

    //  manage a report session.
    bool getSubmitReports() const {
        return mSubmitReports;
    }

    const GameReporting::GameTypeCollection* getGameTypeCollection() {
        return mGameTypeCollection;
    }
    const GameReporting::GameReportingConfig* getComponentConfig() {
        return mComponentConfig;
    }

    uint32_t getReportUpdateCount(GameManager::GameReportingId gameReportingId) const {
        ReportUpdateCountById::const_iterator iter = mReportUpdateCount.find(gameReportingId);
        return (iter != mReportUpdateCount.end()) ? iter->second : 0;
    }

    const char8_t* pickRandomGameType(bool isOnline = true);
    GameManager::GameReportingId pickRandomSubmittedGameId(const char8_t* gameReportName);
    void addGameReportingId(BlazeId blazeId, GameManager::GameReportingId reportingId);

    typedef eastl::vector_map<BlazeId, const Collections::AttributeMap*> PlayerAttributeMap;
    //  reserves a custom report generator for the report keyed by gameReportingId.  only one generator is created per gameReportingId.
    void startReport(GameManager::GameReportingId gameReportingId, const Collections::AttributeMap *gameAttrs);
    //  allows custom code to modify report data during a game.
    void updateReport(GameManager::GameReportingId gameReportingId, const Collections::AttributeMap* gameAttrs, const PlayerAttributeMap* players);
    //  submits a report to gamereporting.
    BlazeRpcError submitReport(GameManager::GameReportingId gameReportingId, GameReporting::GameReportingSlave* proxy);
    //  finishes a reporting session - must call startReport again to invoke custom code.
    void endReport(GameManager::GameReportingId gameReportingId);

    //  submits an offline report to gamereporting - game type is determined by configuration settings, and this instantiates a custom report generator
    //  for this offline report.
    BlazeRpcError submitOfflineReport(BlazeId blazeId, GameReporting::GameReportingSlave* proxy);

private:
    GameReporting::GameTypeCollection *mGameTypeCollection;
    GameReporting::GameReportingConfig *mComponentConfig;
    bool mSubmitReports;

    typedef eastl::list<GameManager::GameReportingId> GameReportingIdList;
    typedef eastl::hash_map<eastl::string, GameReportingIdList, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> GameReportingIdListByType;
    GameReportingIdListByType mSubmittedIdListByType;
    typedef eastl::hash_map<BlazeId, eastl::string> GameReportNameByBlazeId;
    GameReportNameByBlazeId mGameReportNameByBlazeId;

    typedef eastl::hash_map<GameManager::GameReportingId, CustomGameReporter*> GameReportingSessions;
    GameReportingSessions mReportingSessions;

    typedef eastl::hash_map<eastl::string, uint32_t, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> GameTypeWeight;
    GameTypeWeight mOnlineGameTypeWeight;
    GameTypeWeight mOfflineGameTypeWeight;

public:
    //  metrics are shown when calling dumpStats
    //  diagnostics for utility not meant to measure load.
    enum Metric
    {        
        METRIC_REPORTS_SUBMITTED,
        METRIC_REPORTS_PROCESSED,
        NUM_METRICS
    };

    void addMetric(Metric metric);
    virtual void dumpStats(StressModule *module);

private:
    typedef eastl::map<GameManager::GameReportingId, uint32_t> ReportUpdateCountById;
    ReportUpdateCountById mReportUpdateCount;
    //  used to display metrics during stat dump
    uint64_t mMetricCount[NUM_METRICS];
    char8_t mLogBuffer[512];
    static const char8_t* METRIC_STRINGS[];
};


///////////////////////////////////////////////////////////////////////////////
//    class GameReportingUtilInstance
//    
//    usage: Declare one per StressInstance.  
//    Initialize using the setStressInstance method.
//    Within the StressInstance's execute override, invoke this class's execute 
//    method to update the instance.
//    
class GameReportingUtilInstance: public AsyncHandler
{
   
public:
    GameReportingUtilInstance(GameReportingUtil *util);
    ~GameReportingUtilInstance() override;

    //  used to call game reporting actions
    GameReporting::GameReportingSlave* getProxy() {
        return mGameReportingProxy;
    }

    //  The callback should know what notification TDF to use based on the notification type.  Type-cast accordingly     
    typedef Functor1<EA::TDF::Tdf*> AsyncNotifyCb;
    void setNotificationCb(const AsyncNotifyCb& cb);
    void clearNotificationCb();

public:
    //  attach this to a stress instance
    void setStressInstance(StressInstance* inst);
    void clearStressInstance() { 
        setStressInstance(NULL); 
    }
    StressInstance* getStressInstance() {
        return mStressInst;
    }
    int32_t getIdent() const {
        return (mStressInst != NULL) ? mStressInst->getIdent() : -1;
    }
    BlazeId getBlazeId() const {
        return (mStressInst != NULL) ? mStressInst->getLogin()->getUserLoginInfo()->getBlazeId() : 0;
    }

    //  if instance logs out, invoke this method to reset state (if applicable)
    void onLogout();

protected:
    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override;

private:
    GameReportingUtil *mUtil;
    StressInstance *mStressInst;
    GameReporting::GameReportingSlave *mGameReportingProxy;

    //  Channels report result notifications to the owning object
    AsyncNotifyCb mResultNotifyCb;    

    //  Caller must allocate TDF.
    void scheduleCustomNotification(EA::TDF::Tdf *notification);
    void runNotificationCb(EA::TDF::TdfPtr tdf);
};


///////////////////////////////////////////////////////////////////////////////
//  class CustomGameReporterRegistration
//
//  users of the CustomGameReporterRegistration class register their custom game reporting code by implementing a creator of this type.
//  Place code in gamereporting/stress/custom/<your title dir> .  For samples 
//  refer to the blaze dist's code.
//    
typedef CustomGameReporter* (*CustomGameReporterCreateFn)(GameReporting::GameReport& gameReport);
typedef bool (*CustomGameReporterPickFn)(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs);

//  see samples in gamereporting/stress/custom for usage.
class CustomGameReporterRegistration
{
public:
    CustomGameReporterRegistration(const char8_t* gameReportName, CustomGameReporterCreateFn creator, CustomGameReporterPickFn typepicker);
    ~CustomGameReporterRegistration();

    CustomGameReporterCreateFn getCreator() const {
        return mCreateFn;
    }
    CustomGameReporterPickFn getPicker() const {
        return mPickerFn;
    }
    const char8_t* getGameReportName() const {
        return mGameReportName.c_str();
    }

private:
    GameManager::GameReportName mGameReportName;
    CustomGameReporterCreateFn mCreateFn;
    CustomGameReporterPickFn mPickerFn;
};


///////////////////////////////////////////////////////////////////////////////
//    class CustomGameReporter
//    
//    usage: allows customization of game report generation.   Since each title
//      has its own report TDF, to accurately stress test a title's game reporting
//      implementation, developers can create their own report generator by
//      inheriting from this interface and using the CustomGameReporterRegistration 
//      class to instantiate the generator.
//
//      Place code in gamereporting/stress/custom/<your title dir> .  For samples 
//      refer to the blaze dist's code.
//    
class CustomGameReporter
{
    NON_COPYABLE(CustomGameReporter);

public:
    CustomGameReporter(GameReporting::GameReport& report) : mGameReport(&report), mRefCount(0) {}
    virtual ~CustomGameReporter() {}

    //  allows custom code to fill in the game report during a game session (i.e. start of game, replay, post-game)
    virtual void update(const Collections::AttributeMap* gameAttrs, const GameReportingUtil::PlayerAttributeMap* players) = 0;   

    //  returns the game report.
    GameReporting::GameReport& getGameReport() {
        return *mGameReport;
    }

    //  manages life of the reporter object by the GameReportingUtil
    uint32_t incRefCount() {
        return (++mRefCount);
    }
    uint32_t decRefCount() {
        return (--mRefCount);
    }
    uint32_t getRefCount() const {
        return mRefCount;
    }

protected:
    GameReporting::GameReportPtr mGameReport; // take ownership of the passed in game report
    uint32_t mRefCount;
};




} // Stress
} // Blaze

#endif // TARGET_gamereporting
#endif
