/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*
 * This class should be used whenever worker threads are needed.  It provides a safe mechanism to make
 * calls from the main slave/master Blaze thread to a worker thread.  It automatically handles
 * getting function arguments to the worker thread for execution and result objects back.  It also
 * handles command execution timeout in a safe way.
 *
 * This class necessarily dictates the calling convention for worker thread calls.  The conventions are:
 *     1. Calls must return a BlazeRpcError value.
 *     2. Calls can return at most one result object.  This class is responsible for allocated this object
 *        and will automatically ensure it is freed in cases where an error is encountered when executing
 *        the call.  Named, if the thread function execution returns anything other than ERR_OK, the result
 *        object will be freed.
 *     3. Calls can take from 1 to 6 parameters.
 *     4. Call parameters must be pointers to objects or primitives.
 *     5. Call parameters must be allocated on the heap.
 *     6. The function call takes ownership of the call parameters.  Once the function returns, the
 *        call parameters will be deleted.
 *
 * While these rules provide some restriction on use, they also provide a safe calling interface which is
 * far more important as historically this has been an area where implementation is often done wrong which
 * results in server crashes.
 *
 * Anyone wanting to use a worker thread in Blaze should inherit from this class and then provide
 * a public API to make worker thread calls.  An example implementation might look like:
 *
 * class TestWorkerThread : public WorkerThread
 * {
 * public:
 *     TestWorkerThread(const char8_t* name) : WorkerThread(name) { }
 *
 *     // Define the public API
 *     BlazeRpcError myTestFunc(Result*& result, Foo* p1, Bar* p2)
 *     {
 *         return scheduleCall<TestWorkerThread, Result, Foo, Bar>(
 *             this, &TestWorkerThread::doMyTestFunc, result, p1, p2);
 *     }
 *
 * private:
 *     BlazeRpcError doMyTestFunc(Result* result, Foo* p1, Bar* p2)
 *     {
 *         // Implement code which runs on the thread
 *         // Store results in "result" parameter which is already allocated
 *
 *         return ERR_OK;
 *     }
 * };
 */

#ifndef BLAZE_WORKERTHREAD_H
#define BLAZE_WORKERTHREAD_H

/*** Include files *******************************************************************************/

