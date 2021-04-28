/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef GAME_PACKER_INTERNAL_H
#define GAME_PACKER_INTERNAL_H

#include "gamepacker/common.h"
#include "gamepacker/evalcontext.h"

#define PACKER_PROPERTY_FIELD_TOKEN "%s[%s]"

namespace Packer
{

struct Game;
struct GameQualityFactorSpec;

struct GameQualityTransform
{
    /**
     *  This game quality factor callback shall attempt to modify the game while taking care to respect pending modifications
     *  already performed by preceding factors. It shall return the estimated components of the improvement/deimprovement in 'factor'
     *  or false if no valid modification that would incorporate incomingParty into the game is possible.
     */
    virtual Score activeEval(EvalContext& cxt, Game& game) = 0;

    /**
     *  This game quality factor callback shall evaluate the game while taking care to respect pending modifications
     *  already performed by preceding factors. It shall return the estimated improvement/deimprovement in 'factor'
     *  or false if no valid modification that would incorporate incomingParty into the game is possible.
     */
    virtual Score passiveEval(EvalContext& cxt, const Game& game) const = 0;

    /**
     * This method is used by factors to precompute party size occupancy bitmasks from a game
     */
    void calculateMutablePartySizeOccupancyBits(PartySizeOccupancyBitset& partySizeOccupancyBits, const Game& game, int32_t partyCount);

    /**
     *  Used by derived transforms to register into the list of specs
     */
    static bool registerFactorSpec(GameQualityFactorSpec& spec);

    static const char8_t* PROPERTY_FIELD_VALUE_PARTICIPATION_TOO_LOW_TEXT;

    PartyBitset mPartyBitsetByPartySize[MULTI_TEAM_MAX_PARTY_SIZE_COUNT]; // used by factors to classify parties by party size
};

}
#endif