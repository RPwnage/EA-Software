
/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class BlazeThread

    All Blaze threads derive from this class.  This class takes care of registering the thread
    with the thread-local system to enable access of thread-locals in GDB.  It also provides a
    common way to get a thread name.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/util/shared/blazestring.h"
#include "framework/system/threadlocal.h"
#include "framework/system/blazethread.h"
#ifdef EA_PLATFORM_LINUX
#include <sys/prctl.h> //for setting thread name
#include <syscall.h>
#endif

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

BlazeThread::BlazeThread(const char8_t* name, const ThreadType threadType, size_t stackSize)
    : mStackSize(stackSize),
      mThreadType(threadType),
      mThreadId(EA::Thread::kThreadIdInvalid)
#ifdef EA_PLATFORM_LINUX
      ,mPid(0)
#endif
{
#if ENABLE_CLANG_SANITIZERS 
    // Sanitizers can increase the stack size usage by 2-3x due to creation of "red zones" around read/store operations. As we don't really care for the RAM
    // usage during testing, we override the caller provided stack size with 8 MB which has been reliable in our testing so far (no false positives).
    mStackSize = 8192 * 1024;
#endif

    setName(name);
}

BlazeThread::~BlazeThread()
{
}

void BlazeThread::setName(const char8_t* name)
{
    blaze_strnzcpy(mName, name, sizeof(mName));
}

const char8_t* BlazeThread::getName() const
{
    return mName;
}

EA::Thread::ThreadId BlazeThread::start()
{
    EA::Thread::ThreadParameters params;
    params.mpName = mName;
    params.mnStackSize = mStackSize;
    mThreadId = mThread.Begin(this, nullptr, &params);
    return mThreadId;
}

void BlazeThread::waitForEnd()
{
    if (mThreadId != EA::Thread::kThreadIdInvalid)
        mThread.WaitForEnd();
}

/*** Private Methods *****************************************************************************/

intptr_t BlazeThread::Run(void* context)
{
    //This should go into EAThread
    #ifdef EA_PLATFORM_LINUX
    char8_t nameBuf[16]; //Limited to 16 bytes, null terminated
    strncpy(nameBuf, mName, sizeof(nameBuf)) ; //intentionally not blaze_strncpy for portability
    nameBuf[15] = '\0';
    int err = prctl(PR_SET_NAME, (unsigned long) nameBuf, 0, 0, 0);
    if (err != 0)
    {
        //Too early to really do anything - not sure if this can fail even
        fprintf(stderr, "Err: prctl to set thread name %s failed with err %i\n", nameBuf, errno);
    }
    mPid = syscall(SYS_gettid);
    #endif

    Fiber::FiberStorageInitializer fiberStorage;

    gCurrentThread = this;

    registerThread();    
    run();
    deregisterThread();

    return 0;
}

} // Blaze



