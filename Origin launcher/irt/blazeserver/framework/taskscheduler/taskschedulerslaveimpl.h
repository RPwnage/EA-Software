/*************************************************************************************************/
/*!
    \file   taskschedulermasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_TASKSCHEDULER_SLAVEIMPL_H
#define BLAZE_TASKSCHEDULER_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "framework/rpc/taskschedulerslave_stub.h"
#include "framework/tdf/taskschedulertypes_server.h"
#include "EASTL/hash_map.h"

#include "framework/database/dbconn.h"
#include "framework/callback.h"
#include "framework/redis/rediskeymap.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define INVALID_TASK_ID 0

namespace Blaze
{

/*! ***************************************************************/
/*! \name Forward declarations.
 *******************************************************************/
class DbConn;
class TaskEventHandler;

typedef eastl::hash_map<TaskId, const EA::TDF::Tdf*> TaskTdfByIdMap;

/*! ***************************************************************/
/*! \name ScheduledTaskInfo
    \brief A wrapper class to handle all information required
           for scheduling a task.
*******************************************************************/
class ScheduledTaskInfo
{
public:
    ScheduledTaskInfo();
    ~ScheduledTaskInfo();

    void setTimerId(TimerId timerId) { mTimerId = timerId; }

    TimerId getTimerId() const { return mTimerId; }
    uint32_t getRecurrence() const { return mTaskInfo.getRecurrence(); }
    uint32_t getStart() const { return mTaskInfo.getStart(); }
    uint32_t getDuration() const { return mTaskInfo.getDuration(); }
    TaskStatus getStatus() const { return mTaskInfo.getStatus(); }

friend class TaskSchedulerSlaveImpl;
protected:
    TaskInfo& getTaskInfo() { return mTaskInfo; }
    const TaskInfo& getTaskInfo() const { return mTaskInfo; }

private:
    TimerId  mTimerId;
    TaskInfo mTaskInfo;
};


/*! ***************************************************************/
/*! \name TaskSchedulerSlaveImpl
    \brief Master component implementation.
*******************************************************************/
class TaskSchedulerSlaveImpl : public TaskSchedulerSlaveStub
{
    NON_COPYABLE(TaskSchedulerSlaveImpl);
public:
    typedef Functor1<BlazeRpcError&> RunOneTimeTaskCb;
    typedef eastl::hash_map<TaskId, ScheduledTaskInfo> TaskMap;
    typedef eastl::hash_map<TaskId, TaskInfo> TaskInfoMap;
    static const uint32_t SCHEMA_VERSION = 1;
    TaskSchedulerSlaveImpl();
    ~TaskSchedulerSlaveImpl() override;

    uint16_t getDbSchemaVersion() const override { return SCHEMA_VERSION; }
    const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const override { return &getConfig().getDbNamesByPlatform(); }

    /*! ***************************************************************/
    /*! \name registration methods
        \brief Components must register/deregister themselves.  
    *******************************************************************/
    bool registerHandler(ComponentId compId, TaskEventHandler* handler);
    bool deregisterHandler(ComponentId compId);

    /*! ***************************************************************/
    /*! \name Task scheduling methods
        \brief Use these methods to handle task scheduling/canceling
    *******************************************************************/
    void createTask(const char8_t* taskName, ComponentId compId, EA::TDF::Tdf* tdf, uint32_t start = 0, 
        uint32_t duration = 0, uint32_t recurrence = 0);
    TaskId createTaskAndGetTaskId(const char8_t* taskName, ComponentId compId, EA::TDF::Tdf* tdf, uint32_t start = 0, 
        uint32_t duration = 0, uint32_t recurrence = 0);
    BlazeRpcError runOneTimeTaskAndBlock(const char8_t* taskName, ComponentId compId, const RunOneTimeTaskCb& cb);
    bool cancelTask(TaskId taskId);

     /*! ***************************************************************/
     /*! \name getComponentTaskByIdMap
           \brief Get all the tasks for a particular component
    *******************************************************************/
    bool getComponentTaskByIdMap(const ComponentId compId, TaskTdfByIdMap& taskIdMap);

