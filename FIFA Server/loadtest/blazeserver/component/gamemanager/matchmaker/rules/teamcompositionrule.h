/*! ************************************************************************************************/
/*!
    \file teamcompositionrule.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_TEAM_COMPOSITION_RULE_H
#define BLAZE_MATCHMAKING_TEAM_COMPOSITION_RULE_H

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/matchmaker/rules/teamcompositionruledefinition.h"
#include "gamemanager/matchmaker/creategamefinalization/matchedsessionsbucket.h"


namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class Matchmaker;
    class CreateGameFinalizationSettings;
    class CreateGameFinalizationTeamInfo;
    struct MatchmakingSupplementalData;
 
    class TeamCompositionRule : public SimpleRule
    {
        NON_COPYABLE(TeamCompositionRule);
    public:
        
        TeamCompositionRule(const TeamCompositionRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        TeamCompositionRule(const TeamCompositionRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~TeamCompositionRule() override;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        const TeamCompositionRuleDefinition& getDefinition() const { return static_cast<const TeamCompositionRuleDefinition&>(mRuleDefinition); }

        bool isEvaluateDisabled() const override { return true; }//eval others vs me even if I'm disabled to ensure don't match if can't (if mm group size not acceptable etc)
        bool isDedicatedServerEnabled() const override { return false; }
 
        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;
        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override;
        void updateAsyncStatus() override;

        uint16_t getTeamCapacityForRule() const { return getDefinition().getTeamCapacityForRule(); }
        const GameTeamCompositionsInfoVector& getAcceptableGameTeamCompositionsInfoVector() const;

        // SimpleRule
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        // Finalization Helpers
        const GameTeamCompositionsInfo* selectNextGameTeamCompositionsForFinalization(const CreateGameFinalizationSettings& finalizationSettings) const;
        const MatchedSessionsBucket* selectNextBucketForFinalization(const CreateGameFinalizationTeamInfo& teamToFill, const CreateGameFinalizationSettings& finalizationSettings, const MatchedSessionsBucketMap& bucketMap) const;
        bool areFinalizingTeamsAcceptableToCreateGame(const CreateGameFinalizationSettings& finalizationSettings) const;

        const char8_t* getCurrentAcceptableGtcVectorInfoCacheLogStr() const { return ((mGroupAndFitThresholdMatchInfoCache == nullptr)? "<no acceptable game team compositions infos>" : mGroupAndFitThresholdMatchInfoCache->toLogStr()); }
        uint16_t getTeamCount() const { return getDefinition().getTeamCount(); }
    private:
        uint16_t getJoiningPlayerCount() const { return mJoiningPlayerCount; }

        float doCalcFitPercent(const TeamCompositionRule& otherTeamCompositionRule, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const;
        float doCalcFitPercent(const Search::GameSessionSearchSlave& gameSession, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const;

        void updateCachedTeamSelectionCriteria();

        // SimpleRule
        FitScore calcWeightedFitScore(bool isExactMatch, float fitPercent) const override;

        typedef eastl::vector<const GameTeamCompositionsInfo*> GameTeamCompositionsInfoPtrVector;
        const char8_t* gameTeamCompositionsInfoPtrVectorToLogStr(const GameTeamCompositionsInfoPtrVector& infoVector, eastl::string& buf, bool newlinePerItem = false) const;
        const char8_t* getSessionLogStr() const { return (mSessionLogStr.empty()? mSessionLogStr.sprintf("id=%" PRIu64 ",size=%u,FG=%s", getMMSessionId(), getJoiningPlayerCount(), (mIsFindMode? "true":"false")).c_str() : mSessionLogStr.c_str()); }

        // this post-RETE rule disabled for diagnostics. For CG finalization, has own metrics
        bool isRuleTotalDiagnosticEnabled() const override { return false; }
    private:
        uint16_t mJoiningPlayerCount;
        TeamSelectionCriteria* mCachedTeamSelectionCriteria;
        bool mIsFindMode;

        const TeamCompositionRuleDefinition::GroupAndFitThresholdMatchInfoCache* mGroupAndFitThresholdMatchInfoCache;
        GameTeamCompositionsInfoVector mEmptyAcceptableGameTeamCompositionsInfoVector;//sentinel
        TeamCompositionRuleDefinition::GroupSizeSet mEmptyAcceptableOtherSessionGroupSizesSet;

        mutable eastl::string mSessionLogStr;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_TEAM_COMPOSITION_RULE_H
