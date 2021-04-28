/*! ************************************************************************************************/
/*!
    \file diagnosticsgamecountutils.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/diagnostics/diagnosticsgamecountutils.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameSessionSearchSlave in updateGameCountsCaches
#include "gamemanager/searchcomponent/searchslaveimpl.h" // for SearchSlaveImpl in getNumGamesVisibleForRuleConditions
#include "gamemanager/matchmaker/rules/playerattributeruledefinition.h" // for PlayerAttributeRuleDefinition in getArbitraryPlayerAttributesGameCountsCache
#include "gamemanager/matchmaker/rules/uedruledefinition.h"
#include "gamemanager/matchmaker/rules/modruledefinition.h"
#include "gamemanager/matchmaker/rules/rolesruledefinition.h"
#include "gamemanager/matchmaker/rules/playerslotutilizationruledefinition.h"
#include "gamemanager/matchmaker/rules/teaminfohelper.h"// for UNSET_TEAM_ID in updateTeamIdsGameCounts

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \param[in] partitions Number of range-partitioning buckets. All but possibly the tail bucket (which maybe larger)
    are same size. 0 or 1 uses 1 bucket.
    ***************************************************************************************************/
    bool GameCountsByBucketCache::initialize(const BucketPartitions& bucketPartitions, const char8_t* logContext)
    {
        mLogContext.sprintf("[GameCountsByBucketCache:%s]", ((logContext != nullptr) ? logContext : ""));

        if (!mBucketPartitions.initialize(bucketPartitions.getMinRange(), bucketPartitions.getMaxRange(),
            bucketPartitions.getPartitions(), logContext))
        {
            return false;
        }
        mCountsCache.resize(mBucketPartitions.getPartitions());
        return true;
    }

    bool GameCountsByBucketCache::validateInitialized() const
    {
        if (mBucketPartitions.getPartitions() != mCountsCache.size())
        {
            ERR_LOG(mLogContext << ".validateInitialized: numBuckets(" << mCountsCache.size() << 
                ") unexpectedly != numPartitions(" << mBucketPartitions.getPartitions() << "). Diagnostics may not be updated.");
            return false;
        }
        return true;
    }


    /*! ************************************************************************************************/
    /*! \brief update the tracked count for the value's bucket.
    ***************************************************************************************************/
    bool GameCountsByBucketCache::updateCount(int64_t gamePreBucketValue, bool increment)
    {
        if (!validateInitialized())
        {
            return false;
        }

        uint64_t bucketIndex = 0;
        if (!mBucketPartitions.calcBucketIndex(bucketIndex, gamePreBucketValue) || (EA_UNLIKELY(bucketIndex >= mCountsCache.size())))
        {
            return false;
        }
        if (!increment && (mCountsCache[bucketIndex] == 0))
        {
            ERR_LOG(mLogContext << ".updateCount: can't decrement count for bucketed val(" << gamePreBucketValue << ") below 0.");
            return false;
        }

        mCountsCache[bucketIndex] += (increment ? 1 : -1);
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief get the tracked counts for all buckets that contain values from the min to max value
    ***************************************************************************************************/
    uint64_t GameCountsByBucketCache::getCount(int64_t minPreBucketValue, int64_t maxPreBucketValue) const
    {
        if (!validateInitialized())
        {
            return false;
        }

        uint64_t minBucketIndex = 0, maxBucketIndex = 0;
        if (!mBucketPartitions.calcBucketIndex(minBucketIndex, minPreBucketValue) ||
            !mBucketPartitions.calcBucketIndex(maxBucketIndex, maxPreBucketValue) ||
            (minBucketIndex > maxBucketIndex || minBucketIndex > mCountsCache.size() || maxBucketIndex > mCountsCache.size()))
        {
            return 0;//logged
        }

        uint64_t count = 0;
        for (uint64_t i = minBucketIndex; i <= maxBucketIndex; ++i)
        {
            count += mCountsCache[i];
        }
        return count;
    }

    //  DiagnosticsSearchSlaveHelper

    /*! ************************************************************************************************/
    /*! \brief FG (RETE) rule diagnostics helper. Return count of games visible for rule's conditions.
        \note Depending on rule, this implementation may double or under count. Only use this fn appropriate.

        \param[in] ruleConditions Rule's list of OR lists. If there's multiple conditions in an OR list,
            sums the conditions' game counts. If there's multiple OR lists, returns min of those sums
    ***************************************************************************************************/
    uint64_t DiagnosticsSearchSlaveHelper::getNumGamesVisibleForRuleConditions(const char8_t* ruleName,
        const Rete::ConditionBlockList& ruleConditions) const
    {
        if (ruleName == nullptr)
        {
            ERR_LOG("[DiagnosticsSearchSlaveHelper].getNumGamesVisibleForRuleConditions: nullptr rule name. Returning 0.");
            return 0;
        }
        uint64_t numOrLists = 0;

        uint64_t resultGameCount = 0;
        const auto& conditionBlock = ruleConditions.at(Rete::CONDITION_BLOCK_BASE_SEARCH);
        for (auto& orList : conditionBlock)
        {
            uint64_t numConditions = 0;
            uint64_t currGameCount = 0;
            for (auto& cond : orList)
            {
                auto* providerName = ((cond.provider != nullptr) ? cond.provider->getProviderName() : nullptr);
                if ((providerName != nullptr) && (blaze_stricmp(ruleName, providerName) == 0))
                {
                    // if have multiple conditions in an OR list, sum their counts (assumes games can't match > 1
                    // condition for the rule, warn below)
                    currGameCount += mSearchSlaveImpl.getReteNetwork().getNumGamesVisibleForCondition(cond.condition);
                    ++numConditions;
                }
            }

            if (numConditions > 0)
            {
                ++numOrLists;

                // if numOrLists > 1, use min over all OR lists (assumes match is an 'AND' of them. Warn below on accuracy 
                // for some rules)
                if ((resultGameCount == 0) || (numOrLists > 1 && currGameCount < resultGameCount))
                {
                    resultGameCount = currGameCount;
                }
                if (numConditions > 1)
                {
                    TRACE_LOG("[DiagnosticsSearchSlaveHelper:" << ruleName << "].getNumGamesVisibleForRuleConditions: warning: " <<
                        numConditions << " conditions, in OR list detected. Depending on rule, having more than 1 could cause" <<
                        " less accurate diagnostics. To address this, rule may require custom handling.");
                }
            }
        }

        if (numOrLists > 1)
        {
            TRACE_LOG("[DiagnosticsSearchSlaveHelper:" << ruleName << "].getNumGamesVisibleForRuleConditions: warning: " <<
                numOrLists << " OR condition lists detected having more than 1 could cause less accurate diagnostics." <<
                " To address this, rule may require custom handling.");
        }

        return resultGameCount;
    }



    /*! ************************************************************************************************/
    /*! \brief update cached diagnostic counts, on game updates
    ***************************************************************************************************/
    void DiagnosticsSearchSlaveHelper::updateGameCountsCaches(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment)
    {
        if (!mSearchSlaveImpl.getMatchmakingConfig().getTrackDiagnostics())
        {
            return;
        }

        ASSERT_COND_LOG((increment || mTotalGamesCount != 0),
            "updateGameCountsCaches: internal error: can't decrement total games below 0, on game(" << gameSessionSlave.getGameId() <<
            ") update. Diagnostics may not be updated correctly");
        if (increment)
        {
            ++mTotalGamesCount;
        }
        else if (mTotalGamesCount != 0)
        {
            --mTotalGamesCount;
        }

        // by default, FG rules use RETE to count games visible. Track total in RETE for their match any cases
        if (mSearchSlaveImpl.isGameForRete(gameSessionSlave))
        {
            ASSERT_COND_LOG((increment || mReteGamesCount != 0),
                "updateGameCountsCaches: internal error: can't decrement MM games below 0, on game(" << gameSessionSlave.getGameId() <<
                ") update. Diagnostics may not be updated correctly");

            if (increment)
            {
                ++mReteGamesCount;
            }
            else if (mReteGamesCount != 0)
            {
                --mReteGamesCount;
            }
        }

        auto& ruleDefinitions = mSearchSlaveImpl.getMatchmakingSlave().getRuleDefinitionCollection().getRuleDefinitionList();
        for (auto& ruleDefinition : ruleDefinitions)
        {
            ruleDefinition->updateDiagnosticsGameCountsCache(*this, gameSessionSlave, increment);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Function that gets called when updating matchmaking diagnostics cached values
    *************************************************************************************************/
    void DiagnosticsSearchSlaveHelper::updateModsGameCounts(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment, const ModRuleDefinition& ruleDefinition)
    {
        uint64_t mods = static_cast<uint64_t>(gameSessionSlave.getGameModRegister());

        if (!mModsGameCounts.isInitialized())
        {
            mModsGameCounts.initialize(ruleDefinition.getName());
        }

        ModsGameCountsCache::CacheBitset bitset;
        bitset.from_uint64(mods);
        if (!mModsGameCounts.updateCount(bitset, increment))
        {
            ERR_LOG("[DiagnosticsSearchSlaveHelper].updateModsGameCounts: failed for game(" << gameSessionSlave.getGameId() << ")");
        }
    }

    const uint64_t DiagnosticsSearchSlaveHelper::getModsGameCount(uint32_t mods) const 
    {
        return mModsGameCounts.getCount(mods);
    }


    /*! ************************************************************************************************/
    /*! \brief Function that gets called when updating matchmaking diagnostics cached values
    *************************************************************************************************/
    void DiagnosticsSearchSlaveHelper::updateRolesGameCounts(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment, const RolesRuleDefinition& ruleDefinition)
    {
        if (!mRolesGameCountsCache.isInitialized())
        {
            mRolesGameCountsCache.initialize(ruleDefinition.getName());
        }

        RolesGameCountsCache::ValuesVector diagnosticVals;

        for (auto& roleIter : gameSessionSlave.getRoleInformation().getRoleCriteriaMap())
        {
            uint16_t roleSpace = 0;
            for (TeamIndex teamIndex = 0; ((teamIndex < gameSessionSlave.getTeamCount()) && (roleSpace == 0)); ++teamIndex)
            {
                // this basic diagnostic tracks as visible when role has at least 1 spot
                roleSpace = (gameSessionSlave.getRoleCapacity(roleIter.first) - gameSessionSlave.getPlayerRoster()->getRoleSize(teamIndex, roleIter.first));
                if (roleSpace > 0)
                {
                    diagnosticVals.push_back() = roleIter.first.c_str();

                    continue;
                }
            }
        }
        if (!diagnosticVals.empty())
        {
            mRolesGameCountsCache.updateCount(diagnosticVals, increment);
        }
    }

    uint64_t DiagnosticsSearchSlaveHelper::getRolesGameCount(const RolesSizeMap& desiredRoles) const
    {
        RolesGameCountsCache::ValuesVector diagnosticVals;
        diagnosticVals.reserve(desiredRoles.size());
        for (const auto& itr : desiredRoles)
        {
            for (const auto& role : itr.first)
            {
                // ANY role matches all games, so don't add any requirements to the search  
                if (blaze_stricmp(role.c_str(), PLAYER_ROLE_NAME_ANY) == 0)
                    continue;

                if (eastl::find(diagnosticVals.begin(), diagnosticVals.end(), role.c_str()) != diagnosticVals.end())
                    continue; // Skip duplicate roles

                diagnosticVals.push_back() = role.c_str();
            }
        }
        return mRolesGameCountsCache.getCount(diagnosticVals);
    }

    const TeamId DiagnosticsSearchSlaveHelper::DIAGNOSTICS_DUPLICATE_TEAMIDS_CACHE_KEY = INVALID_TEAM_ID;

    /*! ************************************************************************************************/
    /*! \brief Function that gets called when updating matchmaking diagnostics cached values
    *************************************************************************************************/
    void DiagnosticsSearchSlaveHelper::updateTeamIdsGameCounts(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment, const TeamChoiceRuleDefinition& ruleDefinition)
    {
        if (!mTeamIdsGameCounts.isInitialized())
        {
            mTeamIdsGameCounts.initialize(ruleDefinition.getName());
        }

        // this basic diagnostic tracks as visible when one is available (won't account for actual space needed for groups etc).
        // It uses all bits to track unset team ids, and a single bit for dupes (precise for team count 2 at least) 
        const auto& teamIds = const_cast<Search::GameSessionSearchSlave&>(gameSessionSlave).getMatchmakingGameInfoCache()->getCachedTeamIds();
        TeamIdsGameCountsCache::ValuesVector diagnosticVals;
        for (auto& team : teamIds)
        {
            if (team == UNSET_TEAM_ID)
            {
                auto unsetBits = TeamIdsGameCountsCache::ALL_BITS;
                mTeamIdsGameCounts.updateCount(unsetBits, increment);
                return;
            }
            // using a single bit to track dupes for simplicity
            if (eastl::find(diagnosticVals.begin(), diagnosticVals.end(), team) != diagnosticVals.end())
            {
                diagnosticVals.push_back(DIAGNOSTICS_DUPLICATE_TEAMIDS_CACHE_KEY);
                continue;
            }

            diagnosticVals.push_back(team);
        }
        if (!diagnosticVals.empty())
        {
            mTeamIdsGameCounts.updateCount(diagnosticVals, increment);
        }
    }

    uint64_t DiagnosticsSearchSlaveHelper::getTeamIdsGameCount(const TeamIdSizeList& desiredTeams) const
    {
        TeamIdsGameCountsCache::ValuesVector diagnosticVals;
        for (auto& team : desiredTeams)
        {
            // any team id doesn't need a bit set (matches any)
            if (team.first == ANY_TEAM_ID)
            {
                continue;
            }
            if (eastl::find(diagnosticVals.begin(), diagnosticVals.end(), team.first) != diagnosticVals.end())
            {
                diagnosticVals.push_back(DIAGNOSTICS_DUPLICATE_TEAMIDS_CACHE_KEY);
                continue;
            }

            diagnosticVals.push_back(team.first);
        }
        uint64_t count = mTeamIdsGameCounts.getCount(diagnosticVals);
        return count;
    }



    /*! ************************************************************************************************/
    /*! \brief Function that gets called when updating matchmaking diagnostics cached values
    *************************************************************************************************/
    void DiagnosticsSearchSlaveHelper::updatePlayerSlotUtilizationGameCounts(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment, const PlayerSlotUtilizationRuleDefinition& ruleDefinition)
    {
        if (!mPlayerSlotUtilizationGameCounts.isInitialized())
        {
            mPlayerSlotUtilizationGameCounts.initialize(ruleDefinition.getDiagnosticsBucketPartitions(), ruleDefinition.getName());
        }

        float valueFlt = (((float)gameSessionSlave.getPlayerRoster()->getParticipantCount()) /
            ((float)gameSessionSlave.getTotalParticipantCapacity()));

        uint64_t valuePct = (uint64_t)(valueFlt * 100);

        if (!mPlayerSlotUtilizationGameCounts.updateCount(valuePct, increment))
        {
            ERR_LOG("[DiagnosticsSearchSlaveHelper].updatePlayerSlotUtilizationGameCounts: failed for game(" << gameSessionSlave.getGameId() << "). This searchSlave may have missed some PlayerLeft/PlayerJoined notifications from the coreMaster, in which case it should be restarted to force it to fetch updated game data from redis.");
        }
    }

    uint64_t DiagnosticsSearchSlaveHelper::getPlayerSlotUtilizationGameCount(int64_t minPreBucketValue, int64_t maxPreBucketValue) const
    {
        return mPlayerSlotUtilizationGameCounts.getCount(minPreBucketValue, maxPreBucketValue);
    }

    /*! ************************************************************************************************/
    /*! \brief Function that gets called when updating matchmaking diagnostics cached values
    *************************************************************************************************/
    void DiagnosticsSearchSlaveHelper::updateUedGameCounts(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment, const UEDRuleDefinition& ruleDefinition)
    {
        auto uedValue = gameSessionSlave.getMatchmakingGameInfoCache()->getCachedGameUEDValue(ruleDefinition);
        if (uedValue == INVALID_USER_EXTENDED_DATA_VALUE)
        {
            return;
        }

        if (!getUEDGameCountsCache(ruleDefinition).updateCount(uedValue, increment))
        {
            ERR_LOG("[DiagnosticsSearchSlaveHelper].updateUedGameCounts: failed for game(" <<
                gameSessionSlave.getGameId() << "), with uedValue(" << uedValue << "). Rule(" <<
                (ruleDefinition.getName() != nullptr ? ruleDefinition.getName() : "<nullptr>") << ")");
        }
    }

    GameCountsByBucketCache& DiagnosticsSearchSlaveHelper::getUEDGameCountsCache(const UEDRuleDefinition& ruleDefinition)
    {
        UEDGameCountsCacheMap::iterator cacheItr = mUEDGameCounts.find(ruleDefinition.getName());
        if (cacheItr == mUEDGameCounts.end())
        {
            // create the cache
            cacheItr = mUEDGameCounts.insert(ruleDefinition.getName()).first;
            cacheItr->second.initialize(ruleDefinition.getDiagnosticBucketPartitions(), ruleDefinition.getName());
        }
        return cacheItr->second;
    }

    uint64_t DiagnosticsSearchSlaveHelper::getUEDGameCount(const char8_t* ruleName, int64_t minPreBucketValue, int64_t maxPreBucketValue) const
    {
        auto cacheItr = mUEDGameCounts.find(ruleName);
        return (cacheItr == mUEDGameCounts.end()) ? 0 : cacheItr->second.getCount(minPreBucketValue, maxPreBucketValue);
    }




    /*! ************************************************************************************************/
    /*! \brief Function that gets called when updating matchmaking diagnostics cached values
    *************************************************************************************************/
    void DiagnosticsSearchSlaveHelper::updatePlayerAttributesGameCounts(const Search::GameSessionSearchSlave& gameSessionSlave, bool increment, const PlayerAttributeRuleDefinition& ruleDefinition)
    {
        // Use game's cached player attributes' integers: For explicit rules the map key int is the poss value index.
        // For arbitrary rules, the value's wme attribute. See MatchmakingGameInfoCache::cachePlayerAttributeValues
        const auto *attribs = gameSessionSlave.getMatchmakingGameInfoCache()->getCachedPlayerAttributeValues(ruleDefinition);
        if (attribs == nullptr)
        {
            return;
        }

        PAttributeGameCountCache& cache = getPAttributeGameCountCache(ruleDefinition.getName());
        for (auto& itr : *attribs)
        {
            auto desiredAttribValueInt = itr.first;

            auto& count = cache[desiredAttribValueInt];
            if (!increment && (count == 0))
            {
                ERR_LOG("updatePlayerAttributesGameCounts: internal error: can't decrement attribute value(" << (ruleDefinition.getAttributeRuleType() == ARBITRARY_TYPE ?
                    ruleDefinition.reteUnHash(desiredAttribValueInt) : ruleDefinition.getPossibleValue(desiredAttribValueInt).c_str()) <<
                    ")'s game count below 0, on game(" << gameSessionSlave.getGameId() << ") update. Cannot update for rule(" <<
                    (ruleDefinition.getName() != nullptr ? ruleDefinition.getName() : "<nullptr>") << ")");
                continue;
            }

            count += (increment ? 1 : -1);
        }
    }

    uint64_t DiagnosticsSearchSlaveHelper::getPlayerAttributeGameCount(const char8_t* ruleName, uint64_t desiredAttribValueInt) const
    {
        auto cache = getPAttributeGameCountCache(ruleName);
        auto cacheItr = cache.find(desiredAttribValueInt);

        return (cacheItr != cache.end() ? cacheItr->second : 0);
    }

    DiagnosticsSearchSlaveHelper::PAttributeGameCountCache&
        DiagnosticsSearchSlaveHelper::getPAttributeGameCountCache(const char8_t* ruleName) const
    {
        PAttributeGameCountCacheMap::iterator cacheItr = mPAttributeGameCounts.find(ruleName);
        if (cacheItr == mPAttributeGameCounts.end())
        {
            cacheItr = mPAttributeGameCounts.insert(ruleName).first;
        }
        return cacheItr->second;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
