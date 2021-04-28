/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_BLAZETHREAD_H
#define BLAZE_BLAZETHREAD_H

/*** Include files *******************************************************************************/

#include "framework/blazedefines.h"
#include "framework/system/fiber.h"

#include "eathread/eathread_thread.h"
#include "eathread/eathread_storage.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class BlazeThread : private EA::Thread::IRunnable
{
    NON_COPYABLE(BlazeThread);

public:
    enum ThreadType 
    {
        UNKNOWN = 0,
        SERVER,
        LOGGER,
        DB_ADMIN,
        DB_WORKER,
        NAME_RESOLVER,
        SHELL_RUNNER,
        EXCEPTION_HANDLER,
        GRPC_CQ,
        EXLOGGER,
        PINLOG,
        OTHER
    };

    BlazeThread(const char8_t* name = nullptr, const ThreadType threadType = BlazeThread::UNKNOWN, size_t stackSize = 0);
    ~BlazeThread() override;

    void setName(const char8_t* name);
    const char8_t* getName() const;

    virtual EA::Thread::ThreadId start() WARN_UNUSED_RESULT;
    virtual void waitForEnd();
    virtual void stop() = 0;
    virtual void run() = 0;

private:
    char8_t mName[256];
    size_t mStackSize;
    EA::Thread::Thread mThread;
    ThreadType mThreadType;
    EA::Thread::ThreadId mThreadId;

    Logging::LogContext mLogContext;
#ifdef EA_PLATFORM_LINUX
    pid_t mPid;
#endif

    intptr_t Run(void* context) override;    
};

extern EA_THREAD_LOCAL BlazeThread* gCurrentThread;

} // Blaze

#endif // BLAZE_BLAZETHREAD_H

