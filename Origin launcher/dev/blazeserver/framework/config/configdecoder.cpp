/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "framework/blaze.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/config/configdecoder.h"
#include "framework/util/shared/bytearrayinputstream.h"
#include "framework/util/shared/bytearrayoutputstream.h"
#include "framework/util/shared/base64.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"
#include "EASTL/sort.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#define CONFIG_DECODER_ERR_LOG(message) BLAZE_ERR_LOG(Log::CONFIG, message << " (for feature '" << mFeatureName << "')")
#define CONFIG_DECODER_WARN_LOG(message) BLAZE_WARN_LOG(Log::CONFIG, message << " (for feature '" << mFeatureName << "')")
#define CONFIG_DECODER_TRACE_LOG(message) BLAZE_TRACE_LOG(Log::CONFIG, message << " (for feature '" << mFeatureName << "')")
/*** Public Methods ******************************************************************************/

ConfigDecoder::ConfigDecoder(const ConfigMap& configMap, bool strictParsing, const char8_t* featureName) 
    :mConfigMap(configMap),
    mErrorCount(0),
    mCurrentKey(nullptr),
    mCurrentKey2(nullptr),
    mFeatureName(featureName),
    mStrictParsing(strictParsing)
{
}

ConfigDecoder::~ConfigDecoder()
{
}

bool ConfigDecoder::decode(EA::TDF::Tdf *tdf)
{        
    if (tdf == nullptr)
        return false;

    mErrorCount = 0;
    EA::TDF::TdfGenericReference tdfRef(*tdf);
    bool isDefault = false;
    decodeTdfClass(mConfigMap, tdfRef, nullptr, isDefault);

    if (mErrorCount != 0)
    {
        return false; 
    }

    return true;
}

