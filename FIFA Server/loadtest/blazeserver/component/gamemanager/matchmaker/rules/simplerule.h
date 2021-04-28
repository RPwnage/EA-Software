/*! ************************************************************************************************/
/*!
\file   simplerule.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_SIMPLE_RULE
#define BLAZE_SIMPLE_RULE

#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmaker/rangelist.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ******************************************************************************/
    /*!
        SimpleRule is a class that takes care of most of the work needed to implement
        a rule for Game Browser and Matchmaker.  It handles the internals of the
        evaluate functions for GB and MM so that derived rules only have to implement
        the calculation of the fit percent & exact match properties & the failure 
        message to output on a mismatch.

        See the Dev Guide for Custom Matchmaking for more information on this class.
    **********************************************************************************/
    class SimpleRule : public Rule
    {
        NON_COPYABLE(SimpleRule);
    public:
        SimpleRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        SimpleRule(const SimpleRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~SimpleRule() override {}

    protected:
        /*********************************************************************/
        /*! \brief Determines if this Rule should be enforced bidirectionally
            
            TRUE then 2 sessions match if and only if both sessions match
            each other. This rule would be a requirement that must be met
            by both sessions to ensure a match.
            
            ex. Session A matches Session B, but B does not match session A.
            This would result in the two sessions not matching.
            
            NOTE: if the sessions don't match now, the requirements still might
            decay and allow these rules to match later when both requirements
            are met.

            FALSE then 2 sessions match if either session matches the other.
            This rule would be a preference that could be met by either rule.
            ex. Session A matches Session B, but B does not match session A.
            This would result in the two sessions matching regardless of B's
            preferences.

            The default is true.
        **********************************************************************/
        virtual bool isBidirectional() const { return true; }

        //////////////////////////////////////////////////////////////////////////
        // MM evaluate Game
        //////////////////////////////////////////////////////////////////////////
    public:

        /*! Evaluate this rule against a game in find game MM. */
        FitScore evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const override;

        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, GameManager::Rete::ConditionBlockId blockId, GameManager::Matchmaker::ReadableRuleConditions& debugRuleConditions) const override;
    protected:
        /*! ************************************************************************************************/
        /*! \brief Helper function used by both MM and GB gameSession evaluate methods.  Handles
            the common logic between these two methods.
        ****************************************************************************************************/
        virtual FitScore evaluateGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const;
        
        /*! ************************************************************************************************/
        /*! \brief Calculate the fitpercent and exact match for the Game Session.  This method is called
            for evaluateForMMFindGame to determine the fitpercent
            for a game.

            This method is expected to be implemented by deriving classes for Game Browser and MM Find Game.

            \param[in] gameSession The GameSession to calculate the fitpercent and exact match.
            \param[out] fitPercent The fitpercent of the GameSession given the rule criteria.
            \param[out] isExactMatch If the match to this Game Session is an exact match.
        ****************************************************************************************************/
        virtual void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch) const;

        /*! ************************************************************************************************/
        /*! \brief override that also provides debug info.  Default implementation calls other version with out debug info.
        ****************************************************************************************************/
        virtual void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const;


        //////////////////////////////////////////////////////////////////////////
        // MM evaluate Session
        //////////////////////////////////////////////////////////////////////////
    public:
        /*! Evaluate this rule against the other sessions rule. */
        void evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const override;

        /*! If false, a rule will skip evaluation when one session has the rule disabled. */
        bool isEvaluateDisabled() const override { return false; }

    protected:
        /*! Helper function used to evaluate the two rules */
        void evaluateRule(const Rule& otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const;

        /*! ************************************************************************************************/
        /*! \brief Calculate the fitpercent and exact match for the Game Session.  This method is called
            for evaluateForMMFindGame to determine the fitpercent
            for a game.

            This method is expected to be implemented by deriving classes for Game Browser and MM Find Game.

            \param[in] gameSession The GameSession to calculate the fitpercent and exact match.
            \param[out] fitPercent The fitpercent of the GameSession given the rule criteria.
            \param[out] isExactMatch If the match to this Game Session is an exact match.
        ****************************************************************************************************/
        virtual void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch) const;

        /*! ************************************************************************************************/
        /*! \brief override that also provides debug info.  Default implementation calls other version with out debug info.
        ****************************************************************************************************/
        virtual void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const;

        
        /*! ************************************************************************************************/
        /*! \brief Access to the rule and eval info return false to early out skipping further evaluations.
            
            NOTE: On failure pf preEval no further evaluation of this rule will happen.  This means on failure
            it is up to preEval to fill out the evalInfo and matchInfo before returning.

            By Default this method does nothing.

            \param[in] otherRule The other rule to pre evaluate.
            \param[out] evalInfo The EvalInfo to update if we fail the preEval.
            \param[out] matchInfo The MatchInfo to update if we fail the preEval.

            \return true on success and false if we failed preeval, default is true.
        ****************************************************************************************************/
        virtual bool preEval(const Rule& otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const { return true; }
        
        /*! ************************************************************************************************/
        /*! \brief Access to the rule and eval info to fix up results after evaluations.  This function
            is used to make any modifications to the eval or match info before the rule finishes evaluation
            of the evaluateForMMCreateGame.  This can be used for restrictive one way matches where only
            one sessio should be allowed to pull the other session in.  This method is called for both
            this rule and the other rule so only need to worry about my match info.

            \param[in] otherRule The other rule to post evaluate.
            \param[in/out] evalInfo The EvalInfo to update after evaluations are finished.
            \param[in/out] matchInfo The MatchInfo to update after evaluations are finished.
        ****************************************************************************************************/
        virtual void postEval(const Rule& otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const {}

        //////////////////////////////////////////////////////////////////////////
        // MM evaluate CreateGameRequest
        //////////////////////////////////////////////////////////////////////////
    public:

        /*! Evaluate this rule against a create game request during create game finalization. */
        FitScore evaluateForMMCreateGameFinalization(const Blaze::GameManager::MatchmakingCreateGameRequest& matchmakingCreateGameRequest, ReadableRuleConditions& ruleConditions) const override { return 0; }

    protected:
        /*! ************************************************************************************************/
        /*! \brief Helper function used by CreateGameFinalization to ensure the pending CreateGameRequest is an acceptable match
        ****************************************************************************************************/
        virtual FitScore evaluateCreateGameRequest(const Blaze::GameManager::MatchmakingCreateGameRequest& matchmakingCreateGameRequest, ReadableRuleConditions& ruleConditions) const;
        
        /*! ************************************************************************************************/
        /*! \brief Calculate the fitpercent and exact match for the CreateGame request.  This method is called
            by evaluateCreateGameRequest to determine the fitpercent for the request.

            This method is expected to be implemented by deriving classes for create game finalization, if finalization events,
            such as voting, could produce an unacceptable match

            \param[in] evaluateCreateGameRequest The CreateGameRequest to calculate the fitpercent and exact match.
            \param[out] fitPercent The fitpercent of the GameSession given the rule criteria.
            \param[out] isExactMatch If the match to this CreateGameRequest is an exact match.
        ****************************************************************************************************/
        virtual void calcFitPercent(const Blaze::GameManager::MatchmakingCreateGameRequest& matchmakingCreateGameRequest, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& ruleConditions) const { fitPercent = 0.0; isExactMatch = true; }


    };

    /*
        Helper class to deal with the range offset list variations of the simple rule:
            Used by PlayerCountRule, GeoLocationRule, PlayerSlotUtil
    */
    class SimpleRangeRule : public SimpleRule
    {
    public:
        SimpleRangeRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        SimpleRangeRule(const SimpleRangeRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~SimpleRangeRule() override {}


        bool isDisabled() const override { return mRangeOffsetList == nullptr; }
        bool isMatchAny() const override { return (mCurrentRangeOffset != nullptr) && mCurrentRangeOffset->isMatchAny(); }

        FitScore calcWeightedFitScore(bool isExactMatch, float fitPercent) const override;
        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override;
        virtual void updateAcceptableRange() = 0;

    protected:
        FitScore evaluateGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const override;

        const RangeList *mRangeOffsetList; // this rule has a rangeOffsetList instead of a minFitThreshold.
        const RangeList::Range *mCurrentRangeOffset;
        RangeList::Range mAcceptableRange; // updated when the rule decays
    };
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_SIMPLE_RULE
