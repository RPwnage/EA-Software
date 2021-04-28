/*! ************************************************************************************************/
/*!
    \file    xblblockplayersrule.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
    */
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/xblblockplayersrule.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSerachSlave in calcFitPercent()

#include "framework/util/shared/blazestring.h"
#include "framework/connection/outboundhttpservice.h"
#include "xblprivacyconfigs/tdf/xblprivacyconfigs.h"
#include "xblprivacyconfigs/rpc/xblprivacyconfigsslave.h"
#include "framework/rpc/oauthslave.h"
#include "framework/tdf/oauth_server.h"
#include "gamemanager/searchcomponent/searchslaveimpl.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

   ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // XblAccountIdToGamesMapping
  
    /*! ************************************************************************************************/
    /*! \brief subscribes the watcher to get the player's updates.
    ***************************************************************************************************/
    void XblAccountIdToGamesMapping::addPlayerWatcher(const ExternalXblAccountId xblId, XblAccountIdWatcher& watcher)
    {
        mPlayersToWatchers[xblId].insert(&watcher);
    }
    /*! ************************************************************************************************/
    /*! \brief if subscribed, unsubscribes watcher from getting player's updates. Cleans internal maps as needed.
    ***************************************************************************************************/
    void XblAccountIdToGamesMapping::removePlayerWatcher(const ExternalXblAccountId xblId, const XblAccountIdWatcher& watcher)
    {
        PlayersToWatchers::iterator iter = mPlayersToWatchers.find(xblId);
        if ((iter == mPlayersToWatchers.end()) ||
            (0 == iter->second.erase(const_cast<XblAccountIdWatcher*>(&watcher))))
        {
            TRACE_LOG("[XblAccountIdToGamesMapping].removePlayerWatcher no op due to attempt to clear invalid/non-existent watcher, " << ((iter == mPlayersToWatchers.end())? "" :" for player with no watchers"));
            return;
        }
        // clean up map if player has no one watching it anymore
        if (iter->second.empty())
        {
            TRACE_LOG("[XblAccountIdToGamesMapping].removePlayerWatcher removed last watcher for player xbl id(" << xblId << ")");
            mPlayersToWatchers.erase(iter);
        }
    }
    /*! ************************************************************************************************/
    /*! \brief notify all the player's watchers about the action
        \param[in] blazeId the player doing the action
        \param[in] isJoining whether player was joining, or else leaving
    ***************************************************************************************************/
    void XblAccountIdToGamesMapping::updateWatchersOnUserAction(const ExternalXblAccountId xblId,
        const Search::GameSessionSearchSlave& gameSession, bool isJoining)
    {
        PlayersToWatchers::iterator itWatchers = mPlayersToWatchers.find(xblId);

        if (itWatchers != mPlayersToWatchers.end())
        {
            // notify the watchers of the player
            WatcherList& watchers = itWatchers->second;
            if (IS_LOGGING_ENABLED(Logging::TRACE))
            {
                TRACE_LOG("[XblAccountIdToGamesMapping].updateWatchersOnUserAction: notify all(" << watchers.size() << ") watchers for xblId(" << xblId << "), about " << (isJoining? "join":"leave") << " of game(" << gameSession.getGameId() << ")");
            }
            WatcherList::iterator listSetIter = watchers.begin();
            WatcherList::iterator listSetEnd = watchers.end();
            for (; listSetIter != listSetEnd; ++listSetIter)
            {
                XblAccountIdWatcher *watcher = *listSetIter;
                watcher->onUserAction(mSearchSlaveImpl.getReteNetwork().getWMEManager(), xblId, gameSession, isJoining);
            }
        }
        else
        {
            TRACE_LOG("[XblAccountIdToGamesMapping].updateWatchersOnUserAction: no update for xblId(" << xblId << "), not being watched. Not notifying anyone of " << (isJoining? "join":"leave") << " of game(" << gameSession.getGameId() << ")");
        }
    }
    /*! ************************************************************************************************/
    /*! \brief Gets current set of games player is in.
    ***************************************************************************************************/
    const UserSessionGameSet* XblAccountIdToGamesMapping::getPlayerCurrentGames(const ExternalXblAccountId xblId) const
    {
        PlatformInfo tempPlatformInfo;
        convertToPlatformInfo(tempPlatformInfo, xblId, nullptr, INVALID_ACCOUNT_ID, xone);
        UserSessionId sessionId = gUserSessionManager->getUserSessionIdByPlatformInfo(tempPlatformInfo);
        if (sessionId == INVALID_USER_SESSION_ID)
        {
            TRACE_LOG("[XblAccountIdToGamesMapping] Attempt to find game sessions for player xbl id(" << xblId << ") who is not online. No games found.");
            return nullptr;
        }
        return mSearchSlaveImpl.getUserSessionGameSetBySessionId(sessionId);
    }

    void XblAccountIdToGamesMapping::markWatcherForInitialMatches(XblAccountIdWatcher& watcher)
    {
        mWatchersWithInitialMatches.insert(&watcher);
    }

    void XblAccountIdToGamesMapping::clearWatcherForInitialMatches(const XblAccountIdWatcher& watcher)
    {
        mWatchersWithInitialMatches.erase(const_cast<XblAccountIdWatcher*>(&watcher));
    }

    void XblAccountIdToGamesMapping::processWatcherInitialMatches()
    {
        // iterate over the matches, add their WMEs
        for (auto& watchter : mWatchersWithInitialMatches)
        {
            watchter->onProcessInitalMatches(mSearchSlaveImpl.getReteNetwork().getWMEManager());
        }

        mWatchersWithInitialMatches.clear();
    }



    XblBlockPlayersRule::XblBlockPlayersRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(ruleDefinition, matchmakingAsyncStatus),
        mPlayerProvider(nullptr),
        mGamesToWatchCounts(BlazeStlAllocator("XblBlockPlayersRule::mGamesToWatchCounts", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mRuleWmeName(""),
        mContextAllowsRete(ruleDefinition.isReteCapable())
    {
    }
    XblBlockPlayersRule::XblBlockPlayersRule(const XblBlockPlayersRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(otherRule, matchmakingAsyncStatus),
        mGamesToWatchCounts(BlazeStlAllocator("XblBlockPlayersRule::mGamesToWatchCounts", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mRuleWmeName(otherRule.mRuleWmeName),
        mContextAllowsRete(otherRule.mContextAllowsRete)
    {
        mXblAccountIdSet = otherRule.mXblAccountIdSet;
        mPlayerProvider = otherRule.mPlayerProvider;
        mGamesToWatchCounts = otherRule.mGamesToWatchCounts;
    }

    XblBlockPlayersRule::~XblBlockPlayersRule()
    {
        if (mPlayerProvider != nullptr)
        {
            ExternalXblAccountIdSet::const_iterator iter = mXblAccountIdSet.begin();
            ExternalXblAccountIdSet::const_iterator iterEnd = mXblAccountIdSet.end();
            for (; iter != iterEnd; ++iter)
            {
                mPlayerProvider->removePlayerWatcher(*iter, *this);
            }

            if (isReteCapable())
            {
                // remove initial matches if we haven't processed them yet
                if (!mInitialWatchedGames.empty())
                {
                    // get initial matches set up to be inserted into the WME tree.
                    mPlayerProvider->clearWatcherForInitialMatches(*this);
                }

                // clean up WMEs
                if (!mXblAccountIdSet.empty())
                {
                    getDefinition().removeAllWMEsForRuleInstance(mPlayerProvider->getGameSessionRepository().getReteNetwork(), mRuleWmeName.c_str());
                }
            }
        }

        

        mXblAccountIdSet.clear();
        mPlayerProvider = nullptr;
        mGamesToWatchCounts.clear();
        mInitialWatchedGames.clear();
    }

    /*! ************************************************************************************************/
    /*! \brief initialize rule
    ***************************************************************************************************/
    bool XblBlockPlayersRule::initialize(const MatchmakingCriteriaData &criteria,
        const MatchmakingSupplementalData &mmSupplementalData, MatchmakingCriteriaError &err)
    {
        if (!mmSupplementalData.mMatchmakingContext.hasPlayerJoinInfo() || 
            mmSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Block rule is disabled for game browser.
            return true;
        }

        // Disable the rule is explicitly told to:
        const XblBlockPlayersRuleCriteria& ruleCriteria = criteria.getXblBlockPlayersRuleCriteria();
        if (ruleCriteria.getDisableRule())
        {
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
        mPlayerProvider = (isSearchingGames? mmSupplementalData.mXblAccountIdToGamesMapping : nullptr);

        if (mmSupplementalData.mXblIdBlockList == nullptr)
        {
            char8_t errBuffer[MatchmakingCriteriaError::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(errBuffer, sizeof(errBuffer), "Server error: external id block list is missing. Pointer should not be nullptr, even if no users are blocked.");
            err.setErrMessage(errBuffer);
            return false;
        }

        // set the wme name
        getDefinition().generateRuleInstanceWmeName(mRuleWmeName, mmSupplementalData.mPrimaryUserInfo->getSessionId());

        // not using rete in the create game context or resettable dedicated server context
        mContextAllowsRete = mmSupplementalData.mMatchmakingContext.isSearchingGames();

        // Convert list to set and add watchers:
        for(const ExternalXblAccountId xblId : *mmSupplementalData.mXblIdBlockList)
        {
            ExternalXblAccountIdSet::insert_return_type ret = mXblAccountIdSet.insert(xblId);
            BLAZE_TRACE_LOG(Log::USER, "[XblBlockPlayersRule].getUserBlockList: Block xblId- " << xblId << ".");

            if (ret.second == true && mPlayerProvider)
            {
                // only need subscribing for player updates, if searching games
                mPlayerProvider->addPlayerWatcher(xblId, *this);

                // increment game's watched-players count by 1
                const UserSessionGameSet *userGameSet = mPlayerProvider->getPlayerCurrentGames(xblId);
                if ((userGameSet != nullptr) && !userGameSet->empty())
                {
                    UserSessionGameSet::const_iterator itGame = userGameSet->begin();
                    UserSessionGameSet::const_iterator itGameEnd = userGameSet->end();
                    for (; itGame != itGameEnd; ++itGame)
                    {
                        Search::GameSessionSearchSlave* gameSession = mPlayerProvider->getGameSessionRepository().getGameSessionSlave(*itGame);
                        if(gameSession != nullptr)
                        {
                            SPAM_LOG("[PlayersRule].initPlayerIdSetAndListenersForBlazeIds Rule "<< (getRuleName()? getRuleName() : "?") << " adding found game id(" << (*itGame) << ") having player xbl id(" << xblId << ").");
                            incrementGameWatchCount(*itGame);
                            mInitialWatchedGames.insert(*itGame);
                        }
                        else
                        {
                            ERR_LOG("[PlayersRule].initPlayerIdSetAndListenersForBlazeIds Rule "<< (getRuleName()? getRuleName() : "?") << " game id(" << (*itGame) << ") having player xbl id(" << xblId << ") was nullptr when lookup was attempted.");
                        }
                    }
                }
            }
        }


        if (!mInitialWatchedGames.empty() && isReteCapable())
        {
            // get initial matches set up to be inserted into the WME tree.
            mPlayerProvider->markWatcherForInitialMatches(*this);
        }

        TRACE_LOG("[XblBlockPlayersRule].initialize xblblock list has '" << mXblAccountIdSet.size() << "' external ids");
        return true;
    }

    BlazeRpcError XblBlockPlayersRule::fillBlockList(const UserJoinInfoList& matchmakingUsers, ExternalXblAccountIdList& blockListXblIdOut, MatchmakingCriteriaError& err)
    {
        UserJoinInfoList::const_iterator extIdIter = matchmakingUsers.begin();
        UserJoinInfoList::const_iterator extIdEnd = matchmakingUsers.end();
        for(; extIdIter != extIdEnd; ++extIdIter)
        {
            ClientPlatformType clientPlatform = (*extIdIter)->getUser().getUserInfo().getPlatformInfo().getClientPlatform();
            if ((clientPlatform != xone) && (clientPlatform != xbsx))
            {
                continue;
            }

            ExternalXblAccountId xblId = (*extIdIter)->getUser().getUserInfo().getPlatformInfo().getExternalIds().getXblAccountId();
            if (xblId == INVALID_EXTERNAL_ID)
            {
                BLAZE_WARN_LOG(Log::USER, "[XblBlockPlayersRule].fillBlockList: INVALID_EXTERNAL_ID provided, skipping block list lookup.");
                continue;
            }

            Blaze::OAuth::GetUserXblTokenRequest req;
            Blaze::OAuth::GetUserXblTokenResponse rsp;
            req.getRetrieveUsing().setExternalId(xblId);

            Blaze::OAuth::OAuthSlave *oAuthSlave = (Blaze::OAuth::OAuthSlave*) gController->getComponent(
                Blaze::OAuth::OAuthSlave::COMPONENT_ID, false, true);
            if (oAuthSlave == nullptr)
            {
                BLAZE_ERR_LOG(Log::USER, "[XblBlockPlayersRule].fillBlockList: OAuthSlave was unavailable, reputation lookup failed.");
                continue;
            }

            // Super permissions are needed to lookup other users' tokens
            UserSession::SuperUserPermissionAutoPtr autoPtr(true);
            BlazeRpcError error = oAuthSlave->getUserXblToken(req, rsp);
            if (error != ERR_OK)
            {
                BLAZE_ERR_LOG(Log::USER, "[XblBlockPlayersRule].fillBlockList: Failed to retrieve xbl auth token from service, for external id: " << xblId << ", with error '" << ErrorHelp::getErrorName(error) <<"'.");
                continue;
            }


            Blaze::XBLPrivacy::GetBlockedListRequest getBlockedListRequest;
            getBlockedListRequest.setXuid(xblId);
            getBlockedListRequest.getGetBlockedListRequestHeader().setAuthToken(rsp.getXblToken());


            Blaze::XBLPrivacy::GetBlockedListResponse getBlockedListResponse;
            Blaze::XBLPrivacy::XBLPrivacyConfigsSlave * xblPrivacyConfigsSlave = (Blaze::XBLPrivacy::XBLPrivacyConfigsSlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLPrivacy::XBLPrivacyConfigsSlave::COMPONENT_INFO.name);

            if (xblPrivacyConfigsSlave == nullptr)
            {
                BLAZE_ERR_LOG(Log::USER, "[XblBlockPlayersRule].fillBlockList: Failed to instantiate XBLPrivacyConfigsSlave object. ExternalXblAccountId: " << xblId << ".");
                continue;
            }

            BLAZE_TRACE_LOG(Log::USER, "[XblBlockPlayersRule].fillBlockList: Beginning external block list lookup. ExternalXblAccountId: " << xblId << ".");
            // update metrics
            //UserSessionsManagerMetrics* metricsObj = gUserSessionManager->getMetricsObj();
            //++(metricsObj->mTotalBlockListLookups);

            // We need to run a shorter timeout here, because if the 1st party service is down, we don't want to block so long that the RPC that triggered this update times out on the client
            RpcCallOptions rpcCallOptions;
            rpcCallOptions.timeoutOverride = gUserSessionManager->getConfig().getReputation().getReputationLookupTimeout();
            error = xblPrivacyConfigsSlave->getBlockedList(getBlockedListRequest, getBlockedListResponse, rpcCallOptions);
            if (error == XBLPRIVACY_AUTHENTICATION_REQUIRED)
            {
                BLAZE_TRACE_LOG(Log::USER, "[XblBlockPlayersRule].fillBlockList: Authentication with Microsoft failed, token may have expired. Forcing token refresh and retrying.");
                req.setForceRefresh(true);
                error = oAuthSlave->getUserXblToken(req, rsp);
                if (error == ERR_OK)
                {
                    getBlockedListRequest.getGetBlockedListRequestHeader().setAuthToken(rsp.getXblToken());
                    error = xblPrivacyConfigsSlave->getBlockedList(getBlockedListRequest, getBlockedListResponse, rpcCallOptions);
                }
            }
            if (error != ERR_OK)
            {
                BLAZE_ERR_LOG(Log::USER, "[XblBlockPlayersRule].fillBlockList: Failed to get block list. xblId: " << xblId << ", with error '" << ErrorHelp::getErrorName(error) << "'.");
                // TODO: If we failed to get the block list due to rate limiting (XBLPRIVACY_RATE_LIMIT_EXCEEDED), then we should retry the request after the tripped limits period ends. [GOSOPS-161807]
                // For now, to comply with XR-073, we fail this rule if getBlockedList returns any error.
                return error;
            }

            BLAZE_SPAM_LOG(Log::USER, "[XblBlockPlayersRule].fillBlockList: getReputationResponse contains '" << getBlockedListResponse.getUsers().size() 
                << "' items for user '" << getBlockedListRequest.getXuid() << "'.");

            XBLPrivacy::UserList::const_iterator usersIter = getBlockedListResponse.getUsers().begin();
            XBLPrivacy::UserList::const_iterator usersEnd = getBlockedListResponse.getUsers().end();
            for (; usersIter != usersEnd; ++usersIter)
            {
                ExternalXblAccountId xblIdUser;
                blaze_str2int((*usersIter)->getXuid(), &xblIdUser);

                // Add to the block list:
                blockListXblIdOut.push_back(xblIdUser);
            }
        }

        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Populates WMEs for initial matches
    ***************************************************************************************************/
    void XblBlockPlayersRule::onProcessInitalMatches(GameManager::Rete::WMEManager& wmeManager)
    {
        for (auto& initialWatchedGameId : mInitialWatchedGames)
        {
            getDefinition().updateWMEOnPlayerAction(wmeManager, mRuleWmeName.c_str(), initialWatchedGameId, true);
        }

        mInitialWatchedGames.clear();
    }

    /*! ************************************************************************************************/
    /*! \brief slave use only. Update my tracking of the games players are in as needed.
    ***************************************************************************************************/
    void XblBlockPlayersRule::onUserAction(GameManager::Rete::WMEManager& wmeManager, const ExternalXblAccountId xblId, const Search::GameSessionSearchSlave& gameSession, bool isJoining)
    {
        // update tracked count. Pre: Assumption is there's no duplicate hits here for the same
        // player join/leave op, so that for efficiency we can avoid tracking set of actual players (just counts).
        uint32_t& count = mGamesToWatchCounts[gameSession.getGameId()];
        if (isJoining)
        {
            count++;
            // upsert a wme, a player was added, if required
            if (isReteCapable())
            {
                getDefinition().updateWMEOnPlayerAction(wmeManager, mRuleWmeName.c_str(), gameSession.getGameId(), true);
            }
        }
        else
        {
            if (count == 0)
            {
                ERR_LOG("[PlayerWatcher].onUserAction internal error, attempted decrementing count below zero, game(" << gameSession.getGameId() << ") user(" << xblId << ")");
                return;
            }
            // Performance note: can skip clearing game's key in mGamesToWatchCounts if
            // 'count' dropped to 0, given the expected lifetimes of rules not too long anyhow.
            count--;
            if (count == 0)
            {
                // remove the WME
                // upsert a wme, a player was added, if required
                if (isReteCapable())
                {
                    getDefinition().updateWMEOnPlayerAction(wmeManager, mRuleWmeName.c_str(), gameSession.getGameId(), false);
                }
                // remove the entry from the map, too, as this may stick around if we're using the browser
                mGamesToWatchCounts.erase(gameSession.getGameId());
                mInitialWatchedGames.erase(gameSession.getGameId());
            }
        }
        TRACE_LOG("[PlayerWatcher].onUserAction game(" << gameSession.getGameId() << ") user(" << xblId << ") , doing " << (isJoining? "join":"leave") << ", changed count to " << count);
    }

    bool XblBlockPlayersRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {
        if (isReteCapable())
        {
            if (EA_LIKELY(!mRuleWmeName.empty()))
            {
                if (!isDisabled())
                {

                    Rete::ConditionBlock& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
                    Rete::OrConditionList& baseOrConditions = baseConditions.push_back();

                    baseOrConditions.push_back(Rete::ConditionInfo(Rete::Condition(mRuleWmeName.c_str(),
                        mRuleDefinition.getWMEBooleanAttributeValue(true), getDefinition(), isAvoidRule()),
                        mRuleDefinition.getWeight(), this));


                    if (!isRequiresPlayers())
                    {
                        // rule accepts games that don't have the listed players
                        baseOrConditions.push_back(Rete::ConditionInfo(Rete::Condition(mRuleWmeName.c_str(),
                            mRuleDefinition.getWMEBooleanAttributeValue(true), getDefinition(), !isAvoidRule()),
                            0, this));

                    }
                }
            }
            else
            {
                ERR_LOG("[XblBlockPlayersRule].addConditions disabled, no ruleWmeName set.");
            }
        }
        else
        {
            WARN_LOG("[XblBlockPlayersRule].addConditions disabled, non-rete");
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Calc fit percent: no-match if game had a user in my xblblock users, else match
    ***************************************************************************************************/
    void XblBlockPlayersRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        
        if (hasWatchedPlayer(gameSession.getGameId()))
        {
            debugRuleConditions.writeRuleCondition("has XBL blocked player");

            TRACE_LOG("[XblBlockPlayersRule].calcFitPercent found a xblblock list player in game id(" << gameSession.getGameId() << "). XblBlocking game.");
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
        }
        else
        {
            debugRuleConditions.writeRuleCondition("no XBL blocked players");

            TRACE_LOG("[XblBlockPlayersRule].calcFitPercent found a xblblock list player in game id(" << gameSession.getGameId() << "). Zero fit.");
            fitPercent = ZERO_FIT;
            isExactMatch = true;
        }
    }


    /*! ************************************************************************************************/
    /*! \brief Calc fit percent: no-match if other rule had a user in my xblblock users, else match
    ***************************************************************************************************/
    void XblBlockPlayersRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        const XblBlockPlayersRule& otherXblBlockPlayersRule = static_cast<const XblBlockPlayersRule &>(otherRule);
        

        if (isSessionBlocked(otherXblBlockPlayersRule.getMatchmakingSession()))
        {
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;

            debugRuleConditions.writeRuleCondition("Found blocked players.");

            return;
        }

        // if here, no xblblock players found.
        TRACE_LOG("[XblBlockPlayersRule].calcFitPercent no xblblock players found in other MM session id(" << otherXblBlockPlayersRule.getMMSessionId() << ").");
        fitPercent = ZERO_FIT;
        isExactMatch = true;

        debugRuleConditions.writeRuleCondition("No blocked players.");
        
    }

    bool XblBlockPlayersRule::isSessionBlocked(const MatchmakingSession* otherSession) const
    {
        if (!mXblAccountIdSet.empty() && otherSession != nullptr)
        {
            MatchmakingSession::MemberInfoList::const_iterator otherIter = otherSession->getMemberInfoList().begin();
            MatchmakingSession::MemberInfoList::const_iterator otherEnd = otherSession->getMemberInfoList().end();
            for (; otherIter != otherEnd; ++otherIter)
            {
                const MatchmakingSession::MemberInfo &memberInfo = static_cast<const MatchmakingSession::MemberInfo &>(*otherIter);
                const Blaze::ExternalXblAccountId otherXblId = memberInfo.mUserSessionInfo.getUserInfo().getPlatformInfo().getExternalIds().getXblAccountId();
                if (mXblAccountIdSet.find(otherXblId) != mXblAccountIdSet.end())
                {
                    TRACE_LOG("[XblBlockPlayersRule].isSessionBlocked found xblblock list player (xblid=" << otherXblId << ",sessionid=" << memberInfo.mUserSessionInfo.getSessionId() << ") in other MM session id(" << otherSession->getMMSessionId() << "). XblBlocking session.");
                    return true;
                }
            }
        }

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief override since default can't handle count via RETE
    ***************************************************************************************************/
    uint64_t XblBlockPlayersRule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        if (diagnosticInfo.mIsRuleTotal)
        {
            uint64_t availableGames = helper.getTotalGamesCount();
            return (EA_LIKELY(availableGames >= mGamesToWatchCounts.size()) ?
                availableGames - mGamesToWatchCounts.size() :
                availableGames);
        }
        ERR_LOG("[XblBlockPlayersRule].getDiagnosticGamesVisible: internal error: no op on unexpected category(" << diagnosticInfo.mCategoryTag.c_str() << ").");
        return 0;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

