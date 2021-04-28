/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include "framework/blaze.h"
#include "eathread/eathread_condition.h"
#include "eathread/eathread_mutex.h"
#include "framework/config/config_map.h"
#include "framework/connection/socketutil.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/nameresolver.h"
#include "framework/system/runnerthread.h"
#include "framework/system/job.h"
#include "framework/connection/selector.h"
#include "framework/test/blazeunittest.h"
#include "framework/config/config_file.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
using namespace Blaze;

static struct
{
    const char8_t* hostname;
    InetAddress* addr;
    bool shouldSucceed;
} mLookups[] =
{
    { "www.google.com", nullptr, true  },
    { "www.yahoo.com", nullptr,  true  },
    { "bogus.ea.com", nullptr, false }
};

/*** Test Classes ********************************************************************************/
class TestNameResolver
{

public:
    TestNameResolver()
        : mSelectorThread(*ConfigFile::createFromString("{\nlogLevel=none\n}\n"), "nameresolver")
    {

    }

    ~TestNameResolver()
    {
        
    }

    void testNameResolver()
    {
        SocketUtil::initializeNetworking();
        mSelectorThread.start();
        while (!mSelectorThread.isRunning())
            ;
        mSelector = &mSelectorThread.getSelector();

        mResolved = 0;

        int32_t count = (int32_t)(sizeof(mLookups) / sizeof(mLookups[0]));
        for(int32_t i = 0; i < count; i++)
        {
            mLookups[i].addr = new InetAddress(mLookups[i].hostname, 80);
            mSelector->scheduleFiberCall(this, &TestNameResolver::doResolve, mLookups[i].addr);
        }

        while (mResolved != count)
        {
            mMutex.Lock();
            mCond.Wait(&mMutex);
            mMutex.Unlock();
        }

        mSelectorThread.stop();
    }

    void testBlockingNameResolver()
    {
        int32_t count = (int32_t)(sizeof(mLookups) / sizeof(mLookups[0]));
        for(int32_t i = 0; i < count; i++)
        {
            InetAddress* address = new InetAddress(mLookups[i].hostname, 80);
            bool success = gNameResolver->blockingResolve(*address);
            BUT_ASSERT(mLookups[i].shouldSucceed == success);
        }
    }

    void doResolve(InetAddress* address)
    {     
        if (gNameResolver->resolve(*address) != Blaze::ERR_OK)
        {
            int32_t count = (int32_t)(sizeof(mLookups) / sizeof(mLookups[0]));

            for(int32_t i = 0; i < count; i++)
            {
                if (mLookups[i].addr == address)
                {
                    BUT_ASSERT(!mLookups[i].shouldSucceed);
                }
            }

            mResolved++;
            mCond.Signal(true);            
        }
        else
        {
            int32_t count = (int32_t)(sizeof(mLookups) / sizeof(mLookups[0]));

            for(int32_t i = 0; i < count; i++)
            {
                if (mLookups[i].addr == address)
                {
                    BUT_ASSERT(mLookups[i].shouldSucceed);
                }
            }

            mResolved++;
            mCond.Signal(true);
        }
    }

private:
    EA::Thread::Mutex mMutex;
    EA::Thread::Condition mCond;
    int32_t mResolved;

    RunnerThread mSelectorThread;
    Selector* mSelector;

};



// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_NAMERESOLVER\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestNameResolver testNameResolver;
    testNameResolver.testNameResolver();
    testNameResolver.testBlockingNameResolver();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

