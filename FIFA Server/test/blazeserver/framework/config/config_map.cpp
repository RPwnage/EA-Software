/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ConfigMap

    The ConfigMap class represents the value of a configuration property which is comprised
    of a map of name-value pairs.  This allows a client to iterate over all pairs, or request them
    by name in a property which may look like: bingoMap = {B = 1, I = 16, N = 31, G = 46, O = 61}.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "EASTL/fixed_vector.h"
#include "framework/util/shared/blazestring.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


/*************************************************************************************************/
/*!
    \brief ConfigMap

    The default constructor.
*/
/*************************************************************************************************/
ConfigMap::ConfigMap() 
    : mItemMap(BlazeStlAllocator("ConfigMap::mItemMap", Blaze::MEM_GROUP_FRAMEWORK_CONFIG)), mIter(nullptr)
{
    resetIter();
}

ConfigMap::ConfigMap(const ConfigMap& configMap)
    : mIter(nullptr)
{
    mItemMap = configMap.mItemMap;
    resetIter();
}

ConfigMap::~ConfigMap()
{
}

ConfigMap *ConfigMap::clone() const
{
    ConfigMap *result = BLAZE_NEW_CONFIG ConfigMap(*this);
    return result;
}

ConfigSequence *ConfigMap::addSequence(const char8_t *propName) 
{
    ItemMap::insert_return_type ret = mItemMap.insert(propName);
    if (ret.second)
    {
        ret.first->second.setSequence();
        return ret.first->second.getSequence();
    }
    return nullptr;
}

ConfigMap *ConfigMap::addMap(const char8_t *propName)
{
    ItemMap::insert_return_type ret = mItemMap.insert(propName);
    if (ret.second)
    {
        ret.first->second.setMap();
        return ret.first->second.getMap();
    }
    return nullptr;
}

const char8_t *ConfigMap::addValue(const char8_t *propName, const char8_t *value) 
{
    ItemMap::insert_return_type ret = mItemMap.insert(propName);
    if (ret.second)
    {
        ret.first->second.setString(value);
        bool isDefault;
        return ret.first->second.getString(nullptr, isDefault);
    }
    return nullptr;
}

/*************************************************************************************************/
/*!
    \brief resetIter

    Reset the iterator to the beginning of the map.  This allows the client to begin iterating
    over the map keys via subsequent calls to nextKey().
*/
/*************************************************************************************************/
void ConfigMap::resetIter() const 
{
    mIter = mItemMap.begin();
}

/*************************************************************************************************/
/*!
    \brief hasNext

    Determine whether any keys remain in the iterator.

    \return - True if there are keys remaining, false otherwise
*/
/*************************************************************************************************/
bool ConfigMap::hasNext() const
{
    return (mIter != mItemMap.end());
}

/*************************************************************************************************/
/*!
    \brief nextKey

    Return the next string key in the iterator for the contained map.  No order is guaranteed
    when iterating over keys.

    \return - The next key or nullptr if none remain
*/
/*************************************************************************************************/
const char8_t* ConfigMap::nextKey() const
{
    const char8_t *result = nullptr;
    
    if (mIter != mItemMap.end())
    {
        result = mIter->first.c_str();
        ++mIter;
    }

    return result;
}

/*************************************************************************************************/
/*!
    \brief nextItem

    Return the next config item value in the iterator for the contained map.  No order is guaranteed
    when iterating over values.

    \return - The next values or nullptr if none remain
*/
/*************************************************************************************************/
const ConfigItem* ConfigMap::nextItem() const
{
    const ConfigItem* result = nullptr;
    
    if (mIter != mItemMap.end())
    {
        result = &mIter->second;
        ++mIter;
    }

    return result;
}

/*************************************************************************************************/
/*!
    \brief nextMappedItem

    Return the next config item value in the iterator for the contained map.  No order is guaranteed
    when iterating over values.

    \return - The next values or nullptr if none remain
*/
/*************************************************************************************************/
const ConfigMap::MappedItem* ConfigMap::nextMappedItem() const
{
    const MappedItem* result = nullptr;
    
    if (mIter != mItemMap.end())
    {
        result = &(*mIter);
        ++mIter;
    }

    return result;
}

