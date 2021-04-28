/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class ConfigSequence

The ConfigSequence class represents the value of a configuration property which is comprised
of a sequential collection of values.  This allows a client to iterate over all values in a
property which may look like: fibonacci = [1, 1, 2, 3, 5, 8, 13].

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/



/*** Private Methods *****************************************************************************/
ConfigSequence::ConfigSequence()
    : mIter(nullptr)
{
    resetIter();
}

ConfigSequence::~ConfigSequence()
{
    ItemVector::iterator itr = mItemVector.begin();
    ItemVector::iterator end = mItemVector.end();
    for (; itr != end; ++itr)
    {
        delete *itr;
    }
}

ConfigSequence *ConfigSequence::clone() const
{ 
    ConfigSequence *result = BLAZE_NEW_CONFIG ConfigSequence();

    ItemVector::const_iterator itr = mItemVector.begin();
    ItemVector::const_iterator end = mItemVector.end();
    for (; itr != end; ++itr)
    {
        result->mItemVector.push_back(BLAZE_NEW_CONFIG ConfigItem(**itr));
    }
    result->resetIter();

    return result;
}


ConfigSequence *ConfigSequence::addSequence() 
{
    ConfigItem *item = BLAZE_NEW_CONFIG ConfigItem();
    mItemVector.push_back(item);
    item->setSequence();
    return item->getSequence();
}

ConfigMap *ConfigSequence::addMap()  
{ 
    ConfigItem *item = BLAZE_NEW_CONFIG ConfigItem();
    mItemVector.push_back(item);
    item->setMap();
    return item->getMap();
}

void ConfigSequence::addValue(const char8_t *value) 
{ 
    ConfigItem *item = BLAZE_NEW_CONFIG ConfigItem();
    mItemVector.push_back(item);
    item->setString(value);
}

void ConfigSequence::addPair(const char8_t* first, char8_t separator, const char8_t* second)
{
    ConfigItem *item = BLAZE_NEW_CONFIG ConfigItem();
    mItemVector.push_back(item);
    item->setPair(first, separator, second);
}

/*************************************************************************************************/
/*!
    \brief resetIter

    Reset the iterator to the beginning of the sequence.  This allows the client to begin iterating
    over the values via subsequent calls to next_SomeType_().
*/
/*************************************************************************************************/
void ConfigSequence::resetIter() const
{
    mIter = mItemVector.begin();
}

/*************************************************************************************************/
/*!
    \brief hasNext

    Determine whether any values remain in the iterator.

    \return - True if there are values remaining, false otherwise
*/
/*************************************************************************************************/
bool ConfigSequence::hasNext() const
{
    return (mIter != mItemVector.end());
}

/*************************************************************************************************/
/*!
    \brief get

    Retrieve value for specific index.  

    \param[in]  at - index to fetch.

    \return - The string value (if any) at the specific index, or nullptr if index is incorrect or 
              item is not a string.
*/
/*************************************************************************************************/
const char8_t* ConfigSequence::at(size_t index) const
{
    if (index < mItemVector.size())
    {
        bool isDefault;
        return mItemVector.at(index)->getString(nullptr, isDefault);
    }

    return nullptr;
}

/*************************************************************************************************/
/*!
    \brief nextBool

    Retrieve a boolean value.  The string representation of a boolean property should be "true"
    or "false" (case-insensitive).

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
bool ConfigSequence::nextBool(bool defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getBool(defaultValue, isDefault) : defaultValue;
}

/*************************************************************************************************/
/*!
    \brief nextChar

    Retrieve a character value.

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
char8_t ConfigSequence::nextChar(char8_t defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getChar(defaultValue, isDefault) : defaultValue;
}

/*************************************************************************************************/
/*!
    \brief nextInt8

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
int8_t ConfigSequence::nextInt8(int8_t defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getInt8(defaultValue, isDefault) : defaultValue;
}

/*************************************************************************************************/
/*!
    \brief nextInt16

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
int16_t ConfigSequence::nextInt16(int16_t defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getInt16(defaultValue, isDefault) : defaultValue;
}

/*************************************************************************************************/
/*!
    \brief nextInt32

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
int32_t ConfigSequence::nextInt32(int32_t defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getInt32(defaultValue, isDefault) : defaultValue;
}

/*************************************************************************************************/
/*!
    \brief nextInt64

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
int64_t ConfigSequence::nextInt64(int64_t defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getInt64(defaultValue, isDefault) : defaultValue;
}

/*************************************************************************************************/
/*!
    \brief nextUInt8

    Retrieve an int value.  The string representation of an uint property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
uint8_t ConfigSequence::nextUInt8(uint8_t defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getUInt8(defaultValue, isDefault) : defaultValue;
}


/*************************************************************************************************/
/*!
    \brief nextUInt16

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
uint16_t ConfigSequence::nextUInt16(uint16_t defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getUInt16(defaultValue, isDefault) : defaultValue;
}

/*************************************************************************************************/
/*!
    \brief nextUInt32

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
uint32_t ConfigSequence::nextUInt32(uint32_t defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getUInt32(defaultValue, isDefault) : defaultValue;

}

/*************************************************************************************************/
/*!
    \brief nextUInt64

    Retrieve an int value.  The string representation of an int property should follow
    the    format defined in the standard library strtol function.

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
uint64_t ConfigSequence::nextUInt64(uint64_t defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getUInt64(defaultValue, isDefault) : defaultValue;

}

/*************************************************************************************************/
/*!
    \brief nextDouble

    Retrieve a double value.  The string representation of a double property should follow
    the    format defined in the standard library strtod function.

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
double_t ConfigSequence::nextDouble(double_t defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getDouble(defaultValue, isDefault) : defaultValue;

}

/*************************************************************************************************/
/*!
    \brief nextTimeInterval

    Retrieve a Time value.

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
TimeValue ConfigSequence::nextTimeInterval(const TimeValue& defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getTimeInterval(defaultValue, isDefault) : defaultValue;
}

/*************************************************************************************************/
/*!
    \brief nextString

    Retrieve a string value.

    \param[in]  defaultValue - value to be returned if the property is unparseable

    \return - The property value or defaultValue
*/
/*************************************************************************************************/
const char8_t* ConfigSequence::nextString(const char8_t* defaultValue) const
{
    const ConfigItem *item = nextItem();
    bool isDefault = false;
    return (item != nullptr) ? item->getString(defaultValue, isDefault) : defaultValue;

}

