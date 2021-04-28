/*************************************************************************************************/
/*! 
    \file mockblazemain.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_MOCK_BLAZE_MAIN_H
#define BLAZE_SERVER_TEST_MOCK_BLAZE_MAIN_H

#include "test/common/gtest/gtesteventlistener.h" // For GtestEventListener parent class
#include "framework/blaze.h" //needed to compile!
#include "framework/controller/processcontroller.h"//for ProcessController::CmdParams mMockParams

namespace Blaze
{
    class PredefineMap;
    class ConfigBootUtil;
    typedef EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, EA::TDF::TdfString > CommandLineArgs;
}

namespace BlazeServerTest
{
    /*! ************************************************************************************************/
    /*! \brief Mocks Blaze Server startup
    ***************************************************************************************************/
    class MockBlazeMain : public TestingUtils::GtestEventListener
    {
    public:
        MockBlazeMain();
        ~MockBlazeMain();

        // GtestEventListener interface
        void onPreTest(const ::testing::TestInfo& test_info) override;
        void onPostTest(const ::testing::TestInfo& test_info) override;

        void mockMain();
        void mockMainEnd();

        Blaze::ServerVersion mMockVersion;
        Blaze::PredefineMap* mMockPredefMap;
        eastl::string mMockCfgName;
        Blaze::ConfigBootUtil* mMockBootUtil;//dont need to be pointers, but blazeserver.cpp main() has this as
        const Blaze::ProcessController::CmdParams mMockParams;
        const Blaze::CommandLineArgs mCommandLineArgs;
    };

    // This GtestEventListener is statically init, in order to precede tests potentially using it
    extern MockBlazeMain gMockBlazeMain;

}

#endif