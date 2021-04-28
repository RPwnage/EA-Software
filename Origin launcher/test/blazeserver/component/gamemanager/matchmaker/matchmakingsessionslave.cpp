/*! ************************************************************************************************/
/*!
\file matchmakingslave.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/connectionmanager.h"
#include "framework/event/eventmanager.h"
#include "gamemanager/matchmaker/matchmakingsessionslave.h"
#include "gamemanager/matchmaker/matchmakingslave.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/tdf/gamemanagerevents_server.h"
#include "gamemanager/matchmaker/rules/preferredgamesrule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief Constructor for the MatchmakingSessionSlave
        
        \param[in] matchmakingSlave - our parent matchmaking slave that created us
        \param[sessionSlaveId - our individual sessionSlave id (only unique to our slave)
    ***************************************************************************************************/
    MatchmakingSessionSlave::MatchmakingSessionSlave(MatchmakingSlave& matchmakingSlave, Rete::ProductionId sessionSlaveId, InstanceId originatingInstanceId)
        : mMatchmakingSlave(matchmakingSlave),
        mReteProductionId(sessionSlaveId),
        mOriginatingInstanceId(originatingInstanceId),
        mGameIntrusiveMap(BlazeStlAllocator("mGameIntrusiveMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mTopMatchHeapSize(0),
        mHasExpired(false),
        mNewSessionFirstMatchAttempted(false),
        mHasMatchedDebugGames(false),
        mStartedDelayed(false),
        mMatchmakingAsyncStatus(),
        mCriteria(matchmakingSlave.getRuleDefinitionCollection(), mMatchmakingAsyncStatus),
        mOwnerBlazeId(INVALID_BLAZE_ID),
        mOwnerSessionId(INVALID_USER_SESSION_ID),
        mSessionStartTime(0),
        mStartingDecayAge(0),
        mSessionAgeSeconds(0),
        mSessionTimeoutSeconds(0),
        mNextElapsedSecondsUpdate(UINT32_MAX),
        mNumFailedFoundGameRequests(0),
        mNumFindGameMatches(0),
        mTotalFitScore(0),
        mPrivilegedGameIdPreference(GameManager::INVALID_GAME_ID),
        mOriginatingSessionId(SlaveSession::INVALID_SESSION_ID),
        mOriginatingScenarioId(GameManager::INVALID_SCENARIO_ID),
        mOtaQuickOutCount(0),
        mOtaFullCount(0),
        mOtrCount(0),
        mMembersUserSessionInfo(DEFAULT_BLAZE_ALLOCATOR, "MembersUserSessionInfo")
    {
        mProductionMatchHeap.clear();
    }

    /*! ************************************************************************************************/
    /*! \brief Destructor for the MatchmakingSlave
    ***************************************************************************************************/
    MatchmakingSessionSlave::~MatchmakingSessionSlave()
    {
        mCriteria.cleanup();

        //Drop all our references.
        Blaze::GameManager::MMGameIntrusiveMap::iterator iter = mGameIntrusiveMap.begin();
        Blaze::GameManager::MMGameIntrusiveMap::iterator end = mGameIntrusiveMap.end();

        while (iter != end)
        {
            // store off our value before we increment.
            Blaze::GameManager::MmSessionGameIntrusiveNode &n = *(iter->second);
            // increment here so we can delete.
            ++iter;
            // Note: we don't call destroyIntrusiveNode here to prevent from erase from the gameMap, which
            // is just getting destroyed by this destructor.
            mMatchmakingSlave.deleteGameIntrusiveNode(&n);
        }
        mGameIntrusiveMap.clear();

        EA_ASSERT(mMatchmakingAsyncStatus.getUEDRuleStatusMap().empty());
        EA_ASSERT(mMatchmakingAsyncStatus.getGameAttributeRuleStatusMap().empty());
        EA_ASSERT(mMatchmakingAsyncStatus.getPlayerAttributeRuleStatusMap().empty());
    }

    /*! ************************************************************************************************/
    /*! \brief Initialize the session given the startMatchmakingRequest.  Adds ourself to the rete tree.

        \param[in] req - the startMatchmakingRequest sent from the client.
        \param[in] evaluateGameProtocolVersionString - whether session will evaluate game version
    ***************************************************************************************************/
    BlazeRpcError MatchmakingSessionSlave::initialize(const StartMatchmakingInternalRequest& startInternalRequest,
        bool evaluateGameProtocolVersionString, MatchmakingCriteriaError& criteriaError,
        const DedicatedServerOverrideMap &dedicatedServerOverrideMap)
    {
        // Store off all values in the start matchmaking request
        startInternalRequest.copyInto(mStartMatchmakingInternalRequest);
        const StartMatchmakingRequest& startMMRequest = getStartMatchmakingRequest();
        mOwnerBlazeId = getOwnerSessionInfo().getUserInfo().getId();
        mOwnerSessionId = getOwnerSessionInfo().getSessionId();

        if (!gUserSessionManager->getSessionExists(getOwnerSessionInfo().getSessionId()))
        {
            return SEARCH_ERR_MATCHMAKING_USERSESSION_NOT_FOUND;
        }

        // &getPrimaryUserSessionInfo() == nullptr
        if (Blaze::GameManager::getHostSessionInfo(mStartMatchmakingInternalRequest.getUsersInfo()) == nullptr)
        {
            ERR_LOG("[MatchmakingSessionSlave(" << mReteProductionId << "/" << getOwnerBlazeId() << "/"
                << getOwnerPersonaName() << ")] - Missing PrimaryUserSessionInfo (host) in StartMatchmakingInternalRequest. " 
                << mStartMatchmakingInternalRequest.getUsersInfo().size() << " users in the request.");
            return SEARCH_ERR_MATCHMAKING_USERSESSION_NOT_FOUND;
        }

        // if remote matchmaker slave initiated this session then point to it here
        InstanceId instanceId = GetInstanceIdFromInstanceKey64(mReteProductionId);
        if (gController->getInstanceId() != instanceId)
        {
            SlaveSessionPtr sess = gController->getConnectionManager().getSlaveSessionByInstanceId(instanceId);
            if (sess != nullptr)
            {
                mOriginatingSessionId = sess->getId();
            }
        }

        //Create node for every user in the group 
        for (UserJoinInfoList::iterator itr = mStartMatchmakingInternalRequest.getUsersInfo().begin(), end = mStartMatchmakingInternalRequest.getUsersInfo().end(); itr != end; ++itr)
        {
            UserSessionInfo& userInfo = (*itr)->getUser();
            mMembersUserSessionInfo.push_back(&userInfo);

            // All members of the request have the same scenario id set, so any is valid:
            if (mOriginatingScenarioId == GameManager::INVALID_SCENARIO_ID)
            {
                mOriginatingScenarioId = (*itr)->getOriginatingScenarioId();
            }
        }

        MatchmakingSupplementalData data(MATCHMAKING_CONTEXT_MATCHMAKER_FIND_GAME);
        startMMRequest.getPlayerJoinData().copyInto(data.mPlayerJoinData);
        data.mNetworkTopology = startMMRequest.getGameCreationData().getNetworkTopology();
        data.mGameTypeList.push_back(startMMRequest.getCommonGameData().getGameType());
        data.mPrimaryUserInfo = &getPrimaryUserSessionInfo();
        data.mMembersUserSessionInfo = &mMembersUserSessionInfo;
        data.mGameProtocolVersionString = startMMRequest.getCommonGameData().getGameProtocolVersionString();
        data.mEvaluateGameProtocolVersionString = evaluateGameProtocolVersionString;
        data.mDuplicateTeamsAllowed = startMMRequest.getGameCreationData().getGameSettings().getAllowSameTeamId();
        const Util::NetworkQosData &networkQosData = getPrimaryUserSessionInfo().getQosData();
        networkQosData.copyInto(data.mNetworkQosData);
        data.mHasMultipleStrictNats = false;
        data.mAllIpsResolved = startInternalRequest.getUserSessionIpInformation().getAreAllSessionIpsResolved();
        data.mXblIdBlockList = &startInternalRequest.getXblAccountIdBlockList();
        data.mMaxPlayerCapacity = startMMRequest.getGameCreationData().getMaxPlayerCapacity();
        data.mIsPseudoRequest = startMMRequest.getSessionData().getPseudoRequest();
        startInternalRequest.getMatchmakingFilters().getMatchmakingFilterCriteriaMap().copyInto(data.mFilterMap);

        const UserSessionsToExternalIpMap &userSessionsToExternalIpMap = startInternalRequest.getUserSessionIpInformation().getUserSessionsToExternalIpMap();
        if (data.mAllIpsResolved)
        {
            UserSessionsToExternalIpMap::const_iterator ipIter = userSessionsToExternalIpMap.find(getPrimaryUserSessionId());
            if (ipIter != userSessionsToExternalIpMap.end())
            {
                data.mHostSessionExternalIp = ipIter->second;
            }
            else
            {
                data.mAllIpsResolved = false;
                ERR_LOG("[MatchmakingSessionSlave(" << mReteProductionId << "/" << getOwnerBlazeId() << "/" 
                    << getOwnerPersonaName() << ")] - owning user session IP not present in StartMatchmakingInternalRequest.");
            }
        }


        QosCriteriaMap::const_iterator qosCriteriaIter = startInternalRequest.getQosCriteria().find(startMMRequest.getGameCreationData().getNetworkTopology());
        if ( qosCriteriaIter != startInternalRequest.getQosCriteria().end())
        {
            const ConnectionCriteria *connectionCriteria = qosCriteriaIter->second;
            // populate avoid lists, we don't care about tier or failed attempts on the search slave
            data.mQosGameIdAvoidList = &connectionCriteria->getAvoidGameIdList();
            data.mQosPlayerIdAvoidList = &connectionCriteria->getAvoidPlayerIdList();
        }
        

        if (startMMRequest.getSessionData().getSessionMode().getCreateGame() && (startMMRequest.getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION))
        {
            // check the topology
            if (isPlayerHostedTopology(data.mNetworkTopology))
            {
                TRACE_LOG("[MatchmakingSessionSlave] game entry type \"" << GameEntryTypeToString(startMMRequest.getPlayerJoinData().getGameEntryType()) 
                          << "\"; not compatible with network topology \"" << GameNetworkTopologyToString(data.mNetworkTopology ) << "\".");
                return SEARCH_ERR_INVALID_GAME_ENTRY_TYPE;

            }
        }

        for (GameManager::UserJoinInfoList::const_iterator itr = startInternalRequest.getUsersInfo().begin(), 
            end = startInternalRequest.getUsersInfo().end(); itr != end; ++itr)
        {
            // Update the NAT info for all the users:
            updateNatSupplementalData((**itr).getUser(), data, userSessionsToExternalIpMap);
        }

        data.mJoiningPlayerCount = startInternalRequest.getUsersInfo().size();
        buildTeamIdRoleSizeMap(data.mPlayerJoinData, data.mTeamIdRoleSpaceRequirements, (uint16_t)data.mJoiningPlayerCount, true);
        buildTeamIdPlayerRoleMap(data.mPlayerJoinData, data.mTeamIdPlayerRoleRequirements, (uint16_t)data.mJoiningPlayerCount, true);

        // supplemental data for PlayerListRule game membership checking
        data.mPlayerToGamesMapping = &mMatchmakingSlave.getPlayerGamesMapping();
        data.mXblAccountIdToGamesMapping = &mMatchmakingSlave.getXblAccountIdGamesMapping();
        data.mCachedTeamSelectionCriteria = &mCriteria.getTeamSelectionCriteriaFromRules();

        if (!mCriteria.initialize(startMMRequest.getCriteriaData(), data, criteriaError))
        {
            TRACE_LOG("[MatchmakingSessionSlave] Failed to initialize matchmaking criteria, " << criteriaError.getErrMessage() << ".");
            return SEARCH_ERR_INVALID_MATCHMAKING_CRITERIA;
        }

        // set privileged game preference into the new session to check when evaluating
        DedicatedServerOverrideMap::const_iterator privilegedIter = dedicatedServerOverrideMap.find(getOwnerBlazeId());
        if (privilegedIter != dedicatedServerOverrideMap.end())
        {
            INFO_LOG("[MatchmakingSessionSlave(" << getProductionId() << "/" << getOwnerBlazeId() 
                << "/" << getOwnerPersonaName() << ")] - setting mPrivilegedGameIdPreference to ("
                << privilegedIter->second << ").");
            mPrivilegedGameIdPreference = privilegedIter->second;
        }

        // Timer related initializations
        // An optional, configurable offset is added to the start time (this is used by Scenarios to allow for delays between
        // subsessions, which are all created at once when the Scenario starts).
        // Note: we can either base our start time off when we sent the request or when we're processing it (now).  We choose the
        // latter -  if we allowed the mmSlave to provide the start time in the matchmaking request, then yearly clock drift could
        // cause the matchmaking session to time out as soon as it arrived at the searchSlave [GOS-29368].

        // (See matchmakingsession.cpp for details on these:)
        TimeValue delay = startMMRequest.getSessionData().getStartDelay();
        TimeValue decay = startMMRequest.getSessionData().getStartingDecayAge();
        TimeValue duration = startMMRequest.getSessionData().getSessionDuration();

        mStartedDelayed = (delay.getMicroSeconds() != 0);
        mSessionStartTime = TimeValue::getTimeOfDay() + delay;
        mSessionTimeoutSeconds = (uint32_t)(duration + decay).getSec();

        if ((uint32_t)(decay.getSec()) <= mSessionTimeoutSeconds)
        {
            mStartingDecayAge = (uint32_t)(decay.getSec());
        }
        else
        {
            mStartingDecayAge = mSessionTimeoutSeconds;
        }

        mSessionAgeSeconds = mStartingDecayAge;

        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Helper to update MatchmakingSupplementalData NAT related fields
        \param[in] userSession - a user of the matchmaking session
        \param[out] data - the supplemental data containing NAT related data to update, based on the user session
    ***************************************************************************************************/
    void MatchmakingSessionSlave::updateNatSupplementalData(const UserSessionInfo& userSessionInfo, MatchmakingSupplementalData& data, const UserSessionsToExternalIpMap &userSessionsToExternalIpMap)
    {
        if (!PlayerInfo::getSessionExists(userSessionInfo))
            return;

        uint16_t poorNatCount = (data.mNetworkQosData.getNatType() >= Util::NAT_TYPE_STRICT_SEQUENTIAL) ? 1 : 0;
        
        Util::NatType userSessionNatType = userSessionInfo.getQosData().getNatType();
        if ( userSessionNatType >= Util::NAT_TYPE_STRICT_SEQUENTIAL )
        {
            bool behindSameFirewall = false;
            if (data.mAllIpsResolved)
            {
                // see if we're behind the same firewall as the host session
                UserSessionsToExternalIpMap::const_iterator ipIter = userSessionsToExternalIpMap.find(userSessionInfo.getSessionId());
                if (ipIter != userSessionsToExternalIpMap.end())
                {
                    if ( data.mHostSessionExternalIp == ipIter->second)
                    {
                        behindSameFirewall = true;
                    }
                }
                else
                {
                    data.mAllIpsResolved = false;
                    ERR_LOG("[MatchmakingSession(" << mReteProductionId << "/" << getOwnerBlazeId() << "/" 
                        << getOwnerPersonaName() << ")] - owning user session IP not present in StartMatchmakingInternalRequest.");
                }

            }

            if (!behindSameFirewall)
            {
                ++poorNatCount;
            }
        }

        if ( userSessionNatType > data.mNetworkQosData.getNatType())
        {
            data.mNetworkQosData.setNatType(userSessionNatType);
            TRACE_LOG("[MatchmakingSessionSlave(" << getProductionId() << "/" << getOwnerBlazeId() 
                << "/" << getOwnerPersonaName() << ")] matchmaking criteria session NAT type changed to: " 
                << (Util::NatTypeToString(userSessionNatType)));
        }   
        
        if (poorNatCount > 1)
        {
            // we don't really do anything with this in FG... need to change eval of viability rule
            // but letting a PG with multiple strict NATs match and fail to connect in FG mode should only impact these PG members
            // and not cause problems with normal players like it would in CG mode.
            data.mHasMultipleStrictNats = true;
        }
    }


    /*! ************************************************************************************************/
    /*! \brief Handler from ProductionListener that tells us when a Production Token has been added. 
        I.E. we have a found a matching game.

        \param[in] token - the ProductionToken from rete, which gives us our matching game.
        \param[in] fitScore - the fitScore match we had with this token.
    ***************************************************************************************************/
    bool MatchmakingSessionSlave::onTokenAdded(const Rete::ProductionToken& token, Rete::ProductionInfo& info)
    {
        TimeValue startTime = TimeValue::getTimeOfDay();

        Search::GameSessionSearchSlave* game = mMatchmakingSlave.getSearchSlave().getGameSessionSlave(token.mId);
        if (EA_UNLIKELY(game == nullptr))
        {
            ERR_LOG("[MatchmakingSessionSlave] onTokenAdded Unable to find game(" << token.mId << ").");
            return true;
        }

        // find or create the intrusive node here
        GameManager::MmSessionGameIntrusiveNode *node = nullptr;
        bool hadMmSessionGameIntrusiveNode = getOrCreateGameIntrusiveNode(&node, *game);

        bool useDebug = getDebugCheckOnly() || IS_LOGGING_ENABLED(Logging::TRACE);

        // Additional fit score from rete rules that were not evaluated as part of RETE. (match any)
        FitScore additionalFitScore = mMatchmakingSlave.getSearchSlave().evaluateAdditionalFitScoreForGame(*game, info, node->mReteDebugResultMap, useDebug);
        FitScore totalReteFitScore = info.fitScore + additionalFitScore;

        // For speed, opt out early if we're not going to make the top N, and we've exceeded our 
        // maximum number of heaped games.
        FitScore worstFitScore = 0;
        if (!mTopMatchHeap.empty())
        {
            MmSessionGameIntrusiveNode &tailNode = static_cast<MmSessionGameIntrusiveNode &>(mTopMatchHeap.back());
            worstFitScore = tailNode.getTotalFitScore();
        }

        if ( (totalReteFitScore + mCriteria.getMaxPossibleNonReteFitScore() + mCriteria.getMaxPossiblePostReteFitScore() <= worstFitScore)  &&
            ( mGameIntrusiveMap.size() >= mMatchmakingSlave.getMatchmakingConfig().getServerConfig().getSlaveNumMatchedGameReferencesPerSession()) )
        {
            SPAM_LOG("[MatchmakingSessionSlave].onTokenAdded dropping match for game(" << token.mId << ") with reteFitScore(" << totalReteFitScore << "), heapSize("
                << mTopMatchHeapSize << "), current matches(" << mGameIntrusiveMap.size() << ")" );
            ++mOtaQuickOutCount;
            mOtaQuickOut += TimeValue::getTimeOfDay() - startTime;
            return false;
        }

        // evaluate RETE rules that didnt put in a fit score due to being range rules.
        FitScore postReteFitScore = getMatchmakingCriteria().evaluateGameReteFitScore(*game, node->mReteDebugResultMap, useDebug);

        // Check early out again now that we have rete fit score from range rules
        if ( (totalReteFitScore + postReteFitScore + mCriteria.getMaxPossibleNonReteFitScore() <= worstFitScore)  &&
            ( mGameIntrusiveMap.size() >= mMatchmakingSlave.getMatchmakingConfig().getServerConfig().getSlaveNumMatchedGameReferencesPerSession()) )
        {
            SPAM_LOG("[MatchmakingSessionSlave].onTokenAdded dropping match for game(" << token.mId << ") with reteFitScore(" << totalReteFitScore << "), heapSize("
                << mTopMatchHeapSize << "), current matches(" << mGameIntrusiveMap.size() << ")" );
            ++mOtaQuickOutCount;
            mOtaQuickOut += TimeValue::getTimeOfDay() - startTime;
            return false;
        }

        // Evaluate all non rete rules.
        FitScore nonReteRulefitscore = 0;
        if (EA_LIKELY(hasNonReteRules()))
        {
            node->mNonReteDebugResultMap.clear();
            nonReteRulefitscore = getMatchmakingCriteria().evaluateGame(*game, node->mNonReteDebugResultMap, useDebug);
        }
        FitScore totalFitScore = totalReteFitScore + postReteFitScore + nonReteRulefitscore;

        Search::GameSessionSearchSlave* gameSession = mMatchmakingSlave.getSearchSlave().getGameSessionSlave(token.mId);

        if (useDebug)
        {
            // print how we got this match.
            mMatchmakingSlave.getSearchSlave().printReteBackTrace(token, info, node->mReteDebugResultMap);
        }

        bool isGameJoinable = isGamePotentiallyJoinable(*game);
        if (!isGameJoinable)
        {
            TRACE_LOG("[MatchmakingSessionSlave] Game(" << game->getGameId() << ") is not potentially joinable for session(" << getProductionId() << "), keeping until dirty.");
        }


        // Check that we still match given non rete rules
        if (isFitScoreMatch(nonReteRulefitscore) && isGameJoinable)
        {
            TRACE_LOG("[MatchmakingResult Session '" << getProductionId() << "'] MATCHED Game '" 
                      << token.mId << ":" << (gameSession!=nullptr?gameSession->getGameName():"") << "' after '" << (startTime - mSessionStartTime).getSec() 
                      << "' seconds,  total fitScore '" << totalFitScore << "' / '" << mCriteria.getMaxPossibleFitScore() 
                      << "' (RETE fitScore '" << totalReteFitScore  << "' + PostRETE fitScore '" << postReteFitScore << "' + NonRETE fitScore '" << nonReteRulefitscore << "').");
         
            node->mOverallResult = MATCH;
            if (useDebug)
            {
                size_t numPlayersInMatch = game->getPlayerRoster()->getPlayers(PlayerRoster::ROSTER_PLAYERS).size();
                uint16_t playerCapacity = game->getTotalPlayerCapacity();
                float percentFull = ((float)numPlayersInMatch / (float)playerCapacity) * 100;

                ++mNumFindGameMatches;
                mTotalFitScore += totalFitScore;
                
                TRACE_LOG("[MatchmakingResult Session '" << getProductionId() << "'] Game '" << token.mId << ":" << (gameSession!=nullptr?gameSession->getGameName():"") 
                    << "' has '" << numPlayersInMatch  << "' of '" << playerCapacity << "' players (" << percentFull << "%), and is currently tracked match '" 
                    << mGameIntrusiveMap.size()+1 << "' of a possible '" << mMatchmakingSlave.getMatchmakingConfig().getServerConfig().getSlaveNumMatchedGameReferencesPerSession() 
                    << "'. '" << mNumFindGameMatches << "' total matches found with an average fitScore of '" << (mTotalFitScore/mNumFindGameMatches) << "'.");
            }

            // if not in heap add. updates fit score
            addGameToMatchHeap(*node, totalFitScore);
        }
        else
        {
            // We did not match the non-rete rules, this game will be put into the dirty games, to be evald, later
            TRACE_LOG("[MatchmakingSessionSlave].onTokenAdded - Session '" << getProductionId() << "' NO_MATCH Game '" 
                      << token.mId << "' after '" <<  (startTime - mSessionStartTime).getSec() 
                      << "' seconds, total fitscore '" << totalFitScore << "' / '" << mCriteria.getMaxPossibleFitScore() 
                      << "' (RETE fitScore '" << totalReteFitScore  << "' + PostRETE fitScore '" << postReteFitScore << "' + NonRETE fitScore '" << nonReteRulefitscore  << "').");

            node->mOverallResult = NO_MATCH;
        }

        // If all nonReteRules were removed, we wouldn't need this functionality.
        if (EA_LIKELY(hasNonReteRules()))
        {
            if (!hadMmSessionGameIntrusiveNode)
            {
                // If we add the node as a reference, we need to track it on the game as well.
                if (addMatchedGameToReferenceMap(*node))
                {
                    //add a reference to the game, regardless if it matches
                    node->mIsGameReferenced = true;
                    game->addReference(*node);
                }
                else
                {
                    // we did not add the node, delete it.
                    ++mOtaFullCount;
                    mOtaFull += TimeValue::getTimeOfDay() - startTime;
                    mMatchmakingSlave.deleteGameIntrusiveNode(node);
                    return true;
                }
            }
        }

        // Save off the pre rete fitscore used as a base fitscore when the game is updated.
        node->mNonReteFitScore = nonReteRulefitscore;
        node->mPostReteFitScore = postReteFitScore;
        node->mReteFitScore = totalReteFitScore;

        ++mOtaFullCount;
        mOtaFull += TimeValue::getTimeOfDay() - startTime;

        return true;
    }

    void MatchmakingSessionSlave::onTokenUpdated(const Rete::ProductionToken& token, Rete::ProductionInfo& info)
    {
        MMGameIntrusiveMap::iterator itr = mGameIntrusiveMap.find(token.mId);
        if (itr != mGameIntrusiveMap.end())
        {
            MmSessionGameIntrusiveNode &n = *(itr->second);

            GameManager::DebugRuleResultMap debugResultMap;
            // Note that only these rules can possibly be updated by the RETE tree currently.  So no reason 
            // to re-run addition fit score, or non rete rules.
            GameManager::FitScore postReteFitScore = mCriteria.evaluateGameReteFitScore(n.mGameSessionSlave, debugResultMap);
            n.mPostReteFitScore = postReteFitScore;

            TRACE_LOG("[MatchmakingResult Session '" << getProductionId() << "'] UPDATED Game '" 
                << token.mId << "' New fit postReteFit '" << postReteFitScore << "' and total fitScore '" << n.getTotalFitScore() << "'.");
        }
    }


    bool MatchmakingSessionSlave::addMatchedGameToReferenceMap(GameManager::MmSessionGameIntrusiveNode& node)
    {
        // We've exceeded our total number of matching games.
        if (mGameIntrusiveMap.size() >= mMatchmakingSlave.getMatchmakingConfig().getServerConfig().getSlaveNumMatchedGameReferencesPerSession())
        {
            // We were a top match, we want to keep this one.
            if (node.mIsCurrentlyMatched)
            {
                insertGameIntrusiveMap(node);
                // Note that reference size != productionMap size due to non-rete rules.
                // Only update the production match if needed.
                if (!mProductionMatchHeap.empty())
                {
                    // Remove something else, removes from gameIntrusiveMap as well.
                    MmSessionGameIntrusiveNode &oldNode = popFrontProductionMatchHeap();
                    destroyIntrusiveNode(oldNode);
                }
                // MM_TODO: how to decide on a node to delete if the production match map is empty?
                // This means we've gotten to a state where we've filled out our total references, but
                // all of them don't match due to non-rete rule.
                return true;
            }
            else
            {
                // We were not a best match, so just drop it.
                return false;
            }
        }
        else
        {
            // Still under the cap, just insert.
            insertGameIntrusiveMap(node);
            return true;
        }
    }


    /*! ************************************************************************************************/
    /*! \brief Handler from ProductionListener that tells us when a Production Token has been removed.
        I.E. a game we had previously matched is no longer a match.

        \param[in] token - the ProductionToken from rete, which gives us our non-matching game.
        \param[in] fitScore - the fitScore match we had with this token.
    ***************************************************************************************************/
    void MatchmakingSessionSlave::onTokenRemoved(const Rete::ProductionToken& token, Rete::ProductionInfo& info)
    {
        TRACE_LOG("[MatchmakingSessionSlave] onTokenRemoved - Session(" << getProductionId() << ") Token id(" << token.mId << "), fitscore(" 
                  << info.fitScore << ")");

        if (hasNonReteRules()) //MM_TODO: we will remove hasNonReteRules (daden) all together
        {
            MMGameIntrusiveMap::iterator itr = mGameIntrusiveMap.find(token.mId);
            if (itr != mGameIntrusiveMap.end())
            {
                MmSessionGameIntrusiveNode &n = *(itr->second);
                removeGameFromMatchHeap(n);
                destroyIntrusiveNode(n);
                ++mOtrCount;
            }
        }
    }

    // Returns true if a game override exists and matchmakign should not proceed.
    bool MatchmakingSessionSlave::checkForGameOverride()
    {
        // The fill overrides do not prevent normal matchmaking attempts, only the PrivilegedGameMatch does that. 
        attemptFillGameOverride();
        return attemptPrivilegedGameMatch();
    }

    bool MatchmakingSessionSlave::attemptFillGameOverride()
    {
        // If the game has a specific game override, skip it from the Fill checks. 
        if (hasPrivilegedGameIdPreference())
        {
            if (mMatchmakingSlave.getSearchSlave().getGameSessionSlave(mPrivilegedGameIdPreference))
            {
                return false;
            }
        }
        if (mMatchmakingSlave.getFillServersOverride().empty())
        {
            return false;
        }

        // Check the current population for all the current fill stuff? 
        // Lookup the games, run the criteria checks on them, then add them in.
        bool foundMatch = false;
        mCriteria.setRuleEvaluationMode(RULE_EVAL_MODE_LAST_THRESHOLD);
        DebugRuleResultMap debugResultMap;
        GameManager::GameIdList::const_iterator curFillGame = mMatchmakingSlave.getFillServersOverride().begin();
        GameManager::GameIdList::const_iterator endFillGame = mMatchmakingSlave.getFillServersOverride().end();
        for (; curFillGame != endFillGame; ++curFillGame)
        {
            Search::GameSessionSearchSlave* gameSessionSlave = mMatchmakingSlave.getSearchSlave().getGameSessionSlave(*curFillGame);
            if (gameSessionSlave != nullptr)
            {
                FitScore fitScore = mCriteria.evaluateGameAllRules(*gameSessionSlave, debugResultMap, false);
                if (isFitScoreMatch(fitScore))
                {
                    GameManager::MmSessionGameIntrusiveNode *node = mMatchmakingSlave.createGameInstrusiveNode(*gameSessionSlave, *this);
                    insertGameIntrusiveMap(*node);
                    insertTopMatchHeap(mTopMatchHeap.begin(), *node);
                    mMatchmakingSlave.addToMatchingSessions(*this);
                    foundMatch = true;
                }
            }
        }

        mCriteria.setRuleEvaluationMode(RULE_EVAL_MODE_NORMAL);
        return foundMatch;
    }

    /*! ************************************************************************************************/
    /*! \brief Attempts to add privileged game id to matches
        \return true if there was a privileged game id specified for an existing game
    ***************************************************************************************************/
    bool MatchmakingSessionSlave::attemptPrivilegedGameMatch()
    {
        if (!hasPrivilegedGameIdPreference())
        {
            return false;
        }

        INFO_LOG("[MatchmakingSessionSlave(" << getProductionId() << ")].attemptPrivilegedGameMatch - attempting to find privileged match game ("
            << mPrivilegedGameIdPreference << ").");

        Search::GameSessionSearchSlave* gameSessionSlave = mMatchmakingSlave.getSearchSlave().getGameSessionSlave(mPrivilegedGameIdPreference);
        if (gameSessionSlave == nullptr)
        {
            // The game is validated to exist by the slave command.  If this search slave doesn't have it, we still don't
            // want to initialize RETE so that this search slave doesn't provide a game match that is not our override
            // server.  So we still return true here.
            TRACE_LOG("[MatchmakingSessionSlave.attemptPrivilegedGameMatch] Unable to locate priviledged game(" << mPrivilegedGameIdPreference 
                      << ") on this search slave to join directly for matchmaking session(" << getProductionId() << ")");
            return true;
        }

        // create and insert intrusive node as top match. insert to GameIntrusiveMap for automatic destructor cleanup
        if (mGameIntrusiveMap.find(gameSessionSlave->getGameId()) == mGameIntrusiveMap.end())
        {
            GameManager::MmSessionGameIntrusiveNode *node = mMatchmakingSlave.createGameInstrusiveNode(*gameSessionSlave, *this);
            insertGameIntrusiveMap(*node);
            insertTopMatchHeap(mTopMatchHeap.begin(), *node);
            mMatchmakingSlave.addToMatchingSessions(*this);
        }
        return true;
    }

    bool MatchmakingSessionSlave::getOrCreateGameIntrusiveNode(GameManager::MmSessionGameIntrusiveNode **node, Search::GameSessionSearchSlave &gameSession)
    {
        MMGameIntrusiveMap::iterator iter = mGameIntrusiveMap.find(gameSession.getGameId());
        const bool hadMmSessionGameIntrusiveNode = iter != mGameIntrusiveMap.end(); //whether already added

        // create and insert intrusive node as top match. insert to GameIntrusiveMap for automatic destructor cleanup
        *node = nullptr;
        if (hadMmSessionGameIntrusiveNode)
        {
            *node = iter->second;
            return true;
        }
        else
        {
            // create and insert intrusive node as top match. insert to GameIntrusiveMap for automatic destructor cleanup
            *node = mMatchmakingSlave.createGameInstrusiveNode(gameSession, *this);
            return false;
        }
    }

    void MatchmakingSessionSlave::matchRequiredDebugGames()
    {
        const PreferredGamesRule *preferredGamesRule = mCriteria.getPreferredGamesRule();
        if (getDebugCheckOnly() && !hasMatchedDebugGames() && preferredGamesRule != nullptr)
        {
            // can only do this once per session.
            mHasMatchedDebugGames = true;

            PreferredGamesRule::GameIdSet::const_iterator iter = preferredGamesRule->getGameIdSet().begin();
            PreferredGamesRule::GameIdSet::const_iterator end = preferredGamesRule->getGameIdSet().end();
            for (; iter != end; ++iter)
            {
                GameId requiredGame = *iter;
                Search::GameSessionSearchSlave* gameSessionSlave = mMatchmakingSlave.getSearchSlave().getGameSessionSlave(requiredGame);
                if (gameSessionSlave == nullptr)
                {
                    // The game is validated to exist by the slave command.  If this search slave doesn't have it, we still don't
                    // want to initialize RETE so that this search slave doesn't provide a game match that is not our override
                    // server.  So we still return true here.
                    TRACE_LOG("[MatchmakingSessionSlave.matchRequiredDebugGames] Unable to locate required game(" << requiredGame 
                        << ") on this search slave to match for matchmaking session(" << getProductionId() << ")");
                    continue;
                }
                
                GameManager::MmSessionGameIntrusiveNode *node = nullptr;
                bool alreadyExisted = getOrCreateGameIntrusiveNode(&node, *gameSessionSlave);

                if (alreadyExisted)
                {
                    // If we have a node, we already matched this required game at least through RETE. 
                    // We could have potentially failed non-RETE rules, in that case add the game below.
                    if (isFitScoreMatch(node->mNonReteFitScore))
                    {
                        continue;
                    }
                    
                }

                node->mOverallResult = NO_MATCH;
                // Find the game in the RETE tree through the production build infos.
                // These are the locations were the game stalled out in matching through the RETE tree.
                bool recurse = true;
                const Rete::ProductionBuildInfo* firstPbi = nullptr;
                Rete::ProductionBuildInfoList::const_iterator it = getProductionBuildInfoList().begin();
                Rete::ProductionBuildInfoList::const_iterator itEnd = getProductionBuildInfoList().end();
                for (; it != itEnd; ++it)
                {
                    const Rete::ProductionBuildInfo &info = *it;
                    // only print out the furthest match we got into the RETE tree.
                    if (info.getParent().getProductionTest().getName() == mTopCondition.getName())
                    {
                        if (firstPbi == nullptr && info.getProductionInfo().conditionBlockId != Rete::CONDITION_BLOCK_DEDICATED_SERVER_SEARCH)
                        {
                            firstPbi = &info;
                        }
                        // we only recurse the first node.  Other nodes at this level should have the same parent
                        bool found = mMatchmakingSlave.getSearchSlave().printReteBackTrace(info, gameSessionSlave->getGameId(), this, node->mReteDebugResultMap, recurse);
                        if (found)
                        {
                            recurse = false;
                        }
                        
                    }
                }
                // If recurse is still true, we never found anything, scan the tree.
                if (recurse)
                {
                    const Rete::JoinNode* joinNode = mMatchmakingSlave.getSearchSlave().scanReteForToken(*firstPbi, gameSessionSlave->getGameId());
                    if (joinNode != nullptr)
                    {
                        // This is the lowest matching joinNode for this game, its children's conditions were not met.
                        bool recurseChild = true;
                        Rete::JoinNode::JoinNodeMap::const_iterator itr = joinNode->getChildren().begin();
                        Rete::JoinNode::JoinNodeMap::const_iterator itrEnd = joinNode->getChildren().end();
                        for (; itr != itrEnd; ++itr)
                        {
                            const Rete::JoinNode* childNode = itr->second;
                            mMatchmakingSlave.getSearchSlave().printReteJoinNodeBackTrace(*childNode, gameSessionSlave->getGameId(), this, node->mReteDebugResultMap, recurseChild);
                            if (recurseChild)
                            {
                                recurseChild = false;
                            }
                        }
                    }
                }

                TRACE_LOG("[MatchmakingSessionSlave.matchRequiredDebugGames] Force adding DEBUG NO_MATCH game(" << requiredGame << ") to match heap.")

                // force it to the top of the heap.
                addGameToMatchHeap(*node, UINT32_MAX);
                // don't call addMatchedGameToReferenceMap to avoid cap on number of games preventing this game from going down.
                insertGameIntrusiveMap(*node);
            }
        }
    }


    /*! ************************************************************************************************/
    /*! \brief Adds game to session's matches
    ***************************************************************************************************/
    void MatchmakingSessionSlave::addGameToMatchHeap(const Search::GameSessionSearchSlave& game, FitScore totalFitScore)
    {
        // first find our intrusive node in our full list of games.
        MMGameIntrusiveMap::iterator iter = mGameIntrusiveMap.find(game.getGameId());
        if (iter == mGameIntrusiveMap.end())
        {
            //pre: we already have the intrusive node created in the mGameIntrusiveMap.
            ERR_LOG("[MatchmakingSessionSlave].addMatchingGameToProductionMatchMap session '" << getProductionId() 
                    << "' missing game '" << game.getGameId() << "' from mGameIntrusiveMap");
            return;
        }

        MmSessionGameIntrusiveNode &curNode = *(iter->second);

        addGameToMatchHeap(curNode, totalFitScore);
    }


    void MatchmakingSessionSlave::addGameToMatchHeap(GameManager::MmSessionGameIntrusiveNode& curNode, FitScore totalFitScore)
    {
        // Fit score update from decaying.  In theory fitscore should remain constant, but we get
        // a separate onTokenAdded.
        if (curNode.mIsCurrentlyMatched || curNode.mIsCurrentlyHeaped)
        {
            TRACE_LOG("[MatchmakingSessionSlave] Session(" << getProductionId() << ") already matched game(" << curNode.getGameId() 
                      << ") with fitScore(" << curNode.getTotalFitScore() << "), received new fitScore(" << totalFitScore << "). Dropping fit score update.");
            return;
        }

        // first match, blindly insert to front
        if (mTopMatchHeap.empty())
        {
            pushBackTopMatchHeap(curNode);
             // Update the slave that our session has a match
            mMatchmakingSlave.addToMatchingSessions(*this);
        }
        else
        {
            MmSessionGameIntrusiveNode &tailNode = static_cast<MmSessionGameIntrusiveNode &>(mTopMatchHeap.back());
            if (totalFitScore > tailNode.getTotalFitScore() && !curNode.mIsCurrentlyMatched)
            {
                // Go over our top match heap and add our node into the top N sorted.
                // We already know the list is not empty.
                ProductionTopMatchHeap::iterator topIter = mTopMatchHeap.begin();
                ProductionTopMatchHeap::iterator end = mTopMatchHeap.end();

                for (; topIter != end; ++topIter)
                {
                    MmSessionGameIntrusiveNode &topNode = static_cast<MmSessionGameIntrusiveNode &>(*topIter);

                    if (totalFitScore > topNode.getTotalFitScore())
                    {
                        insertTopMatchHeap(topIter, curNode);
                        break;
                    }
                }
            }
            // We didn't beat our lowest node, but there may still be room in the top match heap.
            // if so, add ourselves here.
            if (!curNode.mIsCurrentlyMatched && mTopMatchHeapSize <  mMatchmakingSlave.getMatchmakingConfig().getServerConfig().getSlaveNumGamesPerSessionRequestToMaster())
            {
                pushBackTopMatchHeap(curNode);
            }
        }

        // we didn't make the top N list, throw into unsorted heap
        if (!curNode.mIsCurrentlyMatched && !curNode.mIsCurrentlyHeaped)
        {
            pushBackProductionMatchHeap(curNode);
        }
        else 
        {
            // We've exceeded our current maximum size of sorted values
            if (mTopMatchHeapSize > mMatchmakingSlave.getMatchmakingConfig().getServerConfig().getSlaveNumGamesPerSessionRequestToMaster())
            {
                MmSessionGameIntrusiveNode &oldNode = popBackTopMatchHeap();
                pushBackProductionMatchHeap(oldNode);
            }
        }
    }

    void MatchmakingSessionSlave::removeGameFromMatchHeap(GameId gameId)
    {
        // Update the slave with a new best match if our best has been removed.
        Blaze::GameManager::MMGameIntrusiveMap::iterator itr = mGameIntrusiveMap.find(gameId);

        if (itr == mGameIntrusiveMap.end())
        {
            ERR_LOG("[MatchmakingSessionSlave].removeGameFromMatchHeap session '" << getProductionId() << "' missing game '" 
                    << gameId << "' from mGameIntrusiveMap");
            return;
        }
        MmSessionGameIntrusiveNode &node = *(itr->second);
        removeGameFromMatchHeap(node);
    }

    /*! ************************************************************************************************/
    /*! \brief Removes game from session's matches
    ***************************************************************************************************/
    void MatchmakingSessionSlave::removeGameFromMatchHeap(GameManager::MmSessionGameIntrusiveNode& curNode)
    {
        if (curNode.mIsCurrentlyHeaped)
        {
            // only need remove from unsorted heap
            removeProductionMatchHeap(curNode);
            return;
        }

        if (curNode.mIsCurrentlyMatched)
        {
            // Remove the wme/fit match from our known top matches
            removeTopMatchHeap(curNode);

            if (mTopMatchHeapSize < mMatchmakingSlave.getMatchmakingConfig().getServerConfig().getSlaveNumGamesPerSessionRequestToMaster() && !mProductionMatchHeap.empty())
            {
                // We no longer sort the list of Top Matched games, we only pull
                // randomly from our unsorted heap. (for now, FIFO)
                MmSessionGameIntrusiveNode &newNode = popFrontProductionMatchHeap();
                pushBackTopMatchHeap(newNode);
            }

            // This was our last match.  Tell the slave, we no longer have any matches
            // to finalize.
            if (mTopMatchHeap.empty())
            {
                mMatchmakingSlave.removeFromMatchingSessions(*this);
            }
        }
    }


    /*! ************************************************************************************************/
    /*! \brief Peeks at our list of best games to determine if we need to add more from
        the unsorted map.
    ***************************************************************************************************/
    void MatchmakingSessionSlave::popBestGames()
    {
        // A session clears lock down when it has no more games in its immediate list of matches
        ++mNumFailedFoundGameRequests;
        ProductionTopMatchHeap::iterator iter = mTopMatchHeap.begin();
        ProductionTopMatchHeap::iterator end = mTopMatchHeap.end();

        uint32_t gameCount = 0;
        while (iter != end && gameCount < mMatchmakingSlave.getMatchmakingConfig().getServerConfig().getSlaveNumGamesPerSessionRequestToMaster())
        {
            MmSessionGameIntrusiveNode &n = static_cast<MmSessionGameIntrusiveNode &>(*iter);

            ++iter;

            // Remove any reference to the game and our match there in
            // if we fail to join this match, we continue on with other matches.
            removeGameFromMatchHeap(n);
            destroyIntrusiveNode(n);

            if (!mProductionMatchHeap.empty())
            {
                // We no longer sort the list of Top Matched games, we only pull
                // randomly from our unsorted heap. (for now, FIFO)
                MmSessionGameIntrusiveNode &newNode = popFrontProductionMatchHeap();
                pushBackTopMatchHeap(newNode);   
            }
            
            ++gameCount;
        }

    }


    void MatchmakingSessionSlave::peekBestGames(MatchmakingFoundGames& foundGames)
    {
        if (mTopMatchHeap.empty())
        {
            return;
        }

        ProductionTopMatchHeap::iterator iter = mTopMatchHeap.begin();
        ProductionTopMatchHeap::iterator end = mTopMatchHeap.end();

        foundGames.setMatchmakingSessionId(getProductionId());
        foundGames.setMatchmakingScenarioId(getScenarioId());
        foundGames.setOwnerUserSessionId(getOwnerUserSessionId());
        getMatchmakingCriteria().getTeamSelectionCriteriaFromRules().copyInto(foundGames.getFoundTeam());

        // We loop over game matches in the top match heap, capped by the configurable number of
        // top matched games.
        uint32_t gameCount = 0;
        while (iter != end && gameCount < mMatchmakingSlave.getMatchmakingConfig().getServerConfig().getSlaveNumGamesPerSessionRequestToMaster())
        {
            MmSessionGameIntrusiveNode &n = static_cast<MmSessionGameIntrusiveNode &>(*iter);

            TRACE_LOG("[MatchmakingSessionSlave] Session(" << getProductionId() << ") adding matched game(" << n.getGameId() << ") with fitScore(" 
                      << n.getTotalFitScore() << ") for finalization.");

            if (n.mGameSessionSlave.isResetable())
            {
                // skip, find game mode does not eval resettable games
                WARN_LOG("[MatchmakingSessionSlave] Session(" << getProductionId() << ") tried to match resettable game(" << n.getGameId() << ") with fitScore(" 
                    << n.getTotalFitScore() << ") for finalization.");
                ++iter;
                continue;
            }
            else
            {
                bool firstGame = foundGames.getMatchedGameList().empty();
                MatchedGame *matchedGame = foundGames.getMatchedGameList().pull_back();
                auto* playerRoster = n.mGameSessionSlave.getPlayerRoster();
                auto gameId = n.getGameId();
                matchedGame->setGameId(gameId);
                matchedGame->setRecordVersion(mMatchmakingSlave.getSearchSlave().getGameRecordVersion(gameId));
                matchedGame->setGameName(n.mGameSessionSlave.getGameName());
                matchedGame->setParticipantCount(playerRoster->getParticipantCount());
                matchedGame->setParticipantCapacity(n.mGameSessionSlave.getTotalParticipantCapacity());
                matchedGame->setFitScore(n.getTotalFitScore());
                matchedGame->setOverallResult(n.mOverallResult);
                matchedGame->setMaxFitScore(mCriteria.calcMaxPossibleFitScore());
                matchedGame->setTimeToMatch(getDuration());

                // PACKER_TODO: validate that this is a packer mm session, legacy sessions don't need this data
                {
                    // PACKER_TODO: We don't want to copy the whole ReplicatedPlayerDataServer object because its way too heavyweight, instead we need to pick out the relevant fields and only send those and only if this is a packer-based matchmaking request...

                    // we use ROSTER_PARTICIPANTS, not ROSTER_PLAYERS because spectators don't have a team
                    auto roster = playerRoster->getPlayers(GameManager::PlayerRoster::ROSTER_PARTICIPANTS);
                    for (auto& member : roster)
                    {
                        auto* replPlayerData = member->getPlayerData();
                        auto memberInfo = matchedGame->getMemberInfoList().pull_back();
                        memberInfo->setTeamIndex(replPlayerData->getTeamIndex());
                        memberInfo->setUserGroupId(replPlayerData->getUserGroupId());
                        matchedGame->getUserInfoList().push_back(&replPlayerData->getUserInfo()); // matchedGame holds a ref to the inernal member, this is safe because its immediately used to send a notification and is removed afterwards
                    }
                }

                if (!n.mReteDebugResultMap.empty())
                {
                    n.mReteDebugResultMap.copyInto(matchedGame->getDebugResultMap());
                }

                if (!n.mNonReteDebugResultMap.empty())
                {
                    // have to splice the conditions from the non-RETE rules into the result map
                    DebugRuleResultMap::const_iterator nonReteConditionsIter = n.mNonReteDebugResultMap.begin();
                    DebugRuleResultMap::const_iterator nonReteConditionsEnd = n.mNonReteDebugResultMap.end();
                    for (; nonReteConditionsIter != nonReteConditionsEnd; ++nonReteConditionsIter)
                    {
                        const DebugRuleResult *nonReteResult = nonReteConditionsIter->second;
                        DebugRuleResultMap::iterator it = matchedGame->getDebugResultMap().find(nonReteConditionsIter->first);

                        if (it != matchedGame->getDebugResultMap().end())
                        {
                            // add our conditions and fitScore to an existing result for the rule.
                            // Can happen when rete rules specify multiple and conditions, or a rule uses rete and non rete together.
                            DebugRuleResult *existingResult = it->second;
                            
                            DebugEvaluationResult result = ((existingResult->getResult() == MATCH) && (nonReteResult->getResult() == MATCH)) ? MATCH : NO_MATCH;
                            existingResult->setResult(result);
                            if (result == NO_MATCH)
                            {
                                existingResult->setFitScore(0);
                            }
                            else
                            {
                                existingResult->setFitScore(existingResult->getFitScore() + nonReteResult->getFitScore());
                            }

                            existingResult->getConditions().insert(existingResult->getConditions().end(), nonReteResult->getConditions().begin(), nonReteResult->getConditions().end());
                        }
                        else
                        {
                            DebugRuleResult *debugResult = matchedGame->getDebugResultMap().allocate_element();
                            nonReteResult->copyInto(*debugResult);
                            matchedGame->getDebugResultMap().insert(eastl::make_pair(nonReteConditionsIter->first, debugResult));
                        }
                    }
                }

                if (firstGame)
                {
                    bool performQosCheck = mMatchmakingSlave.getSearchSlave().shouldPerformQosCheck(n.mGameSessionSlave.getGameNetworkTopology());
                    // Single player session check + indirect matchmaking check:
                    if (isSinglePlayerSession() && !isIndirectMatchmakingRequest())
                    {
                        // We alone are joining the game.
                        foundGames.getGameRequest().switchActiveMember(GameRequest::MEMBER_JOINGAMEREQUEST);
                        initJoinGameMasterRequest(*(foundGames.getGameRequest().getJoinGameRequest()));

                        foundGames.getGameRequest().getJoinGameRequest()->getUserJoinInfo().setPerformQosValidation(performQosCheck);
                    }
                    else
                    {
                        // I've brought a game group
                        foundGames.getGameRequest().switchActiveMember(GameRequest::MEMBER_JOINGAMEBYGROUPREQUEST);
                        initJoinGameByGroupRequest(*(foundGames.getGameRequest().getJoinGameByGroupRequest()));

                        // Set all non-optional group members to perform QoS Validation:
                        UserJoinInfoList::const_iterator ujiItr = foundGames.getGameRequest().getJoinGameByGroupRequest()->getUsersInfo().begin();
                        UserJoinInfoList::const_iterator ujiEnd = foundGames.getGameRequest().getJoinGameByGroupRequest()->getUsersInfo().end();
                        for (; ujiItr != ujiEnd; ++ujiItr)
                        {
                            // All non-optional members of the group join should have this set the same:
                            if (!(*ujiItr)->getIsOptional())
                            {
                                (*ujiItr)->setPerformQosValidation(performQosCheck);
                            }
                        }
                    }
                }
            }

            ++gameCount;
            ++iter;
        }
    }

    void MatchmakingSessionSlave::updateSessionAge(const TimeValue &now)
    {
        if (now < mSessionStartTime)
        {
            return;
        }

        mSessionAgeSeconds = static_cast<uint32_t>( (now - mSessionStartTime).getSec() ) + mStartingDecayAge;
    }

     /*! ************************************************************************************************/
    /*! \brief Updates this sessions age, triggering decay

        \param[in] now - The start time of the idle that is triggering the update
        \return true if the session has now changed its criteria rules, false otherwise.
    ***************************************************************************************************/
    UpdateThresholdStatus MatchmakingSessionSlave::updateFindGameSession()
    {
        UpdateThresholdStatus updateStatus;
        updateStatus.reteRulesResult = NO_RULE_CHANGE;
        updateStatus.nonReteRulesResult = NO_RULE_CHANGE;

        // Early out if we don't need to decay yet.
        if ((mNextElapsedSecondsUpdate != UINT32_MAX) && (mSessionAgeSeconds < mNextElapsedSecondsUpdate))
        {
            TRACE_LOG("[MatchmakingSessionSlave] Session(" << mReteProductionId << ") at age " << mSessionAgeSeconds << " will not decay again until " << mNextElapsedSecondsUpdate << " seconds.");
            return updateStatus;
        }


        if (!mStartMatchmakingInternalRequest.getRequest().getSessionData().getDebugFreezeDecay())
        {
            // Update the rule criteria showing our decay age
            updateStatus = mCriteria.updateCachedThresholds(mSessionAgeSeconds, false);
            mNextElapsedSecondsUpdate = mCriteria.getNextElapsedSecondsUpdate();

            // Next update is when the session expires.
            if (mNextElapsedSecondsUpdate == UINT32_MAX)
            {
                mNextElapsedSecondsUpdate = mSessionTimeoutSeconds;
            }
        }

        TRACE_LOG("[MatchmakingSessionSlave] Session(" << mReteProductionId << ") at age " << mSessionAgeSeconds 
            << " decayed thresholds, will decay again in " << mNextElapsedSecondsUpdate 
            << " seconds, will expire in " << (mSessionTimeoutSeconds - mSessionAgeSeconds) << "");

        return updateStatus;
    }


    void MatchmakingSessionSlave::updateNonReteRules(const Search::GameSessionSearchSlave& game)
    {
        FitScore startingFitScore = 0;
        bool useDebug = getDebugCheckOnly() || IS_LOGGING_ENABLED(Logging::TRACE);
        
        MMGameIntrusiveMap::iterator itr = mGameIntrusiveMap.find(game.getGameId());
        if (itr == mGameIntrusiveMap.end())
        {
            if (!mGameIntrusiveMap.empty()) 
            {
                ERR_LOG("[MatchmakingSessionSlave] updateNonReteRules Unable to find game(" << game.getGameId() << ") in mGameIntrusiveMap.");
            }
            return;
        }

        MmSessionGameIntrusiveNode &n = *(itr->second);
        n.mNonReteDebugResultMap.clear();

        FitScore nonReteFitScore = getMatchmakingCriteria().evaluateGame(game, n.mNonReteDebugResultMap, useDebug);
        startingFitScore = n.mPostReteFitScore;
        n.mNonReteFitScore = nonReteFitScore;      // Update the nonRete score with the latest results

        bool isGameJoinable = isGamePotentiallyJoinable(game);
        if (!isGameJoinable)
        {
            TRACE_LOG("[MatchmakingSessionSlave] Game(" << game.getGameId() << ") is not potentially joinable for session(" << getProductionId() << "), keeping until dirty.");
        }

        FitScore totalFitScore = nonReteFitScore + startingFitScore;
        if (isFitScoreMatch(nonReteFitScore) && isGameJoinable)
        {
            TRACE_LOG("[MatchmakingSessionSlave].updateNonReteRules session '" << getProductionId() << "' MATCH for game '" 
                      << game.getGameId() << "' after '" << mSessionAgeSeconds << "' seconds updated with startingFitScore '" << startingFitScore << "' + nonReteFitScore '" 
                      << nonReteFitScore << "' = totalFitScore '" << totalFitScore << "'");
            n.mOverallResult = MATCH;
            addGameToMatchHeap(game, totalFitScore);
        }
        else if (!isFitScoreMatch(nonReteFitScore))
        {
            TRACE_LOG("[MatchmakingSessionSlave].updateNonReteRules session '" << getProductionId() << "' NO_MATCH for game '" 
                      << game.getGameId() << "' after '" << mSessionAgeSeconds << "' seconds updated with startingFitScore '" << startingFitScore << "' + nonReteFitScore '" << nonReteFitScore 
                      << "' = totalFitScore '" << totalFitScore << ".");

            n.mOverallResult = NO_MATCH;
            removeGameFromMatchHeap(game.getGameId());
            // Note that we don't destroy the node.  We need the reference active to possibly
            // re-evaluate the non-rete rules again if the game becomes dirty.
        }
    }


    /*! ************************************************************************************************/
    /*! \brief check if it's time for another async status notification

        \param[in] statusNotificationPeriodMs - status notification period from config file
        \return true if time is due, otherwise false
    ***************************************************************************************************/
    bool MatchmakingSessionSlave::isTimeForAsyncNotifiation(const Blaze::TimeValue &now, const uint32_t statusNotificationPeriodMs)
    {
        if ((now - mLastStatusAsyncTimestamp).getMillis() >= statusNotificationPeriodMs)
        {
            mLastStatusAsyncTimestamp = now;
            return true;
        }

        return false;
    }

    void MatchmakingSessionSlave::submitMatchmakingFailedEvent(MatchmakingResult matchmakingResult) const
    {
        // submit matchmaking succeeded event, this will catch the game creator and all joining sessions.
        FailedMatchmakingSession failedMatchmakingSession;
        failedMatchmakingSession.setMatchmakingSessionId(mReteProductionId);
        failedMatchmakingSession.setMatchmakingScenarioId(mOriginatingScenarioId);      // ?
        failedMatchmakingSession.setUserSessionId(getOwnerUserSessionId());
        failedMatchmakingSession.setMatchmakingResult(MatchmakingResultToString(matchmakingResult));

        gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_FAILEDMATCHMAKINGSESSIONEVENT), failedMatchmakingSession);

    }

    

    /*! ************************************************************************************************/
    /*! \brief Initializes a joinGameRequest with the proper values for this matchmaking session.

        \param[in/out] joinRequest - the joinGameRequest we are going to initialize
    ***************************************************************************************************/
    void MatchmakingSessionSlave::initJoinGameRequest(JoinGameRequest &joinRequest) const
    {
        // Note that we do not set the game id in the join request as we use one request for
        // multiple game id's.  The game id will be added to the join request on the master.
        // We also don't set the joining team index as it can be different for each game.
        // This too will be set by the master.
        // The values are stored in the outer found game request to the master.
        getStartMatchmakingRequest().getPlayerJoinData().copyInto(joinRequest.getPlayerJoinData());

        joinRequest.setJoinMethod( JOIN_BY_MATCHMAKING );
        joinRequest.getPlayerJoinData().setDefaultSlotType(SLOT_PUBLIC_PARTICIPANT);
        joinRequest.getCommonGameData().setGameProtocolVersionString(getStartMatchmakingRequest().getCommonGameData().getGameProtocolVersionString());
    }

    void MatchmakingSessionSlave::initJoinGameMasterRequest(JoinGameMasterRequest &joinRequest) const
    {
        initJoinGameRequest(joinRequest.getJoinRequest());
        joinRequest.setUserJoinInfo(*mStartMatchmakingInternalRequest.getUsersInfo().front());
        joinRequest.setExternalSessionData(const_cast<GameManager::ExternalSessionMasterRequestData&>(mStartMatchmakingInternalRequest.getGameExternalSessionData()));
    }

    /*! ************************************************************************************************/
    /*! \brief Initializes a JoinGameByGroupMasterRequest with the proper values for this matchmaking session.

        \param[in/out] joinRequest - the JoinGameByGroupMasterRequest we are going to initialize
    ***************************************************************************************************/
    void MatchmakingSessionSlave::initJoinGameByGroupRequest(JoinGameByGroupMasterRequest &joinRequest) const
    {
        initJoinGameRequest(joinRequest.getJoinRequest());

        // If this was an indirect matchmaking attempt, everyone will use the indirect join game flow:
        if (isIndirectMatchmakingRequest())
            joinRequest.setJoinLeader(false);

        joinRequest.getJoinRequest().getPlayerJoinData().setGroupId(getStartMatchmakingRequest().getPlayerJoinData().getGroupId());
        joinRequest.setUsersInfo(const_cast<GameManager::UserJoinInfoList&>(mStartMatchmakingInternalRequest.getUsersInfo()));
        joinRequest.setExternalSessionData(const_cast<GameManager::ExternalSessionMasterRequestData&>(mStartMatchmakingInternalRequest.getGameExternalSessionData()));
    }

    void MatchmakingSessionSlave::reevaluateNonReteRules()
    {
        MMGameIntrusiveMap::const_iterator allMatchIter = mGameIntrusiveMap.begin();
        MMGameIntrusiveMap::const_iterator allMatchEnd = mGameIntrusiveMap.end();

        for(; allMatchIter != allMatchEnd; ++allMatchIter)
        {
            const MmSessionGameIntrusiveNode &n = *(allMatchIter->second);
            const Search::GameSessionSearchSlave *game = mMatchmakingSlave.getSearchSlave().getGameSessionSlave(n.getGameId());
            if (game != nullptr)
            {
                updateNonReteRules(*game);
            }

        }
    }

    /*! ************************************************************************************************/
    /*! \brief Checks to see if the provided game session is potentially joinable by this session.

        \param[in] fitScore - the game to check
        \return true if the game is potentially joinable
    ***************************************************************************************************/
    inline bool MatchmakingSessionSlave::isGamePotentiallyJoinable(const Search::GameSessionSearchSlave &game) const
    {
        // we don't evaluate resettable servers in Find game matchmaking
        if (game.isResetable())
        {
            return false;
        }

        // join in progress not allowed
        if ((game.getGameState() == IN_GAME) && !game.getGameSettings().getJoinInProgressSupported())
        {
            return false;
        }

        // Check the game entry criteria & role entry criteria for all users:
        if (!isGamePotentiallyJoinableForUserSessions(game, mStartMatchmakingInternalRequest.getUsersInfo()))
        {
            return false;
        }

        return true;        
    }

    bool MatchmakingSessionSlave::isGamePotentiallyJoinableForUserSessions(const Search::GameSessionSearchSlave &game, const UserJoinInfoList& sessionInfos) const
    {
        // Check the game entry criteria & role entry criteria:
        for (GameManager::UserJoinInfoList::const_iterator itr = sessionInfos.begin(),
            end = sessionInfos.end(); itr != end; ++itr)
        {
            const char8_t* roleName = lookupPlayerRoleName(getStartMatchmakingRequest().getPlayerJoinData(), (*itr)->getUser().getUserInfo().getId());
            if (!game.isJoinableForUser((**itr).getUser(), false, roleName, true))
                return false;
        }

        return true;
    }

    void MatchmakingSessionSlave::destroyIntrusiveNode(Blaze::GameManager::MmSessionGameIntrusiveNode& node)
    {
        eraseGameIntrusiveMap(node);

        mMatchmakingSlave.deleteGameIntrusiveNode(&node);
    }

    void MatchmakingSessionSlave::insertTopMatchHeap(const ProductionTopMatchHeap::iterator& loc, MmSessionGameIntrusiveNode &node)
    {
        EA_ASSERT(!node.mIsCurrentlyHeaped);
        EA_ASSERT(!node.mIsCurrentlyMatched);

        mTopMatchHeap.insert(loc, node);
        ++mTopMatchHeapSize;
        node.mIsCurrentlyMatched = true;
        node.mIsToBeSentToMatchmakerSlave = true;
    }

    void MatchmakingSessionSlave::pushBackTopMatchHeap(MmSessionGameIntrusiveNode &node)
    {
        EA_ASSERT(!node.mIsCurrentlyHeaped);
        EA_ASSERT(!node.mIsCurrentlyMatched);

        mTopMatchHeap.push_back(node);
        ++mTopMatchHeapSize;
        node.mIsCurrentlyMatched = true;
        node.mIsToBeSentToMatchmakerSlave = true;
    }

    MmSessionGameIntrusiveNode& MatchmakingSessionSlave::popBackTopMatchHeap()
    {
        EA_ASSERT(!mTopMatchHeap.empty());

        MmSessionGameIntrusiveNode& node = static_cast<MmSessionGameIntrusiveNode &>(mTopMatchHeap.back());

        mTopMatchHeap.pop_back();
        --mTopMatchHeapSize;
        node.mIsCurrentlyMatched = false;
        node.mIsToBeSentToMatchmakerSlave = false;
        mRemovedGameList.push_back(node.getGameId());

        return node;
    }

    void MatchmakingSessionSlave::removeTopMatchHeap(MmSessionGameIntrusiveNode &node)
    {
        EA_ASSERT(node.mIsCurrentlyMatched);

        ProductionTopMatchHeap::remove(node);
        --mTopMatchHeapSize;
        node.mIsCurrentlyMatched = false;

        node.mIsToBeSentToMatchmakerSlave = false;
        mRemovedGameList.push_back(node.getGameId());
    }


    void MatchmakingSessionSlave::pushBackProductionMatchHeap(MmSessionGameIntrusiveNode &node)
    {
        EA_ASSERT(!node.mIsCurrentlyHeaped);
        EA_ASSERT(!node.mIsCurrentlyMatched);

        mProductionMatchHeap.push_back(node);
        node.mIsCurrentlyHeaped = true;
    }

    MmSessionGameIntrusiveNode& MatchmakingSessionSlave::popFrontProductionMatchHeap()
    {
        EA_ASSERT(!mProductionMatchHeap.empty());

        MmSessionGameIntrusiveNode& node = static_cast<MmSessionGameIntrusiveNode &>(mProductionMatchHeap.front());

        mProductionMatchHeap.pop_front();
        node.mIsCurrentlyHeaped = false;

        return node;
    }

    void MatchmakingSessionSlave::removeProductionMatchHeap(MmSessionGameIntrusiveNode &node) const
    {
        EA_ASSERT(node.mIsCurrentlyHeaped);

        ProductionMatchHeap::remove(node);
        node.mIsCurrentlyHeaped = false;
    }

    void MatchmakingSessionSlave::insertGameIntrusiveMap(GameManager::MmSessionGameIntrusiveNode& node)
    {
        const auto gameNodeItr = mGameIntrusiveMap.find(node.getGameId());
        if (gameNodeItr != mGameIntrusiveMap.end())
        {
            WARN_LOG("[MatchmakingSessionSlave].insertGameIntrusiveMap Attempted to add the nodes multiple times for game: " << gameNodeItr->first << ". Removing initial node added." );
            destroyIntrusiveNode(*gameNodeItr->second);
        }

        mGameIntrusiveMap.insert(eastl::make_pair(node.getGameId(), &node));
    }

    void MatchmakingSessionSlave::eraseGameIntrusiveMap(GameManager::MmSessionGameIntrusiveNode& node)
    {
        MMGameIntrusiveMap::iterator iter = mGameIntrusiveMap.find(node.getGameId());
        if (iter != mGameIntrusiveMap.end())
        {
            mGameIntrusiveMap.erase(iter);
        }
    }


    /*! ************************************************************************************************/
    /*! \brief increment the MM session's find game diagnostics counts, for its criteria (added to system
        wide totals when MM session completes)
    ***************************************************************************************************/
    void MatchmakingSessionSlave::tallySessionDiagnosticsFG(MatchmakingSubsessionDiagnostics& diagnostics,
        const DiagnosticsSearchSlaveHelper& helper)
    {
        // Set the across-rules diagnostics:
        diagnostics.setSessions(1);
        diagnostics.setFindRequestsGamesAvailable(helper.getTotalGamesCount());

        // Set per rule diagnostics (using RETE conditions and the SS helper):

        const Rete::ConditionBlockList* conditions = nullptr;
        Rete::ConditionBlockList tmpConditions;
        if (!getConditions().empty() && !getConditions().front().empty())
        {
            // non-delayed sessions already have conditions
            conditions = &getConditions();
        }
        else
        {
            // delayed sessions will need to make a temp conditions to take a snapshot
            tmpConditions.resize(Rete::CONDITION_BLOCK_MAX_SIZE);
            for (auto& rule : getReteRules())
            {
                if (!rule->isDisabled())
                    rule->addConditions(tmpConditions);
            }
            conditions = &tmpConditions;
        }

        for (auto& rule : getMatchmakingCriteria().getRules())
        {
            rule->tallyRuleDiagnosticsFindGame(diagnostics, *conditions, helper);
        }
    }
}
}
}
