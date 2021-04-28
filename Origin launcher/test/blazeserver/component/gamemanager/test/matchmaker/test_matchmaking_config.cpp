/*
    (c) Electronic Arts. All Rights Reserved.
*/

#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/config/config_file.h"
#include "framework/config/config_map.h"
#include "gamemanager/matchmaker/matchmakingconfig.h"
#include "framework/util/shared/blazestring.h"
#include "framework/test/blazeunittest.h"
#include "EASTL/string.h"

using namespace Blaze::GameManager::Matchmaker;
/*! ************************************************************************************************/
/*!  A series of unit tests over the MatchmakingConfig class (reading/parsing of the matchmaker config file)
     NOTE: all matchmaker test classes have the same name so that we can minimize the number of friends
     to declare (for testing) in the actual matchmaker classes.
*************************************************************************************************/
class TestMatchmaker
{
public:


    ~TestMatchmaker()
    {
        // junk line so we can have a breakpoint, keeping the window open after all tests have run.
        int x = 3;
        x = 4;
    }

    void setUp()
    {
        Blaze::Logger::initialize();
    }
    
    void tearDown()
    {
        Blaze::Logger::destroy();
    }

    
    // we should get an error if the matchmaking config map is missing entirely, or if it's empty
    void testMissingMatchmakingConfig()
    {
        // expect error if matchmaking cfg is missing entirely
        {
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString("");
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.getErrorCount() == 1);
            assertDefaultSettings(matchmakingConfig);
            delete configMap;
        }

