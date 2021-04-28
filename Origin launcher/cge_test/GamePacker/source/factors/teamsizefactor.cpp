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

struct FactorTransform : public GameQualityTransform
{
    FactorTransform();
    Score activeEval(EvalContext& cxt, Game& game) override;
    Score passiveEval(EvalContext& cxt, const Game& game) const override;
    static void getSmallestAvailableTeam(TeamIndex& minTeamIndex, int32_t& minTeamPlayerCount, const Game& game, GameState state);
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
    Score score = INVALID_SCORE;
    const auto incomingPartyIndex = game.getIncomingPartyIndex();
    const GameParty& incomingGameParty = game.mGameParties[incomingPartyIndex];
    auto incomingPartyTeamIndex = game.getTeamIndexByPartyIndex(incomingPartyIndex, GameState::FUTURE);
    if (incomingPartyTeamIndex != INVALID_TEAM_INDEX) // TODO: we need to implement ability to reconcile additional pending evictions despite incoming party being assigned.
    {
        score = passiveEval(cxt, game);
    }
    else
    {
        TeamIndex minTeamIndex = 0;
        int32_t minTeamPlayerCount = 0;
        getSmallestAvailableTeam(minTeamIndex, minTeamPlayerCount, game, GameState::FUTURE);
        auto incomingPlayerCount = (int32_t)incomingGameParty.mPlayerCount;
        int32_t futureTeamPlayerCount = minTeamPlayerCount + incomingPlayerCount;
        if (futureTeamPlayerCount <= game.getTeamCapacity())
        {
            game.getFutureState().assignPartyToTeam(incomingPartyIndex, incomingPlayerCount, minTeamIndex);

            // get new smallest team size
            getSmallestAvailableTeam(minTeamIndex, minTeamPlayerCount, game, GameState::FUTURE);
            const auto newLowTeamPlayerCount = eastl::min(futureTeamPlayerCount, minTeamPlayerCount);

            cxt.mEvalResult = { (Score)newLowTeamPlayerCount };
            score = cxt.computeScore();
        }
        else
        {
            // NOTE: currently we don't attempt to evict players when incoming party has more players than
            // there are slots on the lowest open team. This can result in no evictions even though an incoming
            // party can be accomodated nicely in an existing game by evicting players from it and reassigning them
            // elsewhere... (see example: gpsettings.teamsize.cfg for repro)
            // FUTURE: we could potentially evict the fewest players to accomodate the new party such that
            // the evicted players are more recent than incoming as well...
        }
    }
    return score;
}

Score FactorTransform::passiveEval(EvalContext& cxt, const Game& game) const
{
    TeamIndex minTeamIndex = 0;
    int32_t minTeamPlayerCount = 0;
    getSmallestAvailableTeam(minTeamIndex, minTeamPlayerCount, game, GameState::FUTURE);
    cxt.mEvalResult = { (Score)minTeamPlayerCount };
    return cxt.computeScore();
}

void FactorTransform::getSmallestAvailableTeam(TeamIndex& minTeamIndex, int32_t& minTeamPlayerCount, const Game& game, GameState state)
{
    minTeamIndex = 0;
    minTeamPlayerCount = game.getTeamPlayerCount(state, minTeamIndex);
    int32_t teamCount = game.getTeamCount();
    for (int32_t teamIndex = 0; teamIndex < teamCount; ++teamIndex)
    {
        int32_t playerCount = game.getTeamPlayerCount(state, teamIndex);
        if (playerCount < minTeamPlayerCount)
        {
            minTeamPlayerCount = playerCount;
            minTeamIndex = teamIndex;
        }
    }
}

}

} // GamePacker
