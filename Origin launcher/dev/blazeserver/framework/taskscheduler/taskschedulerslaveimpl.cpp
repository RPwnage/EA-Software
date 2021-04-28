/*************************************************************************************************/
/*!
    \file   taskschedulermasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class TaskSchedulerSlaveImpl

    TaskScheduler Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "taskschedulerslaveimpl.h"

// global includes
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/connection/selector.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/protocol/shared/heat2encoder.h"
#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicator.h"
#include "framework/util/shared/rawbuffer.h"

namespace Blaze
{

static const int64_t TASK_LOCK_DURATION = 30 * 1000 * 1000;

static const char8_t DB_INSERT_TASK[] = 
    "INSERT INTO `task_scheduler` ("
        "`task_name`, `component_id`, `tdf_type`, `tdf_raw`, `start`, `duration`, `recurrence`)"
        " VALUES (?, ?, ?, ?, ?, ?, ?)";

/*! ***************************************************************/
/*! \name ScheduledTaskInfo
    \brief A wrapper class to handle all information required
           for scheduling a task.
*******************************************************************/
ScheduledTaskInfo::ScheduledTaskInfo()
: mTimerId(INVALID_TIMER_ID)
{
}

ScheduledTaskInfo::~ScheduledTaskInfo()
{
}

// static
TaskSchedulerSlave* TaskSchedulerSlave::createImpl()
{
    return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "Component") TaskSchedulerSlaveImpl();
}

/*** Public Methods ******************************************************************************/
TaskSchedulerSlaveImpl::TaskSchedulerSlaveImpl()
: mScheduledTasks(BlazeStlAllocator("ScheduledTasks", COMPONENT_MEMORY_GROUP)),
  mCanceledTasks(BlazeStlAllocator("CanceledTasks", COMPONENT_MEMORY_GROUP)),
  mHandlers(BlazeStlAllocator("Handlers", COMPONENT_MEMORY_GROUP)),
  mTaskInsertStatementId(0),
  mTaskIdLockMap(Blaze::RedisId(COMPONENT_INFO.name, "taskIdLockMap"), false)
{
}

TaskSchedulerSlaveImpl::~TaskSchedulerSlaveImpl()
{
    // Clean out the EA::TDF::Tdf* and the TaskInfo* in the
    // scheduled tasks map
    TaskMap::iterator taskIter = mScheduledTasks.begin();
    TaskMap::iterator taskEnd = mScheduledTasks.end();

    for (; taskIter != taskEnd; taskIter++)
    {
        if (taskIter->second.getTimerId() != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(taskIter->second.getTimerId());
        }
    }
    mScheduledTasks.clear();

    mCanceledTasks.clear();
}

/*! ***************************************************************/
/*! \brief Configuration sets up database, finds saved tasks..
*******************************************************************/
bool TaskSchedulerSlaveImpl::onValidateConfig(TaskSchedulerConfig& config, const TaskSchedulerConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    if (config.getOneTimeTaskUpdateInterval() < TimeValue(5000))
    {
        eastl::string msg;
        msg.sprintf("[TaskSchedulerSlaveImpl].onValidateConfig(): oneTimeTaskUpdateInterval cannot be shorter than 5s");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    if (config.getOneTimeTaskMaxIdleUpdatePeriods() < 2)
    {
        eastl::string msg;
        msg.sprintf("[TaskSchedulerSlaveImpl].onValidateConfig(): oneTimeTaskMaxIdleUpdatePeriods cannot be less than 2");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    return validationErrors.getErrorMessages().empty();
}

bool TaskSchedulerSlaveImpl::onConfigure()
{
    mTaskInsertStatementId = gDbScheduler->registerPreparedStatement(getDbId(), "task_insert", DB_INSERT_TASK);
    return loadTasksFromDatabase();
}

/*! ***************************************************************/
/*! \brief Register your component with the task scheduler
        \param component id - id of the registering component
        \param handler - a pointer to the component object that is
                         registering
*******************************************************************/
bool TaskSchedulerSlaveImpl::registerHandler(ComponentId compId, TaskEventHandler* handler)
{
    const char8_t* componentName = BlazeRpcComponentDb::getComponentNameById(compId);
    if(handler == nullptr)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].registerHandler: nullptr handler passed in for component(" << componentName << "), can't register.");
        return false;
    }
    if(mHandlers.find(compId) != mHandlers.end())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].registerHandler: Component(" << componentName << ") already registered.");
        return false;
    }
    mHandlers[compId] = handler;
    
    // setup all the tasks for this component
    ASSERT_WORKER_FIBER();
    TaskMap::iterator itr = mScheduledTasks.begin();
    TaskMap::iterator end = mScheduledTasks.end();
    for (; itr != end; ++itr)
    {
        if (itr->second.getTaskInfo().getCompId() == compId)
        {
            const TimeValue startTime(secToUsec(itr->second.getTaskInfo().getStart()));
            const TimerId timerId = gSelector->scheduleFiberTimerCall(startTime, 
                this, &TaskSchedulerSlaveImpl::onTaskExpiration, itr->first,
                "TaskSchedulerSlaveImpl::onTaskExpiration");
            itr->second.setTimerId(timerId);

            signalOnScheduledTask(itr->second.getTaskInfo());
        }
    }

    BLAZE_INFO_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].registerHandler: Component(" << componentName << ") now registered with task_scheduler.");
    return true;
}

