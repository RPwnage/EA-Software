/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Xml2Decoder

    This class provides a mechanism to decode XML into a TDF object.

    For example:

    <?xml version="1.0" encoding="UTF8"?>
    <classname>
        <id>100</id>
        <msg>Invalid username</msg>
        <arrayname>
            <structname>
                <structmember1>7</structmember1>
                <structmember2>Ken</structmember2>
            </structname>
            <structname>
                <structmember1>8</structmember1>
                <structmember2>Dave</structmember2>
            </structname>
        </arrayname>
        <mapname>
            <entry key="1">Milt</entry>
            <entry key="4">Franco</entry>
        </mapname>
        <mapname2>
            <entry key="17">
                <structname>
                    <structmember1>SomeData</structmember1>
                    <structmember2>MoreData</structmember2>
                </structname>
            </entry>
        </mapname2>
        <unionname1 member="1">
            <valu>
                <unionmember>3</unionmember>
            </valu>
        </unionname1>
        <unionname2>
            <structname>
                <structmember1>Fast</structmember1>
                <structmember2>
                    <struct2member1>70</struct2member1>
                </structmember2>
            </structname>
        </unionname2>
    </classname>

    In the above, the elements like "classname", "arrayname", etc, are not literal.
    They are replaced with the name of the TDF class, array, map, union, etc.

*/
/*************************************************************************************************/

#include "framework/blaze.h"

/*** Include Files *******************************************************************************/
#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/blazesdk.h"
#endif
#ifdef ENABLE_LEGACY_CODECS
#include "framework/util/shared/rawbuffer.h"
#include "framework/protocol/shared/httpdecoder.h"
#include "framework/protocol/shared/heat2util.h"
#include "framework/protocol/shared/xml2decoder.h"

#include "framework/util/shared/bytearrayinputstream.h"
#include "framework/util/shared/bytearrayoutputstream.h"
#include "framework/util/shared/base64.h"

#include "EATDF/tdfmemberinfo.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#include "BlazeSDK/shared/framework/locales.h"
#else
#include "framework/util/shared/blazestring.h"
#include "framework/util/locales.h"
#endif

//SDK does not use the standard "EA_ASSERT" so we redefine it here as "BlazeAssert"
#if !defined(EA_ASSERT) && defined(BlazeAssert)
#define EA_ASSERT(X) BlazeAssert(X)
#endif

#include "EASTL/sort.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
/*************************************************************************************************/
/*!
    \brief Xml2Decoder

    Construct a new xml2 decoder that decodes from the given buffer

    \param[in]  buffer - the buffer to decode from

*/
/*************************************************************************************************/
Xml2Decoder::Xml2Decoder() :
    mKey(nullptr),
    mKeyBufSize(0),
    mKeyLen(0),
#ifdef BLAZE_CLIENT_SDK
    mParamMap(MEM_GROUP_FRAMEWORK, "Xml2Decoder::mParamMap"),
    mArraySizeMap(MEM_GROUP_FRAMEWORK, "Xml2Decoder::mArraySizeMap"),
    mArrayIndexMap(MEM_GROUP_FRAMEWORK, "Xml2Decoder::mArrayIndexMap"),
    mVarArrayMap(MEM_GROUP_FRAMEWORK, "Xml2Decoder::mVarArrayMap"),
#endif
    mStateDepth(0)
{
    memset(&mStateStack[0], 0, sizeof(StateStruct));
}

/*************************************************************************************************/
/*!
    \brief ~Xml2Decoder

    Destructor for Xml2Decoder class.
*/
/*************************************************************************************************/
Xml2Decoder::~Xml2Decoder()
{
#ifdef BLAZE_CLIENT_SDK
    BLAZE_DELETE_ARRAY(MEM_GROUP_FRAMEWORK, mKey);
#else
    delete[] mKey;
#endif
}

/*************************************************************************************************/
/*!
    \brief setBuffer

    Set the raw buffer that contains the HTTP request to be decoded

    \param[in]  buffer - the buffer to decode from

*/
/*************************************************************************************************/
void Xml2Decoder::setBuffer(RawBuffer* buffer)
{
    mBuffer = buffer;

    // Important to set state depth to 0, as nested structs will result in multiple calls to
    // begin() during decoding, and only the call when state depth is 0 will perform initialization
    mStateDepth = 0;
}

bool Xml2Decoder::visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue)
{
    // Initialize our state
    mErrorCount = 0;
    mValidationError = Blaze::ERR_OK;

    // make sure the key is alloc'd
    growString(mKey, mKeyBufSize, mKeyLen, 1);

    mKey[0] = '\0';
    mKeyLen = 0;
    mParamMap.clear();
    mArraySizeMap.clear();
    mArrayIndexMap.clear();

    XmlReader xmlReader(*mBuffer);
    if (!xmlReader.parse(this))
    {
        ++mErrorCount;
        return false;
    }

    mStateStack[0].state = STATE_NORMAL;

    // start with the root node
    pushKey(tdf.getClassName(), strlen(tdf.getClassName()));

    tdf.visit(*this, tdf, tdf);

    return (mErrorCount == 0);
}

bool Xml2Decoder::visit(EA::TDF::TdfUnion &tdf, const EA::TDF::TdfUnion &referenceValue)
{
    ++mErrorCount;
    return false;
}