#include "framework/system/jobqueue.h"
#include "framework/system/blazethread.h"
#include "eathread/eathread_mutex.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class WorkerThread : public BlazeThread, public eastl::intrusive_list_node
{
    NON_COPYABLE(WorkerThread);

public:
    ~WorkerThread() override;

    EA::Thread::ThreadId start() override;
    void stop() override;

protected:
    WorkerThread(const char8_t* name, const ThreadType threadType, size_t stackSize = DEFAULT_STACK_SIZE);

    template <class Caller, class Result, class P1>
        BlazeRpcError scheduleCall(Caller* caller,
                BlazeRpcError (Caller::*mf)(Result*, const P1*),
                Result*& result,
                const P1*& arg1);

    template <class Caller, class Result, class P1, class P2>
        BlazeRpcError scheduleCall(Caller* caller,
                BlazeRpcError (Caller::*mf)(Result*, const P1*, const P2*),
                Result*& result,
                const P1*& arg1, const P2*& arg2);

    template <class Caller, class Result, class P1, class P2, class P3>
        BlazeRpcError scheduleCall(Caller* caller,
                BlazeRpcError (Caller::*mf)(Result*, const P1*, const P2*, const P3*),
                Result*& result,
                const P1*& arg1, const P2*& arg2, const P3*& arg3);

    template <class Caller, class Result, class P1, class P2, class P3, class P4>
        BlazeRpcError scheduleCall(Caller* caller,
                BlazeRpcError (Caller::*mf)(Result*, const P1*, const P2*, const P3*, const P4*),
                Result*& result,
                const P1*& arg1, const P2*& arg2, const P3*& arg3, const P4*& arg4);

    template <class Caller, class Result, class P1, class P2, class P3, class P4, class P5>
        BlazeRpcError scheduleCall(Caller* caller,
                BlazeRpcError (Caller::*mf)(Result*, const P1*, const P2*, const P3*, const P4*, const P5*),
                Result*& result,
                const P1*& arg1, const P2*& arg2, const P3*& arg3, const P4*& arg4, const P5*& arg5);

    template <class Caller, class Result, class P1, class P2, class P3, class P4, class P5, class P6>
        BlazeRpcError scheduleCall(Caller* caller,
                BlazeRpcError (Caller::*mf)(Result*, const P1*, const P2*, const P3*, const P4*, const P5*, const P6*),
                Result*& result,
                const P1*& arg1, const P2*& arg2, const P3*& arg3, const P4*& arg4, const P5*& arg5, const P6*& arg6);

private:
    class WorkerThreadJobBase : public Job
    {
        friend class WorkerThread;

    public:
        WorkerThreadJobBase()
            : mLogContext(nullptr),
              mReturnValue(ERR_OK)
        { }

        void initialize(const Fiber::EventHandle& eventHandle, const LogContextWrapper& logContext, EA::TDF::TimeValue timeout)
        {
            mEventHandle = eventHandle;
            mLogContext = &logContext;
            mTimeout = timeout;
            mPriority = Fiber::getCurrentPriority();
        }

        const Fiber::EventHandle& getEventHandle() const { return mEventHandle; }
        const LogContextWrapper& getLogContext() const { return *mLogContext; }
        BlazeRpcError getReturnValue() const { return mReturnValue; }

    protected:
        Fiber::EventHandle mEventHandle;
        const LogContextWrapper* mLogContext;
        EA::TDF::TimeValue mTimeout;
        BlazeRpcError mReturnValue;

        NON_COPYABLE(WorkerThreadJobBase);
    };

    template <class Result>
    class WorkerThreadJobResultBase : public WorkerThreadJobBase
    {
    public:
        WorkerThreadJobResultBase()
            : mResult(BLAZE_NEW Result)
        {
        }

        ~WorkerThreadJobResultBase() override
        {
            delete mResult;
        }

        Result* takeResult() { Result* res = mResult; mResult = nullptr; return res; }

    protected:
        Result* mResult;

        NON_COPYABLE(WorkerThreadJobResultBase);
    };

    template <class Caller, class Result, class P1>
    class WorkerThreadJob1 : public WorkerThreadJobResultBase<Result>
    {
    public:
        typedef BlazeRpcError (Caller::*MemberFunc)(Result*, const P1*);

        WorkerThreadJob1(Caller* caller, MemberFunc mf, const P1* arg1)
            : mCaller(caller),
              mFunc(mf),
              mArg1(arg1)
        {
        }

        virtual ~WorkerThreadJob1()
        {
            delete mArg1;
        }

        virtual void execute() { this->WorkerThreadJobBase::mReturnValue = (mCaller->*mFunc)(this->WorkerThreadJobResultBase<Result>::mResult, mArg1); }

    private:
        Caller* mCaller;
        MemberFunc mFunc;

        const P1* mArg1;

        NON_COPYABLE(WorkerThreadJob1);
    };

    template <class Caller, class Result, class P1, class P2>
    class WorkerThreadJob2 : public WorkerThreadJobResultBase<Result>
    {
    public:
        typedef BlazeRpcError (Caller::*MemberFunc)(Result*, const P1*, const P2*);

        WorkerThreadJob2(Caller* caller, MemberFunc mf, const P1* arg1, const P2* arg2)
            : mCaller(caller),
              mFunc(mf),
              mArg1(arg1),
              mArg2(arg2)
        {
        }

        virtual ~WorkerThreadJob2()
        {
            delete mArg1;
            delete mArg2;
        }

        virtual void execute() { this->WorkerThreadJobBase::mReturnValue = (mCaller->*mFunc)(this->WorkerThreadJobResultBase<Result>::mResult, mArg1, mArg2); }

    private:
        Caller* mCaller;
        MemberFunc mFunc;

        const P1* mArg1;
        const P2* mArg2;

        NON_COPYABLE(WorkerThreadJob2);
    };

    template <class Caller, class Result, class P1, class P2, class P3>
    class WorkerThreadJob3 : public WorkerThreadJobResultBase<Result>
    {
    public:
        typedef BlazeRpcError (Caller::*MemberFunc)(Result*, const P1*, const P2*, const P3*);

        WorkerThreadJob3(Caller* caller, MemberFunc mf, const P1* arg1, const P2* arg2, const P3* arg3)
            : mCaller(caller),
              mFunc(mf),
              mArg1(arg1),
              mArg2(arg2),
              mArg3(arg3)
        {
        }

        virtual ~WorkerThreadJob3()
        {
            delete mArg1;
            delete mArg2;
            delete mArg3;
        }

        virtual void execute() { this->WorkerThreadJobBase::mReturnValue = (mCaller->*mFunc)(this->WorkerThreadJobResultBase<Result>::mResult, mArg1, mArg2, mArg3); }

    private:
        Caller* mCaller;
        MemberFunc mFunc;

        const P1* mArg1;
        const P2* mArg2;
        const P3* mArg3;

        NON_COPYABLE(WorkerThreadJob3);
    };

    template <class Caller, class Result, class P1, class P2, class P3, class P4>
    class WorkerThreadJob4 : public WorkerThreadJobResultBase<Result>
    {
    public:
        typedef BlazeRpcError (Caller::*MemberFunc)(Result*, const P1*, const P2*, const P3*, const P4*);

        WorkerThreadJob4(Caller* caller, MemberFunc mf, const P1* arg1, const P2* arg2, const P3* arg3, const P4* arg4)
            : mCaller(caller),
              mFunc(mf),
              mArg1(arg1),
              mArg2(arg2),
              mArg3(arg3),
              mArg4(arg4)
        {
        }

        virtual ~WorkerThreadJob4()
        {
            delete mArg1;
            delete mArg2;
            delete mArg3;
            delete mArg4;
        }

        virtual void execute() { this->WorkerThreadJobBase::mReturnValue = (mCaller->*mFunc)(this->WorkerThreadJobResultBase<Result>::mResult, mArg1, mArg2, mArg3, mArg4); }

    private:
        Caller* mCaller;
        MemberFunc mFunc;

        const P1* mArg1;
        const P2* mArg2;
        const P3* mArg3;
        const P4* mArg4;

        NON_COPYABLE(WorkerThreadJob4);
    };

    template <class Caller, class Result, class P1, class P2, class P3, class P4, class P5>
    class WorkerThreadJob5 : public WorkerThreadJobResultBase<Result>
    {
    public:
        typedef BlazeRpcError (Caller::*MemberFunc)(Result*, const P1*, const P2*, const P3*, const P4*, const P5*);

        WorkerThreadJob5(Caller* caller, MemberFunc mf, const P1* arg1, const P2* arg2, const P3* arg3, const P4* arg4, const P5* arg5)
            : mCaller(caller),
              mFunc(mf),
              mArg1(arg1),
              mArg2(arg2),
              mArg3(arg3),
              mArg4(arg4),
              mArg5(arg5)
        {
        }

        virtual ~WorkerThreadJob5()
        {
            delete mArg1;
            delete mArg2;
            delete mArg3;
            delete mArg4;
            delete mArg5;
        }

        virtual void execute() { this->WorkerThreadJobBase::mReturnValue = (mCaller->*mFunc)(this->WorkerThreadJobResultBase<Result>::mResult, mArg1, mArg2, mArg3, mArg4, mArg5); }

    private:
        Caller* mCaller;
        MemberFunc mFunc;

        const P1* mArg1;
        const P2* mArg2;
        const P3* mArg3;
        const P4* mArg4;
        const P5* mArg5;

        NON_COPYABLE(WorkerThreadJob5);
    };

    template <class Caller, class Result, class P1, class P2, class P3, class P4, class P5, class P6>
    class WorkerThreadJob6 : public WorkerThreadJobResultBase<Result>
    {
    public:
        typedef BlazeRpcError (Caller::*MemberFunc)(Result*, const P1*, const P2*, const P3*, const P4*, const P5*, const P6*);

        WorkerThreadJob6(Caller* caller, MemberFunc mf, const P1* arg1, const P2* arg2, const P3* arg3, const P4* arg4, const P5* arg5, const P6* arg6)
            : mCaller(caller),
              mFunc(mf),
              mArg1(arg1),
              mArg2(arg2),
              mArg3(arg3),
              mArg4(arg4),
              mArg5(arg5),
              mArg6(arg6)
        {
        }

        virtual ~WorkerThreadJob6()
        {
            delete mArg1;
            delete mArg2;
            delete mArg3;
            delete mArg4;
            delete mArg5;
            delete mArg6;
        }

        virtual void execute() { this->WorkerThreadJobBase::mReturnValue = (mCaller->*mFunc)(this->WorkerThreadJobResultBase<Result>::mResult, mArg1, mArg2, mArg3, mArg4, mArg5, mArg6); }

    private:
        Caller* mCaller;
        MemberFunc mFunc;

        const P1* mArg1;
        const P2* mArg2;
        const P3* mArg3;
        const P4* mArg4;
        const P5* mArg5;
        const P6* mArg6;

        NON_COPYABLE(WorkerThreadJob6);
    };

private:
    void run() override;
    BlazeRpcError queueJob(WorkerThreadJobBase* job);
    void cleanupJob(WorkerThreadJobBase* job);

    template <class Result>
    BlazeRpcError executeJobOnThread(WorkerThreadJobResultBase<Result>* job, Result*& result);

private:
    static const size_t DEFAULT_STACK_SIZE = 256 * 1024;

    JobQueue mQueue;
    bool mShutdown;

    EA::Thread::Mutex mFreeListMutex;
    WorkerThreadJobBase* mFreeListHead;
};