/*! ***************************************************************/
/*! \brief Deregister your component with the scheduler
        \param component id
*******************************************************************/
bool TaskSchedulerSlaveImpl::deregisterHandler(ComponentId compId)
{
    const char8_t* componentName = BlazeRpcComponentDb::getComponentNameById(compId);
    HandlersByComponent::iterator it = mHandlers.find(compId);
    if(it == mHandlers.end())
    {
        BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].deregisterHandler: Could not find component(" << componentName << ") in registration list.");
        return false;
    }
    mHandlers.erase(it);

    BLAZE_INFO_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].deregisterHandler: Component(" << componentName << ") de-registered with task_scheduler.");
    return true;
}

/*! ***************************************************************/
/*! \brief Schedule a task to be called in the future
        \param taskName - a unique name for the task
        \param component id
        \param tdf - a new tdf object to be stored and returned when
                     task is executed.  This memory is now owned by 
                     the task scheduler.
        \param start - epoch time of task to start in seconds
        \param duration - the length of time the task could possibly
                          start in seconds
        \param recurrence - the length of time between recurrences of
                            this task in seconds. Default 0, which means
                            no recurrence.
*******************************************************************/
void TaskSchedulerSlaveImpl::createTask(const char8_t* taskName, ComponentId compId, EA::TDF::Tdf* tdf, 
    uint32_t start/*= 0*/, uint32_t duration/*= 0*/, uint32_t recurrence/*= 0*/)
{
    if (!Fiber::isCurrentlyBlockable())
    {
        // Schedule a fiber to save task information to the database (must be non-blocking)
        Fiber::CreateParams params(Fiber::STACK_SMALL, false);
        gSelector->scheduleFiberCall(this, &TaskSchedulerSlaveImpl::saveScheduledTaskToDatabase, 
            taskName, compId, tdf, start, duration, recurrence, "TaskSchedulerSlaveImpl::saveScheduledTaskToDatabase", params);
    }
    else
    {
        saveScheduledTaskToDatabase(taskName, compId, tdf, start, duration, recurrence);
    }
}

/*! ***************************************************************/
/*! \brief Schedule a task to be called in the future and return the TaskId
           Must be running on Worker Fiber
        \param taskName - a unique name for the task
        \param component id
        \param tdf - a new tdf object to be stored and returned when
                     task is executed.  This memory is now owned by 
                     the task scheduler.
        \param start - epoch time of task to start in seconds
        \param duration - the length of time the task could possibly
                          start in seconds
        \param recurrence - the length of time between recurrences of
                            this task in seconds. Default 0, which means
                            no recurrence.
*******************************************************************/
TaskId TaskSchedulerSlaveImpl::createTaskAndGetTaskId(const char8_t* taskName, ComponentId compId, EA::TDF::Tdf* tdf, 
    uint32_t start/*= 0*/, uint32_t duration/*= 0*/, uint32_t recurrence/*= 0*/)
{
    ASSERT_WORKER_FIBER();
    ScheduledTaskInfo taskInfo;
    BlazeRpcError err = saveTaskToDatabase(taskName, compId, tdf, start, duration, recurrence, taskInfo);
    if (err != ERR_OK)
        return INVALID_TASK_ID;
    sendTaskScheduledToAllSlaves(&(taskInfo.getTaskInfo()));
    return taskInfo.getTaskInfo().getTaskId();
}

/*! ***************************************************************/
/*! \brief Run a one time task now on a single server instance, and
           block until task is in COMPLETED status for all other instances.
           Useful for scheduling/coordinating startup tasks.
        \param taskName - a unique name for the task
        \param component id
        \param cb - a callback method that would execute the task
*******************************************************************/
BlazeRpcError TaskSchedulerSlaveImpl::runOneTimeTaskAndBlock(const char8_t* taskName, ComponentId compId, const RunOneTimeTaskCb& cb)
{
    if (!Fiber::isCurrentlyBlockable())
    {
        // Schedule a fiber to save task information to the database (must be non-blocking)
        Fiber::CreateParams params(Fiber::STACK_SMALL, false);
        gSelector->scheduleFiberCall(this, &TaskSchedulerSlaveImpl::saveOneTimeTaskToDatabase, 
            taskName, compId, (EA::TDF::Tdf*)nullptr, (BlazeRpcError*)nullptr, &cb, "TaskSchedulerSlaveImpl::saveOneTimeTaskToDatabase", params);
        return ERR_OK;
    }
    else
    {
        BlazeRpcError error = ERR_SYSTEM;
        saveOneTimeTaskToDatabase(taskName, compId, (EA::TDF::Tdf*)nullptr, &error, &cb);
        return error;
    }
}

/*! ***************************************************************/
/*! \brief Cancel a scheduled task
        \param task Id
*******************************************************************/
bool TaskSchedulerSlaveImpl::cancelTask(TaskId taskId)
{
    // remove the task from the database even if the task is not scheduled or finishes running
    if (!Fiber::isCurrentlyBlockable())
    {
        // Schedule a fiber to remove the task information from the database (must be non-blocking)
        Fiber::CreateParams params(Fiber::STACK_SMALL, false);
        gSelector->scheduleFiberCall(this, &TaskSchedulerSlaveImpl::removeTaskFromDatabase, 
            taskId, "TaskSchedulerSlaveImpl::removeTaskFromDatabase", params);
    }
    else
    {
        removeTaskFromDatabase(taskId);
    }

    TaskMap::iterator taskItr = mScheduledTasks.find(taskId);
    if (taskItr == mScheduledTasks.end())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].cancelTask: Could not find timer associated with task TaskId=" << taskId << ".");
        return false;
    }

    TaskInfo info;
    info.setTaskId(taskId);
    // framework triggers the local notification handler before going on to encode the notification for other slaves
    // since we will erase this task from our mScheduledTasks in onTaskCanceled(), if we just pass in the TaskInfo*
    // from mScheduledTasks, it will be deleted locally before notifications to other instances are encoded
    sendTaskCanceledToAllSlaves(&info, false);
    return true;
}

 /*! ***************************************************************/
 /*! \brief Get all the tasks for a particular component
*******************************************************************/
bool TaskSchedulerSlaveImpl::getComponentTaskByIdMap(const ComponentId compId, TaskTdfByIdMap& taskIdMap)
{
    TaskMap::iterator itr = mScheduledTasks.begin();
    TaskMap::iterator end = mScheduledTasks.end();
    for (; itr != end; ++itr)
    {
        if (itr->second.getTaskInfo().getCompId() == compId)
        {
            taskIdMap[itr->first] = itr->second.getTaskInfo().getTdf();
        }
    }
   
    return true;
}

 /*! ***************************************************************/
 /*! \brief Get a specific task identified by the taskId
*******************************************************************/
const ScheduledTaskInfo* TaskSchedulerSlaveImpl::getTask(TaskId taskId) const
{
    TaskMap::const_iterator itr = mScheduledTasks.find(taskId);
    TaskMap::const_iterator end = mScheduledTasks.end();
    if (itr == end)
        return nullptr;
    return &(itr->second);

}