bool Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue)
{
    State state = mStateStack[mStateDepth].state;

    if (state == STATE_ARRAY)
    {
        //  append tag values to create the parameter key used to lookup this variable tdf inside the array.
        BLAZE_EASTL_STRING classname = mVarArrayMap[mKey][mStateStack[mStateDepth].dimensionIndex];

        pushKey(classname.c_str(), classname.size());
        if (mStateStack[mStateDepth].dimensionIndex > 0)
        {
            pushArrayIndex(mStateStack[mStateDepth].dimensionIndex);
        }

        ++mStateStack[mStateDepth].dimensionIndex;
    }

    mStateStack[mStateDepth].readVariableTdf = StateStruct::VARTDF_TDFID;

    //First get the uint8_t that says whether we're present or not
    bool success = true;
    // get the uint32_t that indicates EA::TDF::TdfId
    EA::TDF::TdfId tdfId = EA::TDF::INVALID_TDF_ID;
    visit(rootTdf, parentTdf, tag, tdfId, tdfId, 0);
   
    mStateStack[mStateDepth].readVariableTdf = StateStruct::VARTDF_TDF;

    if (tdfId != EA::TDF::INVALID_TDF_ID)
    {
        value.create(tdfId);
        if (!value.isValid())
        {
            ++mErrorCount;
            return false;
        }
        //Now visit the TDF
        success = visit(rootTdf, parentTdf, tag, *value.get(), *value.get());
    }

    //  revert the push at the start of this method (it won't be done in the visit(EA::TDF::Tdf))
    if (state == STATE_ARRAY)
    {
        //  done parsing variable tdf specific information, clearing this flag is necessary for proper key cleanup.
        popKey();
    }

    return success;
}

bool Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue)
{
    State state = mStateStack[mStateDepth].state;

    if (state == STATE_ARRAY)
    {
        //  append tag values to create the parameter key used to lookup this variable tdf inside the array.
        BLAZE_EASTL_STRING classname = mVarArrayMap[mKey][mStateStack[mStateDepth].dimensionIndex];

        pushKey(classname.c_str(), classname.size());
        if (mStateStack[mStateDepth].dimensionIndex > 0)
        {
            pushArrayIndex(mStateStack[mStateDepth].dimensionIndex);
        }

        ++mStateStack[mStateDepth].dimensionIndex;
    }

    mStateStack[mStateDepth].readVariableTdf = StateStruct::VARTDF_TDFID;

    //First get the uint8_t that says whether we're present or not
    bool success = true;
    // get the uint32_t that indicates EA::TDF::TdfId
    EA::TDF::TdfId tdfId = EA::TDF::INVALID_TDF_ID;
    visit(rootTdf, parentTdf, tag, tdfId, tdfId, 0);
   
    mStateStack[mStateDepth].readVariableTdf = StateStruct::VARTDF_TDF;

    if (tdfId != EA::TDF::INVALID_TDF_ID)
    {
        value.create(tdfId);
        if (!value.isValid())
        {
            ++mErrorCount;
            return false;
        }
        //Now visit the TDF
        visitReference(rootTdf, parentTdf, tag, value.getValue(), &referenceValue.getValue());
    }

    //  revert the push at the start of this method (it won't be done in the visit(EA::TDF::Tdf))
    if (state == STATE_ARRAY)
    {
        //  done parsing variable tdf specific information, clearing this flag is necessary for proper key cleanup.
        popKey();
    }

    return success;

}

bool Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue)
{
    State parentState = mStateStack[mStateDepth].state;

    //  variable array keys are updated inside the visit(EA::TDF::VariableTdfBase)
    if (parentState == STATE_ARRAY)
    {
        if (mStateStack[mStateDepth].readVariableTdf == StateStruct::VARTDF_NULL)
        {
            pushKey(value.getClassName(), strlen(value.getClassName()));

            if (mStateStack[mStateDepth].dimensionIndex > 0)
            {
                pushArrayIndex(mStateStack[mStateDepth].dimensionIndex);
            }
            ++mStateStack[mStateDepth].dimensionIndex;
        }
    }
    else
    {
        pushKey(parentTdf, tag);
    }

    pushStack(STATE_NORMAL);

    value.visit(*this, rootTdf, value);

    popStack();

    //  variable array keys are popped inside the visit(EA::TDF::VariableTdfBase), so do not pop variable array index keys.
    if (parentState != STATE_ARRAY || mStateStack[mStateDepth].readVariableTdf == StateStruct::VARTDF_NULL)
    {
        popKey();
    }

    return (mErrorCount == 0);
}


