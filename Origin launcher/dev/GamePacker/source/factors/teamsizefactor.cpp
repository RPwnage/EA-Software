/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/game.h"

// factor description: The span between largest and smallest team by player count

namespace Packer
{


namespace TeamSizeFactor
{

static const char8_t* INVALID_TEAM_COUNT_TEXT = "Number of teams too low for computing minimum size team.";

struct FactorTransform : public GameQualityTransform
{
    FactorTransform();
    Score activeEval(EvalContext& cxt, Game& game) override;
    Score passiveEval(EvalContext& cxt, const Game& game) const override;
    static TeamIndex getSmallestTeam(const GameInternalState& internalState, const int32_t teamCount);
    static const uint32_t MIN_FACTOR_TEAM_COUNT = 1; // minimum team count that makes sense when you use this factor (for balancing teams by the sum of the properties of the team members)
} gTransform;

FactorTransform::FactorTransform()
{
    GameQualityFactorSpec spec;

    spec.mTransformName = "size.min";

    spec.mFactorTransform = this;
    spec.mFactorResultSize = 1;
    spec.mRequiresProperty = false;
    registerFactorSpec(spec);
}

Score FactorTransform::activeEval(EvalContext& cxt, Game& game)
{
    const auto teamCount = game.getTeamCount();
    if (teamCount < MIN_FACTOR_TEAM_COUNT)
    {
        // If there is one team, or no team there is nothing to balance!
        cxt.setResultReason(INVALID_TEAM_COUNT_TEXT);
        return INVALID_SCORE;
    }

    Score score = INVALID_SCORE;
    const auto incomingPartyIndex = game.getIncomingPartyIndex();
    auto& internalState = game.getStateByContext(cxt);
    auto incomingPartyTeamIndex = internalState.mTeamIndexByParty[incomingPartyIndex];
    // TODO: we need to implement ability to reconcile additional pending evictions despite incoming party being assigned.
    if (incomingPartyTeamIndex != INVALID_TEAM_INDEX)
    {
        score = passiveEval(cxt, game);
    }
    else
    {
        const auto teamCapacity = game.getTeamCapacity();
        const auto incomingPlayerCount = (int32_t)game.mGameParties[incomingPartyIndex].mPlayerCount;
        for (TeamIndex minTeamIndex = 0; minTeamIndex < teamCount; ++minTeamIndex)
        {
            auto teamPlayerCount = internalState.mPlayerCountByTeam[minTeamIndex];
            if (teamPlayerCount + incomingPlayerCount <= teamCapacity)
            {
                // determine if this is the lowest team that can accomodate this party
                for (TeamIndex i = minTeamIndex + 1; i < teamCount; ++i)
                {
                    auto candidateTeamPlayerCount = internalState.mPlayerCountByTeam[i];
                    if ((candidateTeamPlayerCount + incomingPlayerCount <= teamCapacity) &&
                        (candidateTeamPlayerCount < teamPlayerCount))
                    {
                        minTeamIndex = i;
                        teamPlayerCount = candidateTeamPlayerCount;
                    }
                }
                game.getFutureState().assignPartyToTeam(incomingPartyIndex, incomingPlayerCount, minTeamIndex);

                auto futureMinTeamIndex = getSmallestTeam(internalState, teamCount);
                cxt.mEvalResult = { (Score)internalState.mPlayerCountByTeam[futureMinTeamIndex] };
                score = cxt.computeScore();
                break;
            }
        }

        // NOTE: currently we don't attempt to evict players when incoming party has more players than
        // there are slots on the lowest open team. This can result in no evictions even though an incoming
        // party can be accomodated nicely in an existing game by evicting players from it and reassigning them
        // elsewhere... (see example: gpsettings.teamsize.cfg for repro)
        // FUTURE: we could potentially evict the fewest players to accomodate the new party such that
        // the evicted players are more recent than incoming as well...
    }
    return score;
}

Score FactorTransform::passiveEval(EvalContext& cxt, const Game& game) const
{
    const auto teamCount = game.getTeamCount();
    auto& internalState = game.getStateByContext(cxt);
    auto minTeamIndex = getSmallestTeam(internalState, teamCount);
    cxt.mEvalResult = { (Score)internalState.mPlayerCountByTeam[minTeamIndex] };
    return cxt.computeScore();
}

TeamIndex FactorTransform::getSmallestTeam(const GameInternalState& internalState, const int32_t teamCount)
{
    TeamIndex minTeamIndex = 0;
    int32_t minTeamPlayerCount = internalState.mPlayerCountByTeam[minTeamIndex];
    for (int32_t teamIndex = 1; teamIndex < teamCount; ++teamIndex)
    {
        int32_t playerCount = internalState.mPlayerCountByTeam[teamIndex];
        if (playerCount < minTeamPlayerCount)
        {
            minTeamPlayerCount = playerCount;
            minTeamIndex = teamIndex;
        }
    }
    return minTeamIndex;
}

}

} // GamePacker
