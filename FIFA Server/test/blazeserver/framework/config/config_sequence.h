/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef CONFIG_SEQUENCE_H
#define CONFIG_SEQUENCE_H

/*** Include files *******************************************************************************/
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "framework/config/config_item.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConfigMap;

class ConfigSequence
{
    NON_COPYABLE(ConfigSequence);

public:
    ConfigSequence();
    ~ConfigSequence();

    void resetIter() const;
    bool hasNext() const;
    const ConfigItem *nextItem() const;

    const char8_t* at(size_t index) const;
    size_t getSize() const { return mItemVector.size(); }

    bool nextBool(bool defaultValue) const;
    char8_t nextChar(char8_t defaultValue) const;
    int8_t nextInt8(int8_t defaultValue) const;
    int16_t nextInt16(int16_t defaultValue) const;
    int32_t nextInt32(int32_t defaultValue) const;
    int64_t nextInt64(int64_t defaultValue) const;
    uint8_t nextUInt8(uint8_t defaultValue) const;
    uint16_t nextUInt16(uint16_t defaultValue) const;
    uint32_t nextUInt32(uint32_t defaultValue) const;
    uint64_t nextUInt64(uint64_t defaultValue) const;
    double_t nextDouble(double_t defaultValue) const;
    EA::TDF::TimeValue nextTimeInterval(const EA::TDF::TimeValue& defaultValue) const;
    const char8_t* nextString(const char8_t* defaultValue) const;
    const ConfigSequence* nextSequence() const;
    const ConfigMap* nextMap() const;
    const ConfigPair* nextPair() const;

    ConfigSequence *addSequence();
    ConfigMap *addMap();
    void addValue(const char8_t *value);
    void addPair(const char8_t* first, char8_t separator, const char8_t* second);

    bool operator==(const ConfigSequence& rhs) const;
    ConfigSequence *clone() const;

    void toString(eastl::string &output, uint32_t indent = 0) const;

private:
    typedef eastl::vector<ConfigItem *> ItemVector;
    ItemVector mItemVector;

    typedef ItemVector::const_iterator ItemVectorIterator;
    mutable ItemVectorIterator mIter;

};

} // Blaze

#endif // CONFIG_SEQUENCE_H

