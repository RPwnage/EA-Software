/*
    (c) Electronic Arts. All Rights Reserved.
*/

#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/config/config_file.h"
#include "gamemanager/gamemanagermasterimpl.h"
#include "gamemanager/matchmaker/matchmaker.h"
#include "gamemanager/matchmaker/rules/legacy/genericrule.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "framework/util/shared/blazestring.h"
#include "framework/test/blazeunittest.h"
#include "framework/usersessions/usersession.h"
#include "EASTL/string.h"


// NOTE: total hack to get matchmaker initialized.  This isn't safe, but it's ok since I know the functions I'm testing
//  don't call gamemanager commands.
char gFakeGameManagerBuffer[sizeof(Blaze::GameManager::GameManagerMasterImpl)];
Blaze::GameManager::GameManagerMasterImpl* gFakeGameManagerMasterImpl = (Blaze::GameManager::GameManagerMasterImpl*) &gFakeGameManagerBuffer;


char gConfigBuffer[2048] =  "{ \n"
                            "  matchmaker = { idlePeriodMS = 250\n"
                            "  criteriaComplexityCap = 32\n"
                            "  rules = {\n"
                            "     gameAttribRule = {\n"
                            "          attributeType = GAME_ATTRIBUTE\n"
                            "          attributeName = mapSize\n"
                            "          votingMethod = OWNER_WINS \n"
                            "          possibleValues = [ RAndom, Large, Medium, Small, ABstain ]\n"
                            "          fitTable = [ 1.0, 1.0, 1.0, 1.0, 1.0,\n"
                            "                       1.0, 1.0, 0.8, 0.2, 1.0,\n"
                            "                       1.0, 0.8, 1.0, 0.8, 1.0,\n"
                            "                       1.0, 0.2, 0.8, 1.0, 1.0,\n"
                            "                       1.0, 1.0, 1.0, 1.0, 1.0]\n"
                            "          weight = 100\n"
                            "          minFitThresholdLists = { \n"
                            "               minFitList1 = [ 0:.8, 5:.5, 10:0 ] \n"
                            "               minFitList2 = [ 0:0 ] \n"

                            "}}}}}\n";



using namespace Blaze::GameManager::Matchmaker;

/*! ************************************************************************************************/
/*!  A series of unit tests over the GenericRule &GenericRuleDefinition classes (reading/parsing of the matchmaker config file)
     NOTE: all matchmaker test classes have the same name so that we can minimize the number of friends
     to declare (for testing) in the actual matchmaker classes.
*************************************************************************************************/
class TestMatchmaker
{
public:

    Matchmaker mMatchmaker;

    TestMatchmaker() :
        mMatchmaker( *gFakeGameManagerMasterImpl )
    {
    }

    ~TestMatchmaker()
    {

        // NOTE: we leak the logger & config map intentionally, since freeing crashes us
    }

