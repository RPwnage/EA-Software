/*! ************************************************************************************************/
/*!
\file gamesettingsrule.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/gamesettingsrule.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    GameSettingsRule::GameSettingsRule(const GameSettingsRuleDefinition& definition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(definition, matchmakingAsyncStatus),
        mGameSetting(GameSettingsRuleDefinition::GAME_SETTING_SIZE)
    {

    }

    GameSettingsRule::GameSettingsRule(const GameSettingsRule& other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(other, matchmakingAsyncStatus),
        mGameSetting(GameSettingsRuleDefinition::GAME_SETTING_SIZE)
    {

    }


    bool GameSettingsRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        if (!isDisabled())
        {
            Rete::ConditionBlock& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
            Rete::OrConditionList& baseOrConditions = baseConditions.push_back();
            baseOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(getDefinition().GameSettingNameToString(getGameSetting()), getDefinition().getWMEBooleanAttributeValue(true), mRuleDefinition), 
                0, this));

            return true;
        }

        return false;
    }

    bool GameSettingsRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule not needed when doing resettable games only
            return true;
        }

        if (matchmakingSupplementalData.mMatchmakingContext.isSearchingByMatchmaking())
        {
            mGameSetting = GameSettingsRuleDefinition::GAME_SETTING_OPEN_TO_MATCHMAKING;
        }
        else if (matchmakingSupplementalData.mMatchmakingContext.isSearchingByGameBrowser() && !matchmakingSupplementalData.mIgnoreGameSettingsRule)
        {
            mGameSetting = GameSettingsRuleDefinition::GAME_SETTING_OPEN_TO_BROWSING;
        }
        // else we stay as the undefined SIZE value, which disables the rule.

        return true;
    }


    Rule* GameSettingsRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW GameSettingsRule(*this, matchmakingAsyncStatus);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
