/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/game.h"

namespace Packer
{

// PACKER_TODO: Need to fix this factor to use player iteration instead of party based iteration.
// There is no time to do this at the moment, and there are no users of this factor presently,
// but the conversion is expected to be simple.

#define ENABLE_MAX2_FACTOR 0 // Turned off for now

namespace GamePlayerPropertyMaxFactor // MaxMax = TopGap = TopRange
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

    spec.mTransformName = "max2";

    spec.mFactorTransform = this;
    spec.mFactorResultSize = 2;
    spec.mRequiresProperty = true;
    registerFactorSpec(spec);
}

Score FactorTransform::activeEval(EvalContext& cxt, Game& game)
{
#if ENABLE_MAX2_FACTOR
    Score score = INVALID_SCORE;
    {
        // this limitation exists in this function because we find the 'first' lowest item, rather then MAX_TEAM_COUNTth item.
        // We can address it by walking down the parties at most: MAX_TEAM_COUNT steps to find the one who is in MAX_TEAM_COUNTth position after the first.
        // in order to compute the proper range between the highest and lowest party assignable to different teams, but this is really goign to be done
        // when we replace this factor with TeamPartyPlayerPropertyMax which will ensure we are computing the gap between the top end and bottom teams.
        static_assert(2 == MAX_TEAM_COUNT, "Only 2 teams supported!");
    }

    struct {
        PartyIndex index;
        PropertyValue propertyValue;
    } incomingParty;

    incomingParty.index = game.getIncomingPartyIndex();
    incomingParty.propertyValue = game.getPartyPropertyValue(cxt, incomingParty.index, PropertyAggregate::MAX);
    const PropertyIndexer propIndexer = game.getPropertyIndexer(partyMaxSpec);

    struct {
        PropertyValue propertyValue;
        Score newScore;
    } evictionCandidateParties[2];

    // PACKER_TODO: This iteration needs to be reworked to use the vanilla player values in descending order. This way we do not depend on automatic party-level aggregate indexing scheme, which is no longer supported!

    
    for (auto i = incomingParty.index - 1; i >= 0; --i)
    {
        const auto iPartyIndex = propIndexer[i];
        const auto iPartyTeamIndex = game.getTeamIndexByPartyIndex(iPartyIndex, GameState::FUTURE);
        if (iPartyTeamIndex != INVALID_TEAM_INDEX)
        {
            evictionCandidateParties[0].propertyValue = game.getPartyPropertyValue(cxt, iPartyIndex, PropertyAggregate::MAX);
            cxt.mEvalResult = { (Score)evictionCandidateParties[0].propertyValue, (Score)incomingParty.propertyValue };
            evictionCandidateParties[0].newScore = cxt.computeScore();
            EA_ASSERT(evictionCandidateParties[0].newScore >= 0);
            score = evictionCandidateParties[0].newScore;
            for (auto j = i - 1; j >= 0; --j)
            {
                const auto jPartyIndex = propIndexer[j];
                const auto jPartyTeamIndex = game.getTeamIndexByPartyIndex(jPartyIndex, GameState::FUTURE);
                if (jPartyTeamIndex != INVALID_TEAM_INDEX)
                {
                    evictionCandidateParties[1].propertyValue = game.getPartyPropertyValue(cxt, jPartyIndex, PropertyAggregate::MAX);
                    cxt.mEvalResult = { (Score)evictionCandidateParties[1].propertyValue, (Score)incomingParty.propertyValue };
                    evictionCandidateParties[1].newScore = cxt.computeScore();
                    EA_ASSERT(evictionCandidateParties[1].newScore >= 0);
                    PartyIndex evictPartyIndex = INVALID_PARTY_INDEX;
                    auto newScore = game.mFactorScores[cxt.mFactor.mFactorIndex];
                    if (evictionCandidateParties[0].newScore > newScore)
                    {
                        evictPartyIndex = jPartyIndex;
                        newScore = evictionCandidateParties[0].newScore;
                    }
                    if (evictionCandidateParties[1].newScore > newScore)
                    {
                        evictPartyIndex = iPartyIndex;
                        newScore = evictionCandidateParties[1].newScore;
                    }
                    if (evictPartyIndex != INVALID_PARTY_INDEX)
                    {
                        const GameParty& evictParty = game.mGameParties[evictPartyIndex];
                        if (!evictParty.mImmutable)
                        {
                            game.evictPartyFromGame(evictPartyIndex, GameState::FUTURE);
                            score = newScore;
                        }
                        else
                            score = INVALID_SCORE; // cant evict the party we want to because it's immutable
                    }
                    else
                        score = INVALID_SCORE; // cant evict the party we want to
                    break;
                }
            }
            break;
        }
    }

    if (score == INVALID_SCORE)
    {
        score = passiveEval(cxt, game);
    }
    return score;
#else
    EA_FAIL_MSG("Change the impl to use player index iteration, instead of party index iteration.");
    return INVALID_SCORE;
#endif
}

