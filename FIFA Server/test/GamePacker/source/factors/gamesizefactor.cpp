/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/game.h"

// factor description: The span between max capacity and total player count

namespace Packer
{

namespace GameSizeFactor
{

struct FactorTransform : public GameQualityTransform
{
    FactorTransform();
    Score activeEval(EvalContext& cxt, Game& game) override;
    Score passiveEval(EvalContext& cxt, const Game& game) const override;
} gTransform;

FactorTransform::FactorTransform()
{
    GameQualityFactorSpec spec;

    spec.mTransformName = "size";

    spec.mFactorTransform = this;
    spec.mFactorResultSize = 1;
    spec.mRequiresProperty = false;
    registerFactorSpec(spec);
}

Score FactorTransform::activeEval(EvalContext& cxt, Game& game)
{
    auto gamePlayerCount = game.getPlayerCount(cxt.mGameState);
    auto gameCapacity = game.getGameCapacity();
    if (gamePlayerCount > gameCapacity)
        gamePlayerCount = gameCapacity; // It's ok to be optimistic, since following factors can evict the party if the game is over capacity.
    cxt.mEvalResult = { (Score)gamePlayerCount };

    return cxt.computeScore();
}

Score FactorTransform::passiveEval(EvalContext& cxt, const Game& game) const
{
    auto gamePlayerCount = game.getPlayerCount(cxt.mGameState);
    auto gameCapacity = game.getGameCapacity();
    if (gamePlayerCount > gameCapacity)
        return INVALID_SCORE; // We must now put our foot down and reject this if no other factors have evicted sufficient number of players to bring the player count into valid range.
    cxt.mEvalResult = { (Score)gamePlayerCount };

    return cxt.computeScore();
}

}

} // GamePacker
