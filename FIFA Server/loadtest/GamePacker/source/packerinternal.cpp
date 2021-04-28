/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "gamepacker/internal/packerinternal.h"

namespace Packer
{

const char8_t* DEFAULT_PROPERTY_NAME = "Property";

namespace GameSizeFactor {
    extern struct FactorTransform gTransform;
}
namespace TeamSizeFactor {
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

bool GameQualityTransform::registerFactorSpec(GameQualityFactorSpec& spec)
{
    auto& allFactorSpecs = getAllFactorSpecs();
    allFactorSpecs.push_back(spec);
    return true;
}

} // GamePacker
