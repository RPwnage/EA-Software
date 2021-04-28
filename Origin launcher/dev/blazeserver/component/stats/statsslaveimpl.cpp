/*************************************************************************************************/
/*!
    \file   statsslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class StatsSlaveImpl

    Implements the slave portion of the stats component.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbconfig.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/database/dbscheduler.h"
#include "framework/connection/selector.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmaster.h"
#include "framework/util/localization.h"

#include "time.h"

#include "stats/statscomponentinterface.h"
#include "stats/statscache.h"
#include "stats/statsslaveimpl.h"
#include "stats/leaderboardindex.h"
#include "stats/rpc/statsmaster.h"
#include "stats/tdf/stats.h"
#include "stats/statsconfig.h"
#include "stats/globalstatscache.h"
#include "stats/globalstatsprovider.h"
#include "stats/statsextendeddataprovider.h"

#include "rollover.h"
#include "userstats.h"
#include "userranks.h"


namespace Blaze
{
namespace Stats
{


/*** Public Methods ******************************************************************************/
// static
StatsSlave* StatsSlave::createImpl()
{
    return BLAZE_NEW_NAMED("StatsSlaveImpl") StatsSlaveImpl();
}
/*************************************************************************************************/
/*!
    \brief StatsSlaveImpl

    Constructor for the StatsSlaveImpl object that owns the core state of the stats and
    leaderboard slave component.
*/
/*************************************************************************************************/
StatsSlaveImpl::StatsSlaveImpl() 
    : mStatsConfig(nullptr),
    mDbId(DbScheduler::INVALID_DB_ID),
    mLeaderboardIndexes(nullptr),
    mStatsCache(nullptr),
    mPopulated(false),
    mUpdateStatsCount(getMetricsCollection(), "updateStatsRequests"),
    mUpdateStatsFailedCount(getMetricsCollection(), "updateStatsRequestsFailed"),
    mStatUpdateTimerId(INVALID_TIMER_ID),
    mGlobalStatsCache(nullptr),
    mLastGlobalCacheInstructionWasWrite(false)
{
    mStatsExtendedDataProvider = BLAZE_NEW StatsExtendedDataProvider(*this);
}

/*************************************************************************************************/
/*!
    \brief ~StatsSlaveImpl

    Destroys the slave stats and leaderboard component.
*/
/*************************************************************************************************/
StatsSlaveImpl::~StatsSlaveImpl()
{
    gUserSessionManager->removeSubscriber(*this);

    delete mStatsCache;
    delete mGlobalStatsCache;
    delete mStatsExtendedDataProvider;
    delete mLeaderboardIndexes;
    delete mStatsConfig;
}


/*************************************************************************************************/
/*!
    \brief onValidateConfig

    Validate the configure data for the stats slave components.

    \param[in]  config - the configure data
    \param[in]  referenceConfigPtr - the former configure data for reconfiguration
    \param[out]  validationErrors - the error messages of validation
    \return - true if successful, false otherwise
*/
/*************************************************************************************************/
bool StatsSlaveImpl::onValidateConfig(StatsConfig& config, const StatsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    // the master would have already validated the config before sending to this slave
    return validationErrors.getErrorMessages().empty();
}

/*************************************************************************************************/
/*!
    \brief onConfigure

    Configure the stats slave component with it's component config obtained from the slave
    properties file.

    \return - true if successful, false otherwise
*/
/*************************************************************************************************/
bool StatsSlaveImpl::onConfigure()
{
    const StatsConfig& config = getConfig();
  
    TRACE_LOG("[StatsSlaveImpl].onConfigure(): parsing config file.");

    const char8_t* dbName = config.getDbName();
    if (dbName[0] == '\0')
    {
        ERR_LOG("[StatsSlaveImpl].onConfigure(): No DB configuration detected");
        return false;
    }        

    mDbId = gDbScheduler->getDbId(dbName);
    if (mDbId == DbScheduler::INVALID_DB_ID)
    {
        ERR_LOG("[StatsSlaveImpl].onConfigure() - cannot init db <" << dbName << ">.");
        return false;
    }

    mStatsConfig = BLAZE_NEW StatsConfigData();
    mStatsConfig->parseStatsConfig(config);
    if (!mStatsConfig->initShards())
    {
        ERR_LOG("[StatsSlaveImpl].onConfigure() - cannot init shards.");
        return false;
    }

    // subscribe after reading the config because the callbacks use the config data
    gUserSessionManager->addSubscriber(*this);

    // User stats
    if (!mStatsExtendedDataProvider->parseExtendedData(config, mStatsConfig))
    {
        ERR_LOG("[StatsSlaveImpl].onConfigure() - could not parse extended data.");
        return false;
    }
    gUserSessionManager->registerExtendedDataProvider(COMPONENT_ID, *mStatsExtendedDataProvider);

    // create stats-level cache
    mStatsCache = BLAZE_NEW StatsCache(*this, (size_t)mStatsConfig->getLruSize());

    // create global stats cache
    mGlobalStatsCache = BLAZE_NEW GlobalStatsCache(*this);

    const RolloverData& rolloverData = config.getRollover();

    mDebugTimer.useTimer = rolloverData.getDebugUseTimers();
    mDebugTimer.dailyTimer = rolloverData.getDebugDailyTimer();
    mDebugTimer.weeklyTimer = rolloverData.getDebugWeeklyTimer();
    mDebugTimer.monthlyTimer = rolloverData.getDebugMonthlyTimer();

    printMemBudget();
    
    return true;
}
/*************************************************************************************************/
/*!
    \brief onReconfigure

    Reconfigure the stats slave component with it's component config obtained from the slave
    properties file.

    \return - true if successful, false otherwise
*/
/*************************************************************************************************/
bool StatsSlaveImpl::onReconfigure()
{
    INFO_LOG("[StatsSlaveImpl].onReconfigure(): start to reconfigure.");

    // update stats cache size only if the new value is different
    uint32_t cacheSize = (uint32_t) mStatsCache->getLruMaxSize();
    if (cacheSize != getConfig().getLruSize())
    {
        mStatsCache->resetAndResize((size_t)getConfig().getLruSize());
    }

    mStatsConfig->setStatsConfig(getConfig());

    return true;
}

/*************************************************************************************************/
/*!
    \brief onResolve
    Resolve the master
    \param[in]  - none
*/
/*************************************************************************************************/
bool StatsSlaveImpl::onResolve()
{
    getMaster()->addNotificationListener(*this);

    gController->addRemoteInstanceListener(*this);

    if (gUserSessionMaster != nullptr)
        gUserSessionMaster->addLocalUserSessionSubscriber(*this);

    if (!initMasterServer())
    {
        return false;
    }

    mLeaderboardIndexes = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndexes(this, getMetricsCollection(), isMaster());
    mLeaderboardIndexes->buildIndexesStructures();
    mPopulated = mLeaderboardIndexes->requestIndexesFromMaster(getMaster());
    if (mPopulated)
    {
        for (UpdateNotificationList::const_iterator it = mBufferedUpdates.begin(); it != mBufferedUpdates.end(); ++it)
            handleStatUpdate(*it);
    }
    mBufferedUpdates.clear();
    return mPopulated;
}

void StatsSlaveImpl::onRemoteInstanceChanged(const RemoteServerInstance& instance)
{
    if (instance.hasComponent(StatsMaster::COMPONENT_ID))
    {
        if (instance.isActive())
        {
            INFO_LOG("[StatsSlaveImpl].onRemoteInstanceChanged now tracking new stats master with instance id (" << instance.getInstanceId() << ").");

            //kick off our init in a seperate fiber to avoid blocking the connection fiber.
            Fiber::CreateParams fiberParams;
            fiberParams.blocking = true;
            fiberParams.namedContext = "StatsSlaveImpl::initMasterServer";
            gFiberManager->scheduleCall(this, &StatsSlaveImpl::initMasterServer, (bool*) nullptr, fiberParams);

        }
        else if(!instance.isConnected())
        {
            WARN_LOG("[StatsSlaveImpl].onRemoteInstanceChanged lost connection to master with id (" << instance.getInstanceId() << ")");
        }
    }
}

bool StatsSlaveImpl::isDrainComplete()
{
    return (getPendingTransactionsCount() == 0);
}

bool StatsSlaveImpl::initMasterServer()
{
    // There are custom components that register themselves as notification listeners, but
    // we don't want them handling global cache instructions, so we explicitly identify ourselves
    // as a stats slave by making this call
    InitializeGlobalCacheRequest initializeGlobalCacheRequest;
    initializeGlobalCacheRequest.setInstanceId(gController->getInstanceId());
    if (getMaster()->initializeGlobalCache(initializeGlobalCacheRequest) != ERR_OK)
    {
        ERR_LOG("[StatsSlaveImpl].initMasterServer: error initializing global cache.");
        return false;
    }

    if (getMaster()->getPeriodIdsMaster(mPeriodIds) != ERR_OK)
    {
        ERR_LOG("[StatsSlaveImpl].initMasterServer: error fetching period ids.");
        return false;
    }

    return true;
}



/*************************************************************************************************/
/*!
    \brief onUpdateCacheNotification
    Received when stats have been updated.
    \param[in]  data - update data (tdf)
*/
/*************************************************************************************************/
void StatsSlaveImpl::onUpdateCacheNotification(const StatUpdateNotification& data, UserSession *)
{
    if (EA_UNLIKELY(!mPopulated))
    {
        data.copyInto(mBufferedUpdates.push_back());
        return;
    }

    handleStatUpdate(data);
}

void StatsSlaveImpl::onUpdateGlobalStatsNotification(const UpdateStatsRequest& data, UserSession *)
{
    TRACE_LOG("[StatsSlaveImpl].onUpdateGlobalStatsNotification");

    // We're passing the TDF parameter (data) by value to the executeGlobalStatsUpdate() method to avoid
    // an allocation.  The data will be copied anyway--whether onto the stack or to allocated heap memory.
    // While with passing-by-value, there will be 2 copies (one by MethodCall1Job when creating the job in
    // scheduleFiberCall, and another when calling the executeGlobalStatsUpdate method), this might still
    // be more efficient than an allocate and single copy.

    // do this on fiber because it may block if the cache is not initialized
    gSelector->scheduleFiberCall<StatsSlaveImpl, UpdateStatsRequest>(this,
        &StatsSlaveImpl::executeGlobalStatsUpdate, data, "StatsSlaveImpl::executeGlobalStatsUpdate");
}

void StatsSlaveImpl::executeGlobalStatsUpdate(const UpdateStatsRequest request)
{
    UserSession::SuperUserPermissionAutoPtr superUserPermission(true);

    InitializeStatsTransactionRequest req;
    InitializeStatsTransactionResponse res;
    request.getStatUpdates().copyInto(req.getStatUpdates());
    req.setStrict(true);
    req.setGlobalStats(true);

    BlazeRpcError rc = initializeStatsTransaction(req, res);
    if (rc != ERR_OK)
    {
        WARN_LOG("[StatsSlaveImpl].executeGlobalStatsUpdate failed to initialize transaction " << ErrorHelp::getErrorName(rc));
        return;
    }

    CommitTransactionRequest commitReq;
    commitReq.setTransactionContextId(res.getTransactionContextId());

    rc = commitStatsTransaction(commitReq);
    if (rc != ERR_OK)
    {
        WARN_LOG("[StatsSlaveImpl].executeGlobalStatsUpdate failed to commit transaction " << ErrorHelp::getErrorName(rc));
    }
}

/*************************************************************************************************/
/*!
    \brief handleStatUpdate

    Process a stat update/delete (based on type).

    \param[in]  update - update data (tdf)
*/
/*************************************************************************************************/
void StatsSlaveImpl::handleStatUpdate(const StatUpdateNotification& update)
{
    TRACE_LOG("[StatsSlaveImpl].handleStatUpdate(" << update.getBroadcast().getActiveMember() << ")");

    switch (update.getBroadcast().getActiveMember())
    {
    case StatBroadcast::MEMBER_STATUPDATE:
        {
            const StatUpdateBroadcast* data = update.getBroadcast().getStatUpdate();
            if (data == nullptr)
            {
                ERR_LOG("[StatsSlaveImpl].handleStatUpdate(" << update.getBroadcast().getActiveMember() << "): missing data");
                break;
            }

            // update stats cache
            if (mStatsCache->isLruEnabled())
            {
                mStatsCache->deleteRows(data->getCacheUpdates());
            }

            // Update leaderboard indexes
            mLeaderboardIndexes->updateStats(data->getCacheUpdates());
        }
        break;

    case StatBroadcast::MEMBER_STATDELETEBYCATEGORY:
        {
            const StatDeleteByCategoryBroadcast* data = update.getBroadcast().getStatDeleteByCategory();
            if (data == nullptr)
            {
                ERR_LOG("[StatsSlaveImpl].handleStatUpdate(" << update.getBroadcast().getActiveMember() << "): missing data");
                break;
            }

            // invalidate stats-level cache
            mStatsCache->reset();

            // Delete from the leaderboard indexes
            mLeaderboardIndexes->deleteCategoryStats(data->getCacheDeletes(), data->getCurrentPeriodIds());
        }
        break;

    case StatBroadcast::MEMBER_STATDELETEANDUPDATE:
        {
            const StatDeleteAndUpdateBroadcast* data = update.getBroadcast().getStatDeleteAndUpdate();
            if (data == nullptr)
            {
                ERR_LOG("[StatsSlaveImpl].handleStatUpdate(" << update.getBroadcast().getActiveMember() << "): missing data");
                break;
            }

            // update stats cache
            if (mStatsCache->isLruEnabled())
            {
                mStatsCache->deleteRows(data->getCacheUpdates());
                mStatsCache->deleteRows(data->getCacheDeletes(), data->getCurrentPeriodIds());
            }

            // delete first
            mLeaderboardIndexes->deleteStats(data->getCacheDeletes(), data->getCurrentPeriodIds(), nullptr);
            // then re-add
            mLeaderboardIndexes->updateStats(data->getCacheUpdates());
        }
        break;

    case StatBroadcast::MEMBER_STATDELETEBYENTITY:
        {
            const StatDeleteByEntityBroadcast* data = update.getBroadcast().getStatDeleteByEntity();
            if (data == nullptr)
            {
                ERR_LOG("[StatsSlaveImpl].handleStatUpdate(" << update.getBroadcast().getActiveMember() << "): missing data.");
                break;
            }

            // actual broadcast doesn't contain any keyscopes (which won't properly update the stats cache),
            // so the easiest thing to do for now is to just invalidate the stats cache.
            mStatsCache->reset();

            mLeaderboardIndexes->deleteStatsByEntity(data->getCacheDeletes(), data->getCurrentPeriodIds(), nullptr);
        }
        break;

    case StatBroadcast::MEMBER_STATCLEARCACHE:
        {
            mStatsCache->reset();
        }
        break;

    default:
        ERR_LOG("[StatsSlaveImpl].handleStatUpdate(" << update.getBroadcast().getActiveMember() << "): unknown data");
        break;
    }
}

void StatsSlaveImpl::onShutdown()
{
    if (gUserSessionMaster != nullptr)
        gUserSessionMaster->removeLocalUserSessionSubscriber(*this);

    gController->removeRemoteInstanceListener(*this);

    if (mLastGlobalCacheInstructionWasWrite)
    {
        // this slave was the last to write global cache to db
        // so it should be the one to write to db on shutdown

        size_t countOfRowsWritten = 0;

        mGlobalStatsCache->serializeToDatabase(
            STATS_GLOBAL_CACHE_FORCE_WRITE, 
            countOfRowsWritten);
    }

    if (mStatsExtendedDataProvider)
    {
        gUserSessionManager->deregisterExtendedDataProvider(COMPONENT_ID);
    }
}

/*************************************************************************************************/
/*!
    \brief getPeriodId

    Returns current period ID for given period.

    \param[in]  periodType - the period type to get current period ID for

    \return - period ID
*/
/*************************************************************************************************/
int32_t StatsSlaveImpl::getPeriodId(int32_t periodType)
{
    switch (periodType)
    {
        case STAT_PERIOD_DAILY:
            return mPeriodIds.getCurrentDailyPeriodId();
        case STAT_PERIOD_WEEKLY:
            return mPeriodIds.getCurrentWeeklyPeriodId();
        case STAT_PERIOD_MONTHLY:
            return mPeriodIds.getCurrentMonthlyPeriodId();
        case STAT_PERIOD_ALL_TIME:
            return 0;
        default:
            return 0;
    }
}        

/*************************************************************************************************/
/*!
    \brief getPeriodId
    Returns current period ID fro given period
    \param[in]  - period - period
    \return - period ID
*/
/*************************************************************************************************/
void StatsSlaveImpl::getPeriodIds(int32_t periodIds[STAT_NUM_PERIODS])
{
    periodIds[STAT_PERIOD_DAILY] =  mPeriodIds.getCurrentDailyPeriodId();
    periodIds[STAT_PERIOD_WEEKLY] = mPeriodIds.getCurrentWeeklyPeriodId();
    periodIds[STAT_PERIOD_MONTHLY] = mPeriodIds.getCurrentMonthlyPeriodId();
    periodIds[STAT_PERIOD_ALL_TIME] = 0;
}

/*************************************************************************************************/
void StatsSlaveImpl::onTrimPeriodNotification(const StatPeriod& data, UserSession *)
{
    INFO_LOG("[StatsSlaveImpl].onTrimPeriodNotification.  Period type: " << data.getPeriod() << "  retention: " << data.getRetention());

    mLeaderboardIndexes->scheduleTrimPeriods(data.getPeriod(), data.getRetention());
}

/*************************************************************************************************/
void StatsSlaveImpl::onSetPeriodIdsNotification(const PeriodIds& data, UserSession *)
{
    data.copyInto(mPeriodIds);
    INFO_LOG("[StatsSlaveImpl].onSetPeriodIdsNotification.  daily: " << data.getCurrentDailyPeriodId() << "  weekly: " 
             << data.getCurrentWeeklyPeriodId() << " monthly: " << data.getCurrentMonthlyPeriodId());
}

/*************************************************************************************************/
bool StatsSlaveImpl::isKeyScopeChangeable(const char8_t* keyscope) const
{
    KeyScopeChangeMap::const_iterator iter = getConfig().getKeyScopeChangeMap().find(keyscope);
    if(iter == getConfig().getKeyScopeChangeMap().end() || iter->second == false)
    {
        return false;
    }
    return true;
}

