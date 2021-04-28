/*! ************************************************************************************************/
/*!
\file   rule.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_RULE
#define BLAZE_RULE

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/diagnostics/diagnosticstallyutils.h"
#include "gamemanager/rete/reterule.h"

namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class MinFitThresholdList;
    struct MatchmakingSupplementalData;
    class MatchmakingCriteria;

    /*! ************************************************************************************************/
    /*! \brief Base class (interface) for all game manager rules.
        
        For more information see the Matchmaking Dev Guide located on confluence.
    *************************************************************************************************/
    class Rule : public Rete::ReteRule
    {
        // Friend of MM Criteria to set the associated MM Session.
        friend class MatchmakingCriteria;

        // MM_AUDIT: When MM subsessions is refactored look into grabbing the matchmakingAsyncStatus from the associated MM Session.
        NON_COPYABLE(Rule);  // The Rule class does not have a standard copy ctor, needs the matchmaking Async Status.
    public:
        /*! ************************************************************************************************/
        /*! \brief Ctor for the Rule class
            
            NOTE: All rules must be initialized before use.
        ****************************************************************************************************/
        Rule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        
        /*! ************************************************************************************************/
        /*! \brief Copy Ctor for the Rule Class
        ****************************************************************************************************/
        Rule(const Rule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        //! \brief destructor
        ~Rule() override {}

        // Base functions

        /*! ************************************************************************************************/
        /*! \brief initialize the rule given the matchmaking criteria data sent from the client.

            Uses the specific matchmaking criteria data for this rule to customize how this rule evaluates
            the other matchmaking sessions and game sessions.  The MatchmakingCriteriaData object should not
            be stored off, but any of the data needed can be copied to member data on the rule.  Also, any
            caching of data that needs to be calculated should be done here.  Caching off values here can
            help a matchmaking rule run more efficiently.

            NOTE: this function is called by matchmaker and gamebrowser.  To distinguish between the two
            during a call from gamebrowser the matchmakingsession pointer will be null.  Anything requiring
            the matchmakingsession should not be needed for a gamebrowser evaluation.
            
            \param[in] criteriaData The MatchmakingCriteriaData passed up by the client.
            \param[out] err An error message to be passed out in the event of initialization failure.
            \return false if there was an error initializing this rule.
        ****************************************************************************************************/
        virtual bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) = 0;

        
        /*! ************************************************************************************************/
        /*! \brief evaluates this rule against a GameSession during Matchmaking Find Game evaluation.
            
            Compares the values on the GameSession against the rule criteria and determines if the
            game is a fit for this rule.  If it is a fit it returns a fitscore (float) which is the
            rule weight multipied by the fitpercent a value between 0.0f and 1.0f, where 0.0f is the
            worst possible match and 1.0f is the best possible match.  If the fitpercent is >= the current
            threshold then the fitscore is returned.  If the fitpercent is < the current threshold then
            NO_MATCH is returned.  The thresholds can decay over time allowing games with lower fitpercents
            to match after a period of time determined by the configuration for the rule.

            \param[in] gameSession The GameSession to evaluate and determine a fitScore.
            \param[in/out] debugRuleConditions Readable Debug information about the matchmaking evaluations that occurred.

            \return The fitscore determining how good of a match this GameSession is given the criteria or NO_MATCH
                if the fitpercent does not meet the threshold.
        ****************************************************************************************************/
        virtual FitScore evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const = 0;

        /*! ************************************************************************************************/
        /*! \brief evaluates this rule against a GameSession during Matchmaking Create Game evaluation.
            
            Compares the values on the GameSession against the rule criteria and determines if the
            game is a fit for this rule.  If it is a fit it returns a fitscore (float) which is the
            rule weight multipied by the fitpercent a value between 0.0f and 1.0f, where 0.0f is the
            worst possible match and 1.0f is the best possible match.  If the fitpercent is >= the current
            threshold then the fitscore is returned.  If the fitpercent is < the current threshold then
            NO_MATCH is returned.  The thresholds can decay over time allowing games with lower fitpercents
            to match after a period of time determined by the configuration for the rule.

            Matchmaking rule criteria does not change over the course of a sessions life cycle.  Only
            the thresholds can change so all evaluations will give the same fitscore.  Sessions are evaluated
            only once and a fitscore and time to match will be calculated.  The time to match is determined
            by walking the threshold list and determining at what time the fitpercent would match the threshold.
            If two sessions never match then they will have a fitscore NO_MATCH and a time NO_MATCH_TIME_SECONDS.
            
            NOTE: It is possible for a session to match another session but not vice versa.

            \param[in] otherSession The other matchmaking session to evaluate against and determine the fitscore and match time.
            \param[out] sessionsEvalInfo Evaluation Information which contains fitpercent, other session, error message, and exact match flag.
            \param[out] sessionsMatchInfo Match information which contains the fitscore and time to match.
        ****************************************************************************************************/
        virtual void evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const = 0;
        
        /*! ************************************************************************************************/
        /*! \brief evaluates this rule against the request being used to create a new game session during CG finalization
            
        Compares the CreateGame request against the rule criteria and determines if the
        create game request is a fit for this rule.  If it is a fit it returns a fitscore (float) which is the
        rule weight multipied by the fitpercent a value between 0.0f and 1.0f, where 0.0f is the
        worst possible match and 1.0f is the best possible match.  If the fitpercent is >= the current
        threshold then the fitscore is returned.  If the fitpercent is < the current threshold then
        NO_MATCH is returned.  The thresholds can decay over time allowing games with lower fitpercents
        to match after a period of time determined by the configuration for the rule.

        IMPORTANT: Most rules can simply accept the CG request as is, a post-finalization evaluation is mainly appropriate
        for rules which may produce a result (ex: after voting) that may not meet the requirements of all involved users.

        \param[in] matchmakingCreateGameRequest The MatchmakingCreateGameRequest to evaluate and determine a fitScore.
        \param[out] ruleConditions - debug rule conditions that were evaluated.

        \return The fitscore determining how good of a match this GameSession is given the criteria or NO_MATCH
        if the fitpercent does not meet the threshold.
        ****************************************************************************************************/
        virtual FitScore evaluateForMMCreateGameFinalization(const Blaze::GameManager::MatchmakingCreateGameRequest& matchmakingCreateGameRequest, ReadableRuleConditions& ruleConditions) const { return 0; }

        /*! ************************************************************************************************/
        /*! \brief Clone the current rule copying the rule criteria using the copy ctor.  Used for subsessions.
        ****************************************************************************************************/
        virtual Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const = 0;

        /*! The rule name as defined by the definition and the configuration file. */
        const char8_t* getRuleName() const override { return mRuleDefinition.getName(); }
        const char8_t* getRuleType() const { return mRuleDefinition.getType(); }

        /*! returns true if this rule is disabled.  By default a rule is disabled if it has no MinFitThreshold List defined. */
        bool isDisabled() const override { return mMinFitThresholdList == nullptr; }

        virtual bool isEvaluateDisabled() const { return false; }

        virtual bool isReteCapable() const { return mRuleDefinition.isReteCapable(); }

        bool callEvaluateForMMFindGameAfterRete() const { return mRuleDefinition.callEvaluateForMMFindGameAfterRete(); }
        bool calculateFitScoreAfterRete() const override { return mRuleDefinition.calculateFitScoreAfterRete(); }

        bool isMatchAny() const override { return ((mCurrentMinFitThreshold == ZERO_FIT)); }

        /*! Returns the rule ID.  All rules of the same type (created by the same definition) have the same id. */
        const RuleDefinitionId getId() const { return mRuleDefinition.getID(); }
        const uint32_t getProviderSalience() const override { return getId(); }

        /*! Returns true if this rule is of the specified rule type. */
        bool isRuleType(const char8_t* ruleType) const { return mRuleDefinition.isType(ruleType); }

        /*! Returns true if thsi rule threshold currently requires an exact match. */
        bool isExactMatchRequired() const { return mMinFitThresholdList->isExactMatchRequired(); }

        //////////////////////////////////////////////////////////////////////////
        // Framework hooks for the MM Rules.
        //////////////////////////////////////////////////////////////////////////
        
        // Threshold Logic

        /*! ************************************************************************************************/
        /*! \brief update the current threshold based upon the current time (only updates if the next threshold is reached).
        ****************************************************************************************************/
        virtual UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate);

        /*! ************************************************************************************************/
        /*! \brief Vote on the values for this game.  Used in Create Game Mode only for a few rules.
        ****************************************************************************************************/
        virtual void voteOnGameValues(const MatchmakingSessionList& votingSessionList, GameManager::CreateGameRequest& createGameRequest) const {}

        /*! ************************************************************************************************/
        /*! \brief Returns the next time since the MM session start that the rule thresholds should be updated.
        ****************************************************************************************************/
        uint32_t getNextElapsedSecondsUpdate() const { return mNextUpdate; }

        // TODO: Refactor Rule Evaluation mode and remove (only port rules that don't use this).
        
        /*! ************************************************************************************************/
        /*! \brief Framework hack which allows rules to be evaluated for a fitscore without NO_MATCH to
                evaluate the final fit for players entering a game.
        ****************************************************************************************************/
        void setRuleEvaluationMode (RuleEvaluationMode ruleEvaluationMode) { mRuleEvaluationMode = ruleEvaluationMode; }

        /*! ************************************************************************************************/
        /*! \brief return the max possible fitscore ie the rule weight x 1.0.
        ****************************************************************************************************/
        FitScore getMaxFitScore() const override { return isDisabled() ? 0 : mRuleDefinition.getMaxFitScore(); }

        void setRuleIndex(uint32_t index) { mRuleIndex = index; }
        
        uint32_t getRuleIndex() const { return mRuleIndex; }

        // Diagnostics
        void tallyRuleDiagnosticsCreateGame(MatchmakingSubsessionDiagnostics& sessionDiagnostics, const MatchInfo *ruleEvalResult) const;
        void tallyRuleDiagnosticsFindGame(MatchmakingSubsessionDiagnostics& sessionDiagnostics, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const;

    protected:
        /*! Get the associated matchmaking session id for this rule. (Or -1 if no session exists) */
        MatchmakingSessionId getMMSessionId() const;

        /*! Get the associated matchmaking session member info list for this rule. This value will be nullptr when using the game browser or matchmaking slave. */
        const MatchmakingSession* getMatchmakingSession() const;

        // Unlike the MatchmakingSession, which may be nullptr, mCriteria will always be set after the constructor (before initialize).
        void setCriteria(const MatchmakingCriteria* criteria) { mCriteria = criteria; }
        const MatchmakingCriteria& getCriteria() const { return *mCriteria; }


        /*! ************************************************************************************************/
        /*! \brief Framework hook which allows this rule to update the MatchmakingAsyncStatus before it gets
            sent to the client.  Modifis this rules portion of mMatchmakingAsyncStatus.
        ****************************************************************************************************/
        virtual void updateAsyncStatus() = 0;

        /*! ************************************************************************************************/
        /*! \brief Fixup the match times for exact match and bidirectional cases. Keeps sessions from pulling
            in other sessions into their match unless both sessions are willing.  If either rule is
            at a threshold that requires an exact match, then they cannot be pulled in unless the match
            is an exact match or the threshold decays.  IE.  Your session could match mine now, but I might
            not match you until I am no longer exact match.  This function modifies your match time to
            match my minimum time to match.

            Also fixup the match times for bidirectional rules.  A bidirectional rule means that the rules
            preferences must be respected by both sessions.  This means that if my sessions preferences haven't
            decayed yet to match yours, your match time will be modified to match my minimum match time
            to avoid my session getting pulled into your game early.

            Any rule that passes in false for isBidirectionalRule means that preferences are not requirements
            and that the threshold does not need to be met in order for them to get pulled into another game.
            They still have to be met for them to create or find a game.
        ****************************************************************************************************/
        void fixupMatchTimesForRequirements(const SessionsEvalInfo &sessionsEvalInfo, const MinFitThresholdList *otherMinFitThresholdList,
            SessionsMatchInfo &sessionsMatchInfo, bool isBidirectionalRule) const;

        
        /*! ************************************************************************************************/
        /*! \brief Helper function to calculate the session match info determining if and when these sessions match.
        ****************************************************************************************************/
        virtual void calcSessionMatchInfo(const Rule& otherRule, const EvalInfo &evalInfo, MatchInfo &matchInfo) const;

        /*! ************************************************************************************************/
        /*! \brief returns the weighted fitScore for the rule (or NO_MATCH) using the rule's current minFitThreshold.

            \param[in] isExactMatch - signifies that the rule's value is an exact match to a game's value
            \param[in] fitPercent - the fitPercent between this rule's value and a game's value
            \param[in] weight - this rule's weight

            \return the weighted fit score of the match or if not a match at the current threshold then NO_MATCH.
        ***************************************************************************************************/
        virtual FitScore calcWeightedFitScore(bool isExactMatch, float fitPercent) const;

        /*! ************************************************************************************************/
        /*! \brief Initialize the threshold list this rule will use from the rule Definition based on
            the minFitListName passed in from the matchmaking criteria.  This function should be called
            by any deriving rule that uses min fit thresholds.

            \param[in] minFitListName The name of the min fit threshold list defined in the config to use.
            \param[in] ruleDef The Rule definition to get the min fit threshold list from.
            \param[out] err If this initialization fails the error message is populated.

            \return true if successful
        ***************************************************************************************************/
        bool initMinFitThreshold( const char8_t* minFitListName, const RuleDefinition& ruleDef,
             MatchmakingCriteriaError& err );

        /*! ************************************************************************************************/
        /*! \brief handles the disabled rule evaluation generically for all rules.  The below list
            sums up how the evaluation is handled for disabled rules of this MM Session and the other
            MM Session to evaluate.

            Both rules disabled - Both sessions match with a fitpercent of 0.0 at time 0.
            This rule disabled - this session match with a fitpercent of 0.0 at time 0, other session match with matchingWeightedFitScore at time 0.
            Other rule disabled - this session match with matchingWeightedFitScore at time 0, other session match with a fitpercent of 0.0 at time 0.
            no rules disabled - Returns false, session Match Info is not updated.

            MM_AUDIT: some rules need to still be evaluated to determine a fit if only 1 rule is disabled (ie. skill rule)
            
            \return true if we don't need to evaluate the other Rule because of disabled rules.
        ****************************************************************************************************/
        bool handleDisabledRuleEvaluation(const Rule &otherRule, SessionsMatchInfo &sessionsMatchInfo, FitScore matchingWeightedFitScore) const;

        /*! ************************************************************************************************/
        /*! \brief Helper to clamp the match age of both sessions to the minimum match time.
        ****************************************************************************************************/
        void clampMatchAgeToMinimumTime(int64_t matchTime, int64_t minMatchTime, uint32_t sessionAge, uint32_t &matchAge) const;

        // Diagnostics

        /*! ************************************************************************************************/
        /*! \brief Returns true if this rule type adds the MM rule-total diagnostic.
        ****************************************************************************************************/
        virtual bool isRuleTotalDiagnosticEnabled() const { return true; }

        /*! ************************************************************************************************/
        /*! \brief return MM session's count of visible games for the specified diagnostic
        ***************************************************************************************************/
        virtual uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const;

        /*! ************************************************************************************************/
        /*! \brief Get list of setup infos for per rule-value breakdown diagnostics (used for both create and
            find mode). Default returns empty, which disables per rule-value diagnostics for the rule.
        ****************************************************************************************************/
        virtual void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const {}

    private:

        /*! ************************************************************************************************/
        /*! \brief returns setup infos, names/tags, for the diagnostics to add for the rule.
            Used for both create and find mode
        ***************************************************************************************************/
        const RuleDiagnosticSetupInfoList& getDiagnosticsSetupInfos() const;

        // caches pointers to diagnostics
        DiagnosticsByRuleCategoryMapPtr getOrAddRuleDiagnostic(DiagnosticsByRuleMap& diagnosticsMap) const;
        MatchmakingRuleDiagnosticCountsPtr getOrAddRuleSubDiagnostic(DiagnosticsByRuleCategoryMap& ruleDiagnostic, const RuleDiagnosticSetupInfo& subDiagnosticInfo) const;
        
        /*! ************************************************************************************************/
        /*! \brief Fixup match times for the required exact match case.  See fixupMatchTimesForRequirements
            for more information.
        ****************************************************************************************************/
        void fixupMatchTimeForRequiredExactMatch(uint32_t mySessionAge, uint32_t otherSessionAge, uint32_t otherRequirementDecayAge, bool otherIsExactMatch,
            uint32_t &sessionMatchAge) const;

    protected:
        /*! The Matchmaking Criteria which owns this rule. */
        const MatchmakingCriteria* mCriteria;

        /*! The Associated Rule Definition for this rule. */
        const RuleDefinition& mRuleDefinition;

        /*! Matchmaking Async Status object to fill in with information on the current rule status. */
        MatchmakingAsyncStatus* mMatchmakingAsyncStatus;

        /*! MM Hack to calculate correct fit scores against games. */
        RuleEvaluationMode mRuleEvaluationMode;

        //////////////////////////////////////////////////////////////////////////
        // Threshold information
        //////////////////////////////////////////////////////////////////////////
        const MinFitThresholdList* mMinFitThresholdList;
        float mCurrentMinFitThreshold; // cached value of the current minFitThreshold
        uint32_t mNextUpdate;  //next elapsed update where this list should be updated.
        uint32_t mRuleIndex;


        //////////////////////////////////////////////////////////////////////////
        // Utilities for Custom Rules
        //////////////////////////////////////////////////////////////////////////
        template<class T> T* getCustomRulePrefFromCriteria(const MatchmakingCriteriaData& criteriaData, const char *ruleName) 
        {
            MatchmakingCriteriaData::VariableCustomRulePrefsMap::const_iterator ruleIt = criteriaData.getVariableCustomRulePrefs().find(RuleName(ruleName));
            if (ruleIt == criteriaData.getVariableCustomRulePrefs().end() || ruleIt->second->get()->getTdfId() != T::TDF_ID)
            {
                return nullptr;
            }
            return (T *)ruleIt->second->get();       
        }
        template<class T> void setCustomRuleAsyncStatus(const char *ruleName, T& status)
        {
            EA::TDF::VariableTdfBase *varTdf = mMatchmakingAsyncStatus->getVariableCustomAsyncStatus().allocate_element();
            varTdf->set(status);
            mMatchmakingAsyncStatus->getVariableCustomAsyncStatus()[ruleName] = varTdf;
        }

    private:
        // load testing shows caching these pointers to the session's diagnostics entries, can improve performance significantly
        mutable DiagnosticsByRuleCategoryMapPtr mCachedRuleDiagnosticPtr;
        mutable RuleDiagnosticSetupInfoList mCachedDiagnosticSetupInfos;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_RULE
