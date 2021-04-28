/*! ************************************************************************************************/
/*!
    \file   matchmakinggameinfocache.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/matchmaker/rules/playerattributeruledefinition.h"
#include "gamemanager/matchmaker/rules/gameattributeruledefinition.h"
#include "gamemanager/matchmaker/rules/dedicatedserverattributeruledefinition.h"
#include "gamemanager/matchmaker/rules/geolocationruledefinition.h"
#include "gamemanager/matchmaker/rules/uedruledefinition.h"
#include "gamemanager/matchmaker/rules/teamuedbalanceruledefinition.h"
#include "gamemanager/matchmaker/rules/teamuedpositionparityruledefinition.h"
#include "gamemanager/matchmaker/rules/propertiesrules/propertyruledefinition.h" // for PropertyRuleDefinition in updateGamePropertyUEDValues()
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/playerinfo.h"
#include "gamemanager/playerroster.h"



// MM_TODO: need to dirty all existing games when MM config is reloaded (and fixup various cached values)

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    //! \brief constructed with all dirty flags set (games are created dirty, as far as the matchmaker is concerned)
    MatchmakingGameInfoCache::MatchmakingGameInfoCache()
        : mExplicitGameAttribActualIndexMap(BlazeStlAllocator("MatchmakingGameInfoCache::mGameAttribActualIndexMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mRulesAndAttributesInfoMap(BlazeStlAllocator("MatchmakingGameInfoCache::mRulesAndAttributesInfoMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mArbitraryValueMap(BlazeStlAllocator("MatchmakingGameInfoCache::mGameArbitraryValueMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mCachedGameUEDValuesMap(BlazeStlAllocator("MatchmakingGameInfoCache::mCachedGameUEDValuesMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mCachedTeamUEDValuesMap(BlazeStlAllocator("MatchmakingGameInfoCache::mCachedTeamUEDValuesMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mCachedTeamIndividualUEDValuesMap(BlazeStlAllocator("MatchmakingGameInfoCache::mCachedTeamIndividualUEDValuesMap", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mTopologyHostLocation(),
        mSortedTeamIdInfoVector(BlazeStlAllocator("MatchmakingGameInfoCache::mSortedTeamIdInfoVector", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mSortedTeamIdVector(*Allocator::getAllocator(GameManagerSlave::COMPONENT_MEMORY_GROUP), "MatchmakingGameInfoCache::mSortedTeamIdVector"),
        mCachedTeamIdWMEs(BlazeStlAllocator("MatchmakingGameInfoCache::mCachedTeamIdWMEs", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mRoleRuleAllowedRolesWMENames(BlazeStlAllocator("MatchmakingGameInfoCache::mRoleRuleAllowedRolesWMENames", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
        mTeamCompositionRuleWMENamesMap(BlazeStlAllocator("MatchmakingGameInfoCache::mTeamCompositionRuleWMENamesMap", GameManagerSlave::COMPONENT_MEMORY_GROUP))
    {
        // all games are dirty when they're created
        setDirty();
    }

    /*! ************************************************************************************************/
    /*! \brief clears ALL caches based on rule definition, setting ALL dirty flags. Used for invalidating 
        the cache when the game may not have changed, but our rules have(reconfiguration).
    ***************************************************************************************************/
    void MatchmakingGameInfoCache::clearRuleCaches()
    {
        mExplicitGameAttribActualIndexMap.clear();
        mArbitraryValueMap.clear();
        setGameAttributeDirty();
        setDedicatedServerAttributeDirty();
//        setMeshAttributeDirty();
        mRulesAndAttributesInfoMap.clear();
        setPlayerAttributeDirty();

        mCachedGameUEDValuesMap.clear();
        mCachedTeamUEDValuesMap.clear();
        setRosterDirty();
    }

    
    /*! ************************************************************************************************/
    /*!
        \brief cache the possibleValueIndex for a particular gameAttribute.

        Looks up the game's value for the rule's attribute, determines the value's possibleValueIndex and
        caches the result.

        \param[in] gameSession the game to examine; note: 'this' is the game's matchmakingGameInfoCache
        \param[in] ruleDefn the matchmaking rule to cache (provides attributeName and possibleValueList)
    ***************************************************************************************************/
    void MatchmakingGameInfoCache::cacheGameAttributeIndex(const Search::GameSessionSearchSlave& gameSession,
            const GameAttributeRuleDefinition& ruleDefn)
    {
        const char8_t* attribValue = ruleDefn.getGameAttributeValue(gameSession);

        RuleDefinitionId ruleDefId = ruleDefn.getID();
        if (attribValue == nullptr)
        {
            mExplicitGameAttribActualIndexMap[ruleDefId] = ATTRIBUTE_INDEX_NOT_DEFINED;
        }
        else
        {
            int actualIndex = ruleDefn.getPossibleValueIndex(attribValue);
            if (actualIndex < 0)
            {
                mExplicitGameAttribActualIndexMap[ruleDefId] = ATTRIBUTE_INDEX_IMPOSSIBLE;
            }
            else
            {
                mExplicitGameAttribActualIndexMap[ruleDefId] = actualIndex;
            } // if
        } // if
    }

    /*! ************************************************************************************************/
    /*!
    \brief cache the possibleValueIndex for a particular dedicatedServerAttribute.

    Looks up the game's value for the rule's attribute, determines the value's possibleValueIndex and
    caches the result.

    \param[in] gameSession the game to examine; note: 'this' is the game's matchmakingGameInfoCache
    \param[in] ruleDefn the matchmaking rule to cache (provides attributeName and possibleValueList)
    ***************************************************************************************************/
    void MatchmakingGameInfoCache::cacheDedicatedServerAttributeIndex(const Search::GameSessionSearchSlave& gameSession, const DedicatedServerAttributeRuleDefinition& ruleDefn)
    {
        const char8_t* attribValue = ruleDefn.getDedicatedServerAttributeValue(gameSession);

        RuleDefinitionId ruleDefId = ruleDefn.getID();
        if (attribValue == nullptr)
        {
            mExplicitDSAttribActualIndexMap[ruleDefId] = ATTRIBUTE_INDEX_NOT_DEFINED;
        }
        else
        {
            int actualIndex = ruleDefn.getPossibleValueIndex(attribValue);
            if (actualIndex < 0)
            {
                mExplicitDSAttribActualIndexMap[ruleDefId] = ATTRIBUTE_INDEX_IMPOSSIBLE;
            }
            else
            {
                mExplicitDSAttribActualIndexMap[ruleDefId] = actualIndex;
            } // if
        } // if
    }

    /*! ************************************************************************************************/
    /*!
        \brief cache the possibleValueIndex for a particular gameAttribute.

        Looks up the game's value for the rule's attribute, determines the value's possibleValueIndex and
        caches the result.

        \param[in] gameSession the game to examine; note: 'this' is the game's matchmakingGameInfoCache
        \param[in] ruleDefn the matchmaking rule to cache (provides attributeName and possibleValueList)
    ***************************************************************************************************/
    void MatchmakingGameInfoCache::cacheArbitraryGameAttributeIndex(const Search::GameSessionSearchSlave& gameSession,
            const GameAttributeRuleDefinition& ruleDefn)
    {
        const char8_t* attribName = ruleDefn.getAttributeName();
        const char8_t* attribValue = ruleDefn.getGameAttributeValue(gameSession);

        RuleDefinitionId ruleDefId = ruleDefn.getID();
        if (attribValue == nullptr)
        {
            TRACE_LOG("[MatchmakingGameInfoCache].cacheGameAttributeIndex: ArbitraryValue is not defined for rule(" 
                      << ruleDefn.getName() << "), Attribute(" << attribName << ")");
            mArbitraryValueMap[ruleDefId] = ATTRIBUTE_VALUE_NOT_DEFINED;
        }
        else
        {
            ArbitraryValue value = ruleDefn.getWMEAttributeValue(attribValue, true);
            mArbitraryValueMap[ruleDefId] = value;
        } // if
    } // cacheGameAttributeIndex

   int MatchmakingGameInfoCache::getCachedGameAttributeIndex(const GameAttributeRuleDefinition& ruleDefn) const
    {
        ExplicitAttributeIndexMap::const_iterator cacheIter = mExplicitGameAttribActualIndexMap.find(ruleDefn.getID());
        if (cacheIter==mExplicitGameAttribActualIndexMap.end())
        {
            // Warning that we are evaluating someone against a game that was not initialized with the value.
            // This can happen post match, when reevaluating a created game
            WARN_LOG("[MatchmakingInfoCache] Trying to get cached arbitrary values on game that was not initialized with value for rule definition " << ruleDefn.getName());
            return ATTRIBUTE_INDEX_NOT_DEFINED;
        }
        return cacheIter->second;
    }
    
   int MatchmakingGameInfoCache::getCachedDedicatedServerAttributeIndex(const DedicatedServerAttributeRuleDefinition& ruleDefn) const
   {
       ExplicitAttributeIndexMap::const_iterator cacheIter = mExplicitDSAttribActualIndexMap.find(ruleDefn.getID());
       if (cacheIter == mExplicitGameAttribActualIndexMap.end())
       {
           // Warning that we are evaluating someone against a game that was not initialized with the value.
           // This can happen post match, when reevaluating a created game
           WARN_LOG("[MatchmakingGameInfoCache].getCachedDedicatedServerAttributeIndex: Trying to get cached dedicated server attribute on game that was not initialized with value for rule definition " << ruleDefn.getName());
           return ATTRIBUTE_INDEX_NOT_DEFINED;
       }
       return cacheIter->second;
   }

    /*! ************************************************************************************************/
    /*! 
        \brief returns the cached ArbitraryValue for the supplied rule's gameAttribute.

        Note: this object is a member of a GameSession instance, and cached values are for that game.

        \param[in] ruleDefn - the rule we're looking up a cached value for
        \param[out] defined - true if found.
        \return the cached possibleValueIndex for the supplied rule's attribute (< 0 indicates an error)
    ***************************************************************************************************/
    ArbitraryValue MatchmakingGameInfoCache::getCachedGameArbitraryValue(const GameAttributeRuleDefinition& ruleDefn,
            bool& defined) const
    {
        defined = false;
        ArbitraryValueMap::const_iterator cacheIter = mArbitraryValueMap.find(ruleDefn.getID());
        if (cacheIter == mArbitraryValueMap.end())
        {
            // Warning that we are evaluating someone against a game that was not initialized with the value.
            // This can happen post match, when reevaluating a created game
            WARN_LOG("[MatchmakingInfoCache] Trying to get cached arbitrary values on game that was not initialized with value for rule definition " << ruleDefn.getName());
            return ATTRIBUTE_VALUE_NOT_DEFINED;
        }
        defined = true;
        return cacheIter->second;
    }

    /*! ************************************************************************************************/
    /*! 
        \brief returns the cached ruleValueBitset for the supplied rule's playerAttribute.  The bitset
        represents the union of 'the playerAttributeValue's indices for the supplied rule'.

        Note: this object is a member of a GameSession instance, and cached values are for that game.

        \param[in] ruleDefn - the rule we're looking up a cached value for
        \return the cached possibleValueIndex for the supplied rule's attribute; return nullptr if no value was cached
    ***************************************************************************************************/
    const AttributeValuePlayerCountMap* MatchmakingGameInfoCache::getCachedPlayerAttributeValues(const PlayerAttributeRuleDefinition& ruleDefn) const
    {
        PlayerAttributesAndIndexBitsetMap::const_iterator cacheIter = mRulesAndAttributesInfoMap.find(ruleDefn.getID());
        if (cacheIter == mRulesAndAttributesInfoMap.end())
        {
            TRACE_LOG("[MatchmakingInfoCache] Trying to get cached attribute values on player that were not initialized with value for rule definition " << ruleDefn.getName());
            return nullptr;
        }
        return &(cacheIter->second);
    }

    UserExtendedDataValue MatchmakingGameInfoCache::getCachedGameUEDValue(const UEDRuleDefinition& uedRuleDefinition) const
    {
        CachedGameUEDValuesMap::const_iterator cacheItr = mCachedGameUEDValuesMap.find(uedRuleDefinition.getID());

        if (cacheItr == mCachedGameUEDValuesMap.end())
        {
            TRACE_LOG("[MatchmakingInfoCache] Trying to get cached UED values on players that were not initialized with value for rule definition " << uedRuleDefinition.getName());
            return INVALID_USER_EXTENDED_DATA_VALUE;
        }

        return cacheItr->second;
    }


    /*! ************************************************************************************************/
    /*!
        \brief build and cache a bitset representing all the possibleValueIndices for a particular
            playerAttribute.  For arbitrary values, it pushes a copy into the cache.

        For example, if player1 has "myAttrib=foo", and player2 has "myAttrib=baz", the following bitset
        is built (assuming the rule's possibleValues are "foo,bar,baz") : 1,0,1.
          Index 0 is set due to player1's attributeValue ("foo" has possibleValueIndex 0)
          Index 1 is unset, since no players have an attributeValue ("bar" has possibleValueIndex 1)
          Index 2 is set due to player2's attributeValue ("baz" has possibleValueIndex 2)

        \param[in] gameSession the game to examine; note: 'this' is the game's matchmakingGameInfoCache
        \param[in] ruleDefn the matchmaking rule to cache (provides attributeName and possibleValueList)
        \param[in] memberSessions for everyone in the game.
    ***************************************************************************************************/
    void MatchmakingGameInfoCache::cachePlayerAttributeValues(const Search::GameSessionSearchSlave& gameSession,
            const PlayerAttributeRuleDefinition& ruleDefn,
            const MatchmakingSessionList* memberSessions/* = nullptr*/)
    {
        RuleDefinitionId ruleDefId = ruleDefn.getID();
        AttributeValuePlayerCountMap& playerCountMap = mRulesAndAttributesInfoMap[ruleDefId];

        // reset all maps first
        playerCountMap.clear();

        const char8_t* attribName = ruleDefn.getAttributeName();

        // if the game members is nullptr, means all players are already added into the game
        if (memberSessions == nullptr)
        {
            // iterate over all active game players, adding player attributes to the valueBitset
            // (we're building a bitset that represents the union of all the player's playerAttribute value indices)
            const PlayerRoster::PlayerInfoList& playerList = gameSession.getPlayerRoster()->getPlayers(PlayerRoster::ROSTER_PARTICIPANTS);
            PlayerRoster::PlayerInfoList::const_iterator playerIter = playerList.begin();
            PlayerRoster::PlayerInfoList::const_iterator end = playerList.end();
            for ( ; playerIter != end; ++playerIter )
            {
                PlayerInfo *playerInfo = *playerIter;

                const char8_t* attribValue = playerInfo->getPlayerAttrib(attribName);
                if (attribValue != nullptr)
                {
                    if (ruleDefn.getAttributeRuleType() == ARBITRARY_TYPE)
                    {
                        ArbitraryValue value = ruleDefn.getWMEAttributeValue(attribValue);
                        ++playerCountMap[value];
                    }
                    else if (ruleDefn.getAttributeRuleType() == EXPLICIT_TYPE)
                    {
                        int actualIndex = ruleDefn.getPossibleValueIndex(attribValue);
                        if (actualIndex >= 0)
                        {
                            ++playerCountMap[(uint64_t)actualIndex];
                        }
                    }
                }
            }
        }
        else
        {
            // We have a valid gameMembers list, which means it's a matchmaking session
            // and all session members in it are not added to the game yet, in this case
            // we will have to calculate the bitset using the player attribute in the
            // matchmaking session instead of from the player itself
            MatchmakingSessionList::const_iterator memberSessionIter = memberSessions->begin();
            MatchmakingSessionList::const_iterator gameMemberEnd = memberSessions->end();
            for ( ; memberSessionIter != gameMemberEnd; ++memberSessionIter)
            {
                PerPlayerJoinDataList::const_iterator curIter = (*memberSessionIter)->getPlayerJoinData().getPlayerDataList().begin();
                PerPlayerJoinDataList::const_iterator endIter = (*memberSessionIter)->getPlayerJoinData().getPlayerDataList().end();

                for (; curIter != endIter; ++curIter)
                {
                    Collections::AttributeMap::const_iterator attrIter = (*curIter)->getPlayerAttributes().find(attribName);
                    const char8_t* attribValue = (attrIter != (*curIter)->getPlayerAttributes().end()) ? attrIter->second : nullptr;
                    if (attribValue != nullptr)
                    {
                        if (ruleDefn.getAttributeRuleType() == ARBITRARY_TYPE)
                        {
                            ArbitraryValue value = ruleDefn.getWMEAttributeValue(attribValue);
                            ++playerCountMap[value];
                        }
                        else if (ruleDefn.getAttributeRuleType() == EXPLICIT_TYPE)
                        {
                            int actualIndex = ruleDefn.getPossibleValueIndex(attribValue);
                            if (actualIndex >= 0)
                            {
                                ++playerCountMap[(uint64_t)actualIndex];
                            }
                        }
                    }
                }
            } // for
        } // if

    } // cachePlayerAttributeValues



    // MM_AUDIT: This would be more efficient if we store off the UED values for
    // each definition for every player.  This way when a player is added
    // or removed the calculation doesn't have to look up all of the 
    // UED values off of the session again.
    void MatchmakingGameInfoCache::updateGameUEDValue(const UEDRuleDefinition &uedRuleDefinition, const Search::GameSessionSearchSlave &gameSession)
    {
        RuleDefinitionId ruleDefId = uedRuleDefinition.getID();
        mCachedGameUEDValuesMap[ruleDefId] = calcGameUEDValue(uedRuleDefinition, gameSession);
    }

    UserExtendedDataValue MatchmakingGameInfoCache::calcGameUEDValue(const CalcUEDUtil &calcUtil, const Search::GameSessionSearchSlave &gameSession) const
    {
        UserExtendedDataValue gameUEDValue = INVALID_USER_EXTENDED_DATA_VALUE;       

        // use roster participants because we care about the players in the game, reserved or active
        const PlayerRosterSlave& playerRoster = *gameSession.getPlayerRoster();
        PlayerRoster::PlayerInfoList playerList = playerRoster.getPlayers(PlayerRoster::ROSTER_PARTICIPANTS);
        if (!playerList.empty())
        {
            UserSessionInfoList infoList;
            infoList.reserve(playerList.size());
            for (PlayerRoster::PlayerInfoList::const_iterator itr = playerList.begin(), end = playerList.end(); itr != end; ++itr)
            {
                PlayerInfo* playerInfo = (*itr);
                uint16_t groupSize = playerRoster.getSizeOfGroupInGame(playerInfo->getUserGroupId());
                UserSessionInfo& sessionInfo = playerInfo->getUserInfo();
                sessionInfo.setGroupSize(groupSize);
                infoList.push_back(&sessionInfo);
            }
            const PlayerInfo* playerInfo = playerRoster.getPlayer(gameSession.getPlatformHostInfo().getPlayerId());
            // need to get the owner's group size and the game member's group map
            if (!calcUtil.calcUEDValue(gameUEDValue, gameSession.getPlatformHostInfo().getPlayerId(), ((playerInfo != nullptr) ? playerRoster.getSizeOfGroupInGame(playerInfo->getUserGroupId()) : 1), infoList))
            {

                TRACE_LOG("[MatchmakingGameInfoCache].calcGameUEDValue for game '" << gameSession.getGameId()
                    << "' failed to calc UED Value for '" << calcUtil.getUEDName() << "', using minimum value '"
                    << calcUtil.getMinRange() << "' Platform Host player '" << gameSession.getPlatformHostInfo().getPlayerId() << "' was "
                    << (playerInfo == nullptr ? "nullptr" : "not nullptr") << ".");
                gameUEDValue = calcUtil.getMinRange();
            }
            else
            {
                TRACE_LOG("[MatchmakingGameInfoCache].calcGameUEDValue for game '" << gameSession.getGameId()
                    << "' calculated UED Value for '" << calcUtil.getUEDName() << "', using value '" << gameUEDValue << "'.");
            }
        }
        else
        {
            // no players, mark it as invalid
            TRACE_LOG("[MatchmakingGameInfoCache].calcGameUEDValue for game '" << gameSession.getGameId()
                << "' no participants in game when calculating UED value for '" << calcUtil.getUEDName() << "', using value 'INVALID_USER_EXTENDED_DATA_VALUE'.");
        }

        return gameUEDValue;
    }

    const TeamUEDVector* MatchmakingGameInfoCache::getCachedTeamUEDVector(const TeamUEDBalanceRuleDefinition& uedRuleDefinition) const
    {
        RuleDefinitionId ruleDefId = uedRuleDefinition.getID();
        CachedTeamUEDValuesMap::const_iterator cacheItr = mCachedTeamUEDValuesMap.find(ruleDefId);
        if (cacheItr == mCachedTeamUEDValuesMap.end())
        {
            TRACE_LOG("[MatchmakingInfoCache] failed to get cached team UED values for rule " << uedRuleDefinition.getName());
            return nullptr;
        }
        return &cacheItr->second;
    }
    // cache off game's team UED's for quick repeat access in FG eval
    void MatchmakingGameInfoCache::cacheTeamUEDValues(const TeamUEDBalanceRuleDefinition &ruleDefinition, const Search::GameSessionSearchSlave& gameSession)
    {
        const UserExtendedDataValue defaultUEDValue = ruleDefinition.getMinRange();
        RuleDefinitionId ruleDefId = ruleDefinition.getID();
        TeamUEDVector& cachedTeamUEDVector = mCachedTeamUEDValuesMap[ruleDefId];
        cachedTeamUEDVector.clear();

        const uint16_t teamCount = gameSession.getTeamCount();
        if (teamCount > 0 && ruleDefinition.getUEDKey() != INVALID_USER_EXTENDED_DATA_KEY)
        {
            cachedTeamUEDVector.reserve(teamCount);

            // use roster participants because we care about the players in the game, reserved or active
            const PlayerRoster::PlayerInfoList& playerList = gameSession.getPlayerRoster()->getPlayers(PlayerRoster::ROSTER_PARTICIPANTS);
            if (playerList.empty())
            {
                TRACE_LOG("[MatchmakingGameInfoCache].cacheTeamUEDValues no active players (yet) for game '" << gameSession.getGameId() << "' to evaluate, Team UED's for Rule '" << ruleDefinition.getName() << "', using default " << defaultUEDValue << ".");
                cachedTeamUEDVector.insert(cachedTeamUEDVector.begin(), teamCount, defaultUEDValue);
                return;
            }
        
            // For each team, calculate the team ued and save to cache
            for (TeamIndex teamIndex = 0; teamIndex < teamCount; ++teamIndex)
            {
                UserExtendedDataValue teamUEDValue = gameSession.calcTeamUEDValue(teamIndex, ruleDefinition.getUEDKey(), ruleDefinition.getTeamValueFormula(), ruleDefinition.getGroupUedExpressionList(), defaultUEDValue);
                ruleDefinition.normalizeUEDValue(teamUEDValue);
                cachedTeamUEDVector.push_back(teamUEDValue);
                SPAM_LOG("[MatchmakingGameInfoCache].cacheTeamUEDValues Team UED Rule '" << ruleDefinition.getName() << "', updated cached Team UED for team index '" << teamIndex << "' for game '" << gameSession.getGameId() << "', to value '" << teamUEDValue << "'.");
            }
            TRACE_LOG("[MatchmakingGameInfoCache].cacheTeamUEDValues Team UED Rule '" << ruleDefinition.getName() << "', updated " << cachedTeamUEDVector.size() << "cached Team UEDs for game '" << gameSession.getGameId() << ".");
        }
    }

    const MatchmakingGameInfoCache::TeamIndividualUEDValues* MatchmakingGameInfoCache::getCachedTeamIndivdualUEDVectors(const TeamUEDPositionParityRuleDefinition& uedRuleDefinition) const
    {
        RuleDefinitionId ruleDefId = uedRuleDefinition.getID();
        CachedTeamIndividualUEDValuesMap::const_iterator cacheItr = mCachedTeamIndividualUEDValuesMap.find(ruleDefId);
        if (cacheItr == mCachedTeamIndividualUEDValuesMap.end())
        {
            TRACE_LOG("[MatchmakingInfoCache].getCachedTeamIndivdualUEDVectors() failed to get cached team individual UED values for rule " << uedRuleDefinition.getName());
            return nullptr;
        }
        return &cacheItr->second;
    }

    void MatchmakingGameInfoCache::sortCachedTeamIndivdualUEDVectors(const TeamUEDPositionParityRuleDefinition& uedRuleDefinition)
    {
        RuleDefinitionId ruleDefId = uedRuleDefinition.getID();
        CachedTeamIndividualUEDValuesMap::iterator cacheItr = mCachedTeamIndividualUEDValuesMap.find(ruleDefId);
        if (cacheItr == mCachedTeamIndividualUEDValuesMap.end())
        {
            TRACE_LOG("[MatchmakingInfoCache].sortCachedTeamIndivdualUEDVectors() unable to sort cached team individual UED values for rule " << uedRuleDefinition.getName());
            return;
        }

        for (size_t i = 0; i  < cacheItr->second.size(); ++i)
        {
           eastl::sort(cacheItr->second[i].begin(), cacheItr->second[i].end(), eastl::greater<UserExtendedDataValue>());
        }
    }

    void MatchmakingGameInfoCache::cacheTeamIndividualUEDValues(const TeamUEDPositionParityRuleDefinition &uedRuleDefinition, const Search::GameSessionSearchSlave& gameSession)
    {
        RuleDefinitionId ruleDefId = uedRuleDefinition.getID();
        UserExtendedDataKey dataKey = uedRuleDefinition.getUEDKey();
        TeamIndividualUEDValues& cachedTeamIndividualUEDVector = mCachedTeamIndividualUEDValuesMap[ruleDefId];
        cachedTeamIndividualUEDVector.clear();
        const uint16_t teamCount = gameSession.getTeamCount();
        if (teamCount > 0)
        {
            cachedTeamIndividualUEDVector.reserve(teamCount);

            // set up the team individual UED vectors
            for (TeamIndex teamIndex = 0; teamIndex < teamCount; ++teamIndex)
            {
                TeamUEDVector& newTeamIndividualUEDVector = cachedTeamIndividualUEDVector.push_back();
                // if these are empty, that's fine
                newTeamIndividualUEDVector.reserve(gameSession.getTeamCapacity());
            }

            // now build out the individual teams, just iterate over the player roster and pull the team index from each player
            const PlayerRoster::PlayerInfoList& playerList = gameSession.getPlayerRoster()->getPlayers(PlayerRoster::ROSTER_PARTICIPANTS);
            PlayerRoster::PlayerInfoList::const_iterator playerListIter = playerList.begin();
            PlayerRoster::PlayerInfoList::const_iterator playerListEnd = playerList.end();
            for (; playerListIter != playerListEnd; ++playerListIter)
            {
                UserExtendedDataValue dataValue;
                const PlayerInfo *playerInfo = *playerListIter;
                // We must check team index vs number of teams because game data changes are processed before player data changes, so for example
                // when the game gets reset the team count drops to 1 and the players are all removed; however, the game reset is processed before
                // the player removals. Therefore we must ensure that we do not attempt to access teams by stale team indexes of players that are about to be tossed.
                if (playerInfo->getTeamIndex() < teamCount)
                {
                    if (UserSession::getDataValue(playerInfo->getUserInfo().getDataMap(), dataKey, dataValue))
                    {
                        // put the user's UED value on his team's vector. We'll sort when actually evaluating later
                        cachedTeamIndividualUEDVector[playerInfo->getTeamIndex()].push_back(dataValue);
                        TRACE_LOG("[MatchmakingGameInfoCache].cacheTeamIndividualUEDValues() Team UED Rule '" << uedRuleDefinition.getName() 
                            << "', updated cached Team UEDs for game '" << gameSession.getGameId() << ", team '" << playerInfo->getTeamIndex() 
                            << "' added user with UED value of '" << dataValue << "'.");
                    }
                    else
                    {
                        WARN_LOG("[MatchmakingGameInfoCache].cacheTeamIndividualUEDValues() - Cannot get value for userSessionId '" << playerInfo->getUserInfo().getSessionId() << "' and dataKey '" << SbFormats::HexLower(dataKey) << "'.");
                    }
                }
            }
        }
    }

    void MatchmakingGameInfoCache::cacheTopologyHostGeoLocation(const Search::GameSessionSearchSlave& gameSession)
    {
        if (gameSession.getTopologyHostSessionExists())
        {
            mTopologyHostLocation.latitude = gameSession.getTopologyHostUserInfo().getLatitude();
            mTopologyHostLocation.longitude = gameSession.getTopologyHostUserInfo().getLongitude();
        }
        else
        {
            TRACE_LOG("[MatchmakingGameInfoCache] Unable to find topology host user session for game(" << gameSession.getGameId() << ") for setting latitude and longitude.");
        }
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
