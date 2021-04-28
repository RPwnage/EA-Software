/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/config/config_sequence.h"
#include "matchmakerrulesutil.h"
#include "matchmakermodule.h"
#include "framework/util/random.h" // for Blaze::Random in setGameNameRulePreferences()
#include <stdio.h> // for FILE ops for game name rule


#define SET_SCENARIO_ATTRIBUTE(scenarioReq, attributeName, value) \
    if (scenarioReq.getScenarioAttributes()[attributeName] == nullptr) \
    { \
        scenarioReq.getScenarioAttributes()[attributeName] = scenarioReq.getScenarioAttributes().allocate_element(); \
    } \
    scenarioReq.getScenarioAttributes()[attributeName]->set(value);

namespace Blaze
{
namespace Stress
{
/*************************************************************************************************/
/*
    MatchmakerRulesUtil methods
    For setting up matchmaker rules criteria, for Matchmaker or GameBrowser modules
*/
/*************************************************************************************************/


MatchmakerRulesUtil::MatchmakerRulesUtil()
{
}

MatchmakerRulesUtil::~MatchmakerRulesUtil()
{
}

/*! ************************************************************************************************/
/*! \brief parses the stress config for MM rules, and initializes the appropriate member data
*************************************************************************************************/
bool MatchmakerRulesUtil::parseRules(const ConfigMap& config)
{
    if (!parsePredefinedRules(config))
    {
        return false;
    }

    if (!parseRoleSettings(config))
    {
        return false;
    }

    if (!parseGenericRules(config))
    {
        return false;
    }

    if (!parseUEDRules(config))
    {
        return false;
    }

    return true;
}

bool MatchmakerRulesUtil::parseRoleSettings( const ConfigMap& config )
{
    blaze_strnzcpy(mPredefinedRoleInformation.sMultiRoleCriteria, config.getString("multiRoleEntryCriteria", ""), 64);
    if (mPredefinedRoleInformation.sMultiRoleCriteria[0] != '\0')
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseRoleSettings: multiRoleEntryCriteria= " << mPredefinedRoleInformation.sMultiRoleCriteria );
    mPredefinedRoleInformation.sTotalChance = 0;

    const ConfigSequence* roleSettingsSequence = config.getSequence("roleSettings");
    if (roleSettingsSequence == nullptr)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseRoleSettings: no roleSettings configured");
    }
    else
    {
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseRoleSettings: roleSettings found.");
        while (roleSettingsSequence->hasNext())
        {
            const ConfigMap* roleSettingsMap = roleSettingsSequence->nextMap();
            if (roleSettingsMap != nullptr)
            {
                RoleSettings curSettings;

                blaze_strnzcpy(curSettings.sRoleName, roleSettingsMap->getString("rolename", ""), 64);
                curSettings.sCapacity = roleSettingsMap->getUInt16("capacity", 0);
                curSettings.sChance = roleSettingsMap->getUInt16("chance", 0);
                blaze_strnzcpy(curSettings.sRoleEntryCriteria, roleSettingsMap->getString("roleEntryCriteria", ""), 64);

                mPredefinedRoleInformation.sRoleSettings.push_back(curSettings); 

                mPredefinedRoleInformation.sTotalChance += curSettings.sChance;

                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseRoleSettings: Added role " << curSettings.sRoleName << " with capacity=" 
                                << curSettings.sCapacity << ", chance=" << curSettings.sChance << " criteria=" << curSettings.sRoleEntryCriteria );
            }
        }
    }

    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseRoleSettings: totalRoleChance= " << mPredefinedRoleInformation.sTotalChance );

    return true;
}

