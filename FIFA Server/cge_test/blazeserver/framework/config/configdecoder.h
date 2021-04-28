/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CONFIGDECODER_H
#define BLAZE_CONFIGDECODER_H

/*** Include files *******************************************************************************/

#include "EATDF/tdf.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConfigItem;
class ConfigMap;
class ConfigSequence;
class ConfigPair;

class ConfigDecoder
{
    NON_COPYABLE(ConfigDecoder);
public:
    ConfigDecoder(const ConfigMap& configMap, bool strictParsing = false, const char8_t* featureName = "");

    ~ConfigDecoder();

    bool decode(EA::TDF::Tdf *tdf);
    uint32_t getErrorCount() const { return mErrorCount; }

private:
    void decodeMember(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName = "");
    void decodeTdfMap(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName = "");
    void decodeTdfList(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName = "");
    void decodeTdfBitfield(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName = "");
    void decodeTdfVariable(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName = "");
    void decodeGenericTdfType(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName = "");
    void decodeTdfUnion(const ConfigItem& configValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName = "");
    void decodeTdfClass(const ConfigMap& classMap, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo, bool& isDefault, const char8_t* altMemberName = "");

private:
    const ConfigMap& mConfigMap;
    int mErrorCount;
    const char8_t *mCurrentKey;     // Used by generic type to do type lookup.
    const char8_t *mCurrentKey2;     // Used by generic type to do type lookup.
    const char8_t *mFeatureName;     // Used for logging purposes
    bool mStrictParsing;
};

} // Blaze

#endif // BLAZE_CONFIGDECODER_H