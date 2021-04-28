/*! ************************************************************************************************/
/*!
    \file   teamuedpositionparityruledefinition.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_UED_POSTITION_PARITY_RULE_DEFINITION
#define BLAZE_MATCHMAKING_TEAM_UED_POSTITION_PARITY_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/rangelistcontainer.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h" // for MatchmakingGameInfoCache::TeamIndividualUEDValues in calcFitPercent()

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class RangeList;
    class GroupUedExpressionList;
    class FitFormula;
    class RangeListContainer;
    class TeamUEDPositionParityRule;

    class TeamUEDPositionParityRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(TeamUEDPositionParityRuleDefinition);
        DEFINE_RULE_DEF_H(TeamUEDPositionParityRuleDefinition, TeamUEDPositionParityRule);
    public:

        // Maps rule name to its RuleDefinitionId. Used to identify the different possible configured TeamUEDPositionParityRules.
        typedef eastl::hash_map<eastl::string, RuleDefinitionId, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> RuleNameToRuleDefinitionIdMap;

    public:
        TeamUEDPositionParityRuleDefinition();
        ~TeamUEDPositionParityRuleDefinition() override;

        bool initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig) override;

        // This rule doesn't use the RETE tree
        bool isReteCapable() const override { return false; }

        /*! \brief Returns true to ensure post-RETE FindGame evaluation is done. */
        bool callEvaluateForMMFindGameAfterRete() const override { return true; }

        // RETE impl
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override {}
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override {}
        // end RETE

        void updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const override;
        
        float calcFitPercent(const MatchmakingGameInfoCache::TeamIndividualUEDValues& teamUedValues, uint16_t teamCapacity, TeamIndex joiningTeamIndex, const TeamUEDVector& joiningTeamUeds, 
            UserExtendedDataValue topPlayersMaxDifference, UserExtendedDataValue bottomPlayersMaxDifference, ReadableRuleConditions& debugRuleConditions) const;
        float getFitPercent(int64_t desiredValue, int64_t actualValue) const;

        const RangeList* getRangeOffsetList(const char8_t *listName) const;
        
        bool isDisabled() const override { return mRangeListContainer.empty(); }

        const char8_t* getUEDName() const { return mRuleConfigTdf.getUserExtendedDataName(); }
        UserExtendedDataKey getUEDKey() const;

        UserExtendedDataValue getMinRange() const { return mRuleConfigTdf.getRange().getMin(); }
        UserExtendedDataValue getMaxRange() const { return mRuleConfigTdf.getRange().getMax(); }

        uint16_t getTopPlayerCount() const { return mRuleConfigTdf.getTopPlayersToCompare(); }
        uint16_t getBottomPlayerCount() const { return mRuleConfigTdf.getBottomPlayersToCompare(); }

        uint16_t getMaxFinalizationPickSequenceRetries() const { return mRuleConfigTdf.getMaxFinalizationPickSequenceRetries(); }
        
    private:
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        bool getHighestAndLowestUEDValueAtRank(uint16_t rankIndex, const MatchmakingGameInfoCache::TeamIndividualUEDValues& teamUedValues, TeamIndex joiningTeamIndex, const TeamUEDVector& joiningTeamUeds, 
            UserExtendedDataValue& highestUedValue, UserExtendedDataValue& lowestUedValue) const;

    private:
        TeamUEDPositionParityRuleConfig mRuleConfigTdf;
        RangeListContainer mRangeListContainer;
        FitFormula* mFitFormula;
        mutable Blaze::UserExtendedDataKey mDataKey; // Lazy init requires mutable.

    };
    
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_UED_POSTITION_PARITY_RULE_DEFINITION
