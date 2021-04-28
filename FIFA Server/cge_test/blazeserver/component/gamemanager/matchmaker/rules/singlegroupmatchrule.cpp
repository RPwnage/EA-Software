/*! ************************************************************************************************/
/*!
\file singlegroupmatchrule.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/tdf/gamemanager.h" 
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/matchmaker/rules/singlegroupmatchrule.h"
#include "gamemanager/matchmaker/rules/singlegroupmatchruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    SingleGroupMatchRule::SingleGroupMatchRule(const SingleGroupMatchRuleDefinition& definition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(definition, matchmakingAsyncStatus),
        mIsSingleGroupMatch(false),
        mIsDisabled(true)
    {
    }

    SingleGroupMatchRule::SingleGroupMatchRule(const SingleGroupMatchRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(otherRule, matchmakingAsyncStatus),
        mIsSingleGroupMatch(otherRule.mIsSingleGroupMatch),
        mIsDisabled(otherRule.mIsDisabled)
    {
    }

    bool SingleGroupMatchRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
    {
        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule is disabled when searching for servers.
            return true;
        }

        mUserGroupId = matchmakingSupplementalData.mPlayerJoinData.getGroupId();
        // We check against game size rule to see if we are enabled/disabled due to this check previously
        // being integrated with the game size rule.
        mIsDisabled = (criteriaData.getPlayerCountRuleCriteria().getRangeOffsetListName()[0] == '\0');
        mIsSingleGroupMatch = criteriaData.getPlayerCountRuleCriteria().getIsSingleGroupMatch() > 0 ? true : false;

        TRACE_LOG("[SingleGroupMatchRule] IsSingleGroupMatch(" << (mIsSingleGroupMatch?"true":"false") << ") UserGroupId(" 
                  << mUserGroupId.toString().c_str() << ")");
        return true;
    }

    bool SingleGroupMatchRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        if (mIsDisabled)
        {
            return false;
        }
        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);

        Rete::OrConditionList& settingOrConditions = conditions.push_back();

        const SingleGroupMatchRuleDefinition& ruleDef = static_cast<const SingleGroupMatchRuleDefinition&>(mRuleDefinition);

        settingOrConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(SingleGroupMatchRuleDefinition::SGMRULE_SETTING_KEY,
            ruleDef.getWMEBooleanAttributeValue(mIsSingleGroupMatch), mRuleDefinition),
            0, this));

        Rete::OrConditionList& groupOrConditions = conditions.push_back();

        eastl::string buf;
        buf.append_sprintf("%s_%" PRId64, SingleGroupMatchRuleDefinition::SGMRULE_GROUP_ID_KEY,  mUserGroupId.id);
        groupOrConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(buf.c_str(), 
            ruleDef.getWMEBooleanAttributeValue(true), mRuleDefinition), 
            0, this));

        // The number of groups currently in the game, always 0 or 1
        groupOrConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(SingleGroupMatchRuleDefinition::SGMRULE_NUM_GROUPS_KEY, 
            ruleDef.getWMEInt64AttributeValue((int64_t)0), mRuleDefinition), 
            0, this));
        groupOrConditions.push_back(Rete::ConditionInfo(
            Rete::Condition(SingleGroupMatchRuleDefinition::SGMRULE_NUM_GROUPS_KEY, 
            ruleDef.getWMEInt64AttributeValue((int64_t)1), mRuleDefinition), 
            0, this));

        return true;
    }

    void SingleGroupMatchRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const 
    {
        // DEPRICATED: Old find game functionality
        fitPercent = ZERO_FIT;
        isExactMatch = false;
        return;
    }


    void SingleGroupMatchRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const 
    {
        // Not yet implemented, need to move completely out of game size for create game
        fitPercent = ZERO_FIT;
        isExactMatch = false;
        return;
    }


}
}
}