void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue)
{
    State parentState = mStateStack[mStateDepth].state;

    if (parentState == STATE_NORMAL || parentState == STATE_ARRAY)
    {
        pushKey(parentTdf, tag);
    }

    pushStack(STATE_ARRAY);
    mStateStack[mStateDepth].dimensionIndex = 0;
    mStateStack[mStateDepth].dimensionSize = 0;

    uint32_t dimSize = 0;
    if (value.getTypeDescription().valueType.type == EA::TDF::TDF_ACTUAL_TYPE_VARIABLE || 
        value.getTypeDescription().valueType.type == EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE)
    {
        if (mVarArrayMap.find(mKey) != mVarArrayMap.end())
        {
            // if it's been parsed, then we'll have a count for it
            dimSize = static_cast<uint32_t>(mVarArrayMap[mKey].size());
        }
    }
    else
    {
        if (mArraySizeMap.find(mKey) != mArraySizeMap.end())
        {
            // if it's been parsed, then we'll have a count for it
            dimSize = mArraySizeMap[mKey];
        }
    }
    mStateStack[mStateDepth].dimensionSize = dimSize;

    if (mStateStack[mStateDepth].dimensionSize > 0)
    {
        value.initVector(mStateStack[mStateDepth].dimensionSize);
    }

    value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);

    popStack();
    popKey();
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue)
{
    State parentState = mStateStack[mStateDepth].state;

    if (parentState == STATE_NORMAL)
    {
        pushKey(parentTdf, tag);
    }

    pushStack(STATE_MAP);
    mStateStack[mStateDepth].readValue = false;
    mStateStack[mStateDepth].mapKeyList.clear();

    // find all items with tag and generate map key list.  simplest way to do this
    // is iterate through all parameters and pick out the ones needed.
    growString(mKey, mKeyBufSize, mKeyLen, 1); // +1 for delimiter

    size_t initkeylen = mKeyLen;
    size_t keylen = initkeylen + blaze_snzprintf(mKey + initkeylen, mKeyBufSize - initkeylen, "|");    // search for "<tag>|"

    ParamMap::iterator itr = mParamMap.begin();
    ParamMap::iterator end = mParamMap.end();
    for (; itr != end; ++itr)
    {
        if (blaze_strnicmp(itr->first.c_str(), mKey, keylen) == 0)
        {
            blaze_strnzcpy(mIndex, itr->first.c_str() + keylen, sizeof(mIndex));
            char8_t* tokpos = strchr(mIndex, '/');
            if (tokpos)
            {
                *tokpos = '\0';
            }

            //avoid adding duplicate keys
            XmlMapKeyList::const_iterator keyItr = mStateStack[mStateDepth].mapKeyList.begin();
            XmlMapKeyList::const_iterator keyEnd = mStateStack[mStateDepth].mapKeyList.end();
            for (; keyItr != keyEnd; ++keyItr)
                if (blaze_strcmp(keyItr->c_str(), mIndex) == 0)
                    break;
            if (keyItr == keyEnd)
                mStateStack[mStateDepth].mapKeyList.push_back(mIndex);
        }
    }

    //map keys need to be sorted
    if (value.getTypeDescription().keyType.isIntegral())
    {
        eastl::sort(mStateStack[mStateDepth].mapKeyList.begin(), mStateStack[mStateDepth].mapKeyList.end(), &compareMapKeysInteger);
    }
    else
    {
        if (value.ignoreCaseKeyStringCompare())
        {
            eastl::sort(mStateStack[mStateDepth].mapKeyList.begin(), mStateStack[mStateDepth].mapKeyList.end(), &compareMapKeysStringIgnoreCase);
        }
        else
        {
            eastl::sort(mStateStack[mStateDepth].mapKeyList.begin(), mStateStack[mStateDepth].mapKeyList.end(), &compareMapKeysString);
        }
    }

    //  reset state variables for map field decoding
    mStateStack[mStateDepth].dimensionSize = static_cast<uint32_t>(mStateStack[mStateDepth].mapKeyList.size());
    mStateStack[mStateDepth].dimensionIndex = 0;

    // Strip off any delimiters we temporarily put on the key
    mKey[initkeylen] = '\0';

    value.initMap(mStateStack[mStateDepth].dimensionSize);
    value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);

    popStack();
    popKey();
}

bool Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue)
{
    // Get the index from the value stored along side the variable name.
    uint32_t index = 0;
    visit(rootTdf, parentTdf, tag, index, index, EA::TDF::TdfUnion::INVALID_MEMBER_INDEX);

    if ((mStateStack[mStateDepth].state == STATE_ARRAY)
        || (mStateStack[mStateDepth].state == STATE_MAP))
    {
        // use its class name as the element name if this union is in a list or a map
        pushKey(value.getClassName(), strlen(value.getClassName()));
        if (mStateStack[mStateDepth].state == STATE_ARRAY)
        {
            // dimension index is incremented after the visit method above so we have to use array index decremented by 1
            if (mStateStack[mStateDepth].dimensionIndex-1 > 0)
            {
                pushArrayIndex(mStateStack[mStateDepth].dimensionIndex-1);
            }
        }
    }
    else
    {
        if (!pushKey(parentTdf, tag))
        {
            return false;
        }
    }

    pushStack(STATE_UNION);

    // Switch the union, and then visit it.
    if (index != EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
    {
        // set the active member if the member index is provided
        value.switchActiveMember(index);
    }
    else
    {
        // look up the field name to determine the active member

        // Since the active member is not yet set, iterate the union member info in order to fetch the field name
        EA::TDF::TdfMemberInfoIterator memberIt(value);
        while (memberIt.next())
        {
            const EA::TDF::TdfMemberInfo* info = memberIt.getInfo();
            const char *memberName = info->getMemberName();
            char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";
            if (!convertMemberToElement(memberName, elementName, sizeof(elementName)))
            {
                ++mErrorCount;
                return false;
            }

            pushKey(elementName, strlen(elementName));

            // look up the array index map to determine if this field name is used at the next inner level 
            ArrayMap::iterator itr = mArrayIndexMap.find(mKey);

            popKey();

            if (itr != mArrayIndexMap.end())
            {
                value.switchActiveMember(memberIt.getIndex());
                break;
            }
        }
    }

    value.visit(*this, rootTdf, value);

    popStack();
    popKey();

    return (mErrorCount == 0);
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue)
{
    int8_t valueReal = 0;
    visit(rootTdf, parentTdf, tag, valueReal, valueReal, defaultValue ? 1 : 0);
    value = (valueReal == 1);
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            value = keyvalue[0];
        else
            value = defaultValue;
        popKey();
    }
    else
    {
        value = defaultValue;
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            blaze_str2int(keyvalue, &value);
        else
            value = defaultValue;
        popKey();
    }
    else
    {
        value = defaultValue;
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            blaze_str2int(keyvalue, &value);
        else
            value = defaultValue;
        popKey();
    }
    else
    {
        value = defaultValue;
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            blaze_str2int(keyvalue, &value);
        else
            value = defaultValue;
        popKey();
    }
    else
    {
        value = defaultValue;
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            blaze_str2int(keyvalue, &value);
        else
            value = defaultValue;
        popKey();
    }
    else
    {
        value = defaultValue;
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            blaze_str2int(keyvalue, &value);
        else
            value = defaultValue;
        popKey();
    }
    else
    {
        value = defaultValue;
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue)
{
    visit(rootTdf, parentTdf, tag, value.getBits(), value.getBits(), 0);
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            blaze_str2int(keyvalue, &value);
        else
            value = defaultValue;
        popKey();
    }
    else
    {
        value = defaultValue;
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            blaze_str2int(keyvalue, &value);
        else
            value = defaultValue;
        popKey();
    }
    else
    {
        value = defaultValue;
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            blaze_str2int(keyvalue, &value);
        else
            value = defaultValue;
        popKey();
    }
    else
    {
        value = defaultValue;
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            blaze_str2flt(keyvalue, value);
        else
            value = defaultValue;
        popKey();
    }
    else
    {
        value = defaultValue;
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && blaze_strcmp(value, defaultValue) != 0)
        return;
#endif

    State parentState = mStateStack[mStateDepth].state;

    if (parentState == STATE_UNION)
    {
        pushKey("valu", 4);
    }
    else 
    {
        pushKey(parentTdf, tag);
    }

    const char8_t *keyvalue = getKeyValue();
    if (keyvalue != nullptr)
    {
        uint32_t keyvaluelen = static_cast<uint32_t>(getKeyValueLen());
        if (maxLength > 0 && maxLength < keyvaluelen)
        {
            // input exceeds the max length defined for this EA::TDF::TdfString
            ++mErrorCount;
            mValidationError = ERR_TDF_STRING_TOO_LONG;
            return;
        }
        value.set(keyvalue, keyvaluelen);
    }
    else
    {
        value.set(defaultValue);
    }
    popKey();
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue)
{
    State parentState = mStateStack[mStateDepth].state;
    
    if (parentState == STATE_UNION)
    {
        pushKey("valu", 4);

        // Get the decode count from /<union name>/valu/count
        pushKey("count", 5);
        uint32_t decodeCount = 0;
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            blaze_str2int(keyvalue, &decodeCount);
        popKey();

        // Get the encoded data
        keyvalue = getKeyValue();
        if (keyvalue != nullptr && decodeCount > 0)
        {
            value.resize(decodeCount);
            ByteArrayInputStream input((const uint8_t *) keyvalue, decodeCount);
            ByteArrayOutputStream output(value.getData(), value.getSize());
            uint32_t size = Base64::decode(&input, &output);  //decode the blob
            value.setCount(size);
        }

        popKey();
    }
    else 
    {
        if (pushKey(parentTdf, tag))
        {
            const char8_t *keyvalue = getKeyValue();
            if (keyvalue != nullptr)
            {
                value.setData((const uint8_t *) keyvalue, (uint32_t) getKeyValueLen());
            }
            popKey();
        }
    }
}