    const ScheduledTaskInfo* getTask(TaskId taskId) const;
    static int64_t secToUsec(uint32_t);

private:
    friend class TaskEventHandler;
    typedef eastl::hash_map<ComponentId, TaskEventHandler*> HandlersByComponent;

    TaskMap mScheduledTasks;
    TaskInfoMap mCanceledTasks;
    HandlersByComponent mHandlers;
    PreparedStatementId mTaskInsertStatementId;
    Blaze::RedisKeyMap<eastl::string, eastl::string> mTaskIdLockMap;

     /*! ***************************************************************/
     /*! \name configuration methods
           \brief methods to handle configuration and reconfiguration
    *******************************************************************/
    bool onValidateConfig(TaskSchedulerConfig& config, const TaskSchedulerConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    bool onConfigure() override;

     /*! ***************************************************************/
     /*! \name notification methods
           \brief methods to handle incoming notifications
    *******************************************************************/
    void onTaskScheduled(const Blaze::TaskInfo& data,UserSession *associatedUserSession) override;
    void onTaskCanceled(const Blaze::TaskInfo& data,UserSession *associatedUserSession) override;

     /*! ***************************************************************/
     /*! \name database methods
           \brief methods to save/load tasks to/from database
    *******************************************************************/    
    void saveOneTimeTaskToDatabase(const char8_t* taskName, ComponentId compId, EA::TDF::Tdf* tdf, BlazeRpcError* error, const RunOneTimeTaskCb* const cb);
    void saveScheduledTaskToDatabase(const char8_t* taskName, ComponentId compId, EA::TDF::Tdf* tdf, uint32_t start, uint32_t duration, uint32_t recurrence);
    BlazeRpcError saveTaskToDatabase(const char8_t* taskName, ComponentId compId, EA::TDF::Tdf* tdf, uint32_t start, uint32_t duration, uint32_t recurrence, ScheduledTaskInfo& schedTaskInfo);
    void removeTaskFromDatabase(TaskId taskId);
    bool loadTasksFromDatabase();
    void updateOneTimeTask(TaskId taskId, EA::TDF::TimeValue scheduledTime);

    void signalOnCanceledTaskById(TaskId taskId);
    void signalOnScheduledTaskById(TaskId taskId);

    void signalOnScheduledTask(const TaskInfo& taskInfo);
    void signalOnCanceledTask(const TaskInfo& taskInfo);
    void signalOnExecutedTask(const TaskInfo& taskInfo);
    void onTaskExpiration(TaskId taskId);
    bool getLockFromRedis(TaskId taskId);
    void releaseLockFromRedis(TaskId taskId);
    BlazeRpcError onRunOneTimeTask(ScheduledTaskInfo& taskInfo, const RunOneTimeTaskCb& cb);
};

/*! ***************************************************************/
/*! \name Utility methods
*******************************************************************/
inline int64_t TaskSchedulerSlaveImpl::secToUsec(uint32_t sec)
{
    return 1000000LL*sec;
}


/*! ***************************************************************/
/*! \name TaskEventHandler
    \brief The task event handler is an interface to be used by calling components
           to have a callback for when a task has occurred. 
*******************************************************************/
class TaskEventHandler 
{ 
public:
    virtual ~TaskEventHandler() {}

private:
    friend class TaskSchedulerSlaveImpl;
   /*! ***************************************************************/
   /*! \name onScheduledTask
       \brief This method will be called on the component
           when an event has occurred. The EA::TDF::Tdf* is owned
           by the task scheduler, the calling component will 
           need to copy the data provided. 
    *******************************************************************/
    virtual void onScheduledTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName) = 0; 


    /*! ***************************************************************/
   /*! \name onExecutedTask
       \brief This method will be called on the component
           when an event has occurred. The EA::TDF::Tdf* is owned
           by the task scheduler, the calling component will 
           execute the task. 
    *******************************************************************/
    virtual void onExecutedTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName) = 0; 

    /*! ***************************************************************/
   /*! \name onCanceledTask
       \brief This method will be called on the component
           when an event has occurred. The EA::TDF::Tdf* is owned
           by the task scheduler, the calling component will 
           cancel the task. 
    *******************************************************************/
    virtual void onCanceledTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName) = 0; 
};

} // Blaze

#endif  // BLAZE_TASKSCHEDULER_SLAVEIMPL_H