/*! ***************************************************************/
/*! \brief The method that is called when a NOTIFY_TASKSCHEDULED
           notification is received
*******************************************************************/
void TaskSchedulerSlaveImpl::onTaskScheduled(const Blaze::TaskInfo& data,UserSession *associatedUserSession)
{
    if (getState() >= SHUTTING_DOWN)
        return; // do nothing, we're shutting down

    // add task to map
    ScheduledTaskInfo& scheduledTask =  mScheduledTasks[data.getTaskId()];
    data.copyInto(scheduledTask.getTaskInfo());

    // Schedule the task for execution only if successfully written to DB
    const TimeValue startTime(secToUsec(data.getStart()));
    const TimerId timerId = gSelector->scheduleFiberTimerCall(startTime, 
        this, &TaskSchedulerSlaveImpl::onTaskExpiration, data.getTaskId(),
        "TaskSchedulerSlaveImpl::onTaskExpiration");
    scheduledTask.setTimerId(timerId);

    const char8_t* componentName = BlazeRpcComponentDb::getComponentNameById(data.getCompId());

    if (data.getTdf() != nullptr)
    {
        BLAZE_INFO_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskScheduled: "
            "Scheduled task for component=" << componentName << ", task_name=" << data.getTaskName() << ", start=" << data.getStart() << "s, duration=" << data.getDuration() << "s, "
            "recurrence=" << data.getRecurrence() << ", taskId=" << data.getTaskId() << ", tdf_id=" << data.getTdfId() << ", tdf=\n" << data.getTdf());
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskScheduled: "
            "Scheduled task for component=" << componentName << ", task_name=" << data.getTaskName() << ", start=" << data.getStart() << "s, duration=" << data.getDuration() << "s, "
            "recurrence=" << data.getRecurrence() << ", taskId=" << data.getTaskId() << ", tdf_id=" << data.getTdfId() << ", tdf=nullptr");
    }

    gSelector->scheduleFiberCall(this, &TaskSchedulerSlaveImpl::signalOnScheduledTaskById, 
        data.getTaskId(), "TaskSchedulerSlaveImpl::signalOnScheduledTaskById");
}

/*! ***************************************************************/
/*! \brief The method that is called when a NOTIFY_TASKCANCELED
           notification is received
*******************************************************************/
void TaskSchedulerSlaveImpl::onTaskCanceled(const Blaze::TaskInfo& data,UserSession *associatedUserSession)
{
    TaskMap::iterator taskItr = mScheduledTasks.find(data.getTaskId());
    if (taskItr == mScheduledTasks.end())
    {
        BLAZE_ERR_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskCanceled: Could not find timer associated with task TaskId=" << data.getTaskId() << ".");
        return;
    }

    // For recurring tasks, if we detect that server is shutting down, we will not schedule the next recurrence
    // and will therefore have INVALID_TIMER_ID as the task timerId
    if (taskItr->second.getTimerId() != INVALID_TIMER_ID)
    {
        if (gSelector->cancelTimer(taskItr->second.getTimerId()))
        {
            // Copy the task info so that info is available after it is erased from the scheduled task map/list.
            // A member map is used to ensure proper clean up in case there are "unfinished" cancels on dtor.
            TaskInfo& canceledTaskInfo = mCanceledTasks[data.getTaskId()];
            (taskItr->second.getTaskInfo()).copyInto(canceledTaskInfo);
            gSelector->scheduleFiberCall(this, &TaskSchedulerSlaveImpl::signalOnCanceledTaskById,
                data.getTaskId(), "TaskSchedulerSlaveImpl::signalOnCanceledTaskById");

            mScheduledTasks.erase(data.getTaskId());
        }
        else
        {
            BLAZE_ERR_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskCanceled: Could not cancel timer " 
                << taskItr->second.getTimerId() << " associated with task TaskId=" << data.getTaskId() << ".");
        }
    }
}

