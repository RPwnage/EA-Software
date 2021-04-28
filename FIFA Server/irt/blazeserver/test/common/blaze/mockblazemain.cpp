/*************************************************************************************************/
/*! \file mockblazemain.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "mockblazemain.h"
#include <gtest/gtest.h>

#include "test/common/gtest/gtestfixtureutil.h" //for GtestFixtureUtil::getTestResultsDir() in mockMain
#include "test/common/blaze/mockprocesscontroller.h" //for gMockProcessController in mockMain
#include "framework/config/config_file.h" //for PredefineMap mMockPredefMap in MockBlazeMain()
#include "framework/config/config_boot_util.h"//for ConfigBootUtil mMockBootUtil in MockBlazeMain()

namespace BlazeServerTest
{
    MockBlazeMain gMockBlazeMain;

    MockBlazeMain::MockBlazeMain() : TestingUtils::GtestEventListener(),
        mMockPredefMap(BLAZE_NEW Blaze::PredefineMap()),
        mMockCfgName("test"),
        mMockBootUtil(nullptr)
    {
    }

    MockBlazeMain::~MockBlazeMain()
    {
        delete mMockBootUtil;
        delete mMockPredefMap;
    }

    // Called before a test starts
    void MockBlazeMain::onPreTest(const ::testing::TestInfo& test_info)
    {
        testlog("*** Test %s.%s starting.\n", (test_info.test_case_name() ? test_info.test_case_name() : "<nullptr>"), (test_info.name() ? test_info.name() : "<nullptr>"));

        // for simplicity enabled for every test (could allow per test/fixture basis, more complex)
        mockMain();
    }
    // Called after a test ends
    void MockBlazeMain::onPostTest(const ::testing::TestInfo& test_info)
    {
        testlog("*** Test %s.%s ended, cleaning up.\n", (test_info.test_case_name() ? test_info.test_case_name() : "<nullptr>"), (test_info.name() ? test_info.name() : "<nullptr>"));
        mockMainEnd();
    }

    /*! ************************************************************************************************/
    /*! \brief Mock blazeserver.cpp main(). Test setup calls this. See Blaze's bootstrap/start flow,
            or Blaze stress client and cfgtest

        Some Blaze component tests require a (mock) process controller and server thread instantiated.
        This will do that, as well starting the MockBlazeLogger.
        
        \note some tests may even crash without this setup (on null Blaze globals etc)
    ***************************************************************************************************/
    void MockBlazeMain::mockMain()
    {
        // Mock: Blaze::Fiber::FiberStorageInitializer fiberStorage;
        if (Blaze::Fiber::getFiberStorageInitialized())
        {
            // Blaze logging not yet up, just call directly:
            GTEST_FATAL_FAILURE_("Duplicate mockStart calls, without shut down. Poss test issue.");
            return;
        }
        Blaze::Fiber::initThreadForFibers();// (cleaned up in MockBlazeMain::mockMainEnd)
        EXPECT_TRUE(nullptr != Blaze::Fiber::getMainFiber());//sanity

        //side: this also does initMemGroupNames() to ensure no crashing later on logging, etc.
        Blaze::BlazeRpcComponentDb::initialize();

        //Blaze::Grpc::grpcCoreSetupAllocationCallbacks();//gtests: poss future mocking
        //grpc_init();

        // side: no need to currently mock actually configuring anything. Just need the util here:
        if (mMockBootUtil == nullptr)
        {
            mMockBootUtil = BLAZE_NEW Blaze::ConfigBootUtil(mMockCfgName, mMockCfgName, *mMockPredefMap, true);
        }

        // Mock: ProcessController processController(bootUtil,..)
        gMockProcessController.mockProcessControllerCtor(*mMockBootUtil, mMockVersion, mMockParams, mCommandLineArgs);

        // Mock: processController.initialize() :
        gMockProcessController.mockInitialize();

        // Mock: processController.start()
        gMockProcessController.mockStart();
    }

    /*! ************************************************************************************************/
    /*! \brief Mock blazeserver.cpp main() ending (in real Blaze, via shutdown RPC/process kill).
        Test teardown calls this. (after a mockStart(), to clean up)
    ***************************************************************************************************/
    void MockBlazeMain::mockMainEnd()
    {
        gMockProcessController.mockShutdown();
        gMockProcessController.mockProcessControllerDtor();

        if (Blaze::Fiber::getFiberStorageInitialized())
        {
            Blaze::Fiber::shutdownThreadForFibers();
        }
        Blaze::Metrics::gMetricsCollection = nullptr;
        Blaze::Metrics::gFrameworkCollection = nullptr;
    }


}//ns
