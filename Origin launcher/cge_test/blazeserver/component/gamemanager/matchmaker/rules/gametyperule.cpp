/*! ************************************************************************************************/
/*!
\file gametyperule.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/gametyperule.h"
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
    GameTypeRule::GameTypeRule(const GameTypeRuleDefinition& definition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(definition, matchmakingAsyncStatus),
        mAcceptsGameGroup(false),
        mAcceptsGameSession(false)
    {
    }

    GameTypeRule::GameTypeRule(const GameTypeRule& other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(other, matchmakingAsyncStatus),
        mAcceptsGameGroup(other.mAcceptsGameGroup),
        mAcceptsGameSession(other.mAcceptsGameSession)
    {
        other.mGameTypeList.copyInto(mGameTypeList);
    }


    bool GameTypeRule::addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const
    {
        Rete::ConditionBlock& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        Rete::OrConditionList& baseOrConditions = baseConditions.push_back();

        GameTypeList::const_iterator itr = mGameTypeList.begin();
        GameTypeList::const_iterator end = mGameTypeList.end();
        for (; itr != end; ++itr)
        {
            baseOrConditions.push_back(Rete::ConditionInfo(Rete::Condition(getDefinition().getGameReportNameStr(), getDefinition().getGameTypeValue(*itr), mRuleDefinition), 0, this));
        }

        if (baseOrConditions.empty())
        {
            baseOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(getDefinition().getGameReportNameStr(), 0, mRuleDefinition), 
                0, this));
        }

        return true;
    }

    bool GameTypeRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err)
    {
        if (!matchmakingSupplementalData.mGameTypeList.empty()) 
        {
            // a game type list was provided, copy it over
            matchmakingSupplementalData.mGameTypeList.copyInto(mGameTypeList);

            // iterate over the game type list
            for (auto& gameTypeIter : mGameTypeList)
            {
                if (gameTypeIter == GAME_TYPE_GAMESESSION)
                {
                    mAcceptsGameSession = true;
                }
                else if (gameTypeIter == GAME_TYPE_GROUP)
                {
                    mAcceptsGameGroup = true;
                }
            }
        }
        else
        {
            // the game type list was empty & only game sessions will be found by default
            mGameTypeList.push_back(GAME_TYPE_GAMESESSION);
            mAcceptsGameSession = true;
        }
        return true;
    }

    Rule* GameTypeRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW GameTypeRule(*this, matchmakingAsyncStatus);
    }

    void GameTypeRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const GameTypeRule& otherGameTypeRule = static_cast<const GameTypeRule &>(otherRule);

        if (this->mAcceptsGameSession != otherGameTypeRule.mAcceptsGameSession)
        {
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;

            debugRuleConditions.writeRuleCondition("Other GameTypeRule %s GAME_TYPE_GAMESESSION, this session %s GAME_TYPE_GAMESESSION", (otherGameTypeRule.mAcceptsGameSession ? "accepts" : "doesn't accept"), (this->mAcceptsGameSession ? "accepts" : "doesn't accept"));

            return;
        }

        if (this->mAcceptsGameGroup != otherGameTypeRule.mAcceptsGameGroup)
        {
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;

            debugRuleConditions.writeRuleCondition("Other GameTypeRule %s GAME_TYPE_GROUP, this session %s GAME_TYPE_GROUP", (otherGameTypeRule.mAcceptsGameGroup ? "accepts" : "doesn't accept"), (this->mAcceptsGameGroup ? "accepts" : "doesn't accept"));

            return;
        }

        fitPercent = 0;
        isExactMatch = true;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
