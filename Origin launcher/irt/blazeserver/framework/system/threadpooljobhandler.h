/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_THREADPOOLJOBHANDLER_H
#define BLAZE_THREADPOOLJOBHANDLER_H

/*** Include files *******************************************************************************/

#include "framework/system/blazethread.h"
#include "framework/system/jobqueue.h"
#include "eathread/eathread_atomic.h"
#include "EASTL/vector.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Logger;

class ThreadPoolJobHandler
{
    NON_COPYABLE(ThreadPoolJobHandler)

public:
    ThreadPoolJobHandler(size_t numWorkers, const char8_t* poolName, BlazeThread::ThreadType threadType);
    virtual ~ThreadPoolJobHandler();

    virtual void start();
    virtual void stop();

    void addWorkers(size_t growBy);
    void removeWorkers(size_t reduceBy);
    void queueJob(Job& job);

    void reconfigure();

    uint32_t getNumWorkers() const;
    uint32_t getRunningJobCount() const { return mRunningJobCount; }
    uint32_t getPeakRunningJobCount() const { return mPeakRunningJobCount; }

private:
    void activateWorker();

    JobQueue mQueue;
    bool mStarted;
    size_t mInitialCount;
    size_t mExtraCount;
    EA::Thread::AtomicUint32 mRunningJobCount;
    uint32_t mPeakRunningJobCount;
    BlazeThread::ThreadType mWorkerThreadType;
    
    class Worker : public BlazeThread
    {
    public:
        typedef enum { STATE_NORMAL, STATE_RECONFIGURE } State;

        Worker(ThreadPoolJobHandler& owner, BlazeThread::ThreadType threadType, int32_t id);
        ~Worker() override;

        void setState(State state);

        void run() override;
        void stop() override;

    private:
        int32_t mId;
        ThreadPoolJobHandler& mOwner;
        State mState;
    };

    typedef eastl::vector<Worker*> ThreadVector;
    ThreadVector mWorkers;

    char8_t mPoolName[32];
    volatile bool mIsQuitting;

    mutable EA::Thread::Mutex mWorkerMutex;

    /*! ***************************************************************************/
    /*! \brief Function run on this object at the start of every thread.
    *******************************************************************************/
    virtual void threadStart() {}
    virtual void threadEnd() {}
    virtual void executeJob(Job& job);

    /*! ***************************************************************************/
    /*! \brief Function run on this object when the thread is to be reconfigured
    *******************************************************************************/
    virtual void threadReconfigure(int32_t workerId);
};

} // Blaze

#endif // BLAZE_THREADPOOLJOBHANDLER_H