/*************************************************************************************************/
/*!
    \brief nextPair

    Retrieve a pair value.

    \return - The property value or null
*/
/*************************************************************************************************/
const ConfigPair* ConfigSequence::nextPair() const
{
    const ConfigItem *item = nextItem();
    return (item != nullptr) ? item->getPair() : nullptr;
}

/*************************************************************************************************/
/*!
    \brief nextSequence

    Retrieve a sequence value.

    \return - The property value or null
*/
/*************************************************************************************************/
const ConfigSequence* ConfigSequence::nextSequence() const
{
    const ConfigItem *item = nextItem();
    return (item != nullptr) ? item->getSequence() : nullptr;
}

/*************************************************************************************************/
/*!
    \brief nextMap

    Retrieve a map value.

    \return - The property value or null
*/
/*************************************************************************************************/
const ConfigMap* ConfigSequence::nextMap() const
{
    const ConfigItem *item = nextItem();
    return (item != nullptr) ? item->getMap() : nullptr;
}

bool ConfigSequence::operator==(const ConfigSequence& rhs) const
{
    if (this == &rhs)
        return true;

    if (mItemVector.size() != rhs.mItemVector.size())
        return false;
  
    ItemVectorIterator itr = mItemVector.begin();
    ItemVectorIterator end = mItemVector.end();
    ItemVectorIterator rhsItr = rhs.mItemVector.begin();

    for (; itr != end; ++itr, ++rhsItr)
    {
        if (!(*itr == *rhsItr))
        {
            return false;
        }
    }

    return true;
}

const ConfigItem* ConfigSequence::nextItem() const
{
    const ConfigItem *result = nullptr;
    if (mIter != nullptr && mIter != mItemVector.end())
    {
        result = *mIter;
        mIter++;
    }

    return result;
}

/*************************************************************************************************/
/*! \brief Dumps the contents of the sequence to a string
 *************************************************************************************************/
void ConfigSequence::toString(eastl::string &output, uint32_t indentSize) const
{
    const uint32_t INDENT = 4;

    eastl::string indentString;
    indentString.append(indentSize, ' ');
    
    for (ItemVectorIterator itr = mItemVector.begin(), end = mItemVector.end(); itr != end; ++itr)
    {
        ConfigItem* item = *itr;
        switch (item->getItemType())
        {
            case ConfigItem::CONFIG_ITEM_MAP:
            {
                output.append(indentString);
                output.append("{\n");

                item->getMap()->toString(output, indentSize + INDENT);

                output.append(indentString);
                output.append("}\n");
                break;
            }
            case ConfigItem::CONFIG_ITEM_SEQUENCE:
            {
                output.append(indentString);
                output.append("[\n");

                item->getSequence()->toString(output, indentSize + INDENT);

                output.append(indentString);
                output.append("]\n");
                break;
            }
            case ConfigItem::CONFIG_ITEM_STRING:
            {
                output.append(indentString);
                bool isDefault = false;
                eastl::string val;
                val.sprintf("\"%s\"%s\n", item->getString("", isDefault), (itr + 1 != end) ? "," : "");
                output.append(val);
                break;
            }
            case ConfigItem::CONFIG_ITEM_PAIR:
            {
                output.append(indentString);
                bool isDefault = false;
                const ConfigPair* pair = item->getPair();
                eastl::string val;
                val.sprintf("\"%s %c %s\"%s\n", pair->getFirst().getString("", isDefault), pair->getSeparator(), pair->getSecond().getString("", isDefault), (itr + 1 != end) ? "," : "");
                output.append(val);
                break;
            }
            default:
                break;
        }
    }
}



} // Blaze
