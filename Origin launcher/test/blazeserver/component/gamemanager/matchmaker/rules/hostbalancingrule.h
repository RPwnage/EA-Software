/*! ************************************************************************************************/
/*!
    \file   hostbalancingrule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_HOST_BALANCING_RULE
#define BLAZE_MATCHMAKING_HOST_BALANCING_RULE

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
    class HostBalancingRuleDefinition;

    //TODO !!!!! NEED TO FIGURE OUT WHAT THE HOST BALANCING MAX FIT SCORE IS!!!

    /*! ************************************************************************************************/
    /*!
        \brief This rule groups good game hosts with bad game hosts (as determined by Network QOS info).
                The goal is to ration the 'good' hosts and distribute them amongst the bad hosts so that
                all game members can connect to each other (according to the game's network topology).

        This is an attempt to keep bad hosts (NAT_TYPE_STRICT, STRICT_SEQUENTIAL, UNKNOWN) 
        from causing or having a bad game experience (matching to games they won't connect to)
        by rationing "good" hosts (NAT_TYPE_OPEN, MODERATE) and preventing similar hosts from 
        grouping together when there are other players available

        HostBalancingRules must be initialized before use(looking up their definition & minFitThreshold from the matchmaker).
        After initialization, update a rule's cachedInfo, then evaluate the rule against a game of matchmaking session.

    *************************************************************************************************/
    class HostBalancingRule : public SimpleRule
    {
        NON_COPYABLE(HostBalancingRule)
    public:

        HostBalancingRule(const HostBalancingRuleDefinition &def, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        HostBalancingRule(const HostBalancingRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~HostBalancingRule() override {}

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override { return BLAZE_NEW HostBalancingRule(*this, matchmakingAsyncStatus); }

        /*! ************************************************************************************************/
        /*!
            \brief Initialize the rule from the MatchmakingCriteria's HostBalancingRulePrefs
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

    protected:
        void calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;

    private:

        Util::NatType mNatType;
        GameNetworkTopology mNetworkTopology;
        const HostBalancingRuleDefinition& mDefinition;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_HOST_BALANCING_RULE
