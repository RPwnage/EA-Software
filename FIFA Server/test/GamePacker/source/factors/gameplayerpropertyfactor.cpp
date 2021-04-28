/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include <EASTL/bitset.h>
#include <EASTL/bonus/adaptors.h>
#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/game.h"

namespace Packer
{

namespace GamePlayerPropertyFactor
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

    spec.mTransformName = "minmax";

    spec.mFactorTransform = this;
    spec.mFactorResultSize = 2;
    spec.mRequiresProperty = true;
    registerFactorSpec(spec);
}

// TODO: This factor currently is limited by having an unnecessary dependency on getTeamIndexByPartyIndex() despite not being a team specific factor,
// because in the past our implementation did not allow for assigning a party to a game without specifying a team index... Now we do!
// Step1: Replace usage of game.getTeamIndexByPartyIndex(partyIndex, GameState::FUTURE) with game.isPartyAssignedToGame(),
// Step2: Instead of iterating the players, we might also be able to use a party indexers over the max property value, and min property value?

Score FactorTransform::activeEval(EvalContext& cxt, Game& game)
{
    Score score = INVALID_SCORE;
    struct {
        PartyIndex index;
        PropertyValue maxPropertyValue;
        PropertyValue minPropertyValue;
        int32_t playerCount;
    } incomingParty = {};

    auto lookup = game.getPropertyValueLookup(cxt);

    incomingParty.index = game.getIncomingPartyIndex();
    incomingParty.maxPropertyValue = lookup.getPartyPropertyValue(incomingParty.index, PropertyAggregate::MAX);
    incomingParty.minPropertyValue = lookup.getPartyPropertyValue(incomingParty.index, PropertyAggregate::MIN);
    incomingParty.playerCount = (int32_t)game.mGameParties[incomingParty.index].mPlayerCount;

    auto& propIndexer = game.getPlayerPropertyIndex(cxt);
    struct {
        PlayerIndex index;
        PropertyValue propertyValue;
    } maxPlayer, minPlayer;
    maxPlayer.index = propIndexer.back();
    maxPlayer.propertyValue = lookup.getPlayerPropertyValue(maxPlayer.index);
    minPlayer.index = propIndexer.front();
    minPlayer.propertyValue = lookup.getPlayerPropertyValue(minPlayer.index);

    // game must have at least 2 parties since otherwise it doesnt make sense to evict anyone, but in reality in order to avoid
    // over-weighting this factor (thereby causing it to evict parties even when the game is not full) we should only engage
    // the eviction logic once the game fills to capacity (or to minViableGame threshold specified by the gamesize factor)
    if (game.getPlayerCount(GameState::PRESENT) + incomingParty.playerCount > game.getGameCapacity() && incomingParty.maxPropertyValue - incomingParty.minPropertyValue < maxPlayer.propertyValue - minPlayer.propertyValue)
    {
        // incoming party property value range is smaller than current game, otherwise there is no way to reduce the range by accomodating existing party

        /*
        A..B = party.maxplayerskill..party.minplayerskill
        C..D = game.maxplayerskill..game.minplayerskill
        // NOTE: Its important to recognize that even in fully disjoint cases such as when
        // A > C, adding AB while evicting members of CD starting with D might result in AD'
        // where D' is a new low, such that |AD'| < |CD|!
        table of possible options:
        1) ABCD // (possible improvement) Evicting N parties between C-D such that the |AD'| < |CD|
        2) ACBD // (possible improvement) Evicting N parties between B-D such that the |AD'| < |CD|
        3) CABD // (possible improvement) Evicting N parties between C-A/B-D such that the |C'D'| < |CD|
        4) CADB // (possible improvement) Evicting N parties between C-A such that the |C'B| < |CD|
        5) CDAB // (possible improvement) Evicting N parties between B-D such that the |C'B| < |AD|
        6) ACDB // (no improvement) Unless |AB| < |CD| and we consider evicting all CD, but then we might as well just add AB to a new game!

        The following example summarizes the possible combinations of parties that can be evicted in order from both the partybymaxplayerskill[] and the partybyminplayerskill[] orders.
        parties to evict
        partybymaxplayerskill[] = { A, B, C, D }
        partybyminplayerskill[] = { E, F, G, H }
        evictparties: 1 -> 2 options
        A
        H
        evictparties: 2 -> 3 options
        AB
        A[H]
        GH
        evictparties: 3 -> 4 options
        ABC
        AB[H]
        A[GH]
        FGH
        evictparties: 4 -> 5 options
        ABCD
        ABC[H]
        AB[GH]
        A[FGH]
        EFGH
        */
        auto evictedPlayers = game.getPlayerCount(GameState::PRESENT) + incomingParty.playerCount - game.getPlayerCount(GameState::FUTURE);
        if (incomingParty.maxPropertyValue > maxPlayer.propertyValue)
        {
            // incoming high above game high
            PartyBitset processedPartyBits;
            PlayerIndex nextRemainingPlayerIndex = INVALID_GAME_PLAYER_INDEX;
            for (auto playerIndex : propIndexer)
            {
                const GamePlayer& gamePlayer = game.mGamePlayers[playerIndex];
                const auto partyIndex = gamePlayer.mGamePartyIndex;
                if (!processedPartyBits.test(partyIndex))
                {
                    processedPartyBits.set(partyIndex); // process each party only once
                    const auto teamIndex = game.getTeamIndexByPartyIndex(partyIndex, GameState::FUTURE);
                    if (teamIndex >= 0)
                    {
                        const auto propertyValue = lookup.getPlayerPropertyValue(playerIndex);
                        if (propertyValue < incomingParty.minPropertyValue)
                        {
                            const GameParty& gameParty = game.mGameParties[partyIndex];
                            if (gameParty.mImmutable)
                                continue; // skip immutable party
                            const auto gamePartyPlayerCount = (int32_t)gameParty.mPlayerCount;
                            if (evictedPlayers + gamePartyPlayerCount <= incomingParty.playerCount)
                            {
                                evictedPlayers += gamePartyPlayerCount;
                                game.evictPartyFromGame(partyIndex, GameState::FUTURE);
                                continue; // proceed to find next player index if there is one
                            }
                        }
                        nextRemainingPlayerIndex = playerIndex;
                        break;
                    }
                }
            }

            const auto domain = incomingParty.maxPropertyValue;
            auto subDomain = incomingParty.minPropertyValue;
            if (nextRemainingPlayerIndex >= 0)
            {
                auto propertyValue = lookup.getPlayerPropertyValue(nextRemainingPlayerIndex);
                if (propertyValue < subDomain)
                    subDomain = propertyValue;
            }
            cxt.mEvalResult = { (Score)domain, (Score)subDomain };
            score = cxt.computeScore();
        }
        else if (incomingParty.minPropertyValue < minPlayer.propertyValue)
        {
            // incoming low above below game low
            PartyBitset processedPartyBits;
            PlayerIndex nextRemainingPlayerIndex = INVALID_GAME_PLAYER_INDEX;
            for (auto playerIndex : eastl::reverse(propIndexer))
            {
                const GamePlayer& gamePlayer = game.mGamePlayers[playerIndex];
                const auto partyIndex = gamePlayer.mGamePartyIndex;
                if (!processedPartyBits.test(partyIndex))
                {
                    processedPartyBits.set(partyIndex); // process each party only once
                    const auto teamIndex = game.getTeamIndexByPartyIndex(partyIndex, GameState::FUTURE);
                    if (teamIndex >= 0)
                    {
                        const auto propertyValue = lookup.getPlayerPropertyValue(playerIndex);
                        if (propertyValue > incomingParty.maxPropertyValue)
                        {
                            const GameParty& gameParty = game.mGameParties[partyIndex];
                            if (gameParty.mImmutable)
                                continue; // skip immutable party
                            const auto gamePartyPlayerCount = (int32_t)gameParty.mPlayerCount;
                            if (evictedPlayers + gamePartyPlayerCount <= incomingParty.playerCount)
                            {
                                evictedPlayers += gamePartyPlayerCount;

                                game.evictPartyFromGame(partyIndex, GameState::FUTURE);
                                continue; // proceed to find next player index if there is one
                            }
                        }
                        nextRemainingPlayerIndex = playerIndex;
                        break;
                    }
                }
            }
            auto domain = incomingParty.maxPropertyValue;
            const auto subDomain = incomingParty.minPropertyValue;
            if (nextRemainingPlayerIndex >= 0)
            {
                auto propertyValue = lookup.getPlayerPropertyValue(nextRemainingPlayerIndex);
                if (propertyValue > domain)
                    domain = propertyValue;
            }
            cxt.mEvalResult = { (Score)domain, (Score)subDomain };
            score = cxt.computeScore();
        }
        else
        {
            // incoming high/low is sandwiched between game high/low (may include one of the values overlapping)
            PlayerIndex curUpperBoundPlayerIndex = INVALID_GAME_PLAYER_INDEX;
            for (auto playerIndex : eastl::reverse(propIndexer))
            {
                const GamePlayer& gamePlayer = game.mGamePlayers[playerIndex];
                const auto partyIndex = gamePlayer.mGamePartyIndex;
                const auto teamIndex = game.getTeamIndexByPartyIndex(partyIndex, GameState::FUTURE);
                if (teamIndex >= 0)
                {
                    curUpperBoundPlayerIndex = playerIndex;
                    break;
                }
            }
            PlayerIndex curLowerBoundPlayerIndex = INVALID_GAME_PLAYER_INDEX;
            for (auto playerIndex : propIndexer)
            {
                const GamePlayer& gamePlayer = game.mGamePlayers[playerIndex];
                const auto partyIndex = gamePlayer.mGamePartyIndex;
                const auto teamIndex = game.getTeamIndexByPartyIndex(partyIndex, GameState::FUTURE);
                if (teamIndex >= 0) // Question: when only 1 party in the game, should the minmax range be propValue..propValue or propValue..0?, if latter we need to avoid setting the lowerBound identical to upper bound...
                {
                    curLowerBoundPlayerIndex = playerIndex;
                    break;
                }
            }
            PropertyValue curDomain = 0;
            if (curUpperBoundPlayerIndex >= 0)
                curDomain = lookup.getPlayerPropertyValue(curUpperBoundPlayerIndex);
            PropertyValue curSubDomain = 0;
            if (curLowerBoundPlayerIndex >= 0)
                curSubDomain = lookup.getPlayerPropertyValue(curLowerBoundPlayerIndex);
            cxt.mEvalResult = { (Score)curDomain, (Score)curSubDomain };
            Score curResultScore = cxt.computeScore();
            const auto partyEvictMax = game.getPartyCount(GameState::PRESENT) - 1; // how many parties are there to be evicted total
            eastl::bitset<MAX_PARTY_COUNT> evictedPartyBits;
            for (int32_t tryevict = 1; tryevict < partyEvictMax; ++tryevict)
            {
                // iterate over all possible sub-sequences of min/max evictions,
                // and select the most optimal one that is viable (evicts no more players than incoming)
                int32_t improvecount = 0; // # of times estimate was improved for a specific sub-sequence length
                for (int32_t i = 0; i <= tryevict; ++i)
                {
                    int32_t tryEvictedPlayers = 0;
                    int32_t tryEvictedTopParties = 0;
                    int32_t tryEvictedBottomParties = 0;
                    eastl::bitset<MAX_PARTY_COUNT> tryEvictedPartyBits;
                    eastl::bitset<MAX_PARTY_COUNT> processedPartyBits;
                    processedPartyBits.reset();
                    PlayerIndex lowerBoundPlayerIndex = INVALID_GAME_PLAYER_INDEX;
                    for (auto playerIndex : propIndexer)
                    {
                        const GamePlayer& gamePlayer = game.mGamePlayers[playerIndex];
                        auto partyIndex = gamePlayer.mGamePartyIndex;
                        if (!processedPartyBits.test(partyIndex))
                        {
                            processedPartyBits.set(partyIndex);
                            TeamIndex teamIndex = game.getTeamIndexByPartyIndex(partyIndex, GameState::FUTURE);
                            if (teamIndex >= 0)
                            {
                                auto& gameParty = game.mGameParties[partyIndex];
                                if (gameParty.mImmutable)
                                    continue; // skip immutable party
                                auto propertyValue = lookup.getPlayerPropertyValue(playerIndex);
                                auto gamePartyPlayerCount = (int32_t)gameParty.mPlayerCount;
                                if (propertyValue < incomingParty.minPropertyValue &&
                                    evictedPlayers + tryEvictedPlayers + gamePartyPlayerCount <= incomingParty.playerCount)
                                {
                                    tryEvictedPlayers += gamePartyPlayerCount;
                                    tryEvictedBottomParties++;
                                    tryEvictedPartyBits.set(partyIndex);
                                    if (tryEvictedBottomParties <= tryevict - i)
                                        continue; // we can still try to evict more
                                }
                                lowerBoundPlayerIndex = playerIndex;
                                break;
                            }
                        }
                    }
                    processedPartyBits.reset();
                    PlayerIndex upperBoundPlayerIndex = INVALID_GAME_PLAYER_INDEX;
                    for (auto playerIndex : eastl::reverse(propIndexer))
                    {
                        const GamePlayer& gamePlayer = game.mGamePlayers[playerIndex];
                        auto partyIndex = gamePlayer.mGamePartyIndex;
                        if (!processedPartyBits.test(partyIndex))
                        {
                            processedPartyBits.set(partyIndex);
                            auto teamIndex = game.getTeamIndexByPartyIndex(partyIndex, GameState::FUTURE);
                            if (teamIndex >= 0)
                            {
                                auto& gameParty = game.mGameParties[partyIndex];
                                if (gameParty.mImmutable)
                                    continue; // skip immutable party
                                auto propertyValue = lookup.getPlayerPropertyValue(playerIndex);
                                auto gamePartyPlayerCount = (int32_t)gameParty.mPlayerCount;
                                if (propertyValue > incomingParty.maxPropertyValue &&
                                    evictedPlayers + tryEvictedPlayers + gamePartyPlayerCount <= incomingParty.playerCount)
                                {
                                    tryEvictedPlayers += gamePartyPlayerCount;
                                    tryEvictedTopParties++;
                                    tryEvictedPartyBits.set(partyIndex);
                                    if (tryEvictedTopParties <= i)
                                        continue; // we can still try to evict more
                                }
                                upperBoundPlayerIndex = playerIndex;
                                break;
                            }
                        }
                    }

                    if (tryEvictedPlayers > 0)
                    {
                        PropertyValue domain = incomingParty.maxPropertyValue;
                        if (upperBoundPlayerIndex >= 0)
                        {
                            auto propertyValue = lookup.getPlayerPropertyValue(upperBoundPlayerIndex);
                            if (domain < propertyValue)
                                domain = propertyValue;
                        }
                        PropertyValue subDomain = incomingParty.minPropertyValue;
                        if (lowerBoundPlayerIndex >= 0)
                        {
                            auto propertyValue = lookup.getPlayerPropertyValue(lowerBoundPlayerIndex);
                            if (subDomain > propertyValue)
                                subDomain = propertyValue;
                        }
                        EvalResult savedResult = cxt.mEvalResult;
                        cxt.mEvalResult = { (Score)domain, (Score)subDomain };
                        auto tempScore = cxt.computeScore();
                        if (tempScore >= 0 && tempScore > curResultScore)
                        {
                            curResultScore = tempScore;
                            evictedPartyBits = tryEvictedPartyBits;
                            improvecount++;
                        }
                        else
                            cxt.mEvalResult = savedResult; // restore previous copy
                    }
                }

                if (improvecount == 0)
                    break; // no evictable sequences of length 'tryevict' that improve current estimate found, longer sequences will not be valid either
            }
            if (evictedPartyBits.any())
            {
                score = curResultScore;
                for (auto partyIndex = evictedPartyBits.find_first(), e = evictedPartyBits.size(); partyIndex != e; partyIndex = evictedPartyBits.find_next(partyIndex))
                {
                    game.evictPartyFromGame((int32_t)partyIndex, GameState::FUTURE);
                }
            }
            else
            {
                score = passiveEval(cxt, game);
            }
        }
    }
    else
    {
        score = passiveEval(cxt, game);
    }

    return score;
}

