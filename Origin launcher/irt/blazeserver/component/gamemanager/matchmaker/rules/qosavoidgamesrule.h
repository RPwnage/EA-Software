/*! ************************************************************************************************/
/*!
    \file   qosavoidgamesrule.h


    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_QOS_AVOID_GAMES_RULE
#define BLAZE_MATCHMAKING_QOS_AVOID_GAMES_RULE

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/matchmaker/rules/qosavoidgamesruledefinition.h"
#include "EASTL/hash_set.h" // for GameIdSet

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief Predefined matchmaking rule dealing with player list searches.
    ***************************************************************************************************/
    class QosAvoidGamesRule : public SimpleRule
    {
        NON_COPYABLE(QosAvoidGamesRule);
    public:

        //! \brief You must initialize the rule before using it.
        // by default it matches any game
        QosAvoidGamesRule(const QosAvoidGamesRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        QosAvoidGamesRule(const QosAvoidGamesRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        //! \brief Destructor
        ~QosAvoidGamesRule() override {}

        //////////////////////////////////////////////////////////////////////////
        // from Rule
        
        /*! ************************************************************************************************/
        /*! \brief Initialize the required rule by the matchmaking criteria.
            \param[in]     criteriaData data required to initialize the matchmaking rule.
            \param[in]     matchmakingSupplementalData supplemental data about matchmaking not passed in through the request and criteriaData.
            \param[in/out] err error message returned on initialization failure.
            \return true on success, false on error (see the errMessage field of err)
        *************************************************************************************************/
        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        bool isDisabled() const override { return mGameIdSet.empty(); }

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        /*! QosAvoidGamesRule is not time sensitive so don't need to calculate anything here. */
        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override { return NO_RULE_CHANGE; }

        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override { WARN_LOG("[QosAvoidGamesRule].addConditions disabled, non rete"); return true; } // non rete
        bool isMatchAny() const override { return false; }
        bool isDedicatedServerEnabled() const override { return false; }

    protected:

        const QosAvoidGamesRuleDefinition& getDefinition() const { return static_cast<const QosAvoidGamesRuleDefinition&>(mRuleDefinition); }

        //////////////////////////////////////////////////////////////////////////
        // SimpleRule implementations
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        // end SimpleRule
        //////////////////////////////////////////////////////////////////////////

        void updateAsyncStatus() override {} // NO_OP. rule is a filter w/o decay

    private:
        typedef eastl::hash_set<GameId> GameIdSet;
        void gameIdListToSet(const Blaze::GameManager::GameIdList& inputList, GameIdSet& result) const;

        uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const override;

    private:
        GameIdSet mGameIdSet;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_QOS_AVOID_GAMES_RULE
#endif
