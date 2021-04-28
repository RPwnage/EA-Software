/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class JsonDecoder

This class provides a mechanism to decode data using the JSON format, for example:

{
    "classname" :
    {
        "intMember" : 2,
        "arrayname":
        [
            "111",
            "222"
        ],
        "mapname":
        {
            "key1" : 1,
            "key2" : 2
        },
        "unionname":
        {
            "mem1": 1 // The active member will be indicated by the member name.
        },
        "variablename":
        {
            "tdfid" : xxxx,
            "classname" : "classname",
            "value":
            {
                "mem1" : 1
            }
        }
    }
}
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
#include "framework/protocol/shared/jsondecoder.h"
#include "EASTL/sort.h" // for eastl::sort() in JsonDecoder::visit

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif

//SDK does not use the standard "EA_ASSERT" so we redefine it here as "BlazeAssert"
#if !defined(EA_ASSERT) && defined(BlazeAssert)
#define EA_ASSERT(X) BlazeAssert(X)
#endif

namespace Blaze
{


JsonDecoder::JsonDecoder() :
    mStateDepth(0),
#ifdef BLAZE_CLIENT_SDK
    mDomNodeStack(MEM_GROUP_DEFAULT, "JsonDecoder::mDomNodeStack"),
    mFilterTags(MEM_GROUP_DEFAULT, "JsonDecoder::mFilterTags"),
#endif
    mFilterIndex(0),
    mEnabled(true)
{
#ifdef BLAZE_CLIENT_SDK
    mMemGroupId = MEM_GROUP_DEFAULT;
#else
    mMemGroupId = DEFAULT_BLAZE_MEMGROUP;
#endif
}

JsonDecoder::JsonDecoder(MemoryGroupId mMemGroupId) :
    mStateDepth(0),
#ifdef BLAZE_CLIENT_SDK
    mDomNodeStack(mMemGroupId, "JsonDecoder::mDomNodeStack"),
    mFilterTags(mMemGroupId, "JsonDecoder::mFilterTags"),
#endif
    mFilterIndex(0),
    mEnabled(true)
{
    this->mMemGroupId = mMemGroupId;
}

JsonDecoder::~JsonDecoder()
{
}


void JsonDecoder::setBuffer(RawBuffer* buffer)
{
    mBuffer = buffer;

    // Important to set state depth to 0, as nested structs will result in multiple calls to
    // begin() during decoding, and only the call when state depth is 0 will perform initialization
    mStateDepth = 0;
}

bool JsonDecoder::visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue)
{
    // Initialize our state
    mErrorCount = 0;
    mValidationError = Blaze::ERR_OK;
        
    //allow JsonDecoder to take in a MemoryGroupId for different allocator
    JsonDomDocument domDocument(Blaze::Allocator::getAllocator(mMemGroupId));
    JsonDomReader domReader(Blaze::Allocator::getAllocator(mMemGroupId));
    domReader.SetString((char*)mBuffer->data(), mBuffer->datasize(), false);

    //construct the document
    Result result = domReader.Build(domDocument);

    if (result != kSuccess)
    {
        ++mErrorCount;
        return false;
    }

    mStateStack[0].state = STATE_NORMAL;

    //we are expecting a single item in the tree
    if (domDocument.mJsonDomNodeArray.size() != 1)
    {
        return false;
    }
    mDomNodeStack.push_back(domDocument.mJsonDomNodeArray.at(0));

    tdf.visit(*this, tdf, tdf);
    popJsonNode();

    return (mErrorCount == 0);
}

bool JsonDecoder::visit(EA::TDF::TdfUnion &tdf, const EA::TDF::TdfUnion &referenceValue)
{
    // UNIONS MUST BE INSIDE ANOTHER TDF"
    return false;
}

bool JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue)
{
    bool matchesTag = false;
    if (!mEnabled && (mFilterIndex == (uint32_t)mFilterTags.size() - 1) && (mFilterTags.at(mFilterIndex) == tag))
    {
        //we've found the tdf that we are looking for, so start decoding
        mEnabled = true;
        matchesTag = true;
    }

    if (mEnabled)
    {
        EA::TDF::TdfId tdfId = EA::TDF::INVALID_TDF_ID;
        bool isSuccess = true;

        if (!matchesTag)
        {
            //since we are doing a partial decode, the variable is already at the top of the json node stack
            isSuccess = pushJsonNode(parentTdf, tag);
        }

        if (isSuccess)
        {
            //get the tdfid node from the current JsonDomNode and create the variable.
            JsonDomObjectValue* objectValue = getJsonDomObjectValue("tdfid", false);
            if (objectValue != nullptr)
            {
                tdfId = (EA::TDF::TdfId)getJsonDomInteger(objectValue->mpNode);
                value.create(tdfId);
                if (!value.isValid())
                {
                    ++mErrorCount;
                    return false;
                }

                pushStack(STATE_VARIABLE);
                visit(rootTdf, parentTdf, tag, *value.get(), *value.get());
                popStack();
            }

            if (!matchesTag)
            {
                popJsonNode();
            }
        }
    }

    if (matchesTag)
    {
        //we were only decoding this element, so turn off decoding for the rest of the tdf
        mEnabled = false;
        mFilterIndex = 0;
    }
    return true;
}

