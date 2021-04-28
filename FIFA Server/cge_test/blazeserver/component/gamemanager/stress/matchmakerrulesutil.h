/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_MATCHMAKERRULE_UTIL
#define BLAZE_STRESS_MATCHMAKERRULE_UTIL

#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/tdf/scenarios.h"

using namespace Blaze::GameManager;

namespace Blaze
{
namespace Stress
{
class MatchmakerModule;
class GamegroupData;

enum SizeRuleType
{
    PLAYER_SLOTS_RULES,
    TEAM_SIZE_RULES,
    TEAM_SIZE_AND_PLAYER_SLOT_RULES
};

struct RoleSettings
{
    char8_t sRoleName[32];
    uint16_t sCapacity;
    uint16_t sChance;
    char8_t sRoleEntryCriteria[64];
};
typedef eastl::vector<RoleSettings> RoleSettingsList;

struct PredefinedRoleInformation
{
    char8_t sMultiRoleCriteria[64];
    RoleSettingsList sRoleSettings;
    uint16_t sTotalChance;
};

struct PredefinedRuleConfiguration
{
    GameNetworkTopology sGameNetworkTopology;

    char8_t sGameProtocolVersionString[64];
    bool sUseRandomizedGameProtocolVersionString;

    SizeRuleType sSizeRuleType;

    char8_t sGameSizeMinFitThresholdName[MAX_MINFITTHRESHOLDNAME_LEN];
    uint16_t sMinPlayerCount;
    uint16_t sMinPlayerCountSeed;
    uint16_t sMaxPlayerCount;
    uint16_t sMaxPlayerCountSeed;
    uint16_t sDesiredGameSizePercent;
    bool sIsSingleGroupMatch;

    char8_t sTeamSizeRangeListName[MAX_MINFITTHRESHOLDNAME_LEN];
    uint16_t sTeamCount;
    uint16_t sMaxTeamSizeDifference;
    uint16_t sMinTeamSize;
    uint16_t sMaxTeamSize;
    uint16_t sDesiredTeamSize;
    eastl::vector<Blaze::GameManager::TeamId> sJoiningTeamIdPossibleValues;
    eastl::vector<Blaze::GameManager::TeamId> sRequestedTeamIdPossibleValues;
    bool sAllowDuplicateTeamIds;
    // Note: see UEDUtil for how to set up UED for testing with the UED based rules.
    char8_t sTeamUEDBalanceRuleName[MAX_RULENAME_LEN];
    char8_t sTeamUEDBalanceRuleRangeListName[MAX_MINFITTHRESHOLDNAME_LEN];
    char8_t sTeamUEDPositionParityRuleName[MAX_RULENAME_LEN];
    char8_t sTeamUEDPositionParityRuleRangeListName[MAX_MINFITTHRESHOLDNAME_LEN];
    char8_t sTeamCompositionRuleName[MAX_RULENAME_LEN];
    char8_t sTeamCompositionRuleMinFitListName[MAX_MINFITTHRESHOLDNAME_LEN];
    char8_t sTeamMinSizeRangeListName[MAX_MINFITTHRESHOLDNAME_LEN];
    char8_t sTeamBalanceRangeListName[MAX_MINFITTHRESHOLDNAME_LEN];

    char8_t sPlayerSlotUtilizationRangeList[MAX_MINFITTHRESHOLDNAME_LEN];
    uint8_t sMinPlayerSlotUtilizationLowerBound;
    uint8_t sMinPlayerSlotUtilizationSeed;
    uint8_t sMaxPlayerSlotUtilizationLowerBound;
    uint8_t sMaxPlayerSlotUtilizationSeed;
    uint8_t sDesiredPlayerSlotUtilizationPercent;

    char8_t sPlayerCountRuleRangeList[MAX_MINFITTHRESHOLDNAME_LEN];
    uint16_t sMinValuePlayerCountRuleLowerBound;
    uint16_t sMinValuePlayerCountRuleSeed;
    uint16_t sMaxValuePlayerCountRuleLowerBound;
    uint16_t sMaxValuePlayerCountRuleSeed;
    uint16_t sDesiredValuePlayerCountRulePercent;

    char8_t sTotalPlayerSlotsRuleRangeList[MAX_MINFITTHRESHOLDNAME_LEN];
    uint16_t sMinTotalPlayerSlotsLowerBound;
    uint16_t sMinTotalPlayerSlotsSeed;
    uint16_t sMaxTotalPlayerSlotsLowerBound;
    uint16_t sMaxTotalPlayerSlotsSeed;
    uint16_t sDesiredTotalPlayerSlotsPercent;