/*! ***************************************************************/
/*! \brief The method that is called when scheduled task is up
        \param task id
*******************************************************************/
void TaskSchedulerSlaveImpl::onTaskExpiration(TaskId taskId)
{
    TaskMap::iterator valueItr = mScheduledTasks.find(taskId);
    if (valueItr == mScheduledTasks.end())
    {
        BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: Could not find task info for task " << taskId << ".");
        return;
    }

    // if recurrence is set, schedule next expiration timer
    // so that recurrence is not lost even if we encounter
    // failures later
    TimerId timerId = INVALID_TIMER_ID;
    TaskInfo taskInfo;
    // we copy off the task info so we can use it after any blocking DB calls below.
    valueItr->second.getTaskInfo().copyInto(taskInfo);

    if (taskInfo.getRecurrence() > 0 && getState() < SHUTTING_DOWN)
    {
        const TimeValue startTime(TimeValue::getTimeOfDay() + secToUsec(taskInfo.getRecurrence()));
        timerId = gSelector->scheduleFiberTimerCall(startTime, 
            this, &TaskSchedulerSlaveImpl::onTaskExpiration, taskInfo.getTaskId(),
            "TaskSchedulerSlaveImpl::onTaskExpiration");
    }
    valueItr->second.setTimerId(timerId);

    // Find a registered handler for this task
    HandlersByComponent::iterator it = mHandlers.find(taskInfo.getCompId());
    if(it == mHandlers.end())
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: Could not find handler for task " << taskId << ".");
        return;
    }
    else
    {
        if (!getLockFromRedis(taskId))
        {
            // Someone beat us to the lock - let them have this task.
            BLAZE_TRACE_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: Skipping taskId " << taskId << " since someone beat us to it.");
            return;
        }
        // We got the lock from Redis for this taskId (and only we have this lock).
        // Now we can select and update this task in the db without fear of contention (until our lock expires at least).

        // this else block is a bit of trickery to limit the scope of the DbConnPtr so that the connection
        // is released back to the pool as soon as we're done with it, before we call signalOnExecutedTask()

        // attempt to obtain lock on the dbrow for this task to ensure this is only executed once
        DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
        if (conn == nullptr)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: Could not get database connection for task " << taskId << ".");
            releaseLockFromRedis(taskId);
            return;
        }

        BlazeRpcError error = ERR_SYSTEM;
        error = conn->startTxn();
        if (error != ERR_OK)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: Could not start database txn for task " << taskId << ".");
            releaseLockFromRedis(taskId);
            return;
        }

        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        query->append("SELECT * FROM task_scheduler WHERE task_id = $D FOR UPDATE", taskId);
        DbResultPtr dbResult;
        error = conn->executeQuery(query, dbResult);
        if (error != DB_ERR_OK || dbResult->empty())
        {
            BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: Could not fetch database row for task " << taskId << 
                ", error " << ErrorHelp::getErrorName(error) << ".");
            conn->rollback();
            if (error == DB_ERR_OK)
            {
                // cancel the timer and erase this task from our map since it doesn't exist in DB.
                onTaskCanceled(taskInfo, nullptr);
            }
            releaseLockFromRedis(taskId);
            return;
        }

        const DbRow *row = *(dbResult->begin());
        const uint32_t start = row->getUInt("start");
        const uint32_t recurrence = row->getUInt("recurrence");
        const uint32_t status = row->getUInt("status");

        // Check if someone has beaten us to this task.  If so, do nothing
        const TimeValue currentTime(TimeValue::getTimeOfDay());
        if ((recurrence > 0 && start > currentTime.getSec()) ||
            status != TASK_STATUS_SCHEDULED)
        {
            if (recurrence == 0)
            {
                mScheduledTasks.erase(taskId);
            }
            BLAZE_TRACE_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: task " << taskId << " has already been executed and updated.");
            conn->rollback();
            releaseLockFromRedis(taskId);
            return; 
        }

        DB_QUERY_RESET(query);
        if (recurrence > 0)
        {
            // If this is a recurring task, update the start time to the next recurrence from now
            query->append("UPDATE task_scheduler SET start=$d WHERE task_id = $D", (currentTime.getSec() + recurrence), taskId);
            error = conn->executeQuery(query, dbResult);
            if (error != DB_ERR_OK)
            {
                BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: Could not update start time for recurring task " << taskId << 
                    ", error " << ErrorHelp::getErrorName(error) << ".");
                conn->rollback();
                releaseLockFromRedis(taskId);
                return;
            }
        }
        else
        {
            // This is not a recurring task, change status to complete and erase it from our map
            // We erase by key to avoid issues with fiber sleeps from db calls above.
            query->append("UPDATE task_scheduler SET status=$d WHERE task_id = $D", TASK_STATUS_COMPLETED, taskId);
            error = conn->executeQuery(query, dbResult);
            if (error != DB_ERR_OK)
            {
                BLAZE_INFO_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: Could not update status for completed task " << taskId << 
                    ", error " << ErrorHelp::getErrorName(error) << ".");
                conn->rollback();
                releaseLockFromRedis(taskId);
                return;
            }
            mScheduledTasks.erase(taskId);
        }
        error = conn->commit();
        if (error != DB_ERR_OK)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: Could not commit modifications for task " << taskId <<
                ", error " << ErrorHelp::getErrorName(error) << ".");
            releaseLockFromRedis(taskId);
            return;
        }

        // Now that we are done, let's go ahead and release our Redis lock so we aren't blocking longer than needed.
        releaseLockFromRedis(taskId);
    }

    // Signal the handler to execute task
    signalOnExecutedTask(taskInfo);
}


/*! ***************************************************************/
/*! \brief Uses Redis to ensure mutual exclusion.  Returns true if we got the lock.
*******************************************************************/
bool TaskSchedulerSlaveImpl::getLockFromRedis(TaskId taskId)
{
    bool didGetLock = false;

    const eastl::string taskIdString(eastl::string::CtorSprintf(), "%" PRIu64, taskId);

    // Need at least enough time to do our select and update (we will remove the lock when finished).
    const EA::TDF::TimeValue timeToLive = TASK_LOCK_DURATION;

    int64_t redisResult = 0;
    Blaze::RedisError redisError = mTaskIdLockMap.insertWithTtl(taskIdString, gController->getInstanceName(), timeToLive, &redisResult);
    if (redisError != Blaze::REDIS_ERR_OK)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: Failed to acquire lock for taskId " << taskId << " due to RedisError " << redisError);
    }
    else if (redisResult == 1)
    {
        didGetLock = true;
        BLAZE_TRACE_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: Lock successfully acquired for taskId " << taskId);
    }

    return didGetLock;
}


