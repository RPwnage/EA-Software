/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class JsonEncoder

    This class provides a mechanism to encode data using the XML encoding format, for example:

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


    In the above, the elements like "component", "classname", "arrayname", etc, are not literal.
    They are replaced with the name of the component, class, array, map, etc.  See Blaze documentation 
    on Confluence for details.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

/*** Include Files *******************************************************************************/
#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/blazesdk.h"
#endif
#ifdef ENABLE_LEGACY_CODECS
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "framework/util/shared/base64.h"
#include "framework/util/shared/bytearrayinputstream.h"
#include "framework/util/shared/bytearrayoutputstream.h"
#include "framework/protocol/shared/jsonencoder.h"
#include "framework/protocol/shared/heat2util.h"

//SDK does not use the standard "EA_ASSERT" so we redefine it here as "BlazeAssert"
#if !defined(EA_ASSERT) && defined(BlazeAssert)
#define EA_ASSERT(X) BlazeAssert(X)
#endif

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief JsonEncoder

    Construct a new XML encoder that encodes into the given buffer

    \param[in]  buffer - the buffer to encode into

*/
/*************************************************************************************************/
JsonEncoder::JsonEncoder()
    : mStateDepth(0),
#ifdef BLAZE_CLIENT_SDK
      mFilterTags(MEM_GROUP_FRAMEWORK, "JsonEncoder::mFilterTags"),
#endif
      mFilterIndex(0),
      mEnabled(true),
      mIndentSpacing(4),
      mLineEnd('\n')
{
    // pre: ok to memset zero out all StateStruct member types (see StateStruct)
    memset(&mStateStack[0], 0, sizeof(StateStruct) * MAX_STATE_DEPTH);
}

/*************************************************************************************************/
/*!
    \brief ~JsonEncoder

    Destructor for JsonEncoder class.
*/
/*************************************************************************************************/
JsonEncoder::~JsonEncoder()
{
//    deallocateXmlAttributeResponseStorage();
}

/*************************************************************************************************/
/*!
    \brief setBuffer

    Set the raw buffer that the XML response will be encoded into

    \param[in]  buffer - the buffer to decode from
*/
/*************************************************************************************************/
void JsonEncoder::setBuffer(RawBuffer* buffer)
{
    mBuffer = buffer;

    // Important to set state depth to 0, as nested structs will result in multiple calls to
    // begin() during encoding, and only the call when state depth is 0 will perform initialization
    mStateDepth = 0;

    mErrorCount = 0;
    mStateStack[0].state = STATE_NORMAL;

    mJsonBuffer.initialize(mBuffer); 
}

bool JsonEncoder::visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue)
{
    // Do top-level only initialization here.
    EA_ASSERT(mStateDepth == 0);

    mStateStack[mStateDepth].mParentTdf = &tdf;
    startDocument();
    if (mFilterTags.empty())
    {
        //only start the object if we are encoding the whole tdf.  Otherwise, don't start it here to allow
        // starting the document with an array.
        mWriter.BeginObject();
    }
    
#ifndef BLAZE_CLIENT_SDK
    if (mFrame->useLegacyJsonEncoding)
    {
        if (!beginElement(tdf.getClassName()))
        {
            return false;
        }
    }
#endif

    tdf.visit(*this, tdf, tdf);

#ifndef BLAZE_CLIENT_SDK
    if (mFrame->useLegacyJsonEncoding)
    {
        endElement();
    }
#endif
    
    if (mFilterTags.empty())
    {
        mWriter.EndObject();
    }

    endDocument();

    if (isOk() == false)
    {
        ++mErrorCount;
    }
    
    return (mErrorCount == 0);
}


bool JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue)
{
    bool result = true;

    bool matchesTag = false;
    if (!mEnabled && (mFilterIndex == (uint32_t)mFilterTags.size() - 1) && (mFilterTags.at(mFilterIndex) == tag))
    {
        //we've found the tdf that we are looking for, so start encoding
        mEnabled = true;
        matchesTag = true;
    }

    if (mEnabled && value.isValid())
    {
        ++mStateDepth;
        EA_ASSERT(mStateDepth < MAX_STATE_DEPTH);

        mStateStack[mStateDepth].state = STATE_VARIABLE;
        mStateStack[mStateDepth].variableTdfId = value.get()->getTdfId();      // Which variable TDF we're encoding
        blaze_strnzcpy(mStateStack[mStateDepth].variableTdfName, value.get()->getFullClassName(), EA::TDF::MAX_TDF_NAME_LEN);

        if(mStateDepth > 0) {
            mStateStack[mStateDepth].mParentTdf = mStateStack[mStateDepth-1].mParentTdf;
        }
        else {
            mStateStack[mStateDepth].mParentTdf = nullptr;
        }

        result = visit(rootTdf, parentTdf, tag, *value.get(), *value.get());

        popStack();

        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            // Our struct is the value part of a map.  Close the map entry and update the map
            // Variable TDF is implemented like a map.
            updateMapState();
        }
    }

    if (matchesTag)
    {
        mEnabled = false;
        mFilterIndex = 0;
    }

    return result;
}

