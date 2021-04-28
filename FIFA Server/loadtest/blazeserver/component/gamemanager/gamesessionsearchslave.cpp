/*! ************************************************************************************************/
/*!
    \file gamesessionsearchslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/gamemanagermasterimpl.h"
#include "gamemanager/searchcomponent/searchslaveimpl.h"
#include "gamemanager/tdf/search_config_server.h" // for Blaze::Search::SearchConfig
#include "gamemanager/matchmaker/matchmakingsessionslave.h" //for MatchmakingSessionSlave in MmSessionGameIntrusiveNode, ~GameSessionSearchSlave
#include "gamemanager/matchmaker/rules/teamuedbalanceruledefinition.h" // for the team ued rule balance definition
#include "gamemanager/matchmaker/rules/propertiesrules/propertyruledefinition.h" // for PropertyRuleDefinition in updateGameProperties()
#include "gamemanager/tdf/gamemanager_server.h"

using namespace Blaze::GameManager;

namespace Blaze
{
namespace GameManager 
{
    MmSessionGameIntrusiveNode::MmSessionGameIntrusiveNode(Search::GameSessionSearchSlave& gameSession, Blaze::GameManager::Matchmaker::MatchmakingSessionSlave& mmSession): 
        mGameSessionSlave(gameSession), mMatchmakingSessionSlave(mmSession), 
        mNonReteFitScore(0), mPostReteFitScore(0), mReteFitScore(0), mOverallResult(NO_MATCH), mIsCurrentlyMatched(false),
        mIsCurrentlyHeaped(false), mIsGameReferenced(false), mIsToBeSentToMatchmakerSlave(false)
    {
        // Need to manually clear these
        MmSessionIntrusiveListNode::mpNext = nullptr;
        MmSessionIntrusiveListNode::mpPrev = nullptr;
        ProductionMatchHeapNode::mpNext = nullptr;
        ProductionMatchHeapNode::mpPrev = nullptr;
        ProductionTopMatchHeapNode::mpNext = nullptr;
        ProductionTopMatchHeapNode::mpPrev = nullptr;
    }

    MmSessionGameIntrusiveNode::~MmSessionGameIntrusiveNode()
    {
        if (mIsGameReferenced)
        {
            MmSessionIntrusiveList::remove(*this);
            mIsGameReferenced = false;
            mGameSessionSlave.decrementMmSessionIntrusiveListSize();
        }

        if (mIsCurrentlyMatched)
        {
            ProductionTopMatchHeap::remove(*this);
            mIsCurrentlyMatched = false;
        }
        if (mIsCurrentlyHeaped)
        {
            ProductionMatchHeap::remove(*this);
            mIsCurrentlyHeaped = false;
        }
        //ASSERT(mGameSessionSlave.getMmSessionIntrusiveListSize() > 0);
        //ASSERT(mMmSessionGameIntrusiveNodeCount > 0);
    }
    
    GameId MmSessionGameIntrusiveNode::getGameId() const
    {
        return mGameSessionSlave.getGameId();
    }
    
    GbListGameIntrusiveNode::GbListGameIntrusiveNode(Search::GameSessionSearchSlave& gameSession,
        Search::GameList& gbList) :
        mGameSessionSlave(gameSession), mGameBrowserList(gbList), mPInfo(nullptr),
        mNonReteFitScore(0), mReteFitScore(0),  mPostReteFitScore(0)
    {
        // Need to manually clear these
        mpNext = mpPrev = nullptr;
    }

    GbListGameIntrusiveNode::~GbListGameIntrusiveNode()
    {
        // if (still) referenced by its GameSessionSeracjSlave::mGbListIntrusiveList, remove
        if (mpNext != nullptr && mpPrev != nullptr)
        {
            GbListIntrusiveList::remove(*this);
            mpNext = mpPrev = nullptr;
            mGameSessionSlave.decrementGbListIntrusiveListSize();
        }
    }    

    GameId GbListGameIntrusiveNode::getGameId() const
    {
        return mGameSessionSlave.getGameId();
    }

}

namespace Search
{
    GameSessionSearchSlave::GameSessionSearchSlave(Search::SearchSlaveImpl &slaveComponentImpl, GameManager::ReplicatedGameDataServer& replicatedGameSession)
        : GameSession(replicatedGameSession),
        mRefCount(0),
        mSearchSlaveImpl(slaveComponentImpl),
        mIsInRete(false),
        mMmSessionIntrusiveListCount(0),
        mGbListIntrusiveListCount(0)
    {
        createCriteriaExpressions();
        updateRoleEntryCriteriaEvaluators();

        mMatchmakingCache.setTeamInfoDirty();
        mMatchmakingCache.setGeoLocationDirty();
        mMatchmakingCache.setRosterDirty();

        // Add the topology host to be associated with the game (important because we must clean out mTopologyHostUserSession when host leaves)
        // Note that the topology host may or may not be a player in the game, either way, we don't want to wait
        // for the notification of the player joining so we do this for all topologies.
        // Also, this is not dependent on the session being valid at this time, due to replication timings,
        // we are only dependent on the id.

        // the internal structure is a set, so we're ok doing this insert for both, even if dedicated and topology are the same
        mSearchSlaveImpl.addGameForUser(getTopologyHostSessionId(), *this);
        mSearchSlaveImpl.addGameForUser(getDedicatedServerHostSessionId(), *this);
        mSearchSlaveImpl.addGameForUser(getExternalOwnerSessionId(), *this);

        mMmSessionIntrusiveList.clear();
        mGbListIntrusiveList.clear();
        mPlayerRoster.setOwningGameSession(*this);
    }

    GameSessionSearchSlave::~GameSessionSearchSlave()
    {
        // During destroy game, the topology host of a c/s dedicated game will not be removed from
        // the player map (since they are not a player in the game).  They will see that that the map
        // is being destroyed, and we clean them up here.  Any player actually in the game, will be notified
        // properly through removal replication.
        if (hasDedicatedServerHost())
        {
            // We do not check the existence of the session here to reduce dependence on ordering of onUserSessionLogout
            // UserManager replication, and GameManager replication.  RemoveGameForUser handles all the unique
            // cases involved, and will cleanup properly. Though we call add on topology host in the constructor, it can be ignored here,
            // as the only time topology & dedicated will differ is for failover modes, and if they are different, the topology host was a player.
            mSearchSlaveImpl.removeGameForUser(getDedicatedServerHostSessionId(), getDedicatedServerHostBlazeId(), *this);
        }
        if (hasExternalOwner())
        {
            mSearchSlaveImpl.removeGameForUser(getExternalOwnerSessionId(), getExternalOwnerBlazeId(), *this);
        }

        //Drop all our references to MM sessions (that found this game through the rete tree)
        while (!mMmSessionIntrusiveList.empty())
        {
            Blaze::GameManager::MmSessionGameIntrusiveNode &n =
                static_cast<Blaze::GameManager::MmSessionGameIntrusiveNode &>(mMmSessionIntrusiveList.front());
            n.mMatchmakingSessionSlave.destroyIntrusiveNode(n);
        }

        //Drop all our references to GB lists (that found this game through the rete tree)
        while (!mGbListIntrusiveList.empty())
        {
            Blaze::GameManager::GbListGameIntrusiveNode &n =
                static_cast<Blaze::GameManager::GbListGameIntrusiveNode &>(mGbListIntrusiveList.front());
            n.mGameBrowserList.destroyIntrusiveNode(n);
            //side above does: mGameManagerSlaveImpl.getGameBrowser().deleteGameIntrusiveNode(&n);
            //pre: atomic loop never interleaves with referenced GameBrowserList cleanups (removed my ref from mm session, so its cleanup wont ever try to delete again)
        }
    }


    /*! ************************************************************************************************/
    /*! \brief handle a player joining the game in either the player map, or the queue map.
    ***************************************************************************************************/
    void GameSessionSearchSlave::insertPlayer(const GameManager::PlayerInfo &joiningPlayer)
    {
        // add player to roster, and update game's mappings
        mSearchSlaveImpl.addGameForUser(joiningPlayer.getPlayerSessionId(), *this);

        // Removal is not dependent on existing user session, so addition shouldn't be either.
        BlazeId playerBlazeId = gUserSessionManager->getBlazeId(joiningPlayer.getPlayerSessionId());
        if (playerBlazeId != INVALID_BLAZE_ID)
        {
            const PlatformInfo& platformInfo = gUserSessionManager->getPlatformInfo(joiningPlayer.getPlayerSessionId());
            mSearchSlaveImpl.getMatchmakingSlave().onUserJoinGame(playerBlazeId, platformInfo, *this);
            mSearchSlaveImpl.onGameSessionUpdated(*this);
            mSearchSlaveImpl.getUserSetSearchManager().onUserJoinGame(playerBlazeId, getGameId());
        }
    }

    void GameSessionSearchSlave::updatePlayer(const GameManager::PlayerInfo &player)
    {
        const ReplicatedGamePlayerServer* prevPlayerSnapshot = player.getPrevSnapshot();
        if (prevPlayerSnapshot != nullptr)
        {
            const GameManager::UserSessionInfo& userSessionInfo = prevPlayerSnapshot->getUserInfo();
            UserSessionId prevSessionId = userSessionInfo.getSessionId();


            bool updateMatchmaking = false;
            if (player.getPlayerSessionId() != prevSessionId)
            {
                //  switch mappings from the reservation maker to the claimer
                BlazeId prevBlazeId = userSessionInfo.getUserInfo().getId();
                mSearchSlaveImpl.removeGameForUser(prevSessionId, prevBlazeId, *this);
                mSearchSlaveImpl.addGameForUser(player.getPlayerSessionId(), *this);
                updateMatchmaking = true;
                mMatchmakingCache.setRosterDirty();
            }

            if (player.getPlayerState() != prevPlayerSnapshot->getPlayerState())
            {
                updateMatchmaking = true;
                mMatchmakingCache.setRosterDirty();
            }

            if (!player.getPlayerAttribs().equalsValue(prevPlayerSnapshot->getPlayerAttribs()))
            {
                updateMatchmaking = true;
                mMatchmakingCache.setPlayerAttributeDirty();
            }

            if (player.getTeamIndex() != prevPlayerSnapshot->getTeamIndex() ||
                player.getSlotType() != prevPlayerSnapshot->getSlotType() ||
                blaze_stricmp(player.getRoleName(), prevPlayerSnapshot->getRoleName()) != 0)
            {
                updateMatchmaking = true;
            }

            if (updateMatchmaking)
            {
                mSearchSlaveImpl.getMatchmakingSlave().onUpdate(*this);
                mSearchSlaveImpl.onGameSessionUpdated(*this);
            }
        }
        else
        {
            ASSERT_LOG("[GameSessionSearchSlave].updatePlayer player(" << player.getPlayerId() << ") in game(" << getGameId() << ") is missing previous snapshot.");
        }
    }

    void GameSessionSearchSlave::erasePlayer(const GameManager::PlayerInfo& departingPlayer)
    {
        mSearchSlaveImpl.removeGameForUser(departingPlayer.getPlayerSessionId(), departingPlayer.getPlayerId(), *this);

        mSearchSlaveImpl.getMatchmakingSlave().onUserLeaveGame(departingPlayer.getPlayerId(), departingPlayer.getPlatformInfo(), *this);
        mSearchSlaveImpl.onGameSessionUpdated(*this);

        mSearchSlaveImpl.getUserSetSearchManager().onUserLeaveGame(departingPlayer.getPlayerId(), getGameId());
    }

    bool GameSessionSearchSlave::isJoinableForUser(const GameManager::UserSessionInfo& userSessionInfo, bool ignoreEntryCriteria, const char8_t* roleName /*= nullptr*/, bool checkAlreadyInRoster /*= false*/) const
    {
        if (checkAlreadyInRoster)
        {
            PlayerInfo* playerInfo = mPlayerRoster.getPlayer(userSessionInfo.getUserInfo().getId());
            if (playerInfo != nullptr && playerInfo->isInRoster())
            {
                TRACE_LOG("[GameSessionSearchSlave].isJoinableForUser: user '" << userSessionInfo.getSessionId() << "/"
                    << userSessionInfo.getUserInfo().getId() << "' is already a member of the game '" << getGameId() << "'. Can not join again.");
                return false;
            }
        }

        EntryCriteriaName criteriaName;
        bool passEntryCriteria = ignoreEntryCriteria || evaluateEntryCriteria(userSessionInfo.getUserInfo().getId(), userSessionInfo.getDataMap(), criteriaName);

        if (!passEntryCriteria)
        {
            TRACE_LOG("[GameSessionSearchSlave].isJoinableForUser user '" << userSessionInfo.getSessionId() << "/"
                << userSessionInfo.getUserInfo().getId() << "' failed entry criteria '" << criteriaName.c_str() << "' for game '" << getGameId() << "'");
            return false;
        }

        // passed base entry criteria, check role-specific criteria, if requested (may not be available if the caller is a GB session)
        if (!ignoreEntryCriteria && passEntryCriteria && roleName)
        {
            RoleEntryCriteriaEvaluatorMap::const_iterator roleCriteriaIter = mRoleEntryCriteriaEvaluators.find(roleName);
            if (roleCriteriaIter != mRoleEntryCriteriaEvaluators.end())
            {
                passEntryCriteria = roleCriteriaIter->second->evaluateEntryCriteria(userSessionInfo.getUserInfo().getId(), userSessionInfo.getDataMap(), criteriaName);
                if (!passEntryCriteria)
                {
                    TRACE_LOG("[GameSessionSearchSlave].isJoinableForUser user '" << userSessionInfo.getSessionId() << "/"
                        << userSessionInfo.getUserInfo().getId() << "' failed role entry criteria '" << criteriaName.c_str() << "' for role '"
                        << roleName << "' in game '" << getGameId() << "'");
                    return false;
                }
            }
        }

        // We do not check the multi-role entry criteria here, because this function only takes one session at a time.  
        // (Additionally, it will be checked for when the game is attempted to be joined)
        return true;
    }

    /*! \brief  Update RETE about this game. Used by GameBrowser & find game Matchmaking. */
    void GameSessionSearchSlave::updateRete()
    {
        mSearchSlaveImpl.updateWorkingMemoryElements(*this);
    }

    /*! ************************************************************************************************/
    /*! \brief retrieve game attribute value for the configured mode attribute

        \return game attribute value for the given attribute, nullptr if the attribute is not found
    ***************************************************************************************************/
    const char8_t* GameSessionSearchSlave::getGameMode() const
    {
        return getGameAttrib(mSearchSlaveImpl.getConfig().getGameSession().getGameModeAttributeName());
    }

    const char8_t* GameSessionSearchSlave::getPINGameModeType() const
    {
        const char8_t* modeType = getGameAttrib(mSearchSlaveImpl.getConfig().getGameSession().getPINGameModeTypeAttributeName());
        return (modeType != nullptr) ? modeType : RiverPoster::PIN_MODE_TYPE;
    }

    const char8_t* GameSessionSearchSlave::getPINGameType() const
    {
        const char8_t* gameType = getGameAttrib(mSearchSlaveImpl.getConfig().getGameSession().getPINGameTypeAttributeName());
        return (gameType != nullptr) ? gameType : RiverPoster::PIN_GAME_TYPE;
    }

    const char8_t* GameSessionSearchSlave::getPINGameMap() const
    {
        return getGameAttrib(mSearchSlaveImpl.getConfig().getGameSession().getPINGameMapAttributeName());
    }

    /*! ************************************************************************************************/
    /*! \brief update the cached attribute values for the supplied game.  Note: the game's MatchmakingGameInfoCache
        member will be modified.

        \param[in] ruleDefinitionCollection - the set of all defined matchmaking (or game browsing) rules
        \param[in] memberSessions - All MM sessions participating in the game
                        NOTE: nullptr for game browsing
    ***************************************************************************************************/
    void GameSessionSearchSlave::updateCachedGameValues(const GameManager::Matchmaker::RuleDefinitionCollection& ruleDefinitionCollection,
        const GameManager::Matchmaker::MatchmakingSessionList* memberSessions/* = nullptr*/)
    {
        if (!mMatchmakingCache.isDirty())
        {
            WARN_LOG("[GameSession].updateCachedGameValues: Called for non-dirty MM cache (gameid = " << getGameId() << ")")
                return;
        }

        const GameManager::Matchmaker::RuleDefinitionList& ruleDefinitionList = ruleDefinitionCollection.getRuleDefinitionList();
        GameManager::Matchmaker::RuleDefinitionList::const_iterator itr = ruleDefinitionList.begin();
        GameManager::Matchmaker::RuleDefinitionList::const_iterator end = ruleDefinitionList.end();

        for (; itr != end; ++itr)
        {
            const GameManager::Matchmaker::RuleDefinition& ruleDefinition = **itr;
            if (!ruleDefinition.isDisabled())
            {
                ruleDefinition.updateMatchmakingCache(mMatchmakingCache, *this, memberSessions);
            }
        }

        // MM_TODO: need to dirty all existing games when MM config is reloaded (and fixup various cached values)
    }

    StorageRecordVersion GameSessionSearchSlave::getGameRecordVersion()
    {
        return (mSearchSlaveImpl.getGameRecordVersion(getGameId()));
    }

    void GameSessionSearchSlave::updateGameProperties()
    {
        // Update to the latest Properties state. PropertyRuleDefinition will compare and update mPrevGamePropertyMap, to optimize updating only the things that have changed.
        
        // general game properties
        mGameProperties.clear();
        mGameProperties.mServerGameData = mGameData;            // PACKER_TODO:  The replicated data should have all the params used below:
        mGameProperties.addPlayerRosterSourceData(&mPlayerRoster);      // Revised Design: Send the roster over to the Properties to be managed there:
        mGameProperties.mGameSession = this;


        // We don't include this code in the PropertyManager code currently, since it may need to be revised as team support is fleshed out in Packer. 
        // team properties
        auto& teamProperties = mGameProperties.mTeamProperties;

        // team sizes
        uint16_t teamCount = getTeamCount();
        uint16_t minTeamSize = UINT16_MAX;
        uint16_t maxTeamSize = 0;
        auto& factionCountValuesMap = teamProperties.getFaction();
        for (uint16_t teamIndex = 0; teamIndex < teamCount; ++teamIndex)
        {
            uint16_t curTeamSize = mPlayerRoster.getTeamSize(teamIndex);
            TeamId teamId = getTeamIdByIndex(teamIndex);
            if (teamId == INVALID_TEAM_ID)
            {
                continue;
            }

            FactionCountValues* factionCountValues = factionCountValuesMap[teamId];
            if (factionCountValues == nullptr)
            {
                factionCountValues = factionCountValuesMap.allocate_element();
                factionCountValuesMap[teamId] = factionCountValues;
            }
            factionCountValues->setCount(factionCountValues->getCount() + 1);
            uint16_t availableTeamCap = getTeamCapacity() - curTeamSize;

            if (availableTeamCap > factionCountValues->getMaxFreeCapacity())
            {
                factionCountValues->setMaxFreeCapacity(availableTeamCap);
            }

            if (curTeamSize < minTeamSize)
            {
                minTeamSize = curTeamSize;
            }
            if (curTeamSize > maxTeamSize)
            {
                maxTeamSize = curTeamSize;
            }
        }

        auto& teamMemberCountValues = teamProperties.getMemberCount();

        teamMemberCountValues.setMin(minTeamSize);
        teamMemberCountValues.setMax(maxTeamSize);
        teamMemberCountValues.setRange(maxTeamSize - minTeamSize);

        // roles list
        auto& roleCriteria = teamProperties.getRoles();
        getRoleInformation().getRoleCriteriaMap().copyInto(roleCriteria);
        auto& roleMemberCountsMap = teamProperties.getRoleMembers();
        RoleCriteriaMap::const_iterator roleIter = getRoleInformation().getRoleCriteriaMap().begin();
        RoleCriteriaMap::const_iterator roleEnd = getRoleInformation().getRoleCriteriaMap().end();
        for (; roleIter != roleEnd; ++roleIter)
        {
            MemberCountValuesPtr roleMemberCountValues = roleMemberCountsMap.allocate_element();
            uint16_t mostRoleMembers = 0;
            uint16_t leastRoleMembers = UINT16_MAX;

            for (TeamIndex teamIndex = 0; teamIndex < teamCount; ++teamIndex)
            {
                uint16_t tempRoleSize = getPlayerRoster()->getRoleSize(teamIndex, roleIter->first);

                if (mostRoleMembers < tempRoleSize)
                {
                    mostRoleMembers = tempRoleSize;
                }

                if (leastRoleMembers > tempRoleSize)
                {
                    leastRoleMembers = tempRoleSize;
                }
            }

            roleMemberCountValues->setMax(mostRoleMembers);
            roleMemberCountValues->setMin(leastRoleMembers);
            roleMemberCountValues->setRange(mostRoleMembers - leastRoleMembers);
            roleMemberCountsMap[roleIter->first] = roleMemberCountValues;
        }

        // need to build out UED map//TODO: rem dependency on rules for team UED properties also
        auto& teamUedValueMap = teamProperties.getUedValue();
        for (const auto teamUedIter : mSearchSlaveImpl.getConfig().getMatchmakerSettings().getRules().getTeamUEDBalanceRuleMap())
        {
            const GameManager::Matchmaker::TeamUEDBalanceRuleDefinition* uedRuleDefinition = (const GameManager::Matchmaker::TeamUEDBalanceRuleDefinition*)mSearchSlaveImpl.getMatchmakingSlave().getRuleDefinitionCollection().getRuleDefinitionByName(teamUedIter.first);
            if (uedRuleDefinition != nullptr)
            {
                const auto uedName = teamUedIter.second->getUserExtendedDataName();
                if (teamUedValueMap.find(uedName) == teamUedValueMap.end())
                {
                    teamUedValueMap[uedName] = teamUedValueMap.allocate_element();
                }

                TeamUEDValuePtr teamUedValues = teamUedValueMap[uedName];
                UserExtendedDataValue lowestTeamUED = INVALID_USER_EXTENDED_DATA_VALUE;
                UserExtendedDataValue highestTeamUED = INVALID_USER_EXTENDED_DATA_VALUE;
                UserExtendedDataValue uedRange = uedRuleDefinition->calcGameImbalance(*this, lowestTeamUED, highestTeamUED);
                GroupValueFormula groupValueFormula = uedRuleDefinition->getTeamValueFormula();
                // until we're not depending on existing UED rules, gonna only set the UED values that are calcuated by the rule
                switch (groupValueFormula)
                {
                case GROUP_VALUE_FORMULA_AVERAGE:
                    teamUedValues->setAvgRange(uedRange);
                    break;
                case GROUP_VALUE_FORMULA_MIN:
                    teamUedValues->setMinRange(uedRange);
                    break;
                case GROUP_VALUE_FORMULA_MAX:
                    teamUedValues->setMaxRange(uedRange);
                    break;
                case GROUP_VALUE_FORMULA_SUM:
                    teamUedValues->setSumRange(uedRange);
                    break;
                default:
                    // do nothing
                    break;
                };
            }
        }
    }

    void GameSessionSearchSlave::updateReplicatedGameData(GameManager::ReplicatedGameDataServer& gameData)
    {
        mPrevSnapshot = mGameData;
        mGameData = &gameData;

        updateGameProperties();
    }

    void GameSessionSearchSlave::discardPrevSnapshot()
    {
        mPrevSnapshot.reset();
    }

    void GameSessionSearchSlave::onUpsert(GameManager::ReplicatedGamePlayerServer& playerData)
    {
        mSearchSlaveImpl.getDiagnosticsHelper().updateGameCountsCaches(*this, false);

        PlayerId playerId = playerData.getUserInfo().getUserInfo().getId();
        PlayerInfo* playerInfo = mPlayerRoster.getPlayer(playerId);
        bool rosterUpdated = false;
        if (playerInfo != nullptr)
        {
            bool inRoster = playerInfo->isInRoster();
            playerInfo->updateReplicatedPlayerData(playerData);
            updatePlayer(*playerInfo);
            playerInfo->discardPrevSnapshot();
            if (!inRoster && playerInfo->isInRoster())
                rosterUpdated = true;
        }
        else
        {
            playerInfo = BLAZE_NEW PlayerInfo(&playerData);
            mPlayerRoster.insertPlayer(*playerInfo);
            insertPlayer(*playerInfo);
            rosterUpdated = playerInfo->isInRoster();
        }
        if (rosterUpdated)
        {
            mMatchmakingCache.setRosterDirty();
            mSearchSlaveImpl.getMatchmakingSlave().onUpdate(*this);

            // updateRete() will call updateGameProperties internally. 
        }
        updateRete();

        mSearchSlaveImpl.getDiagnosticsHelper().updateGameCountsCaches(*this, true);
    }

    void GameSessionSearchSlave::onErase(GameManager::ReplicatedGamePlayerServer& playerData)
    {
        mSearchSlaveImpl.getDiagnosticsHelper().updateGameCountsCaches(*this, false);

        PlayerId playerId = playerData.getUserInfo().getUserInfo().getId();
        PlayerInfo* playerInfo = mPlayerRoster.getPlayer(playerId);
        if (playerInfo != nullptr)
        {
            bool inRoster = playerInfo->isInRoster();
            erasePlayer(*playerInfo);
            mPlayerRoster.erasePlayer(playerId);
            if (inRoster)
            {
                // Matchmaking only cares about player updates for players actually in game.
                mMatchmakingCache.setRosterDirty();
                mSearchSlaveImpl.getMatchmakingSlave().onUpdate(*this);
            }
            updateRete();
        }

        mSearchSlaveImpl.getDiagnosticsHelper().updateGameCountsCaches(*this, true);
    }

    void intrusive_ptr_add_ref(GameSessionSearchSlave* ptr)
    {
        ++ptr->mRefCount;
    }

    void intrusive_ptr_release(GameSessionSearchSlave* ptr)
    {
        if (ptr->mRefCount > 0)
        {
            --ptr->mRefCount;
            if (ptr->mRefCount == 0)
                delete ptr;
        }
    }

}// namespace Search
}// namespace Blaze
