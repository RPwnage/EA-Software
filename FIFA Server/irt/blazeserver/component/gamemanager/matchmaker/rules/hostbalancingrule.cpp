/*! ************************************************************************************************/
/*!
    \file   hostbalancingrule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/hostbalancingrule.h"
#include "gamemanager/matchmaker/rules/hostbalancingruledefinition.h"
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    //! Note: you must initialize each HostBalancingRule before using it.
    HostBalancingRule::HostBalancingRule(const HostBalancingRuleDefinition &def, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(def, matchmakingAsyncStatus),
          mNatType(Util::NAT_TYPE_UNKNOWN),
          mNetworkTopology(CLIENT_SERVER_PEER_HOSTED),
          mDefinition(def)
    {}

    HostBalancingRule::HostBalancingRule(const HostBalancingRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(other, matchmakingAsyncStatus),
          mNatType(other.mNatType),
          mNetworkTopology(other.mNetworkTopology),
          mDefinition(other.mDefinition)
    {}


    /*! ************************************************************************************************/
    /*!
        \brief Initialize the rule from the MatchmakingCriteria's HostBalancingRulePrefs
            (defined in the startMatchmakingRequest). Returns true on success, false otherwise.

        \param[in]  criteriaData - data required to initialize the matchmaking rule.
        \param[in]  matchmakingSupplementalData - used to lookup the rule definition
        \param[out] err - errMessage is filled out if initialization fails.
        \return true on success, false on error (see the errMessage field of err)
    *************************************************************************************************/

    bool HostBalancingRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
    {
        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule not needed when doing resettable games only
            return true;
        }

        const HostBalancingRulePrefs& rulePrefs = criteriaData.getHostBalancingRulePrefs();

        // This rule only applies to Create Game searches
        if (matchmakingSupplementalData.mMatchmakingContext.isSearchingPlayers())
        {
            mNatType = matchmakingSupplementalData.mNetworkQosData.getNatType();
            mNetworkTopology = matchmakingSupplementalData.mNetworkTopology;

            // lookup the minFitThreshold list we want to use
            const char8_t* listName = rulePrefs.getMinFitThresholdName();

            bool minFitIni = initMinFitThreshold( listName, mDefinition, err );
            if (minFitIni)
            {
                updateAsyncStatus();
            }

            return minFitIni;
        }
        return true;
    }

    // from SimpleRule
    void HostBalancingRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        // TODO: This call is used when evaluating a newly created game, need to implement it for that case.
        fitPercent = 0.0;
        isExactMatch = false;
    }
    // from SimpleRule
    void HostBalancingRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        fitPercent = mDefinition.getFitPercent(mNetworkTopology, mNatType, static_cast<const HostBalancingRule&>(otherRule).mNatType);
        isExactMatch = false;  // This rule doesn't do exact match.

        
        debugRuleConditions.writeRuleCondition("My NAT %s versus Other's NAT %s and mesh %s", NatTypeToString(mNatType), NatTypeToString(static_cast<const HostBalancingRule&>(otherRule).mNatType), GameNetworkTopologyToString(mNetworkTopology));
        
    }


    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    *************************************************************************************************/
    void  HostBalancingRule::updateAsyncStatus()
    {
        // MM_AUDIT: Why aren't these just defined in the TDF rather than in the definition?
        // This could be collapsed to a single assignment at that point.

        // Threshold value expecting now
        HostBalanceRuleStatus::HostBalanceValues hostBalanceValues;
        if (mCurrentMinFitThreshold == HostBalancingRuleDefinition::HOSTS_STRICTLY_BALANCED)
        {
            hostBalanceValues = HostBalanceRuleStatus::HOSTS_STRICTLY_BALANCED;
        }
        else if (mCurrentMinFitThreshold == HostBalancingRuleDefinition::HOSTS_BALANCED)
        {
            hostBalanceValues = HostBalanceRuleStatus::HOSTS_BALANCED;
        }
        else
            hostBalanceValues = HostBalanceRuleStatus::HOSTS_UNBALANCED;

        mMatchmakingAsyncStatus->getHostBalanceRuleStatus().setMatchedHostBalanceValue(hostBalanceValues);
    }

    /*! ************************************************************************************************/
    /*! \brief Implemented to enable per rule criteria value break down diagnostics
    ****************************************************************************************************/
    void HostBalancingRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
    {
        setupInfos.push_back(RuleDiagnosticSetupInfo("natType", NatTypeToString(mNatType)));
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
