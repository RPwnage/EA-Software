/*! ************************************************************************************************/
/*!
    \file    preferredplayersrule.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
    */
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/preferredplayersrule.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSearchSlave in calcFitPercent()

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    PreferredPlayersRule::PreferredPlayersRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : PlayersRule(ruleDefinition, matchmakingAsyncStatus),
        mRequirePreferredPlayer(false)
    {
    }
    PreferredPlayersRule::PreferredPlayersRule(const PreferredPlayersRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : PlayersRule(otherRule, matchmakingAsyncStatus),
        mRequirePreferredPlayer(otherRule.mRequirePreferredPlayer)
    {
    }
    PreferredPlayersRule::~PreferredPlayersRule()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief initialize rule
    ***************************************************************************************************/
    bool PreferredPlayersRule::initialize(const MatchmakingCriteriaData &criteria,
        const MatchmakingSupplementalData &mmSupplementalData, MatchmakingCriteriaError &err)
    {
        // This rule does not apply to dedicated server games
        if (mmSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            return true;
        }

        // validate if requires a preferred player, the set of them cannot be empty
        const PreferredPlayersRuleCriteria& ruleCriteria = criteria.getPreferredPlayersRuleCriteria();
        mRequirePreferredPlayer = ruleCriteria.getRequirePreferredPlayer();
        if (mRequirePreferredPlayer && ruleCriteria.getPreferredList().empty() && ruleCriteria.getPreferredAccountList().empty() && (ruleCriteria.getPreferredListId() == EA::TDF::OBJECT_ID_INVALID))
        {
            err.setErrMessage("mRequirePreferredPlayer = true, but 0 input preferred players were specified for matchmaking");
            return false;
        }

        // check if disabled  (list id has put players in the PreferredList.  If list is empty, disable the rule unless it was required)
        if (ruleCriteria.getPreferredList().empty() && ruleCriteria.getPreferredAccountList().empty() && (!mRequirePreferredPlayer || ruleCriteria.getPreferredListId() == EA::TDF::OBJECT_ID_INVALID))
        {
            TRACE_LOG("[PreferredPlayersRule] disabled, preferred players empty/disabled.");
            return true;
        }

        // validate remaining inputs
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

        // cache BlazeIdList's and AccountIdList's players, and if FG setup listeners for games.
        PlayersRule::initIdSetsAndListeners(ruleCriteria.getPreferredList(), ruleCriteria.getPreferredAccountList(), getDefinition().getMaxPlayersUsed());

        // We can't lookup ids from the getPreferredListId here because that would add a blocking operation to the Rule init.
        // Instead, the players should be looked up via playersRulesUserSetsToBlazeIds prior to the Rule being used (either via Scenarios, or in GameList creation)

        TRACE_LOG("[PreferredPlayersRule].initialize preferred list has '" << mPlayerIdSet.size() << "' players, '" << mAccountIdSet.size() << "' accounts, mRequirePreferredPlayer = " << mRequirePreferredPlayer);
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Calc fit percent: 100% match if it had a user in my preferred users or I have no
        users preferred, otherwise 0% match. If required preferred player but had none, no-match.
    ***************************************************************************************************/
    void PreferredPlayersRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        uint32_t numWatched = getNumWatchedPlayers(gameSession.getGameId());
        if (numWatched != 0)
        {
            TRACE_LOG("[PreferredPlayersRule].calcFitPercent found a preferred list player in game id(" << gameSession.getGameId() << "). Max fit.");
            fitPercent = 1.0f;
            isExactMatch = true;
            debugRuleConditions.writeRuleCondition("Game %" PRIu64 " has %u preferred players.", gameSession.getGameId(), numWatched);
        }
        else if (mRequirePreferredPlayer)
        {
            TRACE_LOG("[PreferredPlayersRule].calcFitPercent mRequirePreferredPlayer was true but no preferred player was found in game id(" << gameSession.getGameId() << "). No match.");
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
            debugRuleConditions.writeRuleCondition("Game %" PRIu64 " has no preferred players, they are required.", gameSession.getGameId());
        }
        else
        {
            TRACE_LOG("[PreferredPlayersRule].calcFitPercent no preferred players found in game id(" << gameSession.getGameId() << "). Not adding fit.");
            fitPercent = ZERO_FIT;
            isExactMatch = mPlayerIdSet.empty() && mAccountIdSet.empty(); // exact match only if no players preferred.
            debugRuleConditions.writeRuleCondition("Game %" PRIu64 " has no preferred players.", gameSession.getGameId());
        }
        
    }


    /*! ************************************************************************************************/
    /*! \brief Calc fit percent: 100% match if it had a user in my preferred users or I have no
        users preferred, otherwise 0% match. If required preferred player but had none, no-match.
    ***************************************************************************************************/
    void PreferredPlayersRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const PreferredPlayersRule& otherPreferredPlayersRule = static_cast<const PreferredPlayersRule &>(otherRule);
        

        if (!mPlayerIdSet.empty() || !mAccountIdSet.empty())
        {
            MatchmakingSession::MemberInfoList::const_iterator otherIter = otherPreferredPlayersRule.getMatchmakingSession()->getMemberInfoList().begin();
            MatchmakingSession::MemberInfoList::const_iterator otherEnd = otherPreferredPlayersRule.getMatchmakingSession()->getMemberInfoList().end();
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
                    TRACE_LOG("[PreferredPlayersRule].calcFitPercent found preferred list player by " << foundBy << " (blazeid=" << otherBlazeId << ",accountid=" << otherAccountId << ",sessionid=" << memberInfo.mUserSessionInfo.getSessionId() << ") in other MM session id(" << otherPreferredPlayersRule.getMatchmakingSession()->getMMSessionId() << "). Max fit.");
                    fitPercent = 1.0f;
                    isExactMatch = true;

                    debugRuleConditions.writeRuleCondition("Preferred player %s(%" PRIu64 ") with accountId(%" PRId64 ") found by %s", memberInfo.mUserSessionInfo.getUserInfo().getPersonaName(), otherBlazeId, otherAccountId, foundBy);
        
                    return;
                }
            }
        }

        // if here, no preferred players found.
        if (mRequirePreferredPlayer)
        {
            TRACE_LOG("[PreferredPlayersRule].calcFitPercent mRequirePreferredPlayer was true but no preferred player was found in other MM session id(" << otherPreferredPlayersRule.getMMSessionId() << "). No match.");
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
        }
        else
        {
            TRACE_LOG("[PreferredPlayersRule].calcFitPercent no preferred players found in other MM session id(" << otherPreferredPlayersRule.getMMSessionId() << "). Not adding fit.");
            fitPercent = ZERO_FIT;
            isExactMatch = mPlayerIdSet.empty() && mAccountIdSet.empty(); // exact match only if no players preferred.       
        }
        debugRuleConditions.writeRuleCondition("No required preferred players");
        
    }

    /*! ************************************************************************************************/
    /*! \brief override since default can't handle count via RETE
    ***************************************************************************************************/
    uint64_t PreferredPlayersRule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        if (diagnosticInfo.mIsRuleTotal)
        {
            return (mRequirePreferredPlayer ? getNumWatchedGames() : helper.getTotalGamesCount());
        }
        ERR_LOG("[PreferredPlayersRule].getDiagnosticGamesVisible: internal error: no op on unexpected category(" << diagnosticInfo.mCategoryTag.c_str() << ").");
        return 0;
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

