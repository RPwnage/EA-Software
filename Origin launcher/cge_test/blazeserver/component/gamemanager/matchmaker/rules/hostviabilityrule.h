/*! ************************************************************************************************/
/*!
    \file   hostviabilityrule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_HOST_VIABILITY_RULE
#define BLAZE_MATCHMAKING_HOST_VIABILITY_RULE

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "util/tdf/utiltypes.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class Matchmaker;
    class MatchmakingSession;
    struct MatchmakingSupplementalData;
    class HostViabilityRuleDefinition;

    class HostViabilityRule : public SimpleRule
    {
        NON_COPYABLE(HostViabilityRule)

    public:
        HostViabilityRule(const HostViabilityRuleDefinition &def, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        HostViabilityRule(const HostViabilityRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~HostViabilityRule() override {}

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW HostViabilityRule(*this, matchmakingAsyncStatus); }

        /*! ************************************************************************************************/
        /*!
            \brief Initialize the rule from the MatchmakingCriteria's HostViabilityRulePrefs
                (defined in the startMatchmakingRequest). Returns true on success, false otherwise.

            \param[in]  criteriaData - data required to initialize the matchmaking rule.
            \param[in]  matchmakingSupplementalData - used to lookup the rule definition
            \param[out] err - errMessage is filled out if initialization fails.
            \return true on success, false on error (see the errMessage field of err)
        *************************************************************************************************/
        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

    
        /*! ************************************************************************************************/
        /*! \brief update matchmaking status for the rule to current status
        ****************************************************************************************************/
        void updateAsyncStatus() override;

        // RETE RULE: TODO: fill these out.
        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override { return true; }

        bool isDedicatedServerEnabled() const override { return false; }

        /*! ************************************************************************************************/
        /*! \brief return true if the supplied matchmaking session's NAT type matches this rule's current
                minFitThreshold.
        
            \param[in] matchmakingSession the session to test for host viability
            \return true if the session could act as host, false otherwise.
        ***************************************************************************************************/
        bool isViableGameHost(const MatchmakingSession &matchmakingSession) const;

        // we have to override the isDisabled() so it always returns true in CG mode, even if a fit threshold was specified
        // to ensure that we can update async status in the MM slave
        /*! returns true if this rule is disabled.  By default a rule is disabled if it has no MinFitThreshold List defined. */
        bool isDisabled() const override { return (mMinFitThresholdList == nullptr) || mIsCreateGameContext; }

         /*! ************************************************************************************************/
        /*! \brief update the current threshold based upon the current time (only updates if the next threshold is reached).
                    This is overriden to use an alternate implementation if we're in a create game context.
        ****************************************************************************************************/
        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override;

         // host viability rule isn't evaluated for create game matchmaking; this override ensures all sessions are a match for this rule
         // during finalization, we ensure that there's at least one viable host before creating a game.
         void evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const override;

    protected:
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
       

    private:

        /*! ************************************************************************************************/
        /*! \brief calculates fit percentage based on current cached NatType versus targeted NatType
        
            \param[in]  otherValue The NatType being compared against.
            \return the UN-weighted fitScore for this rule. Will not return a negative fit score
        *************************************************************************************************/
        float calcFitPercentInternal( Util::NatType otherNatType, GameNetworkTopology gamesTopology ) const;

        //this finalization and post-RETE only rule disabled for diagnostics currently
        bool isRuleTotalDiagnosticEnabled() const override { return false; }

    private:
        Util::NatType mNatType;
        const HostViabilityRuleDefinition& mDefinition;
        bool mIsCreateGameContext;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_HOST_VIABILITY_RULE