// enum decoder doing type-checking
void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    // decode the value
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        size_t keyvaluelen = getKeyValueLen();
        if (keyvalue != nullptr && keyvaluelen > 0)
        {
            BLAZE_EASTL_STRING trimmedKeyValue(keyvalue, (BLAZE_EASTL_STRING::size_type)keyvaluelen);
            if ((enumMap == nullptr) || !enumMap->findByName(trimmedKeyValue.c_str(), value))
            {
                // if value is not enum constant identifier
                // try and see if it's number (backwards compatibility)
                if (*blaze_str2int(trimmedKeyValue.c_str(), &value) != '\0')
                {
                    // not a number? then it's error
                    ++mErrorCount;

                    char8_t buf[5];
                    Heat2Util::decodeTag(tag, buf, 5);

                    mValidationError = ERR_INVALID_TDF_ENUM_VALUE;
                }
            }
        }
        else
            value = defaultValue;
        popKey();
    }
    else
    {
        value = defaultValue;
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            value = EA::TDF::ObjectType::parseString(keyvalue);
        popKey();
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
            value = EA::TDF::ObjectId::parseString(keyvalue);
        popKey();
    }
}

void Xml2Decoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue &referenceValue, const TimeValue defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (pushKey(parentTdf, tag))
    {
        const char8_t *keyvalue = getKeyValue();
        if (keyvalue != nullptr)
        {
            int64_t microseconds = 0;
            blaze_str2int(keyvalue, &microseconds);
            value = TimeValue(microseconds);
        }
        else
        {
            value = defaultValue;
        }
        popKey();
    }
}

/*************************************************************************************************/
/*!
    brief Start Document event.

    This function gets called when the first XML element is encountered. Any whitespace,
    comments and the <?xml tag at the start of the document are skipped before this
    function is called.
*/
/*************************************************************************************************/
void Xml2Decoder::startDocument()
{
    EA_ASSERT(mStateDepth == 0);
    mStateStack[0].state = STATE_NORMAL;
}

/*************************************************************************************************/
/*!
    \brief End Document event.

    This function gets called when a close element matching the first open element
    is encountered. This marks the end of the XML document. Any data following this
    close element is ignored by the parser.
*/
/*************************************************************************************************/
void Xml2Decoder::endDocument()
{
    EA_ASSERT(mKeyLen == 0);
    EA_ASSERT(mStateDepth == 0);
}