bool MatchmakerRulesUtil::parsePredefinedRules( const ConfigMap &config )
{
    // game topology configuration
    mPredefinedRuleConfig.sGameNetworkTopology = CLIENT_SERVER_PEER_HOSTED; // default "safe" value for stress tester
    char8_t topologyBuf[128];
    GameNetworkTopology parsedTopology;
    blaze_strnzcpy(topologyBuf, config.getString("gameNetworkTopology", ""), 128);
    if(ParseGameNetworkTopology(topologyBuf, parsedTopology))
    {
        mPredefinedRuleConfig.sGameNetworkTopology = parsedTopology;
    }

    // game protocol version string configuration
    blaze_strnzcpy(mPredefinedRuleConfig.sGameProtocolVersionString, config.getString("gameProtocolVersionString", ""), 64);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: gameProtocolVersionString = " << mPredefinedRuleConfig.sGameProtocolVersionString);
    mPredefinedRuleConfig.sUseRandomizedGameProtocolVersionString = config.getBool("useRandomizedGameProtocolVersionString", false);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: useRandomizedGameProtocolVersionString = " << (mPredefinedRuleConfig.sUseRandomizedGameProtocolVersionString ? "true" : "false"));

    // game and team size rule configuration
    if(!parseSizeRules(config))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: problem parsing size rules.");
        return false;
    }

    // dnf rule configuration
    mPredefinedRuleConfig.sMaxDNFValue = config.getInt32("maxDNFValue", 101);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: maxDNFValue = " << mPredefinedRuleConfig.sMaxDNFValue);

    // skill rule configuration
    blaze_strnzcpy(mPredefinedRuleConfig.sSkillRuleFitThresholdName, config.getString("skillRuleFitThresholdName", ""), Blaze::GameManager::MAX_MINFITTHRESHOLDNAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: skillRuleFitThresholdName = " << mPredefinedRuleConfig.sSkillRuleFitThresholdName);
    mPredefinedRuleConfig.sSkillRuleMinimumSkill = config.getInt32("minimumSkill", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: minimumSkill = " << mPredefinedRuleConfig.sSkillRuleMinimumSkill);
    mPredefinedRuleConfig.sSkillRuleMaximumSkill = config.getInt32("maximumSkill", 1000);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: maximumSkill = " << mPredefinedRuleConfig.sSkillRuleMaximumSkill);
    mPredefinedRuleConfig.sUseSkillValueOverrride = config.getBool("userSkillValueOverrride", true);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: useSkillValueOverride = " << (mPredefinedRuleConfig.sUseSkillValueOverrride ? "true" : "false"));
    blaze_strnzcpy(mPredefinedRuleConfig.sSkillRuleName, config.getString("skillRuleName", ""), Blaze::GameManager::MAX_RULENAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: skillRuleName = " << mPredefinedRuleConfig.sSkillRuleName);


    // ranked game rule configuration
    blaze_strnzcpy(mPredefinedRuleConfig.sRankedGameFitThresholdName, config.getString("rankedGameFitThresholdName", ""), Blaze::GameManager::MAX_MINFITTHRESHOLDNAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: rankedGameFitThresholdName = " << mPredefinedRuleConfig.sRankedGameFitThresholdName);
    mPredefinedRuleConfig.sRankedGameRuleUnrankedFrequency = config.getUInt16("unrankedFrequency", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: unrankedFrequency = " << mPredefinedRuleConfig.sRankedGameRuleUnrankedFrequency);
    mPredefinedRuleConfig.sRankedGameRuleRankedFrequency = config.getUInt16("rankedFrequency", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: rankedFrequency = " << mPredefinedRuleConfig.sRankedGameRuleRankedFrequency);
    mPredefinedRuleConfig.sRankedGameRuleRandomFrequency = config.getUInt16("randomFrequency", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: randomFrequency = " << mPredefinedRuleConfig.sRankedGameRuleRandomFrequency);
    mPredefinedRuleConfig.sRankedGameRuleAbstainFrequency = config.getUInt16("abstainFrequency", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: abstainFrequency = " << mPredefinedRuleConfig.sRankedGameRuleAbstainFrequency);
    
    // host balancing rule configuration
    blaze_strnzcpy(mPredefinedRuleConfig.sHostBalancingFitThresholdName, config.getString("hostBalancingFitThresholdName", ""), Blaze::GameManager::MAX_MINFITTHRESHOLDNAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: hostBalancingFitThresholdName = " << mPredefinedRuleConfig.sHostBalancingFitThresholdName);

    // host viability rule configuration
    blaze_strnzcpy(mPredefinedRuleConfig.sHostViabilityFitThresholdName, config.getString("hostViabilityFitThresholdName", ""), Blaze::GameManager::MAX_MINFITTHRESHOLDNAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: hostViabilityFitThresholdName = " << mPredefinedRuleConfig.sHostViabilityFitThresholdName);

    // ping site rule configuration
    blaze_strnzcpy(mPredefinedRuleConfig.sPingSiteRangeOffsetListName, config.getString("pingSiteRangeOffsetListName", ""), Blaze::GameManager::MAX_MINFITTHRESHOLDNAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: pingSiteRangeOffsetListName = " << mPredefinedRuleConfig.sPingSiteRangeOffsetListName);

    // geolocation rule configuration
    blaze_strnzcpy(mPredefinedRuleConfig.sGeolocationFitThresholdName, config.getString("geoLocationFitThresholdName", ""), Blaze::GameManager::MAX_MINFITTHRESHOLDNAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: geoLocationFitThresholdName = " << mPredefinedRuleConfig.sGeolocationFitThresholdName);    

    mPredefinedRuleConfig.sUseGameModRule = config.getBool("useGameModRule", false);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: useGameModRule = " << mPredefinedRuleConfig.sUseGameModRule);    
    mPredefinedRuleConfig.sMaxModRegisterValue = config.getUInt32("maxModRegisterValue", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: maxModRegisterValue = " << mPredefinedRuleConfig.sMaxModRegisterValue);

    // avoid/prefer rules:
    mPredefinedRuleConfig.sMinAvoidPlayerCount = config.getInt32("minAvoidPlayerCount", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: minAvoidPlayerCount = " << mPredefinedRuleConfig.sMinAvoidPlayerCount);    
    mPredefinedRuleConfig.sMaxAvoidPlayerCount = config.getInt32("maxAvoidPlayerCount", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: maxAvoidPlayerCount = " << mPredefinedRuleConfig.sMaxAvoidPlayerCount);    
    mPredefinedRuleConfig.sMinPreferPlayerCount = config.getInt32("minPreferPlayerCount", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: minPreferPlayerCount = " << mPredefinedRuleConfig.sMinPreferPlayerCount);    
    mPredefinedRuleConfig.sMaxPreferPlayerCount = config.getInt32("maxPreferPlayerCount", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules: maxPreferPlayerCount = " << mPredefinedRuleConfig.sMaxPreferPlayerCount);    


    // gamename rule configuration
    if (!parseGameNameRuleConfig(config))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePredefinedRules:: problem parsing game name rule. To disable GameNameRule stress, see stress .cfg");
        return false;
    }

    return true;

}

bool MatchmakerRulesUtil::parseUEDRules(const ConfigMap& config)
{
    const Blaze::ConfigMap *uedRulesMap = config.getMap("uedRules");

    while ((uedRulesMap != nullptr) && uedRulesMap->hasNext())
    {
        const char8_t* ruleName = uedRulesMap->nextKey();
        const Blaze::ConfigMap *ruleMap = uedRulesMap->getMap(ruleName);
        if (ruleMap == nullptr)
            continue;

        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseUEDRules: Configuring UED rule " << ruleName << ".");
        mUEDRuleDefinitions.push_back();
        UEDRuleDefintion& newRule = mUEDRuleDefinitions.back();

        newRule.mRuleName = ruleName;
        newRule.mRuleFreq = ruleMap->getUInt16("ruleFrequency", 0);

        const Blaze::ConfigMap *fitThresholdsMap = ruleMap->getMap("minFitThresholdValues");
        while(fitThresholdsMap->hasNext())
        {
            const char8_t* thresholdName = fitThresholdsMap->nextKey();
            newRule.mThresholds.push_back();
            GenericRuleFitThreshold& threshold = newRule.mThresholds.back();
            threshold.sThresholdName = thresholdName;
            threshold.sThresholdFreq = fitThresholdsMap->getUInt16(thresholdName, 0);
        }
    }

    return true;
}

bool MatchmakerRulesUtil::parseGenericRules(const ConfigMap& config)
{
    const Blaze::ConfigMap *genericRulesMap = config.getMap("genericRules");

    while((genericRulesMap != nullptr) && genericRulesMap->hasNext())
    {
        const char8_t* ruleName = genericRulesMap->nextKey();
        const Blaze::ConfigMap *genericRuleMap = genericRulesMap->getMap(ruleName);

        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGenericRules: Configuring generic rule " << ruleName << ".");
        mGenericRuleDefinitions.push_back();
        GenericRuleDefinition& newRule = mGenericRuleDefinitions.back();
        newRule.sIsPlayerAttribRule = false;
        newRule.sRuleName = ruleName;
        newRule.sRuleFreq = genericRuleMap->getUInt16("ruleFrequency", 0);
        const Blaze::ConfigMap *fitThresholdsMap = genericRuleMap->getMap("minFitThresholdValues");
        while(fitThresholdsMap->hasNext())
        {
            const char8_t* thresholdName = fitThresholdsMap->nextKey();
            newRule.sThresholds.push_back();
            GenericRuleFitThreshold& threshold = newRule.sThresholds.back();
            threshold.sThresholdName = thresholdName;
            threshold.sThresholdFreq = fitThresholdsMap->getUInt16(thresholdName, 0);
        }

        const Blaze::ConfigSequence *valueFrequenciesSeq = genericRuleMap->getSequence("valueFrequencies");
        if(valueFrequenciesSeq != nullptr)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGenericRules: " << ruleName << ".valueFrequencies = ");
            while(valueFrequenciesSeq->hasNext())
            {
                const Blaze::ConfigMap* valueFrequencyPair = valueFrequenciesSeq->nextMap(); // get next {value=x,frequency=y}
                newRule.sRulePossibleValues.push_back();
                GenericRulesPossibleValues& possibleValue = newRule.sRulePossibleValues.back();
                const char8_t* ruleValue = valueFrequencyPair->getString("value", "");
                possibleValue.sValueList.push_back(ruleValue);
                possibleValue.sValueFreq = valueFrequencyPair->getUInt16("frequency", 0);
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGenericRules: {value = " << ruleValue << ", frequency = " << possibleValue.sValueFreq << "}");
                if ( ruleValue[0] == '\0' || ! valueFrequencyPair->isDefined("frequency") ) // warn if no value or frequency
                {
                    BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGenericRules: " << ruleName << ".valueFrequencies: item parsed was missing '"
                                   << (ruleValue[0] == '\0'? "value" : "frequency") << "'");
                }
            }
        }
        
        const Blaze::ConfigSequence *playerAttributeFrequenciesSeq = genericRuleMap->getSequence("playerAttributeFrequencies");
        if(playerAttributeFrequenciesSeq != nullptr)
        {
            BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGenericRules: " << ruleName << ".playerAttributeFrequencies = ");
            newRule.sIsPlayerAttribRule = true;
            newRule.sAttribName = genericRuleMap->getString("playerAttributeName", "");
            while(playerAttributeFrequenciesSeq->hasNext())
            {
                const Blaze::ConfigMap* valueFrequencyPair = playerAttributeFrequenciesSeq->nextMap(); // get next {value=x,frequency=y}
                newRule.sAttributePossibleValues.push_back();
                GenericRulesPossibleValues& possibleAttribValue = newRule.sAttributePossibleValues.back();
                const char8_t* attribValue = valueFrequencyPair->getString("value", "");
                possibleAttribValue.sValueList.push_back(attribValue);
                possibleAttribValue.sValueFreq = valueFrequencyPair->getUInt16("frequency", 0);
                BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGenericRules: {value = " << attribValue << ", frequency = " << possibleAttribValue.sValueFreq << "}");
                if ( attribValue[0] == '\0' || ! valueFrequencyPair->isDefined("frequency") ) // warn if no value or frequency
                {
                    BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGenericRules: " << ruleName << ".playerAttributeFrequencies: item parsed was missing '" 
                                   << (attribValue[0] == '\0'? "value" : "frequency") << "'");
                }
            }
        }
        else // game attribute
        {
            newRule.sIsPlayerAttribRule = false;
            newRule.sAttribName = genericRuleMap->getString("gameAttributeName", "");
        }
    }
    return true;
}

bool MatchmakerRulesUtil::parseSizeRules( const ConfigMap& config )
{
    char8_t sizeRuleBuf[128];
    blaze_strnzcpy(sizeRuleBuf, config.getString("sizeRule", "GAME_SIZE_RULE"), 128);
    SizeRuleType parsedSizeRuleType;
    if(!parseSizeRuleType(sizeRuleBuf, parsedSizeRuleType))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseSizeRules: Unkown size rule type, '" << parsedSizeRuleType << "'");
        return false;
    }

    // we always need to parse the team count.
    mPredefinedRuleConfig.sTeamCount = config.getUInt16("teamCount", 1);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseSizeRules: teamCount = " << mPredefinedRuleConfig.sTeamCount);

    mPredefinedRuleConfig.sSizeRuleType = parsedSizeRuleType;

    switch(mPredefinedRuleConfig.sSizeRuleType)
    {
    case PLAYER_SLOTS_RULES:
        {
            return parseAllPlayerSlotRules(config);
        }
    case TEAM_SIZE_RULES:
        {
            return parseTeamSizeRules(config);
        }
    case TEAM_SIZE_AND_PLAYER_SLOT_RULES:
        {
            return parseTeamSizeAndPlayerSlotRulesCombination(config);
        }
    default:
        return false;
    }
}

bool MatchmakerRulesUtil::parseSizeRuleType(const char8_t* sizeRuleString, SizeRuleType &parsedSizeRuleType) const
{
    if(blaze_strcmp("PLAYER_SLOTS_RULES", sizeRuleString) == 0)
    {
        parsedSizeRuleType = PLAYER_SLOTS_RULES;
        return true;
    }

    if(blaze_strcmp("TEAM_SIZE_RULES", sizeRuleString) == 0)
    {
        parsedSizeRuleType = TEAM_SIZE_RULES;
        return true;
    }

    if(blaze_strcmp("TEAM_SIZE_AND_PLAYER_SLOT_RULES", sizeRuleString) == 0)
    {
        parsedSizeRuleType = TEAM_SIZE_AND_PLAYER_SLOT_RULES;
        return true;
    }

    return false;
}
bool MatchmakerRulesUtil::parseOldTeamSizeRule( const ConfigMap& config )
{
    // team size rule config
    blaze_strnzcpy(mPredefinedRuleConfig.sTeamSizeRangeListName, config.getString("teamSizeRangeListName", ""), 32);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseOldTeamSizeRule: teamSizeRangeListName = " << mPredefinedRuleConfig.sTeamSizeRangeListName);
    return parseNewAndOldTeamSizeRulesCommon(config);
}

bool MatchmakerRulesUtil::parseTeamSizeRules( const ConfigMap& config )
{
    // TeamUEDBalanceRule. Note: see UEDUtil for methods of setting the session UED values.
    blaze_strnzcpy(mPredefinedRuleConfig.sTeamUEDBalanceRuleName, config.getString("teamUEDBalanceRuleName", ""), MAX_RULENAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseOldTeamSizeRule: teamUEDBalanceRuleName = " << mPredefinedRuleConfig.sTeamUEDBalanceRuleName);    
    blaze_strnzcpy(mPredefinedRuleConfig.sTeamUEDBalanceRuleRangeListName, config.getString("teamUEDBalanceRuleRangeListName", ""), MAX_MINFITTHRESHOLDNAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseOldTeamSizeRule: teamUEDBalanceRuleRangeListName = " << mPredefinedRuleConfig.sTeamUEDBalanceRuleRangeListName);    

    // TeamUEDPositionParityRule. Note: see UEDUtil for methods of setting the session UED values.
    blaze_strnzcpy(mPredefinedRuleConfig.sTeamUEDPositionParityRuleName, config.getString("teamUEDPositionParityRuleName", ""), MAX_RULENAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseOldTeamSizeRule: teamUEDPositionParityRuleName = " << mPredefinedRuleConfig.sTeamUEDPositionParityRuleName);    
    blaze_strnzcpy(mPredefinedRuleConfig.sTeamUEDPositionParityRuleRangeListName, config.getString("teamUEDPositionParityRuleRangeListName", ""), MAX_MINFITTHRESHOLDNAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseOldTeamSizeRule: teamUEDPositionParityRuleRangeListName = " << mPredefinedRuleConfig.sTeamUEDPositionParityRuleRangeListName);

    // TeamCompositionRule
    blaze_strnzcpy(mPredefinedRuleConfig.sTeamCompositionRuleName, config.getString("TeamCompositionRuleName", ""), MAX_RULENAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseOldTeamSizeRule: TeamCompositionRuleName = " << mPredefinedRuleConfig.sTeamCompositionRuleName);    
    blaze_strnzcpy(mPredefinedRuleConfig.sTeamCompositionRuleMinFitListName, config.getString("TeamCompositionRuleMinFitListName", ""), MAX_MINFITTHRESHOLDNAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseOldTeamSizeRule: TeamCompositionRuleMinFitListName = " << mPredefinedRuleConfig.sTeamCompositionRuleMinFitListName);

    // new team size rules configs
    blaze_strnzcpy(mPredefinedRuleConfig.sTeamMinSizeRangeListName, config.getString("teamMinSizeRangeListName", ""), MAX_MINFITTHRESHOLDNAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseOldTeamSizeRule: teamMinSizeRangeListName = " << mPredefinedRuleConfig.sTeamMinSizeRangeListName);
    blaze_strnzcpy(mPredefinedRuleConfig.sTeamBalanceRangeListName, config.getString("teamBalanceRangeListName", ""), MAX_MINFITTHRESHOLDNAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseOldTeamSizeRule: teamBalanceRangeListName = " << mPredefinedRuleConfig.sTeamBalanceRangeListName);
    return parseNewAndOldTeamSizeRulesCommon(config);
}

bool MatchmakerRulesUtil::parseNewAndOldTeamSizeRulesCommon( const ConfigMap& config )
{
    mPredefinedRuleConfig.sMinTeamSize = config.getUInt16("minTeamSize", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseNewAndOldTeamSizeRulesCommon: minTeamSize = " << mPredefinedRuleConfig.sMinTeamSize);
    mPredefinedRuleConfig.sMaxTeamSize = config.getUInt16("maxTeamSize", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseNewAndOldTeamSizeRulesCommon: maxTeamSize = " << mPredefinedRuleConfig.sMaxTeamSize);
    mPredefinedRuleConfig.sDesiredTeamSize = config.getUInt16("desiredTeamSize", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseNewAndOldTeamSizeRulesCommon: desiredTeamSize = " << mPredefinedRuleConfig.sDesiredTeamSize);
    mPredefinedRuleConfig.sAllowDuplicateTeamIds = config.getBool("allowDuplicateTeamIds", true);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseNewAndOldTeamSizeRulesCommon: allowDuplicateTeamIds = " << ((mPredefinedRuleConfig.sAllowDuplicateTeamIds == true)? "true":"false"));
    mPredefinedRuleConfig.sMaxTeamSizeDifference = config.getUInt16("maxTeamSizeDifference", mPredefinedRuleConfig.sMaxTeamSize);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseNewAndOldTeamSizeRulesCommon: maxTeamSizeDifference = " << mPredefinedRuleConfig.sMaxTeamSizeDifference);
    // team count is parsed in elsewhere, since every MM request may need the team count, not just teams-specific ones.

    //TODO: need to read in the team lists

    const Blaze::ConfigSequence *joiningTeamSequence = config.getSequence("joiningTeamIdPossibleValues");
    while((joiningTeamSequence != nullptr) && joiningTeamSequence->hasNext())
    {
        const char8_t* joiningTeamIdString = joiningTeamSequence->nextString("ANY_TEAM_ID");
        if(blaze_strcmp("ANY_TEAM_ID", joiningTeamIdString) == 0)
        {
            mPredefinedRuleConfig.sJoiningTeamIdPossibleValues.push_back(ANY_TEAM_ID);
        }
        else
        {
            Blaze::GameManager::TeamId joiningTeamId;
            blaze_str2int(joiningTeamIdString, &joiningTeamId);
            mPredefinedRuleConfig.sJoiningTeamIdPossibleValues.push_back(joiningTeamId);
        }
    }

    const Blaze::ConfigSequence *opponentTeamSequence = config.getSequence("requestedTeamIdPossibleValues");
    while((opponentTeamSequence != nullptr) && opponentTeamSequence->hasNext())
    {
        const char8_t* opponentTeamIdString = opponentTeamSequence->nextString("ANY_TEAM_ID");
        if(blaze_strcmp("ANY_TEAM_ID", opponentTeamIdString) == 0)
        {
            mPredefinedRuleConfig.sRequestedTeamIdPossibleValues.push_back(ANY_TEAM_ID);
        }
        else
        {
            Blaze::GameManager::TeamId opponentTeamId;
            blaze_str2int(opponentTeamIdString, &opponentTeamId);
            mPredefinedRuleConfig.sRequestedTeamIdPossibleValues.push_back(opponentTeamId);
        }
    }

    return true;
}

bool MatchmakerRulesUtil::parseGameSizeRule( const ConfigMap& config )
{
    // game size version rule config
    blaze_strnzcpy(mPredefinedRuleConfig.sGameSizeMinFitThresholdName, config.getString("gameSizeMinFitThresholdName", ""), 32);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameSizeRule: gameSizeMinFitThresholdName = " << mPredefinedRuleConfig.sGameSizeMinFitThresholdName);
    mPredefinedRuleConfig.sMinPlayerCount = config.getUInt16("minPlayerCount", 2);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameSizeRule: minPlayerCount = " << mPredefinedRuleConfig.sMinPlayerCount);
    mPredefinedRuleConfig.sMinPlayerCountSeed = config.getUInt16("minPlayerCountSeed", 4);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameSizeRule: minPlayerCountSeed = " << mPredefinedRuleConfig.sMinPlayerCountSeed);
    mPredefinedRuleConfig.sMaxPlayerCount = config.getUInt16("maxPlayerCount", 1);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameSizeRule: maxPlayerCount = " << mPredefinedRuleConfig.sMaxPlayerCount);
    mPredefinedRuleConfig.sMaxPlayerCountSeed = config.getUInt16("maxPlayerCountSeed", 10);
    if (mPredefinedRuleConfig.sMaxPlayerCountSeed <= 0)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameSizeRule: sMaxPlayerCountSeed has to be >= 0");
        return false;
    }

    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameSizeRule: maxPlayerCountSeed = " << mPredefinedRuleConfig.sMaxPlayerCountSeed);
    mPredefinedRuleConfig.sDesiredGameSizePercent = config.getUInt16("desiredGameSizePercent", 100);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameSizeRule: desiredGameSizePercent = " << mPredefinedRuleConfig.sDesiredGameSizePercent);
    mPredefinedRuleConfig.sIsSingleGroupMatch = config.getBool("gameSizeIsSingleGroupMatch", false);
    return true;
}

bool MatchmakerRulesUtil::parseGameAndTeamSizeRuleCombination( const ConfigMap& config )
{
    if(!parseOldTeamSizeRule(config))
    {
        return false;
    }

    blaze_strnzcpy(mPredefinedRuleConfig.sGameSizeMinFitThresholdName, config.getString("gameSizeMinFitThresholdName", ""), 32);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameAndTeamSizeRuleCombination: gameSizeMinFitThresholdName = " << mPredefinedRuleConfig.sGameSizeMinFitThresholdName);

    mPredefinedRuleConfig.sMaxPlayerCount = mPredefinedRuleConfig.sMaxTeamSize * mPredefinedRuleConfig.sTeamCount;
    mPredefinedRuleConfig.sMinPlayerCount = mPredefinedRuleConfig.sMinTeamSize * mPredefinedRuleConfig.sTeamCount;
    if(mPredefinedRuleConfig.sMinPlayerCount < 1)
        mPredefinedRuleConfig.sMinPlayerCount = 1;
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameAndTeamSizeRuleCombination: minPlayerCount = " << mPredefinedRuleConfig.sMinPlayerCount);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameAndTeamSizeRuleCombination: maxPlayerCount = " << mPredefinedRuleConfig.sMaxPlayerCount);
    mPredefinedRuleConfig.sDesiredGameSizePercent = mPredefinedRuleConfig.sDesiredTeamSize * mPredefinedRuleConfig.sTeamCount;
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameAndTeamSizeRuleCombination: desiredGameSize = "<< mPredefinedRuleConfig.sDesiredGameSizePercent);
    mPredefinedRuleConfig.sIsSingleGroupMatch = config.getBool("gameSizeIsSingleGroupMatch", false);
    return true;
}


bool MatchmakerRulesUtil::parseAllPlayerSlotRules( const ConfigMap& config )
{
    if (!parsePlayerCountRule(config))
        return false;
    if (!parseTotalPlayerSlotsRule(config))
        return false;
    if (!parsePlayerSlotUtilizationRule(config))
        return false;
    if (!parseFreePlayerSlotsRule(config))
        return false;
    return true;
}

bool MatchmakerRulesUtil::parsePlayerCountRule( const ConfigMap& config )
{
    blaze_strnzcpy(mPredefinedRuleConfig.sPlayerCountRuleRangeList, config.getString("playerCountRuleRangeList", ""), 32);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerCountRule: playerCountRuleRangeList = " << mPredefinedRuleConfig.sPlayerCountRuleRangeList);
    mPredefinedRuleConfig.sMinValuePlayerCountRuleLowerBound = config.getUInt16("minValuePlayerCountRuleLowerBound", 1);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerCountRule: minValuePlayerCountRuleLowerBound = " << mPredefinedRuleConfig.sMinValuePlayerCountRuleLowerBound);

    mPredefinedRuleConfig.sMinValuePlayerCountRuleSeed = config.getUInt16("minValuePlayerCountRuleSeed", 4);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerCountRule: minValuePlayerCountRuleSeed = " << mPredefinedRuleConfig.sMinValuePlayerCountRuleSeed);
    
    mPredefinedRuleConfig.sMaxValuePlayerCountRuleLowerBound = config.getUInt16("maxValuePlayerCountRuleLowerBound", 1);
    if (mPredefinedRuleConfig.sMaxValuePlayerCountRuleLowerBound < mPredefinedRuleConfig.sMinValuePlayerCountRuleLowerBound)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerCountRule: maxValuePlayerCountRuleLowerBound has to be >= minValuePlayerCountRuleLowerBound");
        return false;
    }
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerCountRule: maxValuePlayerCountRuleLowerBound = " << mPredefinedRuleConfig.sMaxValuePlayerCountRuleLowerBound);
    
    mPredefinedRuleConfig.sMaxValuePlayerCountRuleSeed = config.getUInt16("maxValuePlayerCountRuleSeed", 32);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerCountRule: maxValuePlayerCountRuleSeed = " << mPredefinedRuleConfig.sMaxValuePlayerCountRuleSeed);

    mPredefinedRuleConfig.sDesiredValuePlayerCountRulePercent = config.getUInt16("desiredValuePlayerCountRulePercent", 100);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerCountRule: desiredValuePlayerCountRulePercent = " << mPredefinedRuleConfig.sDesiredValuePlayerCountRulePercent);
    return true;
}

bool MatchmakerRulesUtil::parseTotalPlayerSlotsRule( const ConfigMap& config )
{
    blaze_strnzcpy(mPredefinedRuleConfig.sTotalPlayerSlotsRuleRangeList, config.getString("totalPlayerSlotsRuleRangeList", ""), 32);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseTotalPlayerSlotsRule: totalPlayerSlotsRuleRangeList = " << mPredefinedRuleConfig.sTotalPlayerSlotsRuleRangeList);
    
    mPredefinedRuleConfig.sMinTotalPlayerSlotsLowerBound = config.getUInt16("minTotalPlayerSlotsLowerBound", 2);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseTotalPlayerSlotsRule: minTotalPlayerSlotsLowerBound = " << mPredefinedRuleConfig.sMinTotalPlayerSlotsLowerBound);

    mPredefinedRuleConfig.sMinTotalPlayerSlotsSeed = config.getUInt16("minTotalPlayerSlotsSeed", 8);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseTotalPlayerSlotsRule: minTotalPlayerSlotsSeed = " << mPredefinedRuleConfig.sMinTotalPlayerSlotsSeed);
    
    mPredefinedRuleConfig.sMaxTotalPlayerSlotsLowerBound = config.getUInt16("maxTotalPlayerSlotsLowerBound", 2);
    if (mPredefinedRuleConfig.sMaxTotalPlayerSlotsLowerBound < mPredefinedRuleConfig.sMinTotalPlayerSlotsLowerBound)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseTotalPlayerSlotsRule: maxTotalPlayerSlotsLowerBound has to be >= minTotalPlayerSlotsLowerBound");
        return false;
    }
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseTotalPlayerSlotsRule: maxTotalPlayerSlotsLowerBound = " << mPredefinedRuleConfig.sMaxTotalPlayerSlotsLowerBound);
    
    mPredefinedRuleConfig.sMaxTotalPlayerSlotsSeed = config.getUInt16("maxTotalPlayerSlotsSeed", 62);//for 2-64 player games like BF4
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseTotalPlayerSlotsRule: maxTotalPlayerSlotsSeed = " << mPredefinedRuleConfig.sMaxTotalPlayerSlotsSeed);

    mPredefinedRuleConfig.sDesiredTotalPlayerSlotsPercent = config.getUInt16("desiredTotalPlayerSlotsPercent", 100);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseTotalPlayerSlotsRule: desiredTotalPlayerSlotsPercent = " << mPredefinedRuleConfig.sDesiredTotalPlayerSlotsPercent);

    return true;
}

bool MatchmakerRulesUtil::parsePlayerSlotUtilizationRule( const ConfigMap& config )
{
    blaze_strnzcpy(mPredefinedRuleConfig.sPlayerSlotUtilizationRangeList, config.getString("playerSlotUtilizationRangeList", ""), 32);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerSlotUtilizationRule: playerSlotUtilizationRangeList = " << mPredefinedRuleConfig.sPlayerSlotUtilizationRangeList);
    mPredefinedRuleConfig.sMinPlayerSlotUtilizationLowerBound = config.getUInt8("minPlayerSlotUtilizationLowerBound", 1);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerSlotUtilizationRule: minPlayerSlotUtilizationLowerBound = " << mPredefinedRuleConfig.sMinPlayerSlotUtilizationLowerBound);

    mPredefinedRuleConfig.sMinPlayerSlotUtilizationSeed = config.getUInt8("minPlayerSlotUtilizationSeed", 80);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerSlotUtilizationRule: minPlayerSlotUtilizationSeed = " << mPredefinedRuleConfig.sMinPlayerSlotUtilizationSeed);

    mPredefinedRuleConfig.sMaxPlayerSlotUtilizationLowerBound = config.getUInt8("maxPlayerSlotUtilizationLowerBound", 35);
    if (mPredefinedRuleConfig.sMaxPlayerSlotUtilizationLowerBound < mPredefinedRuleConfig.sMinPlayerSlotUtilizationLowerBound)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerSlotUtilizationRule: maxPlayerSlotUtilizationLowerBound has to be >= minPlayerSlotUtilizationLowerBound");
        return false;
    }
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerSlotUtilizationRule: maxPlayerSlotUtilizationLowerBound = " << mPredefinedRuleConfig.sMaxPlayerSlotUtilizationLowerBound);

    mPredefinedRuleConfig.sMaxPlayerSlotUtilizationSeed = config.getUInt8("maxPlayerSlotUtilizationSeed", 65);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerSlotUtilizationRule: maxPlayerSlotUtilizationSeed = " << mPredefinedRuleConfig.sMaxPlayerSlotUtilizationSeed);

    mPredefinedRuleConfig.sDesiredPlayerSlotUtilizationPercent = (uint8_t)config.getUInt16("desiredplayerSlotUtilizationPercent", 100);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parsePlayerSlotUtilizationRule: desiredplayerSlotUtilizationPercent = " << mPredefinedRuleConfig.sDesiredPlayerSlotUtilizationPercent);

    return true;
}

bool MatchmakerRulesUtil::parseFreePlayerSlotsRule( const ConfigMap& config )
{
    mPredefinedRuleConfig.sMinFreePlayerSlotsLowerBound = config.getUInt16("minFreePlayerSlotsLowerBound", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseFreePlayerSlotsRule: minFreePlayerSlotsLowerBound = " << mPredefinedRuleConfig.sMinFreePlayerSlotsLowerBound);

    mPredefinedRuleConfig.sMinFreePlayerSlotsSeed = config.getUInt16("minFreePlayerSlotsSeed", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseFreePlayerSlotsRule: minFreePlayerSlotsSeed = " << mPredefinedRuleConfig.sMinFreePlayerSlotsSeed);

    mPredefinedRuleConfig.sMaxFreePlayerSlotsLowerBound = config.getUInt16("maxFreePlayerSlotsLowerBound", 0);
    if (mPredefinedRuleConfig.sMaxFreePlayerSlotsLowerBound < mPredefinedRuleConfig.sMinFreePlayerSlotsLowerBound)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseFreePlayerSlotsRule: maxFreePlayerSlotsLowerBound has to be >= minFreePlayerSlotsLowerBound");
        return false;
    }
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseFreePlayerSlotsRule: maxFreePlayerSlotsLowerBound = " << mPredefinedRuleConfig.sMaxFreePlayerSlotsLowerBound);

    mPredefinedRuleConfig.sMaxFreePlayerSlotsSeed = config.getUInt16("maxFreePlayerSlotsSeed", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseFreePlayerSlotsRule: maxFreePlayerSlotsSeed = " << mPredefinedRuleConfig.sMaxFreePlayerSlotsSeed);

    return true;
}

bool MatchmakerRulesUtil::parseGameSizeAndAllPlayerSlotRulesCombination( const ConfigMap& config )
{
    if (!parseGameSizeRule(config))
        return false;

    if (!parseAllPlayerSlotRules(config))
        return false;

    if ((mPredefinedRuleConfig.sPlayerCountRuleRangeList[0] != '\0') && (mPredefinedRuleConfig.sGameSizeMinFitThresholdName[0] != '\0'))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameSizeAndAllPlayerSlotRulesCombination: PlayerCountRule and deprecated GameSizeRule are not enabled together.");
        return false;
    }

    if ((mPredefinedRuleConfig.sTotalPlayerSlotsRuleRangeList[0] != '\0') && (mPredefinedRuleConfig.sGameSizeMinFitThresholdName[0] != '\0'))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameSizeAndAllPlayerSlotRulesCombination: TotalPlayerSlotsRule and deprecated GameSizeRule are not enabled together.");
        return false;
    }

    return true;
}

bool MatchmakerRulesUtil::parseTeamSizeAndPlayerSlotRulesCombination( const ConfigMap& config )
{
    if (!parseTeamSizeRules(config))
        return false;
    if (!parseAllPlayerSlotRules(config))
        return false;
    return true;
}

bool MatchmakerRulesUtil::parseGameNameRuleConfig( const ConfigMap& config )
{
    // gameNameRuleFrequency is percent chance use rule. If non-0, game-creates also name via settings.
    mPredefinedRuleConfig.sGameNameRuleFrequency = config.getUInt16("gameNameRuleFrequency", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: gameNameRuleFrequency = " << mPredefinedRuleConfig.sGameNameRuleFrequency << "(0 disables rule for all clients)");
    const bool ruleEnabled = (mPredefinedRuleConfig.sGameNameRuleFrequency > 0);

    // gameNameRule lengths of searches/names
    mPredefinedRuleConfig.sGameNameRuleMinLength = eastl::max(config.getUInt32("gameNameRuleMinLength", 1), (uint32_t)1);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: gameNameRuleMinLength = " << mPredefinedRuleConfig.sGameNameRuleMinLength);
    mPredefinedRuleConfig.sGameNameRuleMaxLengthForCreate = eastl::min(config.getUInt32("gameNameRuleMaxLengthForCreate", 0), Blaze::GameManager::MAX_GAMENAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: gameNameRuleMaxLengthForCreate = " << mPredefinedRuleConfig.sGameNameRuleMaxLengthForCreate);
    mPredefinedRuleConfig.sGameNameRuleMaxLengthForSearch = eastl::min(config.getUInt32("gameNameRuleMaxLengthForSearch", 0), Blaze::GameManager::MAX_GAMENAME_LEN);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: gameNameRuleMaxLengthForSearch = " << mPredefinedRuleConfig.sGameNameRuleMaxLengthForSearch);
    // validate the lengths
    if (ruleEnabled && (mPredefinedRuleConfig.sGameNameRuleMaxLengthForCreate < mPredefinedRuleConfig.sGameNameRuleMinLength))
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: Possible problem parsing game name rule: gameNameRuleMinLength " << mPredefinedRuleConfig.sGameNameRuleMinLength << " is greater than gameNameRuleMaxLengthForCreate " << mPredefinedRuleConfig.sGameNameRuleMaxLengthForCreate);
        return false;
    }
    if (ruleEnabled && (mPredefinedRuleConfig.sGameNameRuleMaxLengthForSearch < mPredefinedRuleConfig.sGameNameRuleMinLength))
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: Possible problem parsing game name rule: gameNameRuleMinLength " << mPredefinedRuleConfig.sGameNameRuleMinLength << " is greater than gameNameRuleMaxLengthForSearch " << mPredefinedRuleConfig.sGameNameRuleMaxLengthForSearch);
        return false;
    }
    

    // gameNameRuleRandomizeFrequency is percent chance search/name random generated (vs. pick from file)
    mPredefinedRuleConfig.sGameNameRuleRandomizeFrequency = config.getUInt16("gameNameRuleRandomizeFrequency", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: gameNameRuleRandomizeFrequency = " << mPredefinedRuleConfig.sGameNameRuleRandomizeFrequency);
    // gameNameRuleRandomizeCharset is set from which a randomly generated search/name's chars picked
    blaze_strnzcpy(mPredefinedRuleConfig.sGameNameRuleRandomizeCharset, config.getString("gameNameRuleRandomizeCharset", ""), 256);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: gameNameRuleRandomizeCharset = " << mPredefinedRuleConfig.sGameNameRuleRandomizeCharset);

    // gameNameRuleDataStringsMaxToUse use from 1st up to this line from file (for scaling match chance with users and reusing file)
    mPredefinedRuleConfig.sGameNameRuleDataStringsMaxToUse = config.getUInt16("gameNameRuleDataStringsMaxToUse", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: gameNameRuleDataStringsMaxToUse = " << mPredefinedRuleConfig.sGameNameRuleDataStringsMaxToUse);
    // gameNameRuleDataStringsObserveOrderInFile whether next pick follows order in file(vs randomly)(help avoid dupes). Separate tracking for searches/names.
    mPredefinedRuleConfig.sGameNameRuleDataStringsObserveOrderInFile = config.getBool("gameNameRuleDataStringsObserveOrderInFile", true);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: gameNameRuleDataStringsObserveOrderInFile = " << mPredefinedRuleConfig.sGameNameRuleDataStringsObserveOrderInFile);
    // gameNameRuleDataStrings data file with possible game names/searches (line-delimited)
    const char8_t* file = config.getString("gameNameRuleDataStrings", "");
    if (file[0] != '\0')
    {
        FILE *fp = fopen(file, "r");
        if (fp == nullptr)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: Possible problem in game name rule config: cannot open file " << file);
            return false;
        }
        uint16_t totalRead = 0;
        char8_t nextValue[Blaze::GameManager::MAX_GAMENAME_LEN];
        while((totalRead < mPredefinedRuleConfig.sGameNameRuleDataStringsMaxToUse) &&
            (fgets(nextValue, sizeof(nextValue), fp) != nullptr))
        {
            // for convenience, skip names already known too short pre-normalize
            if ((nextValue[0] != '\0') && (strlen(nextValue) >= mPredefinedRuleConfig.sGameNameRuleMinLength))
            {
                mPredefinedRuleConfig.sGameNameRuleDataStrings.push_back(nextValue);
                totalRead++;
            }
        }
        fclose(fp);
    }
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: gameNameRuleDataStrings loaded " << mPredefinedRuleConfig.sGameNameRuleDataStrings.size() << " strings.");


    // gameNameRuleInjectSubstringsFrequency is percent chance use 'gameNameRuleInjectSubstrings'
    mPredefinedRuleConfig.sGameNameRuleInjectSubstringsFrequency = config.getUInt16("gameNameRuleInjectSubstringsFrequency", 0);
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: gameNameRuleInjectSubstringsFrequency = " << mPredefinedRuleConfig.sGameNameRuleInjectSubstringsFrequency);
    // gameNameRuleInjectSubstrings is set of substrings, to inject 1 at random spot in final search/name (sets to entire substring if its >= max len)
    const Blaze::ConfigSequence *commonSequence = config.getSequence("gameNameRuleInjectSubstrings");
    while((commonSequence != nullptr) && commonSequence->hasNext())
    {
        const char8_t* nextValue = commonSequence->nextString("");
        if (nextValue[0] != '\0')
        {
            mPredefinedRuleConfig.sGameNameRuleInjectSubstrings.push_back(nextValue);
            if (ruleEnabled && (mPredefinedRuleConfig.sGameNameRuleInjectSubstringsFrequency > 0) && ((strlen(nextValue) > mPredefinedRuleConfig.sGameNameRuleMaxLengthForSearch) || (strlen(nextValue) > mPredefinedRuleConfig.sGameNameRuleMaxLengthForCreate)))
            {
                BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: Possible problem in len of substring parsed for game name rule common substring " << nextValue << " has length greater than gameNameRuleMaxLengthForSearch=" << mPredefinedRuleConfig.sGameNameRuleMaxLengthForSearch << ", or gameNameRuleMaxLengthForCreate=" << mPredefinedRuleConfig.sGameNameRuleMaxLengthForCreate << "]");
            }
        }
    }
    BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: gameNameRuleInjectSubstrings loaded " << mPredefinedRuleConfig.sGameNameRuleInjectSubstrings.size() << " strings.");


    // final validate at least 1 of source of string/chars is enabled
    const bool dataStringsDisabled = ((mPredefinedRuleConfig.sGameNameRuleRandomizeFrequency == 100) || mPredefinedRuleConfig.sGameNameRuleDataStrings.empty());
    const bool randCharSetDisabled = (mPredefinedRuleConfig.sGameNameRuleRandomizeCharset[0] == '\0');
    const bool injectdStrsDisabled = ((mPredefinedRuleConfig.sGameNameRuleInjectSubstringsFrequency == 0) || mPredefinedRuleConfig.sGameNameRuleInjectSubstrings.empty());
    if (ruleEnabled && dataStringsDisabled && randCharSetDisabled && injectdStrsDisabled)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].parseGameNameRuleConfig: Possible problem parsing game name rule, rule was enabled, but no possible substrings could be generated from all empty char/string sets");
        return false;
    }
    return true;
}


// get setting parsed from config
Blaze::GameNetworkTopology MatchmakerRulesUtil::getNetworkTopology() const
{
    return mPredefinedRuleConfig.sGameNetworkTopology;
}
const char8_t *MatchmakerRulesUtil::getGameProtocolVersionString() const
{
    return mPredefinedRuleConfig.sGameProtocolVersionString;
}
bool MatchmakerRulesUtil::getUseRandomizedGameProtocolVersionString() const
{
    return mPredefinedRuleConfig.sUseRandomizedGameProtocolVersionString;
}
void MatchmakerRulesUtil::buildRoleSettings(StartMatchmakingRequest &request, StartMatchmakingScenarioRequest &scenarioReq, const GamegroupData* ggData /*= nullptr*/, BlazeId blazeId /*= 0*/) const
{
    // If no roles were specified, note it and return.
    if (mPredefinedRoleInformation.sTotalChance == 0)
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].getNetworkTopology: buildRoleSettings: Skipping role settings, since no role information was provided.");
        return;
    }

    // Randomly select roles for the players
    uint32_t curGgMember = 0;
    bool setOwner = false;

    PerPlayerJoinDataList playerDataList;

    eastl::map<RoleName, int> curRoleCount;
    const size_t numDefinedRoles = mPredefinedRoleInformation.sRoleSettings.size();
    for (;;)
    {
        BlazeId curBlazeId = INVALID_BLAZE_ID;
        // iterate over every player: 
        if (ggData)
        {
            if (curGgMember >= ggData->getMemberCount())
                break;

            curBlazeId = ggData->getMemberByIndex(curGgMember);
            ++curGgMember;
        }
        else
        {
            if (setOwner)
                break;

            curBlazeId = blazeId;
            setOwner = true;
        }

        if (curBlazeId == INVALID_BLAZE_ID)
            break;

        // Randomly choose how many roles to desire
        uint16_t numDesiredRoles = (uint16_t) Random::getRandomNumber(numDefinedRoles) + 1; // Always desire at least 1 role
        BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].getNetworkTopology: buildRoleSettings: Player " << curBlazeId << " desires " << numDesiredRoles << " roles out of "
            << numDefinedRoles << " available roles.");
        eastl::hash_set<RoleName, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> curDesiredRoles;

        // Player desires all roles, so use ANY role
        if (numDesiredRoles == numDefinedRoles)
        {
            curDesiredRoles.insert(GameManager::PLAYER_ROLE_NAME_ANY);
        }
        else
        {
            // Randomly choose roles (as long as they pass capacity)
            do
            {
                RoleName roleName = GameManager::PLAYER_ROLE_NAME_DEFAULT;
                bool tryAgain = false;
                int loopNum = 0;
                do
                {
                    ++loopNum;
                    if (loopNum > 100)  // Unlikely
                    {
                        BLAZE_ERR_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].getNetworkTopology: buildRoleSettings: Excessive looping while attempting to form a valid team. Check that your max role capacities will not be exceeded with the game group size.");
                        return;
                    }

                    // Find the value to use: 
                    uint16_t roleChoice = (uint16_t)Random::getRandomNumber(mPredefinedRoleInformation.sTotalChance);

                    // iterate over every role:
                    uint16_t roleCapacity = 0;
                    for (uint32_t i = 0; i < numDefinedRoles; ++i)
                    {
                        const RoleSettings& curSettings = mPredefinedRoleInformation.sRoleSettings[i];
                        if (roleChoice < curSettings.sChance)
                        {
                            roleName = curSettings.sRoleName;
                            roleCapacity = curSettings.sCapacity;
                            break;
                        }
                        else
                        {
                            roleChoice -= curSettings.sChance;
                        }
                    }

                    // see if we need to choose a different role:
                    auto it = curRoleCount.insert(eastl::make_pair(roleName, 0)).first;
                    tryAgain = ((it->second + 1) > roleCapacity) || (curDesiredRoles.find(roleName) != curDesiredRoles.end());
                } while (tryAgain);

                BLAZE_INFO_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].getNetworkTopology: buildRoleSettings: Choosing role '" << roleName.c_str() << "' for user " << curBlazeId << ".");
                curDesiredRoles.insert(roleName);
                ++curRoleCount[roleName];
                --numDesiredRoles;
            } while (numDesiredRoles > 0);
        }

        // Add the player to the role list:
        PerPlayerJoinData* joinData = playerDataList.pull_back();
        joinData->getUser().setBlazeId(curBlazeId);

        for (const auto& role : curDesiredRoles)
            joinData->getRoles().push_back(role);
    }
    playerDataList.copyInto(request.getPlayerJoinData().getPlayerDataList());
    playerDataList.copyInto(scenarioReq.getPlayerJoinData().getPlayerDataList());

    // Fill in the create game information:
    RoleInformation roleInformation;

    for (uint32_t i = 0; i < mPredefinedRoleInformation.sRoleSettings.size(); ++i)
    {
        const RoleSettings& curSettings = mPredefinedRoleInformation.sRoleSettings[i];
        
        Blaze::GameManager::RoleCriteria *roleCriteria = roleInformation.getRoleCriteriaMap().allocate_element();
        roleCriteria->setRoleCapacity(curSettings.sCapacity);
        if (curSettings.sRoleEntryCriteria[0])
        {
            roleCriteria->getRoleEntryCriteriaMap()["rule_1"] = curSettings.sRoleEntryCriteria;
        }
        roleInformation.getRoleCriteriaMap()[curSettings.sRoleName] = roleCriteria;
    }

    if (mPredefinedRoleInformation.sMultiRoleCriteria[0])
    {
        roleInformation.getMultiRoleCriteria()["overall"] = mPredefinedRoleInformation.sMultiRoleCriteria;
    }

    roleInformation.copyInto(request.getGameCreationData().getRoleInformation());
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "ROLE_INFORMATION", roleInformation);
}

void MatchmakerRulesUtil::buildAvoidPreferSettings(StartMatchmakingRequest &request, StartMatchmakingScenarioRequest &scenarioReq, const GamegroupData* ggData /*= nullptr*/, BlazeId blazeId /*= 0*/) const
{
    uint32_t avoidRangeMin = mPredefinedRuleConfig.sMinAvoidPlayerCount;
    uint32_t avoidRangeMax = mPredefinedRuleConfig.sMaxAvoidPlayerCount;
    uint32_t avoidRangeDiff = avoidRangeMax - avoidRangeMin;

    BlazeIdList avoidList;
    if (avoidRangeDiff > 0)
    {
        uint32_t avoidRangeAmount = (rand() % avoidRangeDiff) + avoidRangeMin;
        uint32_t numAvoided = 0;
        for (uint32_t i = 0; i < avoidRangeAmount; ++i)
        {
            BlazeId avoidId = blazeId + (rand() % avoidRangeMax) - (avoidRangeMax/2);

            // Don't avoid players in our PG, or ourselves:
            if (avoidId != blazeId && (!ggData || !ggData->hasMember(avoidId)))
            {
                avoidList.push_back(avoidId);
                ++numAvoided;
            }
        }
    }
    avoidList.copyInto(request.getCriteriaData().getAvoidPlayersRuleCriteria().getAvoidList());
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "AVOID_LIST", avoidList);


    // Since we don't do any overlap testing here, it's possible that we'll both prefer and avoid specific blazeids. 
    uint32_t preferRangeMin = mPredefinedRuleConfig.sMinPreferPlayerCount;
    uint32_t preferRangeMax = mPredefinedRuleConfig.sMaxPreferPlayerCount;
    uint32_t preferRangeDiff = preferRangeMax - preferRangeMin;

    BlazeIdList preferList;
    if (preferRangeDiff > 0)
    {
        uint32_t preferRangeAmount = (rand() % preferRangeDiff) + preferRangeMin;
        for (uint32_t i = 0; i < preferRangeAmount; ++i)
        {
            BlazeId preferId = blazeId + (rand() % preferRangeMax) - (preferRangeMax/2);
            preferList.push_back(preferId);
        }
    }
    preferList.copyInto(request.getCriteriaData().getPreferredPlayersRuleCriteria().getPreferredList());
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "PREFER_LIST", preferList);
}

/*! \param[in] myGroupSize used to avoid start matchmaking request fails due to invalid
    impossible-to-match player slot counts, for the slot count rules. 
    if myGroupSize is default to 0, Matchmaking might fail since MM request has to have at least 1 player, 
    whereas performing a game browser list search with group size of 0 is fine, it means to find full games.*/
void MatchmakerRulesUtil::buildMatchmakingCriteria(MatchmakingCriteriaData &criteria, StartMatchmakingScenarioRequest& scenarioReq, uint16_t myGroupSize /*= 0*/, bool isGBList /*= false*/) const
{
    // ranked game rule
    criteria.getRankedGameRulePrefs().setMinFitThresholdName(mPredefinedRuleConfig.sRankedGameFitThresholdName);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "RANKED_FIT_THR", mPredefinedRuleConfig.sRankedGameFitThresholdName);

    uint16_t rankPref = (uint16_t) rand() % 100;
    RankedGameRulePrefs::RankedGameDesiredValue rankedDesire;
    if (rankPref <= mPredefinedRuleConfig.sRankedGameRuleUnrankedFrequency)
    {
        rankedDesire = RankedGameRulePrefs::UNRANKED;
    }
    else if (rankPref <= mPredefinedRuleConfig.sRankedGameRuleUnrankedFrequency + mPredefinedRuleConfig.sRankedGameRuleRankedFrequency )
    {
        rankedDesire = RankedGameRulePrefs::RANKED;
    }
    else if (rankPref <= mPredefinedRuleConfig.sRankedGameRuleUnrankedFrequency + mPredefinedRuleConfig.sRankedGameRuleRankedFrequency + mPredefinedRuleConfig.sRankedGameRuleRandomFrequency)
    {
        rankedDesire = RankedGameRulePrefs::RANDOM;
    }
    else // abstain
    {
        rankedDesire = RankedGameRulePrefs::ABSTAIN;
    }

    criteria.getRankedGameRulePrefs().setDesiredRankedGameValue(rankedDesire);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "RANKED_DESIRE", rankedDesire);
    switch(mPredefinedRuleConfig.sSizeRuleType)
    {
    case PLAYER_SLOTS_RULES:
        {
            setAllPlayerSlotRulesPreferences(mPredefinedRuleConfig, criteria, scenarioReq, myGroupSize, isGBList);
            criteria.getTeamCountRulePrefs().setTeamCount(mPredefinedRuleConfig.sTeamCount);
            SET_SCENARIO_ATTRIBUTE(scenarioReq, "TEAM_COUNT", mPredefinedRuleConfig.sTeamCount);
            break;
        }
    case TEAM_SIZE_RULES:
        {
            setTeamSizeRulesPreferences(mPredefinedRuleConfig, criteria, scenarioReq);
            break;
        }
    case TEAM_SIZE_AND_PLAYER_SLOT_RULES:
        {
            setTeamSizeRulesPreferences(mPredefinedRuleConfig, criteria, scenarioReq);
            setAllPlayerSlotRulesPreferences(mPredefinedRuleConfig, criteria, scenarioReq, myGroupSize, isGBList);
            break;
        }
    default:
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].buildMatchmakingCriteria: Unhandled game size rules type!");
    }
    

    // dnf rule
    //criteria.getDNFRulePrefs().setMaxDNFValue(mPredefinedRuleConfig.sMaxDNFValue);
    // SET_SCENARIO_ATTRIBUTE(scenarioReq, "MAX_DNF_VALUE", mPredefinedRuleConfig.sMaxDNFValue);

    //geolocation rule
    if (mPredefinedRuleConfig.sGeolocationFitThresholdName[0] != '\0')
    {
        criteria.getGeoLocationRuleCriteria().setMinFitThresholdName(mPredefinedRuleConfig.sGeolocationFitThresholdName);  
        SET_SCENARIO_ATTRIBUTE(scenarioReq, "GEO_LOC_FIT_THR", mPredefinedRuleConfig.sGeolocationFitThresholdName);
    }

    if (mPredefinedRuleConfig.sUseGameModRule)
    {
        uint32_t modReg = 0;
        
        if (mPredefinedRuleConfig.sMaxModRegisterValue > 0)
            modReg = (uint32_t) rand() % (mPredefinedRuleConfig.sMaxModRegisterValue + 1);
        
        criteria.getModRuleCriteria().setIsEnabled(true);       // Is this even checked? 
        SET_SCENARIO_ATTRIBUTE(scenarioReq, "IS_MOD_ENABLED", true);
        criteria.getModRuleCriteria().setDesiredModRegister(modReg);
        SET_SCENARIO_ATTRIBUTE(scenarioReq, "DESIRED_MOD", modReg);
    }

    //skill rule
    //if (mPredefinedRuleConfig.sSkillRuleFitThresholdName[0] != '\0')
    //{
    //    SkillRulePrefs *skillRulePrefs = BLAZE_NEW SkillRulePrefs();
    //    skillRulePrefs->setMinFitThresholdName(mPredefinedRuleConfig.sSkillRuleFitThresholdName);
    //    skillRulePrefs->setRuleName(mPredefinedRuleConfig.sSkillRuleName);
    //    if(mPredefinedRuleConfig.sUseSkillValueOverrride)
    //    {
    //        skillRulePrefs->setUseSkillValueOverride(CLIENT_OVERRIDE_SKILL_VALUE);
    //        int32_t skillValue = (int32_t) rand() % mPredefinedRuleConfig.sSkillRuleMaximumSkill;
    //        if(skillValue < mPredefinedRuleConfig.sSkillRuleMinimumSkill)
    //        {
    //            skillValue = mPredefinedRuleConfig.sSkillRuleMinimumSkill;
    //        }
    //        skillRulePrefs->setSkillValueOverride(skillValue);
    //    }
    //    else
    //    {
    //        skillRulePrefs->setUseSkillValueOverride(USE_SERVER_SKILL_VALUE);
    //    }
    //    criteria.getSkillRulePrefsList().push_back(skillRulePrefs);
    // SET_SCENARIO_ATTRIBUTE(scenarioReq, ,);
    //}

    //QOS rules
    criteria.getHostBalancingRulePrefs().setMinFitThresholdName(mPredefinedRuleConfig.sHostBalancingFitThresholdName);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "HOST_BALANCING_FIT_THR", mPredefinedRuleConfig.sHostBalancingFitThresholdName);
    criteria.getHostViabilityRulePrefs().setMinFitThresholdName(mPredefinedRuleConfig.sHostViabilityFitThresholdName);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "HOST_VIABILITY_FIT_THR", mPredefinedRuleConfig.sHostViabilityFitThresholdName);
    criteria.getExpandedPingSiteRuleCriteria().setRangeOffsetListName(mPredefinedRuleConfig.sPingSiteRangeOffsetListName);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "PING_SITE_OFFSET_LIST", mPredefinedRuleConfig.sPingSiteRangeOffsetListName);

    //game name rule
    setGameNameRulePreferences(mPredefinedRuleConfig, criteria, scenarioReq);

    //set up generic rules
    GenericRuleDefinitionList::const_iterator genericIter = mGenericRuleDefinitions.begin();
    GenericRuleDefinitionList::const_iterator genericEnd = mGenericRuleDefinitions.end();
    for(; genericIter != genericEnd; ++genericIter)
    {
        int32_t roll = rand() % 100;
        if(roll < genericIter->sRuleFreq)
        {
            eastl::string fitName, desireName; 
            fitName.sprintf("%s_FIT_THR", genericIter->sRuleName);
            desireName.sprintf("%s_DESIRED", genericIter->sRuleName);

            if (genericIter->sIsPlayerAttribRule)
            {
                GameManager::PlayerAttributeRuleCriteria* genericRulePrefs = criteria.getPlayerAttributeRuleCriteriaMap()[genericIter->sRuleName];
                if (genericRulePrefs == nullptr)
                {
                    genericRulePrefs = criteria.getPlayerAttributeRuleCriteriaMap()[genericIter->sRuleName] = criteria.getPlayerAttributeRuleCriteriaMap().allocate_element();
                }
                else
                {
                    // Only one player attribute rule of a given name can be used at a time.  Subsequent entries were previously ignored in blazeserver code.
                    continue;
                }

                //set threshold
                int32_t thresholdRoll = rand() % 100;
                int32_t thresholdFreq = 0;
                FitThresholdList::const_iterator thresholdIter = genericIter->sThresholds.begin();
                FitThresholdList::const_iterator thresholdEnd = genericIter->sThresholds.end();
                for (; thresholdIter != thresholdEnd; ++thresholdIter)
                {
                    thresholdFreq += thresholdIter->sThresholdFreq;
                    if (thresholdRoll < thresholdFreq)
                    {
                        genericRulePrefs->setMinFitThresholdName(thresholdIter->sThresholdName);
                        SET_SCENARIO_ATTRIBUTE(scenarioReq, fitName.c_str(), thresholdIter->sThresholdName);
                        break;
                    }
                }

                //set desired value
                int32_t desiredValueRoll = rand() % 100;
                int32_t desiredValueFreq = 0;
                PossibleValuesList::const_iterator desiredValueIter = genericIter->sRulePossibleValues.begin();
                PossibleValuesList::const_iterator desiredValueEnd = genericIter->sRulePossibleValues.end();
                for (; desiredValueIter != desiredValueEnd; ++desiredValueIter)
                {
                    desiredValueFreq += desiredValueIter->sValueFreq;
                    if (desiredValueRoll < desiredValueFreq)
                    {
                        eastl::vector<const char8_t*>::const_iterator valueIter = desiredValueIter->sValueList.begin();
                        eastl::vector<const char8_t*>::const_iterator valueEnd = desiredValueIter->sValueList.end();
                        Collections::AttributeValueList desiredValues;
                        for (; valueIter != valueEnd; ++valueIter)
                        {
                            desiredValues.push_back(*valueIter);
                        }
                        desiredValues.copyInto(genericRulePrefs->getDesiredValues());
                        SET_SCENARIO_ATTRIBUTE(scenarioReq, desireName.c_str(), desiredValues);
                        break;
                    }
                }
            }
            else
            {
                // Game Attribute rule
                GameManager::GameAttributeRuleCriteria* genericRulePrefs = criteria.getGameAttributeRuleCriteriaMap()[genericIter->sRuleName];
                if (genericRulePrefs == nullptr)
                {
                    genericRulePrefs = criteria.getGameAttributeRuleCriteriaMap()[genericIter->sRuleName] = criteria.getGameAttributeRuleCriteriaMap().allocate_element();
                }
                else
                {
                    // Only one game attribute rule of a given name can be used at a time.  Subsequent entries were previously ignored in blazeserver code.
                    continue;
                }

                //set threshold
                int32_t thresholdRoll = rand() % 100;
                int32_t thresholdFreq = 0;
                FitThresholdList::const_iterator thresholdIter = genericIter->sThresholds.begin();
                FitThresholdList::const_iterator thresholdEnd = genericIter->sThresholds.end();
                for (; thresholdIter != thresholdEnd; ++thresholdIter)
                {
                    thresholdFreq += thresholdIter->sThresholdFreq;
                    if (thresholdRoll < thresholdFreq)
                    {
                        genericRulePrefs->setMinFitThresholdName(thresholdIter->sThresholdName);
                        SET_SCENARIO_ATTRIBUTE(scenarioReq, fitName.c_str(), thresholdIter->sThresholdName);
                        break;
                    }
                }

                //set desired value
                int32_t desiredValueRoll = rand() % 100;
                int32_t desiredValueFreq = 0;
                PossibleValuesList::const_iterator desiredValueIter = genericIter->sRulePossibleValues.begin();
                PossibleValuesList::const_iterator desiredValueEnd = genericIter->sRulePossibleValues.end();
                for (; desiredValueIter != desiredValueEnd; ++desiredValueIter)
                {
                    desiredValueFreq += desiredValueIter->sValueFreq;
                    if (desiredValueRoll < desiredValueFreq)
                    {
                        eastl::vector<const char8_t*>::const_iterator valueIter = desiredValueIter->sValueList.begin();
                        eastl::vector<const char8_t*>::const_iterator valueEnd = desiredValueIter->sValueList.end();
                        Collections::AttributeValueList desiredValues;
                        for (; valueIter != valueEnd; ++valueIter)
                        {
                            desiredValues.push_back(*valueIter);
                        }
                        desiredValues.copyInto(genericRulePrefs->getDesiredValues());
                        SET_SCENARIO_ATTRIBUTE(scenarioReq, desireName.c_str(), desiredValues);
                        break;
                    }
                }
            }
        }
    }

    // setup UED rules
    UEDRuleDefinitionList::const_iterator uedIter = mUEDRuleDefinitions.begin();
    UEDRuleDefinitionList::const_iterator uedEnd = mUEDRuleDefinitions.end();
    for (; uedIter != uedEnd; ++uedIter)
    {
        int32_t roll = rand() % 100;
        if(roll < uedIter->mRuleFreq)
        {
            eastl::string fitName, desireName; 
            fitName.sprintf("%s_FIT_THRESHOLD", uedIter->mRuleName);
            desireName.sprintf("%s_DESIRED", uedIter->mRuleName);

            GameManager::UEDRuleCriteria* uedRulePrefs = criteria.getUEDRuleCriteriaMap()[uedIter->mRuleName];
            if (uedRulePrefs == nullptr)
            {
                uedRulePrefs = criteria.getUEDRuleCriteriaMap()[uedIter->mRuleName] = criteria.getUEDRuleCriteriaMap().allocate_element();
            }
            else
            {
                // Only one ued rule of a given name can be used at a time.  Subsequent entries were previously ignored in blazeserver code.
                continue;
            }

            //set threshold
            int32_t thresholdRoll = rand() % 100;
            int32_t thresholdFreq = 0;
            FitThresholdList::const_iterator thresholdIter = uedIter->mThresholds.begin();
            FitThresholdList::const_iterator thresholdEnd = uedIter->mThresholds.end();
            for(; thresholdIter != thresholdEnd; ++thresholdIter)
            {
                thresholdFreq += thresholdIter->sThresholdFreq;
                if(thresholdRoll < thresholdFreq)
                {
                    uedRulePrefs->setThresholdName(thresholdIter->sThresholdName);
                    SET_SCENARIO_ATTRIBUTE(scenarioReq, fitName.c_str(), thresholdIter->sThresholdName);
                    break;
                }
            }
        }
    }
}


