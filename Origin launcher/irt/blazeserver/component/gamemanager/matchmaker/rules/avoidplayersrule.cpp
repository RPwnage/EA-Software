/*! ************************************************************************************************/
/*!
    \file    avoidplayersrule.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
    */
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/avoidplayersrule.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSerachSlave in calcFitPercent()

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    AvoidPlayersRule::AvoidPlayersRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : PlayersRule(ruleDefinition, matchmakingAsyncStatus)
    {
    }
    AvoidPlayersRule::AvoidPlayersRule(const AvoidPlayersRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : PlayersRule(otherRule, matchmakingAsyncStatus)
    {
    }

    AvoidPlayersRule::~AvoidPlayersRule()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief initialize rule
    ***************************************************************************************************/
    bool AvoidPlayersRule::initialize(const MatchmakingCriteriaData &criteria,
        const MatchmakingSupplementalData &mmSupplementalData, MatchmakingCriteriaError &err)
    {
        if (mmSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule not needed when doing resettable games only
            return true;
        }

        const AvoidPlayersRuleCriteria& ruleCriteria = criteria.getAvoidPlayersRuleCriteria();

        // check if disabled
        if (ruleCriteria.getAvoidList().empty() && ruleCriteria.getAvoidListIds().empty() && ruleCriteria.getAvoidAccountList().empty())
        {
            TRACE_LOG("[AvoidPlayersRule] disabled rule, avoid player lists empty/disabled.");
            return true;
        }

        // validate inputs
        const bool isSearchingGames = (mmSupplementalData.mMatchmakingContext.isSearchingGames());
        if (isSearchingGames && (mmSupplementalData.mPlayerToGamesMapping == nullptr))
        {
            char8_t errBuffer[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(errBuffer, sizeof(errBuffer), "Server error: player list rule for FG/GB improperly set up to cache search-able games for players.");
            err.setErrMessage(errBuffer);
            return false;
        }

        // set the wme name
        getDefinition().generateRuleInstanceWmeName(mRuleWmeName, mmSupplementalData.mPrimaryUserInfo->getSessionId());
        PlayersRule::setMatchmakingContext(mmSupplementalData.mMatchmakingContext);

        mPlayerProvider = (isSearchingGames? mmSupplementalData.mPlayerToGamesMapping : nullptr);

        // cache BlazeIdList's players, and if FG/GB setup listeners for games.
        PlayersRule::initIdSetsAndListeners(ruleCriteria.getAvoidList(), ruleCriteria.getAvoidAccountList(), getDefinition().getMaxPlayersUsed());

        // We can't lookup ids from the getAvoidListIds here because that would add a blocking operation to the Rule init.
        // Instead, the players should be looked up via playersRulesUserSetsToBlazeIds prior to the Rule being used (either via Scenarios, or in GameList creation)

        TRACE_LOG("[AvoidPlayersRule].initialize avoid list has '" << mPlayerIdSet.size() << "' players, '" << mAccountIdSet.size() << "' accounts.");
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Calc fit percent: no-match if game had a user in my avoid users, else match
    ***************************************************************************************************/
    void AvoidPlayersRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        uint32_t numWatched = getNumWatchedPlayers(gameSession.getGameId());
        if (numWatched != 0)
        {
            TRACE_LOG("[AvoidPlayersRule].calcFitPercent found a avoid list player in game id(" << gameSession.getGameId() << "). Avoiding game.");
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            debugRuleConditions.writeRuleCondition("Game %" PRIu64 " has %u avoided players.", gameSession.getGameId(), numWatched);
        }
        else
        {
            TRACE_LOG("[AvoidPlayersRule].calcFitPercent found a avoid list player in game id(" << gameSession.getGameId() << "). Max fit.");
            fitPercent = ZERO_FIT;
            isExactMatch = true;
            debugRuleConditions.writeRuleCondition("Game %" PRIu64 " has no avoided players.", gameSession.getGameId());
        }
        
    }


    /*! ************************************************************************************************/
    /*! \brief Calc fit percent: no-match if other rule had a user in my avoid users, else match
    ***************************************************************************************************/
    void AvoidPlayersRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        const AvoidPlayersRule& otherAvoidPlayersRule = static_cast<const AvoidPlayersRule &>(otherRule);
        if (!mPlayerIdSet.empty() || !mAccountIdSet.empty())
        {
            MatchmakingSession::MemberInfoList::const_iterator otherIter = otherAvoidPlayersRule.getMatchmakingSession()->getMemberInfoList().begin();
            MatchmakingSession::MemberInfoList::const_iterator otherEnd = otherAvoidPlayersRule.getMatchmakingSession()->getMemberInfoList().end();
            for(; otherIter != otherEnd; ++otherIter)
            {
                const MatchmakingSession::MemberInfo &memberInfo =
                    static_cast<const MatchmakingSession::MemberInfo &>(*otherIter);
                const Blaze::BlazeId otherBlazeId = memberInfo.mUserSessionInfo.getUserInfo().getId();
                const AccountId otherAccountId = memberInfo.mUserSessionInfo.getUserInfo().getPlatformInfo().getEaIds().getNucleusAccountId();
                const char8_t* foundBy = nullptr;

                if (mPlayerIdSet.find(otherBlazeId) != mPlayerIdSet.end())
                    foundBy = "BlazeId";
                else if (mAccountIdSet.find(otherAccountId) != mAccountIdSet.end())
                    foundBy = "AccountId";

                if (foundBy != nullptr)
                {
                    SPAM_LOG("[AvoidPlayersRule].calcFitPercent found avoid list player by " << foundBy << " (blazeid=" << otherBlazeId << ",sessionid=" << memberInfo.mUserSessionInfo.getSessionId() << ") in other MM session id(" << otherAvoidPlayersRule.getMMSessionId() << "). Avoiding session.");
                    fitPercent = FIT_PERCENT_NO_MATCH;
                    isExactMatch = false;
                    debugRuleConditions.writeRuleCondition("Found avoided player %s(%" PRIu64 ")", memberInfo.mUserSessionInfo.getUserInfo().getPersonaName(), otherBlazeId);
        
                    return;
                }
            }
        }

        // if here, no avoid players found.
        SPAM_LOG("[AvoidPlayersRule].calcFitPercent no avoid players found in other MM session id(" << otherAvoidPlayersRule.getMMSessionId() << ").");
        fitPercent = ZERO_FIT;
        isExactMatch = true;
        debugRuleConditions.writeRuleCondition("No avoided players.");
        
    }

    /*! ************************************************************************************************/
    /*! \brief override since default can't handle count via RETE
    ***************************************************************************************************/
    uint64_t AvoidPlayersRule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        if (diagnosticInfo.mIsRuleTotal)
        {
            uint64_t availableGames = helper.getTotalGamesCount();
            return (EA_LIKELY(availableGames >= getNumWatchedGames()) ?
                availableGames - getNumWatchedGames() :
                availableGames);
        }
        ERR_LOG("[AvoidPlayersRule].getDiagnosticGamesVisible: internal error: no op on unexpected category(" << diagnosticInfo.mCategoryTag.c_str() << ").");
        return 0;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

