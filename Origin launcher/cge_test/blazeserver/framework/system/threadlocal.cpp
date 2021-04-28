/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    This file maintains all of the thread-local global variables used by the Blaze framework.
    There is currently a bug in GDB which prevents thread-local variables from being printing
    which makes it very difficult to debug a blaze executable.  By tracking pointers to all
    the thread-locals in a global map, it is then possible to locate the correct pointers in
    GDB by calling the getThreadLocals() function passing the thread ID.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/system/threadlocal.h"
#include "framework/controller/processcontroller.h"
#include "framework/system/fiber.h"
#include "framework/system/fibermanager.h"
#include "framework/system/shellexec.h"
#include "framework/connection/selector.h"
#include "framework/connection/blockingsocket.h"
#include "framework/controller/controller.h"
#include "framework/connection/nameresolver.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/metrics/metrics.h"
#include "framework/dynamicinetfilter/dynamicinetfilter.h"
#include "framework/system/serverrunnerthread.h"
#include "framework/identity/identity.h"
#include "framework/slivers/slivermanager.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/usersessions/usersessionsmasterimpl.h"
#include "framework/userset/userset.h"
#include "framework/logger.h"
#include "framework/petitionablecontent/petitionablecontent.h"
#include "framework/util/profanityfilter.h"
#include "framework/util/localization.h"
#include "framework/database/dbscheduler.h"
#include "framework/component/censusdata.h"
#include "framework/event/eventmanager.h"
#include "framework/uniqueid/uniqueidmanager.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/util/random.h"
#include "framework/vault/vaultmanager.h"
#include "framework/connection/sslcontext.h"

#ifdef EA_PLATFORM_LINUX
#include <syscall.h>
#endif

class PerThreadMemTracker;
// imported from allocation.cpp
extern void initPerThreadMemTracker();
extern void shutdownPerThreadMemTracker();

namespace Blaze
{

class Replicator;

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

EA_THREAD_LOCAL FiberManager* gFiberManager = nullptr;
EA_THREAD_LOCAL bool gAllowThreadExceptions = true;

EA_THREAD_LOCAL Selector* gSelector = nullptr;
EA_THREAD_LOCAL Controller* gController = nullptr;
EA_THREAD_LOCAL DbScheduler* gDbScheduler = nullptr;
EA_THREAD_LOCAL RedisManager* gRedisManager = nullptr;
EA_THREAD_LOCAL NameResolver* gNameResolver = nullptr;
EA_THREAD_LOCAL ShellRunner* gShellRunner = nullptr;
EA_THREAD_LOCAL BlazeThread* gCurrentThread = nullptr;
EA_THREAD_LOCAL IdentityManager* gIdentityManager = nullptr;
EA_THREAD_LOCAL SliverManager* gSliverManager = nullptr;
EA_THREAD_LOCAL UserSessionManager* gUserSessionManager = nullptr;
EA_THREAD_LOCAL UserSessionsMasterImpl* gUserSessionMaster = nullptr;
EA_THREAD_LOCAL UserSetManager* gUserSetManager = nullptr;
EA_THREAD_LOCAL CensusDataManager* gCensusDataManager = nullptr;
EA_THREAD_LOCAL PetitionableContentManager* gPetitionableContentManager = nullptr;
EA_THREAD_LOCAL Replicator* gReplicator = nullptr;
EA_THREAD_LOCAL Event::EventManager* gEventManager = nullptr;
EA_THREAD_LOCAL DynamicInetFilter::Filter* gDynamicInetFilter = nullptr;
EA_THREAD_LOCAL UniqueIdManager* gUniqueIdManager = nullptr;
EA_THREAD_LOCAL MemoryGroupId gCurrentNonCompliantMemGroupOverride = MEM_GROUP_NONCOMPLIANCE_DEFAULT;
EA_THREAD_LOCAL const char8_t* gCurrentNonCompliantNameOverride = BLAZE_UNNAMED_ALLOC_NAME;
EA_THREAD_LOCAL RawBufferPool* gRawBufferPool = nullptr;
EA_THREAD_LOCAL ProfanityFilter* gProfanityFilter = nullptr;
EA_THREAD_LOCAL Localization* gLocalization = nullptr;
EA_THREAD_LOCAL Random* gRandom = nullptr;
EA_THREAD_LOCAL BlockingSocketManager* gBlockingSocketManager = nullptr;
EA_THREAD_LOCAL OutboundHttpService* gOutboundHttpService = nullptr;
EA_THREAD_LOCAL PerThreadMemTracker* gPerThreadMemTracker = nullptr;
EA_THREAD_LOCAL bool gIsMainThread = false;
EA_THREAD_LOCAL bool gIsDbThread = false;
EA_THREAD_LOCAL uint64_t gDbThreadTimeout = 0;
EA_THREAD_LOCAL OutboundMetricsManager* gOutboundMetricsManager= nullptr;
EA_THREAD_LOCAL VaultManager* gVaultManager = nullptr;
EA_THREAD_LOCAL Metrics::MetricsCollection* Metrics::gMetricsCollection = nullptr;
EA_THREAD_LOCAL Metrics::MetricsCollection* Metrics::gFrameworkCollection = nullptr;
EA_THREAD_LOCAL SslContextManager *gSslContextManager = nullptr;

struct ThreadLocalInfo
{
    ThreadLocalInfo() {}

