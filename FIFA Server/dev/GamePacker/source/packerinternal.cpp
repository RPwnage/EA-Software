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

const char8_t* DEFAULT_PROPERTY_NAME = "Property";
const char8_t* GameQualityTransform::PROPERTY_FIELD_VALUE_PARTICIPATION_TOO_LOW_TEXT = "Property field value participation is too low."; // no players specify a value for this field


namespace GameSizeFactor {
    extern struct FactorTransform gTransform;
}
namespace TeamSizeFactor {
    extern struct FactorTransform gTransform;
}
namespace TeamFillFactor {
    extern struct FactorTransform gTransform;
}
namespace TeamPlayerPropertySumFactor {
    extern struct FactorTransform gTransform;
}
namespace GamePlayerPropertyFactor {
    extern struct FactorTransform gTransform;
}
namespace GamePlayerPropertyMaxFactor {
    extern struct FactorTransform gTransform;
}
namespace GamePlayerPropertyAvgFactor {
    extern struct FactorTransform gTransform;
}

// Necessary to avoid having the linker strip out factors
GameQualityTransform* gFactorTransforms[] = {
    reinterpret_cast<GameQualityTransform*>(&GameSizeFactor::gTransform),
    reinterpret_cast<GameQualityTransform*>(&TeamSizeFactor::gTransform),
    reinterpret_cast<GameQualityTransform*>(&TeamFillFactor::gTransform),
    reinterpret_cast<GameQualityTransform*>(&TeamPlayerPropertySumFactor::gTransform),
    reinterpret_cast<GameQualityTransform*>(&GamePlayerPropertyFactor::gTransform),
    reinterpret_cast<GameQualityTransform*>(&GamePlayerPropertyMaxFactor::gTransform),
    reinterpret_cast<GameQualityTransform*>(&GamePlayerPropertyAvgFactor::gTransform),
};

GameQualityFactorSpecList gAllFactorSpecs;      // PACKER_TODO - Globals are bad and you should feel bad for using them. 

GameQualityFactorSpecList& getAllFactorSpecs()
{
    return gAllFactorSpecs;
}


/**
* This method is used by factors to precompute mutable party size occupancy bitmasks from a game
*/
void GameQualityTransform::calculateMutablePartySizeOccupancyBits(PartySizeOccupancyBitset& partySizeOccupancyBits, const Game& game, int32_t partyCount)
{
    for (PartyIndex partyIndex = 0; partyIndex < partyCount; ++partyIndex)
    {
        auto& gameParty = game.mGameParties[partyIndex];
        if (gameParty.mImmutable)
            continue;
        auto playerCount = gameParty.mPlayerCount;
        auto sizeBitIndex = playerCount - 1;
        auto& partyBitset = mPartyBitsetByPartySize[sizeBitIndex];
        if (!partySizeOccupancyBits.test(sizeBitIndex))
        {
            partySizeOccupancyBits.set(sizeBitIndex);
            partyBitset.reset();
        }

        partyBitset.set(partyIndex); // add party to bitset
    }
}

bool GameQualityTransform::registerFactorSpec(GameQualityFactorSpec& spec)
{
    auto& allFactorSpecs = getAllFactorSpecs();
    allFactorSpecs.push_back(spec);
    return true;
}

} // GamePacker