void MatchmakerRulesUtil::setTeamSizeRulesPreferences(
    const PredefinedRuleConfiguration &predefinedRuleConfig,
    MatchmakingCriteriaData &criteria,
    StartMatchmakingScenarioRequest& scenarioReq) const
{   
    uint16_t minTeamSize = predefinedRuleConfig.sMinTeamSize;

    // team count
    criteria.getTeamCountRulePrefs().setTeamCount(predefinedRuleConfig.sTeamCount);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TEAM_COUNT", predefinedRuleConfig.sTeamCount);

    // team min size rule
    criteria.getTeamMinSizeRulePrefs().setRangeOffsetListName(predefinedRuleConfig.sTeamMinSizeRangeListName);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TEAM_MIN_SIZE_RANGE_OFFSET", predefinedRuleConfig.sTeamMinSizeRangeListName);
    criteria.getTeamMinSizeRulePrefs().setTeamMinSize(minTeamSize);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TEAM_MIN_SIZE", minTeamSize);

    // team balance rule
    criteria.getTeamBalanceRulePrefs().setRangeOffsetListName(predefinedRuleConfig.sTeamBalanceRangeListName);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TEAM_BALANCE_RANGE_OFFSET", predefinedRuleConfig.sTeamBalanceRangeListName);
    criteria.getTeamBalanceRulePrefs().setMaxTeamSizeDifferenceAllowed(predefinedRuleConfig.sMaxTeamSizeDifference);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TEAM_MAX_SIZE_DIFF", predefinedRuleConfig.sMaxTeamSizeDifference);

    // team UED balance.  Note: see UEDUtil for methods of setting the session UED values.
    criteria.getTeamUEDBalanceRulePrefs().setRangeOffsetListName(predefinedRuleConfig.sTeamUEDBalanceRuleRangeListName);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TEAM_UED_RANGE_OFFSET", predefinedRuleConfig.sTeamUEDBalanceRuleRangeListName);
    criteria.getTeamUEDBalanceRulePrefs().setRuleName(predefinedRuleConfig.sTeamUEDBalanceRuleName);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TEAM_UED_RULE_NAME", predefinedRuleConfig.sTeamUEDBalanceRuleName);

    // team UED Position Parity.  Note: see UEDUtil for methods of setting the session UED values.
    criteria.getTeamUEDPositionParityRulePrefs().setRangeOffsetListName(predefinedRuleConfig.sTeamUEDPositionParityRuleRangeListName);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TEAM_UED_PP_RANGE_OFFSET", predefinedRuleConfig.sTeamUEDPositionParityRuleRangeListName);
    criteria.getTeamUEDPositionParityRulePrefs().setRuleName(predefinedRuleConfig.sTeamUEDPositionParityRuleName);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TEAM_UED_PP_RULE_NAME", predefinedRuleConfig.sTeamUEDPositionParityRuleName);

    // team composition
    criteria.getTeamCompositionRulePrefs().setMinFitThresholdName(predefinedRuleConfig.sTeamCompositionRuleMinFitListName);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TEAM_COMP_FIT_THR", predefinedRuleConfig.sTeamCompositionRuleMinFitListName);
    criteria.getTeamCompositionRulePrefs().setRuleName(predefinedRuleConfig.sTeamCompositionRuleName);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TEAM_COMP_RULE_NAME", predefinedRuleConfig.sTeamCompositionRuleName);
}