/*************************************************************************************************/
/*!
    \brief Start Element event.

    This function gets called when an open element is encountered. (\<element\>)

    \param name name of the element. (Not null-terminated)
    \param nameLen length of the name of the element.
    \param attributes list of attributes for this element (can be nullptr if attributeCount = 0)
    \param attributeCount number of attributes.
    \note The name pointers of the element and any attributes passed to this function
    will point directly to the actual XML data. This is why the strings are not
    null-terminated and the length is provided instead.
*/
/*************************************************************************************************/
void Xml2Decoder::startElement(const char *name, size_t nameLen, const XmlAttribute *attributes, size_t attributeCount)
{
    State parentState = mStateStack[mStateDepth].state;
    State thisState = STATE_NORMAL;

    // remember the parent in case we're updating an array -- the parent will hold the array count
    BLAZE_EASTL_STRING parentKey = mKey;

    if ( (mStateDepth == 0) && (nameLen == 5) && blaze_strnicmp(name, "error", 5) == 0)
    {
        pushStack(STATE_ERROR);
        return;
    }

    if (parentState == STATE_ERROR)
    {
        if (nameLen == 9 && blaze_strnicmp(name, "component", 9) == 0)
        {
            // do nothing
            pushStack(STATE_ERROR);
            return;
        }
        else if (nameLen == 9 && blaze_strnicmp(name, "errorCode", 9) == 0)
        {
            // do nothing
            pushStack(STATE_ERROR);
            return;
        }
        else if (nameLen == 9 && blaze_strnicmp(name, "errorName", 9) == 0)
        {
            // do nothing
            pushStack(STATE_ERROR);
            return;
        }
        else
        {
            // this is our response object
            thisState = STATE_NORMAL;
        }
    }

    if (parentState == STATE_UNION)
    {
        if (nameLen == 4 && blaze_strnicmp(name, "valu", 4) == 0)
        {
            // In a union, the "valu" block represents the union type, which we don't technically know yet.
            // keep our union state so we can substitute on the visit end.
            thisState = STATE_UNION;

            // For blob, the decode count is in the valu's count attribute, store it under key /<union name>/valu/count
            if (attributes[0].nameLen == 5 && blaze_strnicmp(attributes[0].name, "count", 5) == 0)
            {
                pushKey("valu", 4);
                pushKey("count", 5);
                mParamMap[mKey].set(attributes[0].value, attributes[0].valueLen);
                popKey();
                popKey();
            }
        }
    }

    if (parentState != STATE_MAP)
    {
        // check for map
        if (attributeCount == 1)
        {
            if (attributes[0].nameLen == 3 && blaze_strnicmp(attributes[0].name, "key", 3) == 0)
            {
                if (nameLen == 5 && blaze_strnicmp(name, "entry", 5) == 0)
                {
                    pushMapIndex(attributes[0].value, attributes[0].valueLen);
                    // create an entry in param map and initialize the value as empty if the entry key is defined
                    mParamMap[mKey].set(nullptr, 0);
                    thisState = STATE_MAP;
                }
            }
        }

        //  adjust the active key since this is not a map key nor a map value (parentState != STATE_MAP), could be a TDF, a variable TDF, or an array.
        if (thisState != STATE_MAP)
        {
            pushKey(name, nameLen);
        }
    }

    // Unions
    if (attributeCount == 1)
    {
        if (attributes[0].nameLen == 6 && blaze_strnicmp(attributes[0].name, "member", 6) == 0)
        {
            // Union
            mParamMap[mKey].set(attributes[0].value, attributes[0].valueLen);
            thisState = STATE_UNION;
        }
    }

    // check for variable tdf or a variable tdf array.
    bool isVariableTdf = false;
    if (attributeCount == 2)
    {
        // the 2nd attribute is tdfclass name, but we only care about the id
        if (attributes[0].nameLen == 5 && blaze_strnicmp(attributes[0].name, "tdfid", 5) == 0)
        {
            if (thisState != STATE_MAP)
            {
                //  Increment variable tdf collection item count.  Total variable tdf children count will be used
                //  when visiting the equivalent variable list in the EA::TDF::Tdf.  At this point we don't need to validate
                //  that this is a variable array - only to collection enough information for the later EA::TDF::Tdf::visit pass.
                //  in this case the key is the parentKey followed by an array index if count > 0
                ClassnameArray& classnames = mVarArrayMap[parentKey];
                classnames.push_back(BLAZE_EASTL_STRING(name, (BLAZE_EASTL_STRING::size_type)nameLen));
                const uint32_t count = static_cast<uint32_t>(classnames.size());
                if (count > 1)
                {
                    pushArrayIndex(count - 1);

                    thisState = STATE_ARRAY;
                }
            }
            mParamMap[mKey].set(attributes[0].value, attributes[0].valueLen);
            isVariableTdf = true;
        }
    }

    //  Handle Non-Variable TDF Arrays
    if (parentState != STATE_MAP && parentState != STATE_UNION && !isVariableTdf)
    {
        // count all combinations in case any are arrays
        uint32_t arrayIndex = ++mArrayIndexMap[mKey];
        if (arrayIndex > 1)
        {
            // another array element, so add the index (zero-based) to the key
            // note that the first array element won't have this index in its key
            pushArrayIndex(arrayIndex - 1);

            // update the parent counter
            if (mArrayIndexMap[parentKey] > 1)
            {
                char8_t parentIndex[16];
                blaze_snzprintf(parentIndex, sizeof(parentIndex), "|%u", mArrayIndexMap[parentKey]);
                parentKey.append(parentIndex);
            }
            thisState = STATE_ARRAY;
        }
        mArraySizeMap[parentKey] = arrayIndex;
    }
    // else this node is a struct for a map element (don't include in key)

    pushStack(thisState);
}

/*************************************************************************************************/
/*!
    \brief End Element event.

    This function gets called when a close element is encountered. (\</element\>)

    \param name name of the element. (Not null-terminated)
    \param nameLen length of the name of the element.
    \note The name pointer passed to this function will point directly to the
    name of the element in the actual XML data. This is why the string is not
    null-terminated and the length is provided instead.
*/
/*************************************************************************************************/
void Xml2Decoder::endElement(const char *name, size_t nameLen)
{
    int32_t parentDepth = mStateDepth - 1;
    if (parentDepth < 0)
        parentDepth = 0;

    if (mStateStack[mStateDepth].state == STATE_ARRAY)
    {
        ++mArrayIndexMap[mKey];
    }

    if (mStateStack[parentDepth].state != STATE_MAP)
    {
        // check for map
        bool isMapEntry = false;
        if (nameLen == 5 && blaze_strnicmp(name, "entry", 5) == 0)
        {
            // check that the tail end of the current key isn't "/entry"
            if (mKeyLen > 6 && blaze_strnicmp(mKey + mKeyLen - 6, "/entry", 6) != 0)
            {
                popIndex();
                isMapEntry = true;
            }
        }
        if (!isMapEntry)
        {
            popKey();
        }
    }
    // else this node is a struct for a map element (not included in key)

    popStack();
}

