/*************************************************************************************************/
/*! 
    \file mockprocesscontroller.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_MOCK_PROCESS_CONTROLLER_H
#define BLAZE_SERVER_TEST_MOCK_PROCESS_CONTROLLER_H

#include "framework/blaze.h"
#include "framework/controller/processcontroller.h"//for ProcessController::CmdParams& params in ctor

namespace Blaze
{
    class ConfigBootUtil;
    class ServerVersion;
    typedef EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, EA::TDF::TdfString > CommandLineArgs;
}

namespace BlazeServerTest
{
    class MockServerRunnerThread;

    class MockProcessController
    {
    public:
        MockProcessController();
        ~MockProcessController();
        void mockProcessControllerCtor(const Blaze::ConfigBootUtil& bootUtil,
            const Blaze::ServerVersion& version,
            const Blaze::ProcessController::CmdParams& params,
            const Blaze::CommandLineArgs& cmdLineArgs);
        void mockProcessControllerDtor();
        void mockInitialize();
        void mockStart();
        void mockShutdown();
        Blaze::ProcessController* mRealProcessController;
        MockServerRunnerThread* mServerRunnerThread;
    };
    
    extern MockProcessController gMockProcessController;
}

#endif