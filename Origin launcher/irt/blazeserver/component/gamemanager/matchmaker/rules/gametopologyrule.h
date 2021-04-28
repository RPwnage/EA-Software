/*! ************************************************************************************************/
/*!
    \file   gametopologyrule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_GAME_TOPOLOGY_RULE
#define BLAZE_MATCHMAKING_GAME_TOPOLOGY_RULE

#include "gamemanager/matchmaker/rules/gametopologyruledefinition.h"
#include "gamemanager/matchmaker/rules/simplerule.h"

namespace Blaze
{
namespace GameManager
{
    class GameSession; 

namespace Matchmaker
{
    class Matchmaker;
    class GameTopologyRuleDefinition;
    struct MatchmakingSupplementalData;

    /*! ************************************************************************************************/
    /*!
    \brief Blaze Matchmaking predefined rule.  This rule attempts to match games that have the desired 
           topology.

    *************************************************************************************************/
    class GameTopologyRule : public SimpleRule
    {
    public:
        GameTopologyRule(const GameTopologyRuleDefinition &definition, MatchmakingAsyncStatus* status);
        GameTopologyRule(const GameTopologyRule &other, MatchmakingAsyncStatus* status);

        ~GameTopologyRule() override {}

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW GameTopologyRule(*this, matchmakingAsyncStatus); }

        /*! ************************************************************************************************/
        /*!
            \brief Initialize the rule from the MatchmakingCriteria's GameTopologyRuleCriteria
                (defined in the startMatchmakingRequest). Returns true on success, false otherwise.

            \param[in]  criteriaData - data required to initialize the matchmaking rule.
            \param[in]  matchmakingSupplementalData - used to lookup the rule definition
            \param[out] err - errMessage is filled out if initialization fails.
            \return true on success, false on error (see the errMessage field of err)
        *************************************************************************************************/
        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;

        bool isDisabled() const override;

        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;

        bool isMatchAny() const override { return false; }

        bool isDedicatedServerEnabled() const override { return true; }

        /*! ************************************************************************************************/
        /*! \brief update matchmaking status for the rule to current status
        ****************************************************************************************************/
        void updateAsyncStatus() override {}

        const GameTopologyRuleDefinition& getDefinition() const { return static_cast<const GameTopologyRuleDefinition&>(mRuleDefinition); }

    protected:

        //Calculation functions from SimpleRule.
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void postEval(const Rule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const override {}
        FitScore calcWeightedFitScore(bool isExactMatch, float fitPercent) const override;
        void calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const override {}


    private:
        GameTopologyRule(const GameTopologyRule& rhs);
        GameTopologyRule &operator=(const GameTopologyRule &);

        GameNetworkTopology mDesiredTopology;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_GAME_TOPOLOGY_RULE
