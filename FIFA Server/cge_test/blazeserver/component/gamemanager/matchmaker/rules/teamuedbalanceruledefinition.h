/*! ************************************************************************************************/
/*!
    \file   teamUedruledefinition.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_UED_BALANCE_RULE_DEFINITION
#define BLAZE_MATCHMAKING_TEAM_UED_BALANCE_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/rangelistcontainer.h"
#include "gamemanager/matchmaker/groupuedexpressionlist.h"


namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class RangeList;
    class FitFormula;
    class RangeListContainer;
    class MatchmakingGameInfoCache;
    class TeamUEDBalanceRule;

    class TeamUEDBalanceRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(TeamUEDBalanceRuleDefinition);
        DEFINE_RULE_DEF_H(TeamUEDBalanceRuleDefinition, TeamUEDBalanceRule);
    public:

        static const char8_t* TEAMUEDBALANCE_ATTRIBUTE_IMBALANCE;
        static const char8_t* TEAMUEDBALANCE_ATTRIBUTE_TOP_TEAM;
        static const char8_t* TEAMUEDBALANCE_ATTRIBUTE_2ND_TOP_TEAM;

        // Maps rule name to its RuleDefinitionId. Used to identify the different possible configured TeamUEDBalanceRule's.
        typedef eastl::hash_map<eastl::string, RuleDefinitionId, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> RuleNameToRuleDefinitionIdMap;

    public:
        TeamUEDBalanceRuleDefinition();
        ~TeamUEDBalanceRuleDefinition() override;

        bool initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig) override;

        // Note: we can disable by setting bucketSize to max for rete
        bool isReteCapable() const override { return (mRuleConfigTdf.getBucketPartitions() != 0); }

        /*! \brief Returns true to ensure post-RETE FindGame evaluation is done to precisely filter joinable games
            (RETE serves as a broad filter, but its typically bucketed, does not accurately account for the team you'll join, etc) */
        bool callEvaluateForMMFindGameAfterRete() const override { return true; }

        // RETE impl
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        // end RETE

        void updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const override;
        
        float getFitPercent(int64_t desiredValue, int64_t actualValue) const;

        const RangeList* getRangeOffsetList(const char8_t *listName) const;
        
        bool isDisabled() const override { return mRangeListContainer.empty(); }

        bool getSessionMaxOrMinUEDValue(bool getMax, UserExtendedDataValue& uedValue, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersSessionInfo) const;

        /*! ************************************************************************************************/
        /*!
            \brief calculates the UED value for the user or group depending on which is valid.  If the sessionItr
                is nullptr then defer to the ownerSession.  If the ownerSession is also nullptr then this function returns
                false.  Both sessionItr and ownerSession cannot be nullptr.
        
            \param[out] uedValue - The calculated UED Value
            \param[in] ownerBlazeId - The blaze Id of the owner, used to calculate if a UED value is individual
            \param[in] ownerGroupSize - The group size of the calling owner.
            \param[in] membersSessionInfo - The UserSessionInfo of a group of users used to calculate the group UED value. If the individual UserSessionInfos have a mGroupSize of 0, we use the owner group size.
        
            \return false if the value fails to be calculated (indicates a system error)
        *************************************************************************************************/
        bool calcUEDValue(UserExtendedDataValue& uedValue, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersSessionInfo) const;

        bool recalcTeamUEDValue(UserExtendedDataValue& uedValue, uint16_t preExistingTeamSize, UserExtendedDataValue joinOrLeavingUedValue, uint16_t joinOrLeavingSessionSize, bool isLeavingSession, TeamUEDVector& memberUEDValues, TeamIndex teamIndex) const;
        bool calcDesiredJoiningMemberUEDValue(UserExtendedDataValue& uedValue, uint16_t otherMemberSize, uint16_t preExistingTeamSize, UserExtendedDataValue preExistingUedValue, UserExtendedDataValue targetUedValue) const;
        
        UserExtendedDataValue calcGameImbalance(const Search::GameSessionSearchSlave& gameSession, UserExtendedDataValue& lowestTeamUED, UserExtendedDataValue& highestTeamUED, const TeamUEDBalanceRule* joiningSessionRule = nullptr, TeamIndex joiningTeamIndex = UNSPECIFIED_TEAM_INDEX, bool ignoreEmptyTeams = false) const;

        bool normalizeUEDValue(UserExtendedDataValue& value, bool warnOnClamp = false) const;

        int64_t getGlobalMaxTeamUEDImbalance() const { return mGlobalMaxTeamUEDImbalance; }

        const char8_t* getUEDName() const { return mRuleConfigTdf.getUserExtendedDataName(); }
        UserExtendedDataKey getUEDKey() const;
        GroupValueFormula getTeamValueFormula() const { return mRuleConfigTdf.getTeamValueFormula(); }
        const GroupUedExpressionList& getGroupUedExpressionList() const { return mGroupUedExpressionList; } 

        UserExtendedDataValue getMinRange() const { return mRuleConfigTdf.getRange().getMin(); }
        UserExtendedDataValue getMaxRange() const { return mRuleConfigTdf.getRange().getMax(); }

        uint16_t getMaxFinalizationPickSequenceRetries() const { return mRuleConfigTdf.getMaxFinalizationPickSequenceRetries(); }

        const char8_t* wmeAttributeNameToString(Rete::WMEAttribute attribute) const { const char8_t* str = getStringTable().getStringFromHash(attribute); return ((str!=nullptr)? str : ""); }
        
    private:
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        void buildWmesMaxOrMin(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSession, UserExtendedDataValue lowestTeamUED, UserExtendedDataValue highestTeamUED, UserExtendedDataValue actualUEDImbal, bool doUpsert) const;
        void buildWmesSum(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSession, UserExtendedDataValue lowestTeamUED, UserExtendedDataValue highestTeamUED, UserExtendedDataValue actualUEDImbal, bool doUpsert) const;
        UserExtendedDataValue get2ndFromTopUedValuedTeam(const Search::GameSessionSearchSlave& gameSession, UserExtendedDataValue lowestTeamUED, UserExtendedDataValue highestTeamUED) const;
        bool calcUnNormalizedTeamUEDValue(UserExtendedDataValue& unNormalizedUedValue, const TeamUEDVector& memberUEDValues) const;
    private:
        TeamUEDBalanceRuleConfig mRuleConfigTdf;
        RangeListContainer mRangeListContainer;
        GroupUedExpressionList mGroupUedExpressionList;
        FitFormula* mFitFormula;

        mutable Blaze::UserExtendedDataKey mDataKey; // Lazy init requires mutable.
        int64_t mGlobalMaxTeamUEDImbalance;
        UserExtendedDataValue mBucketSize;
    };
    
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_UED_BALANCE_RULE_DEFINITION