bool JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue)
{
    bool matchesTag = false;
    if (!mEnabled && (mFilterIndex == (uint32_t)mFilterTags.size() - 1) && (mFilterTags.at(mFilterIndex) == tag))
    {
        //we've found the tdf that we are looking for, so start decoding
        mEnabled = true;
        matchesTag = true;
    }

    if (mEnabled)
    {
        EA::TDF::TdfId tdfId = EA::TDF::INVALID_TDF_ID;
        bool isSuccess = true;

        if (!matchesTag)
        {
            //since we are doing a partial decode, the variable is already at the top of the json node stack
            isSuccess = pushJsonNode(parentTdf, tag);
        }

        if (isSuccess)
        {
            //get the tdfid node from the current JsonDomNode and create the variable.
            JsonDomObjectValue* objectValue = getJsonDomObjectValue("tdfid", false);
            if (objectValue != nullptr)
            {
                tdfId = (EA::TDF::TdfId)getJsonDomInteger(objectValue->mpNode);
                value.create(tdfId);
                if (!value.isValid())
                {
                    ++mErrorCount;
                    return false;
                }

                pushStack(STATE_VARIABLE);
                visitReference(rootTdf, parentTdf, tag, value.get(), &value.get());
                popStack();
            }

            if (!matchesTag)
            {
                popJsonNode();
            }
        }
    }

    if (matchesTag)
    {
        //we were only decoding this element, so turn off decoding for the rest of the tdf
        mEnabled = false;
        mFilterIndex = 0;
    }
    return true;
}