/*! ***************************************************************/
/*! \brief Releases a lock from Redis.
*******************************************************************/
void TaskSchedulerSlaveImpl::releaseLockFromRedis(TaskId taskId)
{
    const eastl::string taskIdString(eastl::string::CtorSprintf(), "%" PRIu64, taskId);
    int64_t redisResult = 0;

    Blaze::RedisError redisError = mTaskIdLockMap.erase(taskIdString, &redisResult);

    if (redisError != Blaze::REDIS_ERR_OK || redisResult != 1)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onTaskExpiration: Failed to remove lock for taskId " << taskId <<
            " due to RedisError " << redisError << " and redisResult " << redisResult);
    }
}


/*! ***************************************************************/
/*! \brief The method that is called when runOneTimeTaskAndBlock() 
           is invoked.  The task will be executed by a single server
           instance and all other server instances will block until
           the task is in COMPLETED status.
        \param taskInfo - all the info related to this one time task
        \param cb - the callback to invoke to execute the task
*******************************************************************/
BlazeRpcError TaskSchedulerSlaveImpl::onRunOneTimeTask(ScheduledTaskInfo& taskInfo, const RunOneTimeTaskCb& cb)
{
    while (true)
    {
        // attempt to obtain lock on the dbrow for this task to ensure this is only executed once
        DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
        if (conn != nullptr)
        {
            BlazeRpcError error = ERR_SYSTEM;
            error = conn->startTxn();
            if (error != ERR_OK)
            {
                BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onRunOneTimeTask: Could not start database txn for task (" << taskInfo.getTaskInfo().getTaskName() << ").");
                return error;
            }
            QueryPtr query = DB_NEW_QUERY_PTR(conn);
            query->append("SELECT task_id, status, (UNIX_TIMESTAMP(NOW()) - UNIX_TIMESTAMP(last_update)) as since_last_update FROM task_scheduler WHERE task_name = '$s' FOR UPDATE", taskInfo.getTaskInfo().getTaskName());
            DbResultPtr dbResult;
            error = conn->executeQuery(query, dbResult);              
            if (error == DB_ERR_OK &&
                !dbResult->empty())
            {
                const DbRow *row = *(dbResult->begin());
                const TaskStatus status = static_cast<TaskStatus>(row->getUInt("status"));
                int64_t secsSinceLastUpdate = row->getInt64("since_last_update");

                // Return immediately if task is completed.  Execute the task if it hasn't been executed, or the instance executing it seems to have died.
                // Otherwise wait until the task is completed.
                if (status >= TASK_STATUS_COMPLETED)
                {
                    error = conn->commit();
                    return ERR_OK;
                }

                if (status == TASK_STATUS_INPROGRESS)
                {
                    // we should wait until completion
                    if (secsSinceLastUpdate < getConfig().getOneTimeTaskUpdateInterval().getSec() * getConfig().getOneTimeTaskMaxIdleUpdatePeriods())
                    {
                        error = conn->commit();
                        error = gSelector->sleep(getConfig().getOneTimeTaskUpdateInterval());
                        if (error != ERR_OK)
                        {
                            return error;
                        }
                        continue;
                    }
                }

                if (status == TASK_STATUS_SCHEDULED ||
                    status == TASK_STATUS_INPROGRESS)
                {
                    if (status == TASK_STATUS_INPROGRESS)
                    {
                        BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onRunOneTimeTask: OneTimeTaskMaxIdleUpdatePeriods exceeded.  Taking over the running of task (" 
                            << taskInfo.getTaskInfo().getTaskName() << ")");
                    }
                    // we should update status to INPROGRESS and begin execution of task
                    const TaskId taskId = static_cast<TaskId>(row->getInt64("task_id"));
                    DB_QUERY_RESET(query);
                    query->append("UPDATE task_scheduler SET status = $d, last_update = NOW() WHERE task_id = $D", TASK_STATUS_INPROGRESS, taskId);
                    error = conn->executeQuery(query);
                    if (error != ERR_OK)
                    {
                        BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onRunOneTimeTask: Could not update status to INPROGRESS for task (" 
                            << taskInfo.getTaskInfo().getTaskName() << "), error " << ErrorHelp::getErrorName(error));
                        conn->rollback();
                        return error;
                    }
                    error = conn->commit();
                    if (error == ERR_OK)
                    {
                        // add the scheduled one-time task to the map so that future periodic updates
                        // can use the map to check to see if this task is still valid
                        ScheduledTaskInfo& scheduledTask = mScheduledTasks[taskId];
                        taskInfo.getTaskInfo().copyInto(scheduledTask.getTaskInfo());

                        // kick off periodic update
                        Fiber::CreateParams params(Fiber::STACK_SMALL, false);
                        TimeValue nextUpdate = TimeValue::getTimeOfDay() + getConfig().getOneTimeTaskUpdateInterval();
                        TimerId timerId = gSelector->scheduleFiberTimerCall(nextUpdate, 
                            this, &TaskSchedulerSlaveImpl::updateOneTimeTask, taskId, nextUpdate, "TaskSchedulerSlaveImpl::updateOneTimeTask", params);
                        taskInfo.setTimerId(timerId);
                        // notify the component to execute the task
                        cb(error);
                        mScheduledTasks.erase(taskId);
                        if (!gSelector->cancelTimer(taskInfo.getTimerId()))
                        {
                            BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onRunOneTimeTask: Could not cancel update timer for task (" 
                                << taskInfo.getTaskInfo().getTaskName() << ").");
                        }
                        // once task has completed execution, we should mark it COMPLETED
                        if (error == ERR_OK)
                        {
                            DB_QUERY_RESET(query);
                            query->append("UPDATE task_scheduler SET STATUS = $d WHERE task_id = $D", TASK_STATUS_COMPLETED, taskId);
                            error = conn->executeQuery(query);
                            if (error != ERR_OK)
                            {
                                BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onRunOneTimeTask: Could not update status to COMPLETED for task (" 
                                    << taskInfo.getTaskInfo().getTaskName() << "), error " << ErrorHelp::getErrorName(error));
                            }
                        }
                        return error;
                    }
                    else
                    {
                        BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onRunOneTimeTask: Could not update status to INPROGRESS for task (" 
                            << taskInfo.getTaskInfo().getTaskName() << "), error " << ErrorHelp::getErrorName(error));
                        conn->rollback();
                        return error;
                    }
                }
            }
            else
            {
                BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onRunOneTimeTask: Could not fetch db entry for task (" << taskInfo.getTaskInfo().getTaskName() << "), error " <<
                    ErrorHelp::getErrorName(error));
                conn->rollback();
                return ERR_SYSTEM;
            }
        }
        else
        {
            BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].onRunOneTimeTask: Could not get database connection for task (" << taskInfo.getTaskInfo().getTaskName() << ").");
            return ERR_SYSTEM;
        }
    }
}

