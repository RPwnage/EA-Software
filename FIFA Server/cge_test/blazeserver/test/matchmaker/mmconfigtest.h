/*************************************************************************************************/
/*!
    \file mmconfigtest.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_MMCONFIG_TEST_H
#define BLAZE_SERVER_TEST_MMCONFIG_TEST_H

#include "test/matchmaker/mmtestfixtureutil.h"
#include "gamemanager/tdf/matchmaking_properties_config_server.h"//for MatchmakingFilterMap, MatchmakingFilterDefinition in addBasicFilterDefnToConfig

namespace Blaze
{
namespace Matchmaker { class MatchmakerConfig; }
namespace GameManager
{
    class SubSessionConfig;
    class CreateGameTemplatesConfig;
    class GameQualityFactorConfig;
}
}

namespace BlazeServerTest
{
namespace Matchmaker
{
    class MockMatchmakerSlaveImpl;

    class MmConfigTest : public testing::Test, public MMTestFixtureUtil
    {
    protected:
        MmConfigTest(){}
        ~MmConfigTest() override {}

        void SetUp() override
        {
        }

        void TearDown() override
        {
        }

        // Objects declared here can be used by all tests in the test case for MmConfigTest.

        void helperTestConfig(const Blaze::Matchmaker::MatchmakerConfig& config, bool expectCanConfigure, bool testCreateSession = true) const;
        void helperTestConfigInternal(const Blaze::Matchmaker::MatchmakerConfig& configuration, bool expectCanConfigure, bool testCreateSession) const;
        void helperTestCreateSession(MockMatchmakerSlaveImpl& mmSlaveImpl) const;
        Blaze::GameManager::SubSessionConfig* addBasicSubsession(Blaze::Matchmaker::MatchmakerConfig& config, const char8_t* scenName = nullptr, const char8_t* subsessionName = nullptr) const;
        Blaze::GameManager::MatchmakingFilterDefinition* addBasicFilterDefnToConfig(Blaze::GameManager::MatchmakingFilterMap& configMap, const char8_t* filterName = nullptr) const;
        static Blaze::GameManager::GameQualityFactorConfig* addBasicFactorToConfig(Blaze::GameManager::CreateGameTemplatesConfig& config, const char8_t* templateName = nullptr);
        static void addClassicMmRequiredRulesToConfig(Blaze::Matchmaker::MatchmakerConfig& config);
    };

} // namespace Matchmaker
} // namespace BlazeServerTest

#endif