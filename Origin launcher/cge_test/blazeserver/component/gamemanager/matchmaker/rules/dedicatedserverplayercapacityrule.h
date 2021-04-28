/*! ************************************************************************************************/
/*!
    \file   dedicatedserverplayercapacityrule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_DEDICATED_SERVER_PLAYER_CAPACITY_RULE
#define BLAZE_MATCHMAKING_DEDICATED_SERVER_PLAYER_CAPACITY_RULE

#include "gamemanager/matchmaker/rules/dedicatedserverplayercapacityruledefinition.h"
#include "gamemanager/matchmaker/rules/simplerule.h"

namespace Blaze
{
namespace GameManager
{
    class GameSession; 

namespace Matchmaker
{
    class Matchmaker;
    class DedicatedServerPlayerCapacityRuleDefinition;
    struct MatchmakingSupplementalData;

    /*! ************************************************************************************************/
    /*!
    \brief Blaze Matchmaking predefined rule.  This rule attempts to match games that have a capacity
            that is within the range of a dedicated server.


    *************************************************************************************************/
    class DedicatedServerPlayerCapacityRule : public SimpleRule
    {
    public:
        DedicatedServerPlayerCapacityRule(const DedicatedServerPlayerCapacityRuleDefinition &definition, MatchmakingAsyncStatus* status);
        DedicatedServerPlayerCapacityRule(const DedicatedServerPlayerCapacityRule &other, MatchmakingAsyncStatus* status);

        ~DedicatedServerPlayerCapacityRule() override {}

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW DedicatedServerPlayerCapacityRule(*this, matchmakingAsyncStatus); }

        /*! ************************************************************************************************/
        /*!
            \brief Initialize the rule from the MatchmakingCriteria's DedicatedServerPlayerCapacityRuleCriteria
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

        const DedicatedServerPlayerCapacityRuleDefinition& getDefinition() const { return static_cast<const DedicatedServerPlayerCapacityRuleDefinition&>(mRuleDefinition); }

    protected:

        //Calculation functions from SimpleRule.
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void postEval(const Rule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const override {}
        FitScore calcWeightedFitScore(bool isExactMatch, float fitPercent) const override;
        void calcSessionMatchInfo(const Rule& otherRule, const EvalInfo& evalInfo, MatchInfo& matchInfo) const override {}


    private:
        DedicatedServerPlayerCapacityRule(const DedicatedServerPlayerCapacityRule& rhs);
        DedicatedServerPlayerCapacityRule &operator=(const DedicatedServerPlayerCapacityRule &);

        uint16_t mDesiredCapacity;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_DESIRED_PLAYER_COUNT
