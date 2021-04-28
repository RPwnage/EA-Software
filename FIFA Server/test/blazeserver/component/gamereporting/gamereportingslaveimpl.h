/*************************************************************************************************/
/*!
    \file   gamereportingslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_SLAVEIMPL_H
#define BLAZE_GAMEREPORTING_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "gamereporting/rpc/gamereportingslave_stub.h"

#include "gamereportcollator.h"
#include "gamereportprocessor.h"
#include "gamereportingmetrics.h"
#include "gametype.h"
#include "gamehistoryconfig.h"
#include "skillconfig.h"
#include "metricsconfig.h"

#include "framework/tdf/attributes.h"
#include "framework/database/dbconn.h"
#include "framework/controller/drainlistener.h"
#include "framework/redis/redis.h"
#include "framework/redis/redistimer.h"
#include "framework/taskscheduler/taskschedulerslaveimpl.h"
#include "framework/storage/storagetable.h"

#include "gamemanager/rpc/gamemanagerslave.h"

#include <EASTL/intrusive_ptr.h>
#include <EASTL/unique_ptr.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

class GameHistoryBootstrap;
class GameReportQuery;
class GameFinishedCommand;
class GameStartedCommand;

typedef GameReportCollator* (*GameReportCollatorCreator)(ReportCollateData& gameReport, GameReportingSlaveImpl& component);
typedef eastl::hash_map<const char8_t*, GameReportCollatorCreator, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > GameReportCollatorRegistryMap;
typedef GameReportProcessor* (*GameReportProcessorCreator)(GameReportingSlaveImpl& component);
typedef eastl::hash_map<const char8_t*, GameReportProcessorCreator, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > GameReportProcessorRegistryMap;

class GameReportMaster
{
public:
    GameReportMaster(GameManager::GameReportingId id, const OwnedSliverRef& ownedSliverRef);
    GameReportMaster(GameManager::GameReportingId id, const ReportCollateDataPtr& reportCollateData, const OwnedSliverRef& ownedSliverRef);

    void setAutoCommitOnRelease() { mAutoCommit = true; }

    ReportCollateData* getData() const { return mReportCollateData.get(); }
    CollatedGameReport& getSavedCollatedReport() const { return *mCollatedGameReport; }

    void saveCollatedReport(const CollatedGameReport& collatedReport);

    GameManager::GameReportingId getGameReportingId() const { return mGameReportingId; }
    void setProcessingStartTimerId(TimerId val) { mProcessingStartTimerId = val; }
    TimerId getProcessingStartTimerId() const { return mProcessingStartTimerId; }

private:
    bool mAutoCommit;
    uint32_t mRefCount;
    OwnedSliverRef mOwnedSliverRef;
    Sliver::AccessRef mOwnedSliverAccessRef;

    GameManager::GameReportingId mGameReportingId;
    ReportCollateDataPtr mReportCollateData;
    TimerId mProcessingStartTimerId;

    CollatedGameReportPtr mCollatedGameReport;

    friend void intrusive_ptr_add_ref(GameReportMaster* ptr);
    friend void intrusive_ptr_release(GameReportMaster* ptr);
};

typedef eastl::intrusive_ptr<GameReportMaster> GameReportMasterPtr;

void intrusive_ptr_add_ref(GameReportMaster* ptr);
void intrusive_ptr_release(GameReportMaster* ptr);

typedef eastl::hash_map<GameManager::GameReportingId, GameReportMasterPtr> GameReportMasterPtrByIdMap;

typedef EA::TDF::tdf_ptr<GameManager::PlayerIdList> PlayerIdListPtr;

class GameReportingSlaveImpl : public GameReportingSlaveStub,
                               public TaskEventHandler,
                               private DrainStatusCheckHandler
{
    friend class GameHistoryBootstrap;

public:
    GameReportingSlaveImpl();
    ~GameReportingSlaveImpl() override;

    FiberJobQueue& getStatsServiceRpcFiberJobQueue()
    {
        return mStatsServiceRpcFiberJobQueue;
    }

    TimeValue& getStatsServiceNextPeriodicLogTime()
    {
        return mStatsServiceNextPeriodicLogTime;
    }

    DbConnPtr getDbConnPtr(const GameManager::GameReportName& gameType) const;
    DbConnPtr getDbReadConnPtr(const GameManager::GameReportName& gameType) const;
    const char8_t* getDbConnNameForGameType(const GameManager::GameReportName& gameType) const;
    const char8_t* getDbSchemaNameForGameType(const GameManager::GameReportName& gameType) const;
    uint32_t getDbIdForGameType(const GameManager::GameReportName& gameType) const;

    UpdateMetricError::Error processUpdateMetric(const Blaze::GameReporting::UpdateMetricRequest &request, const Message* message) override;

    BlazeRpcError getNextGameReportingId(GameManager::GameReportingId& id);

    const GameTypeCollection& getGameTypeCollection() const {
        return mGameTypeCollection;
    }

    const GameReportQueryConfig *getGameReportQueryConfig(const char8_t *name) const;
    const GameReportViewConfig *getGameReportViewConfig(const char8_t *name) const;
    
    //  executed on its own fiber.
    BlazeRpcError saveHistoryReport(ProcessedGameReport *report, const GameType &gameType, 
        const GameManager::PlayerIdList &participantIds, const GameManager::PlayerIdList &submitterIds, const bool isOnline=true ) const;

    const GameReportQueriesCollection& getGameReportQueriesCollection() const {
        return mGameHistoryConfig.getGameReportQueriesCollection();
    }

    const GameReportViewsCollection& getGameReportViewsCollection() const {
        return mGameHistoryConfig.getGameReportViewsCollection();
    }

    const SkillDampingTable *getSkillDampingTable(const char8_t* name)  {
        return mSkillDampingTableCollection->getSkillDampingTable(name);
    }

    int32_t getMaxSkillValue() const
    {
        return getConfig().getSkillInfo().getMaxValue();
    }

    void submitSucceededEvent(const ProcessedGameReport& processedReport, const GameInfo *gameInfo = nullptr, bool isOffline=false) const;

    /*! ************************************************************************************************/
    /*! \brief helper function to submit a event to client if failed to process a game report.

    \param[in] processedReport - the processed game report.
    \param[in] gameInfo - the game report information
    \param[in] isOffline - if the report is an offline report.
    ***************************************************************************************************/
    void submitGameReportProcessFailedEvent(const ProcessedGameReport& processedReport, const GameInfo *gameInfo = nullptr, bool isOffline=false) const;

    void submitGameReportCollationFailedEvent(GameReportsMap& reportsMap, const GameManager::GameReportingId& reportingId) const;
    void submitGameReportOrphanedEvent(const GameInfo& gameInfo, const GameManager::GameReportingId& reportingId) const;

    GameReportingSlaveMetrics& getMetrics() { return mMetrics; }
    GameReportingMetricsCollection& getCustomMetrics() { return mGameReportingMetricsCollection; }

    //    Returns the custom configuration EA::TDF::Tdf object as found in gamereporting.cfg (customGlobalConfig = { tdf="", value={}} )
    const EA::TDF::Tdf* getCustomConfig() const { return getConfig().getCustomGlobalConfig(); }
    //    Returns the GameReportingConfig object representing the current game reporting config.
    const GameReportingConfig& getConfig() const { return GameReportingSlaveStub::getConfig(); }

    bool  isGameManagerRunning() const { return (gController->getComponent(GameManager::GameManagerSlave::COMPONENT_ID, false) != nullptr); }

    //    Trusted Report Processing
    BlazeRpcError scheduleTrustedReportProcess(BlazeId reportOwnerId, ReportType reportType, const SubmitGameReportRequest& submitReportRequest);

    //    Submit a debug game report and gamemanager info.
    BlazeRpcError submitDebugGameReport(const SubmitDebugGameRequest &request);

    //DrainStatusCheckHandler interface
    bool isDrainComplete() override;
    
    const eastl::string populateUserCoreIdentHelper(UserInfoPtr userinfo, const char8_t* ident);

    void registerGameReportCollator(const char8_t* className, GameReportCollatorCreator creator);
    GameReportCollator* createGameReportCollator(const char8_t* className, ReportCollateData& gameReport);
    void registerGameReportProcessor(const char8_t* className, GameReportProcessorCreator creator);
    GameReportProcessor* createGameReportProcessor(const char8_t* className);

    BlazeRpcError collectGameReport(BlazeId reportOwnerId, ReportType reportType, const SubmitGameReportRequest& submitReportRequest);
    void completeCollation(GameReportCollator &collator, GameReportMasterPtr gameReportMaster);

    StorageTable& getReportCollateDataTable() { return mReportCollateDataTable; }
    GameReportMaster* getGameReportMaster(GameManager::GameReportingId id) const;
    const GameReportMaster* getReadOnlyGameReport(GameManager::GameReportingId id) const;
    GameReportMasterPtr getWritableGameReport(GameManager::GameReportingId id, bool autoCommit = true);
    void eraseGameReport(GameManager::GameReportingId id);

    eastl::string getLocalizedString(const char8_t* str, uint32_t locale = 0) const;
