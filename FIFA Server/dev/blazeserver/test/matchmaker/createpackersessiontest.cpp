/*************************************************************************************************/
/*!
    \file createpackersessiontest.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "matchmaker/createpackersessiontest.h"
#include "test/common/blaze/mockmatchmakerslaveimpl.h"
#include "gamepacker/packer.h"

#include "framework/usersessions/usersessionmanager.h" // gUserSessionManager

namespace BlazeServerTest
{
namespace Matchmaker
{

    CreatePackerSessionTest::CreatePackerSessionTest()
    {}
    
    CreatePackerSessionTest::~CreatePackerSessionTest()
    {}

    //////////// TESTS

    // test: add factor used by subsession, without any rules nor filters. should pass
    TEST_F(CreatePackerSessionTest, DISABLED_FactorAndFilterNoRules_i)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        // add subsession having the factor
        eastl::string templateName = getTestName();
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();
        auto subsessionConfig = addSubsessionConfig(config);
        return_if_testerr();
        subsessionConfig->setCreateGameTemplate(templateName.c_str());

        // update subsession having filter
        const char8_t* filterName = templateName.c_str();
        addBasicFilterDefnToConfig(config.getFilters(), filterName);
        return_if_testerr();
        subsessionConfig->getFiltersList().push_back(filterName);

        auto mmSession = testConstructSession(config);
        return_if_testerr();
        ASSERT_TRUE(mmSession != nullptr);
    }

    // test: variant, but using global filter
    TEST_F(CreatePackerSessionTest, DISABLED_FactorAndFilterNoRules_ii)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        // add subsession having the factor
        eastl::string templateName = getTestName();
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();
        auto subsessionConfig = addSubsessionConfig(config);
        return_if_testerr();
        subsessionConfig->setCreateGameTemplate(templateName.c_str());

        // update subsession having filter
        const char8_t* filterName = templateName.c_str();
        addBasicFilterDefnToConfig(config.getFilters(), filterName);
        return_if_testerr();
        config.getScenariosConfig().getGlobalFilters().push_back(filterName);

        auto mmSession = testConstructSession(config);
        return_if_testerr();
        ASSERT_TRUE(mmSession != nullptr);
    }

    // test: add factor more than once to the config's create game template
    TEST_F(CreatePackerSessionTest, DISABLED_FactorDuplicates_i)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        eastl::string templateName = getTestName();
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();

        // add second
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();

        // for sanity add subsession using the cg template
        auto subsessionConfig = addSubsessionConfig(config);
        return_if_testerr();
        auto mmSession = testConstructSession(config);
        subsessionConfig->setCreateGameTemplate(templateName.c_str());

        return_if_testerr();
        ASSERT_TRUE(mmSession != nullptr);
    }

    // test: add factor many times to the config's create game template
    TEST_F(CreatePackerSessionTest, DISABLED_FactorDuplicates_ii)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        eastl::string templateName = getTestName();

        for (int8_t i = 0; i < 100; ++i)
        {
            addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
            return_if_testerr();
        }

        // for sanity add subsession using the cg template
        auto subsessionConfig = addSubsessionConfig(config);
        return_if_testerr();
        subsessionConfig->setCreateGameTemplate(templateName.c_str());

        auto mmSession = testConstructSession(config);
        return_if_testerr();
        ASSERT_TRUE(mmSession != nullptr);

    }

    // test: add factor used by subsession, without any rules nor filters. should pass
    TEST_F(CreatePackerSessionTest, DISABLED_FactorSubsessionOnly)// DISABLED_ need fix GOS-31491 in ml (from f-mm-modernization)
    {
        Blaze::Matchmaker::MatchmakerConfig config;

        // add subsession having the factor
        eastl::string templateName = getTestName();
        addBasicFactorToConfig(config.getCreateGameTemplatesConfig(), templateName.c_str());
        return_if_testerr();

        eastl::string scenName = getTestName();
        eastl::string subsessName = getTestName();
        Blaze::GameManager::MatchmakingSessionMode mode;
        mode.setCreateGame(true);
        auto subsessionConfig = addSubsessionConfig(config, scenName.c_str(), subsessName.c_str(), &mode);
        return_if_testerr();
        //auto scenarioConfig = config.getScenariosConfig().getScenarios()[scenName.c_str()];

        subsessionConfig->setCreateGameTemplate(templateName.c_str());

        auto mmSession = testConstructSession(config);
        return_if_testerr();
        ASSERT_TRUE(mmSession != nullptr);

        /* The below sample shows how we *could* go further, mocking Blaze internal flow. But, to avoid complexity/dependencies/maintenance,
            we decided Unit Tests won't go that far (leave that up to regression tests). No need currently fixing any of below etc.

        // fake logging in
        ASSERT_TRUE(Blaze::gUserSessionManager != nullptr);
        auto userSessionMap = const_cast<Blaze::UserSessionManager::UserSessionPtrByIdMap&>(Blaze::gUserSessionManager->getUserSessionPtrByIdMap());
        Blaze::UserSessionId userSessionId = 123;
        auto ret = userSessionMap.insert(userSessionId);
        Blaze::UserSession userSess(nullptr);//todo fill in existence data
        ret.second = &userSess;
        
        // setup the request as MM is expected to internally, before sending to game packer etc.
        Blaze::GameManager::StartMatchmakingInternalRequest masterRequest;
        masterRequest.setCreateGameTemplateName(subsessionConfig->getCreateGameTemplate());
        ASSERT_STREQ(templateName.c_str(), masterRequest.getCreateGameTemplateName());//sanity
        masterRequest.getRequest().getSessionData().getSessionMode().setBits(subsessionConfig->getMatchmakingSettings().getSessionMode().getBits());
        ASSERT_EQ(mode.getBits(), masterRequest.getRequest().getSessionData().getSessionMode().getBits());
        
        //// add the filters for this subsession//TODO: only the filter session gets this
        //masterRequest.getMatchmakingFilters().getMatchmakingFilterMap().clear();
        //for (auto& matchmakingFilterListItr : config.getScenariosConfig().getGlobalFilters())
        //{
        //    masterRequest.getMatchmakingFilters().getMatchmakingFilterMap()[matchmakingFilterListItr.first] = masterRequest.getMatchmakingFilters().getMatchmakingFilterMap().allocate_element();
        //    Blaze::GameManager::MatchmakingFilterDefinitionPtr matchmakingFilterDefinition = masterRequest.getMatchmakingFilters().getMatchmakingFilterMap()[matchmakingFilterListItr.first];
        //    matchmakingFilterListItr.second->copyInto(*matchmakingFilterDefinition);
        //}

        //auto scenMap = config.getScenariosConfig().getScenarios()[scenName.c_str()];
        //scenMap->get .mScenarioFilterMap.copyInto(masterRequest.getMatchmakingFilters().getMatchmakingFilterMap());
        //mConfigInfo.mPropertyNameMembersMap.copyInto(masterRequest.getMatchmakingFilters().getPropertyNameMembersMap());
        //mScenarioOwner.mManager.getMatchmakingFiltersUtil().getGameSessionProperties().copyInto(masterRequest.getMatchmakingFilters().getSampleGameProperties());

        // We update the subsessions names for each user:
        Blaze::GameManager::UserJoinInfo* userInfo = masterRequest.getUsersInfo().pull_back();
        userInfo->getScenarioInfo().setScenarioName(scenName.c_str());
        userInfo->getScenarioInfo().setSubSessionName(subsessName.c_str());
        userInfo->getUser().setSessionId(userSessionId);


        Blaze::GameManager::MatchmakingCriteriaError err;
        Blaze::GameManager::Matchmaker::DedicatedServerOverrideMap dedicatedServerOverrideMap;
        bool mmInitializeResult = mmSession->initialize(masterRequest, err, dedicatedServerOverrideMap);
        ASSERT_STREQ("", err.getErrMessage());
        ASSERT_TRUE(mmInitializeResult);
        */
    }



}//ns Matchmaker
}//ns BlazeServerTest