template <class Result>
BlazeRpcError WorkerThread::executeJobOnThread(WorkerThreadJobResultBase<Result>* job, Result*& result)
{
    BlazeRpcError rc = queueJob(job);
    if (rc == ERR_OK)
    {
        result = job->takeResult();
        delete job;
        return ERR_OK;
    }

    cleanupJob(job);
    return rc;
}

template <class Caller, class Result, class P1>
BlazeRpcError WorkerThread::scheduleCall(Caller* caller, BlazeRpcError (Caller::*mf)(Result*, const P1*), Result*& result, const P1*& arg1)
{
    // Caller should not allocate the result object.
    EA_ASSERT(result == nullptr);

    WorkerThreadJob1<Caller, Result, P1>* job = BLAZE_NEW WorkerThreadJob1<Caller, Result, P1>(caller, mf, arg1);
    // Clear the parameters to prevent the calling fiber from accessing then once the call returns
    arg1 = nullptr;
    
    return executeJobOnThread(job, result);
}

template <class Caller, class Result, class P1, class P2>
BlazeRpcError WorkerThread::scheduleCall(Caller* caller, BlazeRpcError (Caller::*mf)(Result*, const P1*, const P2*), Result*& result, const P1*& arg1, const P2*& arg2)
{
    // Caller should not allocate the result object.
    EA_ASSERT(result == nullptr);

    WorkerThreadJob2<Caller, Result, P1, P2>* job = BLAZE_NEW WorkerThreadJob2<Caller, Result, P1, P2>(caller, mf, arg1, arg2);
    // Clear the parameters to prevent the calling fiber from accessing then once the call returns
    arg1 = nullptr;
    arg2 = nullptr;
    
    return executeJobOnThread(job, result);
}

