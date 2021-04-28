/*************************************************************************************************/
/*!
    \file gamereportingutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

#ifdef TARGET_gamereporting
#include "gamereportingutil.h"
#include "gamemanager/rpc/gamemanagerslave.h"

#include "blazerpcerrors.h"
#include "framework/util/shared/blazestring.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/util/random.h"
#include "framework/config/config_file.h"
#include "framework/config/config_boot_util.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"
#include "framework/config/configdecoder.h"
#include "framework/connection/selector.h"

//  including gamereporting component internal headers as needed
#include "gamereporting/gametype.h"
#include "gamereporting/tdf/gamereporting_server.h"
#include "gamereporting/rpc/gamereportingslave.h"

#include "stressmodule.h"

using namespace Blaze::GameReporting;

namespace Blaze
{
namespace Stress
{

///////////////////////////////////////////////////////////////////////////////////////////////

const char8_t* GameReportingUtil::METRIC_STRINGS[] =  {
    "reportsSubmitted",
    "reportsProcessed"
};


///////////////////////////////////////////////////////////////////////////////////////////////
class CustomGameReporterFactory
{
public:
    static CustomGameReporterFactory& get() {
        static CustomGameReporterFactory instance;
        return instance;
    }

    typedef eastl::hash_map<const char8_t*, CustomGameReporterRegistration* , eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > CustomGameReporterCreatorMap;    
    CustomGameReporterCreateFn getCreator(const char8_t *gameReportName) const {
        CustomGameReporterCreatorMap::const_iterator cit = mCreators.find(gameReportName);
        if (cit == mCreators.end())
            return nullptr;
        return cit->second->getCreator();
    }
    bool pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap *gameAttrs) {
        for (CustomGameReporterCreatorMap::const_iterator cit = mCreators.begin(), citEnd = mCreators.end(); 
                cit != citEnd; ++cit)
        {
            CustomGameReporterPickFn fn = cit->second->getPicker();
            if ((*fn)(gameReportName, gameAttrs))
                return true;               
        }

        return false;
    }
    void registerCreator(CustomGameReporterRegistration& registeree) {
        mCreators[registeree.getGameReportName()] = &registeree;
    }
    void deregisterCreator(CustomGameReporterRegistration& registeree) {
        mCreators.erase(registeree.getGameReportName());
    }

private:
    CustomGameReporterCreatorMap mCreators;  
};


CustomGameReporterRegistration::CustomGameReporterRegistration(const char8_t* gameReportName, CustomGameReporterCreateFn creator, CustomGameReporterPickFn picker)
{
    mGameReportName.set(gameReportName);
    mCreateFn = creator;
    mPickerFn = picker;

    CustomGameReporterFactory::get().registerCreator(*this);
}


CustomGameReporterRegistration::~CustomGameReporterRegistration()
{
    CustomGameReporterFactory::get().deregisterCreator(*this);
}


///////////////////////////////////////////////////////////////////////////////////////////////

GameReportingUtil::GameReportingUtil()
{
    mGameTypeCollection = nullptr;
    mComponentConfig = nullptr;
    mSubmitReports = false;

    for (int i = 0; i < NUM_METRICS; ++i)
    {
        mMetricCount[i] = 0;
    }
}


GameReportingUtil::~GameReportingUtil()
{
    //  clear out remaining game reporting sessions.
    for (GameReportingSessions::iterator itSession = mReportingSessions.begin(), itSessionEnd = mReportingSessions.end(); itSession != itSessionEnd; ++itSession)
    {        
        delete itSession->second;   // frees the custom game reporter.
    }
    mReportingSessions.clear();

    if (mGameTypeCollection != nullptr)
    {
        delete mGameTypeCollection;
        mGameTypeCollection = nullptr;
    }
}


//  Parse game manager utility specific configuration for setup
bool GameReportingUtil::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    mSubmitReports = config.getBool("grSubmitReports", false);
    if (!mSubmitReports)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].initialize: GAME REPORTING STRESS INACTIVE.");
        return true;
    }

    //  LOAD ENTIRE GAME REPORTING CONFIGURATION
    ConfigMap *grConfigFile = nullptr;
    if (bootUtil != nullptr)
    {
        grConfigFile = bootUtil->createFrom("gamereporting", ConfigBootUtil::CONFIG_IS_COMPONENT);
    }
    else        
    {
        grConfigFile = ConfigFile::createFromFile("component/gamereporting.cfg", true);
    }

    if (grConfigFile == nullptr)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].initialize: warning config file not found for gamereporting.");
        return true;
    }

    const ConfigMap *grConfigMap = grConfigFile->getMap("component.gamereporting");
    if (grConfigMap == nullptr)
    {
        grConfigMap = grConfigFile->getMap("gamereporting");
    }
    if (grConfigMap != nullptr)
    {
        ConfigDecoder configDecoder(*grConfigMap);
        mComponentConfig = BLAZE_NEW GameReporting::GameReportingConfig;
  
        if (!configDecoder.decode(mComponentConfig))
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].initialize: warning failed to decode game reporting config.");
            mComponentConfig = nullptr;
        }
    }
    else
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].initialize: warning config entry not found for gamereporting in config file.");
    }
    delete grConfigFile;

    if (!mComponentConfig)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].initialize: warning no configuration for game reporting.");
        return false;
    }
   
    //  LOAD GAME TYPES
    mGameTypeCollection = BLAZE_NEW GameReporting::GameTypeCollection();
    if (!mGameTypeCollection->init(mComponentConfig->getGameTypes()))
    {
        delete mGameTypeCollection;
        mGameTypeCollection = nullptr;
        BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].initialize: failed to load game types.");
        return false;
    }

    const ConfigSequence* onlineGameTypeSequence = config.getSequence("onlineReportGameTypes");
    if (onlineGameTypeSequence == nullptr)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].initialize: no online reportGameTypes configured");
    }
    else
    {
        while (onlineGameTypeSequence->hasNext())
        {
            const ConfigMap* gameTypeMap = onlineGameTypeSequence->nextMap();
            if (gameTypeMap != nullptr)
            {
                const char8_t* gameReportNameStr = gameTypeMap->getString("typeName", nullptr);
                uint32_t weight = gameTypeMap->getUInt32("weight", 0);

                if (gameReportNameStr != nullptr)
                {
                    const GameReporting::GameType* gameType = mGameTypeCollection->getGameType(GameManager::GameReportName(gameReportNameStr));
                    if (gameType != nullptr)
                    {
                        mOnlineGameTypeWeight.insert(eastl::make_pair(gameType->getGameReportName().c_str(), weight));
                    }
                    else
                    {
                        BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].initialize: unrecognized online game type: '" << gameReportNameStr << "'");
                        return false;
                    }
                }
            }
        }
    }

    const ConfigSequence* offlineGameTypeSequence = config.getSequence("offlineReportGameTypes");
    if (offlineGameTypeSequence == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].initialize: no offline reportGameTypes configured");
        return false;
    }
    else
    {
        while (offlineGameTypeSequence->hasNext())
        {
            const ConfigMap* gameTypeMap = offlineGameTypeSequence->nextMap();
            if (gameTypeMap != nullptr)
            {
                const char8_t* gameReportNameStr = gameTypeMap->getString("typeName", nullptr);
                uint32_t weight = gameTypeMap->getUInt32("weight", 0);

                if (gameReportNameStr != nullptr)
                {
                    const GameReporting::GameType* gameType = mGameTypeCollection->getGameType(GameManager::GameReportName(gameReportNameStr));
                    if (gameType != nullptr)
                    {
                        if (!gameType->getArbitraryUserOfflineReportProcessing())
                        {
                            BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].initialize: game type '" << gameReportNameStr <<
                                "' does not support updating user stats via offline reporting");
                        }
                        mOfflineGameTypeWeight.insert(eastl::make_pair(gameType->getGameReportName().c_str(), weight));
                    }
                    else
                    {
                        BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].initialize: unrecognized offline game type: '" << gameReportNameStr << "'");
                        return false;
                    }
                }
            }
        }
    }

    //  determine whether to activate the stress test for game reporting when utility is called by the stress module.
    BLAZE_INFO_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].initialize: GAME REPORTING STRESS ACTIVE.");

    return true;
}


const char8_t* GameReportingUtil::pickRandomGameType(bool isOnline)
{
    uint32_t rand = static_cast<uint32_t>(Blaze::Random::getRandomNumber(100)) + 1;

    uint32_t startProb = 1;

    GameTypeWeight& weights = (isOnline) ? mOnlineGameTypeWeight : mOfflineGameTypeWeight;

    for (GameTypeWeight::const_iterator it = weights.begin(); it != weights.end(); ++it)
    {
        if (rand >= startProb && rand < startProb + it->second)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil.pickRandomGameType]: randomly selected game type '" << it->first << "'");
            return it->first.c_str();
        }
        startProb = startProb + it->second;
    }

    return nullptr;
}


GameManager::GameReportingId GameReportingUtil::pickRandomSubmittedGameId(const char8_t* gameReportName)
{
    GameReportingIdListByType::const_iterator iter = mSubmittedIdListByType.find(gameReportName);
    if (iter != mSubmittedIdListByType.end())
    {
        GameReportingIdList::const_iterator idIter = iter->second.begin();
        eastl::advance(idIter, Blaze::Random::getRandomNumber(iter->second.size()));
        return *idIter;
    }
    return 0;
}

void GameReportingUtil::addGameReportingId(BlazeId blazeId, GameManager::GameReportingId reportingId)
{
    GameReportNameByBlazeId::iterator iter = mGameReportNameByBlazeId.find(blazeId);
    if (iter != mGameReportNameByBlazeId.end())
    {
        GameReportingIdListByType::insert_return_type inserter = mSubmittedIdListByType.insert(iter->second);
        inserter.first->second.push_back(reportingId);
        mGameReportNameByBlazeId.erase(blazeId);
    }
}


//  reserves a custom report generator for the report keyed by gameReportingId.  only one generator is created per gameReportingId.
void GameReportingUtil::startReport(GameManager::GameReportingId gameReportingId, const Collections::AttributeMap *gameAttrs)
{
    if (!mSubmitReports)
        return;

    GameManager::GameReportName gameReportName;

    //  randomly choose a game type based on probabilities
    const char8_t* randomGameTypeStr = pickRandomGameType();
    if (randomGameTypeStr != nullptr)
    {
        gameReportName.set(randomGameTypeStr);
    }
    else
    {
        //  choose a game type from custom code.
        if (!CustomGameReporterFactory::get().pickGameType(gameReportName, gameAttrs))
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].startReport: no game type selected for report " << gameReportingId << ".");
            return;
        }
    }

    //  validate gametype as we'll need it to generate the report.
    const GameReporting::GameType *gameType = mGameTypeCollection->getGameType(gameReportName.c_str());
    if (gameType == nullptr)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].startReport: invalid game type '" << gameReportName.c_str() << "' specified for report " << gameReportingId << ".");
        return;
    }

    GameReportingSessions::insert_return_type inserter = mReportingSessions.insert(gameReportingId);
    GameReportingSessions::iterator sessionIt = inserter.first;
    if (inserter.second)
    {   
        sessionIt->second = nullptr;
        EA::TDF::TdfPtr reportTdf = gameType->createReportTdf(gameType->getConfig().getReportTdf());
        if (!reportTdf)
        {
            //  failed to create report TDF - keep session around but any report operation will be a no-op, but endReport will remove this entry.
            return;
        }
          
        //  invoke custom code generator for game type.
        CustomGameReporterCreateFn creator = CustomGameReporterFactory::get().getCreator(gameReportName.c_str());
        if (creator == nullptr)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].startReport: custom game reporter failed to create report '" << gameReportName.c_str() << "' specified for report " << gameReportingId << ".");
            //  failed to custom game reportor - keep session around but any report operation will be a no-op, but endReport will remove this entry.
            return;
        }

        //  set this reporting session's reporter.
        GameReport *gameReport = BLAZE_NEW GameReport;
        gameReport->setGameReportingId(gameReportingId);
        gameReport->setGameReportName(gameReportName.c_str());
        gameReport->setReport(*reportTdf);

        sessionIt->second = (*creator)(*gameReport);
    }
    if (sessionIt->second != nullptr)
    {
        sessionIt->second->incRefCount();
        BLAZE_INFO_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].startReport: reportId=" 
            << gameReportingId << ", refCount=" << sessionIt->second->getRefCount());
    }
    // clear out the report count
    mReportUpdateCount[gameReportingId] = 0;
}


//  allows custom code to modify report data during a game.
void GameReportingUtil::updateReport(GameManager::GameReportingId gameReportingId, const Collections::AttributeMap* gameAttrs, const PlayerAttributeMap* players)
{
    GameReportingSessions::iterator it = mReportingSessions.find(gameReportingId);
    if (it == mReportingSessions.end() || it->second == nullptr)
        return;


    it->second->update(gameAttrs, players);
    mReportUpdateCount[gameReportingId]++;
}


//  submits a report to gamereporting.
BlazeRpcError GameReportingUtil::submitReport(GameManager::GameReportingId gameReportingId, GameReportingSlave* proxy)
{
    GameReportingSessions::iterator it = mReportingSessions.find(gameReportingId);
    if (it == mReportingSessions.end() || it->second == nullptr)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].submitReport: report " 
            << gameReportingId << " not found in mReportingSessions, could not submit!");
        return Blaze::ERR_SYSTEM;
    }


    BlazeRpcError err = ERR_SYSTEM;
    eastl::string gameReportName;
    {
        GameReport& gameReport = it->second->getGameReport();
        gameReportName = gameReport.getGameReportName(); // must cache string, since gameReport may go away after blocking call

        SubmitGameReportRequest req;
        gameReport.copyInto(req.getGameReport());
        //  TBD. how to handle private reports.
        err = proxy->submitGameReport(req);
    }
    if (err == ERR_OK)
    {
        addMetric(METRIC_REPORTS_SUBMITTED);
        GameReportingIdListByType::insert_return_type inserter = mSubmittedIdListByType.insert(gameReportName);
        inserter.first->second.push_back(gameReportingId);
        BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].submitReport: report " 
            << gameReportingId << " successfully submitted (" << (ErrorHelp::getErrorName(err)) << ").");
    }
    else
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].submitReport: report " 
            << gameReportingId << " failed to submit (" << (ErrorHelp::getErrorName(err)) << ").");
    }

    return err;
}


//  finishes a reporting session - must call startReport again to invoke custom code.
void GameReportingUtil::endReport(GameManager::GameReportingId gameReportingId)
{
    GameReportingSessions::iterator it = mReportingSessions.find(gameReportingId);
    if (it == mReportingSessions.end())
        return;

    if (it->second != nullptr)
    {
        if (it->second->getRefCount() == 0 || it->second->decRefCount() == 0)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].endReport: delete report with reportId=" 
                << gameReportingId << " and erase it from mReportingSessions");
            delete it->second;   // frees the custom game reporter.
            it->second = nullptr;
        }
        else
        {
            //  if there are still instances referencing the reporter, keep alive.
            return;
        }
    }

    mReportingSessions.erase(gameReportingId);
    mReportUpdateCount.erase(gameReportingId);
}


BlazeRpcError GameReportingUtil::submitOfflineReport(BlazeId blazeId, GameReporting::GameReportingSlave* proxy)
{
    //  pick a game type from the configuration.
    const char8_t *gameReportNameStr = pickRandomGameType(false);

    //  validate gametype as we'll need it to generate the report.
    const GameReporting::GameType *gameType = mGameTypeCollection->getGameType(gameReportNameStr);
    if (gameType == nullptr)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].submitOfflineReport: invalid game type '" << gameReportNameStr << "' specified for report.");
        return ERR_SYSTEM;
    }

    CustomGameReporterCreateFn creator = CustomGameReporterFactory::get().getCreator(gameReportNameStr);
    if (creator == nullptr)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].submitOfflineReport: for game type '" << gameReportNameStr << "' not registered report generator.");
        return ERR_SYSTEM;
    }

    EA::TDF::Tdf *reportTdf = gameType->createReportTdf(gameType->getConfig().getReportTdf());
    if (reportTdf == nullptr)
    {
        //  failed to create report TDF
        BLAZE_WARN_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].submitOfflineReport: for game type '" << gameReportNameStr << "' failed to create report tdf.");
        return ERR_SYSTEM;
    }

    BlazeRpcError submitReportErr = ERR_OK;
    GameReport* gameReport = BLAZE_NEW GameReport;
    gameReport->setGameReportName(gameReportNameStr);
    gameReport->setReport(*reportTdf);
    CustomGameReporter *reporter = (*creator)(*gameReport);
    if (reporter != nullptr)
    {
        PlayerAttributeMap player;
        player[blazeId] = nullptr;         // no attribute map, but used by the reporter if the submitter ID is needed.
        reporter->update(nullptr, &player);
        SubmitGameReportRequest req;
        gameReport->copyInto(req.getGameReport());
        submitReportErr = proxy->submitOfflineGameReport(req);
        delete reporter;
        reporter = nullptr;
    }

    if (submitReportErr == ERR_OK)
    {
        addMetric(METRIC_REPORTS_SUBMITTED);
        mGameReportNameByBlazeId[blazeId] = gameReportNameStr;
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtil].submitOfflineReport: for game type '" << gameReportNameStr << "' failed to submit report (err=" << (ErrorHelp::getErrorName(submitReportErr)) << ").");
    }

    return submitReportErr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
//    diagnostics for utility not meant to measure load.
void GameReportingUtil::addMetric(Metric metric)
{
    mMetricCount[metric]++;
}

void GameReportingUtil::dumpStats(StressModule *module)
{
    if (!getSubmitReports())
        return;

    char8_t statbuffer[128];
    blaze_snzprintf(statbuffer, sizeof(statbuffer), "time=%.2f", module->getTotalTestTime());

    char8_t *metricOutput = mLogBuffer;
    metricOutput[0] = '\0';
    char8_t *curStr = metricOutput;

    for (int i = 0; i < NUM_METRICS; ++i)
    {
        blaze_snzprintf(curStr, sizeof(mLogBuffer) - strlen(metricOutput), "%s=%" PRIu64 " ",METRIC_STRINGS[i], mMetricCount[i]);
        curStr += strlen(curStr);
    }

    BLAZE_INFO_LOG(Log::CONTROLLER, "[GameReportingUtil].dumpStats: " << statbuffer << " @ " << metricOutput);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//  GameReportingUtilInstance

GameReportingUtilInstance::GameReportingUtilInstance(GameReportingUtil *util)
{
    mUtil = util;
    mStressInst = nullptr;
    mGameReportingProxy = nullptr;
}

GameReportingUtilInstance::~GameReportingUtilInstance()
{
    setStressInstance(nullptr);
}


void GameReportingUtilInstance::setStressInstance(StressInstance* inst) 
{
    if (inst == nullptr && mStressInst != nullptr)
    {
        StressConnection *conn = mStressInst->getConnection();
        conn->removeAsyncHandler(GameReportingSlave::COMPONENT_ID, GameReportingSlave::NOTIFY_RESULTNOTIFICATION, this);

        if (mGameReportingProxy != nullptr)
        {
            delete mGameReportingProxy;
            mGameReportingProxy = nullptr;
        }

        mResultNotifyCb.clear();
        return;
    }

    if (inst != nullptr)
    {
        StressConnection *conn = inst->getConnection();

        mGameReportingProxy = BLAZE_NEW GameReportingSlave(*conn);

        //    this instance has joined a game.
        conn->addAsyncHandler(GameReportingSlave::COMPONENT_ID, GameReportingSlave::NOTIFY_RESULTNOTIFICATION, this); 
    }

    mStressInst = inst;
}


void GameReportingUtilInstance::onLogout()
{
}


void GameReportingUtilInstance::setNotificationCb(const AsyncNotifyCb& cb)
{
    if (mStressInst == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtilInstance].setNotificationCb: Unable to set notification callback on uninitialized instance.");
        return;
    }
    mResultNotifyCb = cb;
}


void GameReportingUtilInstance::clearNotificationCb()
{
    mResultNotifyCb = AsyncNotifyCb();
}


void GameReportingUtilInstance::scheduleCustomNotification(EA::TDF::Tdf *notification)
{
    if (mResultNotifyCb.isValid())
    {
        EA::TDF::TdfPtr cbTdf = notification->clone();
        gSelector->scheduleFiberCall(this,  &GameReportingUtilInstance::runNotificationCb, cbTdf, "GameReportingUtilInstance::runNotificationCb");
    }
}


void GameReportingUtilInstance::runNotificationCb(EA::TDF::TdfPtr tdf)
{
    StressLogContextOverride stressLogContextOverride(getIdent(), getBlazeId());

    mResultNotifyCb(tdf);    
}


void GameReportingUtilInstance::onAsync(uint16_t component, uint16_t iType, RawBuffer* payload)
{
    Heat2Decoder decoder;

    if (component != Blaze::GameManager::GameManagerSlave::COMPONENT_ID &&
        component != Blaze::GameReporting::GameReportingSlave::COMPONENT_ID)
    {
        return;
    }

    //  GameManager async updates
    GameReportingSlave::NotificationType type = (GameReportingSlave::NotificationType)iType;

    switch (type)
    {
    case GameReportingSlave::NOTIFY_RESULTNOTIFICATION:
        {
            GameReporting::ResultNotification notify;
            if (decoder.decode(*payload, notify) != ERR_OK)
            {
                BLAZE_ERR_LOG(BlazeRpcLog::gamereporting, "[GameReportingUtilInstance].onAsync: Failed to decode notification(" << type << ")");
            }
            else
            {
                mUtil->addGameReportingId(getBlazeId(), notify.getGameReportingId());
                scheduleCustomNotification(&notify);
            }
        }
        break;
    }
}

}    // Stress
}    // Blaze
#endif // TARGET_gamereporting

