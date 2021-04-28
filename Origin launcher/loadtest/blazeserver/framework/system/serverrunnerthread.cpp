/*************************************************************************************************/
/*!
    \file   serverrunnerthread.h.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ServerRunnerThread

    Extends RunnerThread to provide additional services specific to running a master or slave
    server:
      * create components
      * create a controller
      * create connections
      * create databases
      * create component locator

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/outboundconnectionmanager.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/connection/blockingsocket.h"
#include "framework/util/localization.h"
#include "framework/util/profanityfilter.h"
#include "framework/util/rawbufferpool.h"
#include "framework/util/random.h"
#include "framework/replication/replicator.h"
#include "framework/system/serverrunnerthread.h"
#include "framework/system/fibermanager.h"
#include "framework/system/shellexec.h"
#include "framework/system/allocator/allocatormanager.h"
#include "framework/usersessions/usersession.h"
#include "framework/component/censusdata.h"
#include "framework/database/dbscheduler.h"
#include "framework/metrics/outboundmetricsmanager.h"
#include "framework/metrics/metrics.h"
#include "framework/vault/vaultmanager.h"
#include "framework/connection/sslcontext.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


ServerRunnerThread::~ServerRunnerThread()
{
}

void ServerRunnerThread::stopInstance()
{
    mThreadLock.Lock();

    // If a Controller initiates the shutdown, it will already have called stopControllerInternal which in turn will
    // have stopped the ServerRunnerThread which in turn will have shutdown and deleted the selector as well as deleted
    // the controller, so make sure not to call stopControllerInternal on a runner thread that that's not running
    if (mRunning)
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[ServerRunnerThread].stopInstance: stop controller for running thread " << getName() << ".");
        stopControllerInternal();
    }
    else
    {
        // it is possible that we're shutting down even before the thread
        // had a chance to initialize, so in that case, signal it to 
        // stop so it doesn't startup the selector
        BLAZE_INFO_LOG(Log::SYSTEM, "[ServerRunnerThread].stopInstance: stopping thread " << getName() << ".");
        stopCommonInternal();
    }

    mThreadLock.Unlock();
}


void ServerRunnerThread::startConfigReload()
{
    mThreadLock.Lock();

    if (mRunning)
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[ServerRunnerThread].startConfigReload: scheduling configReload for running thread " << getName() << ".");
        mSelector->scheduleCall(mController, &Controller::reconfigureController);
    }

    mThreadLock.Unlock();
}


void ServerRunnerThread::setServiceState(bool inService)
{
    mThreadLock.Lock();

    if (mRunning)
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[ServerRunnerThread].setServiceState: scheduling setServiceState(" << inService << ") for running thread " << getName() << ".");
        mSelector->scheduleCall(mController, &Controller::setServiceState, inService);        
    }

    mThreadLock.Unlock();
}


void ServerRunnerThread::stop()
{
    BLAZE_INFO_LOG(Log::SYSTEM, "[ServerRunnerThread].stop: stopping running thread " << getName() << ".");

    mThreadLock.Lock();

    stopCommonInternal();

    mThreadLock.Unlock();

    BLAZE_INFO_LOG(Log::SYSTEM, "[ServerRunnerThread].stop: stopped running thread " << getName() << ".");
}


/*** Protected Methods ***************************************************************************/

