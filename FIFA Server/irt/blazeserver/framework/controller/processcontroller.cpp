#include "framework/blaze.h"
#include "framework/controller/processcontroller.h"
#include "framework/system/job.h"
#include "framework/system/serverrunnerthread.h"
#include "framework/config/config_boot_util.h"
#include "framework/connection/sslcontext.h"
#include "framework/connection/nameresolver.h"
#include "framework/connection/socketutil.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/util/random.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "EASTL/map.h"

//Includes for the exception whitelist functionality
#include "EACallstack/EACallstack.h"
#ifdef EA_PLATFORM_LINUX
#include <execinfo.h>
#endif 

namespace Blaze
{

typedef eastl::map<eastl::string, eastl::string> PredefMap;

///////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(EA_PLATFORM_LINUX)
//  Process-wide SIGSEGV handler
//      Invokes the fiber specific exception handler if possible.
//      Otherwise thunks down to the default OS behavior.
static siginfo_t* gExceptionSignalInfo = nullptr;
static void *gExceptionSignalContext = nullptr;
static  void SigSegVHandler(int signum, siginfo_t* sigInfo, void *context)
{
    if (signum == SIGSEGV || signum == SIGILL || signum == SIGFPE || signum == SIGTRAP)
    {
        if (gAllowThreadExceptions && !Fiber::isCurrentlyInMainFiber() && !Blaze::Fiber::isCurrentlyInException() && gProcessController->handleException())
        {      
            gExceptionSignalInfo = sigInfo;
            gExceptionSignalContext = context;

            //  switches context.
            Blaze::Fiber::exceptCurrentFiber();
        }
        else
        {
            //  an exception while handling an exception will result in termination with default exception handler.
            signal(signum, SIG_DFL);
        }
    }
}
#elif defined(EA_PLATFORM_WINDOWS)

static LONG WINAPI FiberExceptionHandler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{    
    //Here's some magic Windows voodoo.  This magic exception value, which doesn't have a constant, is
    //used by visual studio to name a thread.  Under the hood in EAThread this is being caught, so we don't want
    //to do it ourselves.  Hopefully this is the only type of exception.  Google 0x406D1388 for more info.
    if (ExceptionInfo != nullptr && ExceptionInfo->ExceptionRecord != nullptr &&
        ExceptionInfo->ExceptionRecord->ExceptionCode == 0x406D1388)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (gAllowThreadExceptions && !Fiber::isCurrentlyInMainFiber() && !Blaze::Fiber::isCurrentlyInException() && gProcessController->handleException())
    {   
        //Won't return from this.
        Blaze::Fiber::exceptCurrentFiber();
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////

ProcessController* gProcessController = nullptr;

eastl::string ProcessController::msShutdownCauseMessage;

ProcessController::ProcessController(const ConfigBootUtil& bootUtil, 
        const ServerVersion& version,
        const CmdParams& params,
        const CommandLineArgs& cmdLineArgs)
    : mControllerState(ProcessController::INITIAL),
      mThreadId(EA::Thread::kThreadIdInvalid),
      mBootConfigUtil(bootUtil),
      mCoreFileWriterThread("corewriter", 1024*1024),
      mQueue("ProcessController"),
      mVersion(version),
      mParams(params),
      mCommandLineArgs(cmdLineArgs),
      mExitCode(EXIT_NORMAL),
      mTotalExceptionsEncountered(0),
      mExceptionHandlerEnabled(false), 
      mReconfigurePending(false),
      mMonitorNoRestart(false),
      mExceptionWhitelist(BlazeStlAllocator("ProcessController::mExceptionWhitelist")),
      mNextInternalPort(mInternalPorts.end()),
      mNextExternalPort(mExternalPorts.end())
{
#if defined(EA_PLATFORM_WINDOWS) 
    mWinAccessExceptionHandle = nullptr;
#elif defined(EA_PLAFORM_LINUX)
    memset(&mOldSigSegvAction, 0, sizeof(mOldSigSegvAction));
    mOldSigSegvAction.sa_handler = SIG_DFL;
    memset(&mOldSigIllAction, 0, sizeof(mOldSigIllAction));
    mOldSigIllAction.sa_handler = SIG_DFL;
    memset(&mOldSigFpeAction, 0, sizeof(mOldSigFpeAction));
    mOldSigFpeAction.sa_handler = SIG_DFL;
#endif

    gProcessController = this;
}

ProcessController::~ProcessController()
{
    gProcessController = nullptr;
     
    OutboundHttpConnectionManager::shutdownHttpForProcess();
}

bool ProcessController::getConfigStringEscapes() const 
{
     return mBootConfigUtil.getConfigStringEscapes(); 
}

bool ProcessController::initializeAssignedPorts(const char8_t* portString, ProcessController::PortList& portList)
{
    const char8_t* pStr = portString;
    while (*pStr != '\0')
    {
        const char8_t* oldPStr = pStr;
        uint16_t portNum = 0;
        pStr = blaze_str2int(pStr, &portNum);
        if (portNum == 0 || pStr == oldPStr)
        {
            BLAZE_FATAL_LOG(Log::CONTROLLER, "[ProcessController].initializeAssignedPorts: Failed to parse assigned port list: '" << pStr << "' (full ports list: '" << portString << "')");
            portList.clear();
            return false;
        }
        portList.push_back(portNum);
        if (*pStr != '\0')
            ++pStr;
    }
    return true;
}

bool ProcessController::initialize()
{
    mControllerState = ProcessController::INITIALIZING;

    Random::initializeSeed();

    // Generate the UUID for this server instance
    UUID uuid;
    UUIDUtil::generateUUID(uuid);
    blaze_strnzcpy(mInstanceUuid, uuid.c_str(), sizeof(mInstanceUuid));

    mThreadId = EA::Thread::GetThreadId();

    if (!initializeAssignedPorts(mParams.internalPorts.c_str(), mInternalPorts) || !initializeAssignedPorts(mParams.externalPorts.c_str(), mExternalPorts))
        return false;

    mNextInternalPort = mInternalPorts.begin();
    mNextExternalPort = mExternalPorts.begin();

    if (!Logger::configure(getBootConfigTdf().getLogging(), false))
        return false;

    SocketUtil::initialize(getBootConfigTdf());

    BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].initialize: Initialization sequence start.");
    BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].initialize: Server starting with boot config '" << mBootConfigUtil.getBootConfigFilename() << "'");

    // Calculate the internal and external addresses to be used for this process
    InetAddress internalAddress;
    InetAddress externalAddress;

    if (getBootConfigTdf().getInternalIpOverride()[0] == '\0')
    {
        internalAddress.setHostname(getBootConfigTdf().getInterfaces().getInternal());
        NameResolver::blockingResolve(internalAddress);
        internalAddress.setIp(SocketUtil::getInterface(&internalAddress), InetAddress::NET);
    }
    else
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].initialize: Overriding internal ip to '" << getBootConfigTdf().getInternalIpOverride() << "'.");
        internalAddress.setHostname(getBootConfigTdf().getInternalIpOverride());
        if (internalAddress.isResolved() == false)
        {
            BLAZE_FATAL_LOG(Log::CONTROLLER, "[ProcessController].initialize: Internal ip address override " << getBootConfigTdf().getInternalIpOverride() << " is not resolved");
            return false;
        }
    }

    if (getBootConfigTdf().getExternalIpOverride()[0] == '\0')
    {
        externalAddress.setHostname(getBootConfigTdf().getInterfaces().getExternal());
        NameResolver::blockingResolve(externalAddress);
        externalAddress.setIp(SocketUtil::getInterface(&externalAddress), InetAddress::NET);
    }
    else
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].initialize: Overriding external ip to '" << getBootConfigTdf().getExternalIpOverride() << "'.");
        externalAddress.setHostname(getBootConfigTdf().getExternalIpOverride());
        if (externalAddress.isResolved() == false)
        {
            BLAZE_FATAL_LOG(Log::CONTROLLER, "[ProcessController].initialize: External ip address override " << getBootConfigTdf().getExternalIpOverride() << " is not resolved");
            return false;
        }
    }

    bool internalHostnameOverride = false;
    bool externalHostnameOverride = false;

    if (getBootConfigTdf().getHostnameOverride()[0] == '\0')
    {
        // Lookup hostname for external ip address.  The hostname, if found, gets sent to the rdir along with the
        // ServerAddress data.  This in turn is sent down to the BlazeSDK client in the getServerInstance() response.
        if (!NameResolver::blockingLookup(externalAddress))
            BLAZE_WARN_LOG(Log::CONTROLLER, "[ProcessController].initialize: Could not find a FQDN for the external address (" << externalAddress << ")");
    }
    else
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].initialize: Overriding external hostname to '" << getBootConfigTdf().getHostnameOverride() << "'.");
        externalAddress.setHostname(getBootConfigTdf().getHostnameOverride());
        externalHostnameOverride = true;
    }

    if (getBootConfigTdf().getInternalHostnameOverride()[0] == '\0')
    {
        // Lookup hostname for internal ip address.  The hostname, if found, gets sent to the rdir along with the
        // ServerAddress data.  This in turn is sent down to the BlazeSDK client in the getServerInstance() response.
        if (!NameResolver::blockingLookup(internalAddress))
            BLAZE_WARN_LOG(Log::CONTROLLER, "[ProcessController].initialize: Could not find a FQDN for the internal address (" << internalAddress << ")");
    }
    else
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].initialize: Overriding internal hostname to '" << getBootConfigTdf().getInternalHostnameOverride() << "'.");
        internalAddress.setHostname(getBootConfigTdf().getInternalHostnameOverride());
        internalHostnameOverride = true;
    }

    NameResolver::setInterfaceAddresses(internalAddress, externalAddress, internalHostnameOverride, externalHostnameOverride);

    gSslContextManager->initializeSsl();

    //Initialize Curl
    OutboundHttpConnectionManager::initHttpForProcess();

    if (mParams.isBootSingleInstance())
    {
        ServerRunnerThread* runner = 
            BLAZE_NEW ServerRunnerThread(mParams.instanceBasePort, mParams.instanceType, mParams.instanceName, internalAddress.getHostname());
        mThreadList.push_back(runner);
    }
    else
    {
        // Initialize all thread instances
        for (ServerInstanceConfigList::const_iterator
            it = mBootConfigUtil.getConfigTdf().getServerInstances().begin(),
            end = mBootConfigUtil.getConfigTdf().getServerInstances().end();
            it != end; ++it)
        {
            const ServerInstanceConfig& config = **it;
            if (config.getStart())
            {
                ServerRunnerThread* runner = 
                    BLAZE_NEW ServerRunnerThread(config.getPort(), config.getType(), config.getName(), internalAddress.getHostname());
                mThreadList.push_back(runner);
            }
        }    
    }
    
    if (mThreadList.empty())
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[ProcessController].initialize: No thread instances were configured for this process.");
        return false;
    }

    //  initialize core writer thread.
    if (mCoreFileWriterThread.start() == EA::Thread::kThreadIdInvalid)
        return false;

    mControllerState = ProcessController::INITIALIZED;

    return true;
    
}