    void initialize()
    {
        mThreadId = EA::Thread::GetThreadId();
#ifdef EA_PLATFORM_LINUX
        mPid = syscall(SYS_gettid);
#endif


        mAllowThreadExceptions = &gAllowThreadExceptions;
        mSelector = &gSelector;
        mDbScheduler = &gDbScheduler;
        mRedisManager = &gRedisManager;
        mController = &gController;
        mNameResolver = &gNameResolver;
        mShellRunner = &gShellRunner;
        mCurrentThread = &gCurrentThread;
        mIdentityManager = &gIdentityManager;
        mSliverManager = &gSliverManager;
        mUserSessionManager = &gUserSessionManager;
        mUserSessionMaster = &gUserSessionMaster;
        mUserSetManager = &gUserSetManager;
        mCensusDataManager = &gCensusDataManager;
        mPetitionableContentManager = &gPetitionableContentManager;
        mRawBufferPool = &gRawBufferPool;
        mProfanityFilter = &gProfanityFilter;
        mLocalization = &gLocalization;
        mRandom = &gRandom;
        mBlockingSocketManager = &gBlockingSocketManager;
        mOutboundHttpService = &gOutboundHttpService;
        mMetricsCollection = &Metrics::gMetricsCollection;
        mPerThreadMemTracker = &gPerThreadMemTracker;
        mIsMainThread = &gIsMainThread;
        mIsDbThread = &gIsDbThread;
        mDbThreadTimeout = &gDbThreadTimeout;
        mReplicator = &gReplicator;
        mEventManager = &gEventManager;
        mDynamicInetFilter = &gDynamicInetFilter;
        mUniqueIdManager = &gUniqueIdManager;
        mCurrentNonCompliantMemGroupOverride = &gCurrentNonCompliantMemGroupOverride;
        mCurrentNonCompliantNameOverride = &gCurrentNonCompliantNameOverride;

        mFiberManager = &gFiberManager;
        mMainFiber = &Fiber::msMainFiber;
        mCurrentFiber = &Fiber::msCurrentFiber;
        mNextFiberId = &Fiber::msNextFiberId;
        mFiberIdMap = &Fiber::msFiberIdMap;
        mNextFiberGroupId = &Fiber::msNextFiberGroupId;
        mFiberGroupIdMap = &Fiber::msFiberGroupIdMap;
        mFiberGroupJoinMap = &Fiber::msFiberGroupJoinMap;
        mNextFiberEventId = &Fiber::msNextEventId;
        mFiberEventMap = &Fiber::msEventMap;
        mFiberStack = &Fiber::msFiberStack;

        mOutboundMetricsManager = &gOutboundMetricsManager;
        mVaultManager = &gVaultManager;

        mSslContextManager = &gSslContextManager;
    }

    EA::Thread::ThreadId mThreadId;
#ifdef EA_PLATFORM_LINUX
    pid_t mPid;
#endif

    //
    // Define pointers to all the TLS variables
    //

    bool* mAllowThreadExceptions;