bool JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue)
{
    bool matchesTag = false;
    bool matchesTagValue = ((mFilterIndex < mFilterTags.size()) && mFilterTags.at(mFilterIndex) == tag);

    if (mEnabled || matchesTagValue)
    {
        if (mFilterIndex == (uint32_t)mFilterTags.size() - 1)
        {
            //we've found the tdf that we are looking for, so start decoding
            mEnabled = true;
            matchesTag = true;
        }
        else
        {
            //we need to travel further down the tdf member hierarchy to find what we are looking for.
            ++mFilterIndex;
        }

        if (!mFilterTags.empty() && matchesTagValue && (mStateStack[mStateDepth].state != STATE_MAP) && (mStateStack[mStateDepth].state != STATE_ARRAY))
        {
            //since we are trying to decode a single json node, it is already on the stack
            pushStack(STATE_NORMAL);
            value.visit(*this, rootTdf, value);
            popStack();

            if (matchesTag)
            {
                //we were only decoding this element, so turn off decoding for the rest of the tdf
                mEnabled = false;
                mFilterIndex = 0;
            }
        }
        else if (pushJsonNode(parentTdf, tag))
        {
            pushStack(STATE_NORMAL);

            value.visit(*this, rootTdf, value);

            popStack();
            popJsonNode();
        }
    }

    return (mErrorCount == 0);
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue)
{
    bool matchesTag = false;
    if (!mEnabled && (mFilterIndex == (uint32_t)mFilterTags.size() - 1) && (mFilterTags.at(mFilterIndex) == tag))
    {
        //we've found the tdf that we are looking for, so enable decoding
        mEnabled = true;
        matchesTag = true;
    }
    
    if (mEnabled)
    {
        bool isSuccess = true;
        if (!matchesTag)
        {
            //since we are doing a partial decode, the vector is already at the top of the json node stack
            isSuccess = pushJsonNode(parentTdf, tag);
        }

        if (isSuccess)
        {
            pushStack(STATE_ARRAY);
    
            //current Node should be the JsonDomArray
            mStateStack[mStateDepth].dimensionSize = (uint32_t)getJsonDomNodeArraySize(mDomNodeStack.back());
    
            if (mStateStack[mStateDepth].dimensionSize > 0)
            {
                value.initVector(mStateStack[mStateDepth].dimensionSize);
            }
    
            value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
    
            popStack();
            if (!matchesTag)
            {
                popJsonNode();
            }
        }
    }

    if (matchesTag)
    {
        //we were only decoding this element, so turn off decoding for the rest of the tdf
        mEnabled = false;
        mFilterIndex = 0;
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue)
{
    bool matchesTag = false;
    if (!mEnabled && (mFilterIndex == (uint32_t)mFilterTags.size() - 1) && (mFilterTags.at(mFilterIndex) == tag))
    {
        //we've found the tdf that we are looking for, so enable decoding
        mEnabled = true;
        matchesTag = true;
    }

    if (mEnabled)
    {
        bool isSuccess = true;

        if (!matchesTag)
        {
            //since we are doing a partial decode, the map is already at the top of the json node stack
            isSuccess = pushJsonNode(parentTdf, tag);
        }

        if (isSuccess)
        {
            pushStack(STATE_MAP);

            if (mDomNodeStack.back() != nullptr && mDomNodeStack.back()->GetNodeType() == kETBeginObject)
            {
                JsonDomObjectValueArray::const_iterator objectValueIter = mDomNodeStack.back()->AsJsonDomObject()->mJsonDomObjectValueArray.begin();
                JsonDomObjectValueArray::const_iterator objectValueEnd = mDomNodeStack.back()->AsJsonDomObject()->mJsonDomObjectValueArray.end();

                for (; objectValueIter != objectValueEnd; ++objectValueIter)
                {
                    //avoid adding duplicate keys
                    MapKeyList::const_iterator keyItr = mStateStack[mStateDepth].mapKeyList.begin();
                    MapKeyList::const_iterator keyEnd = mStateStack[mStateDepth].mapKeyList.end();
                    for (; keyItr != keyEnd; ++keyItr)
                        if (blaze_strcmp(keyItr->c_str(), (*objectValueIter).mNodeName.c_str()) == 0)
                            break;
                    if (keyItr == keyEnd)
                        mStateStack[mStateDepth].mapKeyList.push_back((*objectValueIter).mNodeName.c_str());
                }
            }

            //map keys need to be sorted
            if (value.getKeyTypeDesc().asEnumMap() != nullptr)
            {
                eastl::sort(mStateStack[mStateDepth].mapKeyList.begin(), mStateStack[mStateDepth].mapKeyList.end(), EnumKeyCompare(value.getKeyTypeDesc().asEnumMap()));
            }
            else if (value.getTypeDescription().asMapDescription()->keyType.isIntegral())
            {
                eastl::sort(mStateStack[mStateDepth].mapKeyList.begin(), mStateStack[mStateDepth].mapKeyList.end(), &compareMapKeysInteger);
            }
            else
            {
                eastl::sort(mStateStack[mStateDepth].mapKeyList.begin(), mStateStack[mStateDepth].mapKeyList.end(), &compareMapKeysString);
            }

            //  reset state variables for map field decoding
            mStateStack[mStateDepth].dimensionSize = static_cast<uint32_t>(mStateStack[mStateDepth].mapKeyList.size());
            mStateStack[mStateDepth].dimensionIndex = 0;

            value.initMap(mStateStack[mStateDepth].dimensionSize);
            value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);

            popStack();

            if (!matchesTag)
            {
                popJsonNode();
            }
        }
    }
    
    if (matchesTag)
    {
        //we were only decoding this element, so turn off decoding for the rest of the tdf
        mEnabled = false;
        mFilterIndex = 0;
    }
}

