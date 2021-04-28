/*************************************************************************************************/
/*!
    \file mmtestfixtureutil.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef TEST_MM_TEST_FIXTURE_UTIL_H
#define TEST_MM_TEST_FIXTURE_UTIL_H

#include "test/mock/mocklogger.h"
#include "test/common/gtest/gtestfixtureutil.h"
#include "framework/blaze.h"
#include "gamemanager/tdf/matchmaking_properties_config_server.h"//for MatchmakingFilterMap, MatchmakingFilterDefinition in addBasicFilterDefnToConfig

namespace Blaze
{
    namespace Matchmaker { class MatchmakerConfig; }
    namespace GameManager
    {
        class SubSessionConfig;
        class CreateGameTemplatesConfig;
        class GameQualityFactorConfig;
        class MatchmakingSessionMode;
        namespace Matchmaker { class MatchmakingSession; }
    }
}


namespace BlazeServerTest
{
namespace Matchmaker
{
    // MM test base
    class MMTestFixtureUtil : public TestingUtils::GtestFixtureUtil
    {
    public:
        MMTestFixtureUtil();
        ~MMTestFixtureUtil() override;

        // Members declared here can be used by all tests in the test case

        Blaze::GameManager::SubSessionConfig* addSubsessionConfig(Blaze::Matchmaker::MatchmakerConfig& config, const char8_t* scenName = nullptr, const char8_t* subsessionName = nullptr, Blaze::GameManager::MatchmakingSessionMode* sessionMode = nullptr) const;
        Blaze::GameManager::MatchmakingFilterDefinition* addBasicFilterDefnToConfig(Blaze::GameManager::MatchmakingFilterMap& configMap, const char8_t* filterName = nullptr) const;
        static Blaze::GameManager::GameQualityFactorConfig* addBasicFactorToConfig(Blaze::GameManager::CreateGameTemplatesConfig& config, const char8_t* templateName = nullptr);
        static void addClassicMmRequiredRulesToConfig(Blaze::Matchmaker::MatchmakerConfig& config);

        static const char8_t* toString(eastl::string& result, const Blaze::ConfigureValidationErrors& validationErrors);

        Blaze::GameManager::Matchmaker::MatchmakingSession* testConstructSession(const Blaze::Matchmaker::MatchmakerConfig& config);
        void testConstructSession(const Blaze::Matchmaker::MatchmakerConfig& config, Blaze::GameManager::Matchmaker::MatchmakingSession*& session);
    private:
        // For testConstructSession():
        Blaze::GameManager::MatchmakingSessionId mNextMatchmakingSessionId;
        Blaze::GameManager::MatchmakingScenarioId mNextMatchmakingScenarioId;
        typedef eastl::hash_map<Blaze::GameManager::MatchmakingSessionId, Blaze::GameManager::Matchmaker::MatchmakingSession*> MatchmakingSessionMap;
        MatchmakingSessionMap mConstructedSessions;//for auto cleanup at fixture end
    };



} // namespace Matchmaker
} // namespace BlazeServerTest

#endif