void ProcessController::removeInstance(ServerRunnerThread* instance)
{
    mQueue.push(BLAZE_NEW MethodCall1Job<ProcessController,ServerRunnerThread*>(this, &ProcessController::internalRemoveInstance, instance));
}

void ProcessController::internalRemoveInstance(ServerRunnerThread* instance)
{
    for (ThreadList::reverse_iterator it = mRunningInstanceList.rbegin(), eit = mRunningInstanceList.rend(); it != eit; ++it)
    {
        if (*it == instance)
        {  
            BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].internalRemoveInstance: instance thread " << (*it)->getName()  << "(" << (*it) << ") has stopped." );
            mRunningInstanceList.erase(it);
            break;
        }
    }   

    // when the last instance is removed, we can finally join all threads and transition to shutdown
    if (mRunningInstanceList.empty())
    {        
        for (ThreadList::reverse_iterator it = mThreadList.rbegin(), eit = mThreadList.rend(); it != eit; ++it)
        {
            (*it)->waitForEnd();
            BLAZE_INFO_LOG(Log::CONTROLLER,"[ProcessController].internalRemoveInstance: instance thread " << (*it)->getName() << "(" << (*it) << ") has exited.");
            delete *it;
        }  

        mThreadList.clear();

        if (mExceptionHandlerEnabled)
        {
            disableExceptionHandler();
            mExceptionHandlerEnabled = false;
        }

        // no more instance is running so transition to shutdown
        mControllerState = ProcessController::SHUTDOWN; 
        BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].internalRemoveInstance: has no more running instances. Transitioned to ProcessController::SHUTDOWN." );
    }
}