/*************************************************************************************************/
/*!
    \brief getRetention

    Returns the number of periods to retain for a given period type

    \param[in]  periodType - the period type to check the retention for

    \return - number of periods to retain for the given period type
*/
/*************************************************************************************************/
int32_t StatsSlaveImpl::getRetention(int32_t periodType)
{
    switch (periodType)
    {
        case STAT_PERIOD_DAILY:
            return mPeriodIds.getDailyRetention();
        case STAT_PERIOD_WEEKLY:
            return mPeriodIds.getWeeklyRetention();
        case STAT_PERIOD_MONTHLY:
            return mPeriodIds.getMonthlyRetention();
        default:
            return 0;
    }            
}

GetStatCategoryListError::Error StatsSlaveImpl::processGetStatCategoryList(
    Blaze::Stats::StatCategoryList &response, const Message *message)
{
    TRACE_LOG("[StatsSlaveImpl].processGetStatCategoryList() - retrieving stat categories");
    CategoryMap::const_iterator catItr = getConfigData()->getCategoryMap()->begin();
    CategoryMap::const_iterator catEnd = getConfigData()->getCategoryMap()->end();
    response.getCategories().reserve(getConfigData()->getCategoryMap()->size());
    for (; catItr != catEnd; ++catItr)
    {
        StatCategorySummary* catSummary = response.getCategories().pull_back();
        const StatCategory* cat = catItr->second;

        catSummary->setName(catItr->first);
        catSummary->setEntityType(cat->getCategoryEntityType());
        catSummary->setDesc(cat->getDesc());
        catSummary->setCategoryType(cat->getCategoryType());
        catSummary->getPeriodTypes().reserve(STAT_NUM_PERIODS);

        for (int32_t periodType = 0; periodType < STAT_NUM_PERIODS; ++periodType)
        {
            if (cat->isValidPeriod(periodType))
            {
                catSummary->getPeriodTypes().push_back(static_cast<StatPeriodType>(periodType));
            }
        }

        if (cat->hasScope())
        {
            ScopeNameSet::const_iterator scopeItr = cat->getScopeNameSet()->begin();
            ScopeNameSet::const_iterator scopeEnd = cat->getScopeNameSet()->end();
            catSummary->getKeyScopes().reserve(cat->getScopeNameSet()->size());
            for (; scopeItr != scopeEnd; ++scopeItr)
            {
                catSummary->getKeyScopes().push_back((*scopeItr).c_str());
            }
        }

    }

    return GetStatCategoryListError::ERR_OK;
}

GetPeriodIdsError::Error StatsSlaveImpl::processGetPeriodIds(PeriodIds &response, const Message *message)
{
    mPeriodIds.copyInto(response);
    return GetPeriodIdsError::ERR_OK;
}

GetStatDescsError::Error StatsSlaveImpl::processGetStatDescs(const Blaze::Stats::GetStatDescsRequest &request,
                                                             Blaze::Stats::StatDescs &response, const Message *message)
{
    TRACE_LOG("[StatsSlaveImpl].processGetStatDescs() - retrieving stat descs.");
    uint32_t locale = 0;
    if (message != nullptr)
    {
        locale = message->getLocale();
    }
    else if (gCurrentUserSession != nullptr)
    {
        locale = gCurrentUserSession->getSessionLocale();
    }

    CategoryMap::const_iterator catIter =
        getConfigData()->getCategoryMap()->find(request.getCategory());
    if (catIter == getConfigData()->getCategoryMap()->end())
    {
        TRACE_LOG("[StatsSlaveImpl].processGetStatDescs() - requested nonexistent category " << request.getCategory());
        return GetStatDescsError::STATS_ERR_UNKNOWN_CATEGORY;
    }

    const StatCategory* cat = catIter->second;

    response.setEntityType(cat->getCategoryEntityType());

    if (request.getStatNames().size() == 0)
    {
        // send back all the stats according to the config
        StatList::const_iterator statItr = cat->getStats().begin();
        StatList::const_iterator statEnd = cat->getStats().end();
        response.getStatDescs().reserve(cat->getStats().size());

        for (; statItr != statEnd; ++statItr)
        {
            const Stat* stat = *statItr;

            StatDescSummary* summary = response.getStatDescs().pull_back();
            summary->setName(stat->getName());
            summary->setShortDesc(gLocalization->localize(stat->getShortDesc(), locale));
            summary->setLongDesc(gLocalization->localize(stat->getLongDesc(), locale));
            summary->setType(stat->getDbType().getType());
            summary->setDefaultValue(stat->getDefaultValAsString());
            summary->setFormat(stat->getStatFormat());
            summary->setDerived(stat->isDerived());         
        }

        return GetStatDescsError::ERR_OK;
    }

    GetStatDescsRequest::StringStatNameList::const_iterator nameIter = request.getStatNames().begin();
    GetStatDescsRequest::StringStatNameList::const_iterator endIter = request.getStatNames().end();
    response.getStatDescs().reserve(request.getStatNames().size());

    for (;nameIter != endIter; ++nameIter)
    {
        const char8_t* statName = nameIter->c_str() ;
        

        StatMap::const_iterator statIter = cat->getStatsMap()->find(statName);
        if (statIter == cat->getStatsMap()->end())
        {
            TRACE_LOG("[StatsSlaveImpl].processGetStatDescs() - Failed to locate stat " << statName << " in category");
            return GetStatDescsError::STATS_ERR_STAT_NOT_FOUND;
        }

        const Stat* stat = statIter->second;

        StatDescSummary* summary = response.getStatDescs().pull_back();
        summary->setName(stat->getName());
        summary->setShortDesc(gLocalization->localize(stat->getShortDesc(), locale));
        summary->setLongDesc(gLocalization->localize(stat->getLongDesc(), locale));
        summary->setType(stat->getDbType().getType());
        summary->setDefaultValue(stat->getDefaultValAsString());
        summary->setFormat(stat->getStatFormat());
        summary->setDerived(stat->isDerived());
    }

    return GetStatDescsError::ERR_OK;
}

GetStatGroupListError::Error StatsSlaveImpl::processGetStatGroupList(
    Blaze::Stats::StatGroupList &response, const Message *message)
{
    TRACE_LOG("[StatsSlaveImpl].processGetStatGroupList() - retrieving statgroup list.");
    uint32_t locale = 0;
    if (message != nullptr)
    {
        locale = message->getLocale();
    }
    else if (gCurrentUserSession != nullptr)
    {
        locale = gCurrentUserSession->getSessionLocale();
    }

    const StatGroupMap* groupMap = getConfigData()->getStatGroupMap();

    // order the groups in the response according to the config file
    StatGroupsList::const_iterator groupIter = getConfigData()->getStatGroupList().begin();
    StatGroupsList::const_iterator groupEnd = getConfigData()->getStatGroupList().end();
    response.getGroups().reserve(getConfigData()->getStatGroupList().size());
    for (; groupIter != groupEnd; ++groupIter)
    {
        const StatGroup* group = *groupIter;
        StatGroupSummary* groupSummary = response.getGroups().pull_back();
        groupSummary->setName(group->getName());

        // the keyscope info comes from the group map (not the loaded config data)
        StatGroupMap::const_iterator mapItr = groupMap->find(group->getName());
        if (mapItr == groupMap->end())
        {
            WARN_LOG("[StatsSlaveImpl].processGetStatGroupList: missing stat group from map: " << group->getName());
        }
        else
        {
            // scope name/unit pairs defined for the group
            const ScopeNameValueMap * groupScopeNameValueMap = mapItr->second->getScopeNameValueMap();
            if (groupScopeNameValueMap)
            {
                groupSummary->getKeyScopeNameValueMap().assign(groupScopeNameValueMap->begin(), groupScopeNameValueMap->end());
            }
        }

        groupSummary->setDesc(gLocalization->localize(group->getDesc(), locale));
        groupSummary->setMetadata(group->getMetadata());
    }
    
    return GetStatGroupListError::ERR_OK;
}


GetStatGroupError::Error StatsSlaveImpl::processGetStatGroup(const Blaze::Stats::GetStatGroupRequest &request,
                                                             Blaze::Stats::StatGroupResponse &response, const Message *message)
{
    TRACE_LOG("[StatsSlaveImpl].processGetStatGroup() - retrieving statgroup: " << request.getName());
    uint32_t locale = 0;
    if (message != nullptr)
    {
        locale = message->getLocale();
    }
    else if (gCurrentUserSession != nullptr)
    {
        locale = gCurrentUserSession->getSessionLocale();
    }

    StatGroupMap::const_iterator iter = getConfigData()->getStatGroupMap()->find(request.getName());
    if (iter != getConfigData()->getStatGroupMap()->end())
    {
        const StatGroup* group = iter->second;

        response.setName(group->getName());
        response.setDesc(gLocalization->localize(group->getDesc(), locale));
        response.setCategoryName(group->getCategory());
        response.setEntityType(group->getStatCategory()->getCategoryEntityType());
        response.setMetadata(group->getMetadata());
        if (group->getScopeNameValueMap() != nullptr)
        {
            group->getScopeNameValueMap()->copyInto(response.getKeyScopeNameValueMap());
        }
        StatDescList::const_iterator descIter = group->getStats().begin();
        StatDescList::const_iterator descEnd = group->getStats().end();
        response.getStatDescs().reserve(group->getStats().size());
        while (descIter != descEnd)
        {
            const StatDesc* desc = *descIter++;
            StatDescSummary* summary = response.getStatDescs().pull_back();
            summary->setName(desc->getStat()->getName());
            summary->setShortDesc(gLocalization->localize(desc->getShortDesc(), locale));
            summary->setLongDesc(gLocalization->localize(desc->getLongDesc(), locale));
            summary->setType(desc->getType());
            summary->setDefaultValue(desc->getStat()->getDefaultValAsString());
            summary->setFormat(desc->getStatFormat());
            summary->setKind(desc->getKind());
            summary->setMetadata(desc->getMetadata());
            summary->setCategory(desc->getStat()->getCategory().getName());
            summary->setDerived(desc->getStat()->isDerived());
        }

        return GetStatGroupError::ERR_OK;
    }
    else
        return GetStatGroupError::STATS_ERR_UNKNOWN_STAT_GROUP;
}

GetDateRangeError::Error StatsSlaveImpl::processGetDateRange(const Blaze::Stats::GetDateRangeRequest &request,
                                                             Blaze::Stats::DateRange &response, const Message *message)
{
    TRACE_LOG("[StatsSlaveImpl].processGetDateRange() - retrieving date range.");

    CategoryMap::const_iterator iter =
        getConfigData()->getCategoryMap()->find(request.getCategory());
    if (iter == getConfigData()->getCategoryMap()->end())
    {
        TRACE_LOG("[StatsSlaveImpl].processGetDateRange() - category " << request.getCategory() << " not found");
        return GetDateRangeError::STATS_ERR_UNKNOWN_CATEGORY;
    }

    TRACE_LOG("[StatsSlaveImpl].processGetDateRange() period: " << request.getPeriodType() << "  offset: "
              << request.getPeriodOffset() << "  found category " << request.getCategory());

    int32_t tStart = 0;
    int32_t tEnd = 0;
    struct tm tM;

    switch (request.getPeriodType())
    {
    case STAT_PERIOD_ALL_TIME:
        break;

    case STAT_PERIOD_MONTHLY:
        if (mDebugTimer.useTimer)
        {
            int32_t monthlyId = getPeriodData()->getCurrentMonthlyPeriodId();
            tStart = monthlyId * static_cast<int32_t>(mDebugTimer.monthlyTimer);
            tEnd = tStart + (int32_t)mDebugTimer.monthlyTimer;
        }
        else
        {
            int32_t monthlyId = request.getPeriodId();
            if (monthlyId == 0)
            {
                monthlyId = getPeriodData()->getCurrentMonthlyPeriodId();
                monthlyId -= request.getPeriodOffset();
            }
            --monthlyId; // period id ends with rollover time, so -1 to get the start time of the period
            tM.tm_year = 70 + monthlyId / 12; // epoch start is 1970, but tm_year is since 1900
            tM.tm_mon = monthlyId % 12;
            tM.tm_mday = getPeriodData()->getMonthlyDay();
            tM.tm_hour = getPeriodData()->getMonthlyHour();
            tM.tm_min = 0;
            tM.tm_sec = 0;
            tStart = static_cast<int32_t>(Rollover::mkgmTime(&tM));
            ++tM.tm_mon;
            if (tM.tm_mon > 11)
            {
                tM.tm_mon -= 12;
                ++tM.tm_year;
            }
            tEnd = static_cast<int32_t>(Rollover::mkgmTime(&tM));
        }
        break;
    case STAT_PERIOD_WEEKLY:
        if (mDebugTimer.useTimer)
        {
            int32_t weeklyId = getPeriodData()->getCurrentWeeklyPeriodId();
            tStart = weeklyId * static_cast<int32_t>(mDebugTimer.weeklyTimer);
            tEnd = tStart + (int32_t)mDebugTimer.weeklyTimer;
        }
        else
        {
            int32_t weeklyId = request.getPeriodId();
            if (weeklyId == 0)
            {
                weeklyId = getPeriodData()->getCurrentWeeklyPeriodId();
                weeklyId -= request.getPeriodOffset();
            }
            --weeklyId; // period id ends with rollover time, so -1 to get the start time of the period
            int64_t t = (3600 * 24 * 7) * (int64_t)weeklyId;
            Rollover::gmTime(&tM, t);
            tM.tm_mday += getPeriodData()->getWeeklyDay() - tM.tm_wday;
            tM.tm_hour = getPeriodData()->getWeeklyHour();
            tM.tm_min = 0;
            tM.tm_sec = 0;
            tStart = static_cast<int32_t>(Rollover::mkgmTime(&tM));
            tEnd = tStart + (3600 * 24 * 7);
        }
        break;
    case STAT_PERIOD_DAILY:
        if (mDebugTimer.useTimer)
        {
            int32_t dailyId = getPeriodData()->getCurrentDailyPeriodId();
            tStart = dailyId * static_cast<int32_t>(mDebugTimer.dailyTimer);
            tEnd = tStart + (int32_t)mDebugTimer.dailyTimer;
        }
        else
        {
            int32_t dailyId = request.getPeriodId();
            if (dailyId == 0)
            {
                dailyId = getPeriodData()->getCurrentDailyPeriodId();
                dailyId -= request.getPeriodOffset();
            }
            --dailyId; // period id ends with rollover time, so -1 to get the start time of the period
            int64_t t = 3600 * 24 * (int64_t)dailyId;
            Rollover::gmTime(&tM, t);
            tM.tm_hour = getPeriodData()->getDailyHour();
            tM.tm_min = 0;
            tM.tm_sec = 0;
            tStart = static_cast<int32_t>(Rollover::mkgmTime(&tM));
            tEnd = tStart + 24 * 3600;
        }
        break;
    default:
        break;        
    }

    response.setStart(tStart);
    response.setEnd(tEnd);

    Rollover::gmTime(&tM, (int64_t)tStart);
    eastl::string timeBuf;
    timeBuf.sprintf("%2d:%02d:%02d", tM.tm_hour, tM.tm_min, tM.tm_sec);
    TRACE_LOG("[StatsSlaveImpl].processGetDateRange(): period: " << request.getPeriodType() << "  offset: " 
              << request.getPeriodOffset() << " Start: " << (tM.tm_year + 1900) << "/" << (tM.tm_mon + 1) << "/" << tM.tm_mday 
              << " (" << tM.tm_wday << ") " << timeBuf.c_str() << " GMT");

    Rollover::gmTime(&tM, (int64_t)tEnd);
    timeBuf.sprintf("%2d:%02d:%02d", tM.tm_hour, tM.tm_min, tM.tm_sec);
    TRACE_LOG("[StatsSlaveImpl].processGetDateRange(): period: " << request.getPeriodType() << "  offset: " 
              << request.getPeriodOffset() << " End: " << (tM.tm_year + 1900) << "/" << (tM.tm_mon + 1) << "/" << tM.tm_mday << " (" 
              << tM.tm_wday << ") " << timeBuf.c_str() << " GMT");

    return GetDateRangeError::ERR_OK;
}


GetLeaderboardGroupError::Error StatsSlaveImpl::processGetLeaderboardGroup(const Blaze::Stats::LeaderboardGroupRequest &request,
    Blaze::Stats::LeaderboardGroupResponse &response, const Message *message)
{
    TRACE_LOG("[StatsSlaveImpl].processGetLeaderboardGroup() - board name: " << request.getBoardName() << " id: " 
              << request.getBoardId() << ".");

    uint32_t locale = 0;
    if (message != nullptr)
    {
        locale = message->getLocale();
    }
    else if (gCurrentUserSession != nullptr)
    {
        locale = gCurrentUserSession->getSessionLocale();
    }

    const StatLeaderboard* leaderboard = getConfigData()->getStatLeaderboard(request.getBoardId(), request.getBoardName());

    if (leaderboard == nullptr)
    {
        return GetLeaderboardGroupError::STATS_ERR_INVALID_LEADERBOARD_ID;
    }

    const LeaderboardGroup* group = leaderboard->getLeaderboardGroup();
    response.setName(group->getGroup());

    LbStatList::const_iterator nmIter = group->getStats().begin();
    LbStatList::const_iterator nmIterEnd = group->getStats().end();
    response.getStatDescSummaries().reserve(group->getStats().size());

    while (nmIter != nmIterEnd)
    {
        const GroupStat* groupStat = *nmIter;
        ++nmIter;

        StatDescSummary* statDescSummary = response.getStatDescSummaries().pull_back();

        statDescSummary->setName(groupStat->getStat()->getName());
        statDescSummary->setShortDesc(gLocalization->localize(groupStat->getShortDesc(), locale));
        statDescSummary->setLongDesc(gLocalization->localize(groupStat->getLongDesc(), locale));
        statDescSummary->setDefaultValue(groupStat->getStat()->getDefaultValAsString());
        statDescSummary->setFormat(groupStat->getStatFormat());
        statDescSummary->setType(groupStat->getStat()->getDbType().getType());
        statDescSummary->setKind(groupStat->getKind());
        statDescSummary->setMetadata(groupStat->getMetadata());
        statDescSummary->setCategory(groupStat->getStat()->getCategory().getName());
        statDescSummary->setDerived(groupStat->getStat()->isDerived());
    }
    response.setEntityType(leaderboard->getStatCategory()->getCategoryEntityType());
    response.setStatName(leaderboard->getStatName());
    response.setBoardName(leaderboard->getBoardName());
    response.setDesc(gLocalization->localize(leaderboard->getDescription(), locale));
    response.setMetadata(leaderboard->getMetadata());
    response.setLeaderboardSize(leaderboard->getSize());
    response.setAscending(leaderboard->isAscending());

    const ScopeNameValueListMap* scopeNameValueListMap = leaderboard->getScopeNameValueListMap();
    if (scopeNameValueListMap != nullptr)
    {
        ScopeNameValueListMap::const_iterator scopeMapItr = scopeNameValueListMap->begin();
        ScopeNameValueListMap::const_iterator scopeMapEnd = scopeNameValueListMap->end();
        for (; scopeMapItr != scopeMapEnd; ++scopeMapItr)
        {
            response.getKeyScopeNameValueListMap().insert(eastl::make_pair(scopeMapItr->first, scopeMapItr->second->clone()));
        }
    }

    return GetLeaderboardGroupError::ERR_OK;
}