/*************************************************************************************************/
/*!
    \brief isDefined

    Determine whether a property name exists in this config object.

    \param[in]  propName - name of the property to search for

    \return - True if the property is contained, false otherwise
*/
/*************************************************************************************************/
bool ConfigMap::isDefined(const char8_t* propName) const
{
    return getItem(propName) != nullptr;
}

/*************************************************************************************************/
/*!
    \brief isNumber

    Determines whether the string representation of a property is an int (number) that follows
    the format defined in the standard library strtol function.  If the property is not found or
    cannot be parsed then false returned instead.
    
    \param[in]  propName - name of the property to search for

    \return - Whether the property value is a number or not
*/
/*************************************************************************************************/
bool ConfigMap::isNumber(const char8_t* propName) const
{
    const ConfigItem *item = getItem(propName);
    return item != nullptr ? item->isNumber() : false;
}

/*************************************************************************************************/
/*!
    \brief getBool

    Retrieve a boolean property.  The string representation of a boolean property should be "true"
    or "false" (case-insensitive).  
    
    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found or unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
bool ConfigMap::getBool(const char8_t* propName, bool defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getBool(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}


/*************************************************************************************************/
/*!
    \brief getChar

    Retrieve a character property.  
    
    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found or unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
char8_t ConfigMap::getChar(const char8_t* propName, char8_t defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getChar(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}


/*************************************************************************************************/
/*!
    \brief getInt8

    Retrieve a 8-bit integer property.  The string representation of an int property should follow
    the format defined in the standard library strtol function.  If the property is not found or
    cannot be parsed then a caller supplied default value is returned instead.

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found or unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
int8_t ConfigMap::getInt8(const char8_t* propName, int8_t defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getInt8(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}

/*************************************************************************************************/
/*!
    \brief getInt16

    Retrieve a 16-bit integer property.  The string representation of an int property should follow
    the format defined in the standard library strtol function.  If the property is not found or
    cannot be parsed then a caller supplied default value is returned instead.

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found or unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
int16_t ConfigMap::getInt16(const char8_t* propName, int16_t defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getInt16(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}

/*************************************************************************************************/
/*!
    \brief getInt32

    Retrieve a 32-bit integer property.  The string representation of an int property should follow
    the format defined in the standard library strtol function.  If the property is not found or
    cannot be parsed then a caller supplied default value is returned instead.

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found or unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
int32_t ConfigMap::getInt32(const char8_t* propName, int32_t defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getInt32(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}

/*************************************************************************************************/
/*!
    \brief getInt64

    Retrieve a 64-bit integer property.  The string representation of an int property should follow
    the format defined in the standard library strtol function.  If the property is not found or
    cannot be parsed then a caller supplied default value is returned instead.

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found or unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
int64_t ConfigMap::getInt64(const char8_t* propName, int64_t defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getInt64(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}

/*************************************************************************************************/
/*!
    \brief getUInt8

    Retrieve a 8-bit unsigned integer property.  The string representation of an int property should follow
    the format defined in the standard library strtol function.  If the property is not found or
    cannot be parsed then a caller supplied default value is returned instead.

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found or unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
uint8_t ConfigMap::getUInt8(const char8_t* propName, uint8_t defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getUInt8(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}

/*************************************************************************************************/
/*!
    \brief getUInt16

    Retrieve a 16-bit unsigned integer property.  The string representation of an int property should follow
    the format defined in the standard library strtol function.  If the property is not found or
    cannot be parsed then a caller supplied default value is returned instead.

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found or unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
uint16_t ConfigMap::getUInt16(const char8_t* propName, uint16_t defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getUInt16(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}

/*************************************************************************************************/
/*!
    \brief getUInt32

    Retrieve a 32-bit unsigned integer property.  The string representation of an int property should follow
    the format defined in the standard library strtol function.  If the property is not found or
    cannot be parsed then a caller supplied default value is returned instead.

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found or unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
uint32_t ConfigMap::getUInt32(const char8_t* propName, uint32_t defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getUInt32(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}

/*************************************************************************************************/
/*!
    \brief getUInt64

    Retrieve a 64-bit unsigned integer property.  The string representation of an int property should follow
    the format defined in the standard library strtol function.  If the property is not found or
    cannot be parsed then a caller supplied default value is returned instead.

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found or unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
uint64_t ConfigMap::getUInt64(const char8_t* propName, uint64_t defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getUInt64(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}

/*************************************************************************************************/
/*!
    \brief getDouble

    Retrieve a 64-bit double property.  The string representation of a double property should follow
    the format defined in the standard library strtod function.  If the property is not found or
    cannot be parsed then a caller supplied default value is returned instead.

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found or unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
double_t ConfigMap::getDouble(const char8_t* propName, double_t defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getDouble(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}

/*************************************************************************************************/
/*!
    \brief getString

    Retrieve a string property.  If the property is not found then a caller    supplied default value
    is returned instead.

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
const char8_t *ConfigMap::getString(const char8_t* propName, const char8_t* defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getString(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}

/*************************************************************************************************/
/*!
    \brief getTime

    Retrieve a Time property, and append CURRENT date. If the property is not found then a caller
    supplied default value is returned instead. Format: "16:56:08".

    Caller is allowed to omit TRAILING components:
        16:56
        16
    But it may be preferable to make it more explicit:
        16:56:00
        16:00:00

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
TimeValue ConfigMap::getTime(const char8_t* propName, const TimeValue& defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getTime(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}

/*************************************************************************************************/
/*!
    \brief getDateTime

    Retrieve a DateTime property.  If the property is not found then a caller
    supplied default value is returned instead. Format: "2008/09/28-16:56:08"
    (interpreted as local time).

    Caller is allowed to omit TRAILING components:
        2008/09/28-16:56
        2008/09/28
    But it may be preferable to make it more explicit:
        2008/09/28-16:56:00
        2008/09/28-00:00:00

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
TimeValue ConfigMap::getDateTime(const char8_t* propName, const TimeValue& defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getDateTime(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }

}

/*************************************************************************************************/
/*!
    \brief getGMDateTime

    Retrieve a DateTime property.  If the property is not found then a caller
    supplied default value is returned instead. Format: "2008/09/28-16:56:08"
    (interpreted as GM time).

    Caller is allowed to omit TRAILING components:
        2008/09/28-16:56
        2008/09/28
    But it may be preferable to make it more explicit:
        2008/09/28-16:56:00
        2008/09/28-00:00:00

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
TimeValue ConfigMap::getGMDateTime(const char8_t* propName, const TimeValue& defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getGMDateTime(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }

}

/*************************************************************************************************/
/*!
    \brief getTimeInterval

    Retrieve a time interval property.  If the property is not found then a caller 
    supplied default value is returned instead. Format: "300d:17h:33m:59s:200ms".

    Caller can provide any combination of time components:
        300d:17h:33m
        300d:17h
        33m:59s
        300d
        17h
        33m
        59s
        200ms
    Finally the caller can 'overflow' the component's natural modulo if it is convenient:
        48h
        600m

    NOTE: The maximum valid number of digits in a time component is 10, i.e.: 1234567890s

    \param[in]  propName - name of the property to search for
    \param[in]  defaultValue - value to be returned if the property is not found
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
TimeValue ConfigMap::getTimeInterval(const char8_t* propName, const TimeValue& defaultValue, bool& isDefault) const
{
    const ConfigItem *item = getItem(propName);
    if (item != nullptr)
    {
        return item->getTimeInterval(defaultValue, isDefault);
    }
    else
    {
        isDefault = true;
        return defaultValue;
    }
}

bool ConfigMap::getBool(const char8_t* propName, bool defaultValue) const
{
    bool isDefault;
    return getBool(propName, defaultValue, isDefault);
}

int8_t ConfigMap::getInt8(const char8_t* propName, int8_t defaultValue) const
{
    bool isDefault;
    return getInt8(propName, defaultValue, isDefault);
}

int16_t ConfigMap::getInt16(const char8_t* propName, int16_t defaultValue) const
{
    bool isDefault;
    return getInt16(propName, defaultValue, isDefault);
}

int32_t ConfigMap::getInt32(const char8_t* propName, int32_t defaultValue) const
{
    bool isDefault;
    return getInt32(propName, defaultValue, isDefault);
}

int64_t ConfigMap::getInt64(const char8_t* propName, int64_t defaultValue) const
{
    bool isDefault;
    return getInt64(propName, defaultValue, isDefault);
}

uint8_t ConfigMap::getUInt8(const char8_t* propName, uint8_t defaultValue) const
{
    bool isDefault;
    return getUInt8(propName, defaultValue, isDefault);
}

uint16_t ConfigMap::getUInt16(const char8_t* propName, uint16_t defaultValue) const
{
    bool isDefault;
    return getUInt16(propName, defaultValue, isDefault);
}

uint32_t ConfigMap::getUInt32(const char8_t* propName, uint32_t defaultValue) const
{
    bool isDefault;
    return getUInt32(propName, defaultValue, isDefault);
}

uint64_t ConfigMap::getUInt64(const char8_t* propName, uint64_t defaultValue) const
{
    bool isDefault;
    return getUInt64(propName, defaultValue, isDefault);
}

double_t ConfigMap::getDouble(const char8_t* propName, double_t defaultValue) const
{
    bool isDefault;
    return getDouble(propName, defaultValue, isDefault);
}

const char8_t* ConfigMap::getString(const char8_t* propName, const char8_t* defaultValue) const
{
    bool isDefault;
    return getString(propName, defaultValue, isDefault);
}

TimeValue ConfigMap::getTime(const char8_t* propName, const TimeValue& defaultValue) const
{
    bool isDefault;
    return getTime(propName, defaultValue, isDefault);
}

TimeValue ConfigMap::getDateTime(const char8_t* propName, const TimeValue& defaultValue) const
{
    bool isDefault;
    return getDateTime(propName, defaultValue, isDefault);
}

TimeValue ConfigMap::getGMDateTime(const char8_t* propName, const TimeValue& defaultValue) const
{
    bool isDefault;
    return getGMDateTime(propName, defaultValue, isDefault);
}

TimeValue ConfigMap::getTimeInterval(const char8_t* propName, const TimeValue& defaultValue) const
{
    bool isDefault;
    return getTimeInterval(propName, defaultValue, isDefault);
}



/*************************************************************************************************/
/*!
    \brief getSequence

    Retrieve a sequence property.  The string representation of a sequence property should be "["
    followed by zero or more elements separated by commas and terminated by "]".  If the property
    is not found then nullptr is returned.

    \param[in]  propName - name of the property to search for

    \return - A pointer to a config sequence if the property is found, null otherwise
*/
/*************************************************************************************************/
const ConfigSequence* ConfigMap::getSequence(const char8_t* propName) const
{
    const ConfigItem *item = getItem(propName);
    return item != nullptr ? item->getSequence() : nullptr;
}