void ProcessController::start()
{
    mControllerState = ProcessController::STARTING;
    
    mQueue.push(BLAZE_NEW MethodCallJob<ProcessController>(this, &ProcessController::internalStart));
    while (mControllerState < ProcessController::SHUTDOWN)
    { 
        Job* job = mQueue.popOne(-1);
        if (job != nullptr)
        {
            job->execute();
            delete job;
        }
    }
}

void ProcessController::logShutdownCause()
{
    if (!msShutdownCauseMessage.empty())
    {
        BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].logShutdownCause:\n --> " << msShutdownCauseMessage.c_str());
    }
}

void ProcessController::shutdown(ExitCode exitCode)
{
    eastl::string message;

    if (exitCode == EXIT_FAIL)
    {
        // NOTE: We only save the logging message in case we are exiting due to a failure, since
        // other types of shutdown causes are often described by a sequence of events that 
        // may not be captured in a single log message.
        StringBuilder& builder = Logger::getCurrentLoggingMessage();
        message += builder.get();
    }

    BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].shutdown("<< exitCode <<")" );

    mQueue.push(BLAZE_NEW MethodCall2Job<ProcessController, ExitCode, eastl::string>(this, &ProcessController::internalShutdown, exitCode, message));
}

void ProcessController::internalStart()
{
    if (mControllerState != ProcessController::STARTING)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[ProcessController].internalStart: unexpected state: " << mControllerState);
        mControllerState = ProcessController::SHUTDOWN;  //to get out of the while loop in start()
        // if not cleaned up, do so
        for (ThreadList::iterator i = mThreadList.begin(), e = mThreadList.end(); i != e; ++i)
        {
            // cleanup() itself below will not delete the thread in this case since we already get out of the while loop in start(). We have to manually delete the thread.
            (*i)->cleanup();
            delete *i;
        }
        mThreadList.clear();
        gSslContextManager->cleanupSsl();
        return;
    }
        
    for (ThreadList::iterator i = mThreadList.begin(), e = mThreadList.end(); i != e; ++i)
    {
        EA::Thread::ThreadId threadId = (*i)->start();
        if (threadId == EA::Thread::kThreadIdInvalid)
        {
            // stop here and delete/remove all this and the rest of the threads from the list
            for (auto& it = i; it != e; ++it)
                delete *it;

            mThreadList.erase(i,e);
            // shutdown because we weren't able to startup all threads
            internalShutdown(EXIT_FAIL);
            return;
        }

        BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].internalStart: instance " << (*i)->getName() << " has started running." );
        mRunningInstanceList.push_back(*i);
    }
    
    mControllerState = ProcessController::STARTED;
}

