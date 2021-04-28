/*! ************************************************************************************************/
/*!
    \file   Platformrule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_PLATFORM_RULE
#define BLAZE_MATCHMAKING_PLATFORM_RULE

#include "gamemanager/matchmaker/rules/platformruledefinition.h"
#include "gamemanager/matchmaker/rules/simplerule.h"
#include "framework/controller/controller.h"

namespace Blaze
{
namespace GameManager
{
    class GameSession; 

namespace Matchmaker
{
    class Matchmaker;
    class PlatformRuleDefinition;
    struct MatchmakingSupplementalData;

    /*! ************************************************************************************************/
    /*!
    \brief Blaze Matchmaking predefined rule.  This rule attempts to match games that have the desired platform.

    *************************************************************************************************/
    class PlatformRule : public SimpleRule
    {
    public:
        PlatformRule(const PlatformRuleDefinition &definition, MatchmakingAsyncStatus* status);
        PlatformRule(const PlatformRule &other, MatchmakingAsyncStatus* status);

        ~PlatformRule() override {}

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW PlatformRule(*this, matchmakingAsyncStatus); }

        /*! ************************************************************************************************/
        /*!
            \brief Initialize the rule from the MatchmakingCriteria's PlatformRuleCriteria
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

        bool isDedicatedServerEnabled() const override { return false; }

        /*! ************************************************************************************************/
        /*! \brief update matchmaking status for the rule to current status
        ****************************************************************************************************/
        void updateAsyncStatus() override {}

        const PlatformRuleDefinition& getDefinition() const { return static_cast<const PlatformRuleDefinition&>(mRuleDefinition); }

    protected:

        //Calculation functions from SimpleRule.
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
        void postEval(const Rule &otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const override {}
        FitScore calcWeightedFitScore(bool isExactMatch, float fitPercent) const override;


    private:
        PlatformRule(const PlatformRule& rhs);
        PlatformRule &operator=(const PlatformRule &);

        bool mCrossplayMustMatch;
        bool mIgnoreCallerPlatform;
        ClientPlatformSet mPlatformsInUse;
        ClientPlatformTypeList mPlatformListOverride;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_PLATFORM_RULE