GetKeyScopesMapError::Error StatsSlaveImpl::processGetKeyScopesMap(Blaze::Stats::KeyScopes &response, const Message *message)
{
    const KeyScopesMap& keyScopesMap = getConfigData()->getKeyScopeMap();

    response.setKeyScopesMap(const_cast<KeyScopesMap&>(keyScopesMap));

    return GetKeyScopesMapError::ERR_OK;
}

GetLeaderboardFolderGroupError::Error StatsSlaveImpl::processGetLeaderboardFolderGroup(const Blaze::Stats::LeaderboardFolderGroupRequest &request,
    Blaze::Stats::LeaderboardFolderGroup &response, const Message *message)
{
    uint32_t locale = 0;
    if (message != nullptr)
    {
        locale = message->getLocale();
    }
    else if (gCurrentUserSession != nullptr)
    {
        locale = gCurrentUserSession->getSessionLocale();
    }

    uint32_t folderId = request.getFolderId();

    if (getConfigData() == nullptr)
    {
        TRACE_LOG("[StatsSlaveImpl].processGetLeaderboardFolderGroup(): Config data not found");
        return GetLeaderboardFolderGroupError::STATS_ERR_CONFIG_NOTAVAILABLE;
    }

    if (getConfigData()->getStatLeaderboardTree() == nullptr)
    {
        TRACE_LOG("[StatsSlaveImpl].processGetLeaderboardFolderGroup(): leaderboard is not available");
        return GetLeaderboardFolderGroupError::STATS_ERR_INVALID_FOLDER_ID;
    }

    //  check for folder name in request.
    if (request.getFolderName() != nullptr && (*request.getFolderName()) != '\0')
    {
        LeaderboardFolderIndexMap::const_iterator it = getConfigData()->getLeaderboardFolderIndexMap()->find(request.getFolderName());
        if (it != getConfigData()->getLeaderboardFolderIndexMap()->end())
        {
            folderId = it->second;
        }
        else 
        {
            folderId = LeaderboardFolderGroupRequest::INVALID_FOLDER;
        }
    }

    TRACE_LOG("[StatsSlaveImpl].processGetLeaderboardFolderGroup(): Retrieving folder: " << folderId << ".");

    StatLeaderboardTreeNodePtr* statLeaderboardTree = getConfigData()->getStatLeaderboardTree();
    uint32_t folderCount = getConfigData()->getStatLeaderboardTreeSize();

    if (folderId >= folderCount)
    {
        TRACE_LOG("[StatsSlaveImpl].processGetLeaderboardFolderGroup(): Invalid folder ID.");
        return GetLeaderboardFolderGroupError::STATS_ERR_INVALID_FOLDER_ID;
    }

    if (statLeaderboardTree[folderId]->getData().getFirstChild() == 0)
    {
        TRACE_LOG("[StatsSlaveImpl].processGetLeaderboardFolderGroup(): Requested folder is a leaderboard " << folderId << " " 
                  <<  SbFormats::HexLower(FolderDescriptor::IS_LEADERBOARD) << ".");
        return GetLeaderboardFolderGroupError::STATS_ERR_INVALID_FOLDER_ID;
    }

    int32_t firstChild = statLeaderboardTree[folderId]->getData().getFirstChild();
    int32_t childCount = statLeaderboardTree[folderId]->getData().getChildCount();

    response.setParentId(statLeaderboardTree[folderId]->getData().getParent());
    response.setFolderId(folderId);
    response.setName(statLeaderboardTree[folderId]->getName());
    response.setDescription(gLocalization->localize(statLeaderboardTree[folderId]->getDesc(), locale));
    response.setShortDesc(gLocalization->localize(statLeaderboardTree[folderId]->getShortDesc(), locale));
    response.setMetadata(statLeaderboardTree[folderId]->getMetadata());
    response.getFolderDescriptors().reserve(childCount);

    for (int32_t j = firstChild; j < firstChild + childCount; ++j)
    {
        FolderDescriptor* desc = response.getFolderDescriptors().pull_back();

        if (statLeaderboardTree[j]->getData().getFirstChild() == 0)
            desc->setFolderId(j | FolderDescriptor::IS_LEADERBOARD);
        else
            desc->setFolderId(j);

        desc->setName(statLeaderboardTree[j]->getName());
        desc->setDescription(gLocalization->localize(statLeaderboardTree[j]->getDesc(), locale));
        desc->setShortDesc(gLocalization->localize(statLeaderboardTree[j]->getShortDesc(), locale));
    }

    return GetLeaderboardFolderGroupError::ERR_OK;
}

uint16_t StatsSlaveImpl::getUserRankId(const char8_t* name) const
{
    return mStatsExtendedDataProvider->getUserRanks().getUserRankId(name);
}

int32_t StatsSlaveImpl::getUserRank(BlazeId blazeId, const char8_t* name) const
{
    return mStatsExtendedDataProvider->getUserRanks().getUserRank(blazeId, name);
}

// send leaderboard tree to the client asynchronously one node per notification

typedef struct {
uint32_t current;
uint32_t end;
} StackFrame;


BlazeRpcError StatsSlaveImpl::processGetLeaderboardTreeAsyncInternal(
    const Blaze::Stats::GetLeaderboardTreeRequest &request, const Message *message)
{
    if (message == nullptr)
        return Blaze::ERR_SYSTEM;

    uint32_t locale = 0;
    locale = message->getLocale();

    if (getConfigData() == nullptr)
    {
        ERR_LOG("[StatsSlaveImpl].processGetLeaderboardTreeAsync: configuration data is not available");
        return Blaze::STATS_ERR_CONFIG_NOTAVAILABLE;
    }

    if (getConfigData()->getStatLeaderboardTree() == nullptr)
    {
        ERR_LOG("[StatsSlaveImpl].processGetLeaderboardTreeAsync: leaderboard is not available");
        return Blaze::STATS_ERR_INVALID_FOLDER_NAME;
    }

    uint32_t folderId;
    const char8_t* folderName = request.getFolderName();

    //  check for folder name in request.
    LeaderboardFolderIndexMap::const_iterator it = getConfigData()->getLeaderboardFolderIndexMap()->find(folderName);
    if (it != getConfigData()->getLeaderboardFolderIndexMap()->end())
    {
        folderId = it->second;
    }
    else 
    {
        ERR_LOG("[StatsSlaveImpl].processGetLeaderboardTreeAsync: requested folder <" << folderName << "> not found");
        return Blaze::STATS_ERR_INVALID_FOLDER_NAME;
    }

    TRACE_LOG("[StatsSlaveImpl].processGetLeaderboardTreeAsync: retrieving leaderboard tree starting from folder: <" << folderName << ">");

    StatLeaderboardTreeNodePtr* statLeaderboardTree = getConfigData()->getStatLeaderboardTree();

    // traversing subtree. Not using recursion not to blow stack
    // first step - make list of nodes
    StackFrame frame;

    eastl::vector<StackFrame> stack;
    eastl::vector<uint32_t> nodes;

    uint32_t m = folderId;
    frame.current = statLeaderboardTree[m]->getData().getFirstChild();
    frame.end = frame.current + statLeaderboardTree[m]->getData().getChildCount();
    nodes.push_back(m);
    do
    {
        if (frame.current == frame.end)
        {            
            if (stack.size() == 0) 
                break;
            frame = stack.back();
            stack.pop_back();
        }
        else
        {
            m = frame.current;
            nodes.push_back(frame.current);
            ++frame.current;
            stack.push_back(frame);
            frame.current = statLeaderboardTree[m]->getData().getFirstChild();
            frame.end = frame.current + statLeaderboardTree[m]->getData().getChildCount();
        }
    }
    while (1);
    
    // at this point we have a list of nodes in subtree
    // send tree nodes to client one at time
    eastl::vector<uint32_t>::const_iterator nodeIter = nodes.begin();
    eastl::vector<uint32_t>::const_iterator nodeEnd = nodes.end();

    LeaderboardTreeNodes treeNodeList;
    treeNodeList.getLeaderboardTreeNodes().reserve(eastl::min((uint32_t) nodes.size(), getConfig().getMaxNodesPerLeaderboardTreeNotification()));

    while(nodeIter != nodeEnd)
    {
        LeaderboardTreeNode* treeNode = treeNodeList.getLeaderboardTreeNodes().pull_back();
        treeNode->setNodeId(*nodeIter);
        treeNode->setNodeName(statLeaderboardTree[*nodeIter]->getName());
        treeNode->setShortDesc(gLocalization->localize(statLeaderboardTree[*nodeIter]->getShortDesc(), locale));
        treeNode->setFirstChild(statLeaderboardTree[*nodeIter]->getData().getFirstChild());
        treeNode->setChildCount(statLeaderboardTree[*nodeIter]->getData().getChildCount());
        treeNode->setRootName(folderName);
         ++nodeIter;
        if (nodeIter == nodeEnd)
        {
            // This is last notification
            treeNode->setLastNode(true);
        }
        else
        {
            treeNode->setLastNode(false);
        }

        if(nodeIter == nodeEnd || treeNodeList.getLeaderboardTreeNodes().size() == getConfig().getMaxNodesPerLeaderboardTreeNotification())
        {
            sendGetLeaderboardTreeNotificationToUserSession(gCurrentLocalUserSession, &treeNodeList, false, message->getMsgNum());
            if(nodeIter == nodeEnd)
               treeNodeList.getLeaderboardTreeNodes().release();
            else
                treeNodeList.getLeaderboardTreeNodes().clear();
        }
    }
    
    return Blaze::ERR_OK;
}


GetLeaderboardTreeAsyncError::Error StatsSlaveImpl::processGetLeaderboardTreeAsync(
    const Blaze::Stats::GetLeaderboardTreeRequest &request, const Message *message)
{
    return GetLeaderboardTreeAsyncError::commandErrorFromBlazeError(processGetLeaderboardTreeAsyncInternal(request, message));
}

GetLeaderboardTreeAsync2Error::Error StatsSlaveImpl::processGetLeaderboardTreeAsync2(
    const Blaze::Stats::GetLeaderboardTreeRequest &request, const Message *message)
{
    return GetLeaderboardTreeAsync2Error::commandErrorFromBlazeError(processGetLeaderboardTreeAsyncInternal(request, message));
}

BlazeRpcError StatsSlaveImpl::getCenteredLeaderboardEntries(const StatLeaderboard& leaderboard,
    const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, EntityId centerEntityId, int32_t count,
    const EA::TDF::ObjectId& userSetId, bool showAtBottomIfNotFound,
    EntityIdList& keys, LeaderboardStatValues& response, bool& sorted) const
{
    return mLeaderboardIndexes->getCenteredLeaderboardRows(leaderboard, scopeNameValueListMap, periodId,
        centerEntityId, count, userSetId, showAtBottomIfNotFound, keys, response, sorted);
}

BlazeRpcError StatsSlaveImpl::getLeaderboardEntries(const StatLeaderboard& leaderboard,
    const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, int32_t startRank, int32_t count,
    const EA::TDF::ObjectId& userSetId, EntityIdList& keys, LeaderboardStatValues& response, bool& sorted) const
{
    return mLeaderboardIndexes->getLeaderboardRows(leaderboard, scopeNameValueListMap, periodId,
        startRank, count, userSetId, keys, response, sorted);
}

BlazeRpcError StatsSlaveImpl::getFilteredLeaderboardEntries(const StatLeaderboard& leaderboard,
    const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, const EntityIdList& idList, uint32_t limit,
    EntityIdList& keys, LeaderboardStatValues& response, bool& sorted) const
{
    return mLeaderboardIndexes->getFilteredLeaderboardRows(leaderboard, scopeNameValueListMap, periodId,
        idList, limit, keys, response, sorted);
}

BlazeRpcError StatsSlaveImpl::getEntityCount(const StatLeaderboard* leaderboard,
    const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, EntityCount* response) const
{
    return mLeaderboardIndexes->getEntityCount(leaderboard, scopeNameValueListMap, periodId, response);
}

BlazeRpcError StatsSlaveImpl::getLeaderboardRanking(const StatLeaderboard* leaderboard, 
    const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, EntityId centerEntityId, int32_t &entityRank) const
{
    return mLeaderboardIndexes->getLeaderboardRanking(leaderboard, scopeNameValueListMap, periodId,
        centerEntityId, entityRank);
}

/*************************************************************************************************/
/*!
    \brief getPeriodIdForTime
    Return period id for the given epoch time and period type
    \param[in] - t - epoch time
    \param[in] - periodType - period type
    \return - period id
*/
/*************************************************************************************************/
int32_t StatsSlaveImpl::getPeriodIdForTime(int32_t t, StatPeriodType periodType) const
{
    int32_t periodId = 0;
    struct tm tM;
    int32_t day;
    if (periodType != STAT_PERIOD_ALL_TIME)
    {
        if (t != 0)
        {
            Rollover::gmTime(&tM, t);
            switch(periodType)
            {
                case STAT_PERIOD_DAILY:
                    if (mDebugTimer.useTimer)
                    {
                        periodId = static_cast<int32_t>(t/mDebugTimer.dailyTimer);
                    }
                    else
                    {
                        periodId = t/(3600 * 24);
                        if (tM.tm_hour >= mPeriodIds.getDailyHour())
                            ++periodId;
                    }
                break;

                case STAT_PERIOD_WEEKLY:
                    if (mDebugTimer.useTimer)
                    {
                        periodId = static_cast<int32_t>(t/mDebugTimer.weeklyTimer);
                    }
                    else
                    {
                        day = mPeriodIds.getWeeklyDay();
                        periodId = t/(3600 * 24 * 7);
                        if ((tM.tm_wday > day) || ((tM.tm_wday == day) && tM.tm_hour >= mPeriodIds.getWeeklyHour()))
                        {
                            ++periodId;
                        }
                    }
                break;

                case STAT_PERIOD_MONTHLY:
                    if (mDebugTimer.useTimer)
                    {
                        periodId = static_cast<int32_t>(t/mDebugTimer.monthlyTimer);
                    }
                    else
                    {
                        day = mPeriodIds.getMonthlyDay();
                        periodId = (tM.tm_year - 70 ) * 12 + tM.tm_mon; //All the previous years since epoch start (1970) + this one
                        if ((tM.tm_mday > day) || ((tM.tm_mday == day) && tM.tm_hour >= mPeriodIds.getMonthlyHour()))
                        {
                            ++periodId;
                        }
                    }
                break;
                
                default:
                break;
            }    
        }
    }
    
    return periodId;
}

/*************************************************************************************************/
/*!
    \brief printMemBudget
    Prints to log memory requirements for leaderboards chache
    \param[in] - none
    \return - none
*/
/*************************************************************************************************/
void StatsSlaveImpl::printMemBudget() const
{
    //INFO_LOG("[StatsSlaveImpl"].printMemBudget: cache data size: " << dataSize << " bytes, overhead: " << overSize 
              //  << " bytes, total:" << (dataSize + overSize) << " bytes");
}

/*************************************************************************************************/
/*!
    \brief getStatusInfo
    Returns comonent status info
    \param[in] - none
    \return - none
*/
/*************************************************************************************************/
void StatsSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    char8_t buf[64];

    ComponentStatus::InfoMap& map = status.getInfo();

    if (mLeaderboardIndexes != nullptr)
        mLeaderboardIndexes->getStatusInfo(status);

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mUpdateStatsCount.getTotal());
    map["TotalUpdateStatsRequests"] = buf;
    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mUpdateStatsFailedCount.getTotal());
    map["TotalUpdateStatsRequestsFailed"] = buf;

    if (mStatsCache != nullptr)
    {
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mStatsCache->mCacheRowFetchCount.getTotal());
        map["TotalCacheRowsFetched"] = buf;
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mStatsCache->mDbRowFetchCount.getTotal());
        map["TotalDbRowsFetched"] = buf;
    }
}

void StatsSlaveImpl::doChangeKeyscopeValue(KeyScopeChangeRequestPtr request)
{
    changeKeyscopeValue(*request);
}

