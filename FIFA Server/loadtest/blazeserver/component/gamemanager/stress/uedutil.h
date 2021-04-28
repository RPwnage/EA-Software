/*************************************************************************************************/
/*!
    \file uedutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_UED_UTIL
#define BLAZE_STRESS_UED_UTIL

#include "framework/tdf/userextendeddatatypes.h"

namespace Blaze
{
    class ConfigMap;
    class UserSessionsSlave;

namespace Stress
{

// Adds user's Client Specified UED. Note: for use with MM TeamUEDBalanceRule, UEDRule etc, you must ensure the UED key
// and name in the usersessions.cfg matches up with the appropriate matchmaker_settings.cfg's rule's userExtendedDataName.
class UEDUtil
{
public:
    bool parseClientUEDSettings( const ConfigMap& config, const size_t& logCat );
    bool initClientUED(Blaze::UserSessionsSlave *userSessionsSlave, const size_t& logCat) const;

private:

    // pre: already parsed
    bool isClientUEDEnabled() const { return !mClientUEDList.empty(); }

    struct ClientUEDStruct
    {
        uint16_t mKey;    
        UserExtendedDataValue mMinValue;
        UserExtendedDataValue mMaxValue;
        UserExtendedDataValue mEliteLevelMin;     //value between min and max, representing top players (possibly rare)
        UserExtendedDataValue mAdvancedLevelMin;  //value between min and max, representing mid level players

        // The below 'chances' must be sum to <= 100. The remainder of the sum is considered non-advanced/elite player.
        uint8_t mEliteLevelChance;    //chance (0 to 100) of rolling player being elite. Its ued/skill will be above sClientUEDEliteLevelMin.
        uint8_t mAdvancedLevelChance; //chance (0 to 100) of rolling player being advanced. Its ued/skill will be above sClientUEDAdvancedLevelMin.
    };

    typedef eastl::vector<ClientUEDStruct> ClientUEDList;
    ClientUEDList mClientUEDList;
};



} // End Stress namespace
} // End Blaze namespace

#endif // BLAZE_STRESS_UED_UTIL