void ConfigDecoder::decodeMember(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName)
{
    isDefault = true;

    // We don't use memberInfo->getDefaultValue() here because we may need to provide defaults for elements that don't have valid memberInfo.
    uint64_t intDefault = memberInfo ? EA::TDF::TdfMemberInfo::getDefaultValueUInt64(memberInfo->memberDefault) : 0;
    float floatDefault = memberInfo ? memberInfo->memberDefault.floatDefault : 0;
    const char8_t* stringDefault = memberInfo ? memberInfo->memberDefault.stringDefault : "";
    const char8_t* memberName = memberInfo ? memberInfo->getMemberName() : altMemberName;

    switch (outValue.getType())
    {
        // Simple types:
        case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:
        {
            outValue.asFloat() = (float)configValue.getDouble(floatDefault, isDefault);
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (float) " << outValue.asFloat() );
            return; 
        }

        case EA::TDF::TDF_ACTUAL_TYPE_ENUM:
        {
            // If the type is an enum, then we read the value as a string and attempt a lookup. 
            // If the lookup fails, it's an invalid value, and an error. 
            const char *enumVal = configValue.getString(nullptr, isDefault);
            if (isDefault)
            {
                outValue.asEnum() = (int32_t)intDefault;
            }
            else
            {
                const EA::TDF::TdfEnumMap *enumMap = outValue.getTypeDescription().asEnumMap();
                if ((enumMap == nullptr) || !enumMap->findByName(enumVal, outValue.asEnum()))
                {
                    ++mErrorCount;
                    CONFIG_DECODER_ERR_LOG("[ConfigDecoder] While reading enum " << memberName << " from config variable: Enum value '" << enumVal << "' is unknown.");
                }
                else
                {
                    CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (enum) " << enumVal);
                }
            }
            return; 
        }

        case EA::TDF::TDF_ACTUAL_TYPE_STRING:
        {
            const char8_t* configString = configValue.getString(stringDefault, isDefault);
            if (configString == nullptr)
            {
                ++mErrorCount;
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] While reading TdfString variable " << memberName << " from config. Could not read string from config variable of type " << configValue.getItemType() << ".");
                return;
            }

            uint32_t maxLength = memberInfo ? memberInfo->additionalValue : 0;
            if (maxLength > 0 && strlen(configString) > maxLength)
            {
                // Since we have no way to get the maxLength for map/list strings, we just use a warning here. 
                CONFIG_DECODER_WARN_LOG("[ConfigDecoder] While reading TdfString variable " << memberName << " from config. The provided string " << 
                                            configString << ". Exceeds the string size limit (" << maxLength << ") for this type.");
            }
            outValue.asString() = configString;

            if (!outValue.asString().isValidUtf8())
            {
                CONFIG_DECODER_WARN_LOG("[ConfigDecoder] While reading TdfString variable " << memberName << " from config. The provided string " <<
                    configString << ". Is not valid Unicode.");

                ++mErrorCount;
                return;
            }

            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (string) " << outValue.asString().c_str() );
            return; 
        }

        case EA::TDF::TDF_ACTUAL_TYPE_TIMEVALUE:
        {
            TimeValue defaultValue(static_cast<int64_t>(intDefault));

            bool isInterval = true;
            outValue.asTimeValue() = configValue.getTimeInterval(defaultValue, isDefault);
            if (isDefault)
            {
                outValue.asTimeValue() = configValue.getGMDateTime(defaultValue, isDefault);
                isInterval = false;
            }

            if (isDefault)
            {
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Could not parse time value " << memberName << " from " << configValue.getString("", isDefault));
                ++mErrorCount;
                return;
            }

            char8_t stringBuf[64];
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (TimeValue) "
                << (isInterval ? outValue.asTimeValue().toIntervalString(stringBuf, sizeof(stringBuf)) : outValue.asTimeValue().toDateFormat(stringBuf, sizeof(stringBuf))));
            return;
        }
        case EA::TDF::TDF_ACTUAL_TYPE_BOOL:
        {
            outValue.asBool() = configValue.getBool((intDefault != 0), isDefault);
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (bool) " << outValue.asBool());
            return; 
        }

        case EA::TDF::TDF_ACTUAL_TYPE_INT8:
        {
            outValue.asInt8() = configValue.getInt8((int8_t)intDefault, isDefault);
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (int8) " << outValue.asInt8() );
            return; 
        }
        case EA::TDF::TDF_ACTUAL_TYPE_UINT8:
        {
            outValue.asUInt8() = configValue.getUInt8((uint8_t)intDefault, isDefault);
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (uint8) " << outValue.asUInt8() );
            return; 
        }
        case EA::TDF::TDF_ACTUAL_TYPE_INT16:
        {
            outValue.asInt16() = configValue.getInt16((int16_t)intDefault, isDefault);
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (int16) " << outValue.asInt16() );
            return; 
        }
        case EA::TDF::TDF_ACTUAL_TYPE_UINT16:
        {
            outValue.asUInt16() = configValue.getUInt16((uint16_t)intDefault, isDefault);
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (uint16) " << outValue.asUInt16() );
            return; 
        }
        case EA::TDF::TDF_ACTUAL_TYPE_INT32:
        {
            outValue.asInt32() = configValue.getInt32((int32_t)intDefault, isDefault);
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (int32) " << outValue.asInt32() );
            return; 
        }
        case EA::TDF::TDF_ACTUAL_TYPE_UINT32:
        {
            outValue.asUInt32() = configValue.getUInt32((uint32_t)intDefault, isDefault);
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (uint32) " << outValue.asUInt32() );
            return; 
        }
        case EA::TDF::TDF_ACTUAL_TYPE_INT64:
        {
            outValue.asInt64() = configValue.getInt64((int64_t)intDefault, isDefault);
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (int64) " << outValue.asInt64() );
            return; 
        }
        case EA::TDF::TDF_ACTUAL_TYPE_UINT64:
        {
            outValue.asUInt64() = configValue.getUInt64((uint64_t)intDefault, isDefault);
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName << " from config variable to: (uint64) " << outValue.asUInt64() );
            return; 
        }

        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_TYPE:  
        {
            const char *val = configValue.getString("", isDefault);
            outValue.asObjectType() = EA::TDF::ObjectType::parseString(val);
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName
                << " from config variable to: (EA::TDF::ObjectType) " << outValue.asObjectType().toString().c_str() );
            return;
        }
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_ID:
        {
            const char *val = configValue.getString("", isDefault);
            outValue.asObjectId() = EA::TDF::ObjectId::parseString(val);
            CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName
                << " from config variable to: (EA::TDF::ObjectId) " << outValue.asObjectId().toString().c_str() );
            return;
        }

        case EA::TDF::TDF_ACTUAL_TYPE_BLOB:
        {
            const char *val = configValue.getString("", isDefault);
            int32_t retValue = outValue.asBlob().decodeBase64(val, (EA::TDF::TdfSizeVal)strlen(val));

            if (retValue < 0)
            {
                ++mErrorCount;
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to parse EA::TDF::TdfBlob value for member " << memberName << ". Blob values must be Base64-encoded strings.");
            }
            else
            {
                CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Setting value of TDF variable " << memberName
                    << " from config variable to: (EA::TDF::TdfBlob) with " << retValue << " Base64-encoded characters." );
            }
            return;
        }


        case EA::TDF::TDF_ACTUAL_TYPE_VARIABLE:  
        {
            decodeTdfVariable(configValue, outValue, memberInfo, isDefault, altMemberName);
            return;
        }
        case EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE:
        {
            decodeGenericTdfType(configValue, outValue, memberInfo, isDefault, altMemberName);
            return;
        }

        case EA::TDF::TDF_ACTUAL_TYPE_LIST:          
        {
            decodeTdfList(configValue, outValue, memberInfo, isDefault, altMemberName);
            return;
        }

        case EA::TDF::TDF_ACTUAL_TYPE_MAP:
        {
            decodeTdfMap(configValue, outValue, memberInfo, isDefault, altMemberName);
            return;
        }

        case EA::TDF::TDF_ACTUAL_TYPE_BITFIELD:
        {
            decodeTdfBitfield(configValue, outValue, memberInfo, isDefault, altMemberName);
            return;
        }
        case EA::TDF::TDF_ACTUAL_TYPE_UNION:
        {
            decodeTdfUnion(configValue, outValue, memberInfo, isDefault, altMemberName);
            return;
        }
        case EA::TDF::TDF_ACTUAL_TYPE_TDF:
        {
            const ConfigMap* configMap = configValue.getMap();
            if (configMap == nullptr)
            {
                // SUPER DUPER SPECIAL CASE: 
                // To make The TdfMappingCode even easier to work with, we provide a special config mapping that will attempt to load the value directly into the default value, as if it was a generic:
                if (outValue.getTdfId() == (EA::TDF::TdfId)0x39dc32b) // Blaze::GameManager::TemplateAttributeDefinition::TDF_ID)  // PACKER_TODO - Move the template defs into Framework, change the old location to use typedefs?
                {
                    const EA::TDF::TdfMemberInfo* memInfo = nullptr;
                    if (outValue.asTdf().getTypeDescription().getMemberInfoByName("default", memInfo))
                    {
                        EA::TDF::TdfGenericReference memberValue;
                        outValue.asTdf().getValueByName("default", memberValue);

                        decodeMember(configValue, memberValue, memInfo, isDefault);
                        if (isDefault == false)
                        {
                            uint32_t index;
                            outValue.asTdf().getMemberIndexByInfo(memInfo, index);
                            outValue.asTdf().markMemberSet(index);
                        }
                        isDefault = false;
                        return;
                    }
                }

                ++mErrorCount;
                bool strIsDefault = false;
                const char8_t* valAsString = configValue.getString("", strIsDefault);
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Expected ConfigMap for class " << memberName << " of type " << outValue.getFullName() << "; instead found ConfigItem of type " <<
                    ConfigItem::ConfigItemTypeToString(configValue.getItemType()) << (strIsDefault ? "" : " with value ") << valAsString);
            }
            else
            {
                decodeTdfClass(*configMap, outValue, memberInfo, isDefault, altMemberName);
            }
            return;
        }
        default:
        {
            ++mErrorCount;
            bool strIsDefault = false;
            const char8_t* valAsString = configValue.getString("", strIsDefault);
            CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unknown type found when parsing config item " << memberName << " of type " <<
                ConfigItem::ConfigItemTypeToString(configValue.getItemType()) << (strIsDefault ? "" : " with value ") << valAsString);
            return;
        }    

    }
}