void MatchmakerRulesUtil::setAllPlayerSlotRulesPreferences(
    const PredefinedRuleConfiguration &predefinedRuleConfig,
    MatchmakingCriteriaData &criteria, StartMatchmakingScenarioRequest& scenarioReq, uint16_t myGroupSize, bool isGBList) const
{
    setPlayerCountRulePreferences(mPredefinedRuleConfig, criteria, scenarioReq, myGroupSize, isGBList);
    setTotalPlayerSlotsRulePreferences(mPredefinedRuleConfig, criteria, scenarioReq, myGroupSize, isGBList);
    setPlayerSlotUtilizationRulePreferences(mPredefinedRuleConfig, criteria, scenarioReq);
    setFreePlayerSlotsRulePreferences(mPredefinedRuleConfig, criteria, scenarioReq);
}

void MatchmakerRulesUtil::setPlayerSlotUtilizationRulePreferences( 
    const PredefinedRuleConfiguration& config, 
    MatchmakingCriteriaData &criteria,
    StartMatchmakingScenarioRequest& scenarioReq) const
{
    uint8_t maxPercentFull = config.sMaxPlayerSlotUtilizationLowerBound +
        ((config.sMaxPlayerSlotUtilizationSeed==0)? 0 : (uint8_t) rand() % config.sMaxPlayerSlotUtilizationSeed);
    uint8_t minPercentFull = config.sMinPlayerSlotUtilizationLowerBound +
        ((config.sMinPlayerSlotUtilizationSeed==0)? 0 : (uint8_t) rand() % config.sMinPlayerSlotUtilizationSeed);

    uint8_t desiredPlayerSlotUtilization;

    if(maxPercentFull <= minPercentFull)
    {
        maxPercentFull = minPercentFull;
        desiredPlayerSlotUtilization = minPercentFull;
    }
    else
    {
        desiredPlayerSlotUtilization = ((uint8_t)((config.sDesiredPlayerSlotUtilizationPercent / 100.0) * (maxPercentFull - minPercentFull)));
        desiredPlayerSlotUtilization = minPercentFull + desiredPlayerSlotUtilization;
    }

    criteria.getPlayerSlotUtilizationRuleCriteria().setRangeOffsetListName(config.sPlayerSlotUtilizationRangeList);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "SLOT_UTIL_RANGE_OFFSET", config.sPlayerSlotUtilizationRangeList);
    criteria.getPlayerSlotUtilizationRuleCriteria().setMinPercentFull(minPercentFull);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "SLOT_UTIL_MIN", minPercentFull);
    criteria.getPlayerSlotUtilizationRuleCriteria().setMaxPercentFull(maxPercentFull);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "SLOT_UTIL_MAX", maxPercentFull);
    criteria.getPlayerSlotUtilizationRuleCriteria().setDesiredPercentFull(desiredPlayerSlotUtilization);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "SLOT_UTIL_DESIRE", desiredPlayerSlotUtilization);
}