Score FactorTransform::passiveEval(EvalContext& cxt, const Game& game) const
{
#if ENABLE_MAX2_FACTOR
    {
        // this limitation exists in this function because we find the 'first' lowest item, rather then MAX_TEAM_COUNTth item.
        // We can address it by walking down the parties at most: MAX_TEAM_COUNT steps to find the one who is in MAX_TEAM_COUNTth position after the first.
        // in order to compute the proper range between the highest and lowest party assignable to different teams, but this is really goign to be done
        // when we replace this factor with TeamPartyPlayerPropertyMax which will ensure we are computing the gap between the top end and bottom teams.
        static_assert(2 == MAX_TEAM_COUNT, "Only 2 teams supported!");
    }
    const auto partyCount = game.getPartyCount(GameState::PRESENT);
    const auto partyLimit = eastl::min(partyCount, game.getTeamCount());
    EA_ASSERT(partyLimit > 0);
    if (partyLimit > 0)
    {
        const PropertyIndexer propIndexer = game.getPropertyIndexer(partyMaxSpec);
        PropertyValue high = 0, low = 0; // MAYBE: set low to factor.minValue?
        for (auto i = partyCount - 1; i >= 0; --i)
        {
            const auto iPartyIndex = propIndexer[i];
            if (game.getTeamIndexByPartyIndex(iPartyIndex, cxt.mGameState) != INVALID_TEAM_INDEX)
            {
                high = game.getPartyPropertyValue(cxt, iPartyIndex, PropertyAggregate::MAX);
                for (auto j = i - 1; j >= 0; --j)
                {
                    const auto jPartyIndex = propIndexer[j];
                    if (game.getTeamIndexByPartyIndex(jPartyIndex, cxt.mGameState) != INVALID_TEAM_INDEX)
                    {
                        low = game.getPartyPropertyValue(cxt, jPartyIndex, PropertyAggregate::MAX);
                        break;
                    }
                }
                break;
            }
        }

        cxt.mEvalResult = { (Score)high, (Score)low };
        if (cxt.mGameState != GameState::PRESENT)
        {
            const auto incomingPartyIndex = game.getIncomingPartyIndex();
            const auto propIndex = cxt.mPropertyFieldIndex;
            // TODO: we may want to precompute this somehow for the party
            const auto incomingPartyMaxPropValue = game.getPartyPropertyValue(cxt, incomingPartyIndex, PropertyAggregate::MAX);
            if (incomingPartyMaxPropValue > low)
                cxt.mEvalResult = { (Score)high, (Score)incomingPartyMaxPropValue };
        }
    }
    else
        EA_FAIL_MSG("Must have at least one player!");
    return cxt.computeScore();
#else
    EA_FAIL_MSG("Change the impl to use player index iteration, instead of party index iteration.");
    return INVALID_SCORE;
#endif
}

}

} // GamePacker


