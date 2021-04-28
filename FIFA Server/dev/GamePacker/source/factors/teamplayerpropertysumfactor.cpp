/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include <EASTL/bonus/adaptors.h>
#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/game.h"
#include "gamepacker/packer.h"

namespace Packer
{

namespace TeamPlayerPropertySumFactor
{

static const char8_t* INVALID_TEAM_COUNT_TEXT = "Number of teams too low for balancing.";
static const char8_t* CANT_EVICT_MORE_PLAYERS_TEXT = "Party chosen for eviction has more players than incoming.";
static const char8_t* CANT_EVICT_IMMUTABLE_TEXT = "Party chosen for eviction is immutable.";
static const char8_t* CANT_EVICT_MORE_THAN_ONE_TEXT = "Force eviction limit reached.";
static const char8_t* INCOMING_PARTY_MAX_VALUE_TOO_HIGH = "Incoming party max property value too high.";

struct FactorTransform : public GameQualityTransform
{
    FactorTransform();
    Score activeEval(EvalContext& cxt, Game& game) override;
    Score passiveEval(EvalContext& cxt, const Game& game) const override;
    static TeamIndex assignPartyToLowestSumTeam(const PropertyValueLookup& lookup, GameInternalState& internalState, PropertyValue gameTeamPropSum[MAX_TEAM_COUNT], PartyIndex partyIndex, int32_t partyPlayerCount, int32_t teamCapacity, int32_t teamCount, int32_t maxTeamCount);
    static eastl::pair<TeamIndex, TeamIndex> getMinMaxSumIndexRange(PropertyValue gameTeamPropSum[MAX_TEAM_COUNT], int32_t teamCount);
} gTransform;

FactorTransform::FactorTransform()
{
    GameQualityFactorSpec spec;

    spec.mTransformName = "sum";

    spec.mFactorTransform = this;
    spec.mFactorResultSize = 2;
    spec.mRequiresProperty = true;
    spec.mRequiresPropertyFieldValues = true; // unset property field values are treated as 0
    registerFactorSpec(spec);
}

Score FactorTransform::activeEval(EvalContext& cxt, Game& game)
{
    const auto teamCount = game.getTeamCount();
    if (teamCount < MULTI_TEAM_MIN_TEAM_COUNT)
    {
        // If there is one team, or no team there is nothing to balance!
        cxt.setResultReason(INVALID_TEAM_COUNT_TEXT);
        return INVALID_SCORE;
    }

    struct {
        PartyIndex partyIndex;
        TeamIndex teamIndex;
        int32_t playerCount;
        PropertyValue maxPropValue = 0;
    } incomingParty;

    incomingParty.partyIndex = game.getIncomingPartyIndex();
    incomingParty.playerCount = game.mGameParties[incomingParty.partyIndex].mPlayerCount;
    incomingParty.teamIndex = game.getTeamIndexByPartyIndex(incomingParty.partyIndex, cxt.mGameState);

    Score score = INVALID_SCORE;

    if (incomingParty.teamIndex != INVALID_TEAM_INDEX) // TODO: we need to implement ability to reconcile additional pending evictions despite incoming party being assigned.
    {
        score = passiveEval(cxt, game);
    }
    else
    {
        auto lookup = game.getPropertyValueLookup(cxt);
        // need to handle pending party evictions
        // PACKER_TODO: we want to precompute this for the party
        incomingParty.maxPropValue = lookup.getPartyPropertyValue(incomingParty.partyIndex, PropertyAggregate::MAX);

        // iterate all *indexed* players in descending order by factor property index
        // for each player determine if his party is assigned to a future team, if not the party is evicted and can be skipped

        PartyBitset processedPartyBits;
        GameInternalState gameState;
        GameInternalState immutableGameState;

        PropertyValue gameTeamPropSum[MAX_TEAM_COUNT] = {};
        PropertyValue immutableGameTeamPropSum[MAX_TEAM_COUNT] = {};

        PartySizeOccupancyBitset partySizeOccupancyBits;
        calculateMutablePartySizeOccupancyBits(partySizeOccupancyBits, game, incomingParty.partyIndex);

        if (game.mImmutable)
        {
            for (PartyIndex partyIndex = 0; partyIndex < incomingParty.partyIndex; ++partyIndex)
            {
                auto& gameParty = game.mGameParties[partyIndex];
                if (gameParty.mImmutable)
                {
                    // PACKER_TODO: #OPTIMIZATION Looking this function in really highlights why its so important to avoid unnecessary indirection in performance critical code. The ugly reality is that this function is actually determining party index by iterating all the player info sorted in order, but then it goes back to find out which players are associated with which party here. That would be fine, except that vast majority of parties have only one player, which means that we already have the player we need and thus don't need to do this step at all!
                    const auto propertyValue = lookup.getPartyPropertyValue(partyIndex, PropertyAggregate::SUM);

                    // PACKER_TODO: should be able to switch GameState::PRESENT -> cxt.mGameState here, since all PRESENT->FUTURE state copy always occurs on first factor eval...
                    const auto existingPartyTeamIndex = game.getTeamIndexByPartyIndex(partyIndex, GameState::PRESENT);
                    EA_ASSERT(existingPartyTeamIndex != INVALID_TEAM_INDEX);
                    const auto playerCount = gameParty.mPlayerCount;
                    immutableGameTeamPropSum[existingPartyTeamIndex] += propertyValue;
                    immutableGameState.assignPartyToGame(partyIndex, playerCount);
                    immutableGameState.assignPartyToTeam(partyIndex, playerCount, existingPartyTeamIndex);
                }
            }
        }

        const auto teamCapacity = game.getTeamCapacity();
        auto preferredTeamCount = teamCount;

        if (game.getGameTemplate().mInterFactorSettings.mTeamFillIncremental)
        {
            // determine the 'estimated number of teams we 'should' be able to fill with the players in this game right now
            const auto estimatedMaxPlayerCount = game.getPlayerCount(cxt.mGameState);
            if (estimatedMaxPlayerCount <= teamCapacity)
                preferredTeamCount = 1;
            else if (estimatedMaxPlayerCount >= teamCapacity * teamCount)
                preferredTeamCount = teamCount;
            else
                preferredTeamCount = estimatedMaxPlayerCount / teamCapacity;
        }

        auto forceEvictCount = 0;
        auto& propIndexer = game.getPlayerPropertyIndex(cxt);
        // Assign highest to lowest parties as ordered by propIndexer, slotting in the incoming party, at its appropriate spot in the order
        do
        {
            int32_t evictedParties = 0;
            int32_t evictedPlayers = 0;
            gameState = immutableGameState;
            memcpy(gameTeamPropSum, immutableGameTeamPropSum, sizeof(gameTeamPropSum));
            processedPartyBits |= immutableGameState.mPartyBits;

            // process parties in descending order of size to improve chance of maximally utilizing the team space in the game (using size heuristic, because optimal bin-packing problem is NP-hard)
            for (auto partySizeBit = partySizeOccupancyBits.find_last(), partySize = partySizeOccupancyBits.size(); partySizeBit != partySize; partySizeBit = partySizeOccupancyBits.find_prev(partySizeBit))
            {
                auto& partyBitset = mPartyBitsetByPartySize[partySizeBit];
                // now walk the player bits in property indexer order and do the usual processing
                // index is always ascending, therefore we must iterate it in reverse order
                for (const PlayerIndex playerIndex : eastl::reverse(propIndexer))
                {
                    const auto& gamePlayer = game.mGamePlayers[playerIndex];
                    EA_ASSERT(gamePlayer.mGamePartyIndex >= 0 && gamePlayer.mGamePartyIndex < incomingParty.partyIndex);
                    if (!partyBitset.test(gamePlayer.mGamePartyIndex))
                        continue;

                    if (processedPartyBits.test(gamePlayer.mGamePartyIndex))
                        continue; // process each party only once

                    struct {
                        int32_t playerCount;
                        TeamIndex teamIndex;
                    } currentParty;
                    currentParty.playerCount = game.mGameParties[gamePlayer.mGamePartyIndex].mPlayerCount;
                    currentParty.teamIndex = game.getTeamIndexByPartyIndex(gamePlayer.mGamePartyIndex, cxt.mGameState);
                    if (currentParty.teamIndex != INVALID_TEAM_INDEX)
                    {
                        // put incoming party in at its spot in the order, assuming it's replacing a party of the same size
                        if ((incomingParty.teamIndex == INVALID_TEAM_INDEX) &&
                            (incomingParty.maxPropValue >= lookup.getPlayerPropertyValue(playerIndex)) &&
                            (incomingParty.playerCount >= currentParty.playerCount))
                        {
                            incomingParty.teamIndex = assignPartyToLowestSumTeam(lookup, gameState, gameTeamPropSum, incomingParty.partyIndex, incomingParty.playerCount, teamCapacity, preferredTeamCount, teamCount);
                            if (incomingParty.teamIndex == INVALID_TEAM_INDEX)
                            {
                                goto finish; // couldn't assign incoming party   //TODO: note: this early out is part of temp workaround until we can access per-player properties in packer (for now, all assigned same value as their party)
                            }
                        }
                        
                        // reassign current party instead
                        currentParty.teamIndex = assignPartyToLowestSumTeam(lookup, gameState, gameTeamPropSum, gamePlayer.mGamePartyIndex, currentParty.playerCount, teamCapacity, preferredTeamCount, teamCount);
                        if (currentParty.teamIndex == INVALID_TEAM_INDEX)
                        {
                            if (evictedPlayers + currentParty.playerCount <= incomingParty.playerCount)
                            {
                                evictedPlayers += (uint32_t) currentParty.playerCount;
                                evictedParties++;
                            }
                            else
                            {
                                incomingParty.teamIndex = INVALID_TEAM_INDEX; // failed to assign enough of the remaining partys, cancel the assignment
                                goto finish;
                            }
                        }
                    }
                    else
                    {
                        evictedPlayers += (uint32_t) currentParty.playerCount;
                        evictedParties++;
                    }
                    processedPartyBits.set(gamePlayer.mGamePartyIndex);
                    if ((PartyIndex) processedPartyBits.count() == incomingParty.partyIndex)
                    {
                        goto finish; // bail once we processed all the parties since all subsequent players will belong to an existing party
                    }
                }
            }
        finish:
            if ((incomingParty.teamIndex == INVALID_TEAM_INDEX) && 
                ((int32_t)processedPartyBits.count() == incomingParty.partyIndex))
            {
                // processed all parties but havent assigned a team to incoming
                incomingParty.teamIndex = assignPartyToLowestSumTeam(lookup, gameState, gameTeamPropSum, incomingParty.partyIndex, incomingParty.playerCount, teamCapacity, preferredTeamCount, teamCount);
            }

            if (incomingParty.teamIndex != INVALID_TEAM_INDEX)
            {
                EA_ASSERT(gameState.mPlayerCount <= gameState.getPlayersAssignedToTeams(teamCount));

                // accept computed future state
                game.copyState(GameState::FUTURE, gameState);
                auto range = getMinMaxSumIndexRange(gameTeamPropSum, teamCount);
                cxt.mEvalResult = { (Score)gameTeamPropSum[range.second], (Score)gameTeamPropSum[range.first] };
                score = cxt.computeScore();
                break; // match
            }

            if (forceEvictCount > 0)
            {
                cxt.setResultReason(CANT_EVICT_MORE_THAN_ONE_TEXT);
                break; // already tried force evict, no match
            }

            const auto memberPartyMinPropValue = lookup.getPlayerPropertyValue(propIndexer.front());
            if (incomingParty.maxPropValue >= memberPartyMinPropValue)
            {
                cxt.setResultReason(INCOMING_PARTY_MAX_VALUE_TOO_HIGH);
                break; // incoming party max prop value is not the smallest, we cannot try evicting smallest party in its favor, and we already compared incoming to greatest value in game, so no other options
            }

            // Edge case: If incoming party max prop value is smallest, and wouldn't fit at the end, try removing/replacing list's 1st party instead, to see if we can get a better score
            auto maxPlayerIndex = propIndexer.back();
            auto forceEvictPartyIndex = game.mGamePlayers[maxPlayerIndex].mGamePartyIndex;
            EA_ASSERT_MSG(forceEvictPartyIndex != INVALID_PARTY_INDEX, "Invalid party index!");
                
            if (game.mGameParties[forceEvictPartyIndex].mImmutable)
            {
                cxt.setResultReason(CANT_EVICT_IMMUTABLE_TEXT);
                break; // cannot evict immutable party
            }

            if (game.mGameParties[forceEvictPartyIndex].mPlayerCount != game.mGameParties[incomingParty.partyIndex].mPlayerCount) // only try this if evicting same sized party, q: should we try if evicted party has fewer players?
            {
                cxt.setResultReason(CANT_EVICT_MORE_PLAYERS_TEXT);
                break; // can't evict party with different number of members than incoming party
            }

            // retry once after force evicting first party
            gameState.clear();
            memset(gameTeamPropSum, 0, sizeof(gameTeamPropSum));
            processedPartyBits.reset(); // retry after omitting force evicted party
            processedPartyBits.set(forceEvictPartyIndex);
            ++forceEvictCount;

        } while (forceEvictCount == 1); // max 1 iteration
    }

    return score;
}

Score FactorTransform::passiveEval(EvalContext& cxt, const Game& game) const
{
    if (game.getTeamCount() < MULTI_TEAM_MIN_TEAM_COUNT)
    {
        // If there is one team, or no team there is nothing to balance!
        cxt.setResultReason(INVALID_TEAM_COUNT_TEXT);
        return INVALID_SCORE;
    }

    PropertyValue gameTeamPropSum[MAX_TEAM_COUNT] = {};
    auto lookup = game.getPropertyValueLookup(cxt);
    auto& gameState = game.getStateByContext(cxt);
    // IMPORTANT: This factor cannot blindly use gameState.mPlayerCount to iterate players and sum their property values because some players may be pending eviction (due to their parties being evicted)
    // also note, gameState.mPlayerCount tracks the actual number of players that are currently assigned to the current game state. In order to iterate just the player array, we would need to create an additional
    // method to count the current size of the game.mGamePlayers array, which in a GameState::FUTURE state can easily exceed gameState.mPlayerCount!
    // This double iteration can be simpified/optimized into a flat iteration over players when we change the game.mGamePlayers to be a vector, whose size is known independently of game.mPlayerCount.
    for (auto partyIndex = gameState.mPartyBits.find_first(), partySize = gameState.mPartyBits.size(); partyIndex != partySize; partyIndex = gameState.mPartyBits.find_next(partyIndex))
    {
        auto teamIndex = gameState.mTeamIndexByParty[partyIndex];
        if (teamIndex != INVALID_TEAM_INDEX)
        {
            const auto& gameParty = game.mGameParties[partyIndex];
            for (uint16_t playerIndex = gameParty.mPlayerOffset, playerIndexEnd = gameParty.mPlayerOffset + gameParty.mPlayerCount; playerIndex < playerIndexEnd; ++playerIndex)
            {
                gameTeamPropSum[teamIndex] += lookup.getPlayerPropertyValue(playerIndex);
            }
        }
    }
    auto range = getMinMaxSumIndexRange(gameTeamPropSum, game.getTeamCount());
    cxt.mEvalResult = { (Score)gameTeamPropSum[range.second], (Score)gameTeamPropSum[range.first] };
    return cxt.computeScore();
}

TeamIndex FactorTransform::assignPartyToLowestSumTeam(const PropertyValueLookup& lookup, GameInternalState& internalState, PropertyValue gameTeamPropSum[MAX_TEAM_COUNT], PartyIndex partyIndex, int32_t partyPlayerCount, int32_t teamCapacity, int32_t teamCount, int32_t maxTeamCount)
{
    EA_ASSERT(teamCapacity >= partyPlayerCount);
    for (TeamIndex minSumTeamIndex = 0; minSumTeamIndex < teamCount; ++minSumTeamIndex)
    {
        if (internalState.mPlayerCountByTeam[minSumTeamIndex] + partyPlayerCount <= teamCapacity)
        {
            auto minSum = gameTeamPropSum[minSumTeamIndex];
            // determine if this is the lowest sum team that can accomodate this party
            for (TeamIndex i = minSumTeamIndex + 1; i < teamCount; ++i)
            {
                if ((internalState.mPlayerCountByTeam[i] + partyPlayerCount <= teamCapacity) &&
                    (gameTeamPropSum[i] < minSum))
                {
                    minSumTeamIndex = i;
                    minSum = gameTeamPropSum[i];
                }
            }
            const auto propertyValue = lookup.getPartyPropertyValue(partyIndex, PropertyAggregate::SUM);
            gameTeamPropSum[minSumTeamIndex] += propertyValue;
            internalState.assignPartyToGame(partyIndex, partyPlayerCount);
            internalState.assignPartyToTeam(partyIndex, partyPlayerCount, minSumTeamIndex);
            return minSumTeamIndex;
        }
    }

    if (teamCount < maxTeamCount)
    {
        // try one more time but with teamcount increased by 1
        return assignPartyToLowestSumTeam(lookup, internalState, gameTeamPropSum, partyIndex, partyPlayerCount, teamCapacity, teamCount + 1, maxTeamCount);
    }

    return INVALID_TEAM_INDEX;
}

eastl::pair<TeamIndex, TeamIndex> FactorTransform::getMinMaxSumIndexRange(PropertyValue gameTeamPropSum[MAX_TEAM_COUNT], int32_t teamCount)
{
    if (teamCount == 1)
        return eastl::pair<TeamIndex, TeamIndex>((TeamIndex)0, (TeamIndex)0);

    eastl::pair<TeamIndex, TeamIndex> range((TeamIndex)0, (TeamIndex)1);

    for (TeamIndex i = 0; i < teamCount; ++i)
    {
        auto value = gameTeamPropSum[i];
        if (value > gameTeamPropSum[range.second])
            range.second = i;
        else if (value < gameTeamPropSum[range.first])
            range.first = i;
    }

    return range;
}

}

} // GamePacker