void MatchmakerRulesUtil::setPlayerCountRulePreferences( 
    const PredefinedRuleConfiguration& config, 
    MatchmakingCriteriaData &criteria, StartMatchmakingScenarioRequest& scenarioReq, uint16_t myGroupSize, bool isGBList) const
{
    uint16_t maxPlayers = config.sMaxValuePlayerCountRuleLowerBound +
        ((config.sMaxValuePlayerCountRuleSeed==0)? 0 : (uint16_t) rand() % config.sMaxValuePlayerCountRuleSeed);
    uint16_t minPlayers = config.sMinValuePlayerCountRuleLowerBound +
        ((config.sMinValuePlayerCountRuleSeed==0)? 0 : (uint16_t) rand() % config.sMinValuePlayerCountRuleSeed);
 
    // we can lower-bound the min by myGroupSize (as long as group is smaller than top poss max).
    if (myGroupSize < (config.sMaxValuePlayerCountRuleLowerBound + config.sMaxValuePlayerCountRuleSeed))
    {
        if (minPlayers < myGroupSize)
            minPlayers = myGroupSize;
    }
    else if (!isGBList) // skip warn log if it is for GameBrowser list
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].setPlayerCountRulePreference: your game group size of " << myGroupSize << " exceeds the max possible player count criteria value of " << (config.sMaxValuePlayerCountRuleLowerBound + config.sMaxValuePlayerCountRuleSeed) << ". Your MM request may fail.");
    }

    uint16_t desiredValuePlayerCountRule;
    if (maxPlayers <= minPlayers)
    {
        maxPlayers = minPlayers;
        desiredValuePlayerCountRule = minPlayers;
    }
    else
    {
        desiredValuePlayerCountRule = ((uint16_t)((config.sDesiredValuePlayerCountRulePercent / 100.0) * (maxPlayers - minPlayers)));
        desiredValuePlayerCountRule = minPlayers + desiredValuePlayerCountRule;
    }

    criteria.getPlayerCountRuleCriteria().setRangeOffsetListName(config.sPlayerCountRuleRangeList);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "PLAYER_COUNT_RANGE_OFFSET", config.sPlayerCountRuleRangeList);
    criteria.getPlayerCountRuleCriteria().setMinPlayerCount(minPlayers);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "PLAYER_COUNT_MIN_COUNT", minPlayers);
    criteria.getPlayerCountRuleCriteria().setMaxPlayerCount(maxPlayers);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "PLAYER_COUNT_MAX_COUNT", maxPlayers);
    criteria.getTotalPlayerSlotsRuleCriteria().setMaxTotalPlayerSlots(maxPlayers);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TOTAL_SLOTS_MAX", maxPlayers);
    criteria.getPlayerCountRuleCriteria().setDesiredPlayerCount(desiredValuePlayerCountRule);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "PLAYER_COUNT_DESIRE", desiredValuePlayerCountRule);
}


