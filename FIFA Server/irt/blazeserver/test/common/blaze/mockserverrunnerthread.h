/*************************************************************************************************/
/*! 
    \file mockserverrunnerthread.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_MOCK_SERVER_RUNNER_THREAD_H
#define BLAZE_SERVER_TEST_MOCK_SERVER_RUNNER_THREAD_H

#include "framework/blaze.h"
#include "framework/system/serverrunnerthread.h"
#include "framework/metrics/metrics.h" // for MetricsCollection in gMockMetricsCollection

namespace Blaze
{
    class Controller;//for MockServerRunnerThread.mController
    class Selector;
    class SslContextManager;
}

namespace BlazeServerTest
{
    class MockServerRunnerThread : public Blaze::ServerRunnerThread
    {
    public:
        MockServerRunnerThread();
        virtual ~MockServerRunnerThread();
        void mockInitialize();
        void mockSetThreadLocalVars();
        void mockClearThreadLocalVars() const;
        void mockStopControllerInternal();
        void mockStop();
        void mockCleanup();

        //currently-unneeded/untested members are commented out below
        volatile bool mCleanedUp;
        Blaze::Selector* mSelector;
        //Blaze::NameResolver* mNameResolver;
        Blaze::Controller* mController;
        //Blaze::DbScheduler* mDbScheduler;
        //Blaze::RedisManager* mRedisManager;
        //Blaze::Localization* mLocalization;
        //Blaze::ProfanityFilter* mProfanityFilter;
        //Blaze::Replicator* mReplicator;
        //Blaze::CensusDataManager* mCensusDataManager;
        Blaze::Random* mRandom;
        //Blaze::BlockingSocketManager* mBlockingSocketManager;
        //Blaze::OutboundHttpService* mOutboundHttpService;
        //Blaze::OutboundMetricsManager* mOutboundMetricsManager;
        Blaze::VaultManager* mVaultManager;
        //Blaze::Metrics::MetricsCollection mMetricsCollection;//globalled below, in case need metrics across tests
        //Blaze::Metrics::MetricsCollection& mInstanceCollection;
        //Blaze::BlazeGeoIP* mGeoIPInterface;
        Blaze::SslContextManager* mSslContextManager;
    };

    extern Blaze::Metrics::MetricsCollection gMockMetricsCollection;
}

#endif