void ConfigDecoder::decodeTdfVariable(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName)
{
    const char8_t* memberName = memberInfo ? memberInfo->getMemberName() : altMemberName;
    const ConfigMap* varTdfConfig = configValue.getMap();
    if (varTdfConfig != nullptr)
    {
        const char *tdfName = varTdfConfig->getString("tdf", nullptr);
        const ConfigItem* tdfValue = varTdfConfig->getItem("value");
        const ConfigMap* tdfValueMap = tdfValue ? tdfValue->getMap() : nullptr;        // For variable type, we require a Map to load the provided Tdf
        if (tdfName != nullptr && tdfValueMap != nullptr)
        {
            //  visit our variable tdf
            EA::TDF::TdfPtr tdf = EA::TDF::TdfFactory::get().create(tdfName, *Allocator::getAllocator(), "ConfigDecoderVarTdf");
            if (tdf != nullptr)
            {
                outValue.asVariable().set(tdf);
                decodeMember(*tdfValue, EA::TDF::TdfGenericReference(*outValue.asVariable().get()), nullptr, isDefault, "(decoded from variable tdf)");
            }
            else
            {
                ++mErrorCount;
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to retrieve TDF ID for tdf='" << tdfName <<"' in variable tdf " << memberName << ".");
            }
        }
        else
        {
            ++mErrorCount;
            if (tdfName == nullptr)
            {
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] No TDF NAME specified for variable tdf " << memberName << ".");
            }
            if (tdfValueMap == nullptr)
            {
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] No TDF VALUE specified for variable tdf " << memberName << ".");
            }
        }
        isDefault = false;      // Config Map exists. We're not default.
    }
    else
    {
        ++mErrorCount;
        bool strIsDefault = false;
        const char8_t* valAsString = configValue.getString("", strIsDefault);
        CONFIG_DECODER_ERR_LOG("[ConfigDecoder] No ConfigMap exists for variable tdf member " << memberName << "; instead found ConfigItem of type " <<
            ConfigItem::ConfigItemTypeToString(configValue.getItemType()) << (strIsDefault ? "" : " with value ") << valAsString);
    }
    return;
}