void MatchmakerRulesUtil::setTotalPlayerSlotsRulePreferences( 
    const PredefinedRuleConfiguration& config, 
    MatchmakingCriteriaData &criteria, StartMatchmakingScenarioRequest& scenarioReq, uint16_t myGroupSize, bool isGBList) const
{
    uint16_t maxSlots = config.sMaxTotalPlayerSlotsLowerBound +
        ((config.sMaxTotalPlayerSlotsSeed == 0)? 0 : (uint16_t) rand() % config.sMaxTotalPlayerSlotsSeed);
    uint16_t minSlots = config.sMinTotalPlayerSlotsLowerBound +
        ((config.sMinTotalPlayerSlotsSeed == 0)? 0 : (uint16_t) rand() % config.sMinTotalPlayerSlotsSeed);

    // we can lower-bound the min by myGroupSize (as long as group is smaller than top poss max).
    if (myGroupSize < (config.sMaxTotalPlayerSlotsLowerBound + config.sMaxTotalPlayerSlotsSeed))
    {
        if (minSlots < myGroupSize)
            minSlots = myGroupSize;
    }
    else if (!isGBList) // skip warn log if it is for GameBrowser list
    {
        BLAZE_WARN_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].setTotalPlayerSlotsRulePreferences: your game group size of " << myGroupSize << " exceeds the max possible total player slots criteria value of " << (config.sMaxTotalPlayerSlotsLowerBound + config.sMaxTotalPlayerSlotsSeed) << ". Your MM request may fail.");
    }

    uint16_t desiredTotalPlayerSlots;
    if(maxSlots <= minSlots)
    {
        maxSlots = minSlots;
        desiredTotalPlayerSlots = minSlots;
    }
    else
    {
        desiredTotalPlayerSlots = ((uint16_t)((config.sDesiredTotalPlayerSlotsPercent / 100.0) * (maxSlots - minSlots)));
        desiredTotalPlayerSlots = minSlots + desiredTotalPlayerSlots;
    }

    criteria.getTotalPlayerSlotsRuleCriteria().setRangeOffsetListName(config.sTotalPlayerSlotsRuleRangeList);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TOTAL_SLOTS_RANGE_OFFSET", config.sTotalPlayerSlotsRuleRangeList);
    criteria.getTotalPlayerSlotsRuleCriteria().setMinTotalPlayerSlots(minSlots);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TOTAL_SLOTS_MIN", minSlots);
    criteria.getTotalPlayerSlotsRuleCriteria().setMaxTotalPlayerSlots(maxSlots);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TOTAL_SLOTS_MAX", maxSlots);
    criteria.getTotalPlayerSlotsRuleCriteria().setDesiredTotalPlayerSlots(desiredTotalPlayerSlots);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "TOTAL_SLOTS_DESIRE", desiredTotalPlayerSlots);
}


