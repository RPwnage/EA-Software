/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PROCESSCONTROLLER_H
#define BLAZE_PROCESSCONTROLLER_H

/*** Include files *******************************************************************************/
#ifdef EA_PLATFORM_LINUX
#include <signal.h>
#endif

#include "eathread/eathread_storage.h"
#include "eathread/eathread_mutex.h"
#include "eathread/eathread_atomic.h"

#include "framework/util/uuid.h"
#include "framework/system/jobqueue.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/system/corefilewriter.h"

#include "EASTL/list.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

class ConfigBootUtil;
class BlazeThread;
class ServerRunnerThread;
class ServerVersion;
class BootConfig;
class InstanceConfig;

class ProcessController
{
    NON_COPYABLE(ProcessController);

public:
    enum ExitCode
    {
        EXIT_NORMAL = 0,                 // exiting due to normal termination for server shutdown (SIGTERM, SIGINT)
        EXIT_FAIL = 1,                   // exiting due to invalid an invalid server command line or another fatal server error prior to component startup
        EXIT_RESTART = 3,                // exiting due to server restart (shutdown/drain RPC with the RESTART/REBOOT flag set to true).
        EXIT_FATAL = -1,                 // exiting due to a fatal, explicit termination of the server.  this does *not* include exceptions.
        EXIT_MASTER_TERM = 10,           // exiting because of losing connection to the master
        EXIT_INSTANCEID_REFRESH = 11,    // exiting because of failure to refresh the InstanceId
        EXIT_INSTANCEINFO_REFRESH = 12,  // exiting because of failure to refresh the InstanceInfo
    };
    
    struct CmdParams
    {
        CmdParams()
        {
            instanceType[0] = '\0';
            instanceName[0] = '\0';
            instanceBasePort = 0;
            monitorPid = 0;
            allowDestruct = false;
            startInService = true;
            processId = 0;
        }
        bool isBootSingleInstance() const { return instanceName[0] != '\0'; }
        bool isUsingAssignedPorts() const { return !internalPorts.empty() || !externalPorts.empty(); }
        char8_t instanceType[32];
        char8_t instanceName[32];
        uint16_t instanceBasePort;
        uint32_t monitorPid;
        bool allowDestruct;
        bool startInService; // True means that instance will go in-service if bootstrap sequence is successful (member is useful if we want to forcefully not let a instance go in-service)
        uint32_t processId;
        eastl::string internalPorts;
        eastl::string externalPorts;
    };
    
    ProcessController(const ConfigBootUtil& bootUtil, 
        const ServerVersion& version, const CmdParams& params, const CommandLineArgs& cmdLineArgs);
    ~ProcessController();

    bool initialize();
    void start();
    void shutdown(ExitCode exitCode);
    void reconfigure();
    void clearReconfigurePending() { mReconfigurePending = false; }

    // Adjust the service state for all instance running in this process.  This is done because
    // taking an instance out of service is used to drain users from it so it can be shut down.
    // There isn't any point in taking only a single slave out of service if there are other
    // slaves running in the same process.  By forcing all instances in the process out of
    // service it should eliminate user error cases where a slave is taken down with one or
    // more instances in the process still servicing clients.
    void setServiceState(bool inService);

    const BootConfig& getBootConfigTdf() const;
    const char8_t* getDefaultServiceName() const;
    const ConfigBootUtil& getBootConfigUtil() const { return mBootConfigUtil; }    

    bool isSharedCluster() const { return getBootConfigTdf().getHostedPlatforms().size() > 1 || getBootConfigTdf().getConfiguredPlatform() == common; }

    bool isUsingAssignedPorts() const { return mParams.isUsingAssignedPorts(); }
    bool allowDestructiveDbActions() const { return mParams.allowDestruct; }
    bool getStartInService() const { return mParams.startInService; }
    const ServerVersion& getVersion() const { return mVersion; }
    bool isShuttingDown() const;
    bool getConfigStringEscapes() const;