void ConfigDecoder::decodeGenericTdfType(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName)
{
    const char8_t* memberName = memberInfo ? memberInfo->getMemberName() : altMemberName;
    // Note: Generic supports several variations: 
    // (mCurrentKey) map<key == string> -> generic = realValue                  - Value's type comes from the map's key
    // (mCurrentKey & mCurrentKey2) map<key == string> -> generic = realValue   - Value's type comes from the map's key
    // generic = { tdfid="Qualified::Tdf::Name", value= realValue }             - Sequence with Single Value
    // generic = { "Qualified::Tdf::Name" = realValue }                         - Alternative sequence
    //
    // New 2 additional default config loading variants:
    // (input) list<string> -> generic = list<string>                           - Use a list of strings, if the input is a list of strings
    // (input) string -> generic                                                - Use a string if the input is a string
    // (These assume that the value will be converted to the correct type later, and the tdfid config will be used if a string/int conversion is not possible.)

    const char *tdfName = nullptr;
    const ConfigItem* tdfValue = &configValue;
    const EA::TDF::TypeDescription* typeDesc = nullptr;

    const ConfigMap* varTdfConfig = configValue.getMap();
    bool valueIsMap  = (varTdfConfig != nullptr);
    bool valueIsList = (configValue.getSequence() != nullptr);
    if (mCurrentKey != nullptr)
    {
        // Case 1: Lookup typename using parent map key as the typename:
        tdfName = mCurrentKey;
        typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(tdfName);

        if (typeDesc == nullptr && mCurrentKey2 != nullptr)
        {
            // Case 2.5: Lookup via class/value mapping-  Ex. IntFilter = { operation = {} }
            tdfName = mCurrentKey2;
            typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(tdfName, mCurrentKey);
        }

        // Case 2: Lookup via class/value using the parent as the value, and the grandparent as the class:
        if (typeDesc == nullptr)
        {
            // Case 2.5: Lookup via class/value mapping- Preconfig Logic - Ex.   (mCurrentKey2 == Grandparent/class)
            if (EA::TDF::TdfFactory::get().lookupTypeName(mCurrentKey2, mCurrentKey, tdfName))
            {
                typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(tdfName);
                if (varTdfConfig == nullptr && typeDesc == nullptr)
                {
                    CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to get type description for generic " << memberName << " with type name " << tdfName << " from map keys " << mCurrentKey2 << ", " << mCurrentKey << ".");
                }
            }
            else if (varTdfConfig == nullptr)
            {
                CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Unable to get type name for generic " << memberName << " with map keys " << mCurrentKey2 << ", " << mCurrentKey << ".  Attempting to decode as string or list<string>, or via type encoding.");
            }
        }
    }

    if (typeDesc == nullptr)
    {
        if (valueIsList)
        {
            // Try to just load the value as a list of strings:
            // If there's an error, it'll show up inside here (harder to debug) but that's the situation:
            tdfName = "list<string>";
            typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(tdfName);
        }
        else if (!valueIsMap)
        {
            // If it's not a list or map, it's a single value, so try to load as a string:
            tdfName = "string";
            typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(tdfName);
        }
        else
        {
            varTdfConfig->resetIter();

            // If there's only one element, try to decode like:
            // myGeneric = { "int" = 10 }
            const ConfigMap::MappedItem* mappedItem = varTdfConfig->nextMappedItem();
            if (mappedItem != nullptr && !varTdfConfig->hasNext())
            {
                tdfName = mappedItem->first.c_str();
                tdfValue = &mappedItem->second;
                typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(tdfName);
                if (tdfName != nullptr && typeDesc == nullptr)
                {
                    CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to get type description for generic " << memberName << " with type name " << tdfName << " specified by direct mapping.");
                }
            }
            else
            {
                // Default, long winded approach: 
                tdfName = varTdfConfig->getString("tdfid", nullptr);   // Uses a different syntax than variable, to distinguish the two.
                tdfValue = varTdfConfig->getItem("value");
                typeDesc = EA::TDF::TdfFactory::get().getTypeDesc(tdfName);
                if (tdfName != nullptr && typeDesc == nullptr)
                {
                    CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to get type description for generic " << memberName << " with type name " << tdfName << " specified by tdfid.");
                }
            }
        }
    }

    if (typeDesc != nullptr && tdfValue != nullptr)
    {
        //  visit our member
        outValue.asGenericType().setType(*typeDesc);
        decodeMember(*tdfValue, EA::TDF::TdfGenericReference(outValue.asGenericType().get()), nullptr, isDefault, tdfName);
        isDefault = false;      // Value exists. We're not default.
    }
    else
    {
        if (tdfValue == nullptr)
        {
            CONFIG_DECODER_ERR_LOG("[ConfigDecoder] No TDF VALUE specified for generic " << memberName << " with " << (tdfName == nullptr ? "unknown " : "") << " type name " << (tdfName == nullptr ? "" : tdfName) << " with map keys " << mCurrentKey2 << ", " << mCurrentKey << ".");
        }
        ++mErrorCount;
    }


    return;
}
void ConfigDecoder::decodeTdfList(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName)
{
    const char8_t* memberName = memberInfo ? memberInfo->getMemberName() : altMemberName;
    // Note: List supports 2 variations: 
    // list = value                        - Single Value
    // list = [ value1, value2, value3 ]   - Sequence with Single Value
    const ConfigSequence* listConfig = configValue.getSequence();
    if (listConfig != nullptr)
    {
        listConfig->resetIter();
        while (listConfig->hasNext())
        {
            EA::TDF::TdfGenericReference newListValue;
            outValue.asList().pullBackRef(newListValue);
            decodeMember(*listConfig->nextItem(), newListValue, nullptr, isDefault, "(in list)");
            if (isDefault)
            {
                ++mErrorCount;
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to decode value at index " << outValue.asList().vectorSize()-1 << " in list " << memberName << " of type " << outValue.getFullName() << ".");
                return;
            }
        }
        isDefault = false;      // Config Sequence exists. We're not default.
    }
    else
    {
        // If no sequence exists, load the element alone:
        EA::TDF::TdfGenericReference newListValue;
        outValue.asList().pullBackRef(newListValue);
        decodeMember(configValue, newListValue, nullptr, isDefault, "(in list)");
        if (isDefault)
        {
            ++mErrorCount;
            CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to decode (only) value in list " << memberName << " of type " << outValue.getFullName() << ".");
            return;
        }
    }

    return;
}
void ConfigDecoder::decodeTdfMap(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName)
{
    const char8_t* memberName = memberInfo ? memberInfo->getMemberName() : altMemberName;
    // Note: Map supports 3 variations: 
    // map = { key = value, key = value }   - Map
    // map = [ v1, v2, v3 ]                 - Sequence with Single Value (primitives only; used for both key & value)
    // map = [ k1:v1, k2:v2, k3:v3 ]        - Sequence with Pairs (primitives only)
    const ConfigMap* mapConfig = configValue.getMap();
    const ConfigSequence* mapListConfig = configValue.getSequence();
    if (mapConfig != nullptr)
    {
        mapConfig->resetIter();
        while (mapConfig->hasNext())
        {
            const ConfigMap::MappedItem* mappedItem = mapConfig->nextMappedItem();

            // Create a temp ConfigItem, so we can convert the string value to a generic value: (Using normal parsing)
            ConfigItem item;
            item.setString(mappedItem->first.c_str());
            EA::TDF::TdfGenericValue key;
            key.setType(outValue.asMap().getKeyTypeDesc());
            EA::TDF::TdfGenericReference keyRef(key);
            decodeMember(item, keyRef, nullptr, isDefault, "(in map)");
            if (isDefault)
            {
                // Default key means it didn't decode properly
                ++mErrorCount;
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to decode (normal) key '" << mappedItem->first.c_str() << "' in map " << memberName << ".");
                return;
            }

            EA::TDF::TdfGenericReference newValueRef;
            if (!outValue.asMap().insertKeyGetValue(keyRef, newValueRef))
            {
                ++mErrorCount;
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Error occurred while trying to insert key '" << mappedItem->first.c_str() <<"' in map " << memberName << ".");
                return;
            }

            const char* oldKey = mCurrentKey2;
            mCurrentKey2 = mCurrentKey;
            mCurrentKey = mappedItem->first.c_str();
            decodeMember(mappedItem->second, newValueRef, nullptr, isDefault, "(in map)");
            mCurrentKey = mCurrentKey2;
            mCurrentKey2 = oldKey;
            if (isDefault)
            {
                ++mErrorCount;
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to decode value for key '" << mappedItem->first.c_str() << "' in map " << memberName << ".");
                return;
            }
        }
        isDefault = false;      // Config Map exists. We're not default.
    }
    else if (mapListConfig != nullptr)
    {
        mapListConfig->resetIter();
        while (mapListConfig->hasNext())
        {
            const ConfigItem* item = mapListConfig->nextItem();
            const ConfigPair* configPair = item->getPair();
            if (configPair != nullptr)
            {
                // map = [ k1:v1, k2:v2, k3:v3 ]        - Sequence with Pairs
                // Get and decode the key: 
                const ConfigItem& keyItem = configPair->getFirst();
                bool strIsDefault = false;
                const char8_t* keyAsString = keyItem.getString("", strIsDefault);

                EA::TDF::TdfGenericValue keyValue;
                keyValue.setType(outValue.asMap().getKeyTypeDesc());
                EA::TDF::TdfGenericReference keyRef(keyValue);
                decodeMember(keyItem, EA::TDF::TdfGenericReference(keyValue), nullptr, isDefault, "(in map)");
                if (isDefault)
                {
                    // Default key means it didn't decode properly
                    ++mErrorCount;
                    CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to decode key '" << keyAsString << "' in map " << memberName << ".");
                    return;
                }

                // Insert/Decode the value: 
                EA::TDF::TdfGenericReference newValueRef;
                if (!outValue.asMap().insertKeyGetValue(keyRef, newValueRef))
                {
                    ++mErrorCount;
                    CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Error occurred while trying to insert key '" << keyAsString << "' in map " << memberName << ".");
                    return;
                }

                const char* oldKey = mCurrentKey2;
                mCurrentKey2 = mCurrentKey;
                mCurrentKey = keyItem.getString(nullptr, isDefault);
                decodeMember(configPair->getSecond(), newValueRef, nullptr, isDefault, "(in map)");
                mCurrentKey = mCurrentKey2;
                mCurrentKey2 = oldKey;
                if (isDefault)
                {
                    ++mErrorCount;
                    CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to decode value for key '" << keyAsString << "' in map " << memberName << ".");
                    return;
                }
            }
            else
            {
                // map = [ v1, v2, v3 ]                    - Sequence with Single Value (used for both key & value)
                EA::TDF::TdfGenericValue keyValue;
                keyValue.setType(outValue.asMap().getKeyTypeDesc());
                EA::TDF::TdfGenericReference keyRef(keyValue);

                const char* oldKey = mCurrentKey2;
                mCurrentKey2 = mCurrentKey;
                const char8_t* keyAsString = item->getString("", isDefault);
                mCurrentKey = (isDefault ? nullptr : keyAsString);
                decodeMember(*item, EA::TDF::TdfGenericReference(keyValue), nullptr, isDefault, "(in map)");
                mCurrentKey = mCurrentKey2;
                mCurrentKey2 = oldKey;
                if (isDefault)
                {
                    // Default key means it didn't decode properly
                    ++mErrorCount;
                    CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to decode key/value '" << keyAsString << "' in map " << memberName << ".");
                    return;
                }

                EA::TDF::TdfGenericReference newValueRef;
                if (!outValue.asMap().insertKeyGetValue(keyRef, newValueRef))
                {
                    ++mErrorCount;
                    CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Error occurred while trying to insert key '" << keyAsString << "' in map " << memberName << ".");
                    return;
                }

                if (!keyValue.convertToIntegral(newValueRef))  // Try to convert to an int first
                {
                    newValueRef.setValue(keyValue);            // Otherwise just set the value directly
                }
            }
            isDefault = false;      // Config Sequence exists. We're not default.
        }            
    }
    else
    {
        ++mErrorCount;
        bool strIsDefault = false;
        const char8_t* valAsString = configValue.getString("", strIsDefault);
        CONFIG_DECODER_ERR_LOG("[ConfigDecoder] No ConfigMap or ConfigSequence exists for map member " << memberName << "; instead found ConfigItem of type " <<
            ConfigItem::ConfigItemTypeToString(configValue.getItemType()) << (strIsDefault ? "" : " with value ") << valAsString);
    }
    return;
}
void ConfigDecoder::decodeTdfBitfield(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName)
{
    const char8_t* memberName = memberInfo ? memberInfo->getMemberName() : altMemberName;
    const ConfigMap* bitfieldMap = configValue.getMap();
    const ConfigSequence* bitfieldList = configValue.getSequence();
    const EA::TDF::TypeDescriptionBitfield* bfDesc = &outValue.asBitfield().getTypeDescription();
    if (bitfieldMap != nullptr)
    {
        // For bitfield members, we don't use recursion, we just get the members directly:
        bitfieldMap->resetIter();
        while (bitfieldMap->hasNext())
        {
            const ConfigMap::MappedItem* mappedItem = bitfieldMap->nextMappedItem();
            const char8_t* key = mappedItem->first.c_str();

            const EA::TDF::TypeDescriptionBitfieldMember* memDesc = nullptr;
            if (!bfDesc->findByName(key, memDesc))
            {
                ++mErrorCount;
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unknown bitfield member key " << key << " found when parsing bitfield " << memberName << " of type " << bfDesc->getFullName() << ".");
                return;
            }

            const ConfigItem* item = &mappedItem->second;
            uint32_t memberValue = item->getUInt32(0, isDefault);
            if (isDefault && memDesc->mIsBool)
            {
                memberValue = item->getBool(0, isDefault);
            }

            if (isDefault)
            {
                ++mErrorCount;
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unable to parse value of bitfield member key " << key << " found when parsing bitfield " << memberName << " of type " << bfDesc->getFullName() << ". Only uint32 (and bool when using a 1-bit member field) are supported.");
                return;
            }

            outValue.asBitfield().setValueByName(key, memberValue);
            CONFIG_DECODER_TRACE_LOG("[ConfigMapDecoder] Setting value of TDF bitfield (map) " << memberName << " member key " << key << " from config to: " << memberValue);
        }

        isDefault = false;      // Config Map exists. We're not default.
    }
    else if (bitfieldList != nullptr)
    {
        // For bitfield members, we don't use recursion, we just get the members directly:
        bitfieldList->resetIter();
        while (bitfieldList->hasNext())
        {
            const ConfigItem* item = bitfieldList->nextItem();

            const char8_t* key = item->getString(nullptr, isDefault);
            const EA::TDF::TypeDescriptionBitfieldMember* memDesc = nullptr;
            if (key == nullptr || !bfDesc->findByName(key, memDesc))
            {
                ++mErrorCount;
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unknown bitfield member key " << (key ? key : "(not a string or map)") << " found when parsing bitfield " << memberName << " of type " << bfDesc->getFullName() << ".");
                return;
            }

            outValue.asBitfield().setValueByName(key, 1);
            CONFIG_DECODER_TRACE_LOG("[ConfigMapDecoder] Setting value of TDF bitfield (list) " << memberName << " member key " << key << " from config to: " << 1);
        }

        isDefault = false;      // Config Map exists. We're not default.
    }
    else
    {
        uint32_t bits = configValue.getUInt32(0, isDefault);
        if (!isDefault)
        {
            outValue.asBitfield().setBits(bits);
            CONFIG_DECODER_TRACE_LOG("[ConfigMapDecoder] Setting value of TDF bitfield (int32) " << memberName << " of type " << bfDesc->getFullName() << " from config to: " << bits);
        }
        else
        {
            // Attempt to load as a single string:
            const char8_t* key = configValue.getString(nullptr, isDefault);
            const EA::TDF::TypeDescriptionBitfieldMember* memDesc = nullptr;
            if (key == nullptr || !bfDesc->findByName(key, memDesc))
            {
                ++mErrorCount;
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unknown bitfield member key " << (key ? key : "(not a string or map)") << " found when parsing bitfield " << memberName << " of type " << bfDesc->getFullName() << ".");
                return;
            }
            outValue.asBitfield().setValueByName(key, 1);
            CONFIG_DECODER_TRACE_LOG("[ConfigMapDecoder] Setting value of TDF bitfield (string) " << memberName << " member key " << key << " from config to: " << 1);
        }
        isDefault = false;      // Some value exists. We're not default.
    }
    return;
}
void ConfigDecoder::decodeTdfUnion(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName)
{
    const char8_t* memberName = memberInfo ? memberInfo->getMemberName() : altMemberName;
    // For unions, we expect to have a map, and only get one value from the map (if there's more than one, that's invalid)
    // Otherwise, it's pretty much the same as with the TDF decode:
    const ConfigMap* unionMap = configValue.getMap();
    if (unionMap != nullptr)
    {
        const EA::TDF::TypeDescriptionClass* unionDesc = &outValue.asUnion().getTypeDescription();
        if (unionMap->getSize() == 1)
        {
            unionMap->resetIter();
            while (unionMap->hasNext())
            {
                const ConfigMap::MappedItem* mappedItem = unionMap->nextMappedItem();
                const char8_t* key = mappedItem->first.c_str();

                const EA::TDF::TdfMemberInfo* memInfo = nullptr;
                if (!unionDesc->getMemberInfoByName(key, memInfo))
                {
                    if (mStrictParsing)
                    {
                        ++mErrorCount;
                        CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unknown union member key '" << key << "' found when parsing union " << memberName << " of type " << unionDesc->getFullName() << ".");
                    }
                    else
                    {
                        CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Skipping unknown union member key '" << key << "' found when parsing union " << memberName << " of type " << unionDesc->getFullName() << ", since Strict Parsing is disabled.");
                    }
                    continue;  // If parsing fails, just move onto the next value.
                }                    

                // Once the member info is known, switch to that member:
                outValue.asUnion().switchActiveMemberByName(key);

                EA::TDF::TdfGenericReference memberValue;
                outValue.asUnion().getValueByName(key, memberValue);

                const ConfigItem* item = &mappedItem->second;
                decodeMember(*item, memberValue, memInfo, isDefault);
                if (isDefault == false)
                {
                    outValue.asUnion().markMemberSet(outValue.asUnion().getActiveMemberIndex());
                }
                outValue.asUnion().markSet();
            }
        }
        else
        {
            ++mErrorCount;
            CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Too many parameters in ConfigMap union " << memberName << " of type " << unionDesc->getFullName() << " - only one member can be used at a time.");
        }

        isDefault = false;      // Config Map exists. We're not default.
    }
    else
    {
        ++mErrorCount;
        bool strIsDefault = false;
        const char8_t* valAsString = configValue.getString("", strIsDefault);
        CONFIG_DECODER_ERR_LOG("[ConfigDecoder] No ConfigMap exists for union member " << memberName << "; instead found ConfigItem of type " <<
            ConfigItem::ConfigItemTypeToString(configValue.getItemType()) << (strIsDefault ? "" : " with value ") << valAsString);
    }
    return;
}
void ConfigDecoder::decodeTdfClass(const ConfigMap& classMap, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName)
{
    const EA::TDF::TypeDescriptionClass* classDesc = &outValue.asTdf().getTypeDescription();
    const char8_t* memberName = memberInfo ? memberInfo->getMemberName() : altMemberName;

    classMap.resetIter();
    while (classMap.hasNext())
    {
        const ConfigMap::MappedItem* mappedItem = classMap.nextMappedItem();
        const char8_t* key = mappedItem->first.c_str();

        const EA::TDF::TdfMemberInfo* memInfo = nullptr;
        if (!classDesc->getMemberInfoByName(key, memInfo))
        {
            if (mStrictParsing)
            {
                ++mErrorCount;
                CONFIG_DECODER_ERR_LOG("[ConfigDecoder] Unknown class member key '" << key << "' found when parsing class " << memberName << " of type " << classDesc->name << ".");
            }
            else
            {
                CONFIG_DECODER_TRACE_LOG("[ConfigDecoder] Skipping unknown class member key '" << key << "' found when parsing class " << memberName << " of type " << classDesc->name << ", since Strict Parsing is disabled.");
            }

            continue;  // If parsing fails, just move onto the next value.
        }                    

        EA::TDF::TdfGenericReference memberValue;
        outValue.asTdf().getValueByName(key, memberValue);

        const ConfigItem* item = &mappedItem->second;
        decodeMember(*item, memberValue, memInfo, isDefault);
        if (isDefault == false)
        {
            uint32_t index;
            outValue.asTdf().getMemberIndexByInfo(memInfo, index);
            outValue.asTdf().markMemberSet(index);
        }
    }

    isDefault = false;      // Config Map exists (even if empty). We're not default.
}


#undef CONFIG_DECODER_ERR_LOG
#undef CONFIG_DECODER_WARN_LOG
#undef CONFIG_DECODER_TRACE_LOG
} // Blaze
