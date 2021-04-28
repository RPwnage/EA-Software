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

namespace Packer
{

namespace TeamPlayerPropertySumFactor
{

static const char8_t* INVALID_TEAM_COUNT_TEXT = "Number of teams too low for balancing.";
static const char8_t* CANT_EVICT_MORE_PLAYERS_TEXT = "Party chosen for eviction has more players than incoming.";
static const char8_t* CANT_EVICT_IMMUTABLE_TEXT = "Party chosen for eviction is immutable.";
static const char8_t* CANT_EVICT_MORE_THAN_ONE_TEXT = "Force eviction limit reached.";
static const char8_t* INCOMING_PARTY_MAX_VALUE_TOO_LOW = "Incoming party max property value too low.";

struct FactorTransform : public GameQualityTransform
{
    FactorTransform();
    Score activeEval(EvalContext& cxt, Game& game) override;
    Score passiveEval(EvalContext& cxt, const Game& game) const override;
    static TeamIndex assignPartyToLowestSumTeam(const PropertyValueLookup& lookup, GameInternalState& internalState, PropertyValue gameTeamPropSum[MAX_TEAM_COUNT], PartyIndex partyIndex, const Game& game);
    static const uint32_t MAX_PARTY_SIZE_COUNT = (MAX_PLAYER_COUNT / 2) - 1; // party of 0 not allowed, hence: - 1
    PartyBitset mPartyBitsetByPartySize[MAX_PARTY_SIZE_COUNT]; // MAYBE: #OPTIMIZATION We could track party sizes in the GameInternalState more efficiently, but no other factor uses this info now...
} gTransform;

FactorTransform::FactorTransform()
{
    GameQualityFactorSpec spec;

    spec.mTransformName = "sum";

    spec.mFactorTransform = this;
    spec.mFactorResultSize = 2;
    spec.mRequiresProperty = true;
    registerFactorSpec(spec);
}

Score FactorTransform::activeEval(EvalContext& cxt, Game& game)
{
    if (game.getTeamCount() < 2)
    {
        // If there is one team, or no team there is nothing to balance!
        cxt.setResultReason(INVALID_TEAM_COUNT_TEXT);
        return INVALID_SCORE;
    }

    Score score = INVALID_SCORE;
    const auto incomingPartyIndex = game.getIncomingPartyIndex();
    auto incomingPartyTeamIndex = game.getTeamIndexByPartyIndex(incomingPartyIndex, GameState::FUTURE);
    if (incomingPartyTeamIndex != INVALID_TEAM_INDEX) // TODO: we need to implement ability to reconcile additional pending evictions despite incoming party being assigned.
    {
        score = passiveEval(cxt, game);
    }
    else
    {
        auto lookup = game.getPropertyValueLookup(cxt);
        // need to handle pending party evictions
        // PACKER_TODO: we may want to precompute this somehow for the party
        const auto incomingPartyMaxPropValue = lookup.getPartyPropertyValue(incomingPartyIndex, PropertyAggregate::MAX);

        // iterate all *indexed* players in descending order by factor property index
        // for each player determine if his party is assigned to a future team, if not the party is evicted and can be skipped

        auto& propIndexer = game.getPlayerPropertyIndex(cxt);
        PartyBitset processedPartyBits;
        PartyBitset immutableProcessedPartyBits;
        GameInternalState gameState;
        GameInternalState immutableGameState;

        PropertyValue gameTeamPropSum[MAX_TEAM_COUNT] = {};
        PropertyValue immutableGameTeamPropSum[MAX_TEAM_COUNT] = {};
        auto forceEvictCount = 0;

        eastl::bitset<MAX_PARTY_SIZE_COUNT> partySizeOccupancyBits;

        uint32_t maxPartySize = 1; // PACKER_TODO: this can be computed once and stored in the game, this would ensure we don't have to do this most of the time...
        for (PartyIndex partyIndex = 0; partyIndex < incomingPartyIndex; ++partyIndex)
        {
            auto& gameParty = game.mGameParties[partyIndex];
            auto playerCount = gameParty.mPlayerCount;
            if (gameParty.mImmutable)
            {
                // PACKER_TODO: #OPTIMIZATION Looking this function in really highlights why its so important to avoid unnecessary indirection in performance critical code. The ugly reality is that this function is actually determining party index by iterating all the player info sorted in order, but then it goes back to find out which players are associated with which party here. That would be fine, except that vast majority of parties have only one player, which means that we already have the player we need and thus don't need to do this step at all!
                const auto propertyValue = lookup.getPartyPropertyValue(partyIndex, PropertyAggregate::SUM);

                // PACKER_TODO: should be able to switch GameState::PRESENT -> cxt.mGameState here, since all PRESENT->FUTURE state copy always occurs on first factor eval...
                const auto existingPartyTeamIndex = game.getTeamIndexByPartyIndex(partyIndex, GameState::PRESENT);
                EA_ASSERT(existingPartyTeamIndex >= 0);
                immutableGameTeamPropSum[existingPartyTeamIndex] += propertyValue;
                immutableGameState.assignPartyToGame(partyIndex, playerCount);
                immutableGameState.assignPartyToTeam(partyIndex, playerCount, existingPartyTeamIndex);
            }

            auto sizeBitIndex = playerCount - 1;
            if (!partySizeOccupancyBits.test(sizeBitIndex))
            {
                partySizeOccupancyBits.set(sizeBitIndex);
                mPartyBitsetByPartySize[sizeBitIndex].reset();

                if (maxPartySize < playerCount)
                    maxPartySize = (uint32_t)playerCount;
            }

            mPartyBitsetByPartySize[sizeBitIndex].set(partyIndex); // add party to bitset
        }
        // Assign highest to lowest parties as ordered by propIndexer, slotting in the incoming party, at its appropriate spot in the order
        do
        {
            uint32_t evictedParties = 0;
            uint32_t evictedPlayers = 0;
            gameState = immutableGameState;
            memcpy(gameTeamPropSum, immutableGameTeamPropSum, sizeof(gameTeamPropSum));
            processedPartyBits |= immutableGameState.mPartyBits;

            if (maxPartySize == 1)
            {
                // this means all parties are of size 1, do the usual stuff
                // index is always ascending, therefore we must iterate it in reverse order
                for (const PlayerIndex playerIndex : eastl::reverse(propIndexer))
                {
                    const auto& gamePlayer = game.mGamePlayers[playerIndex];
                    EA_ASSERT(gamePlayer.mGamePartyIndex >= 0 && gamePlayer.mGamePartyIndex < incomingPartyIndex);
                    if (processedPartyBits.test(gamePlayer.mGamePartyIndex))
                        continue; // process each party only once

                    const auto existingPartyTeamIndex = game.getTeamIndexByPartyIndex(gamePlayer.mGamePartyIndex, GameState::FUTURE);
                    if (existingPartyTeamIndex >= 0)
                    {
                        // put incoming party in at its spot in the order
                        if ((incomingPartyTeamIndex == INVALID_TEAM_INDEX) &&
                            (incomingPartyMaxPropValue >= lookup.getPlayerPropertyValue(playerIndex)))
                        {
                            incomingPartyTeamIndex = assignPartyToLowestSumTeam(lookup, gameState, gameTeamPropSum, incomingPartyIndex, game);
                            if (incomingPartyTeamIndex == INVALID_TEAM_INDEX)
                                goto finish; // couldn't assign incoming party   //TODO: note: this early out is part of temp workaround until we can access per-player properties in packer (for now, all assigned same value as their party)
                        }

                        auto existingPartyNewTeamIndex = assignPartyToLowestSumTeam(lookup, gameState, gameTeamPropSum, gamePlayer.mGamePartyIndex, game);
                        if (existingPartyNewTeamIndex == INVALID_TEAM_INDEX)
                        {
                            if (evictedPlayers + game.mGameParties[gamePlayer.mGamePartyIndex].mPlayerCount <= game.mGameParties[incomingPartyIndex].mPlayerCount)
                            {
                                evictedPlayers += (uint32_t) game.mGameParties[gamePlayer.mGamePartyIndex].mPlayerCount;
                                evictedParties++;
                            }
                            else
                            {
                                incomingPartyTeamIndex = INVALID_TEAM_INDEX; // failed to assign enough of the remaining parties, cancel the assignment
                                goto finish;
                            }
                        }
                    }
                    else
                    {
                        evictedPlayers += (uint32_t) game.mGameParties[gamePlayer.mGamePartyIndex].mPlayerCount;
                        evictedParties++;
                    }
                    processedPartyBits.set(gamePlayer.mGamePartyIndex);
                    if ((PartyIndex) processedPartyBits.count() == incomingPartyIndex)
                        goto finish; // bail once we processed all the parties since all subsequent players will belong to an existing party
                }
            }
            else
            {
                // At least one(including incoming) party's size is > 1, process parties in descending order of size to improve chance of maximally utilizing the team space in the game (using size heuristic, because optimal bin-packing problem is NP-hard)
                for (uint32_t partySize = maxPartySize; partySize > 0; --partySize)
                {
                    auto partySizeBit = partySize - 1;
                    if (!partySizeOccupancyBits.test(partySizeBit))
                        continue;

                    auto& partyBitset = mPartyBitsetByPartySize[partySizeBit];
                    // now walk the player bits in property indexer order and do the usual processing
                    // index is always ascending, therefore we must iterate it in reverse order
                    for (const PlayerIndex playerIndex : eastl::reverse(propIndexer))
                    {
                        const auto& gamePlayer = game.mGamePlayers[playerIndex];
                        EA_ASSERT(gamePlayer.mGamePartyIndex >= 0 && gamePlayer.mGamePartyIndex < incomingPartyIndex);
                        if (!partyBitset.test(gamePlayer.mGamePartyIndex))
                            continue;

                        if (processedPartyBits.test(gamePlayer.mGamePartyIndex))
                            continue; // process each party only once

                        const auto existingPartyTeamIndex = game.getTeamIndexByPartyIndex(gamePlayer.mGamePartyIndex, GameState::FUTURE);
                        if (existingPartyTeamIndex >= 0)
                        {
                            // put incoming party in at its spot in the order, assuming it's replacing a party of the same size
                            if ((incomingPartyTeamIndex == INVALID_TEAM_INDEX) &&
                                (incomingPartyMaxPropValue >= lookup.getPlayerPropertyValue(playerIndex)) &&
                                (game.mGameParties[gamePlayer.mGamePartyIndex].mPlayerCount <= game.mGameParties[incomingPartyIndex].mPlayerCount))
                            {
                                incomingPartyTeamIndex = assignPartyToLowestSumTeam(lookup, gameState, gameTeamPropSum, incomingPartyIndex, game);
                                if (incomingPartyTeamIndex == INVALID_TEAM_INDEX)
                                {
                                    //EA_ASSERT(partySize == 1);
                                    goto finish; // couldn't assign incoming party   //TODO: note: this early out is part of temp workaround until we can access per-player properties in packer (for now, all assigned same value as their party)
                                }
                            }

                            auto existingPartyNewTeamIndex = assignPartyToLowestSumTeam(lookup, gameState, gameTeamPropSum, gamePlayer.mGamePartyIndex, game);
                            if (existingPartyNewTeamIndex == INVALID_TEAM_INDEX)
                            {
                                if (evictedPlayers + game.mGameParties[gamePlayer.mGamePartyIndex].mPlayerCount <= game.mGameParties[incomingPartyIndex].mPlayerCount)
                                {
                                    evictedPlayers += (uint32_t) game.mGameParties[gamePlayer.mGamePartyIndex].mPlayerCount;
                                    evictedParties++;
                                }
                                else
                                {
                                    incomingPartyTeamIndex = INVALID_TEAM_INDEX; // failed to assign enough of the remaining partys, cancel the assignment
                                    //EA_ASSERT(partySize == 1);
                                    goto finish;
                                }
                            }
                        }
                        else
                        {
                            evictedPlayers += (uint32_t) game.mGameParties[gamePlayer.mGamePartyIndex].mPlayerCount;
                            evictedParties++;
                        }
                        processedPartyBits.set(gamePlayer.mGamePartyIndex);
                        if ((PartyIndex) processedPartyBits.count() == incomingPartyIndex)
                        {
                            goto finish; // bail once we processed all the parties since all subsequent players will belong to an existing party
                        }
                    }
                }
            }
        finish:
            if (incomingPartyTeamIndex == INVALID_TEAM_INDEX && (int32_t)processedPartyBits.count() == incomingPartyIndex)
            {
                // processed all parties but havent assigned a team to incoming
                incomingPartyTeamIndex = assignPartyToLowestSumTeam(lookup, gameState, gameTeamPropSum, incomingPartyIndex, game);
            }

            if (incomingPartyTeamIndex != INVALID_TEAM_INDEX)
            {
                EA_ASSERT(gameState.mPlayerCount <= gameState.mGameTeams[0].mPlayerCount + gameState.mGameTeams[1].mPlayerCount);

                // accept computed game teams as future teams
                game.copyState(GameState::FUTURE, gameState);
                uint32_t highSumIndex = 0;
                uint32_t lowSumIndex = 1;
                static_assert(MAX_TEAM_COUNT == 2, "Only 2 teams supported right now!");
                if (gameTeamPropSum[lowSumIndex] > gameTeamPropSum[highSumIndex])
                {
                    highSumIndex = 1;
                    lowSumIndex = 0;
                }
                cxt.mEvalResult = { (Score)gameTeamPropSum[highSumIndex], (Score)gameTeamPropSum[lowSumIndex] };
                score = cxt.computeScore();
                break; // match
            }

            if (forceEvictCount > 0)
            {
                cxt.setResultReason(CANT_EVICT_MORE_THAN_ONE_TEXT);
                break; // already tried force evict, no match
            }

            if (incomingPartyMaxPropValue >= lookup.getPlayerPropertyValue(propIndexer.front()))
            {
                cxt.setResultReason(INCOMING_PARTY_MAX_VALUE_TOO_LOW);
                break; // incoming party max prop value is not the smallest, no other options
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

            if (game.mGameParties[forceEvictPartyIndex].mPlayerCount != game.mGameParties[incomingPartyIndex].mPlayerCount) // only try this if evicting same sized party, q: should we try if evicted party has fewer players?
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
    if (game.getTeamCount() < 2)
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
    int32_t highSumIndex = 0;
    int32_t lowSumIndex = 1;
    static_assert(MAX_TEAM_COUNT == 2, "Only 2 teams supported right now!");
    if (gameTeamPropSum[lowSumIndex] > gameTeamPropSum[highSumIndex])
    {
        highSumIndex = 1;
        lowSumIndex = 0;
    }
    cxt.mEvalResult = { (Score)gameTeamPropSum[highSumIndex], (Score)gameTeamPropSum[lowSumIndex] };
    return cxt.computeScore();
}

TeamIndex FactorTransform::assignPartyToLowestSumTeam(const PropertyValueLookup& lookup, GameInternalState& internalState, PropertyValue gameTeamPropSum[MAX_TEAM_COUNT], PartyIndex partyIndex, const Game& game)
{
    TeamIndex lesserTeam = 0;
    TeamIndex greaterTeam = 1;
    if (gameTeamPropSum[lesserTeam] > gameTeamPropSum[greaterTeam])
    {
        lesserTeam = 1;
        greaterTeam = 0;
    }

    const int32_t partyPlayerCount = (int32_t)game.mGameParties[partyIndex].mPlayerCount;
    int32_t lesserTeamPlayerCount = internalState.mGameTeams[lesserTeam].mPlayerCount;
    TeamIndex selectedTeam = INVALID_TEAM_INDEX;
    auto teamCapacity = game.getTeamCapacity(); // PACKER_TODO: this never changes for a given game template, so we need to look this up once outside of any party iteration.
    if (lesserTeamPlayerCount + partyPlayerCount <= teamCapacity)
    {
        selectedTeam = lesserTeam;
        //if (extra) printf("party(%" PRId64 "), players(%d) to team(%d), t0=%" PRId64 ", t1=%" PRId64 "\n", game.mGameParties[partyIndex].mPartyId, partyPlayerCount, selectedTeam, gameTeamPropSum[0], gameTeamPropSum[1]);
    }
    else
    {
        int32_t greaterTeamPlayerCount = internalState.mGameTeams[greaterTeam].mPlayerCount;
        if (greaterTeamPlayerCount + partyPlayerCount <= teamCapacity) {
            selectedTeam = greaterTeam; // PACKER_TODO: #ALGORITHM, This behavior is misleading at best and wrong at worst since it conflicts with the purpose of this function (assign to lowest sum team). The reason it is required here is the overriding interest of assigning the party where it may fit into a game rather that optimizing the team skill sum factor specifically. This is likely why deferring hard team assignment (ala Karmarkar-Karp algorithm) until after the game is filled(which is usually the most important condition) may very well be in our interest, because it avoids the necessity of trying to assign some team to a party in the interest of filling out the game (while at the detriment to the size of the game).
            //if (extra) printf("party(%" PRId64 "), players(%d) to team(%d), t0=%" PRId64 ", t1=%" PRId64 "\n", game.mGameParties[partyIndex].mPartyId, partyPlayerCount, selectedTeam, gameTeamPropSum[0], gameTeamPropSum[1]);
        }
        else
        {
            //if (extra) printf("party(%" PRId64 "), players(%d) could not fit into team, t0=%" PRId64 ", t1=%" PRId64 "\n", game.mGameParties[partyIndex].mPartyId, partyPlayerCount, gameTeamPropSum[0], gameTeamPropSum[1]);
            return INVALID_TEAM_INDEX;
        }
    }
    // PACKER_TODO: #OPTIMIZATION Looking at the call site of this function in really highlights why its so important to avoid unnecessary indirection in performance critical code. The ugly reality is that this function is actually determining party index by iterating all the player info sorted in order, but then it goes back to find out which players are associated with which party here. That would be fine, except that vast majority of parties have only one player, which means that we already have the player we need and thus don't need to do this step at all!
    const auto propertyValue = lookup.getPartyPropertyValue(partyIndex, PropertyAggregate::SUM);
    gameTeamPropSum[selectedTeam] += propertyValue;
    internalState.assignPartyToGame(partyIndex, partyPlayerCount);
    internalState.assignPartyToTeam(partyIndex, partyPlayerCount, selectedTeam);
    return selectedTeam;
}

}

} // GamePacker