void MatchmakerRulesUtil::setFreePlayerSlotsRulePreferences( 
    const PredefinedRuleConfiguration& config, 
    MatchmakingCriteriaData &criteria,
    StartMatchmakingScenarioRequest& scenarioReq) const
{
    uint16_t maxSlots = config.sMaxFreePlayerSlotsLowerBound +
        ((config.sMaxFreePlayerSlotsSeed == 0)? 0 : (uint16_t) rand() % config.sMaxFreePlayerSlotsSeed);
    uint16_t minSlots = config.sMinFreePlayerSlotsLowerBound +
        ((config.sMinFreePlayerSlotsSeed == 0)? 0 : (uint16_t) rand() % config.sMinFreePlayerSlotsSeed);

    if(maxSlots <= minSlots)
    {
        maxSlots = minSlots;
    }

    criteria.getFreePlayerSlotsRuleCriteria().setMinFreePlayerSlots(minSlots);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "FREE_SLOTS_MIN", minSlots);
    criteria.getFreePlayerSlotsRuleCriteria().setMaxFreePlayerSlots(maxSlots);
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "FREE_SLOTS_MAX", maxSlots);
}


// set game name rule prefs if rolled to do so.
void MatchmakerRulesUtil::setGameNameRulePreferences(
    const PredefinedRuleConfiguration &predefinedRuleConfig,
    MatchmakingCriteriaData &criteria,
    StartMatchmakingScenarioRequest& scenarioReq) const
{
    criteria.getGameNameRuleCriteria().setSearchString("");
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "GAME_NAME_SEARCH", "");

    if (!isGameNameRuleEnabled())
    {
        return;
    }

    const int32_t roll = Blaze::Random::getRandomNumber(100);
    if (roll >= mPredefinedRuleConfig.sGameNameRuleFrequency)
    {
        // dont use GameNameRule
        return;
    }

    eastl::string search;
    makeGameNameRuleFromConfig(predefinedRuleConfig, false, search);
    criteria.getGameNameRuleCriteria().setSearchString(search.c_str());
    SET_SCENARIO_ATTRIBUTE(scenarioReq, "GAME_NAME_SEARCH", search.c_str());
    BLAZE_SPAM_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].setFreePlayerSlotsRulePreferences: assigned GameNameRule search string: " << search.c_str());
}