bool JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue)
{
    bool result = true;

    bool matchesTag = false;
    if (!mEnabled && (mFilterIndex == (uint32_t)mFilterTags.size() - 1) && (mFilterTags.at(mFilterIndex) == tag))
    {
        //we've found the tdf that we are looking for, so start encoding
        mEnabled = true;
        matchesTag = true;
    }

    if (mEnabled && value.isValid())
    {
        ++mStateDepth;
        EA_ASSERT(mStateDepth < MAX_STATE_DEPTH);

        mStateStack[mStateDepth].state = STATE_VARIABLE;
        mStateStack[mStateDepth].variableTdfId = value.get().getTdfId();      // Which variable TDF we're encoding
        blaze_strnzcpy(mStateStack[mStateDepth].variableTdfName, value.get().getFullName(), EA::TDF::MAX_TDF_NAME_LEN);

        if(mStateDepth > 0) {
            mStateStack[mStateDepth].mParentTdf = mStateStack[mStateDepth-1].mParentTdf;
        }
        else {
            mStateStack[mStateDepth].mParentTdf = nullptr;
        }

        visitReference(rootTdf, parentTdf, tag, value.getValue(), &referenceValue.getValue());

        popStack();

        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            // Our struct is the value part of a map.  Close the map entry and update the map
            // Variable TDF is implemented like a map.
            updateMapState();
        }
    }

    if (matchesTag)
    {
        mEnabled = false;
        mFilterIndex = 0;
    }

    return result;
}