bool JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue)
{

    bool matchesTag = false;
    if (!mEnabled && (mFilterIndex < mFilterTags.size()) && mFilterTags.at(mFilterIndex) == tag)
    {
        if (mFilterIndex == (uint32_t)mFilterTags.size() - 1)
        {
            //we've found the tdf that we are looking for, so start decoding
            mEnabled = true;
            matchesTag = true;
        }
        else
        {
            //we need to travel further down the tdf member hierarchy to find what we are looking for.
            ++mFilterIndex;
        }
    }

    if (mEnabled)
    {
        bool isSuccess = true;

        if (!matchesTag)
        {
            //since we are doing a partial decode, the union is already at the top of the json node stack
            isSuccess = pushJsonNode(parentTdf, tag);
        }

        if (isSuccess)
        {
            pushStack(STATE_UNION);

            //In Union just has one active member, so in the current JsonDomNode only has one node, and the node should be the active member.
            JsonDomObjectValue* objectValue = getFirstJsonDomObjectValue();
            if (objectValue != nullptr)
            {
                JsonDomObjectValue& unionNode = *objectValue;
                const char* index = unionNode.mNodeName.c_str();
                const EA::TDF::TdfMemberInfo* memberInfo = nullptr;
                uint32_t memberIndex = 0;

                bool foundMember = value.getMemberInfoByName(index, memberInfo, &memberIndex);
                if (foundMember)
                {
                    if (memberIndex != EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
                    {
                        value.switchActiveMember(memberIndex);
                        mDomNodeStack.push_back(unionNode.mpNode);
                    }
                }

                value.visit(*this, rootTdf, value);
            }

            popStack();

            if (!matchesTag)
            {
                popJsonNode();
            }
        }
    }

    if (matchesTag)
    {
        //we were only decoding this element, so turn off decoding for the rest of the tdf
        mEnabled = false;
        mFilterIndex = 0;
    }
    return (mErrorCount == 0);
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue)
{
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        if (mDomNodeStack.back()->GetNodeType() == kETString)
        {
            const char8_t *tmp = getJsonDomString(mDomNodeStack.back());
            value = ((blaze_stricmp(tmp, "true") == 0) || (strcmp(tmp, "1") == 0));
        }
        else
        {
            value = getJsonDomBool(mDomNodeStack.back(), defaultValue);
        }
        popJsonNode();
    }
    else
    {
        value = defaultValue;
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        const char8_t *keyvalue = nullptr;
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;

            if (!mStateStack[mStateDepth].readValue)
            {
                keyvalue = mStateStack[mStateDepth].mapKeyList[curMapIndex].c_str();
                if (keyvalue != nullptr)
                    blaze_str2int(keyvalue, &value);
                else
                    value = defaultValue;
            }
            else
            {
                value = static_cast<char8_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
            }
        }
        else
        {
            value = static_cast<char8_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
        }

        popJsonNode();
    }
    else
    {
        value = defaultValue;
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        const char8_t *keyvalue = nullptr;
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;

            if (!mStateStack[mStateDepth].readValue)
            {
                keyvalue = mStateStack[mStateDepth].mapKeyList[curMapIndex].c_str();
                if (keyvalue != nullptr)
                    blaze_str2int(keyvalue, &value);
                else
                    value = defaultValue;
            }
            else
            {
                value = static_cast<int8_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
            }
        }
        else
        {
            value = static_cast<int8_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
        }
        popJsonNode();
    }
    else
    {
        value = defaultValue;
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        const char8_t *keyvalue = nullptr;
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;

            if (!mStateStack[mStateDepth].readValue)
            {
                keyvalue = mStateStack[mStateDepth].mapKeyList[curMapIndex].c_str();
                if (keyvalue != nullptr)
                    blaze_str2int(keyvalue, &value);
                else
                    value = defaultValue;
            }
            else
            {
                value = static_cast<uint8_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
            }
        }
        else
        {
            value = static_cast<uint8_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
        }
        popJsonNode();
    }
    else
    {
        value = defaultValue;
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        const char8_t *keyvalue = nullptr;
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;

            if (!mStateStack[mStateDepth].readValue)
            {
                keyvalue = mStateStack[mStateDepth].mapKeyList[curMapIndex].c_str();
                if (keyvalue != nullptr)
                    blaze_str2int(keyvalue, &value);
                else
                    value = defaultValue;
            }
            else
            {
                value = static_cast<int16_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
            }
        }
        else
        {
            value = static_cast<int16_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
        }
        popJsonNode();
    }
    else
    {
        value = defaultValue;
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        const char8_t *keyvalue = nullptr;
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;

            if (!mStateStack[mStateDepth].readValue)
            {
                keyvalue = mStateStack[mStateDepth].mapKeyList[curMapIndex].c_str();
                if (keyvalue != nullptr)
                    blaze_str2int(keyvalue, &value);
                else
                    value = defaultValue;
            }
            else
            {
                value = static_cast<uint16_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
            }
        }
        else
        {
            value = static_cast<uint16_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
        }
        popJsonNode();
    }
    else
    {
        value = defaultValue;
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        const char8_t *keyvalue = nullptr;
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;

            if (!mStateStack[mStateDepth].readValue)
            {
                keyvalue = mStateStack[mStateDepth].mapKeyList[curMapIndex].c_str();
                if (keyvalue != nullptr)
                    blaze_str2int(keyvalue, &value);
                else
                    value = defaultValue;
            }
            else
            {
                value = static_cast<int32_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
            }
        }
        else
        {
            value = static_cast<int32_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
        }
        popJsonNode();
    }
    else
    {
        value = defaultValue;
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue)
{
    visit(rootTdf, parentTdf, tag, value.getBits(), value.getBits(), 0);
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        const char8_t *keyvalue = nullptr;
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;

            if (!mStateStack[mStateDepth].readValue)
            {
                keyvalue = mStateStack[mStateDepth].mapKeyList[curMapIndex].c_str();
                if (keyvalue != nullptr)
                    blaze_str2int(keyvalue, &value);
                else
                    value = defaultValue;
            }
            else
            {
                value = static_cast<uint32_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
            }
        }
        else
        {
            value = static_cast<uint32_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
        }
        popJsonNode();
    }
    else
    {
        value = defaultValue;
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        const char8_t *keyvalue = nullptr;
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;

            if (!mStateStack[mStateDepth].readValue)
            {
                keyvalue = mStateStack[mStateDepth].mapKeyList[curMapIndex].c_str();
                if (keyvalue != nullptr)
                    blaze_str2int(keyvalue, &value);
                else
                    value = defaultValue;
            }
            else
            {
                value = getJsonDomInteger(mDomNodeStack.back(), defaultValue);
            }
        }
        else
        {
            value = getJsonDomInteger(mDomNodeStack.back(), defaultValue);
        }
        popJsonNode();
    }
    else
    {
        value = defaultValue;
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        const char8_t *keyvalue = nullptr;
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;

            if (!mStateStack[mStateDepth].readValue)
            {
                keyvalue = mStateStack[mStateDepth].mapKeyList[curMapIndex].c_str();
                if (keyvalue != nullptr)
                    blaze_str2int(keyvalue, &value);
                else
                    value = defaultValue;
            }
            else
            {
                value = static_cast<uint64_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
            }
        }
        else
        {
            value = static_cast<uint64_t>(getJsonDomInteger(mDomNodeStack.back(), defaultValue));
        }
        popJsonNode();
    }
    else
    {
        value = defaultValue;
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        value = static_cast<float>(getJsonDomDouble(mDomNodeStack.back(), defaultValue));
        popJsonNode();
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && blaze_strcmp(value, defaultValue) != 0)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        const char8_t *keyvalue = nullptr;
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;

            if (!mStateStack[mStateDepth].readValue)
            {
                keyvalue = mStateStack[mStateDepth].mapKeyList[curMapIndex].c_str();
            }
            else
            {
                keyvalue = getJsonDomString(mDomNodeStack.back());
            }
        }
        else
        {
            keyvalue = getJsonDomString(mDomNodeStack.back());
        }
        if (keyvalue != nullptr)
        {
            size_t keyvaluelen = strlen(keyvalue);
            value.set(keyvalue, (EA::TDF::TdfStringLength)keyvaluelen);
        }
        else
        {
            value.set(defaultValue);
        }
        popJsonNode();
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue)
{
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        JsonDomObjectValue* objectValue = getJsonDomObjectValue("data", false);
        if (objectValue != nullptr)
        {
            const char8_t* blobValue = getJsonDomString(objectValue->mpNode);
            if (blobValue != nullptr)
            {
                EA::TDF::TdfSizeVal blobValueLen = (EA::TDF::TdfSizeVal)strlen(blobValue);
                JsonDomObjectValue* encObjectValue = getJsonDomObjectValue("enc", false);
                if (encObjectValue != nullptr)
                {
                    const char8_t* encValue = getJsonDomString(encObjectValue->mpNode);
                    if (encValue != nullptr && blaze_stricmp(encValue, "base64") == 0)
                    {
                        value.decodeBase64(blobValue, blobValueLen);
                        popJsonNode();
                        return;
                    }
                }

                value.setData((const uint8_t*)blobValue, blobValueLen);
            }
        }
        popJsonNode();
    }
}

