/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/game.h"

// factor description: Attempts to minimize the number of partially filled teams in the game, while assigning all possible parties.

namespace Packer
{


namespace TeamFillFactor
{

static const char8_t* NO_TEAMS_CAN_ACCOMODATE_PARTY_WITHOUT_EVICTION_TEXT = "No teams can accomodate incoming party without eviction.";
static const char8_t* NO_TEAMS_CAN_ACCOMODATE_PARTY_ASSIGNED_IN_ORDER_TEXT = "No teams can accomodate incoming party assigned in order.";

struct FactorTransform : public GameQualityTransform
{
    FactorTransform();
    Score activeEval(EvalContext& cxt, Game& game) override;
    Score passiveEval(EvalContext& cxt, const Game& game) const override;
} gTransform;

FactorTransform::FactorTransform()
{
    GameQualityFactorSpec spec;

    spec.mTransformName = "size.count_underfilled";

    spec.mFactorTransform = this;
    spec.mFactorResultSize = 1;
    spec.mRequiresProperty = false;
    spec.mRequiresIncrementalTeamFill = true;
    registerFactorSpec(spec);
}

Score FactorTransform::activeEval(EvalContext& cxt, Game& game)
{
    struct {
        PartyIndex partyIndex;
        TeamIndex teamIndex;
        int32_t playerCount;
    } incomingParty;

    incomingParty.partyIndex = game.getIncomingPartyIndex();
    incomingParty.playerCount = game.mGameParties[incomingParty.partyIndex].mPlayerCount;
    incomingParty.teamIndex = game.getTeamIndexByPartyIndex(incomingParty.partyIndex, cxt.mGameState);

    auto& internalState = game.getStateByContext(cxt);
    if (incomingParty.teamIndex == INVALID_TEAM_INDEX)
    {
        const auto teamCount = game.getTeamCount();
        const auto teamCapacity = game.getTeamCapacity();
        const auto gamePlayerCount = internalState.mPlayerCount;
        if (incomingParty.playerCount == 1)
        {
            // Fast Path: if incoming player count is 1 we can leave all assignments as they were, 
            // because the incoming greedy assignment of 1 will never reorder any existing assignments.

            // find first team that gets closest to being fully filled by having the incoming party added to it
            TeamIndex candidateTeamIndex = INVALID_TEAM_INDEX;
            int32_t processedPlayers = 0; // used to quickly end the search if all partially filled teams have already been processed
            for (TeamIndex i = 0; (i < teamCount) && (processedPlayers < gamePlayerCount); ++i)
            {
                const auto teamPlayerCount = internalState.mPlayerCountByTeam[i];
                const auto improvedTeamPlayerCount = teamPlayerCount + incomingParty.playerCount;
                processedPlayers += teamPlayerCount;
                if (improvedTeamPlayerCount < teamCapacity)
                {
                    if (candidateTeamIndex == INVALID_TEAM_INDEX)
                    {
                        // this is now a candidate team, whether it is the best candidate depends on whether it is the largest team
                        candidateTeamIndex = i;
                        processedPlayers += incomingParty.playerCount; // accounts for the first time we found a valid team for the incoming party
                    }
                    else if (teamPlayerCount > internalState.mPlayerCountByTeam[candidateTeamIndex])
                    {
                        // this is now a candidate team, whether it is the best candidate depends on whether it is the largest team
                        candidateTeamIndex = i;
                    }
                }
                else if (improvedTeamPlayerCount == teamCapacity)
                {
                    candidateTeamIndex = i;
                    break; // we are done, once we find a team we can fill fully, we don't need to search more
                }
            }

            if (candidateTeamIndex != INVALID_TEAM_INDEX)
            {
                game.getFutureState().assignPartyToTeam(incomingParty.partyIndex, incomingParty.playerCount, candidateTeamIndex);
            }
            else
            {
                cxt.setResultReason(NO_TEAMS_CAN_ACCOMODATE_PARTY_WITHOUT_EVICTION_TEXT);
                return INVALID_SCORE;
            }
        }
        else
        {
            // Slow Path: incoming party has > 1 player, we may need to reorder existing assignments to accomodate the incoming party

            PartySizeOccupancyBitset partySizeOccupancyBits;
            // NOTE: We deliberately calculate size occupancy bits using all parties *including* the incoming party, to ensure its correct prioritization by the algorithm
            calculateMutablePartySizeOccupancyBits(partySizeOccupancyBits, game, incomingParty.partyIndex + 1);

            GameInternalState newGameState;

            if (game.mImmutable)
            {
                for (PartyIndex partyIndex = 0; partyIndex < incomingParty.partyIndex; ++partyIndex)
                {
                    auto& gameParty = game.mGameParties[partyIndex];
                    if (gameParty.mImmutable)
                    {
                        // PACKER_TODO: should be able to switch GameState::PRESENT -> cxt.mGameState here, since all PRESENT->FUTURE state copy always occurs on first factor eval...
                        const auto existingPartyTeamIndex = game.getTeamIndexByPartyIndex(partyIndex, GameState::PRESENT);
                        EA_ASSERT(existingPartyTeamIndex >= 0);
                        const auto playerCount = gameParty.mPlayerCount;
                        newGameState.assignPartyToGame(partyIndex, playerCount);
                        newGameState.assignPartyToTeam(partyIndex, playerCount, existingPartyTeamIndex);
                    }
                }
            }

            // NOTE: simple polynomial bin packing algorithm that greedily minimizes the number of used bins(ie: teams). See: https://www.geeksforgeeks.org/bin-packing-problem-minimize-number-of-used-bins
            // process parties in descending order of size to improve chance of maximally utilizing the team space in the game (using size heuristic, because optimal bin packing problem is NP-hard)
            for (auto partySizeBit = partySizeOccupancyBits.find_last(), partySizeBits = partySizeOccupancyBits.size(); partySizeBit != partySizeBits; partySizeBit = partySizeOccupancyBits.find_prev(partySizeBit))
            {
                const auto partySize = partySizeBit + 1;
                const auto& partyBitset = mPartyBitsetByPartySize[partySizeBit];
                for (auto partyIndex = partyBitset.find_first(), partyCount = partyBitset.size(); partyIndex != partyCount; partyIndex = partyBitset.find_next(partyIndex))
                {
                    EA_ASSERT(game.mGameParties[partyIndex].mPlayerCount == partySize);

                    TeamIndex candidateTeamIndex = INVALID_TEAM_INDEX;
                    for (TeamIndex i = 0; i < teamCount; ++i)
                    {
                        const auto teamPlayerCount = newGameState.mPlayerCountByTeam[i];
                        const auto improvedTeamPlayerCount = teamPlayerCount + partySize;
                        if (improvedTeamPlayerCount < teamCapacity)
                        {
                            if (candidateTeamIndex == INVALID_TEAM_INDEX)
                            {
                                // this is now a candidate team, whether it is the best candidate depends on whether it is the largest team
                                candidateTeamIndex = i;
                            }
                            else if (teamPlayerCount > newGameState.mPlayerCountByTeam[candidateTeamIndex])
                            {
                                // this is now a candidate team, whether it is the best candidate depends on whether it is now the largest team
                                candidateTeamIndex = i;
                            }
                        }
                        else if (improvedTeamPlayerCount == teamCapacity)
                        {
                            candidateTeamIndex = i;
                            break; // we are done, once we find a team we can fill fully, no need to search more
                        }
                    }

                    if (candidateTeamIndex != INVALID_TEAM_INDEX)
                    {
                        newGameState.assignPartyToGame((int32_t)partyIndex, (int32_t)partySize);
                        newGameState.assignPartyToTeam((int32_t)partyIndex, (int32_t)partySize, candidateTeamIndex);
                    }
                    else if (partyIndex == incomingParty.partyIndex)
                    {
                        cxt.setResultReason(NO_TEAMS_CAN_ACCOMODATE_PARTY_ASSIGNED_IN_ORDER_TEXT);
                        return INVALID_SCORE;
                    }
                }
            }

            game.copyState(GameState::FUTURE, newGameState);
        }
    }
    
    auto score = passiveEval(cxt, game);
    return score;
}

Score FactorTransform::passiveEval(EvalContext& cxt, const Game& game) const
{
    const auto teamCount = game.getTeamCount();
    const auto teamCapacity = game.getTeamCapacity();
    auto& internalState = game.getStateByContext(cxt);
    int32_t numUnderfilledTeams = 0;
    for (TeamIndex i = 0; i < teamCount; ++i)
    {
        auto playerCount = internalState.mPlayerCountByTeam[i];
        if (playerCount > 0 && playerCount < teamCapacity)
            ++numUnderfilledTeams;
    }
    cxt.mEvalResult = { (Score) numUnderfilledTeams };
    return cxt.computeScore();
}

}

} // GamePacker
