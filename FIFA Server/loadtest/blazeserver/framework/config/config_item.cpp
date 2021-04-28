/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class ConfigItem


*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/config/config_item.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/locales.h"

namespace Blaze
{
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/



/*** Private Methods *****************************************************************************/

/*************************************************************************************************/
/*!
    \brief ~ConfigPair

    The constructor.
*/
/*************************************************************************************************/
ConfigPair::ConfigPair(const ConfigItem& first, char8_t separator, const ConfigItem& second)
{
    mFirst = first;
    mSecond = second;
    mSeparator = separator;

    bool def;
    mFullString.append_sprintf("%s%c%s", first.getString("", def), separator, second.getString("", def));
}
ConfigPair::ConfigPair(const char8_t* first, char8_t separator, const char8_t* second)
{
    mFirst.setString(first);
    mSecond.setString(second);
    mSeparator = separator;
    mFullString.append_sprintf("%s%c%s", first, separator, second);
}

bool ConfigPair::operator==(const ConfigPair& rhs) const
{
    return mFirst == rhs.mFirst;
}

/*************************************************************************************************/
/*!
    \brief ~ConfigPair

    The destructor.
*/
/*************************************************************************************************/
ConfigPair::~ConfigPair()
{
}

/*************************************************************************************************/
/*!
    \brief ~ConfigItem

    The destructor.
*/
/*************************************************************************************************/
ConfigItem::~ConfigItem()
{
    clear();
}

void ConfigItem::setString(const char8_t *value)
{
    clear();
    mType = CONFIG_ITEM_STRING;
    mValue.str = BLAZE_NEW_CONFIG eastl::string(value);
}

void ConfigItem::setMap()
{
    clear();
    mType = CONFIG_ITEM_MAP;
    mValue.map = BLAZE_NEW_CONFIG ConfigMap();
}

void ConfigItem::setSequence()
{
    clear();
    mType = CONFIG_ITEM_SEQUENCE;
    mValue.seq = BLAZE_NEW_CONFIG ConfigSequence();
}

void ConfigItem::setPair(const char8_t *first, char8_t separator, const char8_t *second)
{
    clear();
    mType = CONFIG_ITEM_PAIR;
    mValue.pair = BLAZE_NEW_CONFIG ConfigPair(first, separator, second);
}

void ConfigItem::clear()
{
    switch(mType)
    {
        case CONFIG_ITEM_STRING: delete mValue.str; break;
        case CONFIG_ITEM_MAP: delete mValue.map; break;
        case CONFIG_ITEM_SEQUENCE: delete mValue.seq; break;
        case CONFIG_ITEM_PAIR: delete mValue.pair; break;
        default: break; //nothing 
    };

    mType = CONFIG_ITEM_NONE;
}

/*************************************************************************************************/
/*!
    \brief isNumber

    Determines whether the string representation of a property is an int (number) that follows
    the    format defined in the standard library strtol function.

    \return - Whether the property value is a number or not
*/
/*************************************************************************************************/
bool ConfigItem::isNumber() const
{
    return (mType == CONFIG_ITEM_STRING && isNumber(mValue.str->c_str()));
}

/*************************************************************************************************/
/*!
    \brief getBool

    Retrieve a boolean value.  The string representation of a boolean property should be "true"
    or "false" (case-insensitive).

    \param[in]  defaultValue - value to be returned if the property is unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
bool ConfigItem::getBool(bool defaultValue, bool& isDefault) const
{    
    bool result = defaultValue;
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        isDefault = false;
        result = (blaze_stricmp(mValue.str->c_str(), "true") == 0);
    }
    return result;
}

/*************************************************************************************************/
/*!
    \brief getChar

    Retrieve a character value.  

    \param[in]  defaultValue - value to be returned if the property is unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
char8_t ConfigItem::getChar(char8_t defaultValue, bool& isDefault) const
{
    char8_t result = defaultValue;
    isDefault = true;
    if (mType == CONFIG_ITEM_STRING)
    {
        if (mValue.str->length() > 1)
        {
            BLAZE_WARN_LOG(Log::CONFIG, "[ConfigItem].getChar: more than one character detected (" << defaultValue << ") will be used.");
            result = defaultValue;
        }
        else
        {
            result = mValue.str->at(0);
            isDefault = false;
        }
    }
    return result;
}

/*************************************************************************************************/
/*!
    \brief getIntegerFromString

    Retrieve an int value from a string.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  str - string that contains number chars.
    \param[in]  defaultValue - value to be returned if the property is unparseable.
    \param[out]  isDefault - true if the default value was used, false otherwise.
    \param[in]  isSigned - whether the template type is signed int.
    \param[in]  bits - how many bits does the template type use.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
template <class T>
T ConfigItem::getIntegerFromString(const char8_t* str, T defaultValue, bool& isDefault, bool isSigned, uint8_t bits)
{
    T result = defaultValue;
    isDefault = true;
    if (ConfigItem::isNumber(str) && (isSigned || str[0] != '-'))
    {
        if (blaze_str2int(str, &result) == str)
        {
            BLAZE_WARN_LOG(Log::CONFIG, "[ConfigItem].getIntegerFromString: Overflow detected, default value(" << defaultValue << ") will be used.");
            result = defaultValue;
        }
        else
            isDefault = false;
    }
    else 
    {
        //  treat "true" and "false" as tokens for 1 and 0 respectively.  for example a bool config item translates to int8_t for gnuc++
        if (blaze_stricmp(str, "true")==0)
        {
            result = 1;
            isDefault = false;
        }
        //  but we don't want to treat any non-true value as "false" 
        //  (i.e. "configItemInt8 = nonumber" shouldn't mean "configItemInt8=0" but instead should mean "configItemInt8=<defaultValue>"
        else if (blaze_stricmp(str, "false")==0)
        {
            result = 0;
            isDefault = false;
        }
    }
    return result;
}

/*************************************************************************************************/
/*!
    \brief getInt8

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
int8_t ConfigItem::getInt8(int8_t defaultValue, bool& isDefault) const
{
    int8_t result = defaultValue;
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        result = getIntegerFromString<int8_t>(mValue.str->c_str(), defaultValue, isDefault, true, 8);
    }
    return result;
}

/*************************************************************************************************/
/*!
    \brief getInt16

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
int16_t ConfigItem::getInt16(int16_t defaultValue, bool& isDefault) const
{
    int16_t result = defaultValue;
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        result = getIntegerFromString<int16_t>(mValue.str->c_str(), defaultValue, isDefault, true, 16);
    }
    return result;
}

/*************************************************************************************************/
/*!
    \brief getInt32

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
int32_t ConfigItem::getInt32(int32_t defaultValue, bool& isDefault) const
{
    int32_t result = defaultValue;
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        result = getIntegerFromString<int32_t>(mValue.str->c_str(), defaultValue, isDefault, true, 32);
    }
    return result;
}

/*************************************************************************************************/
/*!
    \brief getInt64

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
int64_t ConfigItem::getInt64(int64_t defaultValue, bool& isDefault) const
{
    int64_t result = defaultValue;
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        result = getIntegerFromString<int64_t>(mValue.str->c_str(), defaultValue, isDefault, true, 64);
    }
    
    return result;
}

/*************************************************************************************************/
/*!
    \brief getUInt8

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
uint8_t ConfigItem::getUInt8(uint8_t defaultValue, bool& isDefault) const
{
    uint8_t result = defaultValue;
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        result = getIntegerFromString<uint8_t>(mValue.str->c_str(), defaultValue, isDefault, false, 8);
    }

    return result;
}

/*************************************************************************************************/
/*!
    \brief getUInt16

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
uint16_t ConfigItem::getUInt16(uint16_t defaultValue, bool& isDefault) const
{
    uint16_t result = defaultValue;
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        result = getIntegerFromString<uint16_t>(mValue.str->c_str(), defaultValue, isDefault, false, 16);
    }

    return result;
}

/*************************************************************************************************/
/*!
    \brief getUInt32

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
uint32_t ConfigItem::getUInt32(uint32_t defaultValue, bool& isDefault) const
{
    uint32_t result = defaultValue;
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        result = getIntegerFromString<uint32_t>(mValue.str->c_str(), defaultValue, isDefault, false, 32);
    }

    return result;
}

/*************************************************************************************************/
/*!
    \brief getUInt64

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
uint64_t ConfigItem::getUInt64(uint64_t defaultValue, bool& isDefault) const
{
    uint64_t result = defaultValue;
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        result = getIntegerFromString<uint64_t>(mValue.str->c_str(), defaultValue, isDefault, false, 64);
    }

    return result;
}

/*************************************************************************************************/
/*!
    \brief getDouble

    Retrieve a double value.  The string representation of a double property should follow
    the    format defined in the standard library strtod function.

    \param[in]  defaultValue - value to be returned if the property is unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
double_t ConfigItem::getDouble(double_t defaultValue, bool& isDefault) const
{
    double_t result = defaultValue;
    isDefault = true;
    if (mType == CONFIG_ITEM_STRING && isNumber(mValue.str->c_str()))
    {
        isDefault = false;
        result = strtod(mValue.str->c_str(), nullptr);
    }

    return result;
}

/*************************************************************************************************/
/*!
    \brief getString

    Retrieve a string value.

    \param[in]  defaultValue - value to be returned if the property is unparseable
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
const char8_t* ConfigItem::getString(const char8_t* defaultValue, bool& isDefault) const
{
    const char8_t* result = defaultValue;
    isDefault = true;
    
    if ( mType == CONFIG_ITEM_STRING )
    {
        isDefault = false;
        result = mValue.str->c_str();
    }
    else if (mType == CONFIG_ITEM_PAIR)
    {
        isDefault = false;
        result = mValue.pair->asString();
    }
    
    return result;
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
    
    \param[in]  defaultValue - value to be returned if the parse fails
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
TimeValue ConfigItem::getTime(const TimeValue& defaultValue, bool& isDefault) const
{    
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        TimeValue val;
        if (val.parseLocalTime(mValue.str->c_str()))
        {
            isDefault = false;
            return val;
        }
        else
        {
            return defaultValue;
        }
    }

    return defaultValue;
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

    \param[in]  defaultValue - value to be returned if the parse fails
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
TimeValue ConfigItem::getDateTime(const TimeValue& defaultValue, bool& isDefault) const
{
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        TimeValue val;
        if (val.parseLocalDateTime(mValue.str->c_str()))
        {
            isDefault = false;
            return val;
        }
        else
        {
            return defaultValue;
        }
    }

    return defaultValue;
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

    \param[in]  defaultValue - value to be returned if the parse fails
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
TimeValue ConfigItem::getGMDateTime(const TimeValue& defaultValue, bool& isDefault) const
{
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        TimeValue val;
        if (val.parseGmDateTime(mValue.str->c_str()))
        {
            isDefault = false;
            return val;
        }
        else
        {
            return defaultValue;
        }
    }

    return defaultValue;
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

    \param[in]  defaultValue - value to be returned if the parse fails
    \param[out]  isDefault - true if the default value was used, false otherwise.

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
TimeValue ConfigItem::getTimeInterval(const TimeValue& defaultValue, bool& isDefault) const
{
    isDefault = true;
    if ( mType == CONFIG_ITEM_STRING )
    {
        TimeValue val;
        if (val.parseTimeAllFormats(mValue.str->c_str()))
        {
            isDefault = false;
            return val;
        }
        else
        {
            return defaultValue;
        }
    }

    return defaultValue;
}


/*************************************************************************************************/
/*!
    \brief getSequence

    Retrieve a sequence value.

    \return - The property value or null
*/
/*************************************************************************************************/
const ConfigSequence* ConfigItem::getSequence() const
{
    const ConfigSequence* result = nullptr;
    if (mType == CONFIG_ITEM_SEQUENCE)
    {
        result = mValue.seq;
        result->resetIter();
    }
    return result;
}

ConfigSequence* ConfigItem::getSequence()
{
    ConfigSequence* result = nullptr;
    if (mType == CONFIG_ITEM_SEQUENCE)
    {
        result = mValue.seq;
        result->resetIter();
    }
    return result;
}

/*************************************************************************************************/
/*!
    \brief getMap

    Retrieve a map value.

    \return - The property value or null
*/
/*************************************************************************************************/
const ConfigMap* ConfigItem::getMap() const
{
    const ConfigMap* result = nullptr;
    if (mType == CONFIG_ITEM_MAP)
    {
        result = mValue.map;
        result->resetIter();  //Whenever we return a map or sequence, by convention its reset.
    }

    return result;
}

ConfigMap* ConfigItem::getMap()
{
    ConfigMap* result = nullptr;
    if (mType == CONFIG_ITEM_MAP)
    {
        result = mValue.map;
        result->resetIter();
    }

    return result;
}

const ConfigPair* ConfigItem::getPair() const
{
    ConfigPair* result = nullptr;
    if (mType == CONFIG_ITEM_PAIR)
    {
        result = mValue.pair;
    }

    return result;
}

bool ConfigItem::operator==(const ConfigItem& rhs) const
{
    if (this == &rhs)
        return true;

    if (mType != rhs.mType)
        return false;

    switch(mType)
    {
        case CONFIG_ITEM_STRING: return *mValue.str == *rhs.mValue.str; 
        case CONFIG_ITEM_MAP: return *mValue.map == *rhs.mValue.map; 
        case CONFIG_ITEM_SEQUENCE: return *mValue.seq == *rhs.mValue.seq;
        case CONFIG_ITEM_PAIR: return *mValue.pair == *rhs.mValue.pair;
        case CONFIG_ITEM_NONE: return true; 
    };

    return false;
}

ConfigItem &ConfigItem::operator=(const ConfigItem& rhs)
{
    //early out for same
    if (this == &rhs)
        return *this;

    clear();

    switch(rhs.mType)
    {
        case CONFIG_ITEM_STRING: mValue.str = BLAZE_NEW_CONFIG eastl::string(*rhs.mValue.str); break;
        case CONFIG_ITEM_MAP: mValue.map = rhs.mValue.map->clone(); break;
        case CONFIG_ITEM_SEQUENCE: mValue.seq = rhs.mValue.seq->clone(); break;
        case CONFIG_ITEM_PAIR: mValue.pair = BLAZE_NEW_CONFIG ConfigPair(rhs.mValue.pair->getFirst(), rhs.mValue.pair->getSeparator(), rhs.mValue.pair->getSecond());
        case CONFIG_ITEM_NONE: break; //Nothing to do
        default: BLAZE_WARN_LOG(Log::CONFIG, "ConfigItem::operator=: Invalid config item type, potential memory stomp.");  break;
    };
    mType = rhs.mType;

    return *this;
}

/*************************************************************************************************/
/*!
    \brief isNumber

    Test string to determine if it is a number.  Will look for standard positive or negative decimal 
    numeric sequences, hexidecimal sequences using the 0x syntax, or floating poing sequences using
    the nnnn.nnn syntax.

    \param[in]  value - nullptr-terminated string to test 

    \return - true of entire string is a number, false otherwise
*/
/*************************************************************************************************/
bool ConfigItem::isNumber(const char8_t* value) 
{
    if ( value == nullptr || *value == '\0' )
    {
        return false;
    }

    bool isHex = (value[0] == '0' && LocaleTokenToLower(value[1]) == 'x'); // Allow hex characters
    uint32_t numDigits = 0;                                     // Not valid until we see at least 1 digit

    if (isHex == true)
    {
        // We've already seen the hex signature, so start at the 3rd character
        for (uint32_t i = 2 ; value[i] != '\0' ; ++i)
        {
            if ((value[i] >= '0' && value[i] <= '9') ||
                (LocaleTokenToLower(value[i]) >= 'a' && LocaleTokenToLower(value[i]) <= 'f'))
            {
                ++numDigits;
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        bool seenDecimalPlace = false;

        // Decimal or floating point
        for (uint32_t i = 0 ; value[i] != '\0' ; ++i)
        {
            if (value[i] >= '0' && value[i] <= '9')
            {
                ++numDigits;
            }
            else if (value[i] == '-' && i == 0)
            {
                /*empty*/;           // First character can be a minus, but it doesn't count as a digit.
            }
            else if (value[i] == '.' && seenDecimalPlace == false)
            {
                seenDecimalPlace = true;        // Hey, it's floating point!  Decimal doesn't count as a digit though.
            }
            else
            {
                return false;
            }
        }
    }

    // All other failure conditions will have returned false from within the loop.  So if we've
    // seen a digit, and we've fallen out of the loop, we're golden.
    return (numDigits > 0);
}

/*************************************************************************************************/
/*!
    \brief ConfigItemTypeToString

    \return - human-readable string representation of the supplied ConfigItemType (for use in logging)
*/
/*************************************************************************************************/
const char8_t* ConfigItem::ConfigItemTypeToString(ConfigItem::ConfigItemType type)
{
    switch (type)
    {
    case ConfigItem::CONFIG_ITEM_NONE:
        return "NONE";
    case ConfigItem::CONFIG_ITEM_STRING:
        return "STRING";
    case ConfigItem::CONFIG_ITEM_MAP:
        return "MAP";
    case ConfigItem::CONFIG_ITEM_SEQUENCE:
        return "SEQUENCE";
    case ConfigItem::CONFIG_ITEM_PAIR:
        return "PAIR";
    default:
        return "unknown";
    };
}


} // Blaze
