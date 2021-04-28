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

// factor attempts to minimize the average of a property value

namespace Packer
{

namespace GamePlayerPropertyAvgFactor
{

static const char8_t* UNABLE_TO_EVICT_SUFFICIENT_PLAYERS_TEXT = "Unable to evict sufficient number of players to accomodate incoming party.";

struct FactorTransform : public GameQualityTransform
{
    FactorTransform();
    Score activeEval(EvalContext& cxt, Game& game) override;
    Score passiveEval(EvalContext& cxt, const Game& game) const override;
} gTransform;

FactorTransform::FactorTransform()
{
    GameQualityFactorSpec spec;

    spec.mTransformName = "avg";

    spec.mFactorTransform = this;
    spec.mFactorResultSize = 1;
    spec.mRequiresProperty = true;
    spec.mRequiresPropertyFieldValues = false; // this factor allows optional field values
    registerFactorSpec(spec);
}

Score FactorTransform::activeEval(EvalContext& cxt, Game& game)
{
    struct {
        PartyIndex index;
        int32_t playerCount;
    } incomingParty = {};
    incomingParty.index = game.getIncomingPartyIndex();
    incomingParty.playerCount = (decltype(incomingParty.playerCount))game.mGameParties[incomingParty.index].mPlayerCount;

    auto needToEvictPlayers = game.getPlayerCount(GameState::FUTURE) - game.getGameCapacity();
    if (needToEvictPlayers > 0)
    {
        auto& gamePartyBits = game.getFutureState().mPartyBits;
        auto& propIndexer = game.getPlayerPropertyIndex(cxt);
        PartyBitset processedPartyBits;
        int32_t newlyEvictedPlayers = 0;
        for (auto playerIndex : eastl::reverse(propIndexer))
        {
            const GamePlayer& gamePlayer = game.mGamePlayers[playerIndex];
            const auto partyIndex = gamePlayer.mGamePartyIndex;
            const GameParty& gameParty = game.mGameParties[partyIndex];
            if (gameParty.mImmutable || processedPartyBits.test(partyIndex) || !gamePartyBits.test(partyIndex))
                continue;

            processedPartyBits.set(partyIndex); // process each party only once

            const auto gamePartyPlayerCount = (int32_t)gameParty.mPlayerCount;
            if (newlyEvictedPlayers + gamePartyPlayerCount <= needToEvictPlayers)
            {
                newlyEvictedPlayers += gamePartyPlayerCount;
                game.evictPartyFromGame(partyIndex, GameState::FUTURE);
                // proceed to find next player index if there is one
            }
            else
                break;
        }

        if (needToEvictPlayers > newlyEvictedPlayers)
        {
            cxt.setResultReason(UNABLE_TO_EVICT_SUFFICIENT_PLAYERS_TEXT);
            return INVALID_SCORE; // we were unable to evict players that we need to evict to accomodate incoming party. PACKER_MAYBE: The newlyEvictedPlayers processing logic above should be generified and used in all factors because they all need to handle the case where evicting players is necessary to accomodate new players.
        }
    }

    Score score = passiveEval(cxt, game);

    return score;
}

Score FactorTransform::passiveEval(EvalContext& cxt, const Game& game) const
{
    int32_t numPlayers = 0; // tracks # of 'value participants' that specify this value in the game (a player can have unset field values)
    PropertyValue propertyValueSum = 0;
    auto& gameState = game.getStateByContext(cxt);

    // IMPORTANT: This factor cannot blindly use gameState.mPlayerCount to iterate players and sum their property values because some parties may be pending eviction.
    auto lookup = game.getPropertyValueLookup(cxt);
    for (auto partyIndex = gameState.mPartyBits.find_first(), partySize = gameState.mPartyBits.size(); partyIndex != partySize; partyIndex = gameState.mPartyBits.find_next(partyIndex))
    {
        auto& gameParty = game.mGameParties[partyIndex];
        for (uint16_t playerIndex = gameParty.mPlayerOffset, playerIndexEnd = gameParty.mPlayerOffset + gameParty.mPlayerCount; playerIndex < playerIndexEnd; ++playerIndex)
        {
            if (lookup.hasPlayerPropertyValue(playerIndex))
            {
                propertyValueSum += lookup.getPlayerPropertyValue(playerIndex);
                numPlayers++;
            }
        }
    }

    if (numPlayers == 0)
    {
        // clamp field score to 0, because value participation score is 0% (see spec.mRequiresPropertyFieldValues = false this factor handles unspecified property values)
        cxt.setResultReason(PROPERTY_FIELD_VALUE_PARTICIPATION_TOO_LOW_TEXT);
        return MIN_SCORE;
    }

    EA_ASSERT(numPlayers <= game.getGameCapacity());
    // TODO: Dont need to iterate to compute averages, they could be updated as part of derived properties in the game
    auto average = (Score)(propertyValueSum / numPlayers); // NOTE: currently intentionally truncating cast to keep consistency to previous factor implementation that used integer valued scores
    cxt.mEvalResult = { average };
    auto score = cxt.computeScore();
    return score;
}

}

} // GamePacker
