/*************************************************************************************************/
/*!
\file gamepackerutiltest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_GAMEPACKERUTIL_TEST_H
#define BLAZE_SERVER_TEST_GAMEPACKERUTIL_TEST_H

#include "gtest/gtest.h"

#include "test/matchmaker/mmtestfixtureutil.h"

namespace Blaze
{
    class ConfigureValidationErrors;
    namespace GameManager
    { 
        class PackerConfig;
        class CreateGameTemplate;
    }
}

namespace BlazeServerTest
{
namespace Matchmaker
{
    class GamePackerUtilTest : public testing::Test, public MMTestFixtureUtil
    {
    protected:
        // You can remove any or all of the following functions if its body
        // is empty.

        GamePackerUtilTest() {
            // You can do set-up work for each test here.
        }

        virtual ~GamePackerUtilTest() {
            // You can do clean-up work that doesn't throw exceptions here.
        }

        // If the constructor and destructor are not enough for setting up
        // and cleaning up each test, you can define the following methods:

        virtual void SetUp() {}

        virtual void TearDown() {
            // Code here will be called immediately after each test (right
            // before the destructor).
        }

        // Objects declared here can be used by all tests in the fixture
        bool doInitPacker(Blaze::ConfigureValidationErrors* configValErrs,
            const Blaze::GameManager::PackerConfig& packerConfig,
            TestingUtils::VerifyBool expectOk,
            uint16_t gameCapacity = 4,
            uint16_t teamCount = 2);
        bool doInitPackerEx(Blaze::ConfigureValidationErrors* configValErrs,
            const Blaze::GameManager::CreateGameTemplate& cfg,
            const char8_t* siloName,
            TestingUtils::VerifyBool expectOk);
    };

}
}  // namespace

#endif