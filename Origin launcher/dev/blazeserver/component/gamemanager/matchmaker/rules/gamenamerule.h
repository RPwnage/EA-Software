/*! ************************************************************************************************/
/*!
    \file   gamenamerule.h


    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_GAME_NAME_RULE
#define BLAZE_MATCHMAKING_GAME_NAME_RULE

#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmaker/rules/gamenameruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief Predefined matchmaking rule (for GameBrowser) dealing with game name substring searches.
    ***************************************************************************************************/
    class GameNameRule : public Rule
    {
        NON_COPYABLE(GameNameRule);
    public:

        //! \brief You must initialize the rule before using it.
        // by default it matches any game
        GameNameRule(const GameNameRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        GameNameRule(const GameNameRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        //! \brief Destructor
        ~GameNameRule() override {}

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

        FitScore evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const override;
        // this rule does not evaluate for create game
        void evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const override;
        /*! returns true if this rule is disabled.  This rule is disabled if the default value for game name substring search is passed in */
        bool isDisabled() const override;

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        /*! GameNameRule is not time sensitive so don't need to calculate anything here. */
        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override { return NO_RULE_CHANGE; }

        /*! GameNameRule doesn't contribute to the fit score because it is a GB filtering rule, returns 0. */
        FitScore getMaxFitScore() const override { return 0; }
        //////////////////////////////////////////////////////////////////////////


        //////////////////////////////////////////////////////////////////////////
        // RETE implementations
        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;
        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, GameManager::Rete::ConditionBlockId blockId, GameManager::Matchmaker::ReadableRuleConditions& debugRuleConditions) const override;
        bool isMatchAny() const override { return (mCurrentMinFitThreshold == ZERO_FIT); }
        bool isDedicatedServerEnabled() const override { return true; }
        // end RETE
        //////////////////////////////////////////////////////////////////////////

    protected:

        const GameNameRuleDefinition& getDefinition() const { return static_cast<const GameNameRuleDefinition&>(mRuleDefinition); }

        //////////////////////////////////////////////////////////////////////////
        // from Rule
        //////////////////////////////////////////////////////////////////////////
        void updateAsyncStatus() override;

    private:
        FitScore evaluate(const Search::GameSessionSearchSlave& game, ReadableRuleConditions& debugRuleConditions) const;

        bool isRuleTotalDiagnosticEnabled() const override { return false; }

    private:
        Blaze::GameManager::GameName mSearchString;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

// BLAZE_MATCHMAKING_GAME_NAME_RULE
#endif