        // expect warning if matchmaking cfg is present, but empty
        {
            // note: the string ctor of ConfigMap expects to parse out wrapping braces...
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString("{ matchmaker = {} }");
            MatchmakingConfig matchmakingConfig(configMap);
            assertDefaultSettings(matchmakingConfig);
            BUT_ASSERT(matchmakingConfig.getErrorCount() > 0);
            delete configMap;
        }
    }

    // test idlePeriodMS (normal, too low, non-numeric)
    void testIdlePeriodValues()
    {
        uint32_t minIdlePeriod = 50;

        // expect success for normal case (>= MIN_IDLE_PERIOD_MS)
        {
            initIdlePeriodConfigString("500");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.getMatchmakerIdlePeriodMs() == 500);
            BUT_ASSERT(matchmakingConfig.getErrorCount() == 0);
            delete configMap;
        }

        // expect success for min case (== MIN_IDLE_PERIOD_MS)
        {
            initIdlePeriodConfigString("50");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.getMatchmakerIdlePeriodMs() == minIdlePeriod);
            BUT_ASSERT(matchmakingConfig.getErrorCount() == 0);
            delete configMap;
        }

        // min for 'too small' numbers (below MIN_IDLE_PERIOD_MS)
        {
            initIdlePeriodConfigString("49");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.getMatchmakerIdlePeriodMs() == minIdlePeriod);
            BUT_ASSERT(matchmakingConfig.getErrorCount() > 0);
            delete configMap;
        }

        // default for invalid input: non-numeric
        {
            initIdlePeriodConfigString("NAN");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.getMatchmakerIdlePeriodMs() == MatchmakingConfig::DEFAULT_IDLE_PERIOD_MS);
            BUT_ASSERT(matchmakingConfig.getErrorCount() > 0);
            delete configMap;
        }
    }


    // test criteriaComplexity
    void testCriteriaComplexityValues()
    {

        // expect success for normal case (>= 1)
        {
            initCriteriaComplexityConfigString("64");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.getCriteriaComplexityCap() == 64);
            BUT_ASSERT(matchmakingConfig.getErrorCount() == 0);
            delete configMap;
        }

        // expect success for min case (== 1)
        {
            initCriteriaComplexityConfigString("1");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.getCriteriaComplexityCap() == 1);
            BUT_ASSERT(matchmakingConfig.getErrorCount() == 0);
            delete configMap;
        }

        // default for 'too small' numbers (below 1)
        {
            initCriteriaComplexityConfigString("0");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.getCriteriaComplexityCap() == MatchmakingConfig::DEFAULT_CRITERIA_COMPLEXITY_CAP);
            BUT_ASSERT(matchmakingConfig.getErrorCount() > 0);
            delete configMap;
        }

        // default for invalid input: non-numeric
        {
            initCriteriaComplexityConfigString("NAN");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.getCriteriaComplexityCap() == MatchmakingConfig::DEFAULT_CRITERIA_COMPLEXITY_CAP);
            BUT_ASSERT(matchmakingConfig.getErrorCount() > 0);      
            delete configMap;
        }
    }

    // test parsing the literal value for the attribute type
    void testAttributeTypeParsing()
    {
        RuleAttributeType attribType;

        // player attrib (note: checking case sensitivity too)
        {
            initAttributeTypeString("player_ATTRIBUTE");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.parseAttributeType(configMap, attribType));
            BUT_ASSERT(attribType == PLAYER_ATTRIBUTE);
            delete configMap;
        }


        // game attrib (note: checking case sensitivity too)
        {
            initAttributeTypeString("game_ATTRIBUTE");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.parseAttributeType(configMap, attribType));
            BUT_ASSERT(attribType == GAME_ATTRIBUTE);
            delete configMap;
        }
        
        // invalid
        {
            initAttributeTypeString("invalidValue");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.parseAttributeType(configMap, attribType) == false);
            delete configMap;
        }
    }


    // test parsing the literal value for the voting method
    void testVotingMethodTypeParsing()
    {
        RuleVotingMethod votingMethod;

        // owner_wins (note: checking case sensitivity too)
        {
            initVotingMethodString("owner_WINS");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.parseVotingMethod(configMap, votingMethod));
            BUT_ASSERT(votingMethod == OWNER_WINS);
            delete configMap;
        }

        // lottery (note: checking case sensitivity too)
        {
            initVotingMethodString("vote_LOTTERY");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.parseVotingMethod(configMap, votingMethod));
            BUT_ASSERT(votingMethod == VOTE_LOTTERY);
            delete configMap;
        }

        // plurality (note: checking case sensitivity too)
        {
            initVotingMethodString("vote_PLURALITY");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.parseVotingMethod(configMap, votingMethod));
            BUT_ASSERT(votingMethod == VOTE_PLURALITY);
            delete configMap;
        }

        // invalid
        {
            initVotingMethodString("invalidValue");
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
            MatchmakingConfig matchmakingConfig(configMap);
            BUT_ASSERT(matchmakingConfig.parseVotingMethod(configMap, votingMethod) == false);
            delete configMap;
        }
    }

    // Test the parsing of an individual fitThresholdPair
    //   Since matchmaker uses its own parser for this, we have more syntax tests than normal...
    void testMinFitThresholdPairParsing()
    {
        Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString("");
        MatchmakingConfig matchmakingConfig(configMap);
        MinFitThresholdList::MinFitThresholdPair thresholdPair;
        
        // NOTE: the below function in the matchmakingconfig is refactored, need to pass in
        // the ruledefination, for now, we pass in nullptr so these test won't work anymore
    
        // test valid input ranges (time & floating value)
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  "0:0", thresholdPair));
        assertThresholdPairValue(thresholdPair, 0, 0);
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  "1:.5", thresholdPair));
        assertThresholdPairValue(thresholdPair, 1, .5f);
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  "2:0.75", thresholdPair));
        assertThresholdPairValue(thresholdPair, 2, .75f);
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  "10:1", thresholdPair));
        assertThresholdPairValue(thresholdPair, 10, 1);
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr, "0:EXACT_match_REQUIRED", thresholdPair));
        assertThresholdPairValue(thresholdPair, 0, MinFitThresholdList::EXACT_MATCH_REQUIRED);

        // test whitespace
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  " 12 : 0.88 ", thresholdPair));
        assertThresholdPairValue(thresholdPair, 12, .88f);
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr, " 0 : EXACT_match_REQUIRED ", thresholdPair));
        assertThresholdPairValue(thresholdPair, 0, MinFitThresholdList::EXACT_MATCH_REQUIRED);

        // test invalid input
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  " ", thresholdPair) == false); // neg time
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  " -1 : 0.88 ", thresholdPair) == false); // neg time
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  " 0.5 : 0.88 ", thresholdPair) == false); // float time
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  " 0 : -1 ", thresholdPair) == false); // value < 0
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  " 0 : 1.01 ", thresholdPair) == false); // value > 0
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  " 0 : 2 ", thresholdPair) == false); // value > 0
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  " 0  2 ", thresholdPair) == false); // missing ':'
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr,  " 0 : badliteral ", thresholdPair) == false); // unknown token
        BUT_ASSERT(matchmakingConfig.parseMinFitThresholdPair(nullptr, " 0 : EXACT_match_REQUIRED trailingToken ", thresholdPair) == false); // unknown trailing value
        
        delete configMap;
    }

    void testSparseFitTableParsing()
    {
        eastl::string configStr;
        configStr.reserve(4096);

        FitTableContainer ftc;

        Blaze::ConfigFile* emptyConfigMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
        MatchmakingConfig matchmakingConfig(emptyConfigMap);

        {
            // init a map with a single default rule
            const char8_t* ruleName = "testRule";
            const char8_t* possibleValues[] = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", nullptr };
            float expectedFitTable[] =
            {
                float(0.9), float(0.8), float(0.7), float(0.6), float(0.5), float(0.4), float(0.3), float(0.2), float(0.2), float(1.0),
                float(0.8), float(0.9), float(0.8), float(0.7), float(0.6), float(0.5), float(0.4), float(0.3), float(0.2), float(1.0),
                float(0.7), float(0.8), float(0.9), float(0.0), float(0.7), float(0.6), float(0.5), float(0.4), float(0.3), float(1.0),
                float(0.6), float(0.7), float(0.8), float(0.9), float(0.8), float(0.7), float(0.6), float(0.5), float(0.4), float(1.0),
                float(0.5), float(0.6), float(0.7), float(0.8), float(0.9), float(0.8), float(0.7), float(0.6), float(0.5), float(1.0),
                float(0.4), float(0.5), float(0.6), float(0.7), float(0.8), float(0.9), float(0.8), float(0.7), float(0.6), float(1.0),
                float(0.3), float(0.4), float(0.5), float(0.6), float(0.7), float(0.8), float(0.9), float(0.8), float(0.7), float(1.0),
                float(0.2), float(0.3), float(0.4), float(0.5), float(0.6), float(0.7), float(0.8), float(0.9), float(0.8), float(1.0),
                float(0.2), float(0.2), float(0.3), float(0.4), float(0.5), float(0.6), float(0.7), float(0.8), float(0.9), float(1.0),
                float(1.0), float(1.0), float(1.0), float(1.0), float(1.0), float(1.0), float(1.0), float(1.0), float(1.0), float(1.0)
            };

            configStr = "identityValue = 0.9\nbaseValue = 0.2\nstepValue = 0.1\nsparseValues = [ [9,-1,1.0],[-1,9,1.0],[3,2,0.0] ]\n";

            surroundWithSparseFitMapStr(configStr);

            surroundFitMapWithRuleStr(configStr, ruleName, possibleValues);
            surroundWithRuleMapString(configStr);

            GenericRuleDefinition* ruleDefn;

            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(configStr.c_str());
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            delete configMap;
            assertPossibleValues(ruleDefn, possibleValues, expectedFitTable);
            delete ruleDefn;
        }
    }

    void testRuleDefinitionParsing()
    {
        eastl::string configStr;
        configStr.reserve(4096);

        Blaze::ConfigFile* emptyConfigMap = Blaze::ConfigFile::createFromString(mConfigBuffer);
        MatchmakingConfig matchmakingConfig(emptyConfigMap);

        GenericRuleDefinition* ruleDefn;


        // valid case (default values)
        {
            // init a map with a single default rule
            const char8_t* ruleName = "testRule";
            addRule(configStr, ruleName);
            surroundWithRuleMapString(configStr);

            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(configStr.c_str());
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            delete configMap;
            assertDefaultRuleValues(ruleDefn, ruleName);
            delete ruleDefn;
        }

        // valid case (non-default values) -- checking that return values aren't hardcoded somehow.
        {
            // init a map with a single rule
            const char8_t* ruleName = "testRule2";
            addRule(configStr, ruleName, "otherAttrib", "PLAYER_ATTRIBUTE", "VOTE_LOTTERY", "foo,bar", "1,0,0,1", "33", "listName = [ 0:.5, 1:.25,5:0]");
            surroundWithRuleMapString(configStr);
            
            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(configStr.c_str());
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            delete configMap;
            
            // validate the non-default values:
            const char8_t* expectedPossibleValues[] = {"foo", "bar"};
            float expectedFitTable[] = {1,0,0,1};
            assertRuleValues(ruleDefn, ruleName, "otherAttrib", PLAYER_ATTRIBUTE, VOTE_LOTTERY, expectedPossibleValues, 2, expectedFitTable, 33 );
            
            // validate minFitThresholdList
            uint32_t expectedTimes[] = {0,1,5};
            float expectedFitThresholds[] = {.5,.25,0};
            assertMinFitThresholdList(ruleDefn, "listName", expectedTimes, expectedFitThresholds, 3);

            delete ruleDefn;
        }


        // invalid attrib name
        {
            // init a map with a single rule
            const char8_t* ruleName = "testRule";
            addRule(configStr, ruleName, "");
            surroundWithRuleMapString(configStr);

            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(configStr.c_str());
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            delete configMap;
            BUT_ASSERT(ruleDefn == nullptr);
        }

        // invalid attrib type
        {
            // init a map with a single rule
            const char8_t* ruleName = "testRule";
            addRule(configStr, ruleName, "attribName", "invalidAttribName");
            surroundWithRuleMapString(configStr);

            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(configStr.c_str());
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            delete configMap;
            BUT_ASSERT(ruleDefn == nullptr);
        }

        // invalid voting method
        {
            // init a map with a single rule
            const char8_t* ruleName = "testRule";
            addRule(configStr, ruleName, "attribName", "PLAYER_ATTRIBUTE", "invalidVotingMethod");
            surroundWithRuleMapString(configStr);

            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(configStr.c_str());
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            delete configMap;
            BUT_ASSERT(ruleDefn == nullptr);
        }

        // invalid possibleValues (empty)
        {
            // init a map with a single rule
            const char8_t* ruleName = "testRule";
            addRule(configStr, ruleName, "attribName", "PLAYER_ATTRIBUTE", "VOTE_PLURALITY", "" );
            surroundWithRuleMapString(configStr);

            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(configStr.c_str());
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            delete configMap;
            BUT_ASSERT(ruleDefn == nullptr);
        }

        // invalid possibleValues (duplicates & case sensitivity)
        {
            // init a map with a single rule
            const char8_t* ruleName = "testRule";
            addRule(configStr, ruleName, "attribName", "PLAYER_ATTRIBUTE", "VOTE_PLURALITY", "a,A,b" );
            surroundWithRuleMapString(configStr);

            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(configStr.c_str());
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            BUT_ASSERT(ruleDefn == nullptr);
            delete configMap;
        }


        // invalid fitTable (too many)
        {
            // init a map with a single rule
            const char8_t* ruleName = "testRule";
            addRule(configStr, ruleName, "attribName", "PLAYER_ATTRIBUTE", "VOTE_PLURALITY", "foo,bar", "1,1,1,1,1" );
            surroundWithRuleMapString(configStr);

            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(configStr.c_str());
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            delete configMap;
            BUT_ASSERT(ruleDefn == nullptr);
        }

        // invalid fitTable (too few)
        {
            // init a map with a single rule
            const char8_t* ruleName = "testRule";
            addRule(configStr, ruleName, "attribName", "PLAYER_ATTRIBUTE", "VOTE_PLURALITY", "foo,bar", "1,1,1," );
            surroundWithRuleMapString(configStr);

            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(configStr.c_str());
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            delete configMap;
            BUT_ASSERT(ruleDefn == nullptr);
        }

        /* TODO: re-enable test once bug 186 has been fixed
        // invalid weight
        {
            // init a map with a single rule
            const char8_t* ruleName = "testRule";
            addRule(configStr, ruleName, "attribName", "PLAYER_ATTRIBUTE", "VOTE_PLURALITY", "foo,bar", "1,1,1,1", "-5" );
            surroundWithRuleMapString(configStr);

            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(configStr.c_str());
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            delete configMap;
            // NOTE: invalid weights are clamped to zero
            BUT_ASSERT(ruleDefn != nullptr);
            BUT_ASSERT(ruleDefn->mWeight == 0);
            delete ruleDefn;
        }
        */


        // invalid rule (no minFitThresholdLists)
        {
            // init a map with a single rule
            const char8_t* ruleName = "testRule";
            addRule(configStr, ruleName, "attribName", "PLAYER_ATTRIBUTE", "VOTE_PLURALITY", "foo,bar", "1,1,1,1", "0", "" );
            surroundWithRuleMapString(configStr);

            Blaze::ConfigFile* configMap = Blaze::ConfigFile::createFromString(configStr.c_str());
            ruleDefn = matchmakingConfig.parseGenericRuleDefinition(ruleName, configMap);
            delete configMap;
            BUT_ASSERT(ruleDefn == nullptr);
        }

        delete emptyConfigMap;
    }

