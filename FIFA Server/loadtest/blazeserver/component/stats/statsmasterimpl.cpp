/*************************************************************************************************/
/*!
    \file   statsmasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class StatsMasterImpl

    Implements the master portion of the stats component.
*/
/*************************************************************************************************/
/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbconfig.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/connection/selector.h"
#include "framework/component/componentmanager.h"
#include "framework/util/random.h"
#include "framework/controller/controller.h"
#include "framework/tdf/controllertypes_server.h"
#include "statsslaveimpl.h"
#include "statsbootstrap.h"
#include "statsmasterimpl.h"
#include "statsconfig.h"
#include "stats/leaderboardindex.h"
#include "stats/tdf/stats.h"

namespace Blaze
{
namespace Stats
{

// static
StatsMaster* StatsMaster::createImpl()
{
    return BLAZE_NEW_NAMED("StatsMasterImpl") StatsMasterImpl();
}

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief StatsMasterImpl

    Constructor for the StatsMasterImpl object that owns the core state of the stats and
    leaderboard master component.
*/
/*************************************************************************************************/
StatsMasterImpl::StatsMasterImpl() :
    mBootstrapSuccess(false),
    mLeaderboardIndexes(nullptr),
    mStatsConfig(nullptr),    
    mDbId(DbScheduler::INVALID_DB_ID),
    mGlobalCacheTimerId(INVALID_TIMER_ID),
    mGlobalCacheIsBeingSerialized(false)
{
}

/*************************************************************************************************/
/*!
    \brief ~StatsMasterImpl

    Destroys the master stats and leaderboard component.
*/
/*************************************************************************************************/
StatsMasterImpl::~StatsMasterImpl()
{
    SlaveSessionStatsMap::iterator itr = mSlaveSessionStatsMap.begin();
    SlaveSessionStatsMap::iterator end = mSlaveSessionStatsMap.end();
    for (; itr != end; ++itr)
    {
        delete itr->second;
    }


    delete mLeaderboardIndexes;
    delete mStatsConfig;
}

/*************************************************************************************************/
/*!
    \brief onValidateConfig

    Validate the configure data for the stats master component.

    \param[in]  config - the configure data
    \param[in]  referenceConfigPtr - the former configure data for reconfiguration
    \param[out]  validationErrors - the error messages of validation
    \return - true if successful, false otherwise
*/
/*************************************************************************************************/
bool StatsMasterImpl::onValidateConfig(StatsConfig& config, const StatsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    if (*(config.getDbName()) == '\0')
    {
        eastl::string msg;
        msg.sprintf("[StatsMasterImpl].onValidateConfig(): No DB configuration detected.", this);
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    else
    {
        INFO_LOG("[StatsMasterImpl].onValidateConfig():Stats is set to use ("<< config.getDbName() <<") db.");
    }

    if (config.getRollover().getDailyBuffer() < 1)
    {
        eastl::string msg;
        msg.sprintf("[StatsMasterImpl].onValidateConfig(): Rollover dailyBuffer can not be lower than 1");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    if (config.getRollover().getWeeklyBuffer() < 1)
    {
        eastl::string msg;
        msg.sprintf("[StatsMasterImpl].onValidateConfig(): Rollover weeklyBuffer can not be lower than 1");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    if (config.getRollover().getMonthlyBuffer() < 1)
    {
        eastl::string msg;
        msg.sprintf("[StatsMasterImpl].onValidateConfig(): Rollover monthlyBuffer can not be lower than 1");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    if (referenceConfigPtr != nullptr)
    {
        if (!StatsConfigData::validateConfigLeaderboardHierarchy(config, referenceConfigPtr, validationErrors))
        {
            eastl::string msg;
            msg.sprintf("[StatsMasterImpl].onValidateConfig(): Failed to validate leaderboard hierarchy");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }
    }

    StatsConfigData* tempStatsConfig = BLAZE_NEW StatsConfigData();
    if (!tempStatsConfig->parseStatsConfig(config, validationErrors))
    {
        delete tempStatsConfig;
        tempStatsConfig = nullptr;

        eastl::string msg;
        msg.sprintf("[StatsMasterImpl].onValidateConfig(): Failed to parse stats configuration");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
        return false;
    }

    if (tempStatsConfig != nullptr)
    {
        delete tempStatsConfig;
        tempStatsConfig = nullptr;
    }

    return validationErrors.getErrorMessages().empty();
}

/*************************************************************************************************/
/*!
    \brief onConfigure

    Configure the stats master component with it's component config obtained from the master
    properties file.

    \return - true if successful, false otherwise
*/
/*************************************************************************************************/
bool StatsMasterImpl::onConfigure()
{
    const StatsConfig& config = getConfig();

    mRollover.setComponent(this);

    configureCommon();

    const char8_t* dbName = config.getDbName();

    mDbId = gDbScheduler->getDbId(dbName);
    if (mDbId == DbScheduler::INVALID_DB_ID)
    {
        ERR_LOG("[StatsMasterImpl].onConfigure: No DB configuration found, shutting down.");
        return false;
    }

    mStatsConfig = BLAZE_NEW StatsConfigData();

    return true;
}

/*************************************************************************************************/
/*!
    \brief onReconfigure

    Reconfigure the stats master component with it's component config obtained from the master
    properties file.

    \return - true if successful, false otherwise
*/
/*************************************************************************************************/
bool StatsMasterImpl::onReconfigure()
{
    INFO_LOG("[StatsMasterImpl].onReconfigure: start to reconfigure.");

    configureCommon();

    return true;
}

void StatsMasterImpl::configureCommon()
{
    const StatsConfig& config = getConfig();

    mRollover.parseRolloverData(config);

    // need to make sure that the current period ids get set before configuring the stat categories.
    // they are used when setting up the partitions of the stats tables.
    TimeValue tv = TimeValue::getTimeOfDay();
    int64_t t = tv.getSec();
    mRollover.getDailyTimer(t);
    mRollover.getWeeklyTimer(t);
    mRollover.getMonthlyTimer(t);
}

bool StatsMasterImpl::onResolve()
{
    BlazeRpcError error;
    StatsSlave* statsSlave = nullptr;
    // NOTE: By using a remote component placeholder we are able to perform an apriori subscription for notifications *without*
    // waiting for the component to actually be active.
    error = gController->getComponentManager().addRemoteComponentPlaceholder(Blaze::Stats::StatsSlave::COMPONENT_ID, reinterpret_cast<Component*&>(statsSlave));
    if (error != ERR_OK || statsSlave == nullptr)
    {
        FATAL_LOG("[SearchShardingBroker].initialize(): Could not register placeholder stats slave.");
        return false;
    }

    statsSlave->addNotificationListener(*this);
    if (statsSlave->notificationSubscribe() != Blaze::ERR_OK)
    {
        FATAL_LOG("[StatsMasterImpl].onResolve() - unable to subscribe for notifications to stats slaves");
        return false;
    }

    // Bootstrap after resolving any currently running stats slaves because if we are restarting a master
    // while a cluster is actively running, we want to first register for notifications before populating
    // our leaderboard data structures so that we receive all notifications that we need to apply immediately afterwards
    const StatsConfig& config = getConfig();
    StatsBootstrap* bootstrap = BLAZE_NEW StatsBootstrap(this, &config, gController->isDBDestructiveAllowed());
    bool success = bootstrap->configure(*mStatsConfig);
    if (!mStatsConfig->initShards())
    {
        ERR_LOG("[StatsMasterImpl].onResolve: cannot init shards.");
        return false;
    }
   
    if (success)
    {
        mLeaderboardIndexes = BLAZE_NEW_STATS_LEADERBOARD LeaderboardIndexes(this, getMetricsCollection(), isMaster());
        mLeaderboardIndexes->buildIndexesStructures();
        mBootstrapSuccess = bootstrap->run(*mStatsConfig, *mLeaderboardIndexes);
    }
    delete bootstrap;

    if (mBootstrapSuccess)
    {
        INFO_LOG("[StatsMasterImpl].onResolve: Successfully completed bootstraping");
        for (UpdateNotificationList::const_iterator it = mBufferedUpdates.begin(); it != mBufferedUpdates.end(); ++it)
            onUpdateCacheNotification(*it, nullptr);
        mBufferedUpdates.clear();
    }
    else
    {
        ERR_LOG("[StatsMasterImpl].onResolve: Failed to bootstrap stats master!");
        return false;
    }

    mRollover.setTimers();

    // set up global cache "write to database" process
    // we write global cache to database every 60 seconds
    mGlobalCacheTimerId = gSelector->scheduleFiberTimerCall(
        TimeValue::getTimeOfDay() + TimeValue(GLOBAL_CACHE_SERIALIZATION_INTERVAL), this, &StatsMasterImpl::onWriteGlobalCacheToDatabase,
        "StatsMasterImpl::onWriteGlobalCacheToDatabase");

    return true;
}

/*************************************************************************************************/
/*!
    \brief onSlaveSessionRemoved

    Remove any data associated with a slave session's id.  At present this basically entails
    some buffered chunks of a leaderboard we may have been sending to a slave at startup.

    \param[in]  session - the session being removed
*/
/*************************************************************************************************/
void StatsMasterImpl::onSlaveSessionRemoved(Blaze::SlaveSession& session)
{
    SlaveSessionStatsMap::iterator iter = mSlaveSessionStatsMap.find(session.getId());
    if (iter != mSlaveSessionStatsMap.end())
    {
        delete iter->second;
        mSlaveSessionStatsMap.erase(iter);
    }

    InstanceId instanceId = SlaveSession::getSessionIdInstanceId(session.getId());
    InstanceIdSet::iterator iiItr = mGlobalCacheSlaveIds.find(instanceId);
    if (iiItr != mGlobalCacheSlaveIds.end())
    {
        mGlobalCacheSlaveIds.erase(iiItr);
    }

    iiItr = mNewGlobalCacheSlaveIds.find(instanceId);
    if (iiItr != mNewGlobalCacheSlaveIds.end())
    {
        mNewGlobalCacheSlaveIds.erase(iiItr);
    }
}

/*************************************************************************************************/
/*!
    \brief onSlaveSessionAdded

    Handle a new slave session connecting.

    \param[in]  session - the session being added
*/
/*************************************************************************************************/
void StatsMasterImpl::onSlaveSessionAdded(Blaze::SlaveSession& session)
{
    startGlobalCacheSynchronization(session);
}

/*************************************************************************************************/
/*!
    \brief startGlobalCacheSynchronization

    Initiate global cache synchronization. First instruct random slave to write its cache.
    Then order all slaves to drop their global cache and re-build it in case cache was dirty.

    \param[in]  session - the session being added
*/
/*************************************************************************************************/
void StatsMasterImpl::startGlobalCacheSynchronization(Blaze::SlaveSession& session)
{
    InstanceId instanceId = SlaveSession::getSessionIdInstanceId(session.getId());
    InstanceIdSet::iterator iiItr = mNewGlobalCacheSlaveIds.find(instanceId);
    if (iiItr != mNewGlobalCacheSlaveIds.end())
    {
        // this is a new stats slave -- initiate global cache synchronization
        if (mGlobalCacheSlaveIds.size() > 0) // check if this is actually the first stats slave added
        {
            if (!mGlobalCacheIsBeingSerialized)
            {
                // there are other slaves active on this master so kick
                // the process of synchronization where one of the active
                // slaves will write its cache to database and then
                // all new slaves can read it

                // cancel current timer
                if (mGlobalCacheTimerId != INVALID_TIMER_ID)
                {
                    gSelector->cancelTimer(mGlobalCacheTimerId);
                    mGlobalCacheTimerId = INVALID_TIMER_ID;
                }

                // kick the process
                onWriteGlobalCacheToDatabase();
            }
        }
        else
        {
            // this is the first stats slave, let it build its global cache

            mGlobalCacheSlaveIds.insert(instanceId);
            mNewGlobalCacheSlaveIds.erase(instanceId);

            StatsGlobalCacheInstruction instruction;
            instruction.setInstruction(STATS_GLOBAL_CACHE_REBUILD);
            sendExecuteGlobalCacheInstructionToSlaveSession(&session, &instruction);
        }
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
int32_t StatsMasterImpl::getPeriodId(int32_t periodType)
{
    switch (periodType)
    {
        case STAT_PERIOD_DAILY:
            return mRollover.getPeriodIds()->getCurrentDailyPeriodId();
        case STAT_PERIOD_WEEKLY:
            return mRollover.getPeriodIds()->getCurrentWeeklyPeriodId();
        case STAT_PERIOD_MONTHLY:
            return mRollover.getPeriodIds()->getCurrentMonthlyPeriodId();
        case STAT_PERIOD_ALL_TIME:
            return 0;
        default:
            return 0;
    }
}        

/*************************************************************************************************/
/*!
    \brief getRetention

    Returns the number of periods to retain for a given period type

    \param[in]  periodType - the period type to check the retention for

    \return - number of periods to retain for the given period type
*/
/*************************************************************************************************/
int32_t StatsMasterImpl::getRetention(int32_t periodType)
{
    switch (periodType)
    {
        case STAT_PERIOD_DAILY:
            return mRollover.getPeriodIds()->getDailyRetention();
        case STAT_PERIOD_WEEKLY:
            return mRollover.getPeriodIds()->getWeeklyRetention();
        case STAT_PERIOD_MONTHLY:
            return mRollover.getPeriodIds()->getMonthlyRetention();
        default:
            return 0;
    }            
}

void StatsMasterImpl::trimPeriods(const StatPeriod& statPeriod)
{
    mLeaderboardIndexes->scheduleTrimPeriods(statPeriod.getPeriod(), statPeriod.getRetention());
}

/*************************************************************************************************/

void StatsMasterImpl::onUpdateCacheNotification(const StatUpdateNotification& data, UserSession *associatedUserSession)
{
    if (EA_UNLIKELY(!mBootstrapSuccess))
    {
        data.copyInto(mBufferedUpdates.push_back());
        return;
    }

    switch (data.getBroadcast().getActiveMember())
    {
    case StatBroadcast::MEMBER_STATUPDATE:
        mLeaderboardIndexes->updateStats(data.getBroadcast().getStatUpdate()->getCacheUpdates());
        break;
    case StatBroadcast::MEMBER_STATDELETEBYCATEGORY:
        mLeaderboardIndexes->deleteCategoryStats(data.getBroadcast().getStatDeleteByCategory()->getCacheDeletes(),
                                                 data.getBroadcast().getStatDeleteByCategory()->getCurrentPeriodIds());
        break;
    case StatBroadcast::MEMBER_STATDELETEANDUPDATE:
        {
            mLeaderboardIndexes->deleteStats(data.getBroadcast().getStatDeleteAndUpdate()->getCacheDeletes(),
                data.getBroadcast().getStatDeleteAndUpdate()->getCurrentPeriodIds(), nullptr);
            mLeaderboardIndexes->updateStats(data.getBroadcast().getStatDeleteAndUpdate()->getCacheUpdates());
            break;
        }
    case StatBroadcast::MEMBER_STATDELETEBYENTITY:
        mLeaderboardIndexes->deleteStatsByEntity(data.getBroadcast().getStatDeleteByEntity()->getCacheDeletes(),
                                                 data.getBroadcast().getStatDeleteByEntity()->getCurrentPeriodIds(), nullptr);
        break;
    default:
        break;
    }
}

void StatsMasterImpl::onUpdateGlobalStatsNotification(const UpdateStatsRequest& data, UserSession *associatedUserSession)
{
    // Intentionally empty, the stats master registers with the stats slaves
    // primarily to receive regular stat update notifications, no action required on global updates
}

GetPeriodIdsMasterError::Error StatsMasterImpl::processGetPeriodIdsMaster( PeriodIds &response, const Message *message)
{
    mRollover.getPeriodIds()->copyInto(response);
    return GetPeriodIdsMasterError::ERR_OK;
}

PopulateLeaderboardIndexError::Error StatsMasterImpl::processPopulateLeaderboardIndex(
    const Blaze::Stats::PopulateLeaderboardIndexRequest &request, Blaze::Stats::PopulateLeaderboardIndexData &response, const Message *message)
{
    SlaveSessionId sessId;
    if (message != nullptr)
    {
        sessId = message->getSlaveSessionId();
    }
    else
    {
        // The only way message is ever nullptr is if the source of this RPCs caller is actually the local instance, which is not supported.
        ERR_LOG("StatsMasterImpl.processPopulateLeaderboardIndex: A local StatsSlave running on the same instance as the StatsMaster is not supported.");
        sessId = SlaveSession::INVALID_SESSION_ID;
    }

    if (sessId == SlaveSession::INVALID_SESSION_ID)
    {
        return PopulateLeaderboardIndexError::ERR_SYSTEM;
    }

    LeaderboardEntries* lbEntries = nullptr;
    SlaveSessionStatsMap::const_iterator iter = mSlaveSessionStatsMap.find(sessId);
    if (iter != mSlaveSessionStatsMap.end())
        lbEntries = iter->second;
    else
    {
        lbEntries = BLAZE_NEW LeaderboardEntries();
        mSlaveSessionStatsMap[sessId] = lbEntries;
    }

    // If this is the first request for this leaderboard from the given slave, buffer up the entire leaderboard,
    // so that no intervening updates between fetches of chunks can corrupt the results
    if (request.getFirst() == 1)
    {
        // At this point, lbEntries really should be empty.
        // If not, then it could mean that a died slave was not cleaned up in onSlaveSessionRemoved().
        if (!lbEntries->empty())
        {
            WARN_LOG("[StatsMasterImpl].processPopulateLeaderboardIndex: buffered chunks of a leaderboard"
                " was not cleaned up from a previous start up of slave " << sessId);

            while (!lbEntries->empty())
            {
                lbEntries->pop_front();
            }
        }

        mLeaderboardIndexes->extract(request, lbEntries);
    }

    // If we still have some data let the client know, otherwise they will get empty back and know they are done
    if (!lbEntries->empty())
    {
        EA::TDF::TdfBlobPtr source = lbEntries->front();
        EA::TDF::TdfBlob& target = response.getData();
        target.setData(source->getData(), source->getSize());
        lbEntries->pop_front();
    }

    return PopulateLeaderboardIndexError::ERR_OK;
}

/*************************************************************************************************/
void StatsMasterImpl::onShutdown()
{
}

/*************************************************************************************************/
/*!
    \brief onWriteGlobalCacheToDatabase
    
    periodically invoked to instruct slaves to write cache to database.

*/
/*************************************************************************************************/
void StatsMasterImpl::onWriteGlobalCacheToDatabase()
{
    mGlobalCacheTimerId = INVALID_TIMER_ID;
    // if slave terminated during last cache serialization then
    // do it again but this time force it
    issueWriteInstructionToSlaves(mGlobalCacheIsBeingSerialized /*forceWrite*/);
}

void StatsMasterImpl::issueWriteInstructionToSlaves(bool forceWrite)
{
    if (mGlobalCacheSlaveIds.size() > 0)
    {
        // Since the mGlobalCacheSlaveIds may have ids removed while this function is running, we copy the values to a temp set and use that instead.
        InstanceIdSet tempIdSet = mGlobalCacheSlaveIds;

        InstanceIdSet::const_iterator it = tempIdSet.begin();

        // pick a slave randomly to start the loop -- it will write, the rest will reset
        eastl::advance(it, Blaze::Random::getRandomNumber(static_cast<int32_t>(tempIdSet.size())));
        InstanceIdSet::const_iterator start = it;

        // the slaves tracked in mGlobalCacheSlaveIds do host stats and are not "new"
        bool found = false;
        do
        {
            SlaveSession *session = mSlaveList.getSlaveSessionByInstanceId(*it);
            if (session != nullptr)
            {
                // send instructions to all the "non-new" stats slaves (one will write, the rest will reset);
                // "new" stats slaves don't have to do anything right now--they'll be told to rebuild after this write
                StatsGlobalCacheInstruction instruction;
                if (!found)
                {
                    found = true;
                    mGlobalCacheIsBeingSerialized = true;
                    // the lucky slave will write the cache
                    instruction.setInstruction(
                        forceWrite ? STATS_GLOBAL_CACHE_FORCE_WRITE : STATS_GLOBAL_CACHE_WRITE);
                }
                else
                {
                    // all other slaves will reset their row flags only
                    instruction.setInstruction(STATS_GLOBAL_CACHE_CLEAR);
                }
                sendExecuteGlobalCacheInstructionToSlaveSession(session, &instruction);
            }
            else
            {
                WARN_LOG("[StatsMasterImpl].issueWriteInstructionToSlaves: session with instance id " << (*it) << " not found.");
            }

            ++it;

            // We've hit the end of the list, so loop back around
            if (it == tempIdSet.end())
                it = tempIdSet.begin();
        } while (it != start);

        if (!found)
        {
            // this is very bad....
            ERR_LOG("[StatsMasterImpl].issueWriteInstructionToSlaves: Could not find slave to write global cache!")
        }
    }

    // schedule next run
    mGlobalCacheTimerId = gSelector->scheduleFiberTimerCall(
        TimeValue::getTimeOfDay() + TimeValue(GLOBAL_CACHE_SERIALIZATION_INTERVAL) , this, &StatsMasterImpl::onWriteGlobalCacheToDatabase,
            "StatsMasterImpl::onWriteGlobalCacheToDatabase");
}

ReportGlobalCacheInstructionExecutionResultError::Error 
    StatsMasterImpl::processReportGlobalCacheInstructionExecutionResult(
        const Blaze::Stats::GlobalCacheInstructionExecutionResult &request, const Message *message)
{
    mGlobalCacheIsBeingSerialized = false;
    if (request.getRowsWritten() >= 0)
    {
        if (mNewGlobalCacheSlaveIds.size() != 0)
        {
            // if new slaves were added instruct them to rebuild the cache
            // so that all operate on same data
            InstanceIdSet sessionIdSet = mNewGlobalCacheSlaveIds;
            StatsGlobalCacheInstruction instruction;
            instruction.setInstruction(STATS_GLOBAL_CACHE_REBUILD);
            InstanceIdSet::const_iterator newSlavesItr = sessionIdSet.begin();
            InstanceIdSet::const_iterator newSlavesEnd = sessionIdSet.end();
            for (; newSlavesItr != newSlavesEnd; ++newSlavesItr)
            {
                SlaveSession *session = mSlaveList.getSlaveSessionByInstanceId(*newSlavesItr);
                if (session == nullptr)
                {
                    WARN_LOG("[StatsMasterImpl].processReportGlobalCacheInstructionExecutionResult: session with instance id " << (*newSlavesItr) << " not found.");
                    continue;
                }
                sendExecuteGlobalCacheInstructionToSlaveSession(session, &instruction);
                mGlobalCacheSlaveIds.insert(*newSlavesItr);
                mNewGlobalCacheSlaveIds.erase(*newSlavesItr);
            }
        }
    }
    else if (request.getRowsWritten() < 0)
    {
        // something went wrong so redo the whole operation only now force write
        if (mGlobalCacheTimerId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mGlobalCacheTimerId);
            mGlobalCacheTimerId = INVALID_TIMER_ID;
        }
        issueWriteInstructionToSlaves(true /*forceWrite*/);
    }

    return ReportGlobalCacheInstructionExecutionResultError::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief processInitializeGlobalCache

    In order to avoid dispatching global stats notifications to notification listeners other than
    stats slaves (i.e. custom components who may be interested in stat rollover notifications) we
    require the stats slaves to call this RPC to explicitly identify themselves. 

    If this RPC comes in after the session has been added (i.e. onSlaveSessionAdded has already
    been called) then we can kick off synchroznization immediately.  If however it has not been
    added yet, then we will kick off synchronization once it presumably gets called later.
*/
/*************************************************************************************************/
InitializeGlobalCacheError::Error StatsMasterImpl::processInitializeGlobalCache(
        const Blaze::Stats::InitializeGlobalCacheRequest &request, const Message *message)
{
    mNewGlobalCacheSlaveIds.insert(request.getInstanceId());

    // global cache synchronization will start when this new stats slave is detected in onSlaveSessionAdded()
    // but in case the session for this instance is added before the instance calls this RPC ...
    SlaveSession *session = mSlaveList.getSlaveSessionByInstanceId(request.getInstanceId());
    if (session != nullptr)
    {
        startGlobalCacheSynchronization(*session);
    }

    return InitializeGlobalCacheError::ERR_OK;
}

} // Stats
} // Blaze