BlazeRpcError StatsSlaveImpl::changeKeyscopeValue(const KeyScopeChangeRequest& request)
{
    UpdateExtendedDataRequest uedRequest;

    ScopeValue aggregateKey = mStatsConfig->getAggregateKeyValue(request.getKeyScopeName());
    if (aggregateKey == request.getOldKeyScopeValue())
    {
        ERR_LOG("[StatsSlaveImpl].changeKeyscopeValue: can't change from aggregate key for keyscope " << request.getKeyScopeName());
        return STATS_ERR_BAD_SCOPE_INFO;
    }
    if (aggregateKey == request.getNewKeyScopeValue())
    {
        ERR_LOG("[StatsSlaveImpl].changeKeyscopeValue: can't change to aggregate key for keyscope " << request.getKeyScopeName());
        return STATS_ERR_BAD_SCOPE_INFO;
    }
    if (!mStatsConfig->isValidScopeValue(request.getKeyScopeName(), request.getNewKeyScopeValue()))
    {
        ERR_LOG("[StatsSlaveImpl].changeKeyscopeValue: invalid value " << request.getNewKeyScopeValue() << " for keyscope " << request.getKeyScopeName());
        return STATS_ERR_BAD_SCOPE_INFO;
    }

    TRACE_LOG("[StatsSlaveImpl].changeKeyscopeValue: started");

    uint32_t dbId;
    BlazeRpcError err;
    err = getConfigData()->getShardConfiguration().lookupDbId(request.getEntityType(), request.getEntityId(), dbId);
    if (err != ERR_OK)
        return err;

    // build query
    DbConnPtr conn = gDbScheduler->getConnPtr(dbId);
    if (conn == nullptr)
        return ERR_SYSTEM;

    BlazeRpcError dbError = conn->startTxn();
    if (dbError != ERR_OK)
    {
        ERR_LOG("[StatsSlaveImpl].changeKeyscopeValue: database transaction was not successful");
        return ERR_SYSTEM;
    }

    StatUpdateNotification notif;
    notif.getBroadcast().switchActiveMember(StatBroadcast::MEMBER_STATDELETEANDUPDATE);
    StatDeleteAndUpdateBroadcast* broadcast = notif.getBroadcast().getStatDeleteAndUpdate();
    CacheRowDeleteList& deleteList = broadcast->getCacheDeletes();
    CacheRowUpdateList& updateList = broadcast->getCacheUpdates();

    cachePeriodIds(broadcast->getCurrentPeriodIds());
    
    StringBuilder lbQueryString;

    // scan for all categories using "scopeName" keyscope
    CategoryMap::const_iterator catItr = mStatsConfig->getCategoryMap()->begin();
    CategoryMap::const_iterator catEnd = mStatsConfig->getCategoryMap()->end();
    for (; catItr != catEnd; ++catItr)
    {
        const StatCategory* cat = catItr->second;
        if (cat->getCategoryEntityType() == request.getEntityType() && cat->isValidScopeName(request.getKeyScopeName()))
        {
            for(int32_t periodType = 0; periodType < STAT_NUM_PERIODS; ++periodType)
            {
                if (cat->isValidPeriod(periodType))
                {
                    QueryPtr query = DB_NEW_QUERY_PTR(conn);
                    query->append("UPDATE `$s` SET `$s` = $D WHERE `entity_id` = $D AND `$s` = $D;",
                        cat->getTableName(periodType),
                        request.getKeyScopeName(),
                        request.getNewKeyScopeValue(),
                        request.getEntityId(),
                        request.getKeyScopeName(),
                        request.getOldKeyScopeValue());

                    DbResultPtr dbResult;
                    dbError = conn->executeQuery(query, dbResult);
                    if (dbError != ERR_OK)
                    {
                        ERR_LOG("[StatsSlaveImpl].changeKeyscopeValue: error processing db update call: " << getDbErrorString(dbError) << ".");
                        if (conn->isTxnInProgress())
                        {
                            conn->rollback();
                        }
                        return ERR_SYSTEM;
                    }

                    uint32_t affectedRowCount = dbResult->getAffectedRowCount();

                    if (affectedRowCount > 0)
                    {
                        // if more than 1 row is affected, assume that this category has more than 1 keyscope and/or period

                        // need to broadcast for any stats-level caching

                        // need to ask DB for stats with updated keyscope because old stats may not have been in LB anyway
                        query = DB_NEW_QUERY_PTR(conn);
                        query->append("SELECT * FROM `$s` WHERE `entity_id` = $D AND `$s` = $D;",
                            cat->getTableName(periodType),
                            request.getEntityId(),
                            request.getKeyScopeName(),
                            request.getNewKeyScopeValue());

                        TRACE_LOG("[StatsSlaveImpl].changeKeyscopeValue: prepared select query");

                        dbError = conn->executeQuery(query, dbResult);
                        if (dbError != ERR_OK)
                        {
                            ERR_LOG("[StatsSlaveImpl].changeKeyscopeValue: error processing db select call: " << getDbErrorString(dbError) << ".");
                            if (conn->isTxnInProgress())
                            {
                                conn->rollback();
                            }
                            return ERR_SYSTEM;
                        }

                        // re-adding entries using a new keyscope value
                        DbResult::const_iterator dbRowItr = dbResult->begin();
                        DbResult::const_iterator dbRowEnd = dbResult->end();
                        for (; dbRowItr != dbRowEnd; ++dbRowItr)
                        {
                            DbRow* dbRow = *dbRowItr;

                            CacheRowDelete *cacheRowDelete = deleteList.pull_back();
                            cacheRowDelete->setCategoryId(cat->getId());
                            cacheRowDelete->setEntityId(request.getEntityId());
                            cacheRowDelete->getPeriodTypes().push_back(periodType);

                            // build the keyscope map
                            ScopeNameValueMap newScopes;
                            const ScopeNameSet* keyScopeNameSet = cat->getScopeNameSet();
                            size_t size = keyScopeNameSet->size(); // keyScopeNameSet can't (shouldn't) be nullptr
                            for (size_t i = 0; i < size; i++)
                            {
                                const char8_t* keyScopeName = (*keyScopeNameSet)[i].c_str();
                                ScopeName scopeName = keyScopeName;
                                ScopeValue scopeValue;
                                if (blaze_stricmp(keyScopeName, request.getKeyScopeName()) == 0)
                                {
                                    scopeValue = request.getNewKeyScopeValue();
                                    newScopes.insert(eastl::make_pair(scopeName, scopeValue));
                                    scopeValue = request.getOldKeyScopeValue();
                                    cacheRowDelete->getKeyScopeNameValueMap().insert(eastl::make_pair(scopeName, scopeValue));
                                }
                                else
                                {
                                    scopeValue = static_cast<ScopeValue>(dbRow->getInt64(keyScopeName));
                                    newScopes.insert(eastl::make_pair(scopeName, scopeValue));
                                    cacheRowDelete->getKeyScopeNameValueMap().insert(eastl::make_pair(scopeName, scopeValue));
                                }
                            }

                            StatValMap statValMap;

                            // fill the stats
                            StatMap::const_iterator catStatItr = cat->getStatsMap()->begin();
                            StatMap::const_iterator catStatEnd = cat->getStatsMap()->end();
                            for (; catStatItr != catStatEnd; ++catStatItr)
                            {
                                const Stat* stat = catStatItr->second;
                                StatVal* statVal = BLAZE_NEW StatVal();
                                statVal->type = stat->getDbType().getType();
                                statVal->changed = true;
                                statValMap[stat->getName()] = statVal;

                                if (stat->getDbType().getType() == STAT_TYPE_INT)
                                {
                                    if (stat->getTypeSize() == 8)
                                    {
                                        statVal->intVal = dbRow->getInt64(stat->getName());
                                    }
                                    else
                                    {
                                        statVal->intVal = dbRow->getInt(stat->getName());
                                    }
                                }
                                else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                                {
                                    statVal->floatVal = dbRow->getFloat(stat->getName());
                                }
                                else
                                {
                                    statVal->stringVal = dbRow->getString(stat->getName());
                                }
                            }

                            int32_t periodId = 0;
                            if (periodType != STAT_PERIOD_ALL_TIME)
                                periodId = dbRow->getInt("period_id");

                            if (periodType == STAT_PERIOD_ALL_TIME || (broadcast->getCurrentPeriodIds().find(periodType)->second == periodId))
                            {
                                mStatsExtendedDataProvider->getUserStats().deleteUserStats(uedRequest, request.getEntityId(),
                                    cat->getId(), periodType, cacheRowDelete->getKeyScopeNameValueMap());
                                mStatsExtendedDataProvider->getUserStats().updateUserStats(uedRequest, request.getEntityId(),
                                    cat->getId(), periodType, newScopes, statValMap);
                            }

                            CacheRowUpdate* cacheRowUpdate = BLAZE_NEW CacheRowUpdate();
                            char8_t keyScopeString[128];
                            LeaderboardIndexes::buildKeyScopeString(keyScopeString, sizeof(keyScopeString), newScopes);
                            mLeaderboardIndexes->updateStats(cat->getId(), request.getEntityId(), periodType, periodId, &statValMap, &newScopes, keyScopeString, lbQueryString, cacheRowUpdate->getStatUpdates());
                            if (!cacheRowUpdate->getStatUpdates().empty() || mStatsCache->isLruEnabled())
                            {
                                // need to send at least the category and entity info to invalidate any LRU cache
                                LeaderboardIndexes::populateCacheRow(*cacheRowUpdate, cat->getId(), request.getEntityId(), periodType, periodId, keyScopeString);
                                updateList.push_back(cacheRowUpdate);
                            }
                            else
                                delete cacheRowUpdate;

                            for (StatValMap::const_iterator svIt = statValMap.begin(); svIt != statValMap.end(); ++svIt)
                            {
                                delete svIt->second;
                            }
                        }
                    }
                }
            }// for each period type
        }// if category
    }

    mLeaderboardIndexes->deleteStats(broadcast->getCacheDeletes(), broadcast->getCurrentPeriodIds(), &lbQueryString);
    mStatsExtendedDataProvider->getUserRanks().updateUserRanks(uedRequest, updateList);
    mStatsExtendedDataProvider->getUserRanks().updateUserRanks(uedRequest, deleteList);

    if (!lbQueryString.isEmpty())
    {
        DbConnPtr lbConn = gDbScheduler->getConnPtr(getDbId());
        if (lbConn == nullptr)
        {
            ERR_LOG("[statsSlaveImpl].changeKeyScopeValue: failed to obtain connection to leaderboard database, will rollback.");
            return ERR_SYSTEM;
        }

        QueryPtr lbQuery = DB_NEW_QUERY_PTR(lbConn);
        lbQuery->append("$s", lbQueryString.get());
        DbError lbError = lbConn->executeMultiQuery(lbQuery);
        if (lbError != DB_ERR_OK)
        {
            WARN_LOG("[StatsSlaveImpl].changeKeyscopeValue: failed to update leaderboard cache tables with error <" << getDbErrorString(lbError) << ">.");
        }
    }

    // Tell master and remote slaves to update cache
    if (!deleteList.empty() || !updateList.empty())
    {
        if (mStatsCache->isLruEnabled())
        {
            mStatsCache->deleteRows(updateList);
            mStatsCache->deleteRows(deleteList, broadcast->getCurrentPeriodIds());
        }
        sendUpdateCacheNotificationToAllSlaves(&notif);
    }

    if (conn->isTxnInProgress())
    {
        dbError = conn->commit();
        if (dbError != ERR_OK)
        {
            ERR_LOG("[StatsSlaveImpl].changeKeyscopeValue: failed to commit stat change. Will rollback.: " << getDbErrorString(dbError));
            conn->rollback();
            return ERR_SYSTEM;
        }
    }

    TRACE_LOG("[StatsSlaveImpl].changeKeyscopeValue: completed");

    uedRequest.setComponentId(StatsSlave::COMPONENT_ID);
    uedRequest.setIdsAreSessions(false);
    BlazeRpcError uedErr = Blaze::ERR_OK;
    uedErr = gUserSessionManager->updateExtendedData(uedRequest);
    if (uedErr != Blaze::ERR_OK)
    {
        WARN_LOG("[StatsSlaveImpl].changeKeyscopeValue: " << (ErrorHelp::getErrorName(uedErr)) << " error updating user extended data");
    }

    return ERR_OK;
}

void StatsSlaveImpl::onLocalUserSessionLogin(const UserSessionMaster& userSession)
{
    updateAccountCountry(userSession, "onLocalUserSessionLogin");
}

void StatsSlaveImpl::onLocalUserAccountInfoUpdated(const UserSessionMaster& userSession)
{
    updateAccountCountry(userSession, "onLocalUserAccountInfoUpdated");
}

void StatsSlaveImpl::updateAccountCountry(const UserSessionMaster& userSession, const char8_t* operation)
{
    if (userSession.getAccountCountry() != userSession.getPreviousAccountCountry())
    {
        /// @todo replace "accountcountry" with a configurable list
        const KeyScopesMap& keyScopeMap = getConfigData()->getKeyScopeMap();
        if (keyScopeMap.find("accountcountry") != keyScopeMap.end())
        {
            eastl::string previousAccountBuf, currentAccountBuf;
            previousAccountBuf.sprintf("0x%04x", userSession.getPreviousAccountCountry());
            currentAccountBuf.sprintf("0x%04x", userSession.getAccountCountry());

            INFO_LOG("[StatsSlaveImpl].updateAccountCountry: " << operation << " : replacing " << previousAccountBuf.c_str() 
                     <<" with " << currentAccountBuf.c_str() <<" for " << userSession.getBlazeId());

            Stats::KeyScopeChangeRequestPtr request = BLAZE_NEW Stats::KeyScopeChangeRequest();
            request->setEntityType(ENTITY_TYPE_USER);
            request->setEntityId(static_cast<EntityId>(userSession.getBlazeId()));
            request->setKeyScopeName("accountcountry");
            request->setOldKeyScopeValue(static_cast<ScopeValue>(userSession.getPreviousAccountCountry()));
            request->setNewKeyScopeValue(static_cast<ScopeValue>(userSession.getAccountCountry()));

            gSelector->scheduleFiberCall(this, &StatsSlaveImpl::doChangeKeyscopeValue, request, "StatsSlaveImpl::doChangeKeyscopeValue");
        }
    }
}

void StatsSlaveImpl::onExecuteGlobalCacheInstruction(
    const StatsGlobalCacheInstruction &data, 
    UserSession *associatedUserSession)
{
    // this must be done on fiber because it involves database access
    gSelector->scheduleFiberCall<StatsSlaveImpl, StatsGlobalCacheInstruction>(
            this, 
            &StatsSlaveImpl::executeGlobalCacheInstruction, 
            data,
            "StatsSlaveImpl::executeGlobalCacheInstruction");
}

void StatsSlaveImpl::executeGlobalCacheInstruction(const StatsGlobalCacheInstruction data)
{
    EA_ASSERT_MSG(mGlobalStatsCache != nullptr, "Global cache can't be nullptr at any time");

    mLastGlobalCacheInstructionWasWrite = false; 

    if (!mGlobalStatsCache->isInitialized() 
        && data.getInstruction() != STATS_GLOBAL_CACHE_REBUILD)
    {
        if (data.getInstruction() == STATS_GLOBAL_CACHE_WRITE)
        {
            // At startup slave can be selected to write cache although it doesn't have it yet
            // this happens because when onSlaveSesseionAdded is invoked on master there will 
            // be more than one slave in the list so master will think that other slaves are 
            // actually already active which isn't the case.
            // So if we've been asked to write cache and we don't have it yet, just report
            // back success with 0 rows written.
            mLastGlobalCacheInstructionWasWrite = true;
            GlobalCacheInstructionExecutionResult request;
            request.setRowsWritten(0);
            getMaster()->reportGlobalCacheInstructionExecutionResult(request);
        }
    }
    else
    {
        size_t countOfRowsWritten = 0;
        if ( data.getInstruction() == STATS_GLOBAL_CACHE_FORCE_WRITE ||
             data.getInstruction() == STATS_GLOBAL_CACHE_WRITE)
        {
            BlazeRpcError result = mGlobalStatsCache->serializeToDatabase(
                data.getInstruction() == STATS_GLOBAL_CACHE_FORCE_WRITE, 
                countOfRowsWritten);

            if (result != Blaze::ERR_OK)
            {
                ERR_LOG("[StatsSlaveImpl].onExecuteGlobalCacheInstruction: failed to write global cache to database. Error was: "
                    << getDbErrorString(result));
            }
            else
            {
                // remember that this slave wrote global cache to database last
                // so when it shuts down it should write the cache back to database
                mLastGlobalCacheInstructionWasWrite = true;
            }

            GlobalCacheInstructionExecutionResult request;
            // report number of rows written or -1 in case of error
            request.setRowsWritten(result == Blaze::ERR_OK ? countOfRowsWritten : -1);
            getMaster()->reportGlobalCacheInstructionExecutionResult(request);
        }
        else if (data.getInstruction() == STATS_GLOBAL_CACHE_CLEAR)
        {
            mGlobalStatsCache->makeCacheRowsClean();
        }
        else if (data.getInstruction() == STATS_GLOBAL_CACHE_REBUILD)
        {
            delete mGlobalStatsCache;
            mGlobalStatsCache = BLAZE_NEW GlobalStatsCache(*this);
            mGlobalStatsCache->initialize();
        }
    }
}


/*************************************************************************************************/
StatsTransactionContext::StatsTransactionContext(const char* description, StatsSlaveImpl &component)
    : TransactionContext(description), mComp(&component)
{
    mNotification.getBroadcast().switchActiveMember(StatBroadcast::MEMBER_STATUPDATE);
    mUpdateState = UNINITIALIZED;
    mStatsProvider = nullptr;
    mProcessGlobalStats = false;
    memset(mPeriodIds, 0, sizeof(mPeriodIds));
}

StatsTransactionContext::~StatsTransactionContext()
{
    clear();
}

BlazeRpcError StatsTransactionContext::commit()
{
    mExtDataReq.setComponentId(StatsSlave::COMPONENT_ID);
    mExtDataReq.setIdsAreSessions(false);

    BlazeRpcError result = commitUpdates();
    if (result != ERR_OK)
    {
        abortTransaction();
        ERR_LOG("[StatsTransactionContext].commit: failed to update stats <" << getDbErrorString(result) << ">. Will rollback.");
        return STATS_ERR_DB_QUERY_FAILED;
    }

    if (!mLbQueryString.isEmpty())
    {
        DbConnPtr lbConn = gDbScheduler->getConnPtr(mComp->getDbId());
        if (lbConn == nullptr)
        {
            WARN_LOG("[StatsTransactionContext].commit: failed to obtain connection to leaderboard database. Cache tables will not be updated.");
        }
        else
        {
            QueryPtr lbQuery = DB_NEW_QUERY_PTR(lbConn);
            lbQuery->append("$s", mLbQueryString.get());
            DbError lbError = lbConn->executeMultiQuery(lbQuery);
            if (lbError != DB_ERR_OK)
            {
                WARN_LOG("[StatsTransactionContext].commit: failed to update leaderboard cache tables with error <" << getDbErrorString(lbError) << ">.");
            }
        }
    }

    if (!mNotification.getBroadcast().getStatUpdate()->getCacheUpdates().empty())
    {
        // Broadcast updates if needed
        mComp->sendUpdateCacheNotificationToRemoteSlaves(&mNotification);

        if (mComp->mStatsCache->isLruEnabled())
            mComp->mStatsCache->deleteRows(mNotification.getBroadcast().getStatUpdate()->getCacheUpdates());
    }

    result = commitTransaction();

    // Update user stats in the user session UserSessionExtendedData objects
    // We call it here to allow user session extended data to be updated before game reporting
    // sends notification to client that update completed
    BlazeRpcError err = Blaze::ERR_OK;
    err = gUserSessionManager->updateExtendedData(mExtDataReq);
    if (err != Blaze::ERR_OK)
    {
        WARN_LOG("[StatsTransactionContext].commit: " << (ErrorHelp::getErrorName(err)) << " error updating user extended data");
    }

    if (!mGlobalStats.getStatUpdates().empty())
    {
        // Broadcast global stat updates if needed
        mComp->sendUpdateGlobalStatsNotificationToAllSlaves(&mGlobalStats);
    }

    return result;
}

BlazeRpcError StatsTransactionContext::rollback()
{
    if ((mUpdateState & ABORTED) != 0 || (mUpdateState & INITIALIZED)==0)
        return ERR_SYSTEM;

    mStatsProvider->rollback();
    mUpdateState = ABORTED;

    return Blaze::ERR_OK;
}

BlazeRpcError StatsSlaveImpl::createTransactionContext(uint64_t customData, TransactionContext*& result)
{
    StatsTransactionContext *context 
        = BLAZE_NEW_NAMED("StatsTransactionContext") StatsTransactionContext("StatsTransactionContext", *this);

    result = static_cast<TransactionContext*>(context);
    return ERR_OK;
}

/*************************************************************************************************/

void StatsTransactionContext::reset()
{
    abortTransaction();
 
    mIncomingRequest.release(); 
    mUpdateState = UNINITIALIZED;

    StatUpdateBroadcast().copyInto(*mNotification.getBroadcast().getStatUpdate());

    clear();
}

