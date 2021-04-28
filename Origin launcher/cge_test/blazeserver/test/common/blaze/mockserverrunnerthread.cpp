/*************************************************************************************************/
/*! \file mockserverrunnerthread.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "test/common/blaze/mockserverrunnerthread.h"
#include "test/common/testinghooks.h" // for testerr() in mockInitialize
#include "gtest/gtest.h"//for ASSERT_EQ in mockInitialize etc
#include "framework/vault/vaultmanager.h"//for gVaultManager in mockSetThreadLocalVars
#include "framework/util/random.h" // for mRandom in mockInitialize

namespace BlazeServerTest
{
    Blaze::Metrics::MetricsCollection gMockMetricsCollection;

    static char8_t mockName[32] = "test";

    MockServerRunnerThread::MockServerRunnerThread() : Blaze::ServerRunnerThread(1234, mockName, mockName, mockName),
        mCleanedUp(false),
        mSelector(nullptr),
        //mNameResolver(nullptr),
        mController(nullptr),
        //mDbScheduler(nullptr),
        //mRedisManager(nullptr),
        //mLocalization(nullptr),
        //mProfanityFilter(nullptr),
        //mReplicator(nullptr),
        //mCensusDataManager(nullptr),
        mRandom(nullptr),
       //mBlockingSocketManager(nullptr),
        //mOutboundHttpService(nullptr),
        //mOutboundMetricsManager(nullptr),
        mVaultManager(nullptr),
        //mInstanceCollection(mMetricsCollection.getCollection(Metrics::Tag::service_name, gProcessController->getBootConfigTdf().getDefaultServiceName()).getCollection(Metrics::Tag::hostname, hostName).getCollection(Metrics::Tag::instance_type, baseName).getCollection(Metrics::Tag::instance_id, instanceId)),
        //mGeoIPInterface(nullptr),
        mSslContextManager(nullptr)
    {
        EXPECT_NE(nullptr, Blaze::Fiber::getMainFiber());
    }

    MockServerRunnerThread::~MockServerRunnerThread()
    {
        // just in case
        mockCleanup();
    }

    // Mock ServerRunnerThread::initialize. Test setup calls this.
    void MockServerRunnerThread::mockInitialize()
    {
        //EXPECT_EQ(nullptr, mDbScheduler);
        //EXPECT_EQ(nullptr, mRedisManager);
        //EXPECT_EQ(nullptr, mLocalization);
        //EXPECT_EQ(nullptr, mProfanityFilter);
        //EXPECT_EQ(nullptr, mReplicator);
        //EXPECT_EQ(nullptr, mCensusDataManager);
        //EXPECT_EQ(nullptr, mBlockingSocketManager);
        //EXPECT_EQ(nullptr, mOutboundHttpService);
        //EXPECT_EQ(nullptr, mOutboundMetricsManager);
        //EXPECT_EQ(nullptr, mGeoIPInterface);
        EXPECT_EQ(nullptr, Blaze::Metrics::gMetricsCollection);
        Blaze::Metrics::gMetricsCollection = &gMockMetricsCollection;

        EXPECT_EQ(nullptr, Blaze::Metrics::gFrameworkCollection);
        Blaze::Metrics::gFrameworkCollection = &Blaze::Metrics::gMetricsCollection->getTaglessCollection("framework");
        //Logger::initializeThreadLocal();//unneeded currently

        EXPECT_EQ(nullptr, mSelector);
        mSelector = Blaze::Selector::createSelector();

        EXPECT_EQ(nullptr, mController);
        mController = BLAZE_NEW Blaze::Controller(*mSelector, "gtest", "gtest", 123);

        EXPECT_EQ(nullptr, mRandom);
        mRandom = BLAZE_NEW Blaze::Random();

        EXPECT_EQ(nullptr, mVaultManager);
        mVaultManager = BLAZE_NEW Blaze::VaultManager();

        EXPECT_EQ(nullptr, mSslContextManager);
        mSslContextManager = BLAZE_NEW Blaze::SslContextManager();

        mockSetThreadLocalVars();

        //Use a blank selector config.
        Blaze::SelectorConfig dummyConfig;
        if (!mSelector->initialize(dummyConfig))
        {
            testerr("Failed to initialize selector. Exiting.");
            return;
        }
        mSelector->open();
    }

    // Mock ServerRunnerThread::setThreadLocalVars()
    void MockServerRunnerThread::mockSetThreadLocalVars()
    {
        //Commented out lines are from Blaze, but aren't currently needed for tests:

        //Mock test note:
        //The below mXX members should be null, test shouldn't be calling this fn twice in row.
        //The below gXX vars checked not null, since test shouldn't be calling this fn twice in row,
        //and to avoid issues, you shouldn't use real ServerRunnerThread when using this mock class.

        EXPECT_NE(nullptr, mSelector);
        EXPECT_EQ(nullptr, Blaze::gSelector);
        Blaze::gSelector = mSelector;

        EXPECT_EQ(nullptr, Blaze::gFiberManager);
        Blaze::gFiberManager = &mSelector->getFiberManager();

        //Blaze::gNameResolver = mNameResolver;

        EXPECT_NE(nullptr, mController);
        EXPECT_EQ(nullptr, Blaze::gController);
        Blaze::gController = mController;

        //Blaze::gDbScheduler = mDbScheduler;
        //Blaze::gRedisManager = mRedisManager;
        //Blaze::gLocalization = mLocalization;
        //Blaze::gProfanityFilter = mProfanityFilter;
        //Blaze::gReplicator = mReplicator;
        //Blaze::gCensusDataManager = mCensusDataManager;

        EXPECT_NE(nullptr, mRandom);
        EXPECT_EQ(nullptr, Blaze::gRandom);
        Blaze::gRandom = mRandom;

        //Blaze::gBlockingSocketManager = mBlockingSocketManager;
        //Blaze::gOutboundHttpService = mOutboundHttpService;
        //Blaze::gOutboundMetricsManager = mOutboundMetricsManager;

        EXPECT_NE(nullptr, mVaultManager);
        EXPECT_EQ(nullptr, Blaze::gVaultManager);
        Blaze::gVaultManager = mVaultManager;

        //Blaze::gGeoIPInterface = mGeoIPInterface;

        EXPECT_NE(nullptr, mSslContextManager);
        EXPECT_EQ(nullptr, Blaze::gSslContextManager);
        Blaze::gSslContextManager = mSslContextManager;
    }

    // Mock ServerRunnerThread::clearThreadLocalVars()
    void MockServerRunnerThread::mockClearThreadLocalVars() const
    {
        //Commented out lines are from Blaze, but aren't currently needed for tests:

        //Mock test note:
        //The below gXX vars checked not null, since test shouldn't be calling this fn twice in row,
        //and to avoid issues, you shouldn't use real ServerRunnerThread when using this mock class.
        EXPECT_NE(nullptr, Blaze::gSelector);
        Blaze::gSelector = nullptr;

        EXPECT_NE(nullptr, Blaze::gFiberManager);
        Blaze::gFiberManager = nullptr;

        //EXPECT_NE(nullptr, Blaze::gNameResolver);
        //Blaze::gNameResolver = nullptr;

        EXPECT_NE(nullptr, Blaze::gController);
        Blaze::gController = nullptr;

        //Blaze::gDbScheduler = nullptr;
        //Blaze::gRedisManager = nullptr;
        //Blaze::gLocalization = nullptr;
        //Blaze::gProfanityFilter = nullptr;
        //Blaze::gReplicator = nullptr;
        //Blaze::gCensusDataManager = nullptr;

        EXPECT_NE(nullptr, Blaze::gRandom);
        Blaze::gRandom = nullptr;

        //EXPECT_NE(nullptr, Blaze::gBlockingSocketManager);
        //Blaze::gBlockingSocketManager = nullptr;

        //Blaze::gOutboundHttpService = nullptr;
        //Blaze::gOutboundMetricsManager = nullptr;

        EXPECT_NE(nullptr, Blaze::gVaultManager);
        Blaze::gVaultManager = nullptr;
        //Blaze::gGeoIPInterface = nullptr;

        EXPECT_NE(nullptr, Blaze::gSslContextManager);
        Blaze::gSslContextManager = nullptr;
    }

    // Mock ServerRunnerThread::stopControllerInternal
    void MockServerRunnerThread::mockStopControllerInternal()
    {
        // make sure no outstanding timers or jobs fire
        // (component shutdown may have oaccidentally scheduled some)
        EXPECT_NE(nullptr, mSelector);
        if (mSelector != nullptr)
            mSelector->removeAllEvents();

        //Mock test note: again, simplifying here vs real Blaze, by skipping some frames
        mockStop();

    }

    /*! ************************************************************************************************/
    /*! \brief Mock ServerRunnerThread::stop(). Call this during test shutdown to cleanup the 'thread'!
        Side: more a mock of ServerRunnerThread::stopCommonInternal (which calls ServerRunnerThread::stop)
    ***************************************************************************************************/
    void MockServerRunnerThread::mockStop()
    {
        // (skipping mocking some extra Blaze steps here)

        mockCleanup();
    }

    void MockServerRunnerThread::mockCleanup()
    {
        //Commented out lines are from Blaze, but aren't currently needed for tests:

        if (mCleanedUp)
        {
            return;
        }
        mCleanedUp = true;

        //Stop the name resolver threads
        //mNameResolver->stop();

        //Stop all the DB threads
        //mDbScheduler->stop();

        //Free memory used in libcurl per instance 
        //OutboundHttpConnectionManager::shutdownHttpForInstance();

        //Destroy all our sub managers.
        //delete mCensusDataManager;
        //delete mReplicator;
        //delete mProfanityFilter;
        //delete mLocalization;
        //delete mDbScheduler;
        //delete mRedisManager;
        //delete mNameResolver;

        //Mock test note: for this mock test class, set the ptrs to null, after deleting, just in case:
        delete mController;
        mController = nullptr;

        /////////////////////////
        //Mock test note: (Not in real Blaze) Extra test validations, before delete mSelector:
        EXPECT_NE(nullptr, mSelector);
        //Mock test note: Blaze flow doesn't change gSelector here. It should be set to the current mSelector:
        EXPECT_TRUE(Blaze::gSelector == nullptr || mSelector == Blaze::gSelector);
        //Mock test note: Also, to be safe/avoid test crashes, ensure removeAllEvents() (real Blaze does earlier)
        if (mSelector != nullptr)
        {
            mSelector->removeAllEvents();
        }
        ////////////////////


        delete mSelector;
        mSelector = nullptr;

        delete mRandom;
        mRandom = nullptr;

        //delete mBlockingSocketManager;
        //delete mOutboundHttpService;
        //delete mOutboundMetricsManager;

        delete mVaultManager;
        mVaultManager = nullptr;

        //delete gRawBufferPool;
        //delete mGeoIPInterface;

        delete mSslContextManager;
        mSslContextManager = nullptr;

        //Logger::destroyThreadLocal();

        mockClearThreadLocalVars();

        //if (gProcessController != nullptr)
        //{
        //    gProcessController->removeInstance(this);
        //}
    }

}//ns BlazeServerTest