    uint16_t sMinFreePlayerSlotsLowerBound;
    uint16_t sMinFreePlayerSlotsSeed;
    uint16_t sMaxFreePlayerSlotsLowerBound;
    uint16_t sMaxFreePlayerSlotsSeed;


    int32_t sMaxDNFValue;

    char8_t sSkillRuleFitThresholdName[MAX_MINFITTHRESHOLDNAME_LEN];
    char8_t sSkillRuleName[MAX_RULENAME_LEN];
    int32_t sSkillRuleMinimumSkill;
    int32_t sSkillRuleMaximumSkill;
    bool sUseSkillValueOverrride;
    

    char8_t sRankedGameFitThresholdName[MAX_MINFITTHRESHOLDNAME_LEN];
    uint16_t sRankedGameRuleUnrankedFrequency;
    uint16_t sRankedGameRuleRankedFrequency;
    uint16_t sRankedGameRuleRandomFrequency;
    uint16_t sRankedGameRuleAbstainFrequency;

    char8_t sHostBalancingFitThresholdName[MAX_MINFITTHRESHOLDNAME_LEN];

    char8_t sHostViabilityFitThresholdName[MAX_MINFITTHRESHOLDNAME_LEN];

    char8_t sPingSiteRangeOffsetListName[MAX_MINFITTHRESHOLDNAME_LEN];
    
    char8_t sGeolocationFitThresholdName[MAX_MINFITTHRESHOLDNAME_LEN];

    bool sUseGameModRule;
    uint32_t sMaxModRegisterValue;

    uint16_t sGameNameRuleFrequency;
    eastl::vector<eastl::string> sGameNameRuleDataStrings;
    uint16_t sGameNameRuleDataStringsMaxToUse;
    bool  sGameNameRuleDataStringsObserveOrderInFile;
    uint16_t sGameNameRuleRandomizeFrequency;
    char8_t  sGameNameRuleRandomizeCharset[256];
    uint32_t sGameNameRuleMinLength;
    uint32_t sGameNameRuleMaxLengthForCreate;
    uint32_t sGameNameRuleMaxLengthForSearch;
    uint16_t sGameNameRuleInjectSubstringsFrequency;
    eastl::vector<const char8_t*> sGameNameRuleInjectSubstrings;

    int32_t sMinAvoidPlayerCount;
    int32_t sMaxAvoidPlayerCount;
    int32_t sMinPreferPlayerCount;
    int32_t sMaxPreferPlayerCount;
};
struct GenericRulesPossibleValues
{
    eastl::vector<const char8_t*> sValueList;
    uint16_t sValueFreq;
};
struct GenericRuleFitThreshold
{
    const char8_t* sThresholdName;
    uint16_t sThresholdFreq;
};
typedef eastl::vector<GenericRulesPossibleValues> PossibleValuesList;
typedef eastl::vector<GenericRuleFitThreshold> FitThresholdList;
struct GenericRuleDefinition
{
    const char8_t* sRuleName;
    uint16_t sRuleFreq;
    PossibleValuesList sRulePossibleValues;
    bool sIsPlayerAttribRule;
    const char8_t* sAttribName;
    PossibleValuesList sAttributePossibleValues;
    FitThresholdList sThresholds;
};
typedef eastl::vector<GenericRuleDefinition> GenericRuleDefinitionList;

struct UEDRuleDefintion
{
    const char8_t* mRuleName;
    uint16_t mRuleFreq;
    FitThresholdList mThresholds;
};
typedef eastl::vector<UEDRuleDefintion> UEDRuleDefinitionList;

/*************************************************************************************************/
/*
    MatchmakerRulesUtil
*/
/*************************************************************************************************/
class MatchmakerRulesUtil
{
    NON_COPYABLE(MatchmakerRulesUtil);

public:
    MatchmakerRulesUtil();

    virtual ~MatchmakerRulesUtil();

    bool parseRules(const ConfigMap& config);

    Blaze::GameNetworkTopology getNetworkTopology() const;
    const char8_t *getGameProtocolVersionString() const;
    bool getUseRandomizedGameProtocolVersionString() const;
    const GenericRuleDefinitionList* getGenericRuleDefinitionList() const { return &mGenericRuleDefinitions; }
    bool getAllowDuplicateTeamIds() const { return mPredefinedRuleConfig.sAllowDuplicateTeamIds; }

    const PredefinedRuleConfiguration &getPredefinedRuleConfig() const { return mPredefinedRuleConfig; }