private:

    // wrap the current string in markers for a Rule Map
    //  assumes newline termination
    void surroundWithRuleMapString(eastl::string &configStr)
    {
        configStr.sprintf("{\n%s}\n", configStr.c_str());
    }

    // encapsulate the current contents of the string in a named rule
    //  assumes that the string is already newline terminated
    void surroundFitMapWithRuleStr(eastl::string &configStr, const char8_t *ruleName, const char8_t *possibleValues[])
    {
        eastl::string possValStr;
        while(*possibleValues != nullptr)
        {
            possValStr.append_sprintf("""%s""", *possibleValues);
            ++possibleValues;
            if(*possibleValues != nullptr)
                possValStr.append(",");
        }
        configStr.sprintf("%s = {\nattributeType = GAME_ATTRIBUTE\nattributeName = ""testrule""\nvotingMethod = OWNER_WINS\npossibleValues = [ %s ]\n%s}\n", ruleName, possValStr.c_str(), configStr.c_str());
    }

    // encapsulate the current contents of the string in a sparse fit map
    //  assumes that the string is already newline terminated
    void surroundWithSparseFitMapStr(eastl::string &configStr)
    {
        configStr.sprintf("sparseFitTable = {\n%s}\n", configStr.c_str());
    }

    // append a (valid by default) matchmaking rule config string to the supplied configStr
    void addRule(eastl::string &configStr, const char8_t* ruleName, const char8_t* attribName = "myAttrib", const char8_t* attribTypeStr = "GAME_ATTRIBUTE",
        const char8_t* votingMethodStr = "OWNER_WINS", const char8_t* possibleValuesStr = "a,b,c", const char8_t* fitTableStr = "1,0,0,0,1,0,0,0,1",
        const char8_t* weightStr = "100", const char8_t *thresholdList = "a = [0:.5]")
    {
        configStr.append_sprintf("%s = {\n attributeType = %s\nattributeName = \"%s\"\nvotingMethod = %s\n", ruleName, attribTypeStr, attribName, votingMethodStr);
        configStr.append_sprintf("possibleValues = [%s]\nfitTable = [ %s ]\nweight=%s\n", possibleValuesStr, fitTableStr, weightStr);
        if (thresholdList)
            configStr.append_sprintf("minFitThresholdLists = { %s }\n}\n", thresholdList);
    }

    // assert the 'default' values from addRule are set in the supplied ruleDefn.
    void assertDefaultRuleValues(GenericRuleDefinition* ruleDefn, const char8_t* expectedRuleName)
    {
        const char8_t* expectedPossibleValues[] = {"a", "b", "c"};
        float expectedFitTable[] = {1,0,0,0,1,0,0,0,1};
        assertRuleValues(ruleDefn, expectedRuleName, "myAttrib", GAME_ATTRIBUTE, OWNER_WINS, expectedPossibleValues, 3, expectedFitTable, 100);

        // minFitThresholdList
        uint32_t expectedTimes[] = {0};
        float expectedFitThresholds[] = {.5};
        assertMinFitThresholdList(ruleDefn, "a", expectedTimes, expectedFitThresholds, 1);
    }

    // assert the rule's values (note: doesn't test any minFitThreshold lists
    void assertRuleValues(GenericRuleDefinition* ruleDefn, const char8_t* expectedRuleName, const char8_t* expectedAttribName,
        RuleAttributeType expectedAttribType, RuleVotingMethod expectedVotingMethod, 
        const char8_t** expectedPossibleValues, size_t numPossibleValues, float *expectedFitTable,
        uint32_t expectedWeight)
    {
        BUT_ASSERT(ruleDefn != nullptr);
        assertRuleName(ruleDefn, expectedRuleName);
        assertAttribInfo(ruleDefn, expectedAttribName, expectedAttribType);
        BUT_ASSERT(ruleDefn->mVotingMethod == expectedVotingMethod);
        BUT_ASSERT(ruleDefn->getWeight() == expectedWeight);
        assertPossibleValues(ruleDefn, expectedPossibleValues, numPossibleValues, expectedFitTable);
    }

    void assertMinFitThresholdList(GenericRuleDefinition* ruleDefn, const char8_t *listName, 
        uint32_t* expectedTimes, float* expectedThresholds, size_t numPairs)
    {
        const MinFitThresholdList* theList= ruleDefn->getMinFitThresholdList(listName);
        BUT_ASSERT(theList != nullptr);
        BUT_ASSERT(theList->mMinThresholdList.size() == numPairs);
        for (size_t i=0; i<numPairs; ++i)
        {
            float actualThreshold = theList->getMinFitThreshold(expectedTimes[i]);
            BUT_ASSERT_DELTA(actualThreshold, expectedThresholds[i], 0.000001); // note: testing float equality w/ an epsilon value
        }
    }

    // assert that the expected possibleValue & fitTable matches what's in the rule defn.
    void assertPossibleValues(GenericRuleDefinition* ruleDefn, const char8_t** expectedPossibleValues, size_t numPossibleValues, float *expectedFitTable)
    {
        // check possibleValue array
        BUT_ASSERT(ruleDefn->mPossibleValues.size() == numPossibleValues);
        for(size_t i=0; i< numPossibleValues; ++i)
        {
            BUT_ASSERT( blaze_stricmp(ruleDefn->mPossibleValues[i], expectedPossibleValues[i]) == 0 );
        }

        // check fitTable
        for (size_t i=0; i<numPossibleValues; ++i)
        {
            for (size_t j=0; j<numPossibleValues; ++j)
            {
                float expectedFitPercent = expectedFitTable[i*numPossibleValues + j];
                BUT_ASSERT_DELTA(ruleDefn->getFitPercent(i,j), expectedFitPercent, 0.000001); // note: testing float equality w/ an epsilon valu
            }
        }
    }

    void assertPossibleValues(GenericRuleDefinition* ruleDefn, const char8_t** expectedPossibleValues, float *expectedFitTable)
    {
        size_t count = 0;
        while(expectedPossibleValues[count] != nullptr)
            count++;

        assertPossibleValues(ruleDefn, expectedPossibleValues, count, expectedFitTable);
    }

    // assert that the expected ruleName matches what's in the rule defn.
    void assertRuleName(GenericRuleDefinition* ruleDefn, const char8_t* expectedRuleName)
    {
        BUT_ASSERT( blaze_stricmp(expectedRuleName, ruleDefn->getName()) == 0 );
    }

    // assert that the expected attrib name & type matches what's in the rule defn.
    void assertAttribInfo(GenericRuleDefinition* ruleDefn, const char8_t* expectedAttribName, RuleAttributeType expectedAttribType)
    {
        BUT_ASSERT( blaze_stricmp(expectedAttribName, ruleDefn->getAttributeName()) == 0 );
        BUT_ASSERT( expectedAttribType == ruleDefn->getAttributeType() );
    }


    // assert that the supplied thresholdPair has the supplied values
    void assertThresholdPairValue(const MinFitThresholdList::MinFitThresholdPair &thresholdPair, uint32_t seconds, float threshold)
    {
        BUT_ASSERT(thresholdPair.first == seconds);
        BUT_ASSERT_DELTA(thresholdPair.second, threshold, 0.000001); // note: testing float equality w/ an epsilon value
    }

    // set mConfigBuffer to a votingMethod key & value
    void initVotingMethodString(const char8_t* votingMethodStr)
    {
        blaze_snzprintf(mConfigBuffer, sizeof(mConfigBuffer), "{ \n %s = %s\n }", MatchmakingConfig::GENERICRULE_VOTING_METHOD_KEY, votingMethodStr);
    }

    // set mConfigBuffer to an attribType key & value
    void initAttributeTypeString(const char8_t* attribTypeStr)
    {
        blaze_snzprintf(mConfigBuffer, sizeof(mConfigBuffer), "{ \n %s = %s\n }", MatchmakingConfig::GENERICRULE_ATTRIBUTE_TYPE_KEY, attribTypeStr);
    }

    // set mConfigBuffer to a matchmaker config that specifies only a criteriaComplexity key & value
    void initCriteriaComplexityConfigString(const char8_t* criteriaCapStr)
    {
        // note: the string ctor of ConfigMap expects to parse out wrapping braces...
        blaze_snzprintf(mConfigBuffer, sizeof(mConfigBuffer), "{ matchmaker = { \ncriteriaComplexityCap = %s\n } }", criteriaCapStr);
    }

    // set mConfigBuffer to a matchmaker config that specifies only an idlePeriod key & value
    void initIdlePeriodConfigString(const char8_t* idlePeriodStr)
    {
        // note: the string ctor of ConfigMap expects to parse out wrapping braces...
        blaze_snzprintf(mConfigBuffer, sizeof(mConfigBuffer), "{ matchmaker = { \nidlePeriodMS = %s\n } }", idlePeriodStr);
    }

    // test the public interfaces of the matchmaking config for their default values:
    // default idlePeriod & complexityCap; empty genericRuleList
    void assertDefaultSettings(Blaze::GameManager::Matchmaker::MatchmakingConfig &matchmakingConfig)
    {
        using namespace Blaze::GameManager::Matchmaker;
        BUT_ASSERT(matchmakingConfig.getCriteriaComplexityCap() == MatchmakingConfig::DEFAULT_CRITERIA_COMPLEXITY_CAP);
        BUT_ASSERT(matchmakingConfig.getMatchmakerIdlePeriodMs() == MatchmakingConfig::DEFAULT_IDLE_PERIOD_MS);

        // expect no rules defined by default
        GenericRuleDefinitionList ruleList;
        matchmakingConfig.initGenericRuleDefinitions(ruleList);
        BUT_ASSERT(ruleList.empty());
    }


private:
    char8_t mConfigBuffer[1024];
};


// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_MATCHMAKER_CONFIG\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestMatchmaker testMatchmaker;
    testMatchmaker.testIdlePeriodValues();
    testMatchmaker.testCriteriaComplexityValues();
    testMatchmaker.testAttributeTypeParsing();
    testMatchmaker.testVotingMethodTypeParsing();
    testMatchmaker.testMinFitThresholdPairParsing();
    testMatchmaker.testSparseFitTableParsing();
    testMatchmaker.testRuleDefinitionParsing();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