    // test initializing a rule, and ensure that the rule's values are what we expect
    //  checks case sensitivity of strings (ruleName, ruleValues, thresholdListNames)
    //  tests a variety of expected failures (notARule, notAFitList, invalidDesiredValue, noDesiredValues)
    void testRuleInit()
    {

        // init a completely valid rule (single desired value).  Note: we're also checking case-sensitivity of names
        {
            GenericRule *rule;
            const char8_t* desiredValues[] = {"SMALL"};
            size_t numDesiredValues = 1;
            BUT_ASSERT( initRule(rule, "gameAttribRULE", "minFitLIST1", desiredValues, numDesiredValues) );
            validateGameAttribRule(*rule); // note: function only validates this specific rule (gameAttribRule)
        }

        // init random
        {
            GenericRule *rule;
            const char8_t* desiredValues[] = {"random"};
            size_t numDesiredValues = 1;
            BUT_ASSERT( initRule(rule, "gameAttribRULE", "minFitLIST1", desiredValues, numDesiredValues) );
            BUT_ASSERT( rule->mDesiredValuesBitset.count() == 1 );
            BUT_ASSERT( rule->mDesiredValuesBitset.test(0) );
        }

        // init abstain
        {
            GenericRule *rule;
            const char8_t* desiredValues[] = {"abstain"};
            size_t numDesiredValues = 1;
            BUT_ASSERT( initRule(rule, "gameAttribRULE", "minFitLIST1", desiredValues, numDesiredValues) );
            BUT_ASSERT( rule->mDesiredValuesBitset.count() == 1 );
            BUT_ASSERT( rule->mDesiredValuesBitset.test(4) );
        }



        //////////////////////////////////////////////////////////////////////////
        // Expected failure cases
        //////////////////////////////////////////////////////////////////////////

        // err: load a non-existent rule
        {
            GenericRule *rule;
            const char8_t* desiredValues[] = {"SMALL"};
            size_t numDesiredValues = 1;
            BUT_ASSERT( ! initRule(rule, "notARule", "minFitLIST1", desiredValues, numDesiredValues) );
        }

        // err: load an existing rule with a non-existent minFit
        {
            GenericRule *rule;
            const char8_t* desiredValues[] = {"SMALL"};
            size_t numDesiredValues = 1;
            BUT_ASSERT( ! initRule(rule, "gameAttribRule", "notAFitList", desiredValues, numDesiredValues) );
        }

        // err: load a valid rule with invalid desired values
        {
            GenericRule *rule;
            const char8_t* invalidDesiredValues[] = {"SMALL", "bogusValue"};
            size_t numDesiredValues = 2;
            BUT_ASSERT( ! initRule(rule, "gameAttribRule", "minFitLIST1", invalidDesiredValues, numDesiredValues) );
        }

        // err: load a valid rule with no desired values.
        {
            GenericRule *rule;
            BUT_ASSERT( ! initRule(rule, "gameAttribRule", "minFitLIST1", nullptr, 0) );
        }

        // err: load a valid rule with duplicate desired values.
        {
            GenericRule *rule;
            const char8_t* desiredValues[] = {"SMALL", "medium", "large", "large"};
            size_t numDesiredValues = 4;
            BUT_ASSERT( !initRule(rule, "gameAttribRULE", "minFitLIST1", desiredValues, numDesiredValues) );
        }


        // err: load a rule the desires both abstain and another rule
        {
            GenericRule *rule;
            const char8_t* desiredValues[] = {"medium", "small", "large", "abstain"};
            size_t numDesiredValues = 4;
            BUT_ASSERT( ! initRule(rule, "gameAttribRULE", "minFitLIST1", desiredValues, numDesiredValues) );
        }

        // err: load a rule the desires both random and another rule
        {
            GenericRule *rule;
            const char8_t* desiredValues[] = {"medium", "random"};
            size_t numDesiredValues = 2;
            BUT_ASSERT( ! initRule(rule, "gameAttribRULE", "minFitLIST1", desiredValues, numDesiredValues) );
        }
    }


    // full validation that the supplied rule matches what we defined for gameAttribRule in the config above
    // Note: values are hardcoded in both the config string above, as well as the function body below.
    void validateGameAttribRule(const GenericRule& rule)
    {
        validateGameAttribRuleDefn(rule.getDefinition());
        BUT_ASSERT( blaze_stricmp(rule.mMinFitThresholdList->getName(), "minFitlist1") == 0 );

        //check the rule's desired bitset
        BUT_ASSERT(rule.mDesiredValuesBitset.count() == 1);
        BUT_ASSERT(rule.mDesiredValuesBitset.test(3)); // small is desired value, and is index 3.
    }

    void validateGameAttribRuleDefn(const GenericRuleDefinition& ruleDefn)
    {
        const double epsilon = 0.000001; // for comparing floats

        // check rule definition values (all but possible values & fitTable)
        BUT_ASSERT(blaze_stricmp(ruleDefn.getName(), "gameAttribRule") == 0);
        BUT_ASSERT(blaze_stricmp(ruleDefn.getAttributeName(), "mapSize") == 0);
        BUT_ASSERT(ruleDefn.getAttributeType() == GAME_ATTRIBUTE);
        BUT_ASSERT(ruleDefn.getVotingMethod() == OWNER_WINS);
        BUT_ASSERT(ruleDefn.getWeight() == 100);
        BUT_ASSERT(ruleDefn.getMaxPossibleFitScore() == 100);

        // definition's possible values
        char8_t* expectedPossibleValues[] = {"random", "large", "medium", "small", "abstain"}; // note: case is different from config string
        BUT_ASSERT(ruleDefn.getPossibleValueCount() == 5);
        for(size_t i=0; i<5; ++i)
        {
            BUT_ASSERT(blaze_stricmp(ruleDefn.getPossibleValue(i), expectedPossibleValues[i]) == 0);
        }

        // definition's possibleRandomValues
        char8_t* expectedPossibleRandomValues[] = {"large", "medium", "small"}; // note: case is different from config string
        BUT_ASSERT(ruleDefn.mPossibleActualValues.size() == 3);
        for(size_t i=0; i<3; ++i)
        {
            BUT_ASSERT(blaze_stricmp(ruleDefn.mPossibleActualValues[i], expectedPossibleRandomValues[i]) == 0);
        }

        // definition's fitTable
        for( size_t row = 0; row<5; ++row )
        {
            for( size_t col = 0; col<5; ++col )
            {
                float actualFitPercent = ruleDefn.getFitPercent(row, col);
                float expectedFitPercent = getExpectedGameAttribRuleFitPercent(row, col);
                BUT_ASSERT_DELTA(expectedFitPercent, actualFitPercent, epsilon); // note: testing float equality w/ an epsilon value
            }
        }

        // fitThresholdLists
        // minFitList1
        {
            const MinFitThresholdList* thresholdList = ruleDefn.getMinFitThresholdList("minfitlist1");
            BUT_ASSERT(thresholdList != nullptr);
            // verify step function
            BUT_ASSERT_DELTA(thresholdList->getMinFitThreshold(0), .8, epsilon); // note: testing float equality w/ an epsilon value
            BUT_ASSERT_DELTA(thresholdList->getMinFitThreshold(4), .8, epsilon); // note: testing float equality w/ an epsilon value
            BUT_ASSERT_DELTA(thresholdList->getMinFitThreshold(5), .5, epsilon); // note: testing float equality w/ an epsilon value
            BUT_ASSERT_DELTA(thresholdList->getMinFitThreshold(9), .5, epsilon); // note: testing float equality w/ an epsilon value
            BUT_ASSERT_DELTA(thresholdList->getMinFitThreshold(10), 0, epsilon); // note: testing float equality w/ an epsilon value
            BUT_ASSERT_DELTA(thresholdList->getMinFitThreshold(11), 0, epsilon); // note: testing float equality w/ an epsilon value
        }

        // minFitList2
        {
            const MinFitThresholdList* thresholdList = ruleDefn.getMinFitThresholdList("minfitlist2");
            BUT_ASSERT(thresholdList != nullptr);
            // verify step function
            BUT_ASSERT_DELTA(thresholdList->getMinFitThreshold(0), 0, epsilon); // note: testing float equality w/ an epsilon value
            BUT_ASSERT_DELTA(thresholdList->getMinFitThreshold(10), 0, epsilon); // note: testing float equality w/ an epsilon value
        }
    }

