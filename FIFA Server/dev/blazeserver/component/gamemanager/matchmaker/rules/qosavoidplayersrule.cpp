/*! ************************************************************************************************/
/*!
    \file    qosqosavoidplayersrule.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
    */
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/qosavoidplayersrule.h"
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
    QosAvoidPlayersRule::QosAvoidPlayersRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : PlayersRule(ruleDefinition, matchmakingAsyncStatus)
    {
    }
    QosAvoidPlayersRule::QosAvoidPlayersRule(const QosAvoidPlayersRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : PlayersRule(otherRule, matchmakingAsyncStatus)
    {
    }

    QosAvoidPlayersRule::~QosAvoidPlayersRule()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief initialize rule
    ***************************************************************************************************/
    bool QosAvoidPlayersRule::initialize(const MatchmakingCriteriaData &criteria,
        const MatchmakingSupplementalData &mmSupplementalData, MatchmakingCriteriaError &err)
    {
        // rule doesn't operate in GB or DS searches.
        if (!mmSupplementalData.mMatchmakingContext.hasPlayerJoinInfo() || 
            mmSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            return true;
        }

        // check if disabled
        if ((mmSupplementalData.mQosPlayerIdAvoidList == nullptr) || mmSupplementalData.mQosPlayerIdAvoidList->empty())
        {
            TRACE_LOG("[QosAvoidPlayersRule] disabled rule, avoid player lists empty/disabled.");
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
        PlayersRule::initPlayerIdSetAndListenersForBlazeIds(*(mmSupplementalData.mQosPlayerIdAvoidList), getDefinition().getMaxPlayersUsed());
 
 
        TRACE_LOG("[QosAvoidPlayersRule].initialize avoid list has '" << mPlayerIdSet.size() << "' players");
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Processes an update of the avoided player Ids when QoS fails in create mode.
    ***************************************************************************************************/
    void QosAvoidPlayersRule::updateQosAvoidPlayerIdsForCreateMode(const MatchmakingSupplementalData &mmSupplementalData)
    {
        // check if disabled
        if ((mmSupplementalData.mQosPlayerIdAvoidList == nullptr))
        {
            ERR_LOG("[QosAvoidPlayersRule] updateQosAvoidPlayerIdsForCreateMode(), supplimental data mQosPlayerIdAvoidList was nullptr.");
            return;
        }
        // cache BlazeIdList's players, and if FG/GB setup listeners for games.
        PlayersRule::initPlayerIdSetAndListenersForBlazeIds(*(mmSupplementalData.mQosPlayerIdAvoidList), getDefinition().getMaxPlayersUsed());
    }

    /*! ************************************************************************************************/
    /*! \brief Calc fit percent: no-match if game had a user in my avoid users, else match
    ***************************************************************************************************/
    void QosAvoidPlayersRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        if (hasWatchedPlayer(gameSession.getGameId()))
        {
            TRACE_LOG("[QosAvoidPlayersRule].calcFitPercent found a avoid list player in game id(" << gameSession.getGameId() << "). Avoiding game.");
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;

            debugRuleConditions.writeRuleCondition("Game %" PRIu64 " has avoided player due to QoS", gameSession.getGameId());
        }
        else
        {
            TRACE_LOG("[QosAvoidPlayersRule].calcFitPercent found a avoid list player in game id(" << gameSession.getGameId() << "). Max fit.");
            fitPercent = ZERO_FIT;
            isExactMatch = true;

            debugRuleConditions.writeRuleCondition("Game %" PRIu64 " doesn't have avoided player due to QoS", gameSession.getGameId());
        }
        
    }


    /*! ************************************************************************************************/
    /*! \brief Calc fit percent: no-match if other rule had a user in my avoid users, else match
    ***************************************************************************************************/
    void QosAvoidPlayersRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const QosAvoidPlayersRule& otherQosAvoidPlayersRule = static_cast<const QosAvoidPlayersRule &>(otherRule);
        

        if (!mPlayerIdSet.empty())
        {
            MatchmakingSession::MemberInfoList::const_iterator otherIter = otherQosAvoidPlayersRule.getMatchmakingSession()->getMemberInfoList().begin();
            MatchmakingSession::MemberInfoList::const_iterator otherEnd = otherQosAvoidPlayersRule.getMatchmakingSession()->getMemberInfoList().end();
            for(; otherIter != otherEnd; ++otherIter)
            {
                const MatchmakingSession::MemberInfo &memberInfo =
                    static_cast<const MatchmakingSession::MemberInfo &>(*otherIter);
                const Blaze::BlazeId otherBlazeId = memberInfo.mUserSessionInfo.getUserInfo().getId();

                if (mPlayerIdSet.find(otherBlazeId) != mPlayerIdSet.end())
                {
                    TRACE_LOG("[QosAvoidPlayersRule].calcFitPercent found avoid list player (blazeid=" << memberInfo.mUserSessionInfo.getUserInfo().getId() << ",sessionid=" << memberInfo.mUserSessionInfo.getSessionId() << ") in other MM session id(" << otherQosAvoidPlayersRule.getMMSessionId() << "). Avoiding session.");
                    fitPercent = FIT_PERCENT_NO_MATCH;
                    isExactMatch = false;

                    debugRuleConditions.writeRuleCondition("Found avoided player %s(%" PRIu64 ") due to QoS", memberInfo.mUserSessionInfo.getUserInfo().getPersonaName(), otherBlazeId);
        
                    return;
                }
            }
        }

        // if here, no avoid players found.
        TRACE_LOG("[QosAvoidPlayersRule].calcFitPercent no avoid players found in other MM session id(" << otherQosAvoidPlayersRule.getMMSessionId() << ").");
        fitPercent = ZERO_FIT;
        isExactMatch = true;

        debugRuleConditions.writeRuleCondition("No avoided players due to QoS.");
        
    }

    /*! ************************************************************************************************/
    /*! \brief override since default can't handle count via RETE
    ***************************************************************************************************/
    uint64_t QosAvoidPlayersRule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        if (diagnosticInfo.mIsRuleTotal)
        {
            uint64_t availableGames = helper.getTotalGamesCount();
            return (EA_LIKELY(availableGames >= getNumWatchedGames()) ?
                availableGames - getNumWatchedGames() :
                availableGames);
        }
        ERR_LOG("[QosAvoidPlayersRule].getDiagnosticGamesVisible: internal error: no op on unexpected category(" << diagnosticInfo.mCategoryTag.c_str() << ").");
        return 0;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