private:
    //  component events
    //  startup and shutdown
    void onShutdown() override;
    bool onResolve() override;
    bool onConfigure() override;
    bool onValidateConfig(GameReportingConfig& config, const GameReportingConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;

    //  Reconfigure callbacks
    bool onReconfigure() override;

    // task event handler methods
    void onScheduledTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName) override;
    void onExecutedTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName) override;
    void onCanceledTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName) override;

    //  Component Status (getStatus)
    void getStatusInfo(ComponentStatus& status) const override;

    //  slave notifications
    void onImportStorageRecord(OwnedSliver& ownedSliver, const StorageRecordSnapshot& snapshot);
    void onExportStorageRecord(StorageRecordId recordId);
    void onCommitStorageRecord(StorageRecordId recordId);

    void processReport(CollatedGameReport& data);
    void processGameReportMaster(GameReportMasterPtr gameReportMaster);
    void processCollatedGameReport(CollatedGameReportPtr collatedGameReport);

    void registerCustomGameReportCollators();
    void registerCustomGameReportProcessors();

    void collateTimeoutHandler(GameManager::GameReportingId gameReportingId);

private:
    friend class GameFinishedCommand;
    friend class GameStartedCommand;

    struct AutoIncDec
    {
        AutoIncDec(Metrics::Gauge& gauge) : mGauge(gauge) { mGauge.increment(); }
        ~AutoIncDec() { mGauge.decrement(); }
        Metrics::Gauge& mGauge;
        NON_COPYABLE(AutoIncDec);
    };

    GameReportingSlaveMetrics mMetrics;

    //  Configurable Data
    GameTypeCollection mGameTypeCollection;
    //  Game-specific metrics.
    GameReportingMetricsCollection mGameReportingMetricsCollection;

    FiberJobQueue mProcessReportsFiberJobQueue;
    FiberJobQueue mStatsServiceRpcFiberJobQueue; // queue that completes RPCs sent to the Stats Service

    TimeValue mStatsServiceNextPeriodicLogTime;

    StorageTable mReportCollateDataTable;
    GameReportMasterPtrByIdMap mGameReportMasterPtrByIdMap;

    GameReportCollatorRegistryMap mGameReportCollatorRegistryMap;
    GameReportProcessorRegistryMap mGameReportProcessorRegistryMap;

