/*************************************************************************************************/
/*!
    \file   gamereportingslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "gamereporting/gamereportingslaveimpl.h"
#include "gamereporting/skillconfig.h"

#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/storage/storagemanager.h"

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/util/localization.h"

#include "gamereporting/tdf/gamereportingevents_server.h"
#include "framework/event/eventmanager.h"

#include "gamereporting/tdf/gamereporting_server.h"
#include "component/gamereporting/gamehistbootstrap.h"
#include "gamemanager/rpc/gamemanagermaster.h"
#include "gamemanager/gamereportingidhelper.h"
#include "gamemanager/pinutil/pinutil.h"

#include "gamereporting/basicgamereportcollator.h"
#include "gamereporting/basicgamereportprocessor.h"
#include "gamereporting/basicstatsservicegamereportprocessor.h"
#include "gamereporting/util/gamereportingconfigutil.h"
#include "gamereporting/util/reportparserutil.h"

#include "stats/rpc/statsslave.h"
#ifdef TARGET_clubs
#include "clubs/rpc/clubsslave.h"
#endif

namespace Blaze
{

namespace Metrics
{
namespace Tag
{
TagInfo<GameReporting::ReportType>* report_type = BLAZE_NEW TagInfo<GameReporting::ReportType>("report_type", [](const GameReporting::ReportType& value, Metrics::TagValue& buffer) { return ReportTypeToString(value); });

TagInfo<const char8_t*>* game_report_name = BLAZE_NEW TagInfo<const char8_t*>("game_report_name");

TagInfo<int64_t>* game_report_attr_value = BLAZE_NEW TagInfo<int64_t>("game_report_attr_value", Blaze::Metrics::int64ToString);
} // namespace Tag
} // namespace Metrics

namespace GameReporting
{

const char8_t* GAMEHISTORY_PURGE_TASKNAME = "GameReportingSlaveImpl::doGameHistoryPurge";

const char8_t PUBLIC_GAME_REPORT_FIELD[] = "pub.gameReport";
const char8_t PRIVATE_GAME_REPORT_FIELD[] = "pri.gameReport";
const char8_t* VALID_GAME_REPORT_FIELD_PREFIXES[] = { PUBLIC_GAME_REPORT_FIELD, PRIVATE_GAME_REPORT_FIELD, nullptr };

EA_THREAD_LOCAL GameReportingSlaveImpl* gGameReportingSlave = nullptr;

void intrusive_ptr_add_ref(GameReportMaster* ptr)
{
    if (ptr->mRefCount == 1)
    {
        // pin the sliver from migrating because we 'may' be writing the game object back
        bool result = ptr->mOwnedSliverRef->getPrioritySliverAccess(ptr->mOwnedSliverAccessRef);
        EA_ASSERT_MSG(result, "Game Session failed to obtain owned sliver access!");
    }
    ++ptr->mRefCount;
}

void intrusive_ptr_release(GameReportMaster* ptr)
{
    if (ptr->mRefCount > 0)
    {
        --ptr->mRefCount;
        if (ptr->mRefCount == 0)
            delete ptr;
        else if (ptr->mRefCount == 1)
        {
            if (ptr->mAutoCommit)
            {
                const GameManager::GameReportingId id = ptr->getGameReportingId();
                const GameReportMaster* report = gGameReportingSlave->getReadOnlyGameReport(id);
                if (report == ptr)
                {
                    // This means we are returning the game report back to the container, trigger a writeback to storage
                    gGameReportingSlave->getReportCollateDataTable().upsertField(id, PRIVATE_GAME_REPORT_FIELD, *ptr->mReportCollateData);
                }
                ptr->mAutoCommit = false;
            }
            // unpin the sliver and enable it to migrate once more (once the pending upserts have been flushed)
            ptr->mOwnedSliverAccessRef.release();
        }
    }
}

GameReportMaster::GameReportMaster(GameManager::GameReportingId id, const OwnedSliverRef& ownedSliverRef) :
    mAutoCommit(false),
    mRefCount(0),
    mOwnedSliverRef(ownedSliverRef),
    mGameReportingId(id),
    mReportCollateData(BLAZE_NEW ReportCollateData),
    mProcessingStartTimerId(INVALID_TIMER_ID)
{
}

GameReportMaster::GameReportMaster(GameManager::GameReportingId id, const ReportCollateDataPtr& reportCollateData, const OwnedSliverRef& ownedSliverRef) :
    mAutoCommit(false),
    mRefCount(0),
    mOwnedSliverRef(ownedSliverRef),
    mGameReportingId(id),
    mReportCollateData(reportCollateData),
    mProcessingStartTimerId(INVALID_TIMER_ID)
{
}

void GameReportMaster::saveCollatedReport(const CollatedGameReport& collatedReport)
{
    mCollatedGameReport = BLAZE_NEW CollatedGameReport;
    collatedReport.copyInto(*mCollatedGameReport);
}

//
//  class GameReportingSlaveImpl
//  
GameReportingSlave* GameReportingSlave::createImpl()
{
    return BLAZE_NEW_NAMED("GameReportingSlaveImpl") GameReportingSlaveImpl();
}

void GameReportingSlaveImpl::registerGameReportCollator(const char8_t* className, GameReportCollatorCreator creator)
{
    GameReportCollatorRegistryMap::iterator it = mGameReportCollatorRegistryMap.find(className);
    if (it != mGameReportCollatorRegistryMap.end())
    {
        TRACE_LOG("[GameReportingSlaveImpl].registerGameReportCollator: creator for " << className << " already registered.");
    }
    mGameReportCollatorRegistryMap[className] = creator;
}

GameReportCollator* GameReportingSlaveImpl::createGameReportCollator(const char8_t* className, ReportCollateData& gameReport)
{
    GameReportCollatorRegistryMap::iterator it = mGameReportCollatorRegistryMap.find(className);
    if (it == mGameReportCollatorRegistryMap.end())
    {
        ERR_LOG("[GameReportingSlaveImpl].createGameReportCollator: no registered creator found for " << className << ".");
        return nullptr;
    }
    GameReportCollatorCreator creator = it->second;
    return creator(gameReport, *this);
}

void GameReportingSlaveImpl::registerGameReportProcessor(const char8_t* className, GameReportProcessorCreator creator)
{
    GameReportProcessorRegistryMap::iterator it = mGameReportProcessorRegistryMap.find(className);
    if (it != mGameReportProcessorRegistryMap.end())
    {
        TRACE_LOG("[GameReportingSlaveImpl].registerGameReportProcessor: creator for " << className << " already registered.");
    }
    mGameReportProcessorRegistryMap[className] = creator;
}

GameReportProcessor* GameReportingSlaveImpl::createGameReportProcessor(const char8_t* className)
{
    GameReportProcessorRegistryMap::iterator it = mGameReportProcessorRegistryMap.find(className);
    if (it == mGameReportProcessorRegistryMap.end())
    {
        ERR_LOG("[GameReportingSlaveImpl].createGameReportProcessor: no registered creator found for " << className << ".");
        return nullptr;
    }
    GameReportProcessorCreator creator = it->second;
    return creator(*this);
}

GameReportingSlaveMetrics::GameReportingSlaveMetrics(Metrics::MetricsCollection& collection, GameReportingSlaveImpl& component)
    : mGames(collection, "games")
    , mOfflineGames(collection, "offlineGames")
    , mReportQueueDelayMs(collection, "reportQueueDelayMs")
    , mReportProcessingDelayMs(collection, "reportProcessingDelayMs")
    , mReportsReceived(collection, "reportsReceived", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mReportsRejected(collection, "reportsRejected", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mDuplicateReports(collection, "duplicateReports", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mReportsForInvalidGames(collection, "reportsForInvalidGames", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mReportsFromPlayersNotInGame(collection, "reportsFromPlayersNotInGame", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mReportsAccepted(collection, "reportsAccepted", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mReportsExpected(collection, "reportsExpected", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mValidReports(collection, "validReports", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mInvalidReports(collection, "invalidReports", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mGamesCollatedSuccessfully(collection, "gamesCollatedSuccessfully", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mGamesWithDiscrepencies(collection, "gamesWithDiscrepencies", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mGamesWithNoValidReports(collection, "gamesWithNoValidReports", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mGamesProcessed(collection, "gamesProcessed", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mGamesProcessedSuccessfully(collection, "gamesProcessedSuccessfully", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mGamesWithProcessingErrors(collection, "gamesWithProcessingErrors", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mGamesWithStatUpdateFailure(collection, "gamesWithStatUpdateFailure", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mGaugeActiveProcessReportCount(collection, "gaugeActiveProcessReportCount")
    , mGamesWithStatsServiceUpdates(collection, "gamesWithStatsServiceUpdates", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mStatsServiceUpdateSuccesses(collection, "statsServiceUpdateSuccesses", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mStatsServiceUpdateQueueFails(collection, "statsServiceUpdateQueueFails", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mStatsServiceUpdateSetupFails(collection, "statsServiceUpdateSetupFails", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mStatsServiceUpdateErrorResponses(collection, "statsServiceUpdateErrorResponses", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mStatsServiceUpdateRetries(collection, "statsServiceUpdateRetries", Metrics::Tag::report_type, Metrics::Tag::game_report_name)
    , mActiveStatsServiceUpdates(collection, "activeStatsServiceUpdates")
    , mActiveStatsServiceRpcs(collection, "activeStatsServiceRpcs", [&component]() { return component.getStatsServiceRpcFiberJobQueue().getJobQueueSize() + component.getStatsServiceRpcFiberJobQueue().getWorkerFiberCount(); })
{
}

GameReportingSlaveImpl::GameReportingSlaveImpl() :
    mMetrics(getMetricsCollection(), *this),
    mProcessReportsFiberJobQueue("GameReportingSlaveImpl::processReport"),
    mStatsServiceRpcFiberJobQueue("GameReportingSlaveImpl::statsServiceRpc"),
    mStatsServiceNextPeriodicLogTime(0),
    mReportCollateDataTable(GameReportingSlave::COMPONENT_ID, GameReportingSlave::COMPONENT_MEMORY_GROUP, GameReportingSlave::LOGGING_CATEGORY),
    mSkillDampingTableCollection(nullptr),
    mPurgeTaskId(INVALID_TASK_ID),
    mHighFrequencyPurge(false),
    mDrainDelayPeriod(5 * 1000 * 1000),
    mGameReportInGameEndPinEvent(true)
{
    EA_ASSERT(gGameReportingSlave == nullptr);
    gGameReportingSlave = this;
}

GameReportingSlaveImpl::~GameReportingSlaveImpl()
{
    delete mSkillDampingTableCollection;

    gGameReportingSlave = nullptr;
}

void GameReportingSlaveImpl::onShutdown()
{
    mProcessReportsFiberJobQueue.join();
    mStatsServiceRpcFiberJobQueue.join();

    TaskSchedulerSlaveImpl *sched = static_cast<TaskSchedulerSlaveImpl*>(gController->getComponent(TaskSchedulerSlave::COMPONENT_ID, true, false));
    if (sched != nullptr)
    {
        sched->deregisterHandler(GameReportingSlave::COMPONENT_ID);
    }
    gController->deregisterDrainStatusCheckHandler(GameReporting::GameReportingSlave::COMPONENT_INFO.name);
}

bool GameReportingSlaveImpl::isDrainComplete()
{
    return mGameReportMasterPtrByIdMap.empty() && !mProcessReportsFiberJobQueue.hasPendingWork() && !mStatsServiceRpcFiberJobQueue.hasPendingWork() && (mMetrics.mGaugeActiveProcessReportCount.get() == 0);
}

bool GameReportingSlaveImpl::onValidateConfig(GameReportingConfig& config, const GameReportingConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    if (*(config.getDbName()) == '\0')
    {
        eastl::string msg;
        msg.sprintf("[GameReportingSlaveImpl].onValidateConfig(): No DB configuration detected.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    else
    {
        INFO_LOG("[GameReportingSlaveImpl].onValidateConfig: Game Reporting is set to use ("<< config.getDbName() <<") db.");
    }

    GameReportingConfigUtil::validateGameHistory(config.getGameHistory(), validationErrors);
    GameReportingConfigUtil::validateGameTypeConfig(config.getGameTypes(), validationErrors);
    GameReportingConfigUtil::validateGameHistoryReportingQueryConfig(config.getGameHistoryReporting().getQueries(), config.getGameTypes(), validationErrors);
    GameReportingConfigUtil::validateGameHistoryReportingViewConfig(config.getGameHistoryReporting().getViews(), config.getGameTypes(), validationErrors);
    GameReportingConfigUtil::validateSkillInfoConfig(config.getSkillInfo(), validationErrors);
    GameReportingConfigUtil::validateCustomEventConfig(config.getEventTypes(), validationErrors);

    if (config.getMaxConcurrentReportsProcessedPerSlave() == 0)
    {
        validationErrors.getErrorMessages().push_back("maxConcurrentReportsProcessedPerSlave must be greater than 0");
    }

    return validationErrors.getErrorMessages().empty();
}

bool GameReportingSlaveImpl::onResolve()
{
    gController->registerDrainStatusCheckHandler(GameReporting::GameReportingSlave::COMPONENT_INFO.name, *this);

    mReportCollateDataTable.registerFieldPrefixes(VALID_GAME_REPORT_FIELD_PREFIXES);
    mReportCollateDataTable.registerRecordHandlers(
        ImportStorageRecordCb(this, &GameReportingSlaveImpl::onImportStorageRecord),
        ExportStorageRecordCb(this, &GameReportingSlaveImpl::onExportStorageRecord));
    mReportCollateDataTable.registerRecordCommitHandler(CommitStorageRecordCb(this, &GameReportingSlaveImpl::onCommitStorageRecord));

    if (gStorageManager == nullptr)
    {
        FATAL_LOG("[GameReportingSlave] Storage manager is missing.  This shouldn't happen (was it in the components.cfg?).");
        return false;
    }
    gStorageManager->registerStorageTable(mReportCollateDataTable);

    return true;
}

bool GameReportingSlaveImpl::onConfigure()
{
#ifdef TARGET_clubs
    // Warn if a local clubs component does not exist on this gamereporting slave. Note that UpdateClubsUtil requires
    // a local clubs component and cannot be initialized without one. This ensures that the gamereporting slave
    // won't lose any in-progress clubs transactions when a remote instance is restarted.
    // The clubs component doesn't need to be active at this point (it's likely still in the CONFIGURING state).
    if (gController->getComponent(Clubs::ClubsSlave::COMPONENT_ID, true, false) == nullptr)
    {
        WARN_LOG("[GameReportingSlaveImpl].onConfigure: No local clubs component found for gamereporting. In-progress clubs transactions may be lost when a remote instance is restarted.");
    }
#endif

    if (!GameManager::GameReportingIdHelper::init())
    {
        ERR_LOG("[GameReportingSlaveImpl].onConfigure: failed initializing game reporting id helper");
        return false;
    }

    //  Parse and cache off GameType information
    const GameReportingConfig& gameReportingConfig = getConfig();

   GameReportingConfigUtil gameReportingConfigUtil(mGameTypeCollection, mGameHistoryConfig);
   if (!gameReportingConfigUtil.init(gameReportingConfig))
       return false;

    mProcessReportsFiberJobQueue.setMaximumWorkerFibers(gameReportingConfig.getMaxConcurrentReportsProcessedPerSlave());
    mProcessReportsFiberJobQueue.setDefaultJobTimeout(gameReportingConfig.getProcessingTimeout());

    // note that each concurrent worker will use its own channel--which we want to keep to a minimum
    mStatsServiceRpcFiberJobQueue.setMaximumWorkerFibers(gameReportingConfig.getMaxConcurrentStatsServiceRpcsPerSlave());

    //  initialize custom metrics for games.
    mGameReportingMetricsCollection.init(getMetricsCollection(), gameReportingConfig.getMetricsInfo());

    //  skill config
    const SkillInfoConfig& skillInfoConfig = gameReportingConfig.getSkillInfo();
    mSkillDampingTableCollection = BLAZE_NEW SkillDampingTableCollection();

    if (!mSkillDampingTableCollection->init(skillInfoConfig))
    {
        ERR_LOG("[GameReportingSlaveImpl].onConfigure: skill damping table configuration invalid");
        return false; 
    }

    //register internal GameReportCollator
    registerGameReportCollator("basic", &BasicGameReportCollator::create);
    //register custom GameReportCollator
    registerCustomGameReportCollators();

    //register internal GameReportProcessor
    registerGameReportProcessor("basic",&BasicGameReportProcessor::create);
    registerGameReportProcessor("basicStatsService", &BasicStatsServiceGameReportProcessor::create);
    //register custom GameReportProcessor
    registerCustomGameReportProcessors();

    BlazeRpcError rc = ERR_SYSTEM;
    TaskSchedulerSlaveImpl *sched = static_cast<TaskSchedulerSlaveImpl*>(gController->getComponent(TaskSchedulerSlave::COMPONENT_ID, true, true, &rc));
    if (sched == nullptr)
    {
        ERR_LOG("[GameReportingSlaveImpl].onConfigure: no task scheduler for bootstrap => error: " << ErrorHelp::getErrorName(rc));
        return false;
    }
    rc = sched->runOneTimeTaskAndBlock("GameReportingSlaveImpl::GameReportingBootStrap", GameReportingSlave::COMPONENT_ID,
        TaskSchedulerSlaveImpl::RunOneTimeTaskCb(this, &GameReportingSlaveImpl::runBootstrap));
    if (rc != ERR_OK)
    {
        ERR_LOG("[GameReportingSlaveImpl].onConfigure: bootstrap error: " << ErrorHelp::getErrorName(rc));
        return false;
    }
    sched->registerHandler(GameReportingSlave::COMPONENT_ID, this);

    mDrainDelayPeriod = gameReportingConfig.getDrainDelayPeriod();
    mGameReportInGameEndPinEvent = gameReportingConfig.getGameReportInGameEndPinEvent();

    return true;
}

bool GameReportingSlaveImpl::onReconfigure()
{
    const GameReportingConfig& config = getConfig();

    //  after reconfiguration, the config references need to be refreshed.
    mGameTypeCollection.updateConfigReferences(config.getGameTypes());
    mGameHistoryConfig.updateConfigReferences(config);
    mSkillDampingTableCollection->init(config.getSkillInfo());

    mProcessReportsFiberJobQueue.setMaximumWorkerFibers(config.getMaxConcurrentReportsProcessedPerSlave());
    mProcessReportsFiberJobQueue.setDefaultJobTimeout(config.getProcessingTimeout());

    mStatsServiceRpcFiberJobQueue.setMaximumWorkerFibers(config.getMaxConcurrentStatsServiceRpcsPerSlave());

    mDrainDelayPeriod = config.getDrainDelayPeriod();
    mGameReportInGameEndPinEvent = config.getGameReportInGameEndPinEvent();

    return true;
}

GameReportMaster* GameReportingSlaveImpl::getGameReportMaster(GameManager::GameReportingId id) const
{
    GameReportMaster* report = nullptr;
    GameReportMasterPtrByIdMap::const_iterator it = mGameReportMasterPtrByIdMap.find(id);
    if (it != mGameReportMasterPtrByIdMap.end())
        report = it->second.get();
    return report;
}

const GameReportMaster* GameReportingSlaveImpl::getReadOnlyGameReport(GameManager::GameReportingId id) const
{
    return getGameReportMaster(id);
}

GameReportMasterPtr GameReportingSlaveImpl::getWritableGameReport(GameManager::GameReportingId id, bool autoCommit)
{
    GameReportMaster* report = getGameReportMaster(id);
    if (autoCommit && (report != nullptr))
        report->setAutoCommitOnRelease();
    return report;
}

void GameReportingSlaveImpl::eraseGameReport(GameManager::GameReportingId id)
{
    GameReportMasterPtrByIdMap::const_iterator it = mGameReportMasterPtrByIdMap.find(id);
    if (it != mGameReportMasterPtrByIdMap.end())
    {
        GameReportMasterPtr report = it->second;
        mGameReportMasterPtrByIdMap.erase(id);
        mReportCollateDataTable.eraseRecord(id);
    }
}

eastl::string GameReportingSlaveImpl::getLocalizedString(const char8_t* str, uint32_t locale) const
{
    ClientPlatformType platform = gCurrentUserSession ? gCurrentUserSession->getClientPlatform() : gController->getDefaultClientPlatform();
    if (locale == 0)
    {
        locale = gCurrentUserSession ? gCurrentUserSession->getSessionLocale() : LOCALE_DEFAULT;
    }
    
    auto platformIter = getConfig().getGameHistoryReporting().getPlatformDescriptionMap().find(platform);
    if (platformIter == getConfig().getGameHistoryReporting().getPlatformDescriptionMap().end())
    {
        platformIter = getConfig().getGameHistoryReporting().getPlatformDescriptionMap().find(common); // Use fallback
    }

    if (platformIter == getConfig().getGameHistoryReporting().getPlatformDescriptionMap().end())
    {
        // no platform override map found. Return original.
        return eastl::string(gLocalization->localize(str, locale));
    }

    const auto overrideMap = platformIter->second;
    auto stringIter = overrideMap->find(str);
    if (stringIter != overrideMap->end())
    {
        return eastl::string(gLocalization->localize(stringIter->second.c_str(), locale));
    }
   
    // string not present in the override map, Return original.
    return eastl::string(gLocalization->localize(str, locale));
}


void GameReportingSlaveImpl::onImportStorageRecord(OwnedSliver& ownedSliver, const StorageRecordSnapshot& snapshot)
{
    StorageRecordSnapshot::FieldEntryMap::const_iterator dataField;

    dataField = getStorageFieldByPrefix(snapshot.fieldMap, PRIVATE_GAME_REPORT_FIELD);
    if (dataField == snapshot.fieldMap.end())
    {
        ERR_LOG("GameReportingSlaveImpl.onImportStorageRecord: Field prefix=" << PRIVATE_GAME_REPORT_FIELD << " matches no fields in gameReportTable record(" << snapshot.recordId << ").");
        mReportCollateDataTable.eraseRecord(snapshot.recordId);
        return;
    }

    ReportCollateDataPtr gameReportCollateData = static_cast<ReportCollateData*>(dataField->second.get());

    GameReportMasterPtrByIdMap::insert_return_type inserted = mGameReportMasterPtrByIdMap.insert(snapshot.recordId);
    if (!inserted.second)
    {
        ASSERT_LOG("GameReportingSlaveImpl.onImportStorageRecord: A ReportCollateData with GameReportingId(" << snapshot.recordId << ") already exists.");
        return;
    }

    GameReportMaster* gameReportMaster = BLAZE_NEW GameReportMaster(snapshot.recordId, gameReportCollateData, &ownedSliver);
    inserted.first->second = gameReportMaster;

    if (gameReportCollateData->getProcessingStartTime() != 0)
    {
        // Schedule the collation timeout handler
        TimerId processingStartTimerId = gSelector->scheduleFiberTimerCall(gameReportCollateData->getProcessingStartTime(), this, &GameReportingSlaveImpl::collateTimeoutHandler,
            snapshot.recordId, "GameReportingSlaveImpl::collateTimeoutHandler");
        gameReportMaster->setProcessingStartTimerId(processingStartTimerId);
    }
}

void GameReportingSlaveImpl::onExportStorageRecord(StorageRecordId recordId)
{
    GameReportMasterPtrByIdMap::iterator it = mGameReportMasterPtrByIdMap.find(recordId);
    if (it == mGameReportMasterPtrByIdMap.end())
    {
        ASSERT_LOG("GameReportingSlaveImpl.onExportStorageRecord: A game report with id(" << recordId << ") does not exist.");
        return;
    }

    // NOTE: Intentionally *not* using GameReportMaster here because it would attempt to aquire sliver access while sliver is locked for migration, which we want to avoid.
    // It is always safe to delete the GameReportMaster object after export, because export is only triggered when there are no more outstanding sliver accesses on this sliver.
    GameReportMaster* gameReportMaster = it->second.detach();
    mGameReportMasterPtrByIdMap.erase(it);

    // Cancel the collation timeout handler
    gSelector->cancelTimer(gameReportMaster->getProcessingStartTimerId());

    delete gameReportMaster;
}

void GameReportingSlaveImpl::onCommitStorageRecord(StorageRecordId recordId)
{
}

void GameReportingSlaveImpl::onScheduledTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName)
{
    if (blaze_strcmp(taskName, GAMEHISTORY_PURGE_TASKNAME) == 0)
        mPurgeTaskId = taskId;
}

void GameReportingSlaveImpl::onExecutedTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName)
{
    doGameHistoryPurge();
}

void GameReportingSlaveImpl::onCanceledTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName)
{
    mPurgeTaskId = INVALID_TASK_ID;
}

DbConnPtr GameReportingSlaveImpl::getDbConnPtr(const GameManager::GameReportName& gameType) const
{
    return gDbScheduler->getConnPtr(getDbIdForGameType(gameType), BlazeRpcLog::gamereporting);
}

DbConnPtr GameReportingSlaveImpl::getDbReadConnPtr(const GameManager::GameReportName& gameType) const
{
    return gDbScheduler->getReadConnPtr(getDbIdForGameType(gameType), BlazeRpcLog::gamereporting);
}

const char8_t* GameReportingSlaveImpl::getDbConnNameForGameType(const GameManager::GameReportName& gameType) const
{
    GameTypeConfigMap::const_iterator iter = getConfig().getGameTypes().find(gameType.c_str());
    if (iter != getConfig().getGameTypes().end())
    {
        const char8_t* dbName = iter->second->getDbName();
        if (dbName[0] != '\0')
            return dbName;
    }

    return getConfig().getDbName();
}

const char8_t* GameReportingSlaveImpl::getDbSchemaNameForGameType(const GameManager::GameReportName& gameType) const
{
    return gDbScheduler->getConfig(getDbIdForGameType(gameType))->getDatabase();
}

uint32_t GameReportingSlaveImpl::getDbIdForGameType(const GameManager::GameReportName& gameType) const
{
    return gDbScheduler->getDbId(getDbConnNameForGameType(gameType));
}

const GameReportQueryConfig *GameReportingSlaveImpl::getGameReportQueryConfig(const char8_t *name) const
{
    return mGameHistoryConfig.getGameReportQueriesCollection().getGameReportQueryConfig(name);
}

const GameReportViewConfig *GameReportingSlaveImpl::getGameReportViewConfig(const char8_t *name) const
{
    return mGameHistoryConfig.getGameReportViewsCollection().getGameReportViewConfig(name);
}

UpdateMetricError::Error GameReportingSlaveImpl::processUpdateMetric(const Blaze::GameReporting::UpdateMetricRequest &request, const Message* )
{
    //Check if we trust this user
    if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_UPDATE_METRIC))
    {
        WARN_LOG("[GameReportingSlaveImpl].processUpdateMetric: User [" << gCurrentUserSession->getBlazeId() << "]  attempted to submit updated a metric ["<< request.getMetricName() << "], no permission!");
        return UpdateMetricError::ERR_AUTHORIZATION_REQUIRED;
    }
    
    mGameReportingMetricsCollection.updateMetrics(request.getMetricName(), request.getValue());
    return UpdateMetricError::ERR_OK;
}

// get next game reporting id
BlazeRpcError GameReportingSlaveImpl::getNextGameReportingId(GameManager::GameReportingId& id)
{
    id = GameManager::INVALID_GAME_REPORTING_ID;
    BlazeRpcError rc = GameManager::GameReportingIdHelper::getNextId(id);
    if (rc != ERR_OK)
    {
        WARN_LOG("[GameReportingSlaveImpl].getNextGameReportingId: failed to get the next game reporting id. Err: " << ErrorHelp::getErrorName(rc));
    }
    return rc;
}

// util method for collecting a report for a finished game
BlazeRpcError GameReportingSlaveImpl::collectGameReport(BlazeId reportOwnerId, ReportType reportType, const SubmitGameReportRequest& submitRequest)
{
    AutoIncDec autoGauge(mMetrics.mGaugeActiveProcessReportCount);
    mMetrics.mReportsReceived.increment(1, reportType, submitRequest.getGameReport().getGameReportName());

    GameManager::GameReportingId gameReportingId = submitRequest.getGameReport().getGameReportingId();
    const EA::TDF::Tdf* reportTdf = submitRequest.getGameReport().getReport();
    if (reportTdf == nullptr)
    {
        ERR_LOG("[GameReportingSlaveImpl].collectGameReport: Received a nullptr game report from slave.  Should have been filtered by the slave before sending up."
            " Incoming report for user id=" << reportOwnerId << ", report (id:" << gameReportingId << ".)");
        return ERR_SYSTEM;
    }
    const char8_t* reportClassname = submitRequest.getGameReport().getReport()->getClassName();
    const char8_t* reportGameType = submitRequest.getGameReport().getGameReportName();
    const GameType* gameType = mGameTypeCollection.getGameType(reportGameType);
    if (gameType == nullptr)
    {
        ERR_LOG("[GameReportingSlaveImpl].collectGameReport: game report has an unknown game type with game reporting id = ((id:" 
                << gameReportingId << ",type:" << reportGameType << ")");
        return ERR_SYSTEM;
    }

    if (!getConfig().getEnableTrustedReportCollation() && (reportType == REPORT_TYPE_TRUSTED_END_GAME || reportType == REPORT_TYPE_TRUSTED_MID_GAME))
    {
        ERR_LOG("[GameReportingSlaveImpl:].collectGameReport: Trusted report collation is turned off.  Rejecting incoming report for blazeId (" << reportOwnerId << ").");
        return GAMEREPORTING_ERR_UNEXPECTED_REPORT;
    }

    TRACE_LOG("[GameReportingSlaveImpl:].collectGameReport: looking for report id " << gameReportingId << " in the local map.");
    GameReportMasterPtr gameReportMaster = getWritableGameReport(gameReportingId);
    if (gameReportMaster == nullptr)
    {
        WARN_LOG("[GameReportingSlaveImpl].collectGameReport: Incoming report for user id=" << reportOwnerId
            << ", report (id:" << gameReportingId << ", class:" << reportClassname << ", gameType: " << reportGameType
            << ", reportType: " << reportType << ") not matched to a pending game report.");

        mMetrics.mReportsForInvalidGames.increment(1, reportType, submitRequest.getGameReport().getGameReportName());

        return GAMEREPORTING_ERR_UNEXPECTED_REPORT;
    }

    // Changes made to gameReport will be committed when gameReportMaster goes out of scope.
    ReportCollateData* gameReport = gameReportMaster->getData();

    //  collate incoming report with the current game report.
    GameReportCollator::ReportResult reportResult = GameReportCollator::RESULT_COLLATE_CONTINUE;

    // create a game report collator
    //  it's possible that a report has been submitted of a different game type for the current game;
    //  if that is the case then simply keep going using the new one.
    bool gameTypeChanged = false;
    if (blaze_strcmp(gameReport->getStartedGameInfo().getGameReportName(), reportGameType) != 0)
    {
        TRACE_LOG("[GameReportingSlaveImpl].collectGameReport: report id " << gameReportingId
            << " ... type " << gameReport->getStartedGameInfo().getGameReportName() << " changed to " << reportGameType);
        gameTypeChanged = true;
        gameReport->getStartedGameInfo().setGameReportName(reportGameType);
    }
    GameReportCollatorPtr collator = GameReportCollatorPtr(createGameReportCollator(gameType->getReportCollatorClass(), *gameReport));
    if (collator == nullptr)
    {
        // the class wasn't registered. Error was logged (by createGameReportCollator).
        return ERR_SYSTEM;
    }

    if (gameTypeChanged)
    {
        collator->onGameTypeChanged();

        //  pass in finished info if it's available
        if (gameReport->getIsFinished())
        {
            reportResult = collator->gameFinished(gameReport->getGameInfo());
            TRACE_LOG("[GameReportingSlaveImpl].collectGameReport: report id " << gameReportingId
                << " game type changed ... gameFinished result: " << GameReportCollator::getReportResultName(reportResult));
        }
    }

    //  TODO - for now enforce report type for collated game report to equal the submitted report type.
    //  Ideally this should be controlled by the GameReportCollator pipeline implemented class
    //  This value is stored in the finalizeCollatedReport inside GameManagerSlaveImpl::processReport()
    gameReport->setReportType(reportType);

    //  If a trusted game report sent and trusted game reporting is turned off, only the dedicated server host can send the report
    if (reportType == REPORT_TYPE_TRUSTED_MID_GAME || reportType == REPORT_TYPE_TRUSTED_END_GAME)
    {
        if (!gameReport->getStartedGameInfo().getTrustedGameReporting() && gameReport->getStartedGameInfo().getTopology() == CLIENT_SERVER_DEDICATED)
        {
            if (gameReport->getStartedGameInfo().getTopologyHostId() != reportOwnerId)
            {
                reportResult = GameReportCollator::RESULT_COLLATE_REJECTED;
            }
        }
    }
    else if (reportType == REPORT_TYPE_STANDARD)
    {
        if (gameReport->getStartedGameInfo().getTrustedGameReporting())
        {
            WARN_LOG("[GameReportingSlaveImpl].collectGameReport: Incoming report for trusted game by user id=" << reportOwnerId 
                << ", report (id:" << gameReportingId << ", type:" << reportClassname << ") not a trusted report.");
            return GAMEREPORTING_ERR_UNEXPECTED_REPORT;
        }
    }

    if (reportType == REPORT_TYPE_TRUSTED_END_GAME && !gameReport->getIsFinished())
    {
        WARN_LOG("[GameReportingSlaveImpl].collectGameReport: Trusted game by user id=" << reportOwnerId << ", report (id:"
            << gameReportingId << ",type:" << reportClassname << ") submitted before the game finishes.");
    }

    //  submit the report.
    if (reportResult == GameReportCollator::RESULT_COLLATE_CONTINUE)
    {
        reportResult = collator->reportSubmitted(reportOwnerId, submitRequest.getGameReport(), submitRequest.getPrivateReport(), submitRequest.getFinishedStatus(), reportType);

        if (reportResult == GameReportCollator::RESULT_COLLATE_CONTINUE || reportResult == GameReportCollator::RESULT_COLLATE_COMPLETE)
        {
            gameReport->getSubmitterPlayerIds().push_back(reportOwnerId);
            //  the submitter (either the player (report owner) who submitted a report or the proxy user for the original submitter) 
            //  will receive notifications.   this is desired behavior since it's likely the original submitter may not be online, 
            //  but a user who acted as the proxy will receive notifications instead.
            gameReport->getSubmitterSessionIds().push_back(gCurrentUserSession->getSessionId());
        }
    }

    //  check the result of submitting the report.
    if (reportResult == GameReportCollator::RESULT_COLLATE_COMPLETE)
    {
        TRACE_LOG("[GameReportingSlaveImpl].collectGameReport: collation completed for report id=" << gameReportingId << "... processing report.");
        completeCollation(*collator, gameReportMaster);
    }
    else if (reportResult == GameReportCollator::RESULT_COLLATE_FAILED)
    {
        TRACE_LOG("[GameReportingSlaveImpl].collectGameReport: collation failed, dropping report id=" << gameReportingId);
        submitGameReportCollationFailedEvent(collator->getCollectedGameReportsMap(), gameReportingId);
        eraseGameReport(gameReportingId);
    }
    else if (reportResult != GameReportCollator::RESULT_COLLATE_CONTINUE)
    {
        INFO_LOG("[GameReportingSlaveImpl].collectGameReport: collation failed, rejecting submitted report from user id=" 
                 << reportOwnerId << ", gameReportingId=" << gameReportingId << ", reason=" << GameReportCollator::getReportResultName(reportResult));

        GameReportRejected reportRejected;
        reportRejected.setPlayerId(reportOwnerId);
        submitRequest.getGameReport().copyInto(reportRejected.getGameReport());
        gEventManager->submitEvent((uint32_t)GameReportingSlave::EVENT_GAMEREPORTREJECTEDEVENT, reportRejected);

        if (reportResult == GameReportCollator::RESULT_COLLATE_REJECT_DUPLICATE)
            mMetrics.mDuplicateReports.increment(1, reportType, submitRequest.getGameReport().getGameReportName());
        else if (reportResult == GameReportCollator::RESULT_COLLATE_REJECT_INVALID_PLAYER)
            mMetrics.mReportsFromPlayersNotInGame.increment(1, reportType, submitRequest.getGameReport().getGameReportName());
        else
            mMetrics.mReportsRejected.increment(1, reportType, submitRequest.getGameReport().getGameReportName());

        return GAMEREPORTING_ERR_UNEXPECTED_REPORT;
    }

    mMetrics.mReportsAccepted.increment(1, reportType, submitRequest.getGameReport().getGameReportName());

    return ERR_OK;
}

void GameReportingSlaveImpl::collateTimeoutHandler(GameManager::GameReportingId gameReportingId)
{
    AutoIncDec autoGauge(mMetrics.mGaugeActiveProcessReportCount);
    TRACE_LOG("[GameReportingSlaveImpl:].collateTimeoutHandler: getting writable game report with id " << gameReportingId);

    // This will pin the sliver associated with the game report (logic found in 'void intrusive_ptr_add_ref(GameReportMaster* ptr)')
    GameReportMasterPtr gameReportMaster = getWritableGameReport(gameReportingId);
    if (gameReportMaster == nullptr)
    {
        ERR_LOG("[GameReportingSlaveImpl].collateTimeoutHandler: report id " << gameReportingId << " already removed from the queue -- this should not have happened");
        return;
    }

    // Changes made to gameReport will be committed when gameReportMaster goes out of scope.
    ReportCollateData* gameReport = gameReportMaster->getData();

    mMetrics.mReportQueueDelayMs.increment(TimeValue::getTimeOfDay().getMillis() - gameReport->getFinishedGameTimeMs());

    TRACE_LOG("[GameReportingSlaveImpl].collateTimeoutHandler: timeout waiting for a complete game report for report id " << gameReportingId);

    GameReportCollatorPtr collator;
    const GameType* gameType = getGameTypeCollection().getGameType(gameReport->getStartedGameInfo().getGameReportName());
    if (gameType != nullptr)
    {
        // if game type name provided is valid, create the report collator
        collator = GameReportCollatorPtr(createGameReportCollator(gameType->getReportCollatorClass(), *gameReport));
    }

    GameReportCollator::ReportResult reportResult = GameReportCollator::RESULT_COLLATE_NO_REPORTS;

    if (collator != nullptr)
    {
        reportResult = collator->timeout();
        if (reportResult == GameReportCollator::RESULT_COLLATE_CONTINUE || reportResult == GameReportCollator::RESULT_COLLATE_COMPLETE)
        {
            // process the report as is
            completeCollation(*collator, gameReportMaster);
            return;
        }
        else if (reportResult != GameReportCollator::RESULT_COLLATE_NO_REPORTS)
        {
            submitGameReportCollationFailedEvent(collator->getCollectedGameReportsMap(), gameReportingId);
        }
    }

    if (reportResult == GameReportCollator::RESULT_COLLATE_NO_REPORTS)
    {
        submitGameReportOrphanedEvent(gameReport->getGameInfo(), gameReportingId);
        WARN_LOG("[GameReportingSlaveImpl].collateTimeoutHandler: no report generated for report id " << gameReportingId);
    }

    TRACE_LOG("[GameReportingSlaveImpl].collateTimeoutHandler: report id " << gameReportingId << " timed out and will be dropped");

    //  drop the game report.
    eraseGameReport(gameReportingId);
}

// Validate a few things before queuing the report for final processing.
void GameReportingSlaveImpl::completeCollation(GameReportCollator &collator, GameReportMasterPtr gameReportMaster)
{
    ReportCollateData& gameReport = *gameReportMaster->getData();

    if (gameReport.getIsProcessing())
    {
        TRACE_LOG("[GameReportingSlaveImpl].completeCollation: already processing report id: " << collator.getGameReportingId());
        return;
    }
    gameReport.setIsProcessing(true);

    if (gameReportMaster->getProcessingStartTimerId() != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(gameReportMaster->getProcessingStartTimerId());
        gameReportMaster->setProcessingStartTimerId(INVALID_TIMER_ID);
    }

    CollatedGameReport &collatedReport = collator.finalizeCollatedGameReport(gameReport.getReportType());

    if ((BlazeRpcError) collatedReport.getError() == GAMEREPORTING_COLLATION_REPORTS_INCONSISTENT)
    {
        mMetrics.mGamesWithDiscrepencies.increment(1, gameReport.getReportType(), collator.getGameReportName());
        GameReportDiscrepancy event;
        event.setGameReports(collator.getCollectedGameReportsMap());
        gEventManager->submitEvent(GameReportingSlave::EVENT_GAMEREPORTDISCREPANCYEVENT, event);
    }
    else if ((BlazeRpcError) collatedReport.getError() == GAMEREPORTING_COLLATION_ERR_NO_REPORTS)
    {
        mMetrics.mGamesWithNoValidReports.increment(1, gameReport.getReportType(), collator.getGameReportName());
        submitGameReportOrphanedEvent(gameReport.getGameInfo(), collator.getGameReportingId());
    }
    else if ((BlazeRpcError) collatedReport.getError() == ERR_OK)
    {
        mMetrics.mGamesCollatedSuccessfully.increment(1, gameReport.getReportType(), collator.getGameReportName());
    }
    collatedReport.setSendTimeMs(TimeValue::getTimeOfDay().getMillis());

    //  enforce collated report values for the ID and game type name
    collatedReport.getGameReport().setGameReportingId(collator.getGameReportingId());
    collatedReport.getGameReport().setGameReportName(collator.getGameReportName());

    gameReport.getSubmitterPlayerIds().copyInto(collatedReport.getSubmitterPlayerIds());
    gameReport.getSubmitterSessionIds().copyInto(collatedReport.getSubmitterSessionIds());

    //  obtain mid-game state if collated report is for a game in progress.
    //  for debug games (slave action), the getGameInfoSnapshotRequest will fail.
    if (!gameReport.getIsFinished())
    {
        GameManager::GameManagerMaster *gmMaster = static_cast<GameManager::GameManagerMaster *>(gController->getComponent(GameManager::GameManagerMaster::COMPONENT_ID, false));
        if (gmMaster != nullptr)
        {
            GameManager::GetGameInfoSnapshotRequest request;
            GameInfo gameInfo;
            request.setGameId(gameReport.getStartedGameInfo().getGameId());
            BlazeRpcError rc = gmMaster->getGameInfoSnapshot(request, gameInfo);
            if (rc != Blaze::ERR_OK)
            {
                WARN_LOG("[GameReportingSlaveImpl].completeCollation: no mid-game information for report id "
                    << collatedReport.getGameReport().getGameReportingId() << ": " << ErrorHelp::getErrorName(rc));
            }
            else
            {
                gameInfo.copyInto(collatedReport.getGameInfo());
            }
        }
        else
        {
            INFO_LOG("[GameReportingSlaveImpl].completeCollation: no mid-game information for report id "
                << collatedReport.getGameReport().getGameReportingId() << " because GameManagerMaster not found");
        }
    }

    //  linked to collectGameReport() - change when that section is refactored so that report type is set by the collator pipeline.
    collatedReport.setReportType(gameReport.getReportType());

    gameReportMaster->saveCollatedReport(collatedReport);

    // Here we pass the GameReportMasterPtr so that the the GameReportMaster::mRefCount remains greater than 1, which keeps the sliver pinned.
    TRACE_LOG("[GameReportingSlaveImpl].completeCollation: Queuing game report id:" << collatedReport.getGameInfo().getGameReportingId() << " for final processing");
    mProcessReportsFiberJobQueue.queueFiberJob(this, &GameReportingSlaveImpl::processGameReportMaster, gameReportMaster, "GameReportingSlaveImpl::processGameReportMaster");
}

void GameReportingSlaveImpl::processGameReportMaster(GameReportMasterPtr gameReportMaster)
{
    processReport(gameReportMaster->getSavedCollatedReport());
}

//    Trusted Report Processing - called from submitTrustedMidGameReport and submitTrustedEndGameReport
BlazeRpcError GameReportingSlaveImpl::scheduleTrustedReportProcess(BlazeId reportOwnerId, ReportType reportType, const SubmitGameReportRequest& submitReportRequest)
{
    const GameReport& gameReport = submitReportRequest.getGameReport();
    GameManager::GameReportingId gameReportingId = gameReport.getGameReportingId();

    if (EA_UNLIKELY(reportType != REPORT_TYPE_TRUSTED_MID_GAME && reportType != REPORT_TYPE_TRUSTED_END_GAME))
    {
        ERR_LOG("[GameReportingSlaveImpl].scheduleTrustedReportProcess: invalid report type (" << reportType << ") passed for report Id: " << gameReportingId );
        return ERR_SYSTEM;
    }

    TRACE_LOG("[GameReportingSlaveImpl].scheduleTrustedReportProcess: game reporting Id: " << gameReportingId );

    //  Prepare collated report.  Contents are:
    //      report type, send time, submitter list.
    //      clone of GameReport
    //      GameInfo (from getGameInfoSnapshot)
    //      clone of PrivateReport
    CollatedGameReport *collatedReport = BLAZE_NEW CollatedGameReport;
    collatedReport->setReportType(reportType);
    collatedReport->setSendTimeMs(TimeValue::getTimeOfDay().getMillis());

    collatedReport->getGameReport().setGameReportingId(gameReportingId);
    collatedReport->getGameReport().setGameReportName(gameReport.getGameReportName());
    // clone game report values for processing fiber (managed by CollatedGameReport)
    collatedReport->getGameReport().setReport(*gameReport.getReport()->clone());

    //  Get the GameInfo if available.
    if (gameReportingId != GameManager::INVALID_GAME_REPORTING_ID)
    {
        GameManager::GameId gameId = GameManager::INVALID_GAME_ID;
        const GameReportMaster* gameReportMaster = getReadOnlyGameReport(gameReportingId);
        if (gameReportMaster != nullptr)
        {
            gameId = gameReportMaster->getData()->getStartedGameInfo().getGameId();
        }
        if (gameId != GameManager::INVALID_GAME_ID)
        {
            GameManager::GameManagerMaster *gmMaster = static_cast<GameManager::GameManagerMaster *>(gController->getComponent(GameManager::GameManagerMaster::COMPONENT_ID, false));
            if (gmMaster != nullptr)
            {
                GameManager::GetGameInfoSnapshotRequest gameInfoReq;
                gameInfoReq.setGameId(gameId);
                BlazeRpcError gameInfoSnapshotErr = gmMaster->getGameInfoSnapshot(gameInfoReq, collatedReport->getGameInfo());
                if (gameInfoSnapshotErr != Blaze::ERR_OK)
                {
                    WARN_LOG("[GameReportingSlaveImpl].scheduleTrustedReportProcess: No GameManager::GameInfo for game reporting Id (" << gameReportingId 
                        << "), err=" << ErrorHelp::getErrorName(gameInfoSnapshotErr) );
                }
            }
        }
    }

    //  private report copy
    const EA::TDF::Tdf* privateReport = submitReportRequest.getPrivateReport();
    if (privateReport != nullptr)
    {
        CollatedGameReport::PrivateReportsMap& privateReports = collatedReport->getPrivateReports();
        privateReports[gCurrentUserSession->getSessionId()] = privateReports.allocate_element();
        privateReports[gCurrentUserSession->getSessionId()]->setData(*(privateReport->clone()));
    }

    //  submitter list (local session)
    UserSessionIdList& submitterList = collatedReport->getSubmitterSessionIds();
    submitterList.push_back(gCurrentUserSession->getSessionId());

    //  DNF Status not set for trusted game reports.
    
    //We cannot process the report here since report processing needs a fiber context in order to read and write stats so
    // we schedule a fiber call.
    mProcessReportsFiberJobQueue.queueFiberJob(this, &GameReportingSlaveImpl::processCollatedGameReport, CollatedGameReportPtr(collatedReport),
        "GameReportingSlaveImpl::processCollatedGameReport(trusted)");

    return ERR_OK;
}

void GameReportingSlaveImpl::processCollatedGameReport(CollatedGameReportPtr collatedGameReport)
{
    processReport(*collatedGameReport);
}

//    Submit a debug game report and gamemanager info.
BlazeRpcError GameReportingSlaveImpl::submitDebugGameReport(const SubmitDebugGameRequest &request)
{
    //  execute gameStarted/submit/gameFinished stages of the collator pipeline
    //  send report to a slave for processing.
    GameManager::GameReportingId gameReportingId = request.getGameInfo().getGameReportingId();

    //  Early out on types of games that we can't simulate.
    const GameInfo& gameInfo = request.getGameInfo();
    ReportType reportType = request.getReportType();

    if (gameInfo.getTrustedGameReporting())
    {
        ERR_LOG("[GameReportingSlaveImpl].submitDebugGameReport() : unsupported action - requires a topology host user to proceed (TBD)");
        return ERR_SYSTEM;
    }

    //  Simulate client submit report/game manager sequencing events
    //  The simulation should follow collation rules and report back errors as if this were a live game.

    // simululate game started
    StartedGameInfo startedGameInfo;
    startedGameInfo.setGameReportingId(gameReportingId);
    startedGameInfo.setGameReportName(gameInfo.getGameReportName());
    startedGameInfo.setTrustedGameReporting(gameInfo.getTrustedGameReporting());
    startedGameInfo.setTopology(request.getTopology());
    startedGameInfo.setTopologyHostId(INVALID_BLAZE_ID);           // TBD - only needed for dedicated server reports.

    BlazeRpcError simResult = gameStarted(startedGameInfo);
    if (simResult != ERR_OK)
    {
        return simResult;
    }

    // submit all reports, DNF first, then finished
    SubmitGameReportsMap::const_iterator cit = request.getDnfGameReports().begin();
    SubmitGameReportsMap::const_iterator citEnd = request.getDnfGameReports().end();
    for (; cit != citEnd; ++cit)
    {
        const SubmitGameReportRequest& submittedGameReport = *(*cit).second;
        BlazeId reportOwnerId = (*cit).first;
        BlazeRpcError collectResult = collectGameReport(reportOwnerId, reportType, submittedGameReport);
        if (simResult == ERR_OK && collectResult != ERR_OK)
        {
            INFO_LOG("[GameReportingSlaveImpl].submitDebugGameReport() : DNF collection failed for report [id=" << gameReportingId << ",player=" << reportOwnerId << "].");
            simResult = collectResult;
        }
    }

    // simulate game finished
    BlazeRpcError gameFinishedError = gameFinished(gameInfo);
    if (gameFinishedError != ERR_OK)
    {
        return gameFinishedError;
    }

    // submit finished player reports
    cit = request.getFinishedGameReports().begin();
    citEnd = request.getFinishedGameReports().end();
    for (; cit != citEnd; ++cit)
    {
        const SubmitGameReportRequest& submittedGameReport = *(*cit).second;
        BlazeId reportOwnerId = (*cit).first;
        BlazeRpcError collectResult = collectGameReport(reportOwnerId, reportType, submittedGameReport);
        if (simResult == ERR_OK && collectResult != ERR_OK)
        {
            INFO_LOG("[GameReportingSlaveImpl].submitDebugGameReport() : finished collection failed for report [id=" << gameReportingId << ",player=" << reportOwnerId << "].");
            simResult = collectResult;
        }
    }

    // completing collation should trigger a ProcessReport notification on a random slave

    return simResult;
}

void GameReportingSlaveImpl::processReport(CollatedGameReport& data)
{
    AutoIncDec autoGauge(mMetrics.mGaugeActiveProcessReportCount);

    //This fiber is granted super user permissions to execute all the RPCs it needs to get the job done.
    UserSession::SuperUserPermissionAutoPtr superUserPermission(true);

    mMetrics.mGamesProcessed.increment(1, data.getReportType(), data.getGameReport().getGameReportName());

    PlayerIdListPtr participantIdListPtr = BLAZE_NEW GameManager::PlayerIdList;
    BlazeRpcError reportError = ERR_OK;

    //  fill player ids with players in game for standard online game reports
    if (data.getReportType() == REPORT_TYPE_STANDARD)
    {
        GameManager::PlayerIdList& participantIds = *participantIdListPtr;

        // Populate playerIds vector with list of players in game from collated game report.
        GameInfo::PlayerInfoMap::const_iterator playerIt = data.getGameInfo().getPlayerInfoMap().begin();
        GameInfo::PlayerInfoMap::const_iterator playerEndIt = data.getGameInfo().getPlayerInfoMap().end();
        for (; playerIt != playerEndIt; playerIt++)
        {
            participantIds.push_back((*playerIt).first);
        }
    }

    bool isFinalResult = true;

    EA::TDF::TdfPtr customNotificationData;
    const char8_t* gameReportName = data.getGameReport().getGameReportName();
    const GameType *gameType = getGameTypeCollection().getGameType(gameReportName);
    if (gameType == nullptr)
    {
        reportError = GAMEREPORTING_ERR_INVALID_GAME_TYPE;
        // FIFA SPECIFIC CODE START
        TRACE_LOG("[GameReportingSlaveImpl].processReport: reportError GAMEREPORTING_ERR_INVALID_GAME_TYPE: " << reportError << ".");
        // FIFA SPECIFIC CODE END
    }
    else if ((BlazeRpcError)data.getError() == ERR_OK)
    {
        GameReportProcessorPtr reportProcessor = GameReportProcessorPtr(createGameReportProcessor(gameType->getReportProcessorClass()));
        if (reportProcessor == nullptr)
        {
            // Likely this warning is a result of a mismatch between the GameReportProcessor class factory and the game reporting configuration
            // (Undefined class name, or customer needs to update class factory)
            ERR_LOG("[GameReportingSlaveImpl].processReport: unable to create processor for submitted game report id: " << data.getGameReport().getGameReportingId());
            reportError = ERR_SYSTEM;
        }
        else
        {
            ProcessedGameReport processedReport(data.getReportType(), *gameType, data, mGameReportingMetricsCollection);
            GameManager::PlayerIdList& participantIds = *participantIdListPtr;

            //  setup default values for processed report based on collated game report.
            //  these can be changed by the GameReportProcessor::process() method if need be - these are just defaults.
            //  set up trusted report settings prior to process
            if (data.getReportType() == REPORT_TYPE_TRUSTED_MID_GAME || data.getReportType() == REPORT_TYPE_TRUSTED_END_GAME)
            {
                //  obtain player ids from report instead of from the game.
                Utilities::ReportParser reportParser(*gameType, processedReport);
                reportParser.parse(*data.getGameReport().getReport(), Utilities::ReportParser::REPORT_PARSE_PLAYERS);
                const Utilities::ReportParser::PlayerIdSet reportPlayerIdSet = reportParser.getPlayerIdSet();

                //  remove duplicate player id in the list
                GameManager::PlayerIdList::iterator iter = participantIds.begin();
                while (iter != participantIds.end())
                {
                    GameManager::PlayerId playerId = *iter;
                    if (reportPlayerIdSet.find(playerId) != reportPlayerIdSet.end())
                    {
                        iter = participantIds.erase(iter);
                    }
                    else
                    {
                        ++iter;
                    }
                }

                participantIds.insert(participantIds.end(), reportPlayerIdSet.begin(), reportPlayerIdSet.end());
            }

            // FIFA SPECIFIC CODE START
            TRACE_LOG("[GameReportingSlaveImpl].processReport: call reportProcessor->process");
            // FIFA SPECIFIC CODE END
            reportError = reportProcessor->process(processedReport, participantIds);

			bool reportProcessedSuccessfully = (reportError == ERR_OK);

            if (reportProcessedSuccessfully)
            {
                mMetrics.mGamesProcessedSuccessfully.increment(1, data.getReportType(), gameReportName);

                BlazeRpcError saveErr = ERR_OK;

                // save the report to the history tables if required
                if (processedReport.needsHistorySave())
                {
                    saveErr = saveHistoryReport(&processedReport, *gameType, participantIds, data.getSubmitterPlayerIds());
                    if (saveErr!= ERR_OK)
                    {
                        WARN_LOG("[GameReportingSlaveImpl].processReport() : Unable to saveHistoryReport for id=" << data.getGameReport().getGameReportingId() << ".");
                    }
                }

                if (saveErr == ERR_OK)
                {
                    // submit success report event
                    const GameInfo& gameInfo = data.getGameInfo();
                    submitSucceededEvent(processedReport, &gameInfo);

                    // encode PIN report to JSON and send to PIN
                    for (auto playerId : participantIds)
                    {
                        auto playerIter = processedReport.getPINGameReportMap().find(playerId);
                        if (playerIter != processedReport.getPINGameReportMap().end())
                        {
                            // GameReports can be large, so we send them individually.
                            PINSubmissionPtr pinSubmission = BLAZE_NEW PINSubmission;
                            GameManager::PINEventHelper::gameEnd(playerId, data.getGameId(), gameInfo, *playerIter->second.getReport(), *pinSubmission, mGameReportInGameEndPinEvent);
                            gUserSessionManager->sendPINEvents(pinSubmission);
                        }
                    }
                }
            }
            else
            {
                INFO_LOG("[GameReportingSlaveImpl].processReport() : Error processing game report for id=" << data.getGameReport().getGameReportingId() << ", type=" 
                         << data.getGameReport().getGameReportName() << ", error=" << (Blaze::ErrorHelp::getErrorName(reportError)));
                mMetrics.mGamesWithProcessingErrors.increment(1, data.getReportType(), gameReportName);

                if (BLAZE_COMPONENT_FROM_ERROR(reportError) == Blaze::Stats::StatsSlave::COMPONENT_ID)
                {
                    mMetrics.mGamesWithStatUpdateFailure.increment(1, data.getReportType(), gameReportName);

                    UserSessionIdList pinErrorSessionIdList;
                    UserSessionIdList userSessionIdList;
                    GMPlayerIdList::const_iterator itr = participantIdListPtr->begin();
                    GMPlayerIdList::const_iterator end = participantIdListPtr->end();
                    for (; itr != end; ++itr)
                    {
                        BlazeRpcError err = gUserSessionManager->getSessionIds((*itr), userSessionIdList);
                        if ((err == ERR_OK) && !userSessionIdList.empty())
                        {
                            pinErrorSessionIdList.insert(pinErrorSessionIdList.end(), userSessionIdList.begin(), userSessionIdList.end());
                        }
                        userSessionIdList.clear();
                    }

                    if (!pinErrorSessionIdList.empty())
                    {
                        gUserSessionManager->sendPINErrorEventToUserSessionIdList(ErrorHelp::getErrorName(GAMEREPORTING_CONFIG_ERR_STAT_UPDATE_FAILED),
                            RiverPoster::stat_update_failed, pinErrorSessionIdList);
                    }
                }

                // submit failure report event
                const GameInfo& gameInfo = data.getGameInfo();
                submitGameReportProcessFailedEvent(processedReport, &gameInfo);
            }

            //  store copy of custom notification data for distribution.
            if (processedReport.getCustomData() != nullptr)
            {
                customNotificationData = processedReport.getCustomData()->clone();
            }

            isFinalResult = processedReport.isFinalResult();

			// FIFA SPECIFIC CODE START
			reportProcessor->postProcess(processedReport, reportProcessedSuccessfully);
			// FIFA SPECIFIC CODE END
        }
    }
    else
    {
        reportError = (BlazeRpcError)data.getError();
        // FIFA SPECIFIC CODE START
        TRACE_LOG("[GameReportingSlaveImpl].processReport: reportError: " << reportError << ".");
        // FIFA SPECIFIC CODE END
    }

    mMetrics.mReportProcessingDelayMs.increment(TimeValue::getTimeOfDay().getMillis() - data.getSendTimeMs());

    // post processing
    GameManager::GameReportingId gameReportingId = data.getGameReport().getGameReportingId();
    TRACE_LOG("[GameReportingSlaveImpl].processReport: report id " << gameReportingId << " completed processing");

    // If this is a trusted game report and trusted report collation is disabled, there won't be an entry for this report in the mGameReportMasterPtrByIdMap map.
    // In this case, we skip the postProcessCollatedReport() step to avoid an unnecessary lookup and log spam. [GOS-29656]
    bool skipPostProcessCollatedReport = (data.getReportType() == REPORT_TYPE_TRUSTED_END_GAME || data.getReportType() == REPORT_TYPE_TRUSTED_MID_GAME) && !getConfig().getEnableTrustedReportCollation();
    if (gameReportingId != GameManager::INVALID_GAME_REPORTING_ID && !skipPostProcessCollatedReport)
    {
        GameReportMasterPtr gameReportMaster = getWritableGameReport(gameReportingId);
        if (gameReportMaster == nullptr)
        {
            WARN_LOG("[GameReportingSlaveImpl].processReport: missing collating data for report id " << gameReportingId);
        }
        else
        {
            // Changes made to gameReport will be committed when gameReportMaster goes out of scope.
            ReportCollateData* gameReport = gameReportMaster->getData();

            // create a game report collator
            GameReportCollatorPtr collator = GameReportCollatorPtr(createGameReportCollator(gameType->getReportCollatorClass(), *gameReport));
            if (collator != nullptr)
            {
                collator->postProcessCollatedReport();
            }

            if (isFinalResult)
            {
                TRACE_LOG("[GameReportingSlaveImpl].processReport: report id " << gameReportingId << " erased");
                //  drop the game report.
                eraseGameReport(gameReportingId);
            }
        }
    }

    // distribute results: send notifications to the players in the game indicating if there was a problem while processing the game reports or not
    ResultNotification result;
    result.setBlazeError(reportError);
    result.setGameReportingId(gameReportingId);
    result.setGameHistoryId(gameReportingId);
    result.setFinalResult(isFinalResult);
    if (customNotificationData != nullptr)
    {
        //  caller owns memory for notification
        result.setCustomData(*customNotificationData);
    }

    UserSessionIdVectorSet sessionIds;

    sessionIds.reserve(data.getSubmitterSessionIds().size() + participantIdListPtr->size());
    sessionIds.insert(data.getSubmitterSessionIds().begin(), data.getSubmitterSessionIds().end());

    GMPlayerIdList::const_iterator itr = participantIdListPtr->begin();
    GMPlayerIdList::const_iterator end = participantIdListPtr->end();
    for (; itr != end; ++itr)
    {
        GameInfo::PlayerInfoMap::const_iterator gameInfoPlayerItr = data.getGameInfo().getPlayerInfoMap().find(*itr);
        if (gameInfoPlayerItr != data.getGameInfo().getPlayerInfoMap().end())
        {
            sessionIds.insert(gameInfoPlayerItr->second->getUserSessionId());
        }
    }

    sendResultNotificationToUserSessionsById(sessionIds.begin(), sessionIds.end(), UserSessionIdIdentityFunc(), &result);
}

void GameReportingSlaveImpl::submitSucceededEvent(const ProcessedGameReport& processedReport, const GameInfo *gameInfo, bool isOffline) const
{
    // submit success report event
    GameReportSucceeded successGameReport;
    const GameReport &gameReport = processedReport.getGameReport();

    successGameReport.setGameHistoryId(processedReport.getGameReport().getGameReportingId());
    successGameReport.setIsOfflineReport(isOffline);
    gameReport.copyInto(successGameReport.getReport());
    
    if (gameInfo != nullptr)
    {
        successGameReport.getGameSettings().setBits(gameInfo->getGameSettings().getBits());
        successGameReport.setDurationMsec(gameInfo->getGameDurationMs());
    }

    processedReport.getReportFlagInfo()->copyInto(successGameReport.getReportFlag());

    //  just submit the report TDF as is.
    gEventManager->submitEvent(EVENT_GAMEREPORTSUCCEEDEDEVENT, successGameReport, true);
}

void GameReportingSlaveImpl::submitGameReportProcessFailedEvent(const ProcessedGameReport& processedReport, const GameInfo *gameInfo, bool isOffline) const
{
    // submit report process failed event
    GameReportProcessFailed failureGameReport;

    const GameReport &gameReport = processedReport.getGameReport();
    gameReport.copyInto(failureGameReport.getReport());
    failureGameReport.setGameReportingId(gameReport.getGameReportingId());
    failureGameReport.setIsOfflineReport(isOffline);

    if (gameInfo != nullptr)
    {
        failureGameReport.getGameSettings().setBits(gameInfo->getGameSettings().getBits());
        failureGameReport.setDurationMsec(gameInfo->getGameDurationMs());
    }

    processedReport.getReportFlagInfo()->copyInto(failureGameReport.getReportFlag());

    gEventManager->submitEvent((uint32_t)GameReportingSlave::EVENT_GAMEREPORTPROCESSFAILEDEVENT, failureGameReport, true);
}

void GameReportingSlaveImpl::submitGameReportCollationFailedEvent(GameReportsMap& reportsMap, const GameManager::GameReportingId& reportingId) const
{
    // submit report collation failed event
    GameReportCollationFailed failureGameReport;
    failureGameReport.setGameReportingId(reportingId);
    failureGameReport.setReports(reportsMap);

    gEventManager->submitEvent((uint32_t)GameReportingSlave::EVENT_GAMEREPORTCOLLATIONFAILEDEVENT, failureGameReport);
}

void GameReportingSlaveImpl::submitGameReportOrphanedEvent(const GameInfo& gameInfo, const GameManager::GameReportingId& reportingId) const
{
    GameReportOrphaned event;
    event.setGameReportingId(reportingId);
    GameInfo::PlayerInfoMap::const_iterator playerIt = gameInfo.getPlayerInfoMap().begin();
    GameInfo::PlayerInfoMap::const_iterator playerEndIt = gameInfo.getPlayerInfoMap().end();
    for (; playerIt != playerEndIt; ++playerIt)
    {
        event.getPlayerIdList().push_back((*playerIt).first);
    }
    gEventManager->submitEvent(GameReportingSlave::EVENT_GAMEREPORTORPHANEDEVENT, event);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Store game history for the processed game report.

BlazeRpcError GameReportingSlaveImpl::saveHistoryReport(ProcessedGameReport *report, const GameType &gameType, 
                                                        const GameManager::PlayerIdList &participantIds, const GameManager::PlayerIdList &submitterIds,
                                                        const bool isOnline) const
{
    if (gameType.getHistoryTableDefinitions().empty())
    {
        WARN_LOG("[GameReportingSlaveImpl].saveHistoryReport: History table for '" << gameType.getGameReportName().c_str() << "' is not configured.");
        return ERR_SYSTEM;
    }

    DbConnPtr conn = getDbConnPtr(gameType.getGameReportName());
    if (conn == nullptr)
    {
        ERR_LOG("[GameReportingSlaveImpl].saveHistoryReport: Failed to obtain connection");
        return ERR_SYSTEM;
    }

    GameHistoryId gameReportingId = report->getGameReport().getGameReportingId();
    uint32_t totalRef = 0;
    GameRefCountMap gameRefCountMap;

    // Now begin the transaction during this transaction we will insert into the
    // game types game, player and team table.  If any of them fail, the transaction
    // will rollback.
    BlazeRpcError err = conn->startTxn();
    if (err != DB_ERR_OK)
    {
        INFO_LOG("[GameReportingSlaveImpl].saveHistoryReport: Failed to start txn.  dberr(" << getDbErrorString(err) << ")");
        return ERR_SYSTEM;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(conn);

    //  set up query to insert game info to built-in game table
    insertIntoBuiltInGameTable(conn, query, gameType, gameReportingId, report->getReportFlagInfo()->getFlag(),
                               report->getReportFlagInfo()->getFlagReason(), isOnline);

    //  set up query to insert game participants to built-in table
    insertIntoBuiltInTable(query, gameType, RESERVED_PARTICIPANTTABLE_NAME, gameReportingId, participantIds);

    //  set up query to insert report submitter to built-in table
    insertIntoBuiltInTable(query, gameType, RESERVED_SUBMITTER_TABLE_NAME, gameReportingId, submitterIds);

    //  initialize game history's common data with the game report.
    GameHistoryReport& historyReport = report->getGameHistoryReport();

    GameHistoryReport::TableRowMap::const_iterator tableIter, tableEnd;
    tableIter = historyReport.getTableRowMap().begin();
    tableEnd = historyReport.getTableRowMap().end();

    for (; tableIter != tableEnd; ++tableIter)
    {
        GameHistoryReport::TableRowList::const_iterator rowIter, rowEnd;
        rowIter = tableIter->second->getTableRowList().begin();
        rowEnd = tableIter->second->getTableRowList().end();

        const HistoryTableDefinitions& historyTableDefinitions = gameType.getHistoryTableDefinitions();
        HistoryTableDefinitions::const_iterator historyTableIter = historyTableDefinitions.find(tableIter->first);
        if (historyTableIter == historyTableDefinitions.end())
        {
            continue;
        }

        HistoryTableDefinition *historyTableDefinition = historyTableIter->second;        
        for (; rowIter != rowEnd; ++rowIter)
        {
            const Collections::AttributeMap& attributeMap = (*rowIter)->getAttributeMap();
            insertIntoHistoryTable(query, gameType, tableIter->first, historyTableDefinition, 
                attributeMap, gameReportingId);

            if (!historyTableDefinition->getPrimaryKeyList().empty())
            {
                updateGameRefTable(conn, query, gameType, tableIter->first, historyTableDefinition, 
                    attributeMap, gameReportingId, gameRefCountMap);
                ++totalRef;
            }
        }
    }

    updateGameRef(query, gameType, gameReportingId, totalRef, gameRefCountMap);

    if (!query->isEmpty())
    {
        err = conn->executeMultiQuery(query);

        if (err != DB_ERR_OK)
        {
            ERR_LOG("[GameReportingSlaveImpl].saveHistoryReport(): Failed to execute history query. Will rollback: " << getDbErrorString(err));
            conn->rollback();
            return err;
        }
    }

    BlazeRpcError commitError = conn->commit();
    if (commitError != DB_ERR_OK)
    {
        ERR_LOG("[GameReportingSlaveImpl].saveHistoryReport(): Failed to commit game history insertion. Will rollback: " << getDbErrorString(commitError));
        conn->rollback();
        return ERR_SYSTEM;
    }

    TRACE_LOG("[GameReportingSlaveImpl].saveHistoryReport(): Committed game history for report id=" << report->getGameReport().getGameReportingId() 
              << ",type=" << gameType.getGameReportName().c_str());

    return ERR_OK;
}

void GameReportingSlaveImpl::insertIntoHistoryTable(QueryPtr& query, const GameType &gameType,
                                                    const char8_t* tableName,
                                                    const HistoryTableDefinition *historyTableDefinition,
                                                    const Collections::AttributeMap& attributeMap,
                                                    const GameHistoryId& gameHistoryId) const
{
    char8_t fullTableName[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(fullTableName, sizeof(fullTableName), "%s_hist_%s",
        gameType.getGameReportName().c_str(), tableName);
    blaze_strlwr(fullTableName);

    query->append("INSERT INTO `$s` SET game_id=$U, ", fullTableName, gameHistoryId);
    if (insertAssignments(query, historyTableDefinition, attributeMap, false) == 0)
    {
        //  no columns added, remove the trailing comma and space - (since we appended the query with a comma/space assuming columns.)
        query->trim(2);
    }
    query->append(";\n");
}

uint32_t GameReportingSlaveImpl::insertAssignments(QueryPtr& query, const HistoryTableDefinition *historyTableDefinition,
                                                   const Collections::AttributeMap& attributeMap, 
                                                   bool onlyPrimaryKey, bool separateByAnd) const
{
    HistoryTableDefinition::ColumnList::const_iterator colIter, colEnd;
    colIter = historyTableDefinition->getColumnList().begin();
    colEnd = historyTableDefinition->getColumnList().end();

    if (colIter == colEnd)
        return 0;

    uint32_t numColumns = 0;

    for (; colIter != colEnd; ++colIter)
    {
        if (onlyPrimaryKey)
        {
            HistoryTableDefinition::PrimaryKeyList::const_iterator pkIter, pkEnd;
            pkIter = historyTableDefinition->getPrimaryKeyList().begin();
            pkEnd = historyTableDefinition->getPrimaryKeyList().end();

            bool isPrimaryKey = false;
            for (; pkIter != pkEnd; ++pkIter)
            {
                if (blaze_strcmp(pkIter->c_str(), colIter->first.c_str()) == 0)
                {
                    isPrimaryKey = true;
                    break;
                }
            }

            if (!isPrimaryKey)
                continue;
        }

        Collections::AttributeMap::const_iterator attribIter;
        attribIter = attributeMap.find(colIter->first);

        if (attribIter != attributeMap.end())
        {
            switch (colIter->second)
            {
            case HistoryTableDefinition::INT: 
            case HistoryTableDefinition::FLOAT: 
            case HistoryTableDefinition::UINT:
            case HistoryTableDefinition::DEFAULT_INT:
            case HistoryTableDefinition::ENTITY_ID:
                query->append("`$s`=$s", colIter->first.c_str(), attribIter->second.c_str());
                if (separateByAnd)
                    query->append(" AND ");
                else
                    query->append(", ");
                ++numColumns;
                break;
            case HistoryTableDefinition::STRING: 
                query->append("`$s`='$s'", colIter->first.c_str(), attribIter->second.c_str());
                if (separateByAnd)
                    query->append(" AND ");
                else
                    query->append(", ");
                ++numColumns;
                break;
            default:
                break;
            }
        }  
    }

    if (numColumns > 0)
    {
        if (separateByAnd)
            query->trim(5);
        else
            query->trim(2);
    }
    return numColumns;
}

BlazeRpcError GameReportingSlaveImpl::updateGameRefTable(DbConnPtr& conn, QueryPtr& query, const GameType &gameType,
                                                   const char *tableName,
                                                   const HistoryTableDefinition *historyTableDefinition,
                                                   const Collections::AttributeMap& attributeMap,
                                                   const GameHistoryId& gameHistoryId, GameRefCountMap &map) const
{
    BlazeRpcError error = DB_ERR_OK;

    uint32_t limit = historyTableDefinition->getHistoryLimit();
    if (limit == 0)
        return error;

    char8_t refTableName[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(refTableName, sizeof(refTableName), "%s_hist_%s_ref", gameType.getGameReportName().c_str(), tableName);
    blaze_strlwr(refTableName);

    DbResultPtr result;

    QueryPtr selectQuery = DB_NEW_QUERY_PTR(conn);
    selectQuery->append("SELECT `$s0`", GAMEID_COLUMN_PREFIX);

    for (uint32_t i = 1; i < limit; ++i)
    {
        selectQuery->append(", `$s$u`", GAMEID_COLUMN_PREFIX, i);
    }

    selectQuery->append(" FROM `$s` WHERE ", refTableName);
    if (insertAssignments(selectQuery, historyTableDefinition, attributeMap, true, true) == 0)
    {
        selectQuery->trim(7); // trim " WHERE " if no columns added
    }
    selectQuery->append(" FOR UPDATE;");

    error = conn->executeQuery(selectQuery, result);


    if (error != DB_ERR_OK)
    {
        ERR_LOG("[GameReportingSlaveImpl].updateGameRefTable: failed to select a row from table " << tableName << ": " << getDbErrorString(error));
    }
    else
    {
        insertIntoGameRefTable(query, refTableName, historyTableDefinition, attributeMap, gameHistoryId, limit, map, result);
    }

    return error;
}

void GameReportingSlaveImpl::insertIntoGameRefTable(QueryPtr& query, const char8_t* tableName,
                                                    const HistoryTableDefinition *historyTableDefinition,
                                                    const Collections::AttributeMap& attributeMap,
                                                    const GameHistoryId& gameHistoryId, uint32_t limit,
                                                    GameRefCountMap &map, DbResultPtr& result) const
{
    if (result != nullptr && !result->empty())
    {
        query->append("UPDATE `$s` SET `$s0`=$U", tableName, GAMEID_COLUMN_PREFIX, gameHistoryId);

        // there should only be one row returned
        DbResult::const_iterator iter = result->begin();

        if (iter != result->end())
        {
            const DbRow *row = *iter;

            for (uint32_t i = 0; i < limit-1; ++i)
            {
                query->append(", `$s$u`=$U", GAMEID_COLUMN_PREFIX, i + 1, row->getUInt64(i));
            }

            GameHistoryId lrGameId = row->getUInt64(limit-1);

            if (lrGameId > 0)
            {
                GameRefCountMap::iterator grcIter = map.find(lrGameId);

                if (grcIter != map.end())
                {
                    ++(grcIter->second);
                }
                else
                {
                    map[lrGameId] = 1;
                }
            }
        }

        query->append(" WHERE ");
        if (insertAssignments(query, historyTableDefinition, attributeMap, true, true) == 0)
        {
            query->trim(7); // remove " WHERE "
        }
        query->append(";\n");
    }
    else
    {
        query->append("INSERT INTO `$s` SET ", tableName);
        if (insertAssignments(query, historyTableDefinition, attributeMap, true) > 0)
        {
            query->append(", ");
        }
        query->append("`$s0`=$U", GAMEID_COLUMN_PREFIX, gameHistoryId);

        for (uint32_t i = 1; i < limit; ++i)
        {
            query->append(", `$s$u`=0", GAMEID_COLUMN_PREFIX, i);
        }

        query->append(";\n");
    }
}

void GameReportingSlaveImpl::insertIntoBuiltInGameTable(DbConnPtr& conn, QueryPtr& query, const GameType &gameType,
                                                        const GameHistoryId& gameHistoryId, const ReportFlag &flag, 
                                                        const char8_t* flagReason, const bool isOnline) const
{
    char8_t builtInGameTable[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(builtInGameTable, sizeof(builtInGameTable), "%s_%s",
        gameType.getGameReportName().c_str(), RESERVED_GAMETABLE_NAME);
    blaze_strlwr(builtInGameTable);

    query->append("INSERT INTO `$s` SET game_id=$U, online=$u, flags=$u, flag_reason='$s';", 
        builtInGameTable, gameHistoryId, isOnline ? 1 : 0, flag.getBits(), flagReason);
}

void GameReportingSlaveImpl::insertIntoBuiltInTable(QueryPtr& query, const GameType &gameType, const char8_t* builtInTableSuffix,
                                                    const GameHistoryId& gameHistoryId, const GameManager::PlayerIdList &playerIds) const
{
    char8_t builtInTableName[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(builtInTableName, sizeof(builtInTableName), "%s_%s",
        gameType.getGameReportName().c_str(), builtInTableSuffix);
    blaze_strlwr(builtInTableName);

    GameManager::PlayerIdList::const_iterator iter = playerIds.begin();
    GameManager::PlayerIdList::const_iterator end = playerIds.end();
    for (; iter != end; ++iter)
    {
        query->append("INSERT INTO `$s` SET game_id=$U, entity_id=$U;", builtInTableName, gameHistoryId, *iter);
    }
}

void GameReportingSlaveImpl::updateGameRef(QueryPtr& query, const GameType &gameType,
                                           const GameHistoryId& gameHistoryId,
                                           uint32_t totalRef, GameRefCountMap &map) const
{
    char8_t builtInGameTable[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(builtInGameTable, sizeof(builtInGameTable), "%s_%s", gameType.getGameReportName().c_str(), RESERVED_GAMETABLE_NAME);
    blaze_strlwr(builtInGameTable);

    query->append("UPDATE `$s` SET total_gameref=$u WHERE game_id=$U;\n", builtInGameTable, totalRef, gameHistoryId);

    if (!map.empty())
    {
        GameRefCountMap::const_iterator grcIter, grcEnd;
        grcIter = map.begin();
        grcEnd = map.end();

        for (; grcIter != grcEnd; ++grcIter)
        {
            if (grcIter->first > 0)
            {
                query->append("UPDATE `$s` SET total_gameref=total_gameref-$u WHERE game_id=$U;\n", 
                    builtInGameTable, grcIter->second, grcIter->first);
                query->append("DELETE FROM `$s` WHERE game_id=$U AND total_gameref=0;\n", 
                    builtInGameTable, grcIter->first);
            }
        }
    }
}

void GameReportingSlaveImpl::runBootstrap(BlazeRpcError& err)
{
    //  GameHistory bootstrap execution.
    GameHistoryBootstrap *bootstrap = BLAZE_NEW GameHistoryBootstrap(this, &mGameTypeCollection.getGameTypeMap(),
        mGameHistoryConfig.getGameReportQueriesCollection().getGameReportQueryConfigMap(),
        mGameHistoryConfig.getGameReportViewsCollection().getGameReportViewConfigMap(),
        gController->isDBDestructiveAllowed());

    bool success = bootstrap->run();
    delete bootstrap;

    if (!success)
    {
        ERR_LOG("[GameReportingSlaveImpl].runBootstrap: failed bootstrapping game history, shutting down.");
        err = ERR_SYSTEM;
    }
    else
    {
        scheduleGameHistoryPurge();
        err = ERR_OK;
    }
}

void GameReportingSlaveImpl::scheduleGameHistoryPurge()
{
    // The fiber is being killed at server shutdown rather than running to completion.
    // Add a check to ensure that this will not be scheduled another timer if the
    // component is shutting down or has been shutdown
    if (mState == SHUTDOWN || mState == SHUTTING_DOWN)
    {
        return;
    }

    // revoke previous timer because during reconfigure we may need to speed up or slow down sweep time
    TaskSchedulerSlaveImpl *sched = static_cast<TaskSchedulerSlaveImpl*>(gController->getComponent(TaskSchedulerSlave::COMPONENT_ID));
    if (sched == nullptr)
    {
        return;
    }

    if (mPurgeTaskId != INVALID_TASK_ID)
    {
        SPAM_LOG("[GameReportingSlaveImpl].scheduleGameHistoryPurge: cancelling purge task id " << mPurgeTaskId);
        sched->cancelTask(mPurgeTaskId);
    }

    uint32_t purgeInterval = mHighFrequencyPurge ? static_cast<uint32_t>(getConfig().getGameHistory().getHighFrequentPurgeInterval().getSec())
                                                  : static_cast<uint32_t>(getConfig().getGameHistory().getLowFrequentPurgeInterval().getSec());

    sched->createTask(GAMEHISTORY_PURGE_TASKNAME, COMPONENT_ID, nullptr, static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec()) + purgeInterval, 0, purgeInterval);

    SPAM_LOG("[GameReportingSlaveImpl].scheduleGameHistoryPurge: scheduled the next purge after " << purgeInterval << " sec with purge task id " << mPurgeTaskId);
}

void GameReportingSlaveImpl::doGameHistoryPurge()
{
    SPAM_LOG("[GameReportingSlaveImpl].doGameHistoryPurge: starting game history purge");

    bool moreFrequentPurge = false;

    GameTypeMap::const_iterator itr = mGameTypeCollection.getGameTypeMap().begin();
    GameTypeMap::const_iterator end = mGameTypeCollection.getGameTypeMap().end();
    for (; itr != end; ++itr)
    {
        const GameType *gameType = itr->second;
        const GameManager::GameReportName& gameReportName = gameType->getGameReportName();
        DbConnPtr conn = getDbConnPtr(gameReportName);
        if (conn == nullptr)
        {
            ERR_LOG("[GameReportingSlaveImpl].doGameHistoryPurge: failed to grab database connection");
            return;
        }

        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        if (query == nullptr)
        {
            ERR_LOG("[GameReportingSlaveImpl].doGameHistoryPurge: failed to grab new query object");
            return;
        }

        DbResultPtr dbResult;
        BlazeRpcError dbError;

        if (gameType->getHistoryTableDefinitions().empty())
        {
            // no history table defined in this game type
            continue;
        }

        char8_t builtInGameTable[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
        blaze_snzprintf(builtInGameTable, sizeof(builtInGameTable), "%s_%s", gameType->getGameReportName().c_str(), RESERVED_GAMETABLE_NAME);
        blaze_strlwr(builtInGameTable);

        // purge games that have been stored in database for too long
        if (mGameHistoryConfig.getConfig().getHistoryExpiry() != 0)
        {
            query->append("DELETE FROM `$s` WHERE timestamp < DATE_SUB(NOW(), INTERVAL $U SECOND) LIMIT $u;",
                builtInGameTable, mGameHistoryConfig.getConfig().getHistoryExpiry().getSec(),
                mGameHistoryConfig.getConfig().getMaxGamesToPurgePerInterval());

            dbError = conn->executeQuery(query, dbResult);

            if (dbError == DB_ERR_OK)
            {
                const uint32_t affected = dbResult->getAffectedRowCount();
                if (affected > 0)
                {
                    INFO_LOG("[GameReportingSlaveImpl].doGameHistoryPurge: purged " << affected << " expired game reports of type " << gameReportName);
                    moreFrequentPurge = true;
                }
                else
                {
                    SPAM_LOG("[GameReportingSlaveImpl].doGameHistoryPurge: found no expired game reports of type " << gameReportName);
                }
            }
            else
            {
                 // delete failed
                 ERR_LOG("[GameReportingSlaveImpl].doGameHistoryPurge: failed to purge expired games: " << getDbErrorString(dbError));
            }

            DB_QUERY_RESET(query);
        }
    }

    // re-schedule the *recurring* purge task if frequency changes
    if (mHighFrequencyPurge != moreFrequentPurge)
    {
        mHighFrequencyPurge = moreFrequentPurge;
        scheduleGameHistoryPurge();
    }
}

///////////////////////////////////////////////////////////////////////////////////
//  Component Status (getStatus)
void GameReportingSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    Blaze::ComponentStatus::InfoMap& gamereportingStatusMap = status.getInfo();
    char8_t buf[64];

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mReportQueueDelayMs.getTotal()); // status metric says "Gauge", but it's really a counter
    gamereportingStatusMap["GaugeQueueDelayMs"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mReportProcessingDelayMs.getTotal());
    gamereportingStatusMap["TotalGameProcessingDelayMs"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mReportsExpected.getTotal()); // non-offline reports expected to be submitted
    gamereportingStatusMap["TotalReportsExpected"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mReportsReceived.getTotal()); // non-offline submitted reports
    gamereportingStatusMap["TotalReportsReceived"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mReportsRejected.getTotal()); // non-offline submitted reports that were rejected
    gamereportingStatusMap["TotalReportsRejected"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mDuplicateReports.getTotal()); // non-offline submitted reports that were duplicates
    gamereportingStatusMap["TotalDuplicateReports"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mReportsForInvalidGames.getTotal()); // non-offline submitted reports that were for invalid games
    gamereportingStatusMap["TotalReportsForInvalidGames"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mReportsFromPlayersNotInGame.getTotal()); // non-offline submitted reports that were from invalid players
    gamereportingStatusMap["TotalReportsFromPlayersNotInGame"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGames.getTotal()); // non-offline games started (almost the call count for gameStarted RPC)
    gamereportingStatusMap["TotalGames"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mReportsAccepted.getTotal()); // non-offline submitted reports that were accepted
    gamereportingStatusMap["TotalReportsAccepted"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesWithNoValidReports.getTotal()); // failed collation due to no valid reports
    gamereportingStatusMap["TotalGamesWithNoValidReports"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesWithDiscrepencies.getTotal()); // failed collation due to discrepancies
    gamereportingStatusMap["TotalGamesWithDiscrepencies"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesCollatedSuccessfully.getTotal());
    gamereportingStatusMap["TotalGamesCollatedSuccessfully"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesProcessed.getTotal());
    gamereportingStatusMap["TotalGamesProcessed"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesProcessedSuccessfully.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_STANDARD) } }));
    gamereportingStatusMap["TotalGamesProcessedSuccessfully"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mValidReports.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_STANDARD) } }));
    gamereportingStatusMap["TotalValidReports"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mInvalidReports.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_STANDARD) } }));
    gamereportingStatusMap["TotalInvalidReports"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesWithProcessingErrors.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_STANDARD) } }));
    gamereportingStatusMap["TotalGamesWithProcessingErrors"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesWithStatUpdateFailure.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_STANDARD) } }));
    gamereportingStatusMap["TotalGamesWithStatUpdateFailure"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mInvalidReports.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_OFFLINE) } }));
    gamereportingStatusMap["TotalInvalidOfflineGames"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mOfflineGames.getTotal()); // offline games reported (almost the call count for submitOfflineGameReport RPC)
    gamereportingStatusMap["TotalOfflineGames"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesProcessedSuccessfully.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_OFFLINE) } }));
    gamereportingStatusMap["TotalOfflineGamesProcessedSuccessfully"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesWithProcessingErrors.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_OFFLINE) } }));
    gamereportingStatusMap["TotalOfflineGamesWithProcessingErrors"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesWithStatUpdateFailure.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_OFFLINE) } }));
    gamereportingStatusMap["TotalOfflineGamesWithStatUpdateFailure"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mValidReports.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_TRUSTED_END_GAME) } }));
    gamereportingStatusMap["TotalTrustedEndReports"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesProcessedSuccessfully.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_TRUSTED_END_GAME) } }));
    gamereportingStatusMap["TotalTrustedEndReportsProcessedSuccessfully"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesWithProcessingErrors.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_TRUSTED_END_GAME) } }));
    gamereportingStatusMap["TotalTrustedEndReportsWithProcessingErrors"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesWithStatUpdateFailure.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_TRUSTED_END_GAME) } }));
    gamereportingStatusMap["TotalTrustedEndReportsWithStatUpdateFailure"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mInvalidReports.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_TRUSTED_END_GAME) } }));
    gamereportingStatusMap["TotalInvalidTrustedEndReports"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mValidReports.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_TRUSTED_MID_GAME) } }));
    gamereportingStatusMap["TotalTrustedMidReports"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesProcessedSuccessfully.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_TRUSTED_MID_GAME) } }));
    gamereportingStatusMap["TotalTrustedMidReportsProcessedSuccessfully"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesWithProcessingErrors.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_TRUSTED_MID_GAME) } }));
    gamereportingStatusMap["TotalTrustedMidReportsWithProcessingErrors"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesWithStatUpdateFailure.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_TRUSTED_MID_GAME) } }));
    gamereportingStatusMap["TotalTrustedMidReportsWithStatUpdateFailure"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mInvalidReports.getTotal({ { Metrics::Tag::report_type, ReportTypeToString(REPORT_TYPE_TRUSTED_MID_GAME) } }));
    gamereportingStatusMap["TotalInvalidTrustedMidReports"] = buf;

    for (auto& gameTypeItr : mGameTypeCollection.getGameTypeMap())
    {
        char8_t nameBuf[128];
        blaze_snzprintf(nameBuf, sizeof(nameBuf), "TotalGamesProcessedSuccessfully_%s", gameTypeItr.first.c_str());
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesProcessedSuccessfully.getTotal({ { Metrics::Tag::game_report_name, gameTypeItr.first.c_str() } }));
        gamereportingStatusMap[nameBuf] = buf;
        blaze_snzprintf(nameBuf, sizeof(nameBuf), "TotalGamesWithDiscrepencies_%s", gameTypeItr.first.c_str());
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesWithDiscrepencies.getTotal({ { Metrics::Tag::game_report_name, gameTypeItr.first.c_str() } }));
        gamereportingStatusMap[nameBuf] = buf;
        blaze_snzprintf(nameBuf, sizeof(nameBuf), "TotalGamesWithProcessingErrors_%s", gameTypeItr.first.c_str());
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mGamesWithProcessingErrors.getTotal({ { Metrics::Tag::game_report_name, gameTypeItr.first.c_str() } }));
        gamereportingStatusMap[nameBuf] = buf;
        blaze_snzprintf(nameBuf, sizeof(nameBuf), "TotalInvalidReports_%s", gameTypeItr.first.c_str());
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mInvalidReports.getTotal({ { Metrics::Tag::game_report_name, gameTypeItr.first.c_str() } }));
        gamereportingStatusMap[nameBuf] = buf;
        blaze_snzprintf(nameBuf, sizeof(nameBuf), "TotalReportsForInvalidGames_%s", gameTypeItr.first.c_str());
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mReportsForInvalidGames.getTotal({ { Metrics::Tag::game_report_name, gameTypeItr.first.c_str() } }));
        gamereportingStatusMap[nameBuf] = buf;
    }

    mGameReportingMetricsCollection.addMetricsToStatusMap(gamereportingStatusMap);

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mActiveStatsServiceUpdates.get());
    gamereportingStatusMap["ActiveStatsServiceUpdates"] = buf;

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mMetrics.mActiveStatsServiceRpcs.get());
    gamereportingStatusMap["ActiveStatsServiceRpcs"] = buf;
}

const eastl::string GameReportingSlaveImpl::populateUserCoreIdentHelper(UserInfoPtr userinfo, const char8_t* ident)
{
    if (blaze_stricmp(ident, "switchId") == 0)
    {
        char8_t result[256];
        blaze_snzprintf(result, sizeof(result), "%s", userinfo->getPlatformInfo().getExternalIds().getSwitchId());
        return result;
    }

    char8_t result[128];
    if (blaze_stricmp(ident, "externalId") == 0)
    {
        blaze_snzprintf(result, sizeof(result), "%" PRId64, getExternalIdFromPlatformInfo(userinfo->getPlatformInfo()));
    }
    else if (blaze_stricmp(ident, "xblId") == 0)
    {
        blaze_snzprintf(result, sizeof(result), "%" PRId64, userinfo->getPlatformInfo().getExternalIds().getXblAccountId());
    }
    else if (blaze_stricmp(ident, "psnId") == 0)
    {
        blaze_snzprintf(result, sizeof(result), "%" PRId64, userinfo->getPlatformInfo().getExternalIds().getPsnAccountId());
    }
    else if (blaze_stricmp(ident, "steamId") == 0)
    {
        blaze_snzprintf(result, sizeof(result), "%" PRId64, userinfo->getPlatformInfo().getExternalIds().getSteamAccountId());
    }
    else if (blaze_stricmp(ident, "personaNamespace") == 0)
    {
        blaze_snzprintf(result, sizeof(result), "%s", userinfo->getPersonaNamespace());
    }
    return result;
}

}   // namespace GameReporting
}   // namespace Blaze