/*! ***************************************************************/
/*! \brief load all saved tasks form the database
*******************************************************************/
bool TaskSchedulerSlaveImpl::loadTasksFromDatabase()
{
    const bool deleteExpiredTasks = gController->isInstanceTypeConfigMaster();
    DbConnPtr conn = deleteExpiredTasks ? gDbScheduler->getConnPtr(getDbId()) : gDbScheduler->getLagFreeReadConnPtr(getDbId());
    if (conn != nullptr)
    {
        const int64_t now = TimeValue::getTimeOfDay().getSec();
        QueryPtr query = DB_NEW_QUERY_PTR(conn);

        // Grab the tasks that are within their expiration window and delete expired tasks        
        if (deleteExpiredTasks)
        {
            // only run the delete on the configMaster so that startup task entries cannot be deleted by server instances starting up later
            query->append("DELETE from task_scheduler WHERE $D > (`start` + `duration`) AND recurrence = 0;", now);
        }
        query->append("SELECT `task_id`, `task_name`, `component_id`, `tdf_type`, `tdf_raw`, `start`, `duration`, `recurrence` "
            "FROM `task_scheduler` ORDER BY task_id;");

        DbResultPtrs dbResults;
        const BlazeRpcError error = conn->executeMultiQuery(query, dbResults);
        if (error == DB_ERR_OK)
        {
            const uint32_t deletedCount = dbResults.front()->getAffectedRowCount();
            if (deletedCount > 0)
            {
                BLAZE_INFO_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].loadTasksFromDatabase: Deleted " << deletedCount << " expired tasks.")
            }
            DbResultPtr& dbResult = dbResults.back();
            DbResult::const_iterator rowIter = dbResult->begin();
            DbResult::const_iterator rowEnd =  dbResult->end();
            for (; rowIter != rowEnd; ++rowIter)
            {
                size_t len;
                EA::TDF::Tdf* tdf = nullptr;
                const DbRow *row = *rowIter;
                const ComponentId compId = static_cast<ComponentId>(row->getUInt("component_id"));
                const char8_t* componentName = BlazeRpcComponentDb::getComponentNameById(compId);
                const uint8_t* bin = row->getBinary("tdf_raw", &len);
                const TaskId taskId = row->getUInt("task_id");
                const EA::TDF::TdfId tdfType = row->getUInt("tdf_type");
                TaskInfo& task = mScheduledTasks[taskId].getTaskInfo();
                if (len > 0)
                {
                    tdf = EA::TDF::TdfFactory::get().create(tdfType, getAllocator(), "TaskSchedulerTdf");
                    if (tdf == nullptr)
                    {
                        BLAZE_ERR_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].loadTasksFromDatabase: "
                            "TaskScheduler requires task TDF to be defined as variable TDF to be registered for "
                            "component(" << componentName << ") in order to build task(" << taskId << ") with TDF of size(" << len << "),skipping...");
                        continue;
                    }
                }
                task.setTaskName(row->getString("task_name"));
                task.setCompId(compId);
                task.setTaskId(taskId);
                task.setStart(row->getUInt("start"));
                task.setDuration(row->getUInt("duration"));
                task.setRecurrence(row->getUInt("recurrence"));
                if (tdf != nullptr)
                {
                    //EA::TDF::Tdf takes ownership of this RawBuffer
                    Blaze::RawBuffer raw(const_cast<uint8_t*>(bin), len);
                    raw.put(len);
                    Heat2Decoder decoder;
                    // Decode the TDF that we stored to pass back to
                    // calling component
                    if (decoder.decode(raw, *tdf) != Blaze::ERR_OK)
                    {
                        BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].loadTasksFromDatabase: "
                            "Failed to decode a TDF of size(" << len << ") for component(" << componentName << ") task(" << taskId << ") stored in database.");
                    }
                    else
                    {
                        task.setTdf(*tdf);
                    }
                }
                // fixup start times for recurring tasks.  The database row for this recurring task
                // will be fixed up the next time this recurring task is executed
                if (task.getRecurrence() > 0)
                {
                    while(task.getStart() < TimeValue::getTimeOfDay().getSec())
                    {
                        task.setStart(task.getStart() + task.getRecurrence());
                    }
                }
            }
            const size_t validCount = dbResult->size();
            if (validCount > 0)
            {
                BLAZE_INFO_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].loadTasksFromDatabase: Loaded " << validCount << " scheduled tasks.");
            }
            // tasks loaded
            return true;
        }
        else
        {
            BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].loadTasksFromDatabase: "
                           << "Failed to fetch scheduled tasks from the database, error: " << ErrorHelp::getErrorName(error) << ".");
        }
    }
    else
    {
        BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].loadTasksFromDatabase: Failed to fetch scheduled tasks from the database, no database connection.");
    }

    return false;
}

