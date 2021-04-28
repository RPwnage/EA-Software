/*************************************************************************************************/
/*!
    \file mmconfigtest.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "test/matchmaker/mmconfigtest.h"
#include "test/common/testinghooks.h" // for VerifyBool
#include "test/common/blaze/mockblazemain.h" // for gMockBlazeMain
#include "test/common/blaze/mockmatchmakerslaveimpl.h"
#include "gamepacker/packer.h"

namespace BlazeServerTest
{
namespace Matchmaker
{
    
    //////////// TESTS


    //TODO: post demo, DISABLE_ or delete this test. It purposely fails with MM rule config ERRs/WARNs logged just for R and I demo.
    TEST_F(MmConfigTest, DISABLED_DEMO_BigConfigManyErrs_Log)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        // add subsession
        eastl::string templateName = getTestName();
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();
        auto subsession = addBasicSubsession(config);
        return_if_testerr();
        subsession->setCreateGameTemplate(templateName.c_str());

        // purposely omit required rules and/or settings

        helperTestConfig(config, true);
    }

    //DISABLED_ : this test is used for manually checking for warnings currently
    TEST_F(MmConfigTest, DISABLED_BigConfigSanityCheckWarns_Log)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        // add subsession
        eastl::string templateName = getTestName();
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();
        auto subsession = addBasicSubsession(config);
        return_if_testerr();
        subsession->setCreateGameTemplate(templateName.c_str());

        // update subsession having filter for sanity
        const char8_t* filterName = templateName.c_str();
        addBasicFilterDefnToConfig(config.getFilters(), filterName);
        return_if_testerr();
        subsession->getFiltersList().push_back(filterName);

        helperTestConfig(config, true);
    }

    // test: add factor used by subsession, without any rules nor filters. should pass
    TEST_F(MmConfigTest, DISABLED_FactorAndFilterNoRules_i)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        // add subsession having the factor
        eastl::string templateName = getTestName();
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();
        auto subsession = addBasicSubsession(config);
        return_if_testerr();
        subsession->setCreateGameTemplate(templateName.c_str());

        // update subsession having filter
        const char8_t* filterName = templateName.c_str();
        addBasicFilterDefnToConfig(config.getFilters(), filterName);
        return_if_testerr();
        subsession->getFiltersList().push_back(filterName);

        helperTestConfig(config, true);
    }

    // test: variant, but using global filter
    TEST_F(MmConfigTest, DISABLED_FactorAndFilterNoRules_ii)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        // add subsession having the factor
        eastl::string templateName = getTestName();
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();
        auto subsession = addBasicSubsession(config);
        return_if_testerr();
        subsession->setCreateGameTemplate(templateName.c_str());

        // update subsession having filter
        const char8_t* filterName = templateName.c_str();
        addBasicFilterDefnToConfig(config.getFilters(), filterName);
        return_if_testerr();
        config.getScenariosConfig().getGlobalFilters().push_back(filterName);

        helperTestConfig(config, true);
    }

    // test: add factor to a create game template but without any subsession actually
    // using it, without any rules nor filters.
    TEST_F(MmConfigTest, DISABLED_FactorDefinitionOnly)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;
        // To avoid Blaze failing/considering us as non-scenarios, add at least a dummy scenario
        addBasicSubsession(config);
        return_if_testerr();

        addBasicFactorToConfig(config.getCreateGameTemplatesConfig());
        return_if_testerr();

        // Don't add any subsession using the factor
        // Currently, Blaze does not validate they HAVE to be used, even if definitions exist
        helperTestConfig(config, true);
    }

    // test: add factor more than once to the config's create game template
    TEST_F(MmConfigTest, DISABLED_FactorDuplicates_i)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        eastl::string templateName = getTestName();
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();

        // add second
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();

        // for sanity add subsession using the cg template
        auto subsession = addBasicSubsession(config);
        return_if_testerr();
        subsession->setCreateGameTemplate(templateName.c_str());

        helperTestConfig(config, true);
    }

    // test: add factor many times to the config's create game template
    TEST_F(MmConfigTest, DISABLED_FactorDuplicates_ii)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        eastl::string templateName = getTestName();

        for (int8_t i = 0; i < 100; ++i)
        {
            addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
            return_if_testerr();
        }

        // for sanity add subsession using the cg template
        auto subsession = addBasicSubsession(config);
        return_if_testerr();
        subsession->setCreateGameTemplate(templateName.c_str());

        helperTestConfig(config, true);
    }

    // test: add factor used by subsession, without any rules nor filters. should pass
    TEST_F(MmConfigTest, DISABLED_FactorSubsessionOnly)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        // add subsession having the factor
        eastl::string templateName = getTestName();
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();
        auto subsession = addBasicSubsession(config);
        return_if_testerr();
        subsession->setCreateGameTemplate(templateName.c_str());

        helperTestConfig(config, true);
    }

    // test: add just the filter definition, without any actual subsession using it
    TEST_F(MmConfigTest, DISABLED_FilterDefinitionOnly)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;
        // To avoid Blaze failing/considering us as non-scenarios, add at least a dummy scenario
        addBasicSubsession(config);
        return_if_testerr();

        addBasicFilterDefnToConfig(config.getFilters());
        return_if_testerr();

        // Without any rules, filters nor factors configured, no matchmaking is possible.
        // Currently, Blaze does not validate they HAVE to be used, even if definitions exist
        helperTestConfig(config, true);
    }


   

    // test: add many (redundant) filter copies
    TEST_F(MmConfigTest, DISABLED_FilterGlobalManyCopies)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;
        addBasicSubsession(config);
        return_if_testerr();

        for (uint8_t i = 0; i < 100; ++i)
        {
            eastl::string filterName(eastl::string::CtorSprintf(), "FilterGlobalManyCopies%u", i);
            addBasicFilterDefnToConfig(config.getFilters(), filterName.c_str());
            return_if_testerr();
            config.getScenariosConfig().getGlobalFilters().push_back(filterName.c_str());
        }
        
        helperTestConfig(config, true);
    }
    // test: add global filter, without any rules nor factors. should pass
    TEST_F(MmConfigTest, DISABLED_FilterGlobalOnly)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;
        addBasicSubsession(config);
        return_if_testerr();

        addBasicFilterDefnToConfig(config.getFilters());
        return_if_testerr();

        config.getScenariosConfig().getGlobalFilters().push_back("globalTest");

        helperTestConfig(config, true);
    }

    TEST_F(MmConfigTest, DISABLED_FilterNameEmpty)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        // add subsession having the factor for sanity
        eastl::string templateName = getTestName();
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();
        auto subsession = addBasicSubsession(config);
        return_if_testerr();
        subsession->setCreateGameTemplate(templateName.c_str());

        // update subsession having filter with the name:
        const char8_t* filterName = "";
        addBasicFilterDefnToConfig(config.getFilters(), filterName);
        return_if_testerr();
        config.getScenariosConfig().getGlobalFilters().push_back(filterName);

        helperTestConfig(config, true);
    }
    //DISABLED_ since there's unrelated MM rule config ERRs
    TEST_F(MmConfigTest, DISABLED_FilterNameEmpty_Logs)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        // add subsession having the factor for sanity
        eastl::string templateName = getTestName();
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();
        auto subsession = addBasicSubsession(config);
        return_if_testerr();
        subsession->setCreateGameTemplate(templateName.c_str());

        // update subsession having filter with the name:
        const char8_t* filterName = "";
        addBasicFilterDefnToConfig(config.getFilters(), filterName);
        return_if_testerr();
        config.getScenariosConfig().getGlobalFilters().push_back(filterName);

        helperTestConfig(config, true);
    }

    // test: add filter used by subsession, without any rules nor factors. should pass
    TEST_F(MmConfigTest, DISABLED_FilterSubsessionOnly)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        const char8_t* filterName = "ConfigFilterSubsessionOnly";
        addBasicFilterDefnToConfig(config.getFilters(), filterName);
        return_if_testerr();

        // add subsession having the filter
        auto subsession = addBasicSubsession(config);
        return_if_testerr();
        subsession->getFiltersList().push_back(filterName);

        helperTestConfig(config, true);
    }

    // test: add duplicates of the same name to the filterList for a subsession
    TEST_F(MmConfigTest, DISABLED_FilterSubsessionDuplicates)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;
        auto subsession = addBasicSubsession(config);
        return_if_testerr();

        eastl::string filterName(eastl::string::CtorSprintf(), "FilterSubsessionDuplicates");
        addBasicFilterDefnToConfig(config.getFilters(), filterName.c_str());
        return_if_testerr();

        for (uint8_t i = 0; i < 100; ++i)
        {
            subsession->getFiltersList().push_back(filterName.c_str());
        }

        helperTestConfig(config, true);
    }

    // test: add many (redundant) filter copies
    TEST_F(MmConfigTest, DISABLED_FilterSubsessionManyCopies)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;
        auto subsession = addBasicSubsession(config);
        return_if_testerr();

        for (uint8_t i = 0; i < 100; ++i)
        {
            eastl::string filterName(eastl::string::CtorSprintf(), "FilterSubsessionManyCopies%u", i);
            addBasicFilterDefnToConfig(config.getFilters(), filterName.c_str());
            return_if_testerr();

            subsession->getFiltersList().push_back(filterName.c_str());
        }

        helperTestConfig(config, true);
    }

    // test: add many (redundant) filter copies
    TEST_F(MmConfigTest, DISABLED_FilterSubsessionAndGlobalManyCopies)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;
        auto subsession = addBasicSubsession(config);
        return_if_testerr();

        for (uint8_t i = 0; i < 100; ++i)
        {
            eastl::string filterName(eastl::string::CtorSprintf(), "FilterSubsessionManyCopies%u", i);
            addBasicFilterDefnToConfig(config.getFilters(), filterName.c_str());
            return_if_testerr();

            config.getScenariosConfig().getGlobalFilters().push_back(filterName.c_str());

            subsession->getFiltersList().push_back(filterName.c_str());
        }

        helperTestConfig(config, true);
    }

  

    // test: without any rules, filters, nor factors, there's no matchmaking possible. Blaze should validate false
    TEST_F(MmConfigTest, DISABLED_NoRulesFiltersNorFactors)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        // for this test, avoid being considered using non-scenarios rules, by adding at least one dummy scenario
        addBasicSubsession(config);
        return_if_testerr();

        // Currently, Blaze does not validate they HAVE to be used. Some titles may only intend to use GM for instance, but the MM component currently might still need to be present?
        helperTestConfig(config, true);
    }

    ////////////////// HELPERS

    void MmConfigTest::helperTestConfig(const Blaze::Matchmaker::MatchmakerConfig& configuration, bool expectCanConfigure, bool testCreateSession) const
    {
        ASSERT_NO_THROW(helperTestConfigInternal(configuration, expectCanConfigure, testCreateSession));
    }

    void MmConfigTest::helperTestConfigInternal(const Blaze::Matchmaker::MatchmakerConfig& configuration, bool expectCanConfigure, bool testCreateSession) const
    {
        Blaze::Matchmaker::MatchmakerConfig config;
        configuration.copyInto(config);//non const copy to compile below

        // don't activate yet, we'll explicitly call configuration methods first
        MockMatchmakerSlaveImpl mmSlaveImpl(config, false);
        ASSERT_TRUE(nullptr != mmSlaveImpl.getMatchmaker());

        // Unit test by calling the internal configure methods, normally done via bootstrap
        // When there's no filters, factors nor rules actually used, configure should fail
        Blaze::ConfigureValidationErrors validationErrors;
        bool result = mmSlaveImpl.onValidateConfigTdf(config, nullptr, validationErrors);
        if (expectCanConfigure) 
        {
            eastl::string buf;
            const char8_t* mmConfigValidationError = toString(buf, validationErrors);
            ASSERT_STREQ("", mmConfigValidationError);
        }
        ASSERT_EQ(expectCanConfigure, result);

        // sanity test can create a MM session on the config
        if (expectCanConfigure)
        {
            if (mmSlaveImpl.getState() != Blaze::ComponentStub::ACTIVE)
            {
                mmSlaveImpl.activateComponent();

                // Sanity: ensure the configure method passed (normally called in 
                // activateComponent() after onValidateConfigTdf())
                ASSERT_EQ(expectCanConfigure, mmSlaveImpl.getMatchmaker()->configure(config));
            }
            if (testCreateSession)
                helperTestCreateSession(mmSlaveImpl);
        }
    }
    void MmConfigTest::helperTestCreateSession(MockMatchmakerSlaveImpl& mmSlaveImpl) const
    {
        ASSERT_NO_THROW(
        {
            ASSERT_TRUE(nullptr != mmSlaveImpl.getMatchmaker());
            Blaze::GameManager::MatchmakingSessionId id = 123;
            Blaze::GameManager::MatchmakingScenarioId scenarioId = 321;
            Blaze::GameManager::Matchmaker::MatchmakingSession mmSession(mmSlaveImpl,
                *mmSlaveImpl.getMatchmaker(), id, scenarioId);
            EXPECT_EQ(id, mmSession.getMMSessionId());
            EXPECT_EQ(scenarioId, mmSession.getMMScenarioId());
        });
    }

    // test helper
    Blaze::GameManager::SubSessionConfig* MmConfigTest::addBasicSubsession(
        Blaze::Matchmaker::MatchmakerConfig& config, const char8_t* scenName, const char8_t* subsessionName) const
    {
        Blaze::GameManager::ScenarioName name = (scenName == nullptr ? getTestName().c_str() : scenName);
        if (has_testerr())
            return nullptr;
        if (subsessionName == nullptr)
            subsessionName = name.c_str();

        auto* scenario = config.getScenariosConfig().getScenarios().allocate_element();
        config.getScenariosConfig().getScenarios()[name.c_str()] = scenario;
        auto* subsession = scenario->getSubSessions().allocate_element();
        scenario->getSubSessions()[subsessionName] = subsession;
        return subsession;
    }
    // test helper
    Blaze::GameManager::MatchmakingFilterDefinition* MmConfigTest::addBasicFilterDefnToConfig(
        Blaze::GameManager::MatchmakingFilterMap& configMap, const char8_t* filterName) const
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
        return defn;
    }
    // test helper
    Blaze::GameManager::GameQualityFactorConfig* MmConfigTest::addBasicFactorToConfig(
        Blaze::GameManager::CreateGameTemplatesConfig& config, const char8_t* templateName)
    {
        Blaze::GameManager::TemplateName tempName =
            (templateName == nullptr ? getTestName().c_str() : templateName);
        if (has_testerr())
            return nullptr;

        auto cgTemplate = config.getCreateGameTemplates().allocate_element();
        config.getCreateGameTemplates()[tempName.c_str()] = cgTemplate;

        auto factor = cgTemplate->getPackerConfig().getQualityFactors().pull_back();
        factor->setGameProperty("game.playerCount");
        //factor->getScoreShaper().setFunction("range"); //GamePacker::Packer::gCalcShaperTypeNames[(int32_t)GamePacker::FactorShaperType::RANGE]);
        //factor->getScoreShaper().setViableValue(0);
        //factor->getScoreShaper().setMaxValue("4");
        //Possible test enhancement: to allow using game.participantCapacity, would require mock pre config of properties:
        //{ goal="maximize", gameProperty="game.playerCount", scoreShaper={ function="range", viableValue="2", maxValue="game.participantCapacity" } }
        //eastl::string maxValue(eastl::string::CtorSprintf(), "%s%s", Blaze::GameManager::GAME_PROPERTY_PREFIX, Blaze::GameManager::PROPERTY_NAME_PARTICIPANT_CAPACITY);
        //factor->getScoreShaper().setMaxValue(maxValue.c_str());

        return factor;
    }
    // test helper. add required 'classic MM' rules
    void MmConfigTest::addClassicMmRequiredRulesToConfig(Blaze::Matchmaker::MatchmakerConfig& config)
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



}//ns Matchmaker
}//ns BlazeServerTest