bool JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue)
{
    bool matchesTag = false;
    char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";
    const char8_t* name = value.getClassName();

    if (mEnabled || ((mFilterTags.size() > mFilterIndex) && (mFilterTags.at(mFilterIndex) == tag)))
    {
        ++mFilterIndex;
        if (!mEnabled && (mFilterIndex == (uint32_t)mFilterTags.size()))
        {
            //we've found the tdf that we are looking for, so start encoding
            mEnabled = true;
            matchesTag = true;
        }
        else
        {
            // Do nested struct initialization here
            if ((mStateDepth > 0) && ((mStateStack[mStateDepth - 1].state == STATE_MAP || mStateStack[mStateDepth - 1].state == STATE_ARRAY) && (mStateStack[mStateDepth].state == STATE_VARIABLE)))
                matchesTag = true;
            
            if ((mStateStack[mStateDepth].state != STATE_ARRAY)
                && (mStateStack[mStateDepth].state != STATE_MAP))
            {
                if (!buildElementName(tag, elementName, sizeof(elementName)))
                    return false;
                name = elementName;
            }
        }

        ++mStateDepth;
        EA_ASSERT(mStateDepth < MAX_STATE_DEPTH);

        mStateStack[mStateDepth].state = STATE_NORMAL;
        mStateStack[mStateDepth].mParentTdf = &value;

        if (mEnabled)
        {
            //write the element name only if we aren't filtering the tdf, or if we found the tdf we are filtering 
            if (!beginElement(name, !matchesTag))
            {
                // Do cleanup
                if (mStateDepth != 0)
                {
                    mFilterIndex = 0;
                    popStack();
                }

                return false;
            }
        }
    
        value.visit(*this, rootTdf, referenceValue);

        if (mEnabled)
        {
            endElement();
        }
    
        popStack();
    
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            // Our struct is the value part of a map.  Close the map entry and update the map        
            updateMapState();
        }
    
        if (isOk() == false)
        {
            ++mErrorCount;
        }

        if (matchesTag)
        {
            mEnabled = false;
            mFilterIndex = 0;
        }

    }

    return (mErrorCount == 0);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue)
{
    bool matchesTag = false;
    if (!mEnabled && (mFilterIndex == (uint32_t)mFilterTags.size() - 1) && (mFilterTags.at(mFilterIndex) == tag))
    {
        //we've found the tdf that we are looking for, so start encoding
        mEnabled = true;
        matchesTag = true;
    }
    
    if (mEnabled)
    {
        // Update the stack and initialize the state for the array
        mStateDepth++;
        EA_ASSERT(mStateDepth < MAX_STATE_DEPTH);

        mStateStack[mStateDepth].state = STATE_ARRAY;
        mStateStack[mStateDepth].mParentTdf = &parentTdf;
        mStateStack[mStateDepth].dimensions = 1;

        mStateStack[mStateDepth].sizes = (uint32_t) value.vectorSize();
        
        char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";

        // Generate the element name to use for each dimension of the array
        // only bother generating it if we are actually going to use it.
        if (!matchesTag)
        {
            if (buildElementName(tag, elementName, sizeof(elementName)) == false)
            {
                return;
            }
            elementName[sizeof(elementName)-1] = '\0';      // Ensure we're nullptr terminated
        }

        // when we are filtering by list, we want to start encoding with '[', so don't include the name        
        if (!beginElement(elementName, !matchesTag))
        {
            // Do cleanup
            if (mStateDepth != 0)
            {
                mFilterIndex = 0;
                popStack();
            }

            return;
        }

        value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);

        EA_ASSERT(mStateStack[mStateDepth].state == STATE_ARRAY);

        endElement();

        if (matchesTag)
        {
            mEnabled = false;
            mFilterIndex = 0;
        }

        popStack();

        if (mStateStack[mStateDepth].state == STATE_MAP)
        {        
            updateMapState();
        }
    }
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue)
{
    bool matchesTag = false;
    if (!mEnabled && (mFilterIndex == (uint32_t)mFilterTags.size() - 1) && (mFilterTags.at(mFilterIndex) == tag))
    {
        //we've found the tdf that we are looking for, so start encoding
        mEnabled = true;
        matchesTag = true;
    }
    
    if (mEnabled)
    {
        // Generate the element name to use for the map
        char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";

        if (!matchesTag && buildElementName(tag, elementName, sizeof(elementName)) == false)
        {
            return;
        }

        mStateDepth++;
        EA_ASSERT(mStateDepth < MAX_STATE_DEPTH);

        mStateStack[mStateDepth].state = STATE_MAP;
        mStateStack[mStateDepth].totalNumMapElements = static_cast<uint32_t>(value.mapSize());    // So we know how many elements to expect
        mStateStack[mStateDepth].currentNumMapElements = 0;                             // So far, no encoded elements
        mStateStack[mStateDepth].nextIsMapKey = true;                                   // Next element encoded will be the key
        mStateStack[mStateDepth].mParentTdf = mStateStack[mStateDepth-1].mParentTdf;

        if (!beginElement(elementName, !matchesTag))
        {
            // Do cleanup
            if (mStateDepth != 0)
                popStack();

            return;
        }

        value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);

        if (mStateStack[mStateDepth].state != STATE_MAP ||
            mStateStack[mStateDepth].currentNumMapElements != mStateStack[mStateDepth].totalNumMapElements ||
            mStateStack[mStateDepth].nextIsMapKey == false)
        {
            ++mErrorCount;
            return;
        }

        endElement();

        // Update the stack
        popStack();

        //need to update the state when handling collections of collections.  This needs to happen after we popStack
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {        
            updateMapState();
        }
    }

    if (matchesTag)
    {
        mEnabled = false;
        mFilterIndex = 0;
    }
}

bool JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue)
{
    if (mEnabled)
    {
        EA_ASSERT(mStateStack[mStateDepth].state != STATE_UNION);

        State parentState = mStateStack[mStateDepth].state;

        char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";
        const char8_t* name = value.getClassName();

        if ((mStateStack[mStateDepth].state != STATE_ARRAY)
            && (mStateStack[mStateDepth].state != STATE_MAP))
        {
            if (!buildElementName(tag, elementName, sizeof(elementName)))
                return false;
            name = elementName;
        }

        mStateDepth++;
        EA_ASSERT(mStateDepth < MAX_STATE_DEPTH);

        mStateStack[mStateDepth].state = STATE_UNION;
        mStateStack[mStateDepth].activeMember = value.getActiveMemberIndex();       // Which element in the union we're encoding
        mStateStack[mStateDepth].mParentTdf = &value;

        if (parentState==STATE_NORMAL || parentState==STATE_ARRAY || parentState==STATE_MAP)
        {
            if (!beginElement(name))
            {
                // Do cleanup
                if (mStateDepth != 0)
                    popStack();

                return false;
            }
        }

        value.visit(*this, rootTdf, value);

        EA_ASSERT(mStateStack[mStateDepth].state == STATE_UNION);

        // Close off open union
        endElement();

        popStack();

        //need to update the state when handling collections of collections.  This needs to happen after we popStack
        if (mStateStack[mStateDepth].state == STATE_MAP)
        {
            updateMapState();
        }
    }
    return isOk();
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        writePrimitive(tag);
        mWriter.Bool(value);
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        if (mStateStack[mStateDepth].state == STATE_MAP && mStateStack[mStateDepth].nextIsMapKey)
        {
            blaze_snzprintf(mChars, sizeof(mChars), "%c", value);
            writePrimitiveMapKey(mChars);
            mStateStack[mStateDepth].nextIsMapKey = false;
        }
        else
        {
            // This is a bit of a work around.  We can't fully support JSON null's due to our
            // inability to set objects to nullptr in TDF infrastructure.  When required, you
            // can fake it by using a new TDF, which uses this character override, instead of an object.
            // Special case the null character to write out a JSON null.
            if (value == '\0')
            {
                writePrimitive(tag);
                mWriter.Null();
                mStateStack[mStateDepth].nextIsMapKey = true;
            }
            else
            {
                writePrimitive(tag);
                mWriter.Integer(static_cast<int64_t>(value));
                mStateStack[mStateDepth].nextIsMapKey = true;
            }

        }
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        if (mStateStack[mStateDepth].state == STATE_MAP && mStateStack[mStateDepth].nextIsMapKey)
        {
            blaze_snzprintf(mChars, sizeof(mChars), "%d", value);
            writePrimitiveMapKey(mChars);
            mStateStack[mStateDepth].nextIsMapKey = false;
        }
        else
        {
            writePrimitive(tag);
            mWriter.Integer(static_cast<int64_t>(value));
            mStateStack[mStateDepth].nextIsMapKey = true;
        }
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        if (mStateStack[mStateDepth].state == STATE_MAP && mStateStack[mStateDepth].nextIsMapKey)
        {
            blaze_snzprintf(mChars, sizeof(mChars), "%u", value);
            writePrimitiveMapKey(mChars);
            mStateStack[mStateDepth].nextIsMapKey = false;
        }
        else
        {
            writePrimitive(tag);
            mWriter.Integer(static_cast<int64_t>(value));
            mStateStack[mStateDepth].nextIsMapKey = true;
        }
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        if (mStateStack[mStateDepth].state == STATE_MAP && mStateStack[mStateDepth].nextIsMapKey)
        {
            blaze_snzprintf(mChars, sizeof(mChars), "%d", value);
            writePrimitiveMapKey(mChars);
            mStateStack[mStateDepth].nextIsMapKey = false;
        }
        else
        {
            writePrimitive(tag);
            mWriter.Integer(static_cast<int64_t>(value));
            mStateStack[mStateDepth].nextIsMapKey = true;
        }
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        if (mStateStack[mStateDepth].state == STATE_MAP && mStateStack[mStateDepth].nextIsMapKey)
        {
            blaze_snzprintf(mChars, sizeof(mChars), "%u", value);
            writePrimitiveMapKey(mChars);
            mStateStack[mStateDepth].nextIsMapKey = false;
        }
        else
        {
            writePrimitive(tag);
            mWriter.Integer(static_cast<int64_t>(value));
            mStateStack[mStateDepth].nextIsMapKey = true;
        }
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        if (mStateStack[mStateDepth].state == STATE_MAP && mStateStack[mStateDepth].nextIsMapKey)
        {
            blaze_snzprintf(mChars, sizeof(mChars), "%d", value);
            writePrimitiveMapKey(mChars);
            mStateStack[mStateDepth].nextIsMapKey = false;
        }
        else
        {
            writePrimitive(tag);
            mWriter.Integer(static_cast<int64_t>(value));
            mStateStack[mStateDepth].nextIsMapKey = true;
        }
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        uint32_t bits = value.getBits();
        visit(rootTdf, parentTdf, tag, bits, bits, 0);
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        if (mStateStack[mStateDepth].state == STATE_MAP && mStateStack[mStateDepth].nextIsMapKey)
        {
            blaze_snzprintf(mChars, sizeof(mChars), "%u", value);
            writePrimitiveMapKey(mChars);
            mStateStack[mStateDepth].nextIsMapKey = false;
        }
        else
        {
            writePrimitive(tag);
            mWriter.Integer(static_cast<int64_t>(value));
            mStateStack[mStateDepth].nextIsMapKey = true;
        }
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        if (mStateStack[mStateDepth].state == STATE_MAP && mStateStack[mStateDepth].nextIsMapKey)
        {
            blaze_snzprintf(mChars, sizeof(mChars), "%" PRIi64, value);
            writePrimitiveMapKey(mChars);
            mStateStack[mStateDepth].nextIsMapKey = false;
        }
        else
        {
            writePrimitive(tag);
            mWriter.Integer(static_cast<int64_t>(value));
            mStateStack[mStateDepth].nextIsMapKey = true;
        }
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        if (mStateStack[mStateDepth].state == STATE_MAP && mStateStack[mStateDepth].nextIsMapKey)
        {
            blaze_snzprintf(mChars, sizeof(mChars), "%" PRIu64, value);
            writePrimitiveMapKey(mChars);
            mStateStack[mStateDepth].nextIsMapKey = false;
        }
        else
        {
            writePrimitive(tag);
            mWriter.Integer(static_cast<int64_t>(value));
            mStateStack[mStateDepth].nextIsMapKey = true;
        }
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        writePrimitive(tag);
        mWriter.Double(static_cast<double>(value));
    }

    clearFilterForPrimitive(matchesTag);
}