    bool getAssignedPort(BindType bindType, uint16_t& port);
    
    //  called on configuration from FrameworkConfig::ExceptionConfig
    void configureExceptionHandler(const ExceptionConfig& config);
    //  if this returns false, Blaze could not handle the exception - callers should treat this as fatal and abort the process.
    bool handleException();
    //  call to dump a core file for this process.
    BlazeRpcError coreDump(const char8_t* source, uint64_t coreSizeLimit);

    //  Returns the exit status code for the process.  This value should either be passed to exit() or returned from the main() function.
    ExitCode getExitCode() const { return mExitCode; }
    uint32_t getMonitorPid() const { return mParams.monitorPid; }
    bool isCoreDumpEnabled() const { return mExceptionConfig.getCoreDumpsEnabled(); }
    uint32_t getProcessId() const { return mParams.processId; }

    void setRestartAfterShutdown(bool restartAfterShutdown);

    const CommandLineArgs& getCommandLineArgs() const { return mCommandLineArgs; }

    void removeInstance(ServerRunnerThread* instance);  

    void signalMonitorNoRestart();
    static void logShutdownCause();

private:
    typedef eastl::list<ServerRunnerThread*> ThreadList;
    typedef eastl::list<uint16_t> PortList;

    enum ControllerState
    {
        INITIAL,
        INITIALIZING,
        INITIALIZED,
        STARTING,
        STARTED,
        SHUTTING,
        SHUTDOWN,
        MAX
    };

    EA::Thread::AtomicInt32 mControllerState;
    EA::Thread::ThreadId mThreadId;    
    const ConfigBootUtil& mBootConfigUtil;    
    ThreadList mThreadList;
    CoreFileWriter mCoreFileWriterThread;
    JobQueue mQueue;

    const ServerVersion& mVersion;
    const CmdParams& mParams;
    const CommandLineArgs& mCommandLineArgs;
    bool mAllowDestruct;

    void internalRemoveInstance(ServerRunnerThread* instance);
    
    void internalStart();
    void internalShutdown(ExitCode exitCode, eastl::string message = "");
    void internalReconfigure();
    void internalSetServiceState(bool inService);
    
    void internalConfigureExceptionHandler(const ExceptionConfig& config);
    void enableExceptionHandler();
    void disableExceptionHandler();

    static bool initializeAssignedPorts(const char8_t* portString, PortList& portList);

    ExitCode mExitCode;
    uint32_t mTotalExceptionsEncountered;
#if defined(EA_PLATFORM_WINDOWS)
    PVOID mWinAccessExceptionHandle;
#elif defined(EA_PLATFORM_LINUX)
    struct sigaction mOldSigSegvAction;
    struct sigaction mOldSigIllAction;
    struct sigaction mOldSigFpeAction;
#endif
    EA::TDF::TimeValue mLastExceptionTime;
    
    ExceptionConfig mExceptionConfig;
    bool mExceptionHandlerEnabled;
    uint32_t mMonitorPid;
    char8_t mInstanceUuid[MAX_UUID_LEN];

    bool mReconfigurePending;
    bool mMonitorNoRestart;
    static eastl::string msShutdownCauseMessage;

    typedef eastl::hash_map<uint64_t, uint64_t> CallstackHashToHitCountMap;
    CallstackHashToHitCountMap mExceptionWhitelist;

    ThreadList mRunningInstanceList;
    EA::Thread::Mutex mPortAssignmentMutex;
    PortList mInternalPorts;
    PortList mExternalPorts;
    PortList::const_iterator mNextInternalPort;
    PortList::const_iterator mNextExternalPort;
};

extern EA_THREAD_LOCAL bool gAllowThreadExceptions;
extern ProcessController* gProcessController;

} // Blaze

#endif // BLAZE_PROCESSCONTROLLER_H