template <class Caller, class Result, class P1, class P2, class P3>
BlazeRpcError WorkerThread::scheduleCall(Caller* caller, BlazeRpcError (Caller::*mf)(Result*, const P1*, const P2*, const P3*), Result*& result, const P1*& arg1, const P2*& arg2, const P3*& arg3)
{
    // Caller should not allocate the result object.
    EA_ASSERT(result == nullptr);

    WorkerThreadJob3<Caller, Result, P1, P2, P3>* job = BLAZE_NEW WorkerThreadJob3<Caller, Result, P1, P2, P3>(caller, mf, arg1, arg2, arg3);
    // Clear the parameters to prevent the calling fiber from accessing then once the call returns
    arg1 = nullptr;
    arg2 = nullptr;
    arg3 = nullptr;
    
    return executeJobOnThread(job, result);
}

template <class Caller, class Result, class P1, class P2, class P3, class P4>
BlazeRpcError WorkerThread::scheduleCall(Caller* caller, BlazeRpcError (Caller::*mf)(Result*, const P1*, const P2*, const P3*, const P4*), Result*& result, const P1*& arg1, const P2*& arg2, const P3*& arg3, const P4*& arg4)
{
    // Caller should not allocate the result object.
    EA_ASSERT(result == nullptr);

    WorkerThreadJob4<Caller, Result, P1, P2, P3, P4>* job = BLAZE_NEW WorkerThreadJob4<Caller, Result, P1, P2, P3, P4>(caller, mf, arg1, arg2, arg3, arg4);
    // Clear the parameters to prevent the calling fiber from accessing then once the call returns
    arg1 = nullptr;
    arg2 = nullptr;
    arg3 = nullptr;
    arg4 = nullptr;
    
    return executeJobOnThread(job, result);
}