void ProcessController::internalShutdown(ExitCode exitCode, eastl::string message)
{    
    if (msShutdownCauseMessage.empty())
    {
        // only set the message on the first shutdown request
        msShutdownCauseMessage = message;
    }

    //  surface the first non-normal exit code only when shutting down the server.
    if (mExitCode == EXIT_NORMAL)
    {
        mExitCode = exitCode;
    }

    if (mExitCode == EXIT_NORMAL || mExitCode == EXIT_FAIL)
    {
        // EXIT_FAIL is when there's a bootstrap error. EXIT_NORMAL is for shutdown without restart.  Either case they want to shutdown the server without a restart, so signal the monitor.
        // This is essential since a child process when crashed does not return an exit code.
        signalMonitorNoRestart();
    }

    if (mControllerState >= ProcessController::SHUTTING)
    {
        // either already shutting down, or shutdown
        BLAZE_TRACE_LOG(Log::CONTROLLER, "[ProcessController].internalShutdown: ignoring duplicate request.");
        return;
    }

    // Take server out of service as we're shutting down
    BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].internalShutdown: set service state to out-of-service");
    setServiceState(false);

    mControllerState = ProcessController::SHUTTING;
    
    BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].internalShutdown: shutting down all thread instances.");
    
    // Tell all threads to shut themselves down (in reverse order of startup)
    for (ThreadList::reverse_iterator i = mThreadList.rbegin(); i != mThreadList.rend(); ++i)
    {
        (*i)->stopInstance();
    }
}

void ProcessController::reconfigure()
{
    // Prevent multiple reconfig events from occurring when they shouldn't 
    if (!mReconfigurePending)
    {
        mReconfigurePending = true;

        // Force request to run on the correct thread
        mQueue.push(BLAZE_NEW MethodCallJob<ProcessController>(this, &ProcessController::internalReconfigure));
    }
    else
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[ProcessController].reconfigure: Reconfigure called while reconfigure is already pending. Check for excessive calls to SIGHUP or CTRL_BREAK_EVENT.");
    }
}

void ProcessController::internalReconfigure()
{
    // when it comes to signaling reconfigure one thread is
    // as good as the next, since they all RPC configMaster
    // to kick off the reconfiguration process.
    ThreadList::iterator it = mThreadList.begin();
    if (it != mThreadList.end())
    {
        (*it)->startConfigReload();
    }
    else
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[ProcessController].internalReconfigure: No threads to reconfigure.");

        // since we didn't start a reconfig on this event, make sure we're able to queue up a fresh reconfig event
        mReconfigurePending = false;
    }
}