/*************************************************************************************************/
/*!
    \brief Characters event.

    This function gets called when XML characters are encountered.

    \param characters pointer to character data.
    \param charLen length of character data.
    \note The character pointer passed to this function will point directly to the
    character data in the actual XML data. This is why the data is not
    null-terminated and the length is provided instead.
*/
/*************************************************************************************************/
void Xml2Decoder::characters(const char *characters, size_t charLen)
{
    if (mStateStack[mStateDepth].state == STATE_ERROR)
    {
        // do nothing
        return;
    }
    
    // Copy xml's data to param map, un-escaping standard entities &quot, &apos, &amp, &lt, &gt etc.
    char8_t* buf = mParamMap[mKey].allocateOwnedData(charLen);
    XmlBuffer::copyChars(buf, characters, charLen + 1, charLen);
}

/*************************************************************************************************/
/*!
    \brief CDATA event.

    This function gets called when a CDATA section is encountered.

    \param data pointer to the data.
    \param dataLen length of the data.
*/
/*************************************************************************************************/
void Xml2Decoder::cdata(const char *data, size_t dataLen)
{
    if (mStateStack[mStateDepth].state == STATE_ERROR)
    {
        // do nothing
        return;
    }
    mParamMap[mKey].set(data, dataLen);
}

/*** Protected Methods ***************************************************************************/

/*************************************************************************************************/
/*!
    \brief getKeyValue

    Returns the value string based on the current decode key.  Returned value depends on
    current state.  For example, the Map state retrieves values based on whether it's in a "mapkey"
    state versus a "mapvalue" state.

    \return - nullptr char8_t string if no value available for current key.
*/
/*************************************************************************************************/
const char8_t* Xml2Decoder::getKeyValue()
{
    const char8_t* strvalue = nullptr;

    if (mStateStack[mStateDepth].state == STATE_MAP)
    {
        uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;

        if (!mStateStack[mStateDepth].readValue)
        {
            strvalue = mStateStack[mStateDepth].mapKeyList[curMapIndex].c_str();
            return strvalue;
        }

        //  else the current lookup key is the <tag>|<mapkey>
    }

    ParamMap::iterator itr = mParamMap.find(mKey);
    if (itr != mParamMap.end())
    {
        strvalue = itr->second.getData();
    }
    return strvalue;
}

/*************************************************************************************************/
/*!
    \brief getKeyValueLen

    Returns the value length based on the current decode key.  Returned value depends on
    current state.  For example, the Map state retrieves values based on whether it's in a "mapkey"
    state versus a "mapvalue" state.

    getKeyValue() should have been called before this method.

    \return - nullptr char8_t string if no value available for current key.
*/
/*************************************************************************************************/
size_t Xml2Decoder::getKeyValueLen()
{
    size_t strvaluelen = 0;

    if (mStateStack[mStateDepth].state == STATE_MAP)
    {
        uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;

        if (!mStateStack[mStateDepth].readValue)
        {
            strvaluelen = mStateStack[mStateDepth].mapKeyList[curMapIndex].length();
            return strvaluelen;
        }

        //  else the current lookup key is the <tag>|<mapkey>
    }

    ParamMap::iterator itr = mParamMap.find(mKey);
    if (itr != mParamMap.end())
    {
        strvaluelen = itr->second.getDataLen();
    }
    return strvaluelen;
}

/*************************************************************************************************/
/*!
    \brief convertMemberToElement

    Utility method to take a member name and convert it into an element name by doing the 
    following: convert everything to lowercase

    \param[in] memberName - unmodified member name
    \param[in] buf - where to write the modified member
    \param[in] buflen - length in bytes of buf

    \return - true if conversion fits in the provided buffer, false if there isn't enough room or parameters are invalid
*/
/*************************************************************************************************/
bool Xml2Decoder::convertMemberToElement(const char8_t *memberName, char8_t* buf, const size_t buflen) const
{
    if (memberName == nullptr || *memberName == '\0' || buf == nullptr || buflen == 0)
    {
        return false;       // Bad parameters
    }
    // Now copy the member into our new storage and make it lowercase as well.
    uint32_t len = 0;
    for ( ; len < buflen-1 ; ++len)
    {
        buf[len] = LocaleTokenToLower(memberName[len]);
        if (buf[len] == '\0')
        {
            break;          // All done
        }
    }

    if (len == buflen && memberName[len] != '\0')
    {
        // Passed in buffer was too small
        return false;
    }

    return true;
}

void Xml2Decoder::growString(char8_t*& buf, size_t& bufSize, size_t bufCount, size_t bytesNeeded)
{
    if (bufSize - bufCount > bytesNeeded)
    {
        return;
    }
    if (bytesNeeded < 1024)
    {
        bytesNeeded = 1024;
    }
    size_t newSize = bufSize + bytesNeeded;
#ifdef BLAZE_CLIENT_SDK
    char8_t* newBuf = BLAZE_NEW_ARRAY(char8_t, newSize + 1, MEM_GROUP_FRAMEWORK, "XmlStringBuf");
#else
    char8_t* newBuf = BLAZE_NEW_ARRAY(char8_t, newSize + 1);
#endif

    EA_ASSERT(newBuf != nullptr);

    if (buf != nullptr)
    {
        memcpy(newBuf, buf, bufCount);
#ifdef BLAZE_CLIENT_SDK
         BLAZE_DELETE_ARRAY(MEM_GROUP_FRAMEWORK, buf);
#else
        delete[] buf;
#endif
    }
    bufSize = newSize;
    buf = newBuf;
    buf[bufCount] = '\0';
}