ServerRunnerThread::ServerRunnerThread(uint16_t basePort, const char8_t* baseName, const char8_t* instanceName, const char8_t* hostName)
    : BlazeThread(instanceName,BlazeThread::SERVER, 1024 * 1024),
      mBasePort(basePort),
      mBaseName(baseName),
      mInstanceName(instanceName),
      mStarted(false),
      mRunning(false),
      mStopping(false),
      mCleanedUp(false),
      mSelector(nullptr),
      mNameResolver(nullptr),
      mShellRunner(nullptr),
      mController(nullptr),
      mDbScheduler(nullptr),
      mRedisManager(nullptr),
      mLocalization(nullptr),
      mProfanityFilter(nullptr),
      mReplicator(nullptr),
      mCensusDataManager(nullptr),
      mRandom(nullptr),
      mBlockingSocketManager(nullptr),
      mOutboundHttpService(nullptr),
      mOutboundMetricsManager(nullptr),
      mVaultManager(nullptr),
      mInstanceCollection(mMetricsCollection.getCollection(Metrics::Tag::service_name, gProcessController->getDefaultServiceName())
          .getCollection(Metrics::Tag::hostname, hostName)
          .getCollection(Metrics::Tag::instance_type, baseName)
          .getCollection(Metrics::Tag::instance_name, instanceName)),
      mSslContextManager(nullptr)
{
}


bool ServerRunnerThread::initialize()
{
    Metrics::gMetricsCollection = &mInstanceCollection;
    Metrics::gFrameworkCollection = &Metrics::gMetricsCollection->getTaglessCollection("framework");
    Logger::initializeThreadLocal();

    mSelector = Selector::createSelector();
    mNameResolver = BLAZE_NEW NameResolver(getName());
    mShellRunner = BLAZE_NEW ShellRunner(getName());
    mController = BLAZE_NEW Controller(getSelector(), mBaseName.c_str(), mInstanceName.c_str(), mBasePort);
    mDbScheduler = BLAZE_NEW_DB DbScheduler();
    mRedisManager = BLAZE_NEW RedisManager();
    mLocalization = BLAZE_NEW Localization();
    mProfanityFilter = BLAZE_NEW ProfanityFilter();
    mReplicator = BLAZE_NEW  Replicator(*mController);
    mCensusDataManager = BLAZE_NEW  CensusDataManager();
    mRandom = BLAZE_NEW Random();
    mBlockingSocketManager = BLAZE_NEW BlockingSocketManager();
    mOutboundHttpService = BLAZE_NEW OutboundHttpService();
    mOutboundMetricsManager = BLAZE_NEW OutboundMetricsManager();
    mVaultManager = BLAZE_NEW VaultManager();
    mSslContextManager = BLAZE_NEW SslContextManager();

    setThreadLocalVars();

    AllocatorManager::getInstance().initMetrics();

    if (!AllocatorManager::getInstance().isUsingPassthroughAllocator())
        gRawBufferPool = BLAZE_NEW RawBufferPool;

    if (!mSelector->initialize(gProcessController->getBootConfigTdf().getSelector()))
    {
        BLAZE_FATAL_LOG(Log::SYSTEM, "[ServerRunnerThread].initialize: Selector could not start.");
        gProcessController->shutdown(ProcessController::EXIT_FAIL);
        return false;
    }   

    //Start listening for events
    mSelector->open();

    //Kick off the DB threads
    mDbScheduler->start();

    //Kick off the name resolver threads
    if (mNameResolver->start() == EA::Thread::kThreadIdInvalid)
        return false;

    //Kick off the shell runner thread
    if (mShellRunner->start() == EA::Thread::kThreadIdInvalid)
        return false;

    // Mark this thread as started
    setStarted();

    // Start the controller to kick off the startup process
    if (!mController->startController())
    {
        BLAZE_FATAL_LOG(Log::SYSTEM, "[ServerRunnerThread].initialize: Controller could not start.");
        gProcessController->shutdown(ProcessController::EXIT_FAIL);
        return false;
    }

    return true;
}


void ServerRunnerThread::run()
{
    BLAZE_TRACE_LOG(Log::SYSTEM, "[ServerRunnerThread].Run: Initializing.");
    if (initialize() == false)
    {
        BLAZE_FATAL_LOG(Log::SYSTEM, "[ServerRunnerThread].Run: Failed initialization.");
        return;
    }

    //If the server was told to stop before we even initialized, it couldn't have told the selector to break.  In that
    //case we don't even bother starting the selector and just move straight to shutdown.
    if (!mStopping)
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "[ServerRunnerThread].Run: Starting main server loop.");

        mRunning = true;
        mSelector->run();

        BLAZE_INFO_LOG(Log::SYSTEM, "[ServerRunnerThread].Run: Stopping main server loop.");    
    }
    else
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "[ServerRunnerThread].Run: Server was shutdown before Selector::run was called.");    
    }

    cleanup();
}


