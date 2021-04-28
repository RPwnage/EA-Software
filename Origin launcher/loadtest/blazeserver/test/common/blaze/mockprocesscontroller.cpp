/*************************************************************************************************/
/*! \file mockprocesscontroller.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "test/common/blaze/mockprocesscontroller.h"
#include "test/common/blaze/mockserverrunnerthread.h" // for MockProcessController::mServerRunnerThread
#include "test/common/testinghooks.h"//for testlog() in mockProcessControllerCtor etc
#include "gtest/gtest.h"//for ASSERT_EQ in mockInitialize etc
#include "framework/config/config_boot_util.h"//for ConfigBootUtil in MockProcessController()
#include "framework/config/config_file.h"//for PredefineMap in MockProcessController()
#include "framework/tdf/controllertypes_server.h"//for CommandLineArgs in MockProcessController()

namespace BlazeServerTest
{
    MockProcessController gMockProcessController;

    MockProcessController::MockProcessController() : mRealProcessController(nullptr), mServerRunnerThread(nullptr)
    {
    }

    MockProcessController::~MockProcessController()
    {
        //Mock test note: should have cleaned up via test shutdown, just in case:
        mockProcessControllerDtor();
        if (mServerRunnerThread != nullptr)
        {
            delete mServerRunnerThread;
            mServerRunnerThread = nullptr;
        }
    }

    void MockProcessController::mockProcessControllerCtor(
        const Blaze::ConfigBootUtil& bootUtil,
        const Blaze::ServerVersion& version,
        const Blaze::ProcessController::CmdParams& params,
        const Blaze::CommandLineArgs& cmdLineArgs)
    {
        if (mRealProcessController != nullptr)
        {
            testlog("Skip Duplicate calls to mockProcessControllerCtor.");
            return;
        }
        //Mock test note: This Blaze ctor sets gProcessController to this real ProcessController, guards vs null deref's,
        //Not actually using gProcessController much tho, as its complex & hard to mock:
        mRealProcessController = BLAZE_NEW Blaze::ProcessController(bootUtil, version, params, cmdLineArgs);
    }
    /*! ************************************************************************************************/
    /*! \brief mock blazeserver main() destroying local processController
    ***************************************************************************************************/
    void MockProcessController::mockProcessControllerDtor()
    {
        if (mRealProcessController == nullptr)
            return;
        delete mRealProcessController;
        mRealProcessController = nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief Mock Blaze ProcessController::initialize(). Test setup calls this.
    ***************************************************************************************************/
    void MockProcessController::mockInitialize()
    {
        //gSslContextManager->initializeStaticMutexes();//unneeded currently

        //To avoid crashing mock tests, create server runner thread after process controller above
        ASSERT_EQ(nullptr, mServerRunnerThread);
        mServerRunnerThread = BLAZE_NEW MockServerRunnerThread();
    }

    // Mock ProcessController::start(). Test setup calls this.
    void MockProcessController::mockStart()
    {
        ASSERT_NE(nullptr, mServerRunnerThread);
        mServerRunnerThread->mockInitialize();
    }

    // Mock ProcessController::shutdown. pretty simplified compared to real Blaze, but good enough
    void MockProcessController::mockShutdown()
    {
        if (mServerRunnerThread != nullptr)
        {
            mServerRunnerThread->mockStopControllerInternal();

            //Mock test note: (Real ServerRunnerThread::stop() schedules a fiber that deletes this):
            delete mServerRunnerThread;
            mServerRunnerThread = nullptr;
        }
    }

}//ns
