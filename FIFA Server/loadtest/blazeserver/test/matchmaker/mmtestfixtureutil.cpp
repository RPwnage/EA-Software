/*************************************************************************************************/
/*!
    \file mmtestfixtureutil.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "test/matchmaker/mmtestfixtureutil.h"
#include "test/common/blaze/mockmatchmakerslaveimpl.h"
#include "gamepacker/packer.h" // for GamePacker::Packer::gCalcShaperTypeNames in addBasicFactorToConfig

namespace BlazeServerTest
{
namespace Matchmaker
{
    MMTestFixtureUtil::MMTestFixtureUtil() :
        mNextMatchmakingSessionId(1),
        mNextMatchmakingScenarioId(1)
    {
    }
    MMTestFixtureUtil::~MMTestFixtureUtil()
    {
        // Note: dtors hit before GtestEventListener::onPostTest (allocators set up in that should still be valid etc)
        for (auto itr : mConstructedSessions)
            delete itr.second;
        mConstructedSessions.clear();
    }

    ////////////////// TEST HELPERS

    // BlazeServer config object setup helper. Adds a subsession/Scenario to test config
    Blaze::GameManager::SubSessionConfig* MMTestFixtureUtil::addSubsessionConfig(Blaze::Matchmaker::MatchmakerConfig& config,
        const char8_t* scenName,
        const char8_t* subsessionName,
        Blaze::GameManager::MatchmakingSessionMode* sessionMode) const
    {
        Blaze::GameManager::ScenarioName name = (scenName == nullptr ? getTestName().c_str() : scenName);
        if (has_testerr())
            return nullptr;
        if (subsessionName == nullptr)
            subsessionName = name.c_str();
        Blaze::GameManager::MatchmakingSessionMode mode;
        if (sessionMode != nullptr)
            mode.setBits(sessionMode->getBits());
        else
            mode.setCreateGame(true);

        auto* scenarioConfig = config.getScenariosConfig().getScenarios().allocate_element();
        config.getScenariosConfig().getScenarios()[name.c_str()] = scenarioConfig;
        
        auto* subsessionConfig = scenarioConfig->getSubSessions().allocate_element();
        scenarioConfig->getSubSessions()[subsessionName] = subsessionConfig;

        subsessionConfig->getMatchmakingSettings().getSessionMode().setBits(mode.getBits());

        return subsessionConfig;
    }
    // BlazeServer config object setup helper. Add a filter definition to the MM Scenarios config map
    Blaze::GameManager::MatchmakingFilterDefinition* MMTestFixtureUtil::addBasicFilterDefnToConfig(Blaze::GameManager::MatchmakingFilterMap& configMap,
        const char8_t* filterName) const
    {
        Blaze::GameManager::MatchmakingFilterName name = (filterName == nullptr ? getTestName().c_str() : filterName);
        if (has_testerr())
            return nullptr;

        eastl::string gamepropName(eastl::string::CtorSprintf(), "%s%s", "game", Blaze::GameManager::PROPERTY_NAME_GPVS);// "game.gameProtocolVersionString"
        auto defn = configMap.allocate_element();
        defn->setDescription(eastl::string(eastl::string::CtorSprintf(), "%s: basic description", name.c_str()).c_str());
        defn->setGameProperty(gamepropName.c_str());

        defn->getRequirement()["IntEqualityFilter"] = defn->getRequirement().allocate_element();
        Blaze::GameManager::TemplateAttributeMapping& mapping = *defn->getRequirement()["IntEqualityFilter"];

        mapping["operation"] = mapping.allocate_element();
        mapping["operation"]->getDefault().set(Blaze::GameManager::EQUAL);

        eastl::string callerpropName(eastl::string::CtorSprintf(), "%s%s", "caller", Blaze::GameManager::PROPERTY_NAME_GPVS);// "caller.gameProtocolVersionString"
        mapping["value"] = mapping.allocate_element();
        mapping["value"]->getDefault().set(callerpropName.c_str());

        configMap[filterName] = defn;
        return defn;
    }
    // BlazeServer config object setup helper. Add a factor definition to the create game template config section
    Blaze::GameManager::GameQualityFactorConfig* MMTestFixtureUtil::addBasicFactorToConfig(
        Blaze::GameManager::CreateGameTemplatesConfig& config,
        const char8_t* templateName)
    {
        Blaze::GameManager::TemplateName tempName = (templateName == nullptr ? getTestName().c_str() : templateName);
        if (has_testerr())
            return nullptr;

        auto cgTemplate = config.getCreateGameTemplates().allocate_element();
        config.getCreateGameTemplates()[tempName.c_str()] = cgTemplate;

        auto factor = cgTemplate->getPackerConfig().getQualityFactors().pull_back();
        factor->setGameProperty("game.playerCount");
        //factor->getScoreShaper().setFunction("range");// Packer::PackerSilo::gCalcShaperTypeNames[(int32_t)GamePacker::FactorShaperType::RANGE]);
        //factor->getScoreShaper().setViableValue(0);
        //factor->getScoreShaper().setMaxValue("4");
        //Possible test enhancement: to allow using game.participantCapacity, would require mock pre config of properties:
        //{ goal="maximize", gameProperty="game.playerCount", scoreShaper={ function="range", viableValue="2", maxValue="game.participantCapacity" } }
        //eastl::string maxValue(eastl::string::CtorSprintf(), "%s%s", Blaze::GameManager::GAME_PROPERTY_PREFIX, Blaze::GameManager::PROPERTY_NAME_PARTICIPANT_CAPACITY);
        //factor->getScoreShaper().setMaxValue(maxValue.c_str());

        return factor;
    }
    // BlazeServer config object setup helper. add required 'classic MM' rules
    void MMTestFixtureUtil::addClassicMmRequiredRulesToConfig(Blaze::Matchmaker::MatchmakerConfig& config)
    {
        // TotalPlayerSlotsRule must have fiftyPercentFitValueDifference and a RangeOffsetList
        auto& totalSlots = config.getMatchmakerSettings().getRules().getPredefined_TotalPlayerSlotsRule();
        totalSlots.getFitFormula().setName(Blaze::GameManager::FIT_FORMULA_GAUSSIAN);
        totalSlots.getFitFormula().getParams()["fiftyPercentFitValueDifference"] = "2";
        auto offList = totalSlots.getRangeOffsetLists().pull_back();
        offList->setName("matchAny");
        auto offset = offList->getRangeOffsets().pull_back();
        offset->setT(0);
        offset->getOffset().push_back(Blaze::GameManager::Matchmaker::RangeListContainer::INFINITY_RANGE_TOKEN);

        // HostViabilityRule must have a list
        auto& hostViab = config.getMatchmakerSettings().getRules().getPredefined_HostViabilityRule();
        auto list = hostViab.getMinFitThresholdLists().allocate_element();
        hostViab.getMinFitThresholdLists()["matchAny"] = list;
        list->push_back("0:CONNECTION_UNLIKELY");
    }

    const char8_t* MMTestFixtureUtil::toString(eastl::string& result, const Blaze::ConfigureValidationErrors& validationErrors)
    {
        result.clear();
        for (auto itr : validationErrors.getErrorMessages())
            result.append_sprintf("%s%s", (result.empty() ? "" : ", "), itr.c_str());
        return result.c_str();
    }

    // Mock constructing a new BlazeServer MM session.
    Blaze::GameManager::Matchmaker::MatchmakingSession* MMTestFixtureUtil::testConstructSession(
        const Blaze::Matchmaker::MatchmakerConfig& config)
    {
        Blaze::GameManager::Matchmaker::MatchmakingSession* session = nullptr;
        EXPECT_NO_THROW(testConstructSession(config, session););
        return session;
    }

    void MMTestFixtureUtil::testConstructSession(const Blaze::Matchmaker::MatchmakerConfig& config,
        Blaze::GameManager::Matchmaker::MatchmakingSession*& session)
    {
        eastl::string buf;

        Blaze::Matchmaker::MatchmakerConfig configuration;
        config.copyInto(configuration);//non const copy to compile below

        // don't activate yet, we'll explicitly call configuration methods first
        MockMatchmakerSlaveImpl mmSlaveImpl(config, false);

        // Unit test calling the internal configure methods, normally done via bootstrap:
        // When there's no filters, factors nor rules actually used, configure should fail
        Blaze::ConfigureValidationErrors validationErrors;
        bool validateResult = mmSlaveImpl.onValidateConfigTdf(configuration, nullptr, validationErrors);

        const char8_t* mmConfigValidationErrors = toString(buf, validationErrors);
        ASSERT_STREQ("", mmConfigValidationErrors);
        ASSERT_TRUE(validateResult);

        if (mmSlaveImpl.getState() != Blaze::ComponentStub::ACTIVE)
        {
            mmSlaveImpl.activateComponent();
            // Must configure rules to avoid crash:
            ASSERT_TRUE(mmSlaveImpl.getMatchmaker()->configure(config));
        }

        ASSERT_TRUE(nullptr != mmSlaveImpl.getMatchmaker());

        // construct it. Store in mConstructedSessions for auto cleanup, when test fixture ends.
        auto mmSessId = mNextMatchmakingSessionId++;
        auto scenarioId = mNextMatchmakingScenarioId++;
        session = BLAZE_NEW Blaze::GameManager::Matchmaker::MatchmakingSession(
            mmSlaveImpl, *mmSlaveImpl.getMatchmaker(), mmSessId, scenarioId);
        mConstructedSessions.insert(eastl::make_pair(mmSessId, session));

        ASSERT_EQ(mmSessId, session->getMMSessionId());
        ASSERT_EQ(scenarioId, session->getMMScenarioId());
    }


}//ns Matchmaker
}//ns BlazeServerTest