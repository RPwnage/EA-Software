/*************************************f************************************************************/
/*!
    \file uedutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

#include "uedutil.h"
#include "framework/config/config_map.h"
#include "framework/rpc/usersessionsslave.h"
#include "framework/util/random.h" // for Random in parseClientUEDSettings() and initClientUED()
#include "framework/config/config_sequence.h"

namespace Blaze
{
namespace Stress
{

// parse and validate config
bool UEDUtil::parseClientUEDSettings( const ConfigMap& config, const size_t& logCat )
{
    const Blaze::ConfigSequence *uedSeq = config.getSequence("clientUED");

    while((uedSeq != nullptr) && uedSeq->hasNext())
    {
        const Blaze::ConfigMap *uedMap = uedSeq->nextMap();

        mClientUEDList.push_back();
        ClientUEDStruct& newUED = mClientUEDList.back();

        // UED key value from Usersession.cfg
        newUED.mKey = uedMap->getUInt16("key", 0);
        BLAZE_INFO_LOG(logCat, "[UEDUtil].parseClientUEDSettings: UED Key = " << newUED.mKey);

        newUED.mMinValue = uedMap->getInt64("minValue", 0);
        BLAZE_INFO_LOG(logCat, "[UEDUtil].parseClientUEDSettings: UED MinValue = " << newUED.mMinValue);
        newUED.mMaxValue = uedMap->getInt64("maxValue", 0);
        BLAZE_INFO_LOG(logCat, "[UEDUtil].parseClientUEDSettings: UED MaxValue = " << newUED.mMaxValue);

        // Validate min/max values
        const UserExtendedDataValue uedRange = (newUED.mMaxValue - newUED.mMinValue);
        if (newUED.mMinValue > newUED.mMaxValue)
        {
            BLAZE_ERR_LOG(logCat, "[UEDUtil].parseClientUEDSettings: Client UED min " << newUED.mMinValue << ", is greater than the max " << newUED.mMaxValue << "."); 
            return false;
        }
        if ((uedRange+1) > Random::MAX)
        {
            BLAZE_ERR_LOG(logCat, "[UEDUtil].parseClientUEDSettings: Client UED range (" << uedRange << ") is too large to generate a random number with. ued min: " << newUED.mMinValue << ", max: " << newUED.mMaxValue << ". Max allowed difference is " << Random::MAX); 
            return false;
        }

        // Advanced settings

        //value between min and max, representing top players (possibly rare)
        newUED.mEliteLevelMin = uedMap->getInt64("eliteLevelMin", 0);
        BLAZE_INFO_LOG(logCat, "[UEDUtil].parseClientUEDSettings: UED EliteLevelMin = " << newUED.mEliteLevelMin);
        newUED.mEliteLevelChance = uedMap->getInt8("eliteLevelChance", 0);
        BLAZE_INFO_LOG(logCat, "[UEDUtil].parseClientUEDSettings: UED EliteLevelChance = " << newUED.mEliteLevelChance);

        //value between min and max, representing mid level players
        newUED.mAdvancedLevelMin = uedMap->getInt64("advancedLevelMin", 0);
        BLAZE_INFO_LOG(logCat, "[UEDUtil].parseClientUEDSettings: UED AdvancedLevelMin = " << newUED.mAdvancedLevelMin);
        newUED.mAdvancedLevelChance = uedMap->getInt8("advancedLevelChance", 0);
        BLAZE_INFO_LOG(logCat, "[UEDUtil].parseClientUEDSettings: UED AdvancedLevelChance = " << newUED.mAdvancedLevelChance);

        const bool eliteEnabled = (newUED.mEliteLevelChance > 0);
        const bool advanceEnabled = (newUED.mAdvancedLevelChance > 0);

        //if level enabled, must be in [sClientUEDMaxValue,sClientUEDMinValue]
        if (eliteEnabled && ((newUED.mEliteLevelMin > newUED.mMaxValue) || (newUED.mEliteLevelMin < newUED.mMinValue)))
        {
            BLAZE_ERR_LOG(logCat, "[UEDUtil].parseClientUEDSettings: Client UED clientUEDEliteLevelMin " << newUED.mEliteLevelMin << ", must be in configured range [" << newUED.mMinValue << "," << newUED.mMaxValue << "]."); 
            return false;
        }
        //if level enabled, must be in [sClientUEDMaxValue,sClientUEDMinValue]
        if (advanceEnabled && ((newUED.mAdvancedLevelMin > newUED.mMaxValue) || (newUED.mAdvancedLevelMin < newUED.mMinValue)))
        {
            BLAZE_ERR_LOG(logCat, "[UEDUtil].parseClientUEDSettings: Client UED clientUEDAdvancedLevelMin " << newUED.mAdvancedLevelMin << ", must be in configured range [" << newUED.mMinValue << "," << newUED.mMaxValue << "]."); 
            return false;
        }
        // percent chances must be <= 100. the sum of chances must be <= 100
        if (newUED.mEliteLevelChance + newUED.mAdvancedLevelChance > 100)
        {
            BLAZE_ERR_LOG(logCat, "[UEDUtil].parseClientUEDSettings: Client UED clientUEDEliteLevelChance " << newUED.mEliteLevelChance << ", and clientUEDAdvancedLevelChance " <<
                newUED.mAdvancedLevelChance << " must be no greater than the max of 100, and, their sum " << (newUED.mEliteLevelChance + newUED.mAdvancedLevelChance) << " must be <= 100."); 
            return false;
        }
        // elite level must be greater than advanced
        if (eliteEnabled && advanceEnabled && (newUED.mEliteLevelMin <= newUED.mAdvancedLevelMin))
        {
            BLAZE_ERR_LOG(logCat, "[UEDUtil].parseClientUEDSettings: Client UED clientUEDEliteLevelMin " << newUED.mEliteLevelMin << ", must be greater than configured clientUEDAdvancedLevelMin " << newUED.mAdvancedLevelMin << "."); 
            return false;
        }

    }

    return true;
}


// Generates random value in the range [sClientUEDMinValue, sClientUEDMaxValue] and sets as the user's client UED value for the key.
bool UEDUtil::initClientUED(Blaze::UserSessionsSlave *userSessionsSlave, const size_t& logCat) const
{
    if (!isClientUEDEnabled())
        return true;

    if (userSessionsSlave == nullptr)
    {
        BLAZE_ERR_LOG(logCat, "[UEDUtil].initClientUED: User sessions slave nullptr, cannot complete request."); 
        return false;
    }

    ClientUEDList::const_iterator iter = mClientUEDList.begin();
    for (; iter != mClientUEDList.end(); ++iter)
    {
        const ClientUEDStruct& clientUED = *iter;
        const bool eliteEnabled = (clientUED.mEliteLevelChance > 0);
        const bool advanceEnabled = (clientUED.mAdvancedLevelChance > 0);

        // determine rollable value range by whether elite, advanced, or other
        uint8_t levelRoll = (uint8_t)Random::getRandomNumber(100);
        UserExtendedDataValue rolledClientUEDMinValue;
        UserExtendedDataValue rolledClientUEDMaxValue;
        if (levelRoll < clientUED.mEliteLevelChance)
        {
            rolledClientUEDMinValue = clientUED.mEliteLevelMin; //rolled elite
            rolledClientUEDMaxValue = clientUED.mMaxValue;
        }
        else if (levelRoll < (clientUED.mEliteLevelChance + clientUED.mAdvancedLevelChance))
        {
            rolledClientUEDMinValue = clientUED.mAdvancedLevelMin; //rolled advanced
            rolledClientUEDMaxValue = (eliteEnabled? clientUED.mEliteLevelMin : clientUED.mMaxValue);
        }
        else
        {
            rolledClientUEDMinValue = clientUED.mMinValue; //rolled normal
            rolledClientUEDMaxValue = (eliteEnabled? clientUED.mEliteLevelMin : clientUED.mMaxValue);
            rolledClientUEDMaxValue = (advanceEnabled? clientUED.mAdvancedLevelMin : rolledClientUEDMaxValue);
        }

        const UserExtendedDataValue uedRange = (rolledClientUEDMaxValue - rolledClientUEDMinValue);
        if ((rolledClientUEDMinValue > rolledClientUEDMaxValue) || ((uedRange+1) > Random::MAX))
        {
            BLAZE_ERR_LOG(logCat, "[UEDUtil].initClientUED: Invalid inputs, cannot complete request. Were inputs properly parsed and set?"); 
            return false;
        }

        UserExtendedDataValue uedValue = Random::getRandomNumber((int)(uedRange+1)) + rolledClientUEDMinValue;
        UpdateExtendedDataAttributeRequest request;
        request.setRemove(false);
        request.setKey(clientUED.mKey);
        request.setComponent(0);    // INVALID_COMPONENT_ID
        request.setValue(uedValue);

        BlazeRpcError uedErr = userSessionsSlave->updateExtendedDataAttribute(request);
        if (uedErr != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(logCat, "[UEDUtil].initClientUED: UpdateExtendedDataAttribute failed on error " << uedErr << ". See server logs for details."); 
        }
    }

    return true;
}

} // End Stress namespace
} // End Blaze namespace