    // get the expected fit table value for the "gameAttributeRule" defined in the config above
    float getExpectedGameAttribRuleFitPercent(size_t row, size_t col)
    {
        // all combinations containing random or abstain have fitScore 1.0
        if ( (row == 0) || (row == 4) || (col == 0) || (col == 4) || (row == col)) // anything including random or abstain, or equal
            return 1.0f;

        //  large/med  and med/small each have a score of .8
        if ( row == 2 || col == 2 )
            return .8f;

        //  large/small has .2, however.
        return .2f;
    }




    void testBasicVoting()
    {
        GenericRule *largeRule;
        const char8_t* desiredValues[] = {"large"};
        size_t numDesiredValues = 1;
        BUT_ASSERT( initRule(largeRule, "gameAttribRule", "minFitList1", desiredValues, numDesiredValues) );

        MatchmakingSessionList emptyMemberList;
        const char8_t *winningValue;

        // test degenerate case (single voter, single value)
        {
            const char8_t* expectedValue = desiredValues[0];
            winningValue = largeRule->voteOwnerWins();
            BUT_ASSERT(blaze_stricmp(expectedValue, winningValue) == 0);

            winningValue = largeRule->voteLottery(emptyMemberList);
            BUT_ASSERT(blaze_stricmp(expectedValue, winningValue) == 0);

            winningValue = largeRule->votePlurality(emptyMemberList);
            BUT_ASSERT(blaze_stricmp(expectedValue, winningValue) == 0);
        }

        // test normal case (multiple voters, same value)
        {
            MatchmakingSessionList gameMembers;
            const char8_t* desiredValues[] = {"large"};
            size_t numDesiredValues = 1;
            addVotingMember(gameMembers, desiredValues, numDesiredValues);

            // all 3 methods should return large, since everyone's voting for it...
            const char8_t* expectedValue = desiredValues[0];

            winningValue = largeRule->voteOwnerWins();
            BUT_ASSERT(blaze_stricmp(expectedValue, winningValue) == 0);

            winningValue = largeRule->voteLottery(gameMembers);
            BUT_ASSERT(blaze_stricmp(expectedValue, winningValue) == 0);

            winningValue = largeRule->votePlurality(gameMembers);
            BUT_ASSERT(blaze_stricmp(expectedValue, winningValue) == 0);
        }
    }