Score FactorTransform::passiveEval(EvalContext& cxt, const Game& game) const
{
    Score score = INVALID_SCORE;
    struct {
        PartyIndex index;
        PropertyValue maxPropertyValue;
        PropertyValue minPropertyValue;
    } incomingParty = {};
    auto lookup = game.getPropertyValueLookup(cxt);
    auto& propIndexer = game.getPlayerPropertyIndex(cxt);

    // incoming high/low is sandwiched between game high/low (may include one of the values overlapping)
    for (auto curUpperBoundPlayerIndex : eastl::reverse(propIndexer))
    {
        const GamePlayer& upperBoundGamePlayer = game.mGamePlayers[curUpperBoundPlayerIndex];
        auto upperBoundTeamIndex = game.getTeamIndexByPartyIndex(upperBoundGamePlayer.mGamePartyIndex, cxt.mGameState);
        if (upperBoundTeamIndex >= 0)
        {
            auto curDomain = lookup.getPlayerPropertyValue(curUpperBoundPlayerIndex);
            for (auto curLowerBoundPlayerIndex : propIndexer)
            {
                const GamePlayer& lowerBoundGamePlayer = game.mGamePlayers[curLowerBoundPlayerIndex];
                auto lowerBoundTeamIndex = game.getTeamIndexByPartyIndex(lowerBoundGamePlayer.mGamePartyIndex, cxt.mGameState);
                if (lowerBoundTeamIndex >= 0)
                {
                    auto curSubDomain = lookup.getPlayerPropertyValue(curLowerBoundPlayerIndex);

                    if (cxt.mGameState != GameState::PRESENT)
                    {
                        incomingParty.index = game.getIncomingPartyIndex();
                        EA_ASSERT(incomingParty.index >= 0);
                        incomingParty.maxPropertyValue = lookup.getPartyPropertyValue(incomingParty.index, PropertyAggregate::MAX);
                        incomingParty.minPropertyValue = lookup.getPartyPropertyValue(incomingParty.index, PropertyAggregate::MIN);

                        if (curDomain < incomingParty.maxPropertyValue)
                            curDomain = incomingParty.maxPropertyValue;
                        if (curSubDomain > incomingParty.minPropertyValue)
                            curSubDomain = incomingParty.minPropertyValue;
                    }

                    cxt.mEvalResult = { (Score)curDomain, (Score)curSubDomain };
                    score = cxt.computeScore();
                    break;
                }
            }
            break;
        }
    }

    return score;
}

}

} // GamePacker