/*************************************************************************************************/
/*!
    \brief getMap

    Retrieve a map property.  The string representation of a map property should be "{"
    followed by zero or more pairs (of the form "name = value") separated by commas,
    and terminated by "}".  If the property is not found then nullptr is returned.

    \param[in]  propName - name of the property to search for

    \return - A pointer to a config map if the property is found, null otherwise
*/
/*************************************************************************************************/
const ConfigMap* ConfigMap::getMap(const char8_t* propName) const
{
    const ConfigItem *item = getItem(propName);
    return item != nullptr ? item->getMap() : nullptr;
}


bool ConfigMap::operator==(const ConfigMap& rhs) const
{
    if (this == &rhs)
        return true;

    if (mItemMap.size() != rhs.mItemMap.size())
        return false;

    // compare the map
    ItemMapIterator thisMapItr = mItemMap.begin();
    ItemMapIterator thisMapEnd = mItemMap.end();

   // Check that both maps have the same number of arguments and that
    // the keys are equal and the values are equal.  As soon as we hit
    // something that is not equal, return false
    for (; thisMapItr != thisMapEnd; ++thisMapItr)
    {
        ItemMapIterator rhsMapItr = rhs.mItemMap.find(thisMapItr->first);
        if (rhsMapItr == rhs.mItemMap.end() || !(thisMapItr == rhsMapItr))
        {
            return false;
        }        
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief getItem

    Parse a dotted-decimal property map and use it to traverse down nested maps.  Return the
    found item.  For example, a property name of "one.two.three.four" would
    traverse the following config file:

    one = {
        two = {
            three = {
                four = "some value"
            }
        }
    }

    The item corresponding to four would be return.

    Note that the common, non-dotted case has a special branch.

    \param propName - Property name to lookup.
   
    \return - The item if found, nullptr otherwise.
*/
/*************************************************************************************************/
const ConfigItem* ConfigMap::getItem(const char8_t* propName) const
{
    //early out on an empty string
    if ( (propName == nullptr) || (propName[0] == '\0') )
        return nullptr;

    const ConfigItem* item = nullptr;
    ItemMap::const_iterator it = mItemMap.find(propName);
    if (it != mItemMap.end())
    {
        item = &it->second;
    }
    else
    {
        const char8_t* dot = strchr(propName, '.');
        if (dot != nullptr)
        {
            eastl::string namePrefix(propName, dot);
            ItemMap::const_iterator itr = mItemMap.find(namePrefix);
            if (itr != mItemMap.end())
            {
                const ConfigMap* subMap = itr->second.getMap();
                if (subMap != nullptr)
                    item = subMap->getItem(dot+1);
            }
        }
    }

    return item;
}

/*************************************************************************************************/
/*!
    \brief upsertItem

    Parse a dotted-decimal property map and use it to set the corresponding property within the
    innermost of the nested maps.  Return true if succesfully inserted the value, false otherwise.
    found item. For example, a property name of one.two.three.four = "some value" would be applied 
    as follows:

    Property to upsert:

    one.two.three.four = "some value"

    ConfigMap before upsert:

    one = {
        two = {
            three = {
                four = "a"
                five = "b"
            }
        }
    }

    ConfigMap after upsert:

    one = {
        two = {
            three = {
                four = "some value"
                five = "b"
            }
        }
    }

    Note that the common, non-dotted case has a special branch.
    Also note that upsert destructively updates the ConfigMap if necessary, 
    in order to create the full nested structure required by the incoming property.

    \param propName - Property name to lookup.
   
    \return - true if succesfully inserted the item, false otherwise.
*/
/*************************************************************************************************/
bool ConfigMap::upsertItem(const char8_t* propName, const ConfigItem& item)
{
    //early out on an empty string
    if ( (propName == nullptr) || (propName[0] == '\0') )
        return false;

    bool result = false;
    const char8_t* dot = strchr(propName, '.');
    if (dot == nullptr)
    {
        mItemMap[propName] = item;
        result = true;
    }
    else
    {
        eastl::string namePrefix(propName, dot);
        ConfigItem& subItem = mItemMap[namePrefix];
        if (subItem.getItemType() != ConfigItem::CONFIG_ITEM_MAP)
            subItem.setMap(); // convert to map
        ConfigMap* subMap = subItem.getMap();
        if (subMap != nullptr)
            result = subMap->upsertItem(dot+1, item);
    }

    return result;
}

/*************************************************************************************************/
/*! \brief Dumps the contents of the map to a string
 *************************************************************************************************/
void ConfigMap::toString(eastl::string &output, uint32_t indentSize) const
{
    const uint32_t INDENT = 4;

    for (ItemMapIterator itr = mItemMap.begin(), end = mItemMap.end(); itr != end; ++itr)
    {
        const ConfigItem* item = &itr->second;
        switch (item->getItemType())
        {
            case ConfigItem::CONFIG_ITEM_MAP:
            {
                output.append(indentSize, ' ');
                output.append_sprintf("%s = {\n", itr->first.c_str());

                item->getMap()->toString(output, indentSize + INDENT);

                output.append(indentSize, ' ');
                output.append("}\n");
                break;
            }
            case ConfigItem::CONFIG_ITEM_SEQUENCE:
            {
                output.append(indentSize, ' ');
                output.append_sprintf("%s = [\n", itr->first.c_str());

                item->getSequence()->toString(output, indentSize + INDENT);

                output.append(indentSize, ' ');
                output.append("]\n");
                break;
            }
            case ConfigItem::CONFIG_ITEM_STRING:
            {
                output.append(indentSize, ' ');
                output.append(itr->first);
                ItemMapIterator nextItr = itr;
                ++nextItr;
                bool isDefault;
                output.append_sprintf(" = \"%s\"%s\n", item->getString("", isDefault), (nextItr != end) ? "," : "");
                break;
            }
            default:
            break;
        }
    }
}

} // Blaze
