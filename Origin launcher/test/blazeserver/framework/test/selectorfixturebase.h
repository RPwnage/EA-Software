/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/**
 * This file defines a GlobalFixture which sets up a selector thread.  It is a convenience
 * class for unit tests that require use of a selector.  It can be subclasses to provide a means
 * of initializing certain class instances on the selector thread via the selectorSetup() and
 * selectorShutdown() methods.
 */

#ifndef BLAZE_TEST_SELECTORFIXTUREBASE_H
#define BLAZE_TEST_SELECTORFIXTUREBASE_H

/*** Include files *******************************************************************************/

#include "eathread/eathread_condition.h"
#include "eathread/eathread_mutex.h"
#include "framework/system/runnerthread.h"
#include "framework/system/job.h"
#include "framework/connection/selector.h"
#include "framework/connection/socketutil.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class SelectorFixtureBase
{
private:
    class SelectorJob;

public:

    SelectorFixtureBase() : mSelectorThread(nullptr), mSelector(nullptr) { }
    virtual ~SelectorFixtureBase() { }

    // This method is called on the selector's thread and can be overridden to do any custom
    // setup work on the selector thread that is necessary for the unit test.
    virtual void selectorSetup() { }

    // This method is called on the selector's thread and can be overridden to do any custom
    // shutdown work on the selector thread that is necessary for the unit test.
    virtual void selectorShutdown() { }

    Selector* getSelector() const { return mSelector; }

    bool setUpWorld()
    {
        SocketUtil::initializeNetworking();
        mConfig = ConfigMap::create("{\nlogLevel=none\n}\n");
        mSelectorThread = new RunnerThread(mConfig, "selector");
        mSelectorThread->start();
        while (!mSelectorThread->isRunning())
            EA::Thread::ThreadSleep(50);
        mSelector = mSelectorThread->getSelector();

        mMutex.Lock();
        mSelector->queueJob(new SelectorJob(this, true));
        mCond.Wait(&mMutex);
        mMutex.Unlock();
        return true;
    }

    bool tearDownWorld()
    {
        mMutex.Lock();
        mSelector->queueJob(new SelectorJob(this, false));
        mCond.Wait(&mMutex);
        mMutex.Unlock();
        mSelectorThread->stop();
        delete mSelectorThread;
        delete mConfig;
        return true;
    }

private:
    EA::Thread::Mutex mMutex;
    EA::Thread::Condition mCond;

    ConfigMap* mConfig;
    RunnerThread* mSelectorThread;
    Selector* mSelector;

    void selectorThreadSetup()
    {
        selectorSetup();
        mCond.Signal(true);
    }

    void selectorThreadShutdown()
    {
        selectorShutdown();
        mCond.Signal(true);
    }

    class SelectorJob : public Job
    {
    public:
        SelectorJob(SelectorFixtureBase* owner, bool setup)
            : mOwner(owner),
              mSetup(setup)
        {
        }

        virtual void execute()
        {
            if (mSetup)
                mOwner->selectorThreadSetup();
            else
                mOwner->selectorThreadShutdown();
        }
    private:
        SelectorFixtureBase* mOwner;
        bool mSetup;
    };
};

} // namespace Blaze

#endif // BLAZE_TEST_SELECTORFIXTURE_H