template <class Caller, class Result, class P1, class P2, class P3, class P4, class P5>
BlazeRpcError WorkerThread::scheduleCall(Caller* caller, BlazeRpcError (Caller::*mf)(Result*, const P1*, const P2*, const P3*, const P4*, const P5*), Result*& result, const P1*& arg1, const P2*& arg2, const P3*& arg3, const P4*& arg4, const P5*& arg5)
{
    // Caller should not allocate the result object.
    EA_ASSERT(result == nullptr);

    WorkerThreadJob5<Caller, Result, P1, P2, P3, P4, P5>* job = BLAZE_NEW WorkerThreadJob5<Caller, Result, P1, P2, P3, P4, P5>(caller, mf, arg1, arg2, arg3, arg4, arg5);
    // Clear the parameters to prevent the calling fiber from accessing then once the call returns
    arg1 = nullptr;
    arg2 = nullptr;
    arg3 = nullptr;
    arg4 = nullptr;
    arg5 = nullptr;
    
    return executeJobOnThread(job, result);
}

template <class Caller, class Result, class P1, class P2, class P3, class P4, class P5, class P6>
BlazeRpcError WorkerThread::scheduleCall(Caller* caller, BlazeRpcError (Caller::*mf)(Result*, const P1*, const P2*, const P3*, const P4*, const P5*, const P6*), Result*& result, const P1*& arg1, const P2*& arg2, const P3*& arg3, const P4*& arg4, const P5*& arg5, const P6*& arg6)
{
    // Caller should not allocate the result object.
    EA_ASSERT(result == nullptr);

    WorkerThreadJob6<Caller, Result, P1, P2, P3, P4, P5, P6>* job = BLAZE_NEW WorkerThreadJob6<Caller, Result, P1, P2, P3, P4, P5, P6>(caller, mf, arg1, arg2, arg3, arg4, arg5, arg6);
    // Clear the parameters to prevent the calling fiber from accessing then once the call returns
    arg1 = nullptr;
    arg2 = nullptr;
    arg3 = nullptr;
    arg4 = nullptr;
    arg5 = nullptr;
    arg6 = nullptr;
    
    return executeJobOnThread(job, result);
}
} // Blaze

#endif // BLAZE_WORKERTHREAD_H