void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        if (mStateStack[mStateDepth].state == STATE_MAP && mStateStack[mStateDepth].nextIsMapKey)
        {
            writePrimitiveMapKey(value.c_str());
            mStateStack[mStateDepth].nextIsMapKey = false;
        }
        else
        {
            writePrimitive(tag);
            mWriter.String(value.c_str(), value.length());
            mStateStack[mStateDepth].nextIsMapKey = true;
        }
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue)
{ 
    bool matchesTag = false;
    if (!mEnabled && (mFilterIndex == (uint32_t)mFilterTags.size() - 1) && (mFilterTags.at(mFilterIndex) == tag))
    {
        //we've found the item that we are looking for, so start encoding
        mEnabled = true;
        matchesTag = true;
    }

    if (mEnabled)
    {
        uint32_t valueCount = value.getCount();
        uint32_t encodedSize = ((valueCount % 3) == 0) ? ((valueCount/3) * 4) : (((valueCount + (3 - valueCount % 3)) / 3) * 4);
        writePrimitive(tag);
        mWriter.BeginObject();
    
        int32_t numberOfwhite = 0;
        uint8_t* ptr = mJsonBuffer.getBuffer()->tail() -2; 
        while (*ptr ==' ')
        {
            numberOfwhite++;
            ptr--;
        }
    
        RawBuffer buffer(numberOfwhite);
        uint8_t *tmpBuf = buffer.acquire(numberOfwhite);
        memcpy(tmpBuf, ptr + 1, numberOfwhite);

        mWriter.BeginObjectValue("count", strlen("count"));
        mWriter.Integer(static_cast<int64_t>(encodedSize));
        mWriter.BeginObjectValue("enc", strlen("enc"));
        mWriter.String("base64", strlen("base64"));
        mWriter.BeginObjectValue("data", strlen("data"));
        // Encode the data to the JsonBuffer
        mJsonBuffer.write('"');
        ByteArrayInputStream input(value.getData(), valueCount);
        Base64::encode(&input, &mJsonBuffer);
        mJsonBuffer.write('"');
        mJsonBuffer.write('\n');
        mJsonBuffer.Write(tmpBuf, numberOfwhite);
        mWriter.EndObject();
    }

    if (matchesTag)
    {
        mEnabled = false;
        mFilterIndex = 0;
    }
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        const char8_t *identifier = nullptr;
   
        if (mStateStack[mStateDepth].state == STATE_MAP && mStateStack[mStateDepth].nextIsMapKey)
        {
            if (EA_LIKELY((enumMap != nullptr) && enumMap->findByValue(value, &identifier)))
                writePrimitiveMapKey(identifier);
            else
                writePrimitiveMapKey("UNKNOWN");
            mStateStack[mStateDepth].nextIsMapKey = false;
        }
        else
        {
            writePrimitive(tag);
            if (EA_LIKELY((enumMap != nullptr) && enumMap->findByValue(value, &identifier)))
                mWriter.String(identifier, strlen(identifier));
            else
                mWriter.String("UNKNOWN", strlen("UNKNOWN"));
            mStateStack[mStateDepth].nextIsMapKey = true;
        }
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        writePrimitive(tag);
        mWriter.BeginObject();
        mWriter.BeginObjectValue("component", strlen("component"));
        mWriter.Integer(static_cast<int64_t>(value.component));
        mWriter.BeginObjectValue("type", strlen("type"));
        mWriter.Integer(static_cast<int64_t>(value.type));
        mWriter.EndObject();
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        writePrimitive(tag);
        mWriter.BeginObject();
        mWriter.BeginObjectValue("type", strlen("type"));
        mWriter.BeginObject();
        mWriter.BeginObjectValue("component", strlen("component"));
        mWriter.Integer(static_cast<int64_t>(value.type.component));
        mWriter.BeginObjectValue("type", strlen("type"));
        mWriter.Integer(static_cast<int64_t>(value.type.type));
        mWriter.EndObject();
        mWriter.BeginObjectValue("id", strlen("id"));
        mWriter.Integer(static_cast<int64_t>(value.id));
        mWriter.EndObject();
    }

    clearFilterForPrimitive(matchesTag);
}

void JsonEncoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue &referenceValue, const TimeValue defaultValue)
{
    bool matchesTag = matchFilterForPrimitive(tag);

    if (mEnabled)
    {
        writePrimitive(tag);
        mWriter.Integer(static_cast<int64_t>(value.getMicroSeconds()));
    }

    clearFilterForPrimitive(matchesTag);
}

/*** Private Methods *****************************************************************************/


/*************************************************************************************************/
/*!
    \brief beginElement

    Called by any member that needs to begin an XML element.  Takes care of constructing the
    element name and does any special handling for specific element types.

    \param[in] objectName - Optional name of object being encoded
    \param[in] includeName - True if the object name should be encoded

    \return - true if successful, false if any error occurred
*/
/*************************************************************************************************/
bool JsonEncoder::beginElement(const char8_t* objectName /*= nullptr*/, bool includeName /*= true*/)
{
    static const char8_t objectNameBackup[] = "tdf";    // What to use if objectName is nullptr

    if (objectName == nullptr)
    {
        // Use the backup name since we weren't provided one
        objectName = objectNameBackup;
    }

    if (*objectName == '\0' && includeName)
    {
        // This must be non-empty.
        return false;
    }

    char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";
    // Convert the objectName to lowercase, copying for us as well.
    uint32_t len = 0;                           // Will hold string length after loop finishes
    for ( ; len < sizeof(elementName)-1 ; ++len)
    {
        if (len == 0)
            elementName[len] =  static_cast<char8_t>(tolower(objectName[len]));
        else
            elementName[len] = objectName[len];

        if ( elementName[len] == '\0' )
        {
            break;      // All done
        }
    }
    elementName[sizeof(elementName)-1] = '\0';  // Ensure long element names are truncated

    // The element name probably ends in "response" (i.e. DummyResponse).  We don't want
    // the response part to be encoded in the XML.
    if ( len > 8 )              // 8 characters in "response"
    {
        char8_t* response = &(elementName[len-8]);
        if ( strcmp(response, "response") == 0 )
        {
            *response = '\0';   // Remove the response
        }
    }

    if (includeName && (mStateDepth == 0 || (mStateDepth - 1 >= 0 && mStateStack[mStateDepth - 1].state != STATE_ARRAY && mStateStack[mStateDepth - 1].state != STATE_MAP)))
    {
        mWriter.BeginObjectValue(elementName, strlen(elementName));
    }

    if (mStateStack[mStateDepth].state == STATE_ARRAY)
    {
        mWriter.BeginArray();
    }
    else 
    {
        mWriter.BeginObject();

        if (mStateDepth > 0 && mStateStack[mStateDepth-1].state == STATE_VARIABLE)
        {
            mWriter.BeginObjectValue("tdfid", strlen("tdfid"));
            mWriter.Integer(static_cast<int64_t>(mStateStack[mStateDepth-1].variableTdfId));
            mWriter.BeginObjectValue("tdfclass", strlen("tdfclass"));
            mWriter.String(mStateStack[mStateDepth-1].variableTdfName, strlen(mStateStack[mStateDepth-1].variableTdfName));
            mWriter.BeginObjectValue("value", strlen("value"));

            mWriter.BeginObject();

        }
    }    
    return (isOk());
}

/*************************************************************************************************/
/*!
    \brief endElement

    Called by any member that needs to end the current XML element.  Mainly called by endStruct()
    to close off any open elements in the struct or document.

    \return - the number of errors that occurred during processing
*/
/*************************************************************************************************/
bool JsonEncoder::endElement()
{
    switch (mStateStack[mStateDepth].state)
    {
    case STATE_ARRAY:         
        mWriter.EndArray();
        break;
    case STATE_MAP:
    case STATE_NORMAL:
    case STATE_UNION:
    case STATE_VARIABLE:
    default:
        if (mStateDepth > 0 && mStateStack[mStateDepth-1].state == STATE_VARIABLE)
            mWriter.EndObject();

        mWriter.EndObject();
    }

    return true;
}


