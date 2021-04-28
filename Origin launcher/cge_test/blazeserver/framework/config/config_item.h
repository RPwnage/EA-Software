/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef CONFIG_ITEM_H
#define CONFIG_ITEM_H

/*** Include files *******************************************************************************/


/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

class ConfigSequence;
class ConfigMap;
class ConfigPair;

class ConfigItem 
{
public:
    ConfigItem() : mType(CONFIG_ITEM_NONE) {}
    ConfigItem(const ConfigItem& rhs) : mType(CONFIG_ITEM_NONE) { *this = rhs; }
    ~ConfigItem();  

    bool isNumber() const;

    bool getBool(bool defaultValue, bool& isDefault) const;
    char8_t getChar(char8_t defaultValue, bool& isDefault) const;
    int8_t getInt8(int8_t defaultValue, bool& isDefault) const;
    int16_t getInt16(int16_t defaultValue, bool& isDefault) const;
    int32_t getInt32(int32_t defaultValue, bool& isDefault) const;
    int64_t getInt64(int64_t defaultValue, bool& isDefault) const;
    uint8_t getUInt8(uint8_t defaultValue, bool& isDefault) const;
    uint16_t getUInt16(uint16_t defaultValue, bool& isDefault) const;
    uint32_t getUInt32(uint32_t defaultValue, bool& isDefault) const;
    uint64_t getUInt64(uint64_t defaultValue, bool& isDefault) const;
    double_t getDouble(double_t defaultValue, bool& isDefault) const;
    const char8_t* getString(const char8_t* defaultValue, bool& isDefault) const;
    EA::TDF::TimeValue getTime(const EA::TDF::TimeValue& defaultValue, bool& isDefault) const;
    EA::TDF::TimeValue getDateTime(const EA::TDF::TimeValue& defaultValue, bool& isDefault) const;
    EA::TDF::TimeValue getGMDateTime(const EA::TDF::TimeValue& defaultValue, bool& isDefault) const;
    EA::TDF::TimeValue getTimeInterval(const EA::TDF::TimeValue& defaultValue, bool& isDefault) const;

    template <class T>
    static T getIntegerFromString(const char8_t* str, T defaultValue, bool& isDefault, bool isSigned, uint8_t bits);

    const ConfigSequence* getSequence() const;
    ConfigSequence* getSequence();

    const ConfigMap* getMap() const;
    ConfigMap* getMap();

    const ConfigPair* getPair() const;


    bool operator==(const ConfigItem& rhs) const;
    ConfigItem &operator=(const ConfigItem& rhs);

    enum ConfigItemType
    {
        CONFIG_ITEM_NONE,
        CONFIG_ITEM_STRING,
        CONFIG_ITEM_MAP,
        CONFIG_ITEM_SEQUENCE,
        CONFIG_ITEM_PAIR
    };

    ConfigItemType getItemType() const { return mType; }

    void setString(const char8_t *value);
    void setMap();
    void setSequence();
    void setPair(const char8_t *first, char8_t separator, const char8_t *second);

    static const char8_t* ConfigItemTypeToString(ConfigItemType type);

private:
    void clear();

    ConfigItemType mType;
    
    union
    {
        eastl::string *str;
        ConfigSequence *seq;
        ConfigMap *map;
        ConfigPair *pair;
    } mValue;

    static bool isNumber(const char8_t* value);
};

class ConfigPair
{
public:
    ConfigPair(const ConfigItem& first, char8_t separator, const ConfigItem& second);
    ConfigPair(const char8_t* first, char8_t separator, const char8_t* second);
    ~ConfigPair();

    const ConfigItem& getFirst() const { return mFirst; }
    const ConfigItem& getSecond() const { return mSecond; }
    char8_t getSeparator() const { return mSeparator; }
    const char8_t* asString() const { return mFullString.c_str(); }

    bool operator==(const ConfigPair& rhs) const;

private: 
    ConfigItem mFirst;
    ConfigItem mSecond;
    char8_t mSeparator;
    eastl::string mFullString;
};

} // Blaze

#endif // CONFIG_ITEM_H