    Selector** mSelector;
    DbScheduler** mDbScheduler;
    RedisManager** mRedisManager;
    Controller** mController;
    NameResolver** mNameResolver;
    ShellRunner** mShellRunner;
    BlazeThread** mCurrentThread;
    IdentityManager** mIdentityManager;
    SliverManager** mSliverManager;
    UserSessionManager** mUserSessionManager;
    UserSessionsMasterImpl** mUserSessionMaster;
    UserSetManager** mUserSetManager;
    CensusDataManager** mCensusDataManager;
    PetitionableContentManager** mPetitionableContentManager;
    Replicator** mReplicator;
    Event::EventManager** mEventManager;
    DynamicInetFilter::Filter** mDynamicInetFilter;
    UniqueIdManager** mUniqueIdManager;

    uint32_t *mCurrentLoggingContext;
    RawBufferPool** mRawBufferPool;
    ProfanityFilter** mProfanityFilter;
    Localization** mLocalization;
    Random** mRandom;
    BlockingSocketManager** mBlockingSocketManager;
    OutboundHttpService** mOutboundHttpService;
    Metrics::MetricsCollection** mMetricsCollection;
    PerThreadMemTracker** mPerThreadMemTracker;

    bool* mIsMainThread;
    bool* mIsDbThread;
    uint64_t *mDbThreadTimeout;

    MemoryGroupId* mCurrentNonCompliantMemGroupOverride;
    const char8_t** mCurrentNonCompliantNameOverride;

    FiberManager** mFiberManager;
    Fiber** mMainFiber;
    Fiber** mCurrentFiber;
    Fiber::FiberId* mNextFiberId;
    Fiber::FiberIdMap** mFiberIdMap;
    Fiber::FiberGroupId* mNextFiberGroupId;
    Fiber::FiberGroupIdListMap** mFiberGroupIdMap;
    Fiber::FiberGroupJoinMap** mFiberGroupJoinMap;
    Fiber::EventId* mNextFiberEventId;
    Fiber::EventMap** mFiberEventMap;
    Fiber::FiberStack** mFiberStack;

    OutboundMetricsManager** mOutboundMetricsManager;
    VaultManager** mVaultManager;
    SslContextManager** mSslContextManager;
};

static ThreadLocalInfo* sThreadLocalInfo = nullptr;
static size_t sThreadLocalInfoCount = 0;
static EA::Thread::Mutex sThreadMapMutex;

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
     \brief registerThread
 