/*** Private Methods *****************************************************************************/

// called by run()
void ServerRunnerThread::cleanup()
{
    if (mCleanedUp)
    {
        return;
    }
    mCleanedUp = true;

    //Stop the name resolver threads
    mNameResolver->stop();    

    //Stop the shell runner thread
    mShellRunner->stop();

    //Stop all the DB threads
    mDbScheduler->stop();

    //Free memory used in libcurl per instance 
    OutboundHttpConnectionManager::shutdownHttpForInstance();

    //Destroy all our sub managers.
    delete mCensusDataManager;
    delete mReplicator;
    delete mProfanityFilter;
    delete mLocalization;
    delete mDbScheduler;
    delete mRedisManager;
    delete mController;
    delete mShellRunner;
    delete mNameResolver;
    delete mSelector;
    delete mRandom;
    delete mBlockingSocketManager;
    delete mOutboundHttpService;
    delete mOutboundMetricsManager;
    delete mVaultManager;
    delete gRawBufferPool;
    delete mSslContextManager;
    Logger::destroyThreadLocal();

    clearThreadLocalVars();

    if (gProcessController != nullptr)
    {
        gProcessController->removeInstance(this);
    }
}


// called by stopInstance() and stop() only which are already thread safe
void ServerRunnerThread::stopCommonInternal()
{
    if (!mStopping)
    {
        mStopping = true;

        if (mRunning)
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "[ServerRunnerThread].stopCommonInternal(): stopping selector for thread " << getName() << ".");
            mSelector->shutdown();
        }

        mRunning = false;
    }
}


// called by stopInstance()
void ServerRunnerThread::stopControllerInternal()
{
    // Note we don't want to run this fiber with the group task id as we want to 
    // cancel everything internal to this group
    Fiber::CreateParams params;
    params.blocking = true;
    params.stackSize = Fiber::STACK_LARGE;
    params.namedContext = "Controller::executeShutdown";

    mSelector->getFiberManager().scheduleCall(mController, &Controller::executeShutdown, false, SHUTDOWN_NORMAL, params);
}


void ServerRunnerThread::setThreadLocalVars()
{
    //Set all our thread locals.
    gSelector = mSelector;
    gFiberManager = &mSelector->getFiberManager();
    gNameResolver = mNameResolver;
    gShellRunner = mShellRunner;
    gController = mController;
    gDbScheduler = mDbScheduler;
    gRedisManager = mRedisManager;
    gLocalization = mLocalization;
    gProfanityFilter = mProfanityFilter;
    gReplicator = mReplicator;
    gCensusDataManager = mCensusDataManager;
    gRandom = mRandom;
    gBlockingSocketManager = mBlockingSocketManager;
    gOutboundHttpService = mOutboundHttpService;
    gOutboundMetricsManager = mOutboundMetricsManager;
    gVaultManager = mVaultManager;
    gSslContextManager = mSslContextManager;
}


void ServerRunnerThread::clearThreadLocalVars() const
{
    gSelector = nullptr;
    gFiberManager = nullptr;
    gNameResolver = nullptr;
    gShellRunner = nullptr;
    gController = nullptr;
    gDbScheduler = nullptr;
    gRedisManager = nullptr;
    gLocalization = nullptr;
    gProfanityFilter = nullptr;
    gReplicator = nullptr;
    gCensusDataManager = nullptr;
    gRandom = nullptr;
    gBlockingSocketManager = nullptr;
    gOutboundHttpService = nullptr;
    gOutboundMetricsManager = nullptr;
    gVaultManager = nullptr;
    gSslContextManager = nullptr;
}

} // namespace Blaze