/*************************************************************************************************/
/*!
    \brief buildElementName

    Called by any member that needs to build an element name out of a member name (or, if 
    a member name is not available a tag name)

    \param[in] tag - tag name of this element to use in case the member name is not available
    \param[in] buf - buffer to output the element name into
    \param[in] buflen - Length of buf

    \return - true if successful, false if any error occurred
*/
/*************************************************************************************************/
bool JsonEncoder::buildElementName(const uint32_t tag, char8_t* buf, const size_t buflen)
{
    const char8_t* memberName = nullptr;
    const EA::TDF::TdfMemberInfo* info = nullptr;
    if (mStateStack[mStateDepth].state == STATE_UNION)
    {
        // union members share the sam tag, grab member info by active member index
        mStateStack[mStateDepth].mParentTdf->getMemberInfoByIndex(mStateStack[mStateDepth].activeMember, info);
    }
    else
    {
        mStateStack[mStateDepth].mParentTdf->getMemberInfoByTag(tag, info);
    }

    // JSON encoder supports use of tdf member nameoverride if defined
    if ((info == nullptr) ||
        (memberName = info->getMemberName()) == nullptr)
    {
        return false;
    }

    if (convertMemberToElement(memberName, buf, buflen) == false)
    {
        return false;
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief convertMemberToElement

    Utility method to take a member name and convert it into an element name by doing the 
    following:  convert everything to lowercase

    \param[in] memberName - Unmodified member name
    \param[in] buf - Where to write the modified member
    \param[in] buflen - Length in bytes of buf

    \return - true if conversion fits in the provided buffer, false if there isn't enough room or parameters are invalid
*/
/*************************************************************************************************/
bool JsonEncoder::convertMemberToElement(const char8_t *memberName, char8_t* buf, const size_t buflen) const
{
    if (memberName == nullptr || *memberName == '\0' || buf == nullptr || buflen == 0)
    {
        return false;       // Bad parameters
    }
    
    // Now copy the member into our new storage and make it lowercase as well.
    uint32_t len = 0;
    for ( ; len < buflen-1 ; ++len)
    {
        if (len == 0)
            buf[len] = static_cast<char8_t>(tolower(memberName[len]));
        else
            buf[len] = memberName[len];

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

/*************************************************************************************************/
/*!
    \brief writePrimitive

    Called by any of the type-specific method to encode primitive values (ints or strings).

    \param[in] value - the tag of the value being encoded
    \param[in] len - the value being encoded as a human readable string

    \return - true if successful, false if any error occurred
*/
/*************************************************************************************************/
bool JsonEncoder::writePrimitiveMapKey(const char8_t* value)
{ 
    // We're getting the key.  Save it so the next run through just has to spit 
    // out the data associated with the key.
    blaze_strnzcpy(mStateStack[mStateDepth].mapKey, value, sizeof(mStateStack[mStateDepth].mapKey));// Truncate really long strings

    // Map is open and we have the key.  Open an "entry" element with the key as the
    // attribute and the data as the XML data.

    mWriter.BeginObjectValue(value, strlen(value));    
    return (isOk());

}

bool JsonEncoder::writePrimitive(uint32_t tag)
{

    char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";

    if (buildElementName(tag, elementName, sizeof(elementName)) == false)
    {
        return false;
    }

    if (mStateStack[mStateDepth].state == STATE_NORMAL || mStateStack[mStateDepth].state == STATE_UNION)
        mWriter.BeginObjectValue(elementName, strlen(elementName));


    // Now update states for those types that require special handling.
    if (mStateStack[mStateDepth].state == STATE_MAP)
    {
        updateMapState();
    }

    return (isOk());
}

/*************************************************************************************************/
/*!
    \brief updateMapState

    Updates the state information for a map after an element has been added in writePrimitive
*/
/*************************************************************************************************/
void JsonEncoder::updateMapState()
{
    if (mStateStack[mStateDepth].state != STATE_MAP)
    {
        ++mErrorCount;
        return;
    }

    ++(mStateStack[mStateDepth].currentNumMapElements);     // We added an element
    if(mStateStack[mStateDepth].currentNumMapElements > mStateStack[mStateDepth].totalNumMapElements)
    {
        ++mErrorCount;
        return;
    }
    mStateStack[mStateDepth].mapKey[0] = '\0';              // Reset the map key, it's been used!
    mStateStack[mStateDepth].nextIsMapKey = true;
}

/*************************************************************************************************/
/*!
    \brief isOk

    Utility method to check if the previous operation was successful.  Since all we're doing is
    writing XML data to a buffer, success is defined as there still being room in the buffer
    to write more data (it is assumed that this method will be called in all cases _except_ the
    last write to the buffer - that being the case even if the most recent write did not overflow
    the buffer it is still a failure if the buffer is just full because we can safely assume
    that more data needs to be written).

    \return - true if the buffer has room remaining, false otherwise
*/
/*************************************************************************************************/
bool JsonEncoder::isOk()
{
    if (mBuffer->tailroom() == 0)
    {
        ++mErrorCount;
        return false;
    }
    return true;
}

/*************************************************************************************************/
/*!
    \brief popStack

    Updates the state stack and sets whether we are back at the top level

    \return - true if state depth updated, false if we're already at the top level
*/
/*************************************************************************************************/
bool JsonEncoder::popStack()
{
    if (mStateDepth > 0)
    {
        --mStateDepth;

        return true;
    }
    else
    {
        return false;
    }
}

void JsonEncoder::startDocument()
{
    mWriter.Reset();
    mWriter.SetStream(&mJsonBuffer);
    mWriter.SetOption(JsonWriter::kOptionIndentSpacing, mIndentSpacing);  // Enables the writing of indented output. 0 would mean no indentation.
    mWriter.SetOption(JsonWriter::kOptionLineEnd, mLineEnd);     // Uses \n for newlines. 0 would mean no newlines.
    mWriter.BeginDocument();
}

void JsonEncoder::endDocument()
{
    mWriter.EndDocument();
}

/*! ************************************************************************************************/
/*! \brief Instructs the decoder that the buffer to be decoded only contains a single subfield,
            and thus, only it should be decoded
    \param[in] tagArray - an Array of member tags corresponding to the tdf hierarchy pointing at
                            the tdf member to decode
    \param[in] count - The number of items in the tag array
***************************************************************************************************/
void JsonEncoder::setSubField(const uint32_t* tagArray, size_t count)
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

void JsonEncoder::setSubField(uint32_t tag)
{
    mFilterTags.clear();
    mFilterTags.push_back(tag);
    mFilterIndex = 0;
    mEnabled = false;
}

/*! ************************************************************************************************/
/*! \brief Clears the subfield filter specified in setSubField
***************************************************************************************************/
void JsonEncoder::clearSubField()
{
    mFilterTags.clear();
    mEnabled = true;
    mFilterIndex = 0;
}

/*! ************************************************************************************************/
/*! \brief Enables decoding if the specified member tag matches the filter specified in setSubField
***************************************************************************************************/
bool JsonEncoder::matchFilterForPrimitive(uint32_t tag)
{
    if (!mEnabled && (mFilterIndex == (uint32_t)mFilterTags.size() - 1) && (mFilterTags.at(mFilterIndex) == tag))
    {
        //we've found the item that we are looking for, so start encoding
        mEnabled = true;
        beginElement(nullptr, false);
        return true;
    }
    return false;
}

/*! ************************************************************************************************/
/*! \brief closes the element and disables encoding
***************************************************************************************************/
void JsonEncoder::clearFilterForPrimitive(bool isMatch)
{
    if (isMatch)
    {
        endElement();
        mEnabled = false;
        mFilterIndex = 0;
    }
}

} // Blaze

#else // ENABLE_LEGACY_CODECS

#include "framework/blaze.h"
#include "framework/protocol/shared/jsonencoder.h"
#include "EAIO/EAStream.h"
#include "framework/util/shared/rawbufferistream.h"
#include "EATDF/tdfbasetypes.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief Json2Encoder

    Construct a new XML encoder that encodes into the given buffer

    \param[in]  buffer - the buffer to encode into

*/
/*************************************************************************************************/
    bool JsonEncoder::encode(RawBuffer& buffer, const EA::TDF::Tdf& tdf
#ifndef BLAZE_CLIENT_SDK
        , const RpcProtocol::Frame* frame
#endif
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        , bool onlyEncodeChanged
#endif
        )
    {
        EA::TDF::MemberVisitOptions visitOpt;
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        visitOpt.onlyIfSet = onlyEncodeChanged;
#endif
        if (mSubfieldTag != 0)
        {
            visitOpt.subFieldTag = mSubfieldTag;
        }

        visitOpt.onlyIfNotDefault = mDefaultDiff;
        
        mEncoder.setIncludeTDFMetaData(mEncodeVariableGenericTdfInfo);

        RawBufferIStream istream(buffer);
        return mEncoder.encode(istream, tdf, nullptr, visitOpt);
    }

    void JsonEncoder::setSubField(uint32_t tag) 
    { 
        mSubfieldTag = tag; 
    }
    
    void JsonEncoder::clearSubField() 
    { 
        mSubfieldTag = 0;
    }
} // Blaze

#endif // ENABLE_LEGACY_CODECS