/*************************************************************************************************/
/*!
    \brief pushKey

    Push the member name of a tag onto the buffer that maintains the current param name.

    \param[in]  parentTdf - TDF with the TagInfoMap
    \param[in]  tag - tag value to push

    \return - true if successful, false if any error occurred
*/
/*************************************************************************************************/
bool Xml2Decoder::pushKey(const EA::TDF::Tdf &parentTdf, uint32_t tag)
{
    if (mStateStack[mStateDepth].state == STATE_MAP)
    {
        //  trying to read more elements than available - fail
        if (mStateStack[mStateDepth].dimensionIndex == mStateStack[mStateDepth].dimensionSize)
        {
            ++mErrorCount;
            return false;
        }

        BLAZE_EASTL_STRING& index = mStateStack[mStateDepth].mapKeyList[mStateStack[mStateDepth].dimensionIndex];
        pushMapIndex(index.c_str(), index.length());
        return true;
    }
    else if (mStateStack[mStateDepth].state == STATE_ARRAY)
    {
        if (mStateStack[mStateDepth].readVariableTdf == StateStruct::VARTDF_TDFID)
        {
            //  variable tdf arrays will already have the desired key - no additional processing needed
            return true;
        }
    }

    const EA::TDF::TdfMemberInfo *info = nullptr;
    bool success = parentTdf.getMemberInfoByTag(tag, info);
    if (!success)
    {
        ++mErrorCount;
        return false;
    }
    // XML2 decoder supports the member name override if one is defined
    const char *memberName = info->getMemberName();
    if (memberName == nullptr)
    {
        ++mErrorCount;
        return false;
    }
    char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";
    if (!convertMemberToElement(memberName, elementName, sizeof(elementName)))
    {
        ++mErrorCount;
        return false;
    }

    if (mStateStack[mStateDepth].state == STATE_UNION)
    {
        // expect the "valu" element if the tag is not set
        if (info->getTag() == EA::TDF::TdfUnion::RESERVED_VALUE_TAG)
        {
            pushKey("valu", 4);
            return true;
        }
    }
    
    pushKey(elementName, strlen(elementName));

    if (mStateStack[mStateDepth].state == STATE_ARRAY)
    {
        // in this case, the array is of primitive types
        // we must make an exception in the variable tdf array case since variable tdfs will read the tdfid here - we don't
        // want to count that as incrementing the dimension index (that is taken care of in the corresponding visit method.)
        if (mStateStack[mStateDepth].dimensionIndex > 0)
        {
            pushArrayIndex(mStateStack[mStateDepth].dimensionIndex);
        }
        ++mStateStack[mStateDepth].dimensionIndex;
    }
    return true;
}

/*************************************************************************************************/
/*!
    \brief pushKey

    Push a tag onto the buffer that maintains the current param name.  Takes care of constructing
    the element name and does any special handling for specific element types.

    \param[in]  name - the tag to push
    \param[in]  nameLen - length of the tag name to push
*/
/*************************************************************************************************/
void Xml2Decoder::pushKey(const char8_t* name, size_t nameLen)
{
    char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";
    if (nameLen >= sizeof(elementName))
    {
        nameLen = sizeof(elementName) - 1;
    }
    size_t elementNameLen;


    // make lowercase and replace '-' with '_' because we don't support '-' in TDF member names
    for (elementNameLen = 0; elementNameLen < nameLen; ++elementNameLen)
    {
        if (name[elementNameLen] == '-')
        {
            elementName[elementNameLen] = '_';
        }
        else
        {
            elementName[elementNameLen] = LocaleTokenToLower(name[elementNameLen]);
        }

        if (elementName[elementNameLen] == '\0')
        {
            break;
        }
    }

    elementName[sizeof(elementName) - 1] = '\0'; // Ensure long element names are truncated

    // Remove any "Response" suffix (e.g. DummyResponse) -- it's not encoded in the XML.
    if (elementNameLen > 8) // 8 characters in "response"
    {
        char8_t* response = &(elementName[elementNameLen - 8]);
        if (strcmp(response, "response") == 0)
        {
            *response = '\0';   // Remove the response
            elementNameLen -= 8;
        }
    }

    growString(mKey, mKeyBufSize, mKeyLen, elementNameLen + 1); // +1 for delimiter

    if (mKeyLen == 0)
    {
        // first
        mKeyLen += blaze_snzprintf(mKey, mKeyBufSize, "%.*s", (int32_t)elementNameLen, elementName);
    }
    else
    {
        mKeyLen += blaze_snzprintf(mKey + mKeyLen, mKeyBufSize - mKeyLen, "/%.*s", (int32_t)elementNameLen, elementName);
    }
}

/*************************************************************************************************/
/*!
    \brief pushArrayIndex

    Push an array index onto the buffer that maintains the current param name.

    \param[in]  index - the index to push
*/
/*************************************************************************************************/
void Xml2Decoder::pushArrayIndex(uint32_t index)
{
    growString(mKey, mKeyBufSize, mKeyLen, 32); // 32 is more than enough for uint32_t and delimiter

    // We assume when pushing an index key that since it is inside an array there must
    // already be some data in mKey and hence it is safe to always precede it with delimiter
    mKeyLen += blaze_snzprintf(mKey + mKeyLen, mKeyBufSize - mKeyLen, "|%u", index);
}

/*************************************************************************************************/
/*!
    \brief pushMapIndex

    Push a map index tag onto the buffer that maintains the current param name.

    \param[in]  name - the tag to push
    \param[in]  nameLen - length of the tag name to push
*/
/*************************************************************************************************/
void Xml2Decoder::pushMapIndex(const char8_t* name, size_t nameLen)
{
    growString(mKey, mKeyBufSize, mKeyLen, nameLen + 1); // +1 for delimiter

    // We assume when pushing an index key that since it is inside an array there must
    // already be some data in mKey and hence it is safe to always precede it with delimiter
    mKeyLen += blaze_snzprintf(mKey + mKeyLen, mKeyBufSize - mKeyLen, "|%.*s", (int32_t)nameLen, name);
}