void ProcessController::setServiceState(bool inService)
{
    // Force request to run on the correct thread
    mQueue.push(BLAZE_NEW MethodCall1Job<ProcessController,bool>(this, &ProcessController::internalSetServiceState, inService));
    return;
}

void ProcessController::internalSetServiceState(bool inService)
{
    // Tell all threads to adjust their state
    ThreadList::iterator it = mThreadList.begin();
    while (it != mThreadList.end())
    {
        (*it)->setServiceState(inService);        
        ++it;
    }
}

bool ProcessController::isShuttingDown() const
{
    return (mControllerState == ProcessController::SHUTTING);
}

void ProcessController::setRestartAfterShutdown(bool restartAfterShutdown)
{
    if ((mExitCode == EXIT_NORMAL) && restartAfterShutdown)
    {
        mExitCode = EXIT_RESTART;
    }
}

const BootConfig& ProcessController::getBootConfigTdf() const
{
    return mBootConfigUtil.getConfigTdf();
}


//  called on configuration from FrameworkConfig::ExceptionConfig
void ProcessController::configureExceptionHandler(const ExceptionConfig& config)
{
    mQueue.push(BLAZE_NEW MethodCall1Job<ProcessController, const ExceptionConfig& >
        (this, &ProcessController::internalConfigureExceptionHandler, config));
}


void ProcessController::internalConfigureExceptionHandler(const ExceptionConfig& config)
{
    config.copyInto(mExceptionConfig);

    if (mExceptionHandlerEnabled != mExceptionConfig.getExceptionHandlingEnabled())
    {
        //there was a change in the enabled property. Toggle the exception handler
        if (mExceptionHandlerEnabled)
        {
            disableExceptionHandler();
        }
        else
        {
            enableExceptionHandler();
        }   
    }

    mExceptionHandlerEnabled = mExceptionConfig.getExceptionHandlingEnabled();

    if (mExceptionHandlerEnabled)
    {
        mExceptionWhitelist.clear();
        ExceptionConfig::Uint64_tList::const_iterator itr = mExceptionConfig.getExceptionCallstackWhitelist().begin();
        ExceptionConfig::Uint64_tList::const_iterator end = mExceptionConfig.getExceptionCallstackWhitelist().end();
        for (; itr != end; ++itr)
        {
            mExceptionWhitelist[*itr] = 0;
        }
    }
}

void ProcessController::enableExceptionHandler()
{
#if defined(EA_PLATFORM_LINUX)
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = Blaze::SigSegVHandler;
    if (sigaction(SIGSEGV, &sa, &mOldSigSegvAction) != 0)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[ProcessController].initialize: Failed to redirect SIGSEGV signals to custom exception handler.");
        return;
    }
    if (sigaction(SIGILL, &sa, &mOldSigIllAction) != 0)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[ProcessController].initialize: Failed to redirect SIGILL signals to custom exception handler.");
        return;
    }
    if (sigaction(SIGFPE, &sa, &mOldSigFpeAction) != 0)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[ProcessController].initialize: Failed to redirect SIGFPE signals to custom exception handler.");
        return;
    }
    if (sigaction(SIGTRAP, &sa, &mOldSigFpeAction) != 0)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[ProcessController].initialize: Failed to redirect SIGTRAP signals to custom exception handler.");
        return;
    }

#elif defined(EA_PLATFORM_WINDOWS)
    mWinAccessExceptionHandle = AddVectoredExceptionHandler(1, &FiberExceptionHandler);    
#endif
    BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].enableExceptionHandler : fiber exception handling ON");
}

void ProcessController::disableExceptionHandler()
{
    //  cleanup exception handler
    if (mExceptionConfig.getExceptionHandlingEnabled())
    {      
    #if defined(EA_PLATFORM_LINUX)
        sigaction(SIGSEGV, &mOldSigSegvAction, nullptr);
        sigaction(SIGILL, &mOldSigIllAction, nullptr);
        sigaction(SIGFPE, &mOldSigFpeAction, nullptr);
    #elif defined(EA_PLATFORM_WINDOWS)    
        if (mWinAccessExceptionHandle != nullptr)
        {
            RemoveVectoredExceptionHandler(mWinAccessExceptionHandle);
            mWinAccessExceptionHandle = nullptr;
        } 
    #endif
        BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].disableExceptionHandler: fiber exception handling OFF");
    }    

}

