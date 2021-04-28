/*! ************************************************************************************************/
/*!
    \file teamcountrule.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/teamcountrule.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerroster.h"
#include "EASTL/sort.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    TeamCountRule::TeamCountRule(const TeamCountRuleDefinition &definition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(definition, matchmakingAsyncStatus),
          mTeamCount(0)
    {
    }

    TeamCountRule::TeamCountRule(const TeamCountRule &other, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(other, matchmakingAsyncStatus),
          mTeamCount(other.mTeamCount)
    {
    }


    TeamCountRule::~TeamCountRule()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief Initialize the rule from the MatchmakingCriteria's TeamCountRulePrefs
            (defined in the startMatchmakingRequest). Returns true on success, false otherwise.

        \param[in]  criteriaData - data required to initialize the matchmaking rule.
        \param[in]  matchmakingSupplementalData - used to lookup the rule definition
        \param[out] err - errMessage is filled out if initialization fails.
        \return true on success, false on error (see the errMessage field of err)
    *************************************************************************************************/
    bool TeamCountRule::initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, 
            MatchmakingCriteriaError &err)
    {
        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule is disabled when searching for servers.
            return true;
        }
        const TeamCountRulePrefs& rulePrefs = criteriaData.getTeamCountRulePrefs();
        mTeamCount = rulePrefs.getTeamCount();

        // Attempt to get the size from the team size or composition rule prefs (if those rules aren't disabled):
        if (isDisabled())
        {
            bool teamCountIsDefault;
            uint16_t teamCountFromCriteria = getCriteria().calcTeamCountFromRules(criteriaData, matchmakingSupplementalData, &teamCountIsDefault);
            mTeamCount = (teamCountIsDefault? 0 : teamCountFromCriteria);
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief override from MMRule; we cache off the current range, not a minFitThreshold value
    ***************************************************************************************************/
    UpdateThresholdResult TeamCountRule::updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate)
    {
        // The rule never changes so we don't need to decay.
        mNextUpdate = UINT32_MAX;
        return NO_RULE_CHANGE;
    }


    /*! ************************************************************************************************/
    /*! \brief evaluate a matchmaking session's team count against your value.  

        \param[in]  session The other matchmaking session to evaluate this rule against.
    *************************************************************************************************/
    void TeamCountRule::evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const
    {
        const TeamCountRule &otherRule = *(static_cast<const TeamCountRule*>(otherSession.getCriteria().getRuleByIndex(getRuleIndex())));

        // if both sessions have disabled teams, they match
        if (isDisabled() && otherRule.isDisabled())
        {
            // rule is disabled, let 'em match
            sessionsMatchInfo.sMyMatchInfo.setMatch(0,0);
            sessionsMatchInfo.sOtherMatchInfo.setMatch(0,0);
            return;
        }

        uint16_t otherTeamCount = otherRule.getTeamCount();
        uint16_t myTeamCount = getTeamCount();

        // we only check for maching team counts if we're both using the rule. (One player might not care how many teams there are)
        if (!isDisabled() && !otherRule.isDisabled())
        {
            // no match if the players have a different team count.
            if (otherTeamCount != myTeamCount)
            {
                sessionsMatchInfo.sMyMatchInfo.setNoMatch();
                sessionsMatchInfo.sOtherMatchInfo.setNoMatch();

                sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Other team count %hu != %hu.", otherTeamCount, myTeamCount);
                
                return; // no match
            }
        }

        // 0,0 means matches at 0 fitscore, with 0 time offset
        sessionsMatchInfo.sMyMatchInfo.setMatch(0,0);
        sessionsMatchInfo.sOtherMatchInfo.setMatch(0,0);

        sessionsMatchInfo.sMyMatchInfo.sDebugRuleConditions.writeRuleCondition("Other team count %hu == %hu.", otherTeamCount, myTeamCount);
        
    }


    /*! ************************************************************************************************/
    /*! \brief update matchmaking status for the rule to current status
    *************************************************************************************************/
    void TeamCountRule::updateAsyncStatus()
    {
    }

    FitScore TeamCountRule::evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const
    {
        if (mTeamCount != 0 && gameSession.getTeamCount() != mTeamCount)
        {
            return FIT_SCORE_NO_MATCH; 
        }

        return 0;
    }

    Rule* TeamCountRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW TeamCountRule(*this, matchmakingAsyncStatus);
    }

    // RETE implementations
    bool TeamCountRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {
        const TeamCountRuleDefinition &myDefn = getDefinition();
        Rete::ConditionBlock& conditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);

        if (mTeamCount != 0)
        {
            Rete::OrConditionList& capacityOrConditions = conditions.push_back();
            capacityOrConditions.push_back(Rete::ConditionInfo(
                Rete::Condition(TeamCountRuleDefinition::MAX_NUM_TEAMS_IN_GAME_KEY, 
                                myDefn.getWMEUInt16AttributeValue(mTeamCount), mRuleDefinition),
                0, this));
        }

        return true;
    }
    // end RETE
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