/*************************************************************************************************/
/*!
    \brief popKey

    Pop the last key off of the buffer that maintains the current param name.

    \return - true if successful, false if any error occurred
*/
/*************************************************************************************************/
bool Xml2Decoder::popKey()
{
    EA_ASSERT(mKey != nullptr);

    char8_t* last = nullptr;
    switch (mStateStack[mStateDepth].state)
    {
    case STATE_MAP:
        if (mStateStack[mStateDepth].readValue)
        {
            //  reading the map value - if we're not reading the tdfid in this case, then we're done
            //  reading the map's value.
            if (mStateStack[mStateDepth].readVariableTdf != StateStruct::VARTDF_TDFID)
            {
                //  if a variable tdf map, state == VARTDF_ID or state == VARTDF_TDF 
                //  (latter indicates that we're finished reading the value)
                ++mStateStack[mStateDepth].dimensionIndex;
                mStateStack[mStateDepth].readValue = false;
            }
        }
        else
        {
            //  finished reading the map key.
            mStateStack[mStateDepth].readValue = true;
        }
        last = strrchr(mKey, '|');
        break;
    case STATE_ARRAY:
        if (mStateStack[mStateDepth].readVariableTdf == StateStruct::VARTDF_TDFID)
        {
            //  still parsing the variable tdfid - so we don't want to trash the array key.
            //  effectively reverse the action of pushKey() here, which was a no-op, so just return.
            return true;
        }
    case STATE_UNION:
    case STATE_NORMAL:
        last = strrchr(mKey, '/');
        break;
    default:
        break;
    }

    if (last == nullptr)
    {
        // If we can't find a delimiter then we're trying to pop the only key,
        // but if there is no key to pop at all then that's an error
        if (mKey[0] == '\0')
        {
            ++mErrorCount;
            return false;
        }
        else
        {
            last = mKey;
        }
    }
    *last = '\0';
    mKeyLen = last - mKey;
    return true;
}

/*************************************************************************************************/
/*!
    \brief popIndex

    Pop the last index off of the buffer that maintains the current param name.

    \return - true if successful, false if any error occurred
*/
/*************************************************************************************************/
bool Xml2Decoder::popIndex()
{
    EA_ASSERT(mKey != nullptr);
    char8_t* last = strrchr(mKey, '|');
    if (last == nullptr)
    {
        // If we can't find a delimiter then we're trying to pop the only key,
        // but if there is no key to pop at all then that's an error
        if (mKey[0] == '\0')
        {
            ++mErrorCount;
            return false;
        }
        else
        {
            last = mKey;
        }
    }
    *last = '\0';
    mKeyLen = last - mKey;
    return true;
}

/*************************************************************************************************/
/*!
    \brief pushStack

    Adds another level to the state depth stack

    \param[in]  state - state of the level to add

    \return - none
*/
/*************************************************************************************************/
void Xml2Decoder::pushStack(State state)
{
    ++mStateDepth;
    EA_ASSERT(mStateDepth < MAX_STATE_DEPTH);
    mStateStack[mStateDepth].state = state;
    mStateStack[mStateDepth].readVariableTdf = StateStruct::VARTDF_NULL;
}

/*************************************************************************************************/
/*!
    \brief popStack

    Updates the state depth stack, and whether we are at the top level of the decode

    \return - true if stack was popped, false if we're already at top level
*/
/*************************************************************************************************/
bool Xml2Decoder::popStack()
{
    if (mStateDepth > 0)
    {
        --mStateDepth;

        if (mStateDepth == 0)
        {
            mAtTopLevel = true;
        }

        return true;
    }

    return false;
}

} // Blaze

#else // ENABLE_LEGACY_CODECS

#include "framework/blaze.h"
#include "framework/protocol/shared/xml2decoder.h"
#include "EAIO/EAStream.h"
#include "framework/util/shared/rawbufferistream.h"
#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif
#include "EATDF/tdfbasetypes.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief Xml3Decoder

    This class provides a mechanism to decode XML into a TDF object.

    \param[in]  buffer - the buffer to decode from

*/
/*************************************************************************************************/
#ifdef BLAZE_CLIENT_SDK
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    Blaze::BlazeError Xml2Decoder::decode(RawBuffer& buffer, EA::TDF::Tdf& tdf, bool onlyDecodeChanged)
#else
    Blaze::BlazeError Xml2Decoder::decode(RawBuffer& buffer, EA::TDF::Tdf& tdf)
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
#else
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    BlazeRpcError Xml2Decoder::decode(RawBuffer& buffer, EA::TDF::Tdf& tdf, bool onlyDecodeChanged)
#else
    BlazeRpcError Xml2Decoder::decode(RawBuffer& buffer, EA::TDF::Tdf& tdf)
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
#endif //BLAZE_CLIENT_SDK
    {
        RawBufferIStream istream(buffer);
        EA::TDF::MemberVisitOptions visitOpt;
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        visitOpt.onlyIfSet = onlyDecodeChanged;
#endif
        bool result = mDecoder.decode(istream, tdf, visitOpt);
        if (result)
            return ERR_OK;

#ifdef BLAZE_CLIENT_SDK
        BLAZE_SDK_DEBUGF("[Xml2Decoder].decode: Failed to decode tdf: %s\n", mDecoder.getErrorMessage());
#else
        BLAZE_TRACE_LOG(Log::SYSTEM, "[Xml2Decoder].decode: Failed to decode tdf: " << mDecoder.getErrorMessage());
#endif

        if (blaze_stricmp("ERR_INVALID_TDF_ENUM_VALUE", mDecoder.getErrorMessage()) == 0)
            return ERR_INVALID_TDF_ENUM_VALUE;

        return ERR_SYSTEM;
    }

} // Blaze
#endif // ENABLE_LEGACY_CODECS