bool ProcessController::handleException()
{
    // mark the fiber as being in an exception, to prevent any exceptions in this method from triggering new calls to handleException()
    // before we're finished here
    Fiber::setIsInException();

    // make exception call stack hash
    StringBuilder sb;
    void* callData[16];
#ifdef EA_PLATFORM_LINUX
    size_t stackSize = (size_t) backtrace(callData, EAArrayCount(callData));
#else
    size_t stackSize = (size_t) EA::Callstack::GetCallstack(callData, EAArrayCount(callData), nullptr);
#endif
    for (size_t counter = 0; counter < stackSize; ++counter)
    {
        sb.append(" 0x%" PRIx64 " ", (intptr_t) callData[counter]);
    }
    eastl::hash<const char8_t*> h;
    uint64_t stackHash = h(sb.get());

    // check if exception white listed
    CallstackHashToHitCountMap::iterator itr = mExceptionWhitelist.find(stackHash);
    if (itr != mExceptionWhitelist.end())
    {
        ++(itr->second);
        BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].handleException: exception filtered out by white list, with call stack hash: " << stackHash << ". Total occurrences of the filtered exception, including this: " << itr->second);
        return true; // Avoid core dump and count towards thresholds below
    }
    BLAZE_INFO_LOG(Log::CONTROLLER, "[ProcessController].handleException: exception encountered. Call stack hash: " << stackHash << ". Call Context: " << sb.get());

    ++mTotalExceptionsEncountered;
    if (mTotalExceptionsEncountered > mExceptionConfig.getTotalFiberExceptionsAllowed())
    {
        Fiber::unsetIsInException();
        return false;   // Exception Limit reached - Blaze will not handle this exception 
    }

    TimeValue curTime = TimeValue::getTimeOfDay();
    if ((curTime - mLastExceptionTime) < mExceptionConfig.getDelayPerFiberException())
    {
        Fiber::unsetIsInException();
        return false;   // Exception occurred during the delay interval - Blaze will not handle this exception.
    }
    mLastExceptionTime = curTime;
    
    if (mExceptionConfig.getCoreDumpsEnabled())
    {
        mCoreFileWriterThread.coreDump(mExceptionConfig.getCoreFilePrefix(), "exception", mExceptionConfig.getCoreFileSize(),gCurrentThread, true /*alwaysSignalMonitor*/);
    }

    return true;
}

BlazeRpcError ProcessController::coreDump(const char8_t* source, uint64_t coreSizeLimit)
{
    return mCoreFileWriterThread.coreDump(mExceptionConfig.getCoreFilePrefix(), source, coreSizeLimit, gCurrentThread);
}


void ProcessController::signalMonitorNoRestart()
{
#if defined(EA_PLATFORM_LINUX)
    if (gProcessController->getMonitorPid() != 0 && !mMonitorNoRestart)
    {
        mMonitorNoRestart = true;
        kill(gProcessController->getMonitorPid(), SIGUSR2);
    }
#endif
}

// Note: The correct way to get the default service name is by calling gController->getDefaultServiceName().
// This method is only called from the ServerRunnerThread constructor, before gController is available.
const char8_t* ProcessController::getDefaultServiceName() const
{
    PlatformToServiceNameMap::const_iterator iter = getBootConfigTdf().getPlatformToDefaultServiceNameMap().find(getBootConfigTdf().getConfiguredPlatform());
    if (iter != getBootConfigTdf().getPlatformToDefaultServiceNameMap().end())
        return iter->second.c_str();

    BLAZE_ERR_LOG(Log::CONTROLLER, "[ProcessController::getDefaultServiceName] configured platform (" << ClientPlatformTypeToString(getBootConfigTdf().getConfiguredPlatform())
        << ") does not have a default service associated with it.");
    
    return "";
}

bool ProcessController::getAssignedPort(BindType bindType, uint16_t& port)
{
    EA::Thread::AutoMutex mutex(mPortAssignmentMutex);
    switch (bindType)
    {
    case BIND_ALL:
    case BIND_EXTERNAL:
    {
        if (mNextExternalPort == mExternalPorts.end())
            return false;
        port = *mNextExternalPort;
        ++mNextExternalPort;
    }
    break;
    case BIND_INTERNAL:
    {
        if (mNextInternalPort == mInternalPorts.end())
            return false;
        port = *mNextInternalPort;
        ++mNextInternalPort;
    }
    break;
    default:
        return false;
    }

    return true;
}

} // Blaze