/*! ***************************************************************/
/*! \brief periodically updates the one time task that is still in progress
*******************************************************************/
void TaskSchedulerSlaveImpl::updateOneTimeTask(TaskId taskId, TimeValue scheduledTime)
{
    TaskMap::iterator taskItr = mScheduledTasks.end();

    DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
    BlazeRpcError error = ERR_SYSTEM;
    if (conn != nullptr)
    {
        // make sure this task is still valid
        taskItr = mScheduledTasks.find(taskId);
        if (taskItr == mScheduledTasks.end())
            return; // the task is no longer in the map, so bail

        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        query->append("UPDATE task_scheduler SET status = $d, last_update = NOW() WHERE task_id = $D", TASK_STATUS_INPROGRESS, taskId);
        error = conn->executeQuery(query);
    }

    // DB calls above are blocking calls, check again
    taskItr = mScheduledTasks.find(taskId);
    if (taskItr == mScheduledTasks.end())
        return; // the task is no longer in the map, so bail

    ScheduledTaskInfo& taskInfo = taskItr->second;
    TimeValue now = TimeValue::getTimeOfDay();
    if ((now - scheduledTime) >= (getConfig().getOneTimeTaskUpdateInterval() * getConfig().getOneTimeTaskMaxIdleUpdatePeriods()))
    {
        BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].updateOneTimeTask: Execution of the update was delayed beyond configured threshold for OneTimeTaskMaxIdelUpdatePeriods for task (" 
            << taskInfo.getTaskInfo().getTaskName() << " ).  Aborting future updates as another instance will have taken over the execution of this task." );
        return;
    }
    if (error != Blaze::ERR_OK)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].updateOneTimeTask: Failed to update taskId (" << taskId 
            << ") with progress TASK_STATUS_INPROGRESS , error " << ErrorHelp::getErrorName(error));
        return;
    }

    Fiber::CreateParams params(Fiber::STACK_SMALL, false);
    TimerId timerId = gSelector->scheduleFiberTimerCall((TimeValue::getTimeOfDay() + getConfig().getOneTimeTaskUpdateInterval()), this, &TaskSchedulerSlaveImpl::updateOneTimeTask, 
        taskId, now, "TaskSchedulerSlaveImpl::updateOneTimeTask", params);
    taskInfo.setTimerId(timerId);
}

/*! ***************************************************************/
/*! \brief Save the task to the database
        \param task information
*******************************************************************/
void TaskSchedulerSlaveImpl::saveOneTimeTaskToDatabase(const char8_t* taskName, ComponentId compId, EA::TDF::Tdf* tdf, BlazeRpcError* error, const RunOneTimeTaskCb* const cb)
{
    ScheduledTaskInfo taskInfo;
    BlazeRpcError err = saveTaskToDatabase(taskName, compId, tdf, 0, 0, 0, taskInfo);

    // ERR_OK - We UPDATEd the value before anyone else
    // DB_ERR_DUP_ENTRY - Someone else already UPDATEd the value before we tried to INSERT it
    // DB_ERR_LOCK_DEADLOCK - Someone else called UPDATE at the same time that we were trying to INSERT it
    if (err == ERR_OK || err == DB_ERR_DUP_ENTRY || err == DB_ERR_LOCK_DEADLOCK)
    {
        if (cb != nullptr)
        {
            err = onRunOneTimeTask(taskInfo, *cb);
        }
        else
        {
            err = ERR_OK;       // Replace the error, since all three ERR codes are valid.
        }
    }
    if (error != nullptr)
        *error = err;
}

void TaskSchedulerSlaveImpl::saveScheduledTaskToDatabase(const char8_t* taskName, ComponentId compId, EA::TDF::Tdf* tdf, uint32_t start, uint32_t duration, uint32_t recurrence)
{
    ScheduledTaskInfo taskInfo;
    BlazeRpcError err = saveTaskToDatabase(taskName, compId, tdf, start, duration, recurrence, taskInfo);
    if (err == ERR_OK)
        sendTaskScheduledToAllSlaves(&(taskInfo.getTaskInfo()));
}