/*************************************************************************************************/
/*!
    \brief clear

    Clear the data structures in general after a stats update.
*/
/*************************************************************************************************/
void StatsTransactionContext::clear()
{
    mDerivedStrings.clear();
    mScopesVectorForRowsVector.clear();
    mUpdateHelper.clear();

    if (mStatsProvider != nullptr)
        delete mStatsProvider;

    mStatsProvider = nullptr;
}

/*************************************************************************************************/
/*!
    \brief preprocessMultiCategoryDerivedStats

    For Blaze servers where multi category derived stats are in use, we may need to fetch
    additional rows beyond those directly identified in the update request (and possibly update
    them as well).  This method looks at our copy of the request object and determines which
    additional rows are needed.  This method must execute before validateRequest, as that method
    will perform sorting on the request so that we lock rows in a consistent order, so we want to
    get our additional rows inserted first here so that we only need to do that sorting once.

    \return - corresponding error code
*/
/*************************************************************************************************/
BlazeRpcError StatsTransactionContext::preprocessMultiCategoryDerivedStats()
{
    UpdateStatsRequest::StatRowUpdateList::const_iterator updateIter = mIncomingRequest.begin();
    UpdateStatsRequest::StatRowUpdateList::const_iterator updateEnd = mIncomingRequest.end(); 

    MultiCategoryDerivedMap map;

    // "Admin" requests are allowed to specify period types, but we enforce that the same period types
    // must be specified for the entire request when multi-category derived stats configs are involved.
    // While it is possible to specify different period types in various stat row updates that would not
    // cause a conflict, we just want to simplify this feature.
    typedef eastl::vector_set<int32_t> PeriodTypeSet; // to filter out dupes and order/sort
    PeriodTypeSet periodTypes;

    // Step one, build up a list of all dependent rows we may need
    for (; updateIter != updateEnd; ++updateIter)
    {
        // Look at a single row in the request
        const StatRowUpdate* rowUpdate = *updateIter;

        // While validateRequest could also take care of this, might as well fast-fail now if
        // the update specifies a bogus category
        const char8_t* categoryName = rowUpdate->getCategory();
        CategoryMap::const_iterator catIter = mComp->getConfigData()->getCategoryMap()->find(categoryName);
        if (catIter == mComp->getConfigData()->getCategoryMap()->end())
        {
            ERR_LOG("[StatsTransactionContext].preprocessMultiCategoryDerivedStats(): category name " << categoryName << " is not valid");
            return STATS_ERR_UNKNOWN_CATEGORY;
        }

        // check that all the period types are the same for all the stat row updates
        if (updateIter == mIncomingRequest.begin())
        {
            periodTypes.insert(rowUpdate->getPeriodTypes().begin(), rowUpdate->getPeriodTypes().end());
        }
        else if (periodTypes.size() > 0 || rowUpdate->getPeriodTypes().size() > 0)
        {
            // admin request case
            PeriodTypeSet periodTypes2;
            periodTypes2.insert(rowUpdate->getPeriodTypes().begin(), rowUpdate->getPeriodTypes().end());

            if (periodTypes2.size() != periodTypes.size())
            {
                ERR_LOG("[StatsTransactionContext].preprocessMultiCategoryDerivedStats(): period types must be the same for all stat row updates when multi-category derived stats configs are involved");
                return STATS_ERR_BAD_PERIOD_TYPE;
            }

            PeriodTypeSet::const_iterator pi = periodTypes.begin();
            PeriodTypeSet::const_iterator pe = periodTypes.end();
            PeriodTypeSet::const_iterator qi = periodTypes2.begin();
            for (; pi != pe; ++pi, ++qi)
            {
                if (*pi != *qi)
                {
                    ERR_LOG("[StatsTransactionContext].preprocessMultiCategoryDerivedStats(): period types must be the same for all stat row updates when multi-category derived stats configs are involved");
                    return STATS_ERR_BAD_PERIOD_TYPE;
                }
            }
        }

        // Look at all the dependencies that this category has on others
        for (StringSet::const_iterator csIter = catIter->second->getCategoryDependency()->catSet.begin();
            csIter != catIter->second->getCategoryDependency()->catSet.end(); ++csIter)
        {
            // Ignoring ourselves, add any other category to the working map - note that potentially
            // some of these may themselves already be in the incoming request, that is why we keep
            // them in a working map instead of adding them directly to the request, so that we can
            // filter out unneeded ones later
            if (blaze_strcmp(categoryName, *csIter) != 0)
                map[*csIter][rowUpdate->getEntityId()].insert(&(rowUpdate->getKeyScopeNameValueMap()));
        }
    }

    // Step two, filter out all those entries from the working map which are unneeded due to
    // corresponding rows already being in the request
    for (updateIter = mIncomingRequest.begin(); updateIter != updateEnd; ++updateIter) 
    {
        const StatRowUpdate* rowUpdate = *updateIter;
        MultiCategoryDerivedMap::iterator it1 = map.find(rowUpdate->getCategory());
        if (it1 == map.end())
            continue;
        EntityToScopeMap::iterator it2 = it1->second.find(rowUpdate->getEntityId());
        if (it2 != it1->second.end())
        {
            it2->second.erase(&(rowUpdate->getKeyScopeNameValueMap()));
        }
    }

    // Step three, add any remaining rows from the working map to the request
    for (MultiCategoryDerivedMap::iterator it1 = map.begin(); it1 != map.end(); ++it1)
    {
        for (EntityToScopeMap::iterator it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
        {
            for (ScopeNameValueMapSet::iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
            {
                StatRowUpdate* update = mIncomingRequest.pull_back(); 
                update->setCategory(it1->first);
                update->setEntityId(it2->first);
                update->getPeriodTypes().reserve(periodTypes.size());
                PeriodTypeSet::const_iterator pi = periodTypes.begin();
                PeriodTypeSet::const_iterator pe = periodTypes.end();
                for (; pi != pe; ++pi)
                {
                    update->getPeriodTypes().push_back(*pi);
                }
                update->getKeyScopeNameValueMap().insert((*it3)->begin(), (*it3)->end());
            }
        }
    }

    return ERR_OK;
}

// as period ID is the same for all rows in update it will not affect the sorting
struct statRowUpdateLess : public eastl::binary_function<const StatRowUpdate*, const StatRowUpdate*, bool>
{
    bool operator()(const StatRowUpdate* lhs, const StatRowUpdate* rhs)
    {
        // IMPORTANT: Until we get rid of statRowUpdateLess comparator object,
        // the UpdateRowKey::operator <() comparison algorithm must be equivalent to it,
        // because currently the statRowUpdateLess algorithm is used to sort input
        // to generate queries and the the UpdateRowKey::operator <() is used to walk
        // the query results in order...
        const int32_t result = blaze_strcmp(lhs->getCategory(), rhs->getCategory());
        if(result < 0) return true;
        if(result > 0) return false;
        if(lhs->getEntityId() < rhs->getEntityId()) return true;
        if(lhs->getEntityId() > rhs->getEntityId()) return false;

        const ScopeNameValueMap& indexMapRowA = lhs->getKeyScopeNameValueMap();
        const ScopeNameValueMap& indexMapRowB = rhs->getKeyScopeNameValueMap();

        ScopeNameValueMap::const_iterator itRowA = indexMapRowA.begin();
        ScopeNameValueMap::const_iterator itRowAEnd = indexMapRowA.end();
        for (; itRowA != itRowAEnd; ++itRowA)
        {
            const char8_t* scopeName = itRowA->first;

            ScopeNameValueMap::const_iterator itRowB = indexMapRowB.find(scopeName);
            if (itRowB != indexMapRowB.end())
            {
                if (itRowA->second < itRowB->second)
                    return true;

                if (itRowA->second > itRowB->second)
                    return false;
            }
        }

        return false;
    }
};

/*************************************************************************************************/
/*!
    \brief validateRequest

    Validates the UpdateStatsRequest needed by fetchStats() and commitStats() to ensure that the 
    stat update request is valid.

    \param[in]  strict - enforce valid request inputs.
 
    \return - corresponding error code
*/
/*************************************************************************************************/
BlazeRpcError StatsTransactionContext::validateRequest(bool strict)
{
    // Sort stat updates so that locks we acquire during updating will be in a consistent order
    // which should help us avoid any deadlocks
    eastl::sort(mIncomingRequest.begin(), mIncomingRequest.end(), statRowUpdateLess());//mRequest.getStatUpdates()

    // Go through each row of requested updates and validate them
    UpdateStatsRequest::StatRowUpdateList::iterator updateIter = mIncomingRequest.begin(); 
    while (updateIter != mIncomingRequest.end())
    {
        StatRowUpdate* rowUpdate = *updateIter;

        // First check that the category exists
        const char8_t* categoryName = rowUpdate->getCategory();
        CategoryMap::const_iterator catIter = mComp->getConfigData()->getCategoryMap()->find(categoryName);
        if (catIter == mComp->getConfigData()->getCategoryMap()->end())
        {
            ERR_LOG("[StatsTransactionContext].validateRequest(): Could not update stats for unknown category " << categoryName);

            if (!strict)
            {
                // Remove the culprit rather than discarding the report.
                updateIter = mIncomingRequest.erase(updateIter);//
                continue;
            }

            return STATS_ERR_UNKNOWN_CATEGORY;
        }
        const StatCategory* cat = catIter->second;

        // for admin requests, check if the period type is supported
        if (!rowUpdate->getPeriodTypes().empty())
        {
            PeriodTypeList::const_iterator pi = rowUpdate->getPeriodTypes().begin();
            PeriodTypeList::const_iterator pe = rowUpdate->getPeriodTypes().end();
            for (; pi != pe; ++pi)
            {
                if (!cat->isValidPeriod(*pi))
                {
                    ERR_LOG("[StatsTransactionContext].validateRequest(): Category " << cat->getName() << " does not support period type " 
                            << *pi << ".");
                    return STATS_ERR_BAD_PERIOD_TYPE;
                }
            }
        }

        // Check if the scope in the row update is supported by the category and the index is valid
        const ScopeNameValueMap& indexMapRow = rowUpdate->getKeyScopeNameValueMap();
        const ScopeNameSet* scopeNameSet = cat->getScopeNameSet();
        if (scopeNameSet && (scopeNameSet->size() != indexMapRow.size()))
        {
            ERR_LOG("[StatsTransactionContext].validateRequest(): Wrong number of scopes provided for category " << cat->getName() << ".");
            return STATS_ERR_BAD_SCOPE_INFO;
        }

        if ((scopeNameSet == nullptr) && !indexMapRow.empty())
        {
            ERR_LOG("[StatsTransactionContext].validateRequest(): Category " << cat->getName() << " does not support keyscopes.");
            return STATS_ERR_BAD_SCOPE_INFO;
        }

        ScopeNameValueMap::const_iterator it = indexMapRow.begin();
        ScopeNameValueMap::const_iterator itend = indexMapRow.end();
        for (; it != itend; ++it)
        {
            ScopeName scopeName = it->first;
            if (!cat->isValidScopeName(scopeName.c_str()))
            {
                ERR_LOG("[StatsTransactionContext].validateRequest(): Invalid scope name " << scopeName.c_str() << " for category " 
                        << cat->getName() << ".");
                return STATS_ERR_BAD_SCOPE_INFO;
            }

            if (!mComp->getConfigData()->isValidScopeValue(scopeName.c_str(), it->second))
            {
                ERR_LOG("[StatsTransactionContext].validateRequest(): Invalid scope value index " << it->second << " for scope name " 
                        << scopeName.c_str() << ".");
                return STATS_ERR_BAD_SCOPE_INFO;
            }

            // the aggregate key value is a valid scope value, but the request shouldn't be
            // providing it--the aggregate is perform implicitly by the server
            if (mComp->getConfigData()->getAggregateKeyValue(scopeName.c_str()) == it->second)
            {
                ERR_LOG("[StatsTransactionContext].validateRequest(): " << it->second << " is reserved aggregate key value for scope name " 
                        << scopeName.c_str() << ".");
                return STATS_ERR_BAD_SCOPE_INFO;
            }
        }

        // Finally check that each stat exists in the category
        StatRowUpdate::StatUpdateList::iterator statUpdateIter = rowUpdate->getUpdates().begin();
        while (statUpdateIter != rowUpdate->getUpdates().end())
        {
            StatUpdate* update = *statUpdateIter;

            StatMap::const_iterator statIter = cat->getStatsMap()->find(update->getName());
            if (statIter == cat->getStatsMap()->end())
            {
                ERR_LOG("[StatsTransactionContext].validateRequest(): Invalid stat " << update->getName() << " for category " 
                        << cat->getName() << ".");
                if (!strict)
                {
                    // Remove the culprit rather than discarding the report.
                    statUpdateIter = rowUpdate->getUpdates().erase(statUpdateIter);
                    continue;
                }
                return STATS_ERR_STAT_NOT_FOUND;
            }

            int32_t updateType = update->getUpdateType();
            if (updateType < 0 || updateType >= STAT_NUM_UPDATE_TYPES)
            {
                ERR_LOG("[StatsTransactionContext].validateRequest(): Invalid update type " << update->getUpdateType());
                return STATS_ERR_INVALID_UPDATE_TYPE;
            }

            const Stat* stat = statIter->second;
            if ((stat->getDbType().getType() == STAT_TYPE_FLOAT) && (updateType == STAT_UPDATE_TYPE_OR || updateType == STAT_UPDATE_TYPE_AND))
            {
                ERR_LOG("[StatsTransactionContext].validateRequest(): Invalid update type " << updateType << " for float stat " 
                        << stat->getName() << ".");
                return STATS_ERR_INVALID_UPDATE_TYPE;
            }

            if ((stat->getDbType().getType() == STAT_TYPE_STRING) && (updateType != STAT_UPDATE_TYPE_ASSIGN && updateType != STAT_UPDATE_TYPE_CLEAR))
            {
                ERR_LOG("[StatsTransactionContext].validateRequest(): Invalid update type " << updateType << " for string stat " 
                        << stat->getName() << ".");
                return STATS_ERR_INVALID_UPDATE_TYPE;
            }

            ++statUpdateIter;
        }

        ++updateIter;
        
        //check if there are multiple stat updates for the same entity id.  Since the rows are sorted, we can
        // compare against the next row to see if they are the same

        if (updateIter != mIncomingRequest.end()) //updateItr has already been incremented
        {
            StatRowUpdate* nextRow = *updateIter;
            ScopeNameValueMap& scopes = rowUpdate->getKeyScopeNameValueMap();
            ScopeNameValueMap& nextScopes = nextRow->getKeyScopeNameValueMap();

            if ((rowUpdate->getEntityId() == nextRow->getEntityId()) &&
                (blaze_strcmp(rowUpdate->getCategory(), nextRow->getCategory()) == 0) &&
                (scopes.size() == nextScopes.size()))
            {
                bool areScopesSame = true;
                ScopeNameValueMap::const_iterator scopeItr = scopes.begin();
                ScopeNameValueMap::const_iterator scopeEnd = scopes.end();
                for (; scopeItr != scopeEnd; ++ scopeItr)
                {
                    ScopeNameValueMap::const_iterator nextScopeItr = nextScopes.find(scopeItr->first);
                    if ((nextScopeItr == nextScopes.end()) || (scopeItr->second != nextScopeItr->second))
                    {
                        areScopesSame = false;
                        break;
                    }
                }

                // NOTE: Period type validation is ignored here because usual client requests won't specify period types.
                // Admin requests can specify period types, but we're expecting only one row update at a time; so period
                // type validation can still be ignored (for now).

                if (areScopesSame)
                {
                    ERR_LOG("[StatsTransactionContext].validateRequest(): Update stats request contains multiple updates for the "
                        "same row which is not supported. Category: " << rowUpdate->getCategory());
                    return STATS_ERR_INVALID_UPDATE_TYPE;
                }
            }
        }
    }

    return ERR_OK;
}

void StatsTransactionContext::initializeStatsProvider()
{
    if (mStatsProvider != nullptr)
    {
        delete mStatsProvider;
        mStatsProvider = nullptr;
    }

    if (mProcessGlobalStats)
    {
        mStatsProvider = (StatsProvider *) (BLAZE_NEW GlobalStatsProvider(*mComp, *mComp->getGlobalStatsCache()));
    }
    else
    {
        //
        // GLOBAL STATS =>
        // take out global stat updates and pack them into broadcast
        // when not processing global updates
        //
        UpdateStatsRequest::StatRowUpdateList &updateList = mIncomingRequest;
        UpdateStatsRequest::StatRowUpdateList::iterator it = updateList.begin();
        while(it != updateList.end())
        {
            const char *categoryName = (*it)->getCategory(); 
            // is this category global category?
            CategoryMap::const_iterator catIter = mComp->getConfigData()->getCategoryMap()->find(categoryName);    
            if (catIter != mComp->getConfigData()->getCategoryMap()->end() && catIter->second->getCategoryType() == CATEGORY_TYPE_GLOBAL)
            {
                // place update into broadcast's request
                StatRowUpdate *globalStatsRowUpdate = *it;
                StatRowUpdate *rowUpdate = mGlobalStats.getStatUpdates().pull_back();
                globalStatsRowUpdate->copyInto(*rowUpdate);

                it = updateList.erase(it);
            }
            else
            {
                it++;
            }
        }

        mStatsProvider = (StatsProvider *)(BLAZE_NEW DbStatsProvider(*mComp));
    }
}

/*************************************************************************************************/
/*!
    \brief fetchStats

    Build the SELECT FOR statements and execute the queries to retrieve the requested stats and
    to lock rows for future update.
 
    \return - corresponding database error code
*/
/*************************************************************************************************/
BlazeRpcError StatsTransactionContext::fetchStats()
{
    if ((mUpdateState & FETCHED) != 0) // only allow to fetch once
    {
        // fetchStats will be called again, after CalcDerivedStats Command is done. 
        return ERR_OK;
    }

    if (mIncomingRequest.empty() && ! mGlobalStats.getStatUpdates().empty())
    {
        // nothing to fetch in this case, it's only global stats update being passed through to master
        return ERR_OK;
    }

    if (mIncomingRequest.empty() ||  
        (mUpdateState & INITIALIZED) == 0)
    {
        return ERR_SYSTEM;
    }

    BlazeRpcError err = ERR_SYSTEM;
    uint16_t retries = 0;
    while (err != ERR_OK)
    {
        clear();
        initializeStatsProvider();

        // Make sure we get the up-to-date period ids each time through this loop, which ensures that in the (hopefully)
        // rare case of a deadlock we are always using the latest values for the period ids.
        mComp->getPeriodIds(mPeriodIds);

        // This is the beginning of the first of three main steps in order to update stats - build up a (potentially large)
        // multi-statement query to fetch all the relevant rows that will be updated during the processing of the request.
        // We lock either the rows themselves, or the index gaps in which they fall, to ensure that no other request is
        // overlapping ours by even a single row.
        UpdateStatsRequest::StatRowUpdateList& updates = mIncomingRequest; 
        mScopesVectorForRowsVector.reserve(updates.size());
        for (UpdateStatsRequest::StatRowUpdateList::iterator ui = updates.begin(), ue = updates.end(); ui != ue; ++ui)
        {
            StatRowUpdate* update = *ui;

            // FIFA SPECIFIC CODE START
            //HACK: do not attempt to write any stats category for SoloGames/UserGameCount_StatCategory which has entityId = 0 to eliminate frequent db locking/contention
            // We actually have to report a ERR_OK to allow the game report be consumed by BMR, or else BMR would think the gamereport process failed.
            if (update != NULL && update->getEntityId() == 0 && (blaze_strcmp(update->getCategory(), "SoloGameStats") == 0 || blaze_strcmp(update->getCategory(), "UserGameCount_StatCategory") == 0))
            {
                return ERR_SYSTEM;
            }
            // FIFA SPECIFIC CODE END

            // use wrapper because sub objects must be destroyed
            ScopeNameValueSetMapWrapper nameValueSetMap;
            ScopesVectorForRows* scopes = nullptr;

            // If keyscopes are involved, build up a couple of helper objects - the nameValueSetMap is a compact mapping
            // of keyscope names to the set of values for that keyscope that need to be fetched, while scopes is a vector of
            // every unique combination of keyscopes that can be made from the same data
            if (!update->getKeyScopeNameValueMap().empty())
            {
                scopes = &(mScopesVectorForRowsVector.push_back());
                getRowsToUpdateForOnePeriod(update->getKeyScopeNameValueMap(), nameValueSetMap, *scopes);
            }

            const StatCategory* cat = mComp->getConfigData()->getCategoryMap()->find(update->getCategory())->second;
            for (int32_t period = 0; period < STAT_NUM_PERIODS; ++period)
            {
                if (!update->getPeriodTypes().empty())
                {
                    // admin request
                    PeriodTypeList::const_iterator pi = update->getPeriodTypes().begin();
                    PeriodTypeList::const_iterator pe = update->getPeriodTypes().end();
                    for (; pi != pe; ++pi)
                    {
                        if (*pi == period)
                        {
                            // this period type requested
                            break;
                        }
                    }
                    if (pi == pe)
                    {
                        // this period type not requested; skip it
                        continue;
                    }
                }

                if (!cat->isValidPeriod(period))
                    continue;

                const int32_t periodId = mPeriodIds[period];

                // Keep track of the data structures we have just built in the helper, we will associate provider results
                // with this later.  
                mUpdateHelper.buildRow(cat, update, period, scopes);

                err = mStatsProvider->addStatsFetchForUpdateRequest(
                    *cat, update->getEntityId(), period, periodId, nameValueSetMap);
                if (err != ERR_OK)
                    return err;
            }
        }

        TRACE_LOG("[StatsTransactionContext].fetchStats: prepared select query");

        // Now begins the second main part of updating stats - executing the fetch query and validating the results
        err = mStatsProvider->executeFetchForUpdateRequest();
        if (err == ERR_DB_LOCK_DEADLOCK)
        {
            if (retries > StatsSlaveImpl::MAX_FETCH_RETRY)
            {
                ERR_LOG("[StatsTransactionContext].fetchStats: DB error: <" << getDbErrorString(err) << "> Too many retries, giving up");
                abortTransaction();
                return STATS_ERR_DB_QUERY_FAILED;
            }
            INFO_LOG("[StatsTransactionContext].fetchStats: DB error: <" << getDbErrorString(err) << "> while fetching stats, will retry");
            mStatsProvider->rollback();
            ++retries;
            continue;
        }
        else if (err != ERR_OK)
        {
            // Abort the transaction to release any DB locks we may have obtained on any potential results we fetched
            ERR_LOG("[StatsTransactionContext].fetchStats: failed to fetch stats for update <" << getDbErrorString(err) << ">. Will rollback.");
            abortTransaction();
            return err;
        }

        // Generally, the number of resultsets is as many as there are number of update rows.
        // Specificaly for db provider each row results in 1 SELECT FOR UPDATE query issued to 
        // the DB, thereby creating 1 resultsets
        uint32_t receivedResults = static_cast<uint32_t>(mStatsProvider->getResultsetCount());
        uint32_t expectedResults = static_cast<uint32_t>(mUpdateHelper.getMap()->size());
        if (expectedResults != receivedResults || receivedResults < 1)
        {
            ERR_LOG("[StatsTransactionContext].fetchStats: Wrong number of resultsets from the stats provider, expected(" << expectedResults 
                    << "), received(" << receivedResults << "). Will rollback.");
            // Abort the transaction to release any DB locks we may have obtained on any potential results we fetched
            abortTransaction();
            return ERR_SYSTEM;
        }

        //
        // Initialize and allocate the stat map.
        //
        // For each SELECT FOR UPDATE that returned no results (meaning that row doesn't exist), we make
        // an initial INSERT of default values to grab an exclusive lock on the row as soon as possible.
        // The subsequent commit later on will be all UPDATEs (rather than a mix of INSERTs and UPDATEs).
        bool needInsert = false;
        // Note that we are iterating over the stat row updates and periods in the same order as was done when
        // building the provider request.  We therefore assume that by iterating in exactly the same order here,
        // that the update result row and the mapped memory row will match up.
        // If the provider were to return results out of order from what was requested, this would need to be addressed.
        UpdateRowMap::iterator helperRowItr = mUpdateHelper.getMap()->begin();
        while (mStatsProvider->hasNextResultSet())
        {
            const StatsProviderResultSet *result = mStatsProvider->getNextResultSet();
            UpdateRow& row = helperRowItr->second;
            uint32_t size = static_cast<uint32_t>(result->size());
            TRACE_LOG("[StatsTransactionContext].fetchStats: Found " << size << " results in this result");
            row.setProviderResultSet(result);

            //
            // Setup the primary key's row in the helper's stat map for this result set
            //
            const StatsProviderRow* provRow = row.getProviderRow();

            // find the primary/default keyscopes vector (if any)
            const UpdateRowKey& key = helperRowItr->first;
            const RowScopesVector* defRowScopes = nullptr;
            if (row.getScopes() != nullptr)
            {
                ScopesVectorForRows::const_iterator si = row.getScopes()->begin();
                ScopesVectorForRows::const_iterator se = row.getScopes()->end();
                for (; si != se; ++si)
                {
                    if (key.isDefaultScopeIndex(*si))
                    {
                        defRowScopes = &(*si);
                        break;
                    }
                }
                // never should happen
                if (defRowScopes == nullptr)
                {
                    ERR_LOG("[StatsTransactionContext].fetchStats: invalid ScopesVectorForRows. Will rollback.");
                    // Abort the transaction to release any DB locks we may have obtained on any potential results we fetched
                    abortTransaction();
                    return ERR_SYSTEM;
                }
            }

            // Prepopulate the stat map with the defaults if we didn't find a row,
            // as every derived stat formula should be triggered in this case
            StatValMap& statValMap = row.getNewStatValMap();

            const StatCategory* cat = row.getCategory();
            StatMap::const_iterator statItr = cat->getStatsMap()->begin();
            StatMap::const_iterator statEnd = cat->getStatsMap()->end();

            if (provRow == nullptr)
            {
                // prepopulate with defaults because we didn't find a result row
                for (; statItr != statEnd; ++statItr)
                {
                    const Stat* stat = statItr->second;

                    if (statValMap.find(stat->getName()) != statValMap.end())
                    {
                        // never should happen
                        WARN_LOG("[StatsTransactionContext].fetchStats: <" << stat->getName()
                            << "> duplicate stat of primary key -- add default value ignored");
                        continue;
                    }

                    StatVal* val = BLAZE_NEW StatVal();
                    val->changed = true;

                    if (stat->getDbType().getType() == STAT_TYPE_INT)
                    {
                        val->intVal = stat->getDefaultIntVal();
                        val->type = 0;
                    }
                    else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                    {
                        val->floatVal = stat->getDefaultFloatVal();
                        val->type = 1;
                    }
                    else if (stat->getDbType().getType() == STAT_TYPE_STRING)
                    {
                        val->stringVal = stat->getDefaultStringVal();
                        val->type = 2;
                    }
                    statValMap[stat->getName()] = val;
                }

                err = mStatsProvider->addDefaultStatsInsertForUpdateRequest(
                    *cat, key.entityId, key.period, mPeriodIds[key.period], defRowScopes);
                if (err != ERR_OK)
                {
                    // Abort the transaction to release any DB locks we may have obtained on any potential results we fetched
                    ERR_LOG("[StatsTransactionContext].fetchStats: failed to insert default stats <" << getDbErrorString(err) << ">. Will rollback.");
                    abortTransaction();
                    return err;
                }

                needInsert = true;
            }
            else
            {
                for (; statItr != statEnd; ++statItr)
                {
                    const Stat* stat = statItr->second;

                    if (statValMap.find(stat->getName()) != statValMap.end())
                    {
                        // never should happen
                        WARN_LOG("[StatsTransactionContext].fetchStats: <" << stat->getName()
                            << "> duplicate stat of primary key -- add fetched value ignored");
                        continue;
                    }

                    StatVal* val = BLAZE_NEW StatVal();

                    if (stat->getDbType().getType() == STAT_TYPE_INT)
                    {
                        val->intVal = provRow->getValueInt64(stat->getName());
                        val->type = 0;
                    }
                    else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                    {
                        val->floatVal = provRow->getValueFloat(stat->getName());
                        val->type = 1;
                    }
                    else if (stat->getDbType().getType() == STAT_TYPE_STRING)
                    {
                        val->stringVal = provRow->getValueString(stat->getName());
                        val->type = 2;
                    }
                    statValMap[stat->getName()] = val;
                }
            }

            //
            // Setup any aggregate keys' rows in the helper's stat map for this result set
            //
            if (row.getScopes() != nullptr)
            {
                ScopesVectorForRows::const_iterator si = row.getScopes()->begin();
                ScopesVectorForRows::const_iterator se = row.getScopes()->end();
                for (; si != se; ++si)
                {
                    const RowScopesVector* aggRowScopes = &(*si);
                    if (aggRowScopes == defRowScopes)
                        continue;

                    UpdateAggregateRowMap::iterator aggValIter = mUpdateHelper.getStatValMapIter(key, *aggRowScopes, cat);
                    StatValMap& statValMapTemp = aggValIter->second;
                    if (aggValIter->first.mStatus != AGG_NEW)
                        continue;

                    // just created, check if provider has data for it
                    // if not set to zero
                    StatsProviderRowPtr provRowTemp = row.getProviderResultSet()->getProviderRow(key.entityId, mPeriodIds[key.period], aggRowScopes);
                    if (provRowTemp == nullptr)
                    {
                        aggValIter->first.mStatus = AGG_INSERT;
                        for (statItr = cat->getStatsMap()->begin(); statItr != statEnd; ++statItr)
                        {
                            const Stat* stat = statItr->second;
                            StatVal* statVal = nullptr;
                            StatValMap::const_iterator statMapItr = statValMapTemp.find(stat->getName());
                            if (statMapItr == statValMapTemp.end())
                            {
                                statVal = BLAZE_NEW StatVal();
                                statValMapTemp[stat->getName()] = statVal;
                            }
                            else
                            {
                                // never should happen
                                WARN_LOG("[StatsTransactionContext].fetchStats: <" << stat->getName()
                                    << "> duplicate stat of aggregate key -- resetting to 0 value");
                                statVal = statMapItr->second;
                            }
                            statVal->changed = true;
                            switch (stat->getDbType().getType())
                            {
                            case STAT_TYPE_INT:
                                statVal->intVal = 0;
                                statVal->type = 0;
                                break;
                            case STAT_TYPE_FLOAT:
                                statVal->floatVal = 0.0;
                                statVal->type = 1;
                                break;
                            case STAT_TYPE_STRING:
                                statVal->stringVal = nullptr;
                                statVal->type = 2;
                                break;
                            default:
                                break;
                            }
                        }

                        err = mStatsProvider->addDefaultStatsInsertForUpdateRequest(
                            *cat, key.entityId, key.period, mPeriodIds[key.period], aggRowScopes);
                        if (err != ERR_OK)
                        {
                            // Abort the transaction to release any DB locks we may have obtained on any potential results we fetched
                            ERR_LOG("[StatsTransactionContext].fetchStats: failed to insert default stats <" << getDbErrorString(err) << ">. Will rollback.");
                            abortTransaction();
                            return err;
                        }

                        needInsert = true;
                    }
                    else // we have data
                    {
                        aggValIter->first.mStatus = AGG_UPDATE;
                        for (statItr = cat->getStatsMap()->begin(); statItr != statEnd; ++statItr)
                        {
                            const Stat* stat = statItr->second;
                            StatVal* statVal = nullptr;
                            StatValMap::const_iterator statMapItr = statValMapTemp.find(stat->getName());
                            if (statMapItr == statValMapTemp.end())
                            {
                                statVal = BLAZE_NEW StatVal();
                                statValMapTemp[stat->getName()] = statVal;
                            }
                            else
                            {
                                // never should happen
                                WARN_LOG("[StatsTransactionContext].fetchStats: <" << stat->getName()
                                    << "> duplicate stat of aggregate key -- resetting to fetched value");
                                statVal = statMapItr->second;
                            }
                            switch (stat->getDbType().getType())
                            {
                            case STAT_TYPE_INT:
                                statVal->intVal = provRowTemp->getValueInt64(stat->getName());
                                statVal->type = 0;
                                break;
                            case STAT_TYPE_FLOAT:
                                statVal->floatVal = provRowTemp->getValueFloat(stat->getName());
                                statVal->type = 1;
                                break;
                            case STAT_TYPE_STRING:
                                statVal->stringVal = provRowTemp->getValueString(stat->getName());
                                statVal->type = 2;
                                break;
                            default:
                                break;
                            }
                        }
                    }
                } // for each keyscope
            } // if any keyscopes

            // remember to increment the row iterator for update helper
            ++helperRowItr;
        }

        if (!needInsert)
            break;

        err = mStatsProvider->executeUpdateRequest();
        if (err == ERR_DB_DUP_ENTRY || err == ERR_DB_LOCK_DEADLOCK)
        {
            if (retries > StatsSlaveImpl::MAX_FETCH_RETRY)
            {
                ERR_LOG("[StatsTransactionContext].fetchStats: DB error: <" << getDbErrorString(err) << "> Too many retries, giving up");
                abortTransaction();
                return STATS_ERR_DB_QUERY_FAILED;
            }
            INFO_LOG("[StatsTransactionContext].fetchStats: DB error: <" << getDbErrorString(err) << "> while inserting default stats, will retry");
            if (mStatsProvider != nullptr) 
                mStatsProvider->rollback();
            ++retries;
        }
        else if (err != ERR_OK)
        {
            ERR_LOG("[StatsTransactionContext].fetchStats: failed to execute request <" << getDbErrorString(err) << ">. Will rollback.");
            abortTransaction();
            return err;
        }
    } // retry loop

    mUpdateState |= FETCHED;

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief getRowsToUpdateForOnePeriod

    Translates a simple scope map provided by the caller to update stats into expanded data
    structures.  The incoming map is a simple collection of scope key/value pairs - each scope name
    mapping to a single scope value.  This method fills in the nameValueSetMap with the same
    scope name keys, but the map values contain the set of all scope values that will need to be
    fetched from the provider (the requested value plus any aggregates encompassing that value).  Next
    this method delegates to getRowScopeSettings to translate nameValueSetMap which is a
    more compact representation into scopeVectorForRows, a vector who's elements represent each
    possible combination of scope name/value pairs.  The number of elements in scopeVectorForRows
    is thus the number of database rows that will ultimately be updated (or inserted).

    Example: Suppose there are two keyscopes, defined as following in the config file.
        level = { jungle = 1, city = 2, desert = 3, hardmaps = {id = 4, values = [jungle, desert]}}
        map = { london = 1, paris = 2, new_york = 3}
    Suppose further that the caller passes in level = 3 and map = 2 in the request.  The
    nameValueSetMap would be a map of two entries: level -> (0, 3, 4), and map -> (0, 2).
    The scopeVectorForRows would be a vector of 6 entries, each being one possible combination
    of the 3 level * 2 map combos, i.e. (level = 0, map = 0), (level = 0, map = 2),
    (level = 3, map = 0), (level = 3, map = 2), (level = 4, map = 0), (level = 4, map = 2).

    \param[in]updateStatIndexRow - index map used for the table
    \param[out]nameValueSetMap - scope name - index set map, caller needs to free the memory after using
    \param[out]scopeVectorForRows - all combinations of name/index need to be update for all rows based on the
        index map, caller needs to free the memory
*/
/*************************************************************************************************/
void StatsTransactionContext::getRowsToUpdateForOnePeriod(const ScopeNameValueMap& updateStatIndexRow,
                                                    ScopeNameValueSetMap& nameValueSetMap,
                                                    ScopesVectorForRows& scopeVectorForRows) const
{
    ScopeNameValueMap::const_iterator it = updateStatIndexRow.begin();
    ScopeNameValueMap::const_iterator itend = updateStatIndexRow.end();
    for (; it != itend; ++it)
    {
        ScopeValueSet* valueSet = BLAZE_NEW ScopeValueSet();

        // insert itself
        valueSet->insert(it->second);

        // insert aggregate
        ScopeValue scopeValue = mComp->getConfigData()->getAggregateKeyValue(it->first.c_str());
        if (scopeValue >= 0)
        {
            valueSet->insert(scopeValue);
        }

        nameValueSetMap.insert(eastl::make_pair(it->first, valueSet));
    }

    getRowScopeSettings(nameValueSetMap, scopeVectorForRows);
}

/*************************************************************************************************/
/*!
    \brief getRowScopeSettings

    Populates a vector of vectors of key scope name value pairs.  Each entry in the main vector
    is one unique combination of scopes from the more compact representation in the input
    nameValueSetMap.  See getRowsToUpdateForOnePeriod for more details on the return value.

    \param[in]  nameValueSetMap - key scope name and index set
    \param[out] scopeVectorForRows - all combinations of name/index which need to be updated for
                                     all rows based on the index map, caller needs to free the memory
*/
/*************************************************************************************************/
void StatsTransactionContext::getRowScopeSettings(const ScopeNameValueSetMap& nameValueSetMap,
                                            ScopesVectorForRows& scopeVectorForRows) const
{
    scopeVectorForRows.clear();

    uint32_t sizeContainer = (uint32_t)nameValueSetMap.size();

    if (nameValueSetMap.size() == 1)
    {
        // single entry, no need to combine
        ScopeNameValueSetMap::const_iterator it = nameValueSetMap.begin();
        ScopeNameValueSetMap::const_iterator itend = nameValueSetMap.end();
        ScopeValueSet* valueSet = it->second;
        size_t size = valueSet->size();
        scopeVectorForRows.reserve(size);
        for (size_t i = 0; i < size; i++)
        {
            RowScopesVector& rowScopesVector = scopeVectorForRows.push_back();
            for (; it != itend; ++it)
            {
                rowScopesVector.push_back();
                ScopeNameIndex& nameIndex = rowScopesVector.back();
                nameIndex.mScopeName = it->first;
                nameIndex.mScopeValue = (*it->second)[i];
            }
            it = nameValueSetMap.begin();
        }
    }
    else
    {
        // let's combine all values in all data sets

        // two arrays helped to get all combinations
        uint32_t* tracking = BLAZE_NEW_ARRAY(uint32_t, sizeContainer);
        uint32_t* radix = BLAZE_NEW_ARRAY(uint32_t, sizeContainer);
        uint32_t numOfCombinations = 1;

        // below two vectors, name and index set needs to be in same order!!
        eastl::vector<ScopeValueSet*> indexValueSetContainer;
        eastl::vector<ScopeName>  scopeNameContainer;

        ScopeNameValueSetMap::const_iterator it = nameValueSetMap.begin();
        ScopeNameValueSetMap::const_iterator itend = nameValueSetMap.end();

        uint32_t i = 0;
        indexValueSetContainer.reserve(nameValueSetMap.size());
        scopeNameContainer.reserve(nameValueSetMap.size());
        for (; it != itend; ++it)
        {
            ScopeValueSet* valueSet = it->second;
            tracking[i] = 0;
            radix[i] = (uint32_t)valueSet->size();

            numOfCombinations = numOfCombinations * radix[i];

            indexValueSetContainer.push_back(valueSet);
            scopeNameContainer.push_back(it->first);

            i++;
        }

        // reserve space for columnsVector
        scopeVectorForRows.reserve(numOfCombinations);

        do
        {
            RowScopesVector& rowScopesVector = scopeVectorForRows.push_back();
            rowScopesVector.reserve(sizeContainer);
            for (uint32_t pos = 0; pos < sizeContainer; pos++)
            {
                rowScopesVector.push_back();
                ScopeNameIndex& scopeNameIndex = rowScopesVector.back();
                scopeNameIndex.mScopeName = scopeNameContainer[pos];
                scopeNameIndex.mScopeValue = (*(indexValueSetContainer[pos]))[tracking[pos]];
            }
        }
        while (getNextCombineIndex(tracking, radix, sizeContainer));

        delete []tracking;
        delete []radix;
    }
}

/*! ************************************************************************************************/
/*!
    \brief getNextCombineIndex

    Get index in each data set needs to be combined for next combination

    \param[in] tracking[] - an uint32_t array which holds data set index for each combination
    \param[in] radix[] - an uint32_t array which holds maximum size for each index in array tracking,
    each element is actually the size of each data set
    \param[in] numOfDataset - the number of data sets to be combined

    \return true means there is still next combination to get, false means we've got all combinations
    based on the given radix[]
*/
/***************************************************************************************************/
bool StatsTransactionContext::getNextCombineIndex(uint32_t tracking[], uint32_t radix[], 
                                               uint32_t numOfDataset) const
{
    for (int i = numOfDataset - 1; i >= 0; i--)
    {
        if (tracking[i] < (radix[i] - 1))
        {
            tracking[i]++;
            break;
        }

        if (tracking[i] == (radix[i] - 1))
        {
            if ( i == 0)
                return false;

            tracking[i] = 0;
        }
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief calcStatsChange

    Wrapper method to trigger the core and/or derived stats computation.

    \param[in]  coreStatsChange - true if caller wishes to update non-derived stats
    \param[in]  derivedStatsChange - true if caller wishes to update derived stats

    \return - corresponding database error code
*/
/*************************************************************************************************/
BlazeRpcError StatsTransactionContext::calcStatsChange(bool coreStatsChange, bool derivedStatsChange)
{
    if (!coreStatsChange && !derivedStatsChange)
        return ERR_OK;

    // fetch stats if it is not done before
    if ((mUpdateState & FETCHED) == 0)
    {
        BlazeRpcError err = fetchStats();
        if (err != ERR_OK)
            return err;
    }

    if (coreStatsChange)
    {
        if ((mUpdateState & APPLIED_UPDATES) != 0)
        {
            ERR_LOG("[StatsTransactionContext].calcStatsChange(): Core stats update being applied multiple times to the same stat transaction. This may cause unexpected stat calculations.");
            return ERR_SYSTEM;
        }

        // iterate all temporary rows (1 row per period)
        for (UpdateRowMap::iterator i = mUpdateHelper.getMap()->begin(), e = mUpdateHelper.getMap()->end(); i != e; ++i)
        {
            updateCoreStatsInRow(i->second);
        }
        mUpdateState |= APPLIED_UPDATES;
    }

    if (derivedStatsChange)
    {
        if ((mUpdateState & APPLIED_DERIVED) != 0)
        {
            TRACE_LOG("[StatsTransactionContext].calcStatsChange(): Derived stats calculation being applied multiple times to the same stat transaction. This may cause unexpected stat calculations.");
        }

        updateDerivedStatsInRows();
        mUpdateState |= APPLIED_DERIVED;
    }

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief updateCoreStatsInRow

    Iterate through all stats in a row and evaluate any updates for non-derived stats.

    \param[in]  row - the row to update
*/
/*************************************************************************************************/
void StatsTransactionContext::updateCoreStatsInRow(UpdateRow& row) const
{
    // Do the actual work - looking at individual stats, their update type,
    // and value, and go compute the updated value
    StatValMap& statValMap = row.getNewStatValMap();
    const StatRowUpdate::StatUpdateList* updates = row.getUpdates();
    StatRowUpdate::StatUpdateList::const_iterator updateIter = updates->begin();
    StatRowUpdate::StatUpdateList::const_iterator updateEnd = updates->end();
    while (updateIter != updateEnd)
    {
        const StatUpdate* update = *updateIter++;
        const Stat* stat = row.getCategory()->getStatsMap()->find(update->getName())->second;
        if (stat->isDerived())
        {
            WARN_LOG("[StatsTransactionContext].updateCoreStatsInRow(): ignoring attempt to manually update derived stat " << stat->getName());
            continue;
        }
        evaluateStats(update, stat, statValMap, row.getModifiedStatValueMap());
    }
}

/*************************************************************************************************/
/*!
    \brief updateDerivedStatsInRows

    Update all derived stats for all non-aggregate rows.  As opposed to the non-derived stats
    which can be done independently, derived stats may have cross-category dependencies.
*/
/*************************************************************************************************/
void StatsTransactionContext::updateDerivedStatsInRows()
{
    bool extraneousDependency = false;
    CategoryUpdateRowMap::const_iterator iter = mUpdateHelper.getCategoryMap()->begin();
    CategoryUpdateRowMap::const_iterator end = mUpdateHelper.getCategoryMap()->end();
    for (; iter != end; ++iter)
    {
        const CategoryUpdateRowKey& key = iter->first;
        const DependencySet& depSet = iter->second;

        for (DependencySet::const_iterator depIterTemp = depSet.begin(); depIterTemp != depSet.end(); ++depIterTemp)
        {
            const CategoryDependency* catDep = *depIterTemp;

            for (CategoryStatList::const_iterator derivedIter = catDep->catStatList.begin(); derivedIter != catDep->catStatList.end(); ++derivedIter)
            {
                const CategoryStat& catStat = *derivedIter;
                UpdateRowKey rowKey(catStat.categoryName, key.entityId, key.period, key.scopeNameValueMap);
                UpdateRowMap::iterator updateRowItr = mUpdateHelper.getMap()->find(rowKey);
                if (updateRowItr == mUpdateHelper.getMap()->end())
                {
                    TRACE_LOG("[StatsTransactionContext].updateDerivedStatsInRows: extraneous dependency of category " << rowKey.category
                        << ", entity " << rowKey.entityId << ", period type " << rowKey.period); // expecting that keyscopes aren't necessary in this trace
                    extraneousDependency = true;
                    continue;
                }
                UpdateRow& row = updateRowItr->second;

                bool modified = false;
                const StatCategory* cat = mComp->getConfigData()->getCategoryMap()->find(catStat.categoryName)->second;
                const Stat* stat = cat->getStatsMap()->find(catStat.statName)->second;
                for (DependencyMap::const_iterator depIter = stat->getDependencies()->begin();
                    depIter != stat->getDependencies()->end(); ++depIter)
                {
                    UpdateRowKey rowKey2(depIter->first, key.entityId, key.period, key.scopeNameValueMap);
                    UpdateRowMap::const_iterator updateRowItr2 = mUpdateHelper.getMap()->find(rowKey2);
                    if (updateRowItr2 == mUpdateHelper.getMap()->end())
                    {
                        TRACE_LOG("[StatsTransactionContext].updateDerivedStatsInRows: extraneous dependency of category " << rowKey2.category
                            << ", entity " << rowKey2.entityId << ", period type " << rowKey2.period); // expecting that keyscopes aren't necessary in this trace
                        extraneousDependency = true;
                        continue;
                    }
                    const UpdateRow& row2 = updateRowItr2->second;
                    for (StatSet::const_iterator statIter = depIter->second.begin(); statIter != depIter->second.end(); ++statIter)
                    {
                        if (row2.getModifiedStatValueMap().find(*statIter) != row2.getModifiedStatValueMap().end())
                        {
                            modified = true;
                            break;
                        }
                    }
                    if (modified)
                        break;
                }

                if (!modified)
                    continue;

                ResolveContainer container;
                container.categoryName = catStat.categoryName;
                container.catRowKey = &key;
                container.updateRowMap = mUpdateHelper.getMap();

                // If we've found even one dependency that changed, update the derived stat
                // or we also do this in the case of a new row, because all derived stats
                // need to be computed
                const Blaze::Expression* derivedExpression = stat->getDerivedExpression();
                StatVal* statVal = row.getNewStatValMap().find(stat->getName())->second;

                Blaze::Expression::ResolveVariableCb cb(&resolveStatValue); // TBD MAKE TWO - ONE FOR CROSS CAT, ONE INTERNAL

                if (stat->getDbType().getType() == STAT_TYPE_INT)
                {
                    int64_t val = derivedExpression->evalInt(cb, static_cast<void*>(&container));
                    if (statVal->intVal != val)
                    {
                        statVal->changed = true;
                    }
                    statVal->intVal = val;
                    statVal->type = 0;
                }
                else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                {
                    float_t val = derivedExpression->evalFloat(cb, static_cast<void*>(&container));
                    if (statVal->floatVal != val)
                    {
                        statVal->changed = true;
                    }
                    statVal->floatVal = val;
                    statVal->type = 1;
                }
                else if (stat->getDbType().getType() == STAT_TYPE_STRING)
                {
                    StringStatValue& stringStatValue = mDerivedStrings.push_back();

                    derivedExpression->evalString(cb, static_cast<void*>(&container),
                        stringStatValue.buf, sizeof(stringStatValue.buf));
                    if (statVal->stringcmp(stringStatValue.buf) != 0)
                    {
                        statVal->changed = true;
                    }
                    statVal->stringVal = stringStatValue.buf;
                    statVal->type = 2;
                }

                row.getModifiedStatValueMap()[stat] = statVal;
            }
        }
    }
    if (extraneousDependency)
    {
        // We shouldn't be creating more dependent update rows than we need.
        WARN_LOG("[StatsTransactionContext].updateDerivedStatsInRows: extraneous dependency");
    }
}

/*************************************************************************************************/
/*!
    \brief evaluateStats

    Performs an individual update on one stat in a request.  Looks at the previously existing
    value, the update type, and the passed in value, and performs the necessary computation.  If
    an update is made, we update the passed in modifiedValMap to track this, which will later
    serve as the trigged to update any derived formulae which might depend on this stat.  This
    differs from the other input called statValMap which contains the existing values for all
    stats in a given row, whether they be modified or not.

    \param[in]  update - the update to perform
    \param[in]  stat - the config for the stat
    \param[out] statValMap - the map to store the new value of the stat into
    \param[out] modifiedValMap - a map to update to track that this stat was modified
*/
/*************************************************************************************************/
void StatsTransactionContext::evaluateStats(const StatUpdate* update, const Stat* stat,
    StatValMap& statValMap, StatPtrValMap& modifiedValMap) const
{
    bool result = true;
    StatValMap::const_iterator statValIter = statValMap.find(stat->getName());
    StatVal* statVal = statValIter->second;

    if (stat->getDbType().getType() == STAT_TYPE_INT)
    {
        int64_t val = 0;
        char* endOfNumber = blaze_str2int(update->getValue(), &val);
        int64_t existingValue = statVal->intVal;

        if (endOfNumber == update->getValue() && update->getUpdateType() != STAT_UPDATE_TYPE_CLEAR)
        {
            // If parsing fails, the value stays the same, no modification:
            result = false;

            if (update->getValue() == nullptr || update->getValue()[0] == '\0')
            {
                WARN_LOG("[StatsTransactionContext].evaluateStats(): Unable to parse value for int stat '" << stat->getName() << 
                    "', because the value provided is nullptr or empty an empty string. The stat will not be updated.");
            }
            else
            {
                WARN_LOG("[StatsTransactionContext].evaluateStats(): Unable to parse value for int stat '" << stat->getName() << 
                    "', as its value (" << update->getValue() << ") cannot be interpreted as int64. The stat will not be updated.");
            }
        }
        else
        {
            switch (update->getUpdateType())
            {
            case STAT_UPDATE_TYPE_ASSIGN:
                break;
            case STAT_UPDATE_TYPE_CLEAR:
                val = stat->getDefaultIntVal();
                break;
            case STAT_UPDATE_TYPE_INCREMENT:
                val = existingValue + val;
                break;
            case STAT_UPDATE_TYPE_DECREMENT:
                val = existingValue - val;
                break;
            case STAT_UPDATE_TYPE_MAX:
                if (val <= existingValue)
                {
                    result = false;
                }
                break;
            case STAT_UPDATE_TYPE_MIN:
                if (val >= existingValue)
                {
                    result = false;
                }
                break;
            case STAT_UPDATE_TYPE_OR:
                val |= existingValue;
                break;
            case STAT_UPDATE_TYPE_AND:
                val &= existingValue;
                break;
            default:
                ERR_LOG("[StatSlaveImpl].evaluateStats(): Unacceptable stat update type (" << update->getUpdateType() << ") for int");
                result = false;
                break;
            }
        }

        if (result)
        {
            if (statVal->intVal != val)
            {
                if (stat->isValidIntVal(val))
                {
                    statVal->intVal = val;
                    statVal->changed = true;
                }
                else
                {
                    WARN_LOG("[StatSlaveImpl].evaluateStats(): Warning, updated stat value " << val
                        << " is out of range and will be ignored for stat " << stat->getName());
                }
            }
        }
    }
    else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
    {
        float_t val = 0;
        char* endOfNumber = blaze_str2flt(update->getValue(), val);
        float_t existingValue = statVal->floatVal;

        if (endOfNumber == update->getValue() && update->getUpdateType() != STAT_UPDATE_TYPE_CLEAR)
        {
            result = false;
            if (update->getValue() == nullptr || update->getValue()[0] == '\0')
            {
                WARN_LOG("[StatsTransactionContext].evaluateStats(): Unable to parse value for float stat '" << stat->getName() << 
                    "', because the value provided is nullptr or empty an empty string. The stat will not be updated.");
            }
            else
            {
                WARN_LOG("[StatsTransactionContext].evaluateStats(): Unable to parse value for float stat '" << stat->getName() << 
                    "', as its value (" << update->getValue() << ") cannot be interpreted as float. The stat will not be updated.");
            }
        }
        else
        {
            switch (update->getUpdateType())
            {
            case STAT_UPDATE_TYPE_ASSIGN:
                break;
            case STAT_UPDATE_TYPE_CLEAR:
                val = stat->getDefaultFloatVal();
                break;
            case STAT_UPDATE_TYPE_INCREMENT:
                val = existingValue + val;
                break;
            case STAT_UPDATE_TYPE_DECREMENT:
                val = existingValue - val;
                break;
            case STAT_UPDATE_TYPE_MAX:
                if (val <= existingValue)
                {
                    result = false;
                }
                break;
            case STAT_UPDATE_TYPE_MIN:
                if (val >= existingValue)
                {
                    result = false;
                }
                break;
            default:
                ERR_LOG("[StatsTransactionContext].evaluateStats(): Unacceptable stat update type (" << update->getUpdateType() << ") for float");
                result = false;
                break;
            }
        }
        if (result)
        {
            if (statVal->floatVal != val)
            {
                statVal->floatVal = val;
                statVal->changed = true;
            }
        }
    }
    else if (stat->getDbType().getType() == STAT_TYPE_STRING)
    {
        const char8_t* val = nullptr;

        switch (update->getUpdateType())
        {
        case STAT_UPDATE_TYPE_ASSIGN:
            val = update->getValue();
            break;
        case STAT_UPDATE_TYPE_CLEAR:
            val = stat->getDefaultStringVal();
            break;
        default:
            ERR_LOG("[StatsTransactionContext].evaluateStats: Unacceptable stat update type (" << update->getUpdateType() << ") for string");
            result = false;
            break;
        }
        if (result)
        {
            if (statVal->stringcmp(val) != 0)
            {
                statVal->stringVal = val;
                statVal->changed = true;
            }
        }
    }
    else
    {
        ERR_LOG("[StatsTransactionContext].evaluateStats: Unexpected stat type (" << stat->getDbType().getType() << ")");
        result = false;
    }

    if (result)
    {
        modifiedValMap[stat] = statVal;
    }
}

/*************************************************************************************************/
/*!
    \brief updateProviderRow

    Build request to update a single row in the provider.

    \param[in]  row - update information for the row
    \param[in]  provRow - provider row fetched from provider
    \param[in]  entityId - the entity id owning the stats to be updated
    \param[in]  period - period
    \param[in]  periodId - period id for the row
    \param[in/out] diffStatValMap - statValMap holding the current value for the row to be updated,
                                    only used when the row to be update is an aggregate row or total row
    \param[in] rowScopes - scopeName and index list for the row to be updated
*/
/*************************************************************************************************/
void StatsTransactionContext::updateProviderRow(UpdateRow& row, const StatsProviderRow* provRow,
                                    EntityId entityId, int32_t period, int32_t periodId,
                                    StatValMap* diffStatValMap, /* = nullptr*/
                                    const RowScopesVector* rowScopes/* = nullptr*/) 
{
    const StatCategory* cat = row.getCategory();
    const StatValMap& statValMap = row.getNewStatValMap();

    // diffStatValMap == nullptr for stat row without keyscopes
    if (diffStatValMap != nullptr)
    {
        // Prepopulate the stat map with the defaults if we didn't find a row,
        // as every derived stat formula should be triggered in this case
        StatMap::const_iterator statItr = cat->getStatsMap()->begin();
        StatMap::const_iterator statEnd = cat->getStatsMap()->end();
        for (; statItr != statEnd; ++statItr)
        {
            const Stat* stat = statItr->second;
            StatVal* diffVal = BLAZE_NEW StatVal();

            if (stat->getDbType().getType() == STAT_TYPE_INT)
            {
                diffVal->intVal = (provRow != nullptr) ? provRow->getValueInt64(stat->getName()) : stat->getDefaultIntVal();
                diffVal->type = 0;
            }
            else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
            {
                diffVal->floatVal = (provRow != nullptr) ? provRow->getValueFloat(stat->getName()) : stat->getDefaultFloatVal();
                diffVal->type = 1;
            }
            else if (stat->getDbType().getType() == STAT_TYPE_STRING)
            {
                diffVal->stringVal = (provRow != nullptr) ? provRow->getValueString(stat->getName()) : stat->getDefaultStringVal();
                diffVal->type = 2;
            }

            diffStatValMap->insert(eastl::make_pair(stat->getName(), diffVal));
        }
    }

    // TBD This can fail, but the method we're in returns void, needs a refactor
    mStatsProvider->addStatsUpdateRequest(*cat, entityId, period, periodId, rowScopes, row.getModifiedStatValueMap(), provRow == nullptr /* newRow */);

    // Calculate differences for aggregate rows
    // diffStatValMap == nullptr for stat row without keyscopes
    if (diffStatValMap != nullptr)
    {
        StatMap::const_iterator statItr = cat->getStatsMap()->begin();
        StatMap::const_iterator statEnd = cat->getStatsMap()->end();
        for (; statItr != statEnd; ++statItr)
        {
            // all stat value maps were filled according category StatMap
            // so checking
            const Stat* stat = statItr->second;
            // derived stats are not aggregates, they will be calculated from the stat values in the row
            if (stat->isDerived()) 
                continue;

            StatValMap::iterator valDiffIter = diffStatValMap->find(stat->getName());
            StatVal* diffVal = valDiffIter->second;
            StatValMap::const_iterator valIter = statValMap.find(stat->getName());
            StatVal* statVal = valIter->second;

            int32_t type = stat->getDbType().getType();
            
            if (type == STAT_TYPE_INT)
            {
                if (provRow == nullptr) diffVal->intVal =  statVal->intVal;
                else diffVal->intVal =  statVal->intVal - diffVal->intVal;

                diffVal->type = 0;
            }
            else if (type == STAT_TYPE_FLOAT)
            {
                if (provRow == nullptr) diffVal->floatVal = statVal->floatVal;
                else diffVal->floatVal = statVal->floatVal - diffVal->floatVal;

                diffVal->type = 1;
            }
            else if (type == STAT_TYPE_STRING)
            {
                diffVal->stringVal = statVal->stringVal;
                diffVal->type = 2;
            }
            else
            {
                WARN_LOG("[StatsTransactionContext].updateProviderRow(): unexpected type <" << type << "> for stat <" << stat->getName() << ">");
            }
        }
    }

    addBroadcastData(cat, entityId, period, periodId, &statValMap,rowScopes);
}

/*************************************************************************************************/
/*!
    \brief addBroadcastData
    Adds stat rows to the broadcast structure
    
    \param[in] cat - StatCategory*
    \param[in] entityId - entity ID
    \param[in] periodType - period type
    \param[in] periodId - period ID
    \param[in] statValMap - StatValMap*
    \param[in] rowScopes - RowScopesVector*
*/
/*************************************************************************************************/
void StatsTransactionContext::addBroadcastData(const StatCategory* cat,EntityId entityId, 
                                         int32_t periodType, int32_t periodId,
                                         const StatValMap* statValMap, const RowScopesVector* rowScopes) 
{
    /// @todo deprecate RowScopesVector
    // convert RowScopesVector to ScopeNameValueMap
    ScopeNameValueMap scopeMap;
    if (rowScopes != nullptr)
    {
        for (RowScopesVector::const_iterator ri = rowScopes->begin(), re = rowScopes->end(); ri != re; ++ri)
        {
            scopeMap.insert(eastl::make_pair(ri->mScopeName, ri->mScopeValue));
        }
    }

    CacheRowUpdate* cacheRowUpdate = BLAZE_NEW CacheRowUpdate();
    char8_t keyScopeString[128];
    LeaderboardIndexes::buildKeyScopeString(keyScopeString, sizeof(keyScopeString), scopeMap);
    mComp->mLeaderboardIndexes->updateStats(cat->getId(), entityId, periodType, periodId, statValMap, &scopeMap, keyScopeString, mLbQueryString, cacheRowUpdate->getStatUpdates());
    if (!cacheRowUpdate->getStatUpdates().empty() || mComp->getStatsCache().isLruEnabled())
    {
        // need to send at least the category and entity info to invalidate any LRU cache
        LeaderboardIndexes::populateCacheRow(*cacheRowUpdate, cat->getId(), entityId, periodType, periodId, keyScopeString);
        mNotification.getBroadcast().getStatUpdate()->getCacheUpdates().push_back(cacheRowUpdate);
    }
    else
        delete cacheRowUpdate;

    mComp->mStatsExtendedDataProvider->getUserStats().updateUserStats(mExtDataReq, entityId, cat->getId(), periodType, scopeMap, *statValMap);
    mComp->mStatsExtendedDataProvider->getUserRanks().updateUserRanks(mExtDataReq, entityId, cat->getId(), periodType);
}

/*************************************************************************************************/
/*!
    \brief updateAggregateDerivedStats

    Calculates derived stats for the stat row. Used by aggregate stat rows only and 
    unconditionally recalculates all derived stats.
*/
/*************************************************************************************************/
void StatsTransactionContext::updateAggregateDerivedStats()
{
    CategoryUpdateAggregateRowMap::const_iterator iter = mUpdateHelper.getCategoryAggregateMap()->begin();
    CategoryUpdateAggregateRowMap::const_iterator end = mUpdateHelper.getCategoryAggregateMap()->end();

    for (; iter != end; ++iter)
    {
        const DependencySet& depSet = iter->second;
        for (DependencySet::const_iterator depIter = depSet.begin(); depIter != depSet.end(); ++depIter)
        {
            const CategoryUpdateAggregateRowKey& key = iter->first;
            const CategoryDependency* catDep = *depIter;

            for (CategoryStatList::const_iterator derivedIter = catDep->catStatList.begin(); derivedIter != catDep->catStatList.end(); ++derivedIter)
            {
                const CategoryStat& catStat = *derivedIter;
                UpdateAggregateRowKey rowKey(catStat.categoryName, key.mEntityId, key.mPeriod, key.mRowScopesVector);
                StatValMap& statValMap = mUpdateHelper.getStatValMaps()->find(rowKey)->second;

                const StatCategory* cat = mComp->getConfigData()->getCategoryMap()->find(catStat.categoryName)->second;
                const Stat* stat = cat->getStatsMap()->find(catStat.statName)->second;

                ResolveAggregateContainer container;
                container.categoryName = catStat.categoryName;
                container.catRowKey = &key;
                container.updateRowMap = mUpdateHelper.getStatValMaps();

                const Blaze::Expression* derivedExpression = stat->getDerivedExpression();
                StatVal* statVal = statValMap.find(stat->getName())->second;

                Blaze::Expression::ResolveVariableCb cb(&resolveAggregateStatValue);

                if (stat->getDbType().getType() == STAT_TYPE_INT)
                {
                    int64_t val = derivedExpression->evalInt(cb, static_cast<void*>(&container));
                    if (statVal->intVal != val)
                    {
                        statVal->changed = true;
                    }
                    statVal->intVal = val;
                    statVal->type = 0;
                }
                else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                {
                    float_t val = derivedExpression->evalFloat(cb, static_cast<void*>(&container));
                    if (statVal->floatVal != val)
                    {
                        statVal->changed = true;
                    }
                    statVal->floatVal = val;
                    statVal->type = 1;
                }
                else if (stat->getDbType().getType() == STAT_TYPE_STRING)
                {
                    StringStatValue& stringStatValue = mDerivedStrings.push_back();

                    derivedExpression->evalString(cb, static_cast<void*>(&container),
                        stringStatValue.buf, sizeof(stringStatValue.buf));
                    if (statVal->stringcmp(stringStatValue.buf) != 0)
                    {
                        statVal->changed = true;
                    }
                    statVal->stringVal = stringStatValue.buf;
                    statVal->type = 2;
                }
            }
        }
    }
}

BlazeRpcError StatsTransactionContext::commitUpdates()
{
    // fetch stats if it is not done before
    if ((mUpdateState & FETCHED) == 0)
    {
        BlazeRpcError err = fetchStats();
        if (err != ERR_OK)
            return err;
    }

    bool applyUpdates = false;
    bool applyDerived = false;

    if ((mUpdateState & APPLIED_UPDATES) == 0)
    {
        applyUpdates = true;
    }
    if ((mUpdateState & APPLIED_DERIVED) == 0)
    {
        applyDerived = true;
    }
    
    BlazeRpcError err = calcStatsChange(applyUpdates, applyDerived);
    if (err != ERR_OK)
        return err;

    // iterate all temporary rows (1 row per period) and build the 'update' statements
    UpdateRowMap::iterator i = mUpdateHelper.getMap()->begin(), e = mUpdateHelper.getMap()->end();
    for (; i != e; ++i)
    {
        const UpdateRowKey& key = i->first;
        UpdateRow& row = i->second;
        const ScopesVectorForRows* scopes = row.getScopes();
        const StatCategory* cat = row.getCategory();

        if (scopes == nullptr)
        {
            const StatsProviderRow* provRow = row.getProviderRow();
            updateProviderRow(row, provRow, key.entityId, key.period, mPeriodIds[key.period]);
        }
        else
        {
            ScopesVectorForRows::const_iterator si = scopes->begin();
            ScopesVectorForRows::const_iterator se = scopes->end();
            // Find default (one which come with update request) scopes index first
            const RowScopesVector* defRowScopes = nullptr;
            for (; si != se; ++si)
            {
                if (key.isDefaultScopeIndex(*si))
                {
                    defRowScopes = &(*si);
                    break;
                }
            }

            // never should happen
            if (defRowScopes == nullptr)
            {
                ERR_LOG("[StatsTransactionContext].commitUpdates: invalid ScopesVectorForRows");
                return ERR_SYSTEM;
            }

            // Update stat row and prepare differences for aggregate rows
            StatValMapWrapper diffValMap; // use wrapper for easier object clean-up
            StatsProviderRowPtr provRow = row.getProviderResultSet()->getProviderRow(key.entityId, mPeriodIds[key.period], defRowScopes);
            updateProviderRow(row, provRow.get(), key.entityId, key.period, mPeriodIds[key.period], &diffValMap, defRowScopes);

            const StatMap* statMap = cat->getStatsMap();
            StatMap::const_iterator statEnd = statMap->end();

            // update aggregates
            si = scopes->begin();
            for (; si != se; ++si)
            {
                const RowScopesVector* aggRowScopes = &(*si);
                if (aggRowScopes == defRowScopes)
                    continue;

                StatMap::const_iterator statIter = statMap->begin();
                UpdateAggregateRowMap::iterator aggValIter = mUpdateHelper.getStatValMapIter(key, *aggRowScopes, cat);
                StatValMap& statValMap = aggValIter->second;
                if (aggValIter->first.mStatus == AGG_NEW)
                {
                    // never should happen
                    WARN_LOG("[StatsTransactionContext].commitUpdates: unexpected new stat values for aggregate key");
                }

                // Update aggregates returned by updateProviderRow()
                StatValMap::iterator diffEnd = diffValMap.end();
                StatValMap::iterator valEnd = statValMap.end();
                statIter = statMap->begin();
                for (; statIter != statEnd; ++statIter)
                {
                    const Stat* stat = statIter->second;
                    // leave derived alone
                    if (stat->isDerived())
                        continue;

                    StatValMap::iterator diffIter = diffValMap.find(stat->getName());
                    if (diffIter == diffEnd)
                        return ERR_SYSTEM; // very unlikely
                    
                    StatValMap::iterator valIter = statValMap.find(stat->getName());
                    if (valIter == valEnd)
                        return ERR_SYSTEM; // very unlikely
                    
                    StatVal* statVal = valIter->second;
                    StatVal* diffVal = diffIter->second;
                    
                    int32_t type = stat->getDbType().getType();
                    
                    switch (type)
                    {
                    case STAT_TYPE_INT:
                        if (diffVal->intVal != 0)
                        {
                            statVal->changed = true;
                        }
                        statVal->intVal += diffVal->intVal;
                        statVal->type = 0;
                        break;
                    case STAT_TYPE_FLOAT:
                        if (diffVal->floatVal != 0.0)
                        {
                            statVal->changed = true;
                        }
                        statVal->floatVal += diffVal->floatVal;
                        statVal->type = 1;
                        break;
                    case STAT_TYPE_STRING:
                        // Just assign last string value
                        if (statVal->stringcmp(diffVal->stringVal) != 0)
                        {
                            statVal->changed = true;
                        }
                        statVal->stringVal = diffVal->stringVal;
                        statVal->type = 2;
                        break;
                    default:
                        return ERR_SYSTEM;
                        break;
                    }
                }
            }
        }
    }

    updateAggregateDerivedStats();

    // Add update or insert queries for the aggregate rows
    UpdateAggregateRowMap* aggValMapMap = mUpdateHelper.getStatValMaps();
    UpdateAggregateRowMap::iterator mmIter = aggValMapMap->begin();
    UpdateAggregateRowMap::iterator mmEnd = aggValMapMap->end();
    for (; mmIter != mmEnd; ++mmIter)
    {

        const UpdateAggregateRowKey& aggKey = mmIter->first;
        StatValMap& statValMap = mmIter->second;

        err = mStatsProvider->addStatsUpdateRequest(mPeriodIds[aggKey.mPeriod], aggKey, statValMap);
        if (err != ERR_OK)
        {
            // Still want to do the cleanup of the aggregate map after this loop, so just break
            // instead of immediately returning an error
            break;
        }

        const StatCategory* cat = mComp->getConfigData()->getCategoryMap()->find(aggKey.mCategory)->second;
        addBroadcastData(cat, aggKey.mEntityId, aggKey.mPeriod, mPeriodIds[aggKey.mPeriod], 
            &statValMap, &aggKey.mRowScopesVector);
    }

    // clean up the statValMap in aggregate map in case we have to retry
    mmIter = aggValMapMap->begin();
    for (; mmIter != mmEnd; ++mmIter)
    {
        StatValMap::const_iterator valIter = mmIter->second.begin();
        StatValMap::const_iterator valIterEnd = mmIter->second.end();
        for (; valIter != valIterEnd; ++valIter)
        {
            delete valIter->second;
        }
        mmIter->second.clear();
    }

    // Now after having done the aggregate map cleanup, if we had an earlier failure return error here
    if (err != ERR_OK)
        return err;

    return mStatsProvider->executeUpdateRequest();
}

BlazeRpcError StatsTransactionContext::commitTransaction()
{
    // prevent commiting more than once or commit after abort
    if ((mUpdateState & COMMITED) != 0 || (mUpdateState & ABORTED) != 0)
        return ERR_SYSTEM;

    BlazeRpcError commitError = mStatsProvider->commit();
    if (commitError != ERR_OK)
    {
        abortTransaction();
        ERR_LOG("[StatsTransactionContext].commitTransaction(): failed to commit stat change. Will rollback.: " 
                << getDbErrorString(commitError));
        return STATS_ERR_DB_QUERY_FAILED;
    }

    mUpdateState = COMMITED;

    return ERR_OK;
}

void StatsTransactionContext::abortTransaction()
{
    // prevent aborting more than once or abort after commit
    if ((mUpdateState & ABORTED) != 0 || (mUpdateState & INITIALIZED)==0)
        return;

    if (mStatsProvider != nullptr) 
        mStatsProvider->rollback();

    mUpdateState = ABORTED;
}

void StatsTransactionContext::updateProviderRows(const FetchedRowUpdateList& cacheList)
{
    // if there is any change from client...
    FetchedRowUpdateList::const_iterator rowItr = cacheList.begin();
    FetchedRowUpdateList::const_iterator rowEnd = cacheList.end();
    for (; rowItr != rowEnd; ++rowItr)
    {
        FetchedNameValueList::const_iterator ii = (*rowItr)->getStatUpdates().begin();
        FetchedNameValueList::const_iterator ee = (*rowItr)->getStatUpdates().end();

        UpdateRowKey rowKey((*rowItr)->getCategory(), (*rowItr)->getEntityId(), (*rowItr)->getPeriodType(), &(*rowItr)->getKeyScopeNameValueMap());
        UpdateRowMap::iterator updateRowItr = this->mUpdateHelper.getMap()->find(rowKey);

        if (updateRowItr == this->mUpdateHelper.getMap()->end())
        {
            WARN_LOG("[StatsTransactionContext].execute: The row is not in our transaction. Row info: category:"
                << (*rowItr)->getCategory() << " entity id:" << (*rowItr)->getEntityId() <<" periodType:" << (*rowItr)->getPeriodType());
            continue;
        }

        for (; ii != ee; ++ii)
        {
            // the value is changed by the other side.
            if ((*ii)->getChanged())
            {
                StatValMap::const_iterator catStatItr = updateRowItr->second.getNewStatValMap().find((*ii)->getName()); 
                if (catStatItr != updateRowItr->second.getNewStatValMap().end())
                {
                    // update cache in mUpdateHelper with new value.
                    if ((*ii)->getType() == STAT_TYPE_INT)
                    {
                        updateRowItr->second.setValueInt((*ii)->getName(), (*ii)->getValueInt());
                    }
                    else if ((*ii)->getType() == STAT_TYPE_FLOAT)
                    {
                        updateRowItr->second.setValueFloat((*ii)->getName(), (*ii)->getValueFloat());
                    }
                    else if ((*ii)->getType() == STAT_TYPE_STRING)
                    {
                        updateRowItr->second.setValueString((*ii)->getName(), (*ii)->getValueString());
                    }
                }
            }
        }
    }
}


} // Stats
} // Blaze