// get GameNameRule search string (or create game request name) via stress config (see). Returns empty string on fail.
uint64_t MatchmakerRulesUtil::mGameNameRuleDataStringsUseCountForCreate = 0;
uint64_t MatchmakerRulesUtil::mGameNameRuleDataStringsUseCountForSearch = 0;
void MatchmakerRulesUtil::makeGameNameRuleFromConfig(const PredefinedRuleConfiguration &config,
                                                     bool forCreateInsteadOfSearch,
                                                     eastl::string& result) const
{
    result.clear();
    const uint32_t maxLen = (forCreateInsteadOfSearch? config.sGameNameRuleMaxLengthForCreate : config.sGameNameRuleMaxLengthForSearch);
    
    // Roll whether to random generate string or pick it from file
    if ((Blaze::Random::getRandomNumber(100) < config.sGameNameRuleRandomizeFrequency) &&
        !config.sGameNameRuleDataStrings.empty())
    {
        // Randomize length (bounded below by configured min len). Side: note might take value less than this if pick shorter non-generated string below.
        const uint32_t rollLen = ((maxLen - config.sGameNameRuleMinLength == 0)? maxLen :
            (Blaze::Random::getRandomNumber(maxLen - config.sGameNameRuleMinLength + 1) + config.sGameNameRuleMinLength));
        result.reserve(rollLen);
        
        // randomize string's chars
        const uint16_t charSetLen = static_cast<uint16_t>(strlen(config.sGameNameRuleRandomizeCharset));
        for (uint16_t i = 0; i < rollLen; ++i)
        {
            const uint16_t index = (uint16_t)Blaze::Random::getRandomNumber(charSetLen);
            result += config.sGameNameRuleRandomizeCharset[index];
        }
    }
    else if (!config.sGameNameRuleDataStrings.empty())
    {
        // Pick 1 string from file, instead of random generating
        uint64_t &linesReadCount = (forCreateInsteadOfSearch? mGameNameRuleDataStringsUseCountForCreate : mGameNameRuleDataStringsUseCountForSearch);
        const uint32_t ind = (uint32_t)(config.sGameNameRuleDataStringsObserveOrderInFile? ((linesReadCount++)%config.sGameNameRuleDataStrings.size())
            : Blaze::Random::getRandomNumber(config.sGameNameRuleDataStrings.size()));
        result = config.sGameNameRuleDataStrings.at(ind);
        if (result.length() > maxLen)
        {
            // clamp length if greater than configured max
            const int32_t indexRoll = Blaze::Random::getRandomNumber(result.length() - maxLen + 1);
            eastl::string temp = result.substr(indexRoll, indexRoll + maxLen);
            result = temp;
        }
        EA_ASSERT_MSG(!result.empty(), "[MatchmakerRulesUtil].makeGameNameRuleFromConfig: internal stress tool/config error gameNameRuleDataStrings may have had null or empty at ind");
    }
    if (result.empty())
    {
        BLAZE_TRACE_LOG(BlazeRpcLog::gamemanager, "[MatchmakerRulesUtil].makeGameNameRuleFromConfig: Warn: returning no assigned/disabled/empty " << (forCreateInsteadOfSearch? "custom game name" : "game name rule search string") << " source chars/string values may have been invalidly left empty in the stress configuration.");
        return;
    }

    // Roll whether to inject one of the common substrings
    if (!config.sGameNameRuleInjectSubstrings.empty() &&
        (Blaze::Random::getRandomNumber(100) < config.sGameNameRuleInjectSubstringsFrequency))
    {
        const uint16_t ind = (uint16_t)Blaze::Random::getRandomNumber(config.sGameNameRuleInjectSubstrings.size());
        const char8_t* substring = config.sGameNameRuleInjectSubstrings.at(ind);
        EA_ASSERT_MSG((substring != nullptr) && (substring[0] != '\0'), "[MatchmakerRulesUtil].makeGameNameRuleFromConfig: internal stress tool/config error gameNameRuleInjectSubstrings may have had null or empty at ind");
        const uint32_t substringLen = static_cast<uint32_t>(strlen(substring));
        if (result.length() <= substringLen)
        {
            // If substring picked is longer than orig picked string's length, simply return substring.
            result = substring;
        }
        else
        {
            // pick location to insert substring at
            const uint32_t substringInsertIndex = Blaze::Random::getRandomNumber(result.length() - substringLen + 1);
            result.replace(substringInsertIndex, substringLen, substring);
        }
    }
}


} // Stress end
} // Blaze end

#undef SET_SCENARIO_ATTRIBUTE