BlazeRpcError TaskSchedulerSlaveImpl::saveTaskToDatabase(const char8_t* taskName, ComponentId compId, EA::TDF::Tdf* tdf, uint32_t start, uint32_t duration, 
                                                         uint32_t recurrence,  ScheduledTaskInfo& schedTaskInfo)
{
    // First encode the TDF for raw storage
    Blaze::RawBuffer raw(4000);
    Heat2Encoder encoder;

    TaskInfo& taskInfo = schedTaskInfo.getTaskInfo();
    taskInfo.setTaskName(taskName);
    taskInfo.setCompId(compId);
    if (tdf != nullptr)
    {
        // take ownership of the tdf
        taskInfo.setTdf(*tdf);
    }
    taskInfo.setStart(start);
    taskInfo.setDuration(duration);
    taskInfo.setRecurrence(recurrence);

    if (tdf != nullptr)
    {
        encoder.encode(raw, *(tdf));
        if (raw.datasize() > 4000)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].saveTaskToDatabase: "
                "Could not save task to database, serialized TDF size exceeds 4000 bytes for EA::TDF::TdfId=" << tdf->getTdfId() << ".");
            return ERR_SYSTEM;
        }
    }

    DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
    if (conn != nullptr)
    {
        PreparedStatement* stmt = conn->getPreparedStatement(mTaskInsertStatementId);
        if (stmt != nullptr)
        {
            uint32_t col = 0;
            stmt->setString(col++, taskInfo.getTaskName());
            stmt->setUInt32(col++, taskInfo.getCompId());
            if (tdf != nullptr)
            {
                stmt->setUInt32(col++, taskInfo.getTdf()->getTdfId());
                stmt->setBinary(col++, raw.data(), raw.datasize());
            }
            else
            {
                stmt->setUInt32(col++, 0);
                stmt->setBinary(col++, nullptr, 0);
            }
            stmt->setUInt32(col++, taskInfo.getStart());
            stmt->setUInt32(col++, taskInfo.getDuration());
            stmt->setUInt32(col++, taskInfo.getRecurrence());

            DbResultPtr dbResult;
            BlazeRpcError error = conn->executePreparedStatement(stmt, dbResult);
            if (error == DB_ERR_OK)
            {
                if (dbResult->getLastInsertId() > INVALID_TASK_ID)
                {
                    taskInfo.setTaskId(dbResult->getLastInsertId());
                    return ERR_OK;
                }
            }
            else if (error == DB_ERR_DUP_ENTRY)
            {
                BLAZE_TRACE_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].saveTaskToDatabase: "
                    "filed because database row already exists for task(" << taskInfo.getTaskName() << ")" );
            }
            else
            {
                BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].saveTaskToDatabase: "
                    "Failed to save task(" << taskInfo.getTaskName() << ") information to database, error: " << (ErrorHelp::getErrorName(error)));
            }

            return error;
        }
    }
    else
    {
        BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].saveTaskToDatabase: "
            "Failed to save task(" << taskInfo.getTaskName() << ") information to database, no database connection.");
    }
    return ERR_SYSTEM;
}


/*! ***************************************************************/
/*! \brief remove the task to the database
        \param task id
*******************************************************************/
void TaskSchedulerSlaveImpl::removeTaskFromDatabase(TaskId taskId)
{
    DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
    if (conn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        query->append("DELETE FROM task_scheduler WHERE task_id = $d;", taskId);
        BlazeRpcError error = conn->executeQuery(query);
        if (error != DB_ERR_OK)
        {
            BLAZE_WARN_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].dbDropTask: "
                "Failed to delete task(" << taskId << ") from database, error: " << (ErrorHelp::getErrorName(error)) << ".");
        }
    }    
}

/*! ***************************************************************/
/*! \brief Signals the registered handlers that task has been scheduled
*******************************************************************/
void TaskSchedulerSlaveImpl::signalOnScheduledTaskById(TaskId taskId)
{
    TaskMap::const_iterator itr = mScheduledTasks.find(taskId);
    if (itr != mScheduledTasks.end())
    {
        signalOnScheduledTask(itr->second.getTaskInfo());
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].signalOnScheduledTaskById: Could not find task " << taskId << ".");
    }
}

/*! ***************************************************************/
/*! \brief Signals the registered handlers that task has been canceled
*******************************************************************/
void TaskSchedulerSlaveImpl::signalOnCanceledTaskById(TaskId taskId)
{
    TaskInfoMap::const_iterator itr = mCanceledTasks.find(taskId);
    if (itr != mCanceledTasks.end())
    {
        signalOnCanceledTask(itr->second);
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].signalOnCanceledTaskById: Could not find task " << taskId << ".");
    }
}

/*! ***************************************************************/
/*! \brief Signals the registered handlers that task has been scheduled
*******************************************************************/
void TaskSchedulerSlaveImpl::signalOnScheduledTask(const TaskInfo& taskInfo)
{
    HandlersByComponent::iterator it = mHandlers.find(taskInfo.getCompId());
    if(it != mHandlers.end())
    {
        it->second->onScheduledTask(taskInfo.getTdf(), taskInfo.getTaskId(), taskInfo.getTaskName());
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].signalOnScheduledTask: Could not find handler for task " << taskInfo.getTaskId() << ".");
    }

}

/*! ***************************************************************/
/*! \brief Signals the registered handlers that task has been canceled
*******************************************************************/
void TaskSchedulerSlaveImpl::signalOnCanceledTask(const TaskInfo& taskInfo)
{
    TaskId taskId = taskInfo.getTaskId();
    HandlersByComponent::iterator it = mHandlers.find(taskInfo.getCompId());
    if(it != mHandlers.end())
    {
        it->second->onCanceledTask(taskInfo.getTdf(), taskInfo.getTaskId(), taskInfo.getTaskName());
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].signalOnCanceledTask: Could not find handler for task " << taskId << ".");
    }

    TaskInfoMap::iterator taskItr = mCanceledTasks.find(taskId);
    if (taskItr != mCanceledTasks.end())
    {
        mCanceledTasks.erase(taskItr);
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].signalOnCanceledTask: Could not find info for task " << taskId << ".");
    }
}

/*! ***************************************************************/
/*! \brief Signals the registered handlers that task has been canceled
*******************************************************************/
void TaskSchedulerSlaveImpl::signalOnExecutedTask(const TaskInfo& taskInfo)
{
    HandlersByComponent::iterator it = mHandlers.find(taskInfo.getCompId());
    if(it != mHandlers.end())
    {
        it->second->onExecutedTask(taskInfo.getTdf(), taskInfo.getTaskId(), taskInfo.getTaskName());
    }
    else
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::taskscheduler, "[TaskSchedulerSlaveImpl].signalOnExecutedTask: Could not find handler for task " << taskInfo.getTaskId() << ".");
    }
}

} // Blaze