    // Note: the 3 voting methods will happily return "abstain", if abstain is the winner.  Another code path translates abstain/random into an actual value.
    void testAbstainVoting()
    {
        GenericRule *largeRule;
        const char8_t* desiredValues[] = {"abstain"};
        size_t numDesiredValues = 1;
        BUT_ASSERT( initRule(largeRule, "gameAttribRule", "minFitList1", desiredValues, numDesiredValues) );

        MatchmakingSessionList emptyMemberList;
        const char8_t *winningValue;


        // verify that abstain votes aren't tallied
        // NOTE: another code path is responsible for translating abstain/random into a random value
        {
            MatchmakingSessionList gameMembers;
            const char8_t* desiredValueLarge[] = {"large"};
            addVotingMember(gameMembers, desiredValueLarge, 1);
            const char8_t* desiredValueAbstain[] = {"abstain"};
            addVotingMember(gameMembers, desiredValueAbstain, 1);
            addVotingMember(gameMembers, desiredValues, numDesiredValues);

            // note: owner wins for votes [abstain, large, abstain] -> abstain
            winningValue = largeRule->voteOwnerWins();
            BUT_ASSERT(blaze_stricmp("abstain", winningValue) == 0);

            // note: lottery for votes [abstain, large, abstain] -> large
            for(size_t i=0; i<10; ++i)
            {
                winningValue = largeRule->voteLottery(gameMembers);
                BUT_ASSERT( (blaze_stricmp("large", winningValue) == 0) );
            }

            // note: plurality for votes [abstain, large, abstain] -> large
            winningValue = largeRule->votePlurality(gameMembers);
            BUT_ASSERT(blaze_stricmp("large", winningValue) == 0);
        }
    }


    // multiple values
    // all abstain
    // all random
    // ties


    void addVotingMember(MatchmakingSessionList& memberSessions, const char8_t** desiredValues, size_t numDesiredValues)
    {
        // NOTE: changes have been made for matchmaking session constructor and initialization. this test won't
        // work properly anymore
        Blaze::UserSession fakeUserSession;
        MatchmakingSession *session = BLAZE_NEW MatchmakingSession(*gFakeGameManagerMasterImpl, mMatchmaker, 1, fakeUserSession);
        memberSessions.push_back(session);

        Blaze::GameManager::MatchmakingCriteriaError err;
        Blaze::GameManager::StartMatchmakingRequest startRequest;
        Blaze::GameManager::GenericRulePrefs* genericRulePrefs = BLAZE_NEW Blaze::GameManager::GenericRulePrefs;
        DedicatedServerOverrideMap dedicatedServerOverrideMap;
        startRequest.getSessionMode().setCreateGame();

        startRequest.getCriteriaData().getGameSizeRulePrefs().setMinPlayerCount(1);
        startRequest.getCriteriaData().getGameSizeRulePrefs().setDesiredPlayerCount(1);
        startRequest.getCriteriaData().getGameSizeRulePrefs().setMaxPlayerCapacity(10);
        startRequest.getCriteriaData().getGameSizeRulePrefs().setMinFitThresholdName("exactMatch");

        startRequest.getCriteriaData().getGenericRulePrefsList().push_back( genericRulePrefs );
        initGenericRuleSettings(*genericRulePrefs, "gameAttribRule", "minFitList1", desiredValues, numDesiredValues);
        MatchmakingSupplementalData matchmakingSupplementalData;
        BUT_ASSERT(session->initialize(startRequest, matchmakingSupplementalData, err, 32, dedicatedServerOverrideMap));
    }

    bool initRule(GenericRule *&rule, const char8_t *ruleName, const char8_t *minFitListName, const char8_t **desiredValues, size_t numDesiredValues)
    {
        Blaze::GameManager::MatchmakingCriteriaError err;
        Blaze::GameManager::GenericRulePrefs genericRuleSettings;
        initGenericRuleSettings(genericRuleSettings, ruleName, minFitListName, desiredValues, numDesiredValues);

        const GenericRuleDefinition* ruleDef =
                mMatchmaker.getMatchmakingRuleDefinitionCollection()->
                    getGenericRuleDefinition(genericRuleSettings.getRuleName());

        MatchmakingSupplementalData matchmakingSupplementalData;
        Blaze::GameManager::MatchmakingAsyncStatus matchmakingAsyncStatus;
        rule = BLAZE_NEW GenericRule(*ruleDef, matchmakingAsyncStatus);
        if (!rule->initialize(genericRuleSettings, matchmakingSupplementalData, err))
        {
            delete rule;
            rule = nullptr;
        }

        return rule != nullptr;        
    }

    void initGenericRuleSettings(Blaze::GameManager::GenericRulePrefs &genericRuleSettings,
        const char8_t *ruleName, const char8_t *minFitListName, const char8_t **desiredValues, size_t numDesiredValues)
    {
        genericRuleSettings.setRuleName(ruleName);
        genericRuleSettings.setMinFitThresholdName(minFitListName);
        for(size_t i=0; i<numDesiredValues; ++i)
        {
            genericRuleSettings.getDesiredValues().push_back( Blaze::GameManager::GenericRuleValue(desiredValues[i]) );
        }
    }


    void setUp()
    {
        Blaze::Logger::initialize();

        Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(gConfigBuffer);
        mMatchmaker.configure(configMap);
        delete configMap;
    }

    void tearDown()
    {
    }
};



// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_MATCHMAKER_GENERICRULE\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestMatchmaker testMatchmaker;
    testMatchmaker.testRuleInit();
    testMatchmaker.testBasicVoting();
    testMatchmaker.testAbstainVoting();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