// enum decoder doing type-checking
void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        const char8_t* keyvalue = nullptr;
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            uint32_t curMapIndex = mStateStack[mStateDepth].dimensionIndex;
            if (!mStateStack[mStateDepth].readValue && (mStateStack[mStateDepth].mapKeyList.size() > curMapIndex))
            {
                keyvalue = mStateStack[mStateDepth].mapKeyList[curMapIndex].c_str();
            }
            else
            {
                keyvalue = getJsonDomString(mDomNodeStack.back());
            }
        }
        else
        {
            keyvalue = getJsonDomString(mDomNodeStack.back());
        }
        if (keyvalue != nullptr)
        {
            size_t keyvaluelen = strlen(keyvalue);
            BLAZE_EASTL_STRING trimmedKeyValue(keyvalue, (BLAZE_EASTL_STRING::size_type)keyvaluelen);
            if ((enumMap == nullptr) || !enumMap->findByName(trimmedKeyValue.c_str(), value))
            {
                // if value is not enum constant identifier
                // try and see if it's number (backwards compatibility)
                if (*blaze_str2int(trimmedKeyValue.c_str(), &value) != '\0')
                {
                    // not a number? then it's error
                    ++mErrorCount;
#ifndef BLAZE_CLIENT_SDK
                    char8_t buf[5];
                    Heat2Util::decodeTag(tag, buf, 5);
                    BLAZE_WARN_LOG(Log::HTTP, "[JsonDecoder].visit: Error decoding enum for field " << buf << " in " << parentTdf.getClassName() << ". Value is not in enum, and is not an int!");
#endif
                    mValidationError = ERR_INVALID_TDF_ENUM_VALUE;
                }
            }
        }
        else
            value = defaultValue;

        popJsonNode();
    }
    else
    {
        value = defaultValue;
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        JsonDomObjectValue* objectValue = getJsonDomObjectValue("component", false);
        if (objectValue != nullptr)
        {
            value.component = (ComponentId)getJsonDomInteger(objectValue->mpNode);
        }
        else
        {
            value.component = defaultValue.component;
        }
        objectValue = getJsonDomObjectValue("type", false);
        if (objectValue != nullptr)
        {
            value.type = (EntityType)getJsonDomInteger(objectValue->mpNode);
        }
        else
        {
            value.type = defaultValue.type;
        }
        popJsonNode();
    }

}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        JsonDomObjectValue* objectValue = getJsonDomObjectValue("type", false);

        if (objectValue != nullptr)
        {
            mDomNodeStack.push_back(objectValue->mpNode);

            objectValue = getJsonDomObjectValue("component", false);
            if (objectValue != nullptr)
            {
                value.type.component = (ComponentId)getJsonDomInteger(objectValue->mpNode);
            }
            else
            {
                value.type.component = defaultValue.type.component;
            }
            objectValue = getJsonDomObjectValue("type", false);
            if (objectValue != nullptr)
            {
                value.type.type = (EntityType)getJsonDomInteger(objectValue->mpNode);
            }
            else
            {
                value.type.type = defaultValue.type.type;
            }
            mDomNodeStack.pop_back();
        }
        else
        {
            value.type.component = defaultValue.type.component;
            value.type.type = defaultValue.type.type;
        }

        objectValue = getJsonDomObjectValue("id", false);
        if (objectValue != nullptr)
        {
            value.id = getJsonDomInteger(objectValue->mpNode);
        }
        else
        {
            value.id = defaultValue.id;
        }
        popJsonNode();
    }
}

void JsonDecoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue &referenceValue, const TimeValue defaultValue)
{
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    if (mOnlyDecodeChanged && value != defaultValue)
        return;
#endif
    if (mEnabled && pushJsonNode(parentTdf, tag))
    {
        value = TimeValue(getJsonDomInteger(mDomNodeStack.back(), defaultValue.getMicroSeconds()));
        popJsonNode();
    }
    else
    {
        value = defaultValue;
    }
}


bool JsonDecoder::convertMemberToElement(const char8_t *memberName, char8_t* buf, const size_t buflen) const
{
    if (memberName == nullptr || *memberName == '\0' || buf == nullptr || buflen == 0)
    {
        return false;       // Bad parameters
    }

    // Now copy the member into our new storage and make it lowercase as well.
    uint32_t len = 0;
    for ( ; len < buflen-1 ; ++len)
    {
        buf[len] = static_cast<char8_t>(tolower(memberName[len]));
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

bool JsonDecoder::pushJsonNode(const EA::TDF::Tdf &parentTdf, uint32_t tag)
{
    if (mStateStack[mStateDepth].state == STATE_UNION)
    {
        return true;
    }

    if (mStateStack[mStateDepth].state == STATE_VARIABLE)
    {
        JsonDomObjectValue* objectValue = getJsonDomObjectValue("value", false);
        mDomNodeStack.push_back(objectValue->mpNode);
        return true;
    }

    if (mStateStack[mStateDepth].state == STATE_MAP)
    {
        //  trying to read more elements than available - fail
        if (mStateStack[mStateDepth].dimensionIndex == mStateStack[mStateDepth].dimensionSize)
        {
            ++mErrorCount;
            return false;
        }

        //get the current node of this map by index
        BLAZE_EASTL_STRING& index = mStateStack[mStateDepth].mapKeyList[mStateStack[mStateDepth].dimensionIndex];
        JsonDomObjectValue* objectValue = getJsonDomObjectValue(index.c_str(), false);
        if (objectValue != nullptr)
        {
            mDomNodeStack.push_back(objectValue->mpNode);
        }
        else
        {
            // This element is missing in the incoming JSON data. Skip it and will not treat this as an error
            return false;
        }
        return true;
    }

    if (mStateStack[mStateDepth].state == STATE_ARRAY)
    {
        if (mStateStack[mStateDepth].dimensionIndex == mStateStack[mStateDepth].dimensionSize)
        {
            ++mErrorCount;
            return false;
        }

        //get the current node of this array by index
        JsonDomNode* pJsonDomNode = getJsonDomNodeFromArray(mDomNodeStack.back(), mStateStack[mStateDepth].dimensionIndex);
        if (pJsonDomNode != nullptr)
        {
            ++mStateStack[mStateDepth].dimensionIndex;
            mDomNodeStack.push_back(pJsonDomNode);
        }
        return true;
    }

    const char8_t* memberName = nullptr;   
    const EA::TDF::TdfMemberInfo* info = nullptr;
    // JSON decoder supports the member nameoverride if one is defined
    if (!parentTdf.getMemberInfoByTag(tag, info) ||
        info == nullptr ||
        (memberName = info->getMemberName()) == nullptr)
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

    JsonDomObjectValue* objectValue = getJsonDomObjectValue(elementName, false);
    if (objectValue != nullptr)
    {
        mDomNodeStack.push_back(objectValue->mpNode);
    }
    else
    {
        // This element is missing in the incoming JSON data. Skip it and will not treat this as an error
        return false;
    }
    return true;
}

bool JsonDecoder::popJsonNode()
{
    if (mStateStack[mStateDepth].state == STATE_MAP)
    {
        if (mStateStack[mStateDepth].readValue)
        {
            ++mStateStack[mStateDepth].dimensionIndex;
        }
        mStateStack[mStateDepth].readValue = !mStateStack[mStateDepth].readValue;
    }

    if (!mDomNodeStack.empty())
        mDomNodeStack.pop_back();
    return true;
}

void JsonDecoder::pushStack(State state)
{
    ++mStateDepth;
    EA_ASSERT(mStateDepth < MAX_STATE_DEPTH);
    mStateStack[mStateDepth].state = state;
    mStateStack[mStateDepth].dimensionSize = 0;
    mStateStack[mStateDepth].dimensionIndex = 0;
    mStateStack[mStateDepth].readValue = false;
    mStateStack[mStateDepth].mapKeyList.clear();
}

bool JsonDecoder::popStack()
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

/*! ************************************************************************************************/
/*! \brief Instructs the decoder that the buffer to be decoded only contains a single subfield,
            and thus, only it should be decoded
    \param[in] tagArray - an Array of member tags corresponding to the tdf hierarchy pointing at
                            the tdf member to decode
    \param[in] count - The number of items in the tag array
***************************************************************************************************/
void JsonDecoder::setSubField(const uint32_t* tagArray, size_t count)
{
    if (tagArray != nullptr)
    {
        mFilterTags.clear();
        for (size_t i = 0; i < count; ++i)
        {
            mFilterTags.push_back(tagArray[i]);
        }

        mFilterIndex = 0;
        mEnabled = false;
    }
}

void JsonDecoder::setSubField(uint32_t tag)
{
    mFilterTags.clear();
    mFilterTags.push_back(tag);
    mFilterIndex = 0;
    mEnabled = false;
}

/*! ************************************************************************************************/
/*! \brief Clears the subfield filter specified in setSubField
***************************************************************************************************/
void JsonDecoder::clearSubField()
{
    mFilterTags.clear();
    mEnabled = true;
    mFilterIndex = 0;
}

bool JsonDecoder::EnumKeyCompare::operator()(const BLAZE_EASTL_STRING& a, const BLAZE_EASTL_STRING& b)
{
    int32_t intValueA = -1, intValueB = -1;

    if (mEnumMap == nullptr || !mEnumMap->findByName(a.c_str(), intValueA) || !mEnumMap->findByName(b.c_str(), intValueB))
        return false;

    return (intValueA < intValueB);
}

int64_t JsonDecoder::getJsonDomInteger(JsonDomNode* node, int64_t defaultValue)
{
    if (node->GetNodeType() == kETInteger)
    {
        return node->AsJsonDomInteger()->mValue;
    }

    return defaultValue;
}

double JsonDecoder::getJsonDomDouble(JsonDomNode* node, double defaultValue)
{
    if (node->GetNodeType() == kETDouble)
    {
        return node->AsJsonDomDouble()->mValue;
    }

    return defaultValue;
}

bool JsonDecoder::getJsonDomBool(JsonDomNode* node, bool defaultValue)
{
    if (node->GetNodeType() == kETBool)
    {
        return node->AsJsonDomBool()->mValue;
    }

    return defaultValue;
}

const char8_t* JsonDecoder::getJsonDomString(JsonDomNode* node)
{
    if (node->GetNodeType() == kETString)
    {
        return node->AsJsonDomString()->mValue.c_str();
    }

    return nullptr;
}

size_t JsonDecoder::getJsonDomNodeArraySize(JsonDomNode* node)
{
    if ((node->GetNodeType() == kETBeginArray) || (node->GetNodeType() == kETBeginDocument))
    {
        return node->AsJsonDomArray()->mJsonDomNodeArray.size();
    }

    return 0;
}

JsonDomNode* JsonDecoder::getJsonDomNodeFromArray(JsonDomNode* node, size_t index)
{
    size_t arraySize = getJsonDomNodeArraySize(node);

    if (arraySize > 0 && index < arraySize)
    {
        return node->AsJsonDomArray()->mJsonDomNodeArray.at((JsonDomNodeArray::size_type)index);
    }

    return nullptr;
}

JsonDomObjectValue* JsonDecoder::getFirstJsonDomObjectValue()
{
    if (mDomNodeStack.back() != nullptr && mDomNodeStack.back()->GetNodeType() == kETBeginObject)
    {
        if (!mDomNodeStack.back()->AsJsonDomObject()->mJsonDomObjectValueArray.empty())
            return &(mDomNodeStack.back()->AsJsonDomObject()->mJsonDomObjectValueArray.at(0));
    }

    return nullptr;
}

JsonDomObjectValue* JsonDecoder::getJsonDomObjectValue(const char8_t* pName, bool bCaseSensitive)
{
    if (mDomNodeStack.back() != nullptr && mDomNodeStack.back()->GetNodeType() == kETBeginObject)
    {
        JsonDomObjectValueArray::iterator ovIter = mDomNodeStack.back()->AsJsonDomObject()->GetNodeIterator(pName, bCaseSensitive);
        if (ovIter != invalidObjectIterator())
            return ovIter;
    }

    return nullptr;
}

JsonDomObjectValueArray::iterator JsonDecoder::invalidObjectIterator()
{
    return mDomNodeStack.back()->AsJsonDomObject()->mJsonDomObjectValueArray.end();
}

} // Blaze
#else
#include "framework/blaze.h"
#include "framework/protocol/shared/jsondecoder.h"
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
    \brief JsonDecoder

    This class provides a mechanism to decode JSON into a TDF object.

    \param[in]  buffer - the buffer to encode into

*/
/*************************************************************************************************/
#ifdef BLAZE_CLIENT_SDK
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    Blaze::BlazeError JsonDecoder::decode(RawBuffer& buffer, EA::TDF::Tdf& tdf, bool onlyDecodeChanged)
#else
    Blaze::BlazeError JsonDecoder::decode(RawBuffer& buffer, EA::TDF::Tdf& tdf)
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
#else
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    BlazeRpcError JsonDecoder::decode(RawBuffer& buffer, EA::TDF::Tdf& tdf, bool onlyDecodeChanged)
#else
    BlazeRpcError JsonDecoder::decode(RawBuffer& buffer, EA::TDF::Tdf& tdf)
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
#endif //BLAZE_CLIENT_SDK
    {
        RawBufferIStream istream(buffer);
        EA::TDF::MemberVisitOptions visitOpt;
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        visitOpt.onlyIfSet = onlyDecodeChanged;
#endif

        if (mSubfieldTag != 0)
        {
            visitOpt.subFieldTag = mSubfieldTag;
        }
        
        bool result = mDecoder.decode(istream, tdf, visitOpt);
        if (result)
            return ERR_OK;

#ifdef BLAZE_CLIENT_SDK
        BLAZE_SDK_DEBUGF("[JsonDecoder].decode: Failed to decode tdf: %s\n", mDecoder.getErrorMessage());
#else
        BLAZE_TRACE_LOG(Log::SYSTEM, "[JsonDecoder].decode: Failed to decode tdf: " << mDecoder.getErrorMessage());
#endif
        
        if (blaze_stricmp("ERR_INVALID_TDF_ENUM_VALUE", mDecoder.getErrorMessage()) == 0)
            return ERR_INVALID_TDF_ENUM_VALUE;

        return ERR_SYSTEM;
    }

} // Blaze
#endif

