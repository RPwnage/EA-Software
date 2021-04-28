/*! ************************************************************************************************/
/*!
    \file   reputationrule.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/reputationrule.h"
#include "gamemanager/matchmaker/rules/reputationruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    ReputationRule::ReputationRule(const ReputationRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(ruleDefinition, matchmakingAsyncStatus),
        mDesiredValue(REJECT_POOR_REPUTATION),
        mIsDisabled(false)
    {}


    ReputationRule::ReputationRule(const ReputationRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(other, matchmakingAsyncStatus), 
        mDesiredValue(other.getDesiredValue()),
        mIsDisabled(other.isDisabled())
    {}


    /*! ************************************************************************************************/
    /*!
        \brief initialize a reputationRule instance for use in a matchmaking session. Return true on success.
    
        \param[in]  criteriaData - Criteria data used to initialize the rule.
        \param[in]  matchmakingSupplementalData - used to lookup the rule definition
        \param[out] err - errMessage is filled out if initialization fails.
        \return true on success, false on error (see the errMessage field of err)
    ***************************************************************************************************/
    bool ReputationRule::initialize(const MatchmakingCriteriaData &criteriaData,
        const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        mIsDisabled = gUserSessionManager->isReputationDisabled();

        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule is disabled when searching for resettable servers.
            mIsDisabled = true;
            return true;
        }

        const ReputationRulePrefs& reputationRulePrefs = criteriaData.getReputationRulePrefs();

        // init the desired value
        mDesiredValue = reputationRulePrefs.getReputationRequirement();

        return true;
    }


    bool ReputationRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        Rete::OrConditionList& orConditions = conditions.push_back();

        if ((mDesiredValue == REJECT_POOR_REPUTATION) || (mDesiredValue == ACCEPT_ANY_REPUTATION))
        {
            orConditions.push_back(Rete::ConditionInfo(Rete::Condition(
                ReputationRuleDefinition::REPUTATIONRULE_DEFINITION_ATTRIBUTE_KEY, 
                mRuleDefinition.getWMEAttributeValue(ReputationRuleDefinition::REPUTATIONRULE_REQUIRE_GOOD_REPUTATION_RETE_KEY), 
                mRuleDefinition), getMaxFitScore(), this));
        }

        if ((mDesiredValue == ACCEPT_ANY_REPUTATION) || (mDesiredValue == MUST_ALLOW_ANY_REPUTATION))
        {
            orConditions.push_back(Rete::ConditionInfo(Rete::Condition(
                ReputationRuleDefinition::REPUTATIONRULE_DEFINITION_ATTRIBUTE_KEY, 
                mRuleDefinition.getWMEAttributeValue(ReputationRuleDefinition::REPUTATIONRULE_ALLOW_ANY_REPUTATION_RETE_KEY), 
                mRuleDefinition), getMaxFitScore(), this));
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief from SimpleRule.
        \param[in] otherRule The other rule to post evaluate.
        \param[in/out] evalInfo The EvalInfo to update after evaluations are finished.
        \param[in/out] matchInfo The MatchInfo to update after evaluations are finished.
    ***************************************************************************************************/
    void ReputationRule::postEval(const Rule& otherRule, EvalInfo &evalInfo, MatchInfo &matchInfo) const
    {
        const ReputationRule& otherReputationRule = static_cast<const ReputationRule&>(otherRule);
        ReputationRule::enforceRestrictiveOneWayMatchRequirements(otherReputationRule, matchInfo);
    }

    void ReputationRule::enforceRestrictiveOneWayMatchRequirements(const ReputationRule &otherRule, MatchInfo &myMatchInfo) const
    {
        // Don't allow ACCEPT_ANY_REPUTATION to pull in REJECT_POOR_REPUTATION, or MUST_ALLOW_ANY_REPUTATION to pull in ACCEPT_ANY_REPUTATION
        if (((mDesiredValue == ACCEPT_ANY_REPUTATION) && (otherRule.mDesiredValue == REJECT_POOR_REPUTATION)) 
            || ((mDesiredValue == MUST_ALLOW_ANY_REPUTATION) && (otherRule.mDesiredValue == ACCEPT_ANY_REPUTATION)))
        {
            myMatchInfo.setNoMatch();
        }
    }

    // Local helper
    void ReputationRule::calcFitPercentInternal(ReputationRequirement otherValue, float &fitPercent, bool &isExactMatch) const
    {
        if ((getDesiredValue() == otherValue) || (getDesiredValue() == ACCEPT_ANY_REPUTATION) || (otherValue == ACCEPT_ANY_REPUTATION))
        {
            isExactMatch = true;
            fitPercent = 1.0;
        }
        else
        {
            isExactMatch = false;
            fitPercent = FIT_PERCENT_NO_MATCH;
        }
    }



    void ReputationRule::calcFitPercent(const Search::GameSessionSearchSlave &gameSession, float &fitPercent, bool &isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        ReputationRequirement otherValue = gameSession.getGameSettings().getAllowAnyReputation()?
            MUST_ALLOW_ANY_REPUTATION : REJECT_POOR_REPUTATION;

        calcFitPercentInternal(otherValue, fitPercent, isExactMatch);

        
        debugRuleConditions.writeRuleCondition("Game is %s %s %s", ReputationRequirementToString(otherValue), isExactMatch?"==":"!=", ReputationRequirementToString(getDesiredValue()));
        
    }


    void ReputationRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const ReputationRule &otherReputationRule = static_cast<const ReputationRule&>(otherRule);

        calcFitPercentInternal(otherReputationRule.getDesiredValue(), fitPercent, isExactMatch);

        
        debugRuleConditions.writeRuleCondition("Other is %s %s %s", ReputationRequirementToString(otherReputationRule.getDesiredValue()), isExactMatch?"==":"!=", ReputationRequirementToString(getDesiredValue()));
        
    }


    Rule* ReputationRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW ReputationRule(*this, matchmakingAsyncStatus);
    }

    // this rule is always owner wins with respect to allowAnyReputation.
    void ReputationRule::voteOnGameValues(const MatchmakingSessionList& votingSessionList, GameManager::CreateGameRequest& createGameRequest) const
    {
        // check if using dynamic reputation requirements
        bool hasBadReputation = false;
        for (MatchmakingSessionList::const_iterator itr = votingSessionList.begin(); itr != votingSessionList.end(); ++itr)
        {
            // if titles mixed dynamic vs non dynamic check everyone's setting. set non dynamic if anyone is for non.
            if (!(*itr)->getDynamicReputationRequirement())
            {
                createGameRequest.getGameCreationData().getGameSettings().clearDynamicReputationRequirement();
                break;
            }

            for (UserSessionInfoList::const_iterator uItr = (*itr)->getMembersUserSessionInfo().begin();
                uItr != (*itr)->getMembersUserSessionInfo().end(); ++uItr)
            {
                if ((*uItr)->getHasBadReputation())
                    hasBadReputation = true;
            }
        }

        // if dynamic reputation, set allowAnyReputation based on actual member reps
        if (createGameRequest.getGameCreationData().getGameSettings().getDynamicReputationRequirement())
        {
            if (hasBadReputation)
                createGameRequest.getGameCreationData().getGameSettings().setAllowAnyReputation();
            else
                createGameRequest.getGameCreationData().getGameSettings().clearAllowAnyReputation();
            return;
        }
        // non dynamic reputation. set allowAnyReputation based on desired value
        switch (mDesiredValue)
        {
        case REJECT_POOR_REPUTATION:
            createGameRequest.getGameCreationData().getGameSettings().clearAllowAnyReputation();
            break;
        case ACCEPT_ANY_REPUTATION:
        case MUST_ALLOW_ANY_REPUTATION:
        default:
            createGameRequest.getGameCreationData().getGameSettings().setAllowAnyReputation();
            break;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Implemented to enable per rule criteria value break down diagnostics
    ****************************************************************************************************/
    void ReputationRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
    {
        setupInfos.push_back(RuleDiagnosticSetupInfo("reputationRequirement", ReputationRequirementToString(mDesiredValue)));
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

