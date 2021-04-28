/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef CONFIG_MAP_H
#define CONFIG_MAP_H

/*** Include files *******************************************************************************/
#include "EASTL/string.h"
#include "EASTL/vector_map.h"
#include "EATDF/time.h"
#include "framework/config/config_item.h"
#include "framework/util/shared/blazestring.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConfigSequence;

class ConfigMap
{
public:
    typedef eastl::map<eastl::string, ConfigItem, CaseInsensitiveStringLessThan> ItemMap;
    typedef ItemMap::value_type MappedItem;

    ConfigMap();
    virtual ~ConfigMap();

    ConfigSequence *addSequence(const char8_t *propName);
    ConfigMap *addMap(const char8_t *propName);
    const char8_t *addValue(const char8_t *propName, const char8_t* value);

    void resetIter() const;
    bool hasNext() const;

    // NOTE: All three of these functions advance together. (Ex. Calling nextKey, and then nextItem, would get the 1st key, and the 2nd item)
    const char8_t* nextKey() const;
    const ConfigItem *nextItem() const;
    const MappedItem *nextMappedItem() const;

    bool isDefined(const char8_t* propName) const;
    size_t getSize() const { return mItemMap.size(); }

    bool isNumber(const char8_t* propName) const;

    bool getBool(const char8_t* propName, bool defaultValue, bool& isDefault) const;
    char8_t getChar(const char8_t* propName, char8_t defaultValue, bool& isDefault) const;
    int8_t getInt8(const char8_t* propName, int8_t defaultValue, bool& isDefault) const;
    int16_t getInt16(const char8_t* propName, int16_t defaultValue, bool& isDefault) const;
    int32_t getInt32(const char8_t* propName, int32_t defaultValue, bool& isDefault) const;
    int64_t getInt64(const char8_t* propName, int64_t defaultValue, bool& isDefault) const;
    uint8_t getUInt8(const char8_t* propName, uint8_t defaultValue, bool& isDefault) const;
    uint16_t getUInt16(const char8_t* propName, uint16_t defaultValue, bool& isDefault) const;
    uint32_t getUInt32(const char8_t* propName, uint32_t defaultValue, bool& isDefault) const;
    uint64_t getUInt64(const char8_t* propName, uint64_t defaultValue, bool& isDefault) const;
    double_t getDouble(const char8_t* propName, double_t defautValue, bool& isDefault) const;
    const char8_t* getString(const char8_t* propName, const char8_t* defautValue, bool& isDefault) const;
    EA::TDF::TimeValue getTime(const char8_t* propName, const EA::TDF::TimeValue& defautValue, bool& isDefault) const;
    EA::TDF::TimeValue getDateTime(const char8_t* propName, const EA::TDF::TimeValue& defautValue, bool& isDefault) const;
    EA::TDF::TimeValue getGMDateTime(const char8_t* propName, const EA::TDF::TimeValue& defautValue, bool& isDefault) const;
    EA::TDF::TimeValue getTimeInterval(const char8_t* propName, const EA::TDF::TimeValue& defautValue, bool& isDefault) const;

    bool getBool(const char8_t* propName, bool defaultValue) const;
    int8_t getInt8(const char8_t* propName, int8_t defaultValue) const;
    int16_t getInt16(const char8_t* propName, int16_t defaultValue) const;
    int32_t getInt32(const char8_t* propName, int32_t defaultValue) const;
    int64_t getInt64(const char8_t* propName, int64_t defaultValue) const;
    uint8_t getUInt8(const char8_t* propName, uint8_t defaultValue) const;
    uint16_t getUInt16(const char8_t* propName, uint16_t defaultValue) const;
    uint32_t getUInt32(const char8_t* propName, uint32_t defaultValue) const;
    uint64_t getUInt64(const char8_t* propName, uint64_t defaultValue) const;
    double_t getDouble(const char8_t* propName, double_t defautValue) const;
    const char8_t* getString(const char8_t* propName, const char8_t* defautValue) const;
    EA::TDF::TimeValue getTime(const char8_t* propName, const EA::TDF::TimeValue& defautValue) const;
    EA::TDF::TimeValue getDateTime(const char8_t* propName, const EA::TDF::TimeValue& defautValue) const;
    EA::TDF::TimeValue getGMDateTime(const char8_t* propName, const EA::TDF::TimeValue& defautValue) const;
    EA::TDF::TimeValue getTimeInterval(const char8_t* propName, const EA::TDF::TimeValue& defautValue) const;

    const ConfigSequence* getSequence(const char8_t* propName) const;
    const ConfigMap* getMap(const char8_t* propName) const;
    bool operator==(const ConfigMap& rhs) const;

    ConfigMap *clone() const;
    const ConfigItem *getItem(const char8_t* propName) const;
    void toString(eastl::string &output, uint32_t indent = 0) const;
protected:
    ConfigMap(const ConfigMap& configMap);
    bool upsertItem(const char8_t* propName, const ConfigItem& item);

    ItemMap mItemMap;   
    
    typedef ItemMap::const_iterator ItemMapIterator;
    mutable ItemMapIterator mIter;

private:
    ConfigMap& operator=(const ConfigMap& otherObj);

};

} // Blaze

#endif // CONFIG_MAP_H