    void buildRoleSettings(StartMatchmakingRequest &request, StartMatchmakingScenarioRequest& scenarioReq, const GamegroupData* ggData = nullptr, BlazeId blazeId = 0) const;
    void buildAvoidPreferSettings(StartMatchmakingRequest &request, StartMatchmakingScenarioRequest& scenarioReq, const GamegroupData* ggData = nullptr, BlazeId blazeId = 0) const;

    // matchmaking criteria rule data
    void buildMatchmakingCriteria(MatchmakingCriteriaData &criteria, StartMatchmakingScenarioRequest& scenarioReq, uint16_t myGroupSize = 0, bool isGBList = false) const;

    // pre: stress configs have already been parsed
    bool isGameNameRuleEnabled() const { return (mPredefinedRuleConfig.sGameNameRuleFrequency > 0); }
    void makeGameNameRuleFromConfig(const PredefinedRuleConfiguration &predefinedRuleConfig, bool forCreateInsteadOfSearch, eastl::string& result) const;
private:

    bool parseRoleSettings( const ConfigMap& config );

    bool parsePredefinedRules( const ConfigMap &config );
    bool parseUEDRules(const ConfigMap& config);
    bool parseGenericRules(const ConfigMap& config);
    bool parseSizeRules( const ConfigMap& config );

    bool parseGameAndTeamSizeRuleCombination( const ConfigMap& config );
    bool parseGameSizeRule( const ConfigMap& config );
    bool parseOldTeamSizeRule( const ConfigMap& config );
    bool parseTeamSizeRules( const ConfigMap& config );
    bool parseNewAndOldTeamSizeRulesCommon( const ConfigMap& config );

    bool parseAllPlayerSlotRules( const ConfigMap& config );
    bool parsePlayerCountRule( const ConfigMap& config );
    bool parseTotalPlayerSlotsRule( const ConfigMap& config );
    bool parsePlayerSlotUtilizationRule( const ConfigMap& config );
    bool parseFreePlayerSlotsRule( const ConfigMap& config );
    bool parseGameSizeAndAllPlayerSlotRulesCombination( const ConfigMap& config );
    bool parseTeamSizeAndPlayerSlotRulesCombination( const ConfigMap& config );

    bool parseSizeRuleType(const char8_t* sizeRuleString, SizeRuleType &parsedSizeRuleType) const;

    bool parseGameNameRuleConfig( const ConfigMap& config );

    UEDRuleDefinitionList mUEDRuleDefinitions;
    GenericRuleDefinitionList mGenericRuleDefinitions;
    PredefinedRoleInformation mPredefinedRoleInformation;

    PredefinedRuleConfiguration mPredefinedRuleConfig;
    void setTeamSizeRulesPreferences( const PredefinedRuleConfiguration& predefinedRuleConfig, MatchmakingCriteriaData &criteria, StartMatchmakingScenarioRequest& scenarioReq ) const;

    void setAllPlayerSlotRulesPreferences(const PredefinedRuleConfiguration &predefinedRuleConfig, MatchmakingCriteriaData &criteria, StartMatchmakingScenarioRequest& scenarioReq, uint16_t myGroupSize, bool isGBList) const;
    void setPlayerSlotUtilizationRulePreferences( const PredefinedRuleConfiguration& predefinedRuleConfig, MatchmakingCriteriaData &criteria, StartMatchmakingScenarioRequest& scenarioReq) const;
    void setPlayerCountRulePreferences( const PredefinedRuleConfiguration& predefinedRuleConfig, MatchmakingCriteriaData &criteria, StartMatchmakingScenarioRequest& scenarioReq, uint16_t myGroupSize, bool isGBList) const;
    void setTotalPlayerSlotsRulePreferences( const PredefinedRuleConfiguration& predefinedRuleConfig, MatchmakingCriteriaData &criteria, StartMatchmakingScenarioRequest& scenarioReq, uint16_t myGroupSize, bool isGBList) const;
    void setFreePlayerSlotsRulePreferences(const PredefinedRuleConfiguration& config, MatchmakingCriteriaData &criteria, StartMatchmakingScenarioRequest& scenarioReq) const;

    void setGameNameRulePreferences(const PredefinedRuleConfiguration &predefinedRuleConfig, MatchmakingCriteriaData &criteria, StartMatchmakingScenarioRequest& scenarioReq) const;
    static uint64_t mGameNameRuleDataStringsUseCountForCreate;
    static uint64_t mGameNameRuleDataStringsUseCountForSearch;
}; // End Class


} // End Stress namespace
} // End Blaze namespace

#endif // BLAZE_STRESS_MATCHMAKERRULE_UTIL

