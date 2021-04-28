/*! ************************************************************************************************/
/*!
\file   exampleupstreambpsrule.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_EXAMPLE_UPSTREAM_BPS_RULE_H
#define BLAZE_MATCHMAKING_EXAMPLE_UPSTREAM_BPS_RULE_H

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "exampleupstreambpsruledefinition.h"


namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief An example custom matchmaking rule.  This rule calculates the fit score between a game
            and a session or two sessions based on the clients Upstream bits per second.  The desired
            Upstream BPS is compared against the actual Upstream BPS of that game/session.

        Fit score is calculated using the difference in Upstream BPS plotted on a Gaussian Function.
            This Gaussian is stored and configured on the Rule Definition.  A Gaussian Function 
            is one way to determine the fit difference between two Upstream BPS values.  A linear
            function would also work, but the Gaussian was chosen to illustrate rule configuration.

        Example Rule derives from Simple Rule which handles most of the boiler plate logic for hooking
            into the matchmaking framework.  When deriving from simple rule all that is required
            is to define how a fit score is determined given two values and if they are an exact match.
    *************************************************************************************************/
    class ExampleUpstreamBPSRule : public SimpleRule
    {
        NON_COPYABLE(ExampleUpstreamBPSRule);
    public:

        /*! ************************************************************************************************/
        /*! \brief Rule constructor.

            \param[in] ruleDefinition - the definition object associated with this particular rule
            \param[in] matchmakingAsyncStatus - The async status object to be able to send notifications of evaluations status
        *************************************************************************************************/
        ExampleUpstreamBPSRule(const ExampleUpstreamBPSRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus);

        /*! ************************************************************************************************/
        /*! \brief Rule constructor.

            \param[in] ruleDefinition - the definition object associated with this particular rule
            \param[in] matchmakingAsyncStatus - The async status object to be able to send notifications of evaluations status
        *************************************************************************************************/
        ExampleUpstreamBPSRule(const ExampleUpstreamBPSRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        
        /*! ************************************************************************************************/
        /*! \brief Rule destructor.  Cleanup any allocated memory
        *************************************************************************************************/
        ~ExampleUpstreamBPSRule() override;

        //////////////////////////////////
        // Rule specific functions
        //////////////////////////////////

        /*! ************************************************************************************************/
        /*! \brief Initialization of the rule based on the desired options of the session.

            \param[in] criteriaData - the options set in the session for the rules configuration.
            \param[in/out] err - Error object to report any problems initializing the rule
        *************************************************************************************************/
        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;

        /*! ************************************************************************************************/
        /*! \brief Calculate the fit percent match of this rule against a game session. Used for calculating
            a match during a matchmaking find game, or a gamebrowser list search.

            \param[in] gameSession - the game you are evaluating against
            \param[in/out] fitPercent - The fit percent match your calculation yields.
            \param[in/out] isExactMatch - if this is an exact match, the evaluation is complete.
        *************************************************************************************************/
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch) const override;

        /*! ************************************************************************************************/
        /*! \brief Calculate the fit percent match of this rule against another rule.  Used for calculating
            a match during a matchmaking create game.

            \param[in] otherRule - The other sessions rule to calculate against
            \param[in/out] fitPercent - The fit percent match your calculation yields.
            \param[in/out] isExactMatch - if this is an exact match, the evaluation is complete.
        *************************************************************************************************/
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch) const override;

        /*! ************************************************************************************************/
        /*! \brief Clones the rule.  This is used to create the rule to match against another rule.

            \param[in] matchmakingAsyncStatus - the async status object to send notifications
        *************************************************************************************************/
        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        /////////////////////////////////////////////////////////////////////////////////////////
        // RETE RULE METHODS
        // RETE functionality not implemented for this rule.
        /////////////////////////////////////////////////////////////////////////////////////////

        /*! ************************************************************************************************/
        /*! \brief adds Rete conditions to the condition block to be evaluated by Rete.
            Note that the ExampleUpstreamBpsRule is not rete capable, this is a no-op.
        
            \param[in/out] conditionBlockList - the condition block where we add our conditions.
            \return true, even though no conditions are added for this rule.  Normaly we would
            return true to evaluate the rule normally, and false to match any.
        ***************************************************************************************************/
        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override { return true; }

        /*! ************************************************************************************************/
        /*! \brief Evaluates the provided game session as an "any" case inside rete. Providing the fit score.
            Note that the ExampleUpstreamBpsRule is not Rete capable, this is a no-op.
        
            \param[in] gameSession - the game session to evaluate a match against.
            \param[in] blockId - the condition block id our match any was under.
            \return the fitscore for a match any for this rule.
        ***************************************************************************************************/
        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, Rete::ConditionBlockId blockId, ReadableRuleConditions& debugRuleConditions) const override { return 0; }

        bool isMatchAny() const override { return true; }

        /*! ************************************************************************************************/
        /*! \brief Checks if dedicated servers are evaluated by this rule.
        
            \return false we do not evaluate against dedicated servers.
        ***************************************************************************************************/
        bool isDedicatedServerEnabled() const override { return false; }

    protected:
        /*! ************************************************************************************************/
        /*! \brief Asynchronous status update of the match for the rule.
        *************************************************************************************************/
        void updateAsyncStatus() override;

    private:

        /*! Desired upstream bits per second as specified in the matchmaking criteria of the rule. */
        uint32_t mDesiredUpstreamBps;

        /*! Actual upstream bits per second determined by the QOS data on the server. */
        uint32_t mMyUpstreamBps;

        /*! ************************************************************************************************/
        /*! \brief Helper function to determine the upstream bps for this session.  If this session
            is a group then mMyUpstreamBps will be the upstream bps value of the user with the highest
            upstream bps.

            \return the determined upstream bps for the session or group
        ****************************************************************************************************/
        uint32_t determineUpstreamBps() const;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_EXAMPLE_UPSTREAM_BPS_RULE_H