      Register the TLS variables for the calling thread such that they are available for
      printing via printThreadLocals().
*/
/*************************************************************************************************/
void registerThread()
{
    initPerThreadMemTracker();
    sThreadMapMutex.Lock();
    ThreadLocalInfo* tli = BLAZE_NEW_ARRAY(ThreadLocalInfo, sThreadLocalInfoCount + 1);
    if (sThreadLocalInfoCount > 0)
    {
        memcpy(tli, sThreadLocalInfo, sizeof(ThreadLocalInfo) * sThreadLocalInfoCount);
        delete [] sThreadLocalInfo;
    }
    sThreadLocalInfo = tli;
    sThreadLocalInfo[sThreadLocalInfoCount].initialize();
    ++sThreadLocalInfoCount;

    sThreadMapMutex.Unlock();
}

/*************************************************************************************************/
/*!
     \brief deregisterThread
 
      Deregister the TLS variables for the calling thread such that they are available for
      printing via printThreadLocals().
*/
/*************************************************************************************************/
void deregisterThread()
{
    EA::Thread::ThreadId threadId = EA::Thread::GetThreadId();

    sThreadMapMutex.Lock();
    for(size_t idx = 0; idx < sThreadLocalInfoCount; ++idx)
    {
        if (sThreadLocalInfo[idx].mThreadId == threadId)
        {
            if (idx < (sThreadLocalInfoCount - 1))
            {
                memcpy(&sThreadLocalInfo[idx], &sThreadLocalInfo[sThreadLocalInfoCount - 1],
                        sizeof(sThreadLocalInfo[0]));
            }
            sThreadLocalInfoCount--;
            break;
        }
    }
    sThreadMapMutex.Unlock();
    shutdownPerThreadMemTracker();
}

void freeThreadLocalInfo()
{
    delete [] sThreadLocalInfo;
}

/*************************************************************************************************/
/*!
     \brief printThreadLocals
 
      Print out all of the TLS variables for the given thread.
  
     \param[in] threadId - Thread ID of the thread to print TLS variables for.
 
*/
/*************************************************************************************************/
void printThreadLocals(EA::Thread::ThreadId threadId)
{
    size_t idx;
    for(idx = 0; idx < sThreadLocalInfoCount; ++idx)
    {
        if (sThreadLocalInfo[idx].mThreadId == threadId)
            break;

    }
    eastl::string buf;
    if (idx == sThreadLocalInfoCount)
    {
        buf.append("Thread not found.\n");
        printf("%s",buf.c_str());
#if defined EA_PLATFORM_WINDOWS
        OutputDebugString(buf.c_str());
#endif
        return;
    }

    ThreadLocalInfo* info = &sThreadLocalInfo[idx];

    buf.append_sprintf("gAllowThreadExceptions=%s\n", *info->mAllowThreadExceptions ? "true" : "false");
    buf.append_sprintf("gSelector=%p\n", *info->mSelector);
    buf.append_sprintf("gDbScheduler=%p\n", *info->mDbScheduler);
    buf.append_sprintf("gRedisManager=%p\n", *info->mRedisManager);
    buf.append_sprintf("gController=%p\n", *info->mController);
    buf.append_sprintf("gNameResolver=%p\n", *info->mNameResolver);
    buf.append_sprintf("gShellRunner=%p\n", *info->mShellRunner);
    buf.append_sprintf("gCurrentThread=%p\n", *info->mCurrentThread);
    buf.append_sprintf("gIdentityManager=%p\n", *info->mIdentityManager);
    buf.append_sprintf("gSliverManager=%p\n", *info->mSliverManager);
    buf.append_sprintf("gUserSessionManager=%p\n", *info->mUserSessionManager);
    buf.append_sprintf("gUserSessionMaster=%p\n", *info->mUserSessionMaster);
    buf.append_sprintf("gUserSetManager=%p\n", *info->mUserSetManager);
    buf.append_sprintf("gCensusDataManager=%p\n", *info->mCensusDataManager);
    buf.append_sprintf("gPetitionableContentManager=%p\n", *info->mPetitionableContentManager);
    buf.append_sprintf("gReplicator=%p\n", *info->mReplicator);
    buf.append_sprintf("gEventManager=%p\n", *info->mEventManager);
    buf.append_sprintf("gDynamicInetFilter=%p\n", *info->mDynamicInetFilter);
    buf.append_sprintf("gUniqueIdManager=%p\n", *info->mUniqueIdManager);
    buf.append_sprintf("gProfanityFilter=%p\n", *info->mProfanityFilter);
    buf.append_sprintf("gLocalization=%p\n", *info->mLocalization);
    buf.append_sprintf("gOutboundHttpService=%p\n", info->mOutboundHttpService);

    buf.append_sprintf("gIsMainThread=%s\n", *info->mIsMainThread ? "true" : "false");
    buf.append_sprintf("gIsDbThread=%s\n", *info->mIsDbThread ? "true" : "false");
    buf.append_sprintf("gDbThreadTimeout=%" PRId64 "\n", *info->mDbThreadTimeout);

    buf.append_sprintf("gFiberManager=%p\n", *info->mFiberManager);
    buf.append_sprintf("mMainFiber=%p\n", *info->mMainFiber);
    buf.append_sprintf("mCurrentFiber=%p\n", *info->mCurrentFiber);
    buf.append_sprintf("Fiber::msNextFiberId=%" PRId64 "\n", *info->mNextFiberId);
    buf.append_sprintf("Fiber::mFiberIdMap=%p\n", *info->mFiberIdMap);    
    buf.append_sprintf("Fiber::msNextFiberGroupId=%" PRId64 "\n", *info->mNextFiberGroupId);
    buf.append_sprintf("Fiber::msFiberGroupIdMap=%p\n", *info->mFiberGroupIdMap);    
    buf.append_sprintf("Fiber::msFiberGroupJoinMap=%p\n", *info->mFiberGroupJoinMap);    
    buf.append_sprintf("Fiber::msNextFiberEventId=%" PRId64 "\n", *info->mNextFiberEventId);
    buf.append_sprintf("Fiber::msFiberEventMap=%p\n", *info->mFiberEventMap);    
    buf.append_sprintf("Fiber::msFiberStack=%p\n", *info->mFiberStack);    

    buf.append_sprintf("gOutboundMetricsManager=%p\n", info->mOutboundMetricsManager);
    buf.append_sprintf("gVaultManager=%p\n", *info->mVaultManager);
    
    printf("%s", buf.c_str());
#if defined EA_PLATFORM_WINDOWS
    OutputDebugString(buf.c_str());
#endif
}

} // Blaze