private:
    /////////////////////////////////////  Game History Specific  /////////////////////////////////////
    typedef eastl::hash_map<uint64_t, uint32_t> GameRefCountMap;

    void runBootstrap(BlazeRpcError& err);
    void scheduleGameHistoryPurge();
    void doGameHistoryPurge();

    void insertIntoHistoryTable(QueryPtr& query, const GameType &gameType,
        const char8_t* tableName, const HistoryTableDefinition *historyTableDefinition,
        const Collections::AttributeMap& attributeMap, const GameHistoryId& gameReportingId) const;

    uint32_t insertAssignments(QueryPtr& query, const HistoryTableDefinition *historyTableDefinition,
        const Collections::AttributeMap& attributeMap, bool onlyPrimaryKey, bool separateByAnd = false) const;

    BlazeRpcError updateGameRefTable(DbConnPtr& conn, QueryPtr& query, const GameType &gameType, 
        const char *tableName, const HistoryTableDefinition *historyTableDefinition,
        const Collections::AttributeMap& attributeMap, const GameHistoryId& gameReportingId, GameRefCountMap &map) const;

    void insertIntoGameRefTable(QueryPtr& query, const char8_t* tableName, 
        const HistoryTableDefinition *historyTableDefinition,
        const Collections::AttributeMap& attributeMap, const GameHistoryId& gameReportingId,
        uint32_t limit, GameRefCountMap &map, DbResultPtr& result) const;

    void insertIntoBuiltInGameTable(DbConnPtr& conn, QueryPtr& query, const GameType &gameType,
        const GameHistoryId& gameReportingId, const ReportFlag &flag, const char8_t* flagReason, const bool isOnline=true) const;

    void insertIntoBuiltInTable(QueryPtr& query, const GameType &gameType, const char8_t* builtInTableSuffix,
        const GameHistoryId& gameReportingId, const GameManager::PlayerIdList &playerIds) const;

    void updateGameRef(QueryPtr& query, const GameType &gameType, const GameHistoryId& gameReportingId, uint32_t totalRef, GameRefCountMap &map) const;

    //  Kept for 2.x compatibility until further notice.
    SkillDampingTableCollection *mSkillDampingTableCollection;

    GameHistoryConfigParser mGameHistoryConfig;

    // game history purge
    TaskId mPurgeTaskId;
    bool mHighFrequencyPurge;

    TimeValue mDrainDelayPeriod;
    bool mGameReportInGameEndPinEvent;
};

extern EA_THREAD_LOCAL GameReportingSlaveImpl* gGameReportingSlave;

extern const char8_t PUBLIC_GAME_REPORT_FIELD[];
extern const char8_t PRIVATE_GAME_REPORT_FIELD[];
extern const char8_t* VALID_GAME_REPORT_FIELD_PREFIXES[];

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_GAMEREPORTING_SLAVEIMPL_H
