/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Xml2Encoder

    This class provides a mechanism to encode data using the XML encoding format, for example:

    <?xml version="1.0" encoding="UTF8"?>
    <component>
        <classname>
            <id>100</id>
            <msg>Invalid username.</msg>
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
            <unionname member="1">
                <valu>
                    <unionmember1>54</unionmember1>
                </valu>
            </unionname>
            <unionname2>
                <unionmember1>474</unionmember1>
            </unionname2>
        </classname>
    </component>

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
#include "framework/util/shared/xmlbuffer.h"
#include "framework/util/shared/base64.h"
#include "framework/util/shared/bytearrayinputstream.h"
#include "framework/util/shared/bytearrayoutputstream.h"
#include "framework/protocol/shared/xml2encoder.h"
#include "framework/protocol/shared/heat2util.h"

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

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief Xml2Encoder

    Construct a new XML encoder that encodes into the given buffer

    \param[in]  buffer - the buffer to encode into

*/
/*************************************************************************************************/
Xml2Encoder::Xml2Encoder()
    : mXmlBuffer(true),
      mStateDepth(0),
#ifdef BLAZE_CLIENT_SDK
      mXmlElements(MEM_GROUP_FRAMEWORK, "Xml2Encoder::mXmlElements"),
#endif
      mUseAttributes(false)
{
}

/*************************************************************************************************/
/*!
    \brief setBuffer

    Set the raw buffer that the XML response will be encoded into

    \param[in]  buffer - the buffer to decode from
*/
/*************************************************************************************************/
void Xml2Encoder::setBuffer(RawBuffer* buffer)
{
    mBuffer = buffer;

    mStateDepth = 0;
    mStateStack[0].state = STATE_NORMAL;

    // The false is important!  It means empty elements will not be printed by default,
    // we take care of enforcing the presence of empty list elements on the fly
    mXmlBuffer.initialize(mBuffer, 4, false);
}

bool Xml2Encoder::visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue)
{
    // Do top-level only initialization here.
#ifndef BLAZE_CLIENT_SDK
    mUseAttributes = (mFrame->xmlResponseFormat == RpcProtocol::XML_RESPONSE_FORMAT_ATTRIBUTES);
#endif
    startDocument();

    beginElement(tdf.getClassName());

    tdf.visit(*this, tdf, tdf);

    endElement();

    if (mUseAttributes)
    {
        EA_ASSERT(mPendingElements.empty() == true);
        writeElementNode(&mXmlElements.front());
    }

    endDocument();

    return true;
}

bool Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue)
{
    if (value.isValid())
    {
        pushStack(STATE_VARIABLE);

        mStateStack[mStateDepth].variableTdfId = value.get()->getTdfId();
        blaze_strnzcpy(mStateStack[mStateDepth].variableTdfName, value.get()->getFullClassName(), EA::TDF::MAX_TDF_NAME_LEN);

        visit(rootTdf, parentTdf, tag, *value.get(), *value.get());

        popStack();
    }

    return true;
}

bool Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue)
{
    if (value.isValid())
    {
        pushStack(STATE_VARIABLE);

        mStateStack[mStateDepth].variableTdfId = value.getValue().getTdfId();
        blaze_strnzcpy(mStateStack[mStateDepth].variableTdfName, value.getValue().getFullName(), EA::TDF::MAX_TDF_NAME_LEN);

        visitReference(rootTdf, parentTdf, tag, value.getValue(), &referenceValue.getValue());

        popStack();
    }

    return true;
}

bool Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue)
{
    char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";
    const char8_t* name = value.getClassName();

    // Do nested struct initialization here
    if ((mStateStack[mStateDepth].state != STATE_ARRAY) && (mStateStack[mStateDepth].state != STATE_MAP))
    {
        //  if this EA::TDF::Tdf is a variable member of a container, we don't want to link the element the container's tag
        //  but to the classname (like if this were a simple nested tdf within a container.)
        if (mStateDepth == 0 || mStateStack[mStateDepth].state != STATE_VARIABLE ||
                (mStateStack[mStateDepth-1].state != STATE_ARRAY && mStateStack[mStateDepth-1].state != STATE_MAP))
        {
            buildElementName(parentTdf, tag, elementName, sizeof(elementName));
            name = elementName;
        }
    }

    pushStack(STATE_NORMAL);

    beginElement(name);

    value.visit(*this, rootTdf, referenceValue);

    endElement();

    popStack();

    return true;
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue)
{
    // Generate the element name to use for the array
    char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";
    buildElementName(parentTdf, tag, elementName, sizeof(elementName));

    pushStack(STATE_ARRAY);

    // Don't bother outputting empty arrays unless the parent is also an array,
    // then an empty element is important to preserve the structure
    bool parentIsArray = (mStateStack[mStateDepth-1].state == STATE_ARRAY);
    if (value.vectorSize() > 0 || parentIsArray)
    {
        if (mUseAttributes == true)
        {
            // take into account whether or not the parent is an array regarding emptiness
            openElementNode(elementName, nullptr, 0, parentIsArray);
        }
        else
        {
            // take into account whether or not the parent is an array regarding emptiness
            mXmlBuffer.putStartElement(elementName, parentIsArray);
        }

        value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);

        if (mUseAttributes == true)
        {
            closeElementNode();
        }
        else
        {
            mXmlBuffer.putEndElement();
        }
    }

    popStack();
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue)
{
    // Generate the element name to use for the map
    char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";
    buildElementName(parentTdf, tag, elementName, sizeof(elementName));

    pushStack(STATE_MAP);

    mStateStack[mStateDepth].nextIsMapKey = true;                                   // Next element encoded will be the key
  
    // Don't bother outputting empty maps unless the parent is an array,
    // then an empty element is important to preserve the structure
    bool parentIsArray = (mStateStack[mStateDepth-1].state == STATE_ARRAY);
    if (value.mapSize() > 0 || parentIsArray)
    {
        if (mUseAttributes == true)
        {
            openElementNode(elementName, nullptr, 0, parentIsArray);
        }
        else
        {
            mXmlBuffer.putStartElement(elementName, parentIsArray); 
        }

        value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);

        if (mUseAttributes == true)
        {
            closeElementNode();
        }
        else
        {
            mXmlBuffer.putEndElement();
        }
    }

    popStack();
}

bool Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue)
{
    EA_ASSERT(mStateStack[mStateDepth].state != STATE_UNION);          // Can't have a union in a union

    char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";
    const char8_t* name = value.getClassName();

    // Do nested struct initialization here
    if ((mStateStack[mStateDepth].state != STATE_ARRAY)
        && (mStateStack[mStateDepth].state != STATE_MAP))
    {
        buildElementName(parentTdf, tag, elementName, sizeof(elementName));
        name = elementName;
    }

    pushStack(STATE_UNION);

    mStateStack[mStateDepth].activeMember = value.getActiveMemberIndex();       // Which element in the union we're encoding

    // only encode member index if the tag of the active member is not defined
    const EA::TDF::TdfMemberInfo* memberInfo = nullptr;
    value.getMemberInfoByIndex(mStateStack[mStateDepth].activeMember, memberInfo);
    mStateStack[mStateDepth].encodeMemberIndex = (memberInfo != nullptr && memberInfo->getTag() == EA::TDF::TdfUnion::RESERVED_VALUE_TAG);

    beginElement(name);

    value.visit(*this, rootTdf, value);

    // Close off open union
    endElement();

    popStack();

    return true;
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%u", value ? 1 : 0);
    writePrimitive(parentTdf, tag, mChars);
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%c", value);
    writePrimitive(parentTdf, tag, mChars);
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%d", value);
    writePrimitive(parentTdf, tag, mChars);
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%u", value);
    writePrimitive(parentTdf, tag, mChars);
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%d", value);
    writePrimitive(parentTdf, tag, mChars);
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%u", value);
    writePrimitive(parentTdf, tag, mChars);
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%d", value);
    writePrimitive(parentTdf, tag, mChars);
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue)
{
    uint32_t bits = value.getBits();
    visit(rootTdf, parentTdf, tag, bits, bits, 0);
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%u", value);
    writePrimitive(parentTdf, tag, mChars);
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%" PRIi64, value);
    writePrimitive(parentTdf, tag, mChars);
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%" PRIu64, value);
    writePrimitive(parentTdf, tag, mChars);
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%.*g", FLT_MAX_10_EXP, value);
    writePrimitive(parentTdf, tag, mChars);
}


void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength)
{
    writePrimitive(parentTdf, tag, value.c_str());
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue)
{
    char8_t sizeStr[11];
    uint32_t valueCount = value.getCount();
    uint32_t encodedSize = ((valueCount % 3) == 0) ? ((valueCount/3) * 4) : (((valueCount + (3 - valueCount % 3)) / 3) * 4);

    XmlAttribute attr[2];
    attr[0].name = "count";
    attr[0].nameLen = strlen(attr[0].name);
    blaze_snzprintf(sizeStr, 11, "%d", encodedSize);
    attr[0].value = sizeStr;
    attr[0].valueLen = strlen(attr[0].value);

    // Allow for alternate encodings in the future, even though the code is not structured
    // for that now.
    attr[1].name = "enc";
    attr[1].nameLen = strlen(attr[1].name);
    attr[1].value = "base64";
    attr[1].valueLen = strlen(attr[1].value);

    char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";
    buildElementName(parentTdf, tag, elementName, sizeof(elementName));
    
    if (mUseAttributes)
    {
        XmlElementAttribute attributes[2];
        attributes[0].name = attr[0].name;
        attributes[0].value = attr[0].value;
        attributes[1].name = attr[1].name;
        attributes[1].value = attr[1].value;

        openElementNode(elementName, attributes, 2);

        uint8_t *data = value.getData();
        XmlElement *element = mPendingElements.top();

        element->mElementValueRaw.reserve(valueCount);
        for (uint32_t dataIdx=0; dataIdx < valueCount; ++dataIdx)
        {
            element->mElementValueRaw.push_back(*data);
            ++data;
        }
        element->mElementValueType = ELEMENT_VALUE_RAW_DATA;
        
        closeElementNode();
    }
    else
    {
        mXmlBuffer.putStartElement(elementName, &attr[0], 2);
        mXmlBuffer.putCharactersRaw("\r\n", 2);

        // Encode the data to the XmlBuffer
        ByteArrayInputStream input(value.getData(), valueCount);
        Base64::encode(&input, &mXmlBuffer);

        mXmlBuffer.putEndElement();
    }
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue)
{
    bool useIdentifier = true;
    
#ifndef BLAZE_CLIENT_SDK
    if (mFrame->enumFormat == RpcProtocol::ENUM_FORMAT_VALUE)
    {
        useIdentifier = false;
    }
#endif

    const char8_t *identifier = nullptr;
   
    if (EA_LIKELY(useIdentifier))
    {
        if (EA_LIKELY((enumMap != nullptr) && enumMap->findByValue(value, &identifier)))
            writePrimitive(parentTdf, tag, identifier);
        else
            writePrimitive(parentTdf, tag, "UNKNOWN");
    }
    else
    {
        // use standard int32_t visitor
        visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
    }
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue)
{
    writePrimitive(parentTdf, tag, value.toString().c_str());
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue)
{
    writePrimitive(parentTdf, tag, value.toString().c_str());
}

void Xml2Encoder::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue &referenceValue, const TimeValue defaultValue)
{
    blaze_snzprintf(mChars, sizeof(mChars), "%" PRIi64, value.getMicroSeconds());
    writePrimitive(parentTdf, tag, mChars);
}

/*** Private Methods *****************************************************************************/

/*************************************************************************************************/
/*!
    \brief beginElement

    Called by any member that needs to begin an XML element.  Takes care of constructing the
    element name and does any special handling for specific element types.

    \param[in] objectName - Optional name of object being encoded
*/
/*************************************************************************************************/
void Xml2Encoder::beginElement(const char8_t* objectName)
{
    bool useRawNames = false;
#ifndef BLAZE_CLIENT_SDK
    if (mFrame->format == RpcProtocol::FORMAT_USE_TDF_RAW_NAMES) {
        useRawNames = true;
    }
#endif

    char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";
    if (useRawNames)
    {
        // Copy object name directly into element name instead of mangling it.
        blaze_strnzcpy(elementName, objectName, MAX_XML_ELEMENT_LENGTH);
    }
    else
    {
        // Convert the objectName to lowercase, copying for us as well.
        uint32_t len = 0;                           // Will hold string length after loop finishes
        for ( ; len < sizeof(elementName)-1 ; ++len)
        {
            elementName[len] = LocaleTokenToLower(objectName[len]);
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
    }

    // write the empty element if this is the top level
    bool writeIfEmpty = (mStateDepth == 0) || (mStateStack[mStateDepth-1].state == STATE_ARRAY);

    if (mUseAttributes)
    {
        if (mStateStack[mStateDepth].state == STATE_UNION && mStateStack[mStateDepth].encodeMemberIndex)
        {
            XmlElementAttribute attribute;
            attribute.name = "member";
            char buf[16] = "";
            blaze_snzprintf(buf, sizeof(buf), "%d", mStateStack[mStateDepth].activeMember);
            attribute.value = buf;

            openElementNode(elementName, &attribute, 1, writeIfEmpty);
        }
        else
        {
            openElementNode(elementName, nullptr, 0, writeIfEmpty);
        }
    }
    else if (mStateDepth > 0 && mStateStack[mStateDepth-1].state == STATE_VARIABLE)
    {
        XmlAttribute attr[2];
        char buf[16] = "";
        blaze_snzprintf(buf, sizeof(buf), "%u", mStateStack[mStateDepth-1].variableTdfId);
        attr[0].set("tdfid", buf);
        attr[1].set("tdfclass", mStateStack[mStateDepth-1].variableTdfName);
        mXmlBuffer.putStartElement(elementName, attr, 2, writeIfEmpty);
    }
    else
    {
        if (mStateStack[mStateDepth].state == STATE_UNION && mStateStack[mStateDepth].encodeMemberIndex)
        {
            // Open the union element:  format <unionname member="activeMember">
            XmlAttribute attr;
            char buf[16] = "";
            blaze_snzprintf(buf, sizeof(buf), "%d", mStateStack[mStateDepth].activeMember);
            attr.set("member", buf);
            mXmlBuffer.putStartElement(elementName, &attr, 1, writeIfEmpty);
        }
        else
        {
            mXmlBuffer.putStartElement(elementName, writeIfEmpty);
        }
    }
}

/*************************************************************************************************/
/*!
    \brief endElement

    Called by any member that needs to end the current XML element.  Mainly called by endStruct()
    to close off any open elements in the struct or document.
*/
/*************************************************************************************************/
void Xml2Encoder::endElement()
{
    if (mUseAttributes == true)
    {
        mPendingElements.pop();
    }
    else
    {
        mXmlBuffer.putEndElement();
    }
}

/*************************************************************************************************/
/*!
    \brief buildElementName

    Called by any member that needs to build an element name out of a member name (or, if 
    a member name is not available a tag name)

    \param[in] tag - tag name of this element to use in case the member name is not available
    \param[in] buf - buffer to output the element name into
    \param[in] buflen - Length of buf
*/
/*************************************************************************************************/
void Xml2Encoder::buildElementName(const EA::TDF::Tdf& parentTdf, const uint32_t tag, char8_t* buf, const size_t buflen)
{
    char8_t tagName[Heat2Util::MAX_TAG_LENGTH+1] = "";     // Used for backup when there is no tag info map
    const char8_t* memberName = nullptr;
    bool useTags = false;
    bool useRawNames = false;
#ifndef BLAZE_CLIENT_SDK
    if(mFrame->format == RpcProtocol::FORMAT_USE_TDF_TAGS) {
        useTags = true;
    }
    else if (mFrame->format == RpcProtocol::FORMAT_USE_TDF_RAW_NAMES) {
        useRawNames = true;
    }
#endif

    const EA::TDF::TdfMemberInfo* info = nullptr;
    // Find out the member name of this tag
    // XML2 encoder supports the member nameoverride if one is defined
    if ( useTags || 
        !parentTdf.getMemberInfoByTag(tag, info) ||
        (mStateStack[mStateDepth].state == STATE_UNION && tag == EA::TDF::TdfUnion::RESERVED_VALUE_TAG) ||
        info == nullptr ||
        (memberName = info->getMemberName()) == nullptr)
    {
        // Member info not available, use the tag name
        Heat2Util::decodeTag(tag, tagName, sizeof(tagName), true);
        memberName = tagName;
    }

    // Convert the member name into an element name.  This is the member name, made all lowercase,
    // and removing any prefixed m or m_ from the front.  Only do this if we are dealing with an
    // actual member.  If we've just got the tag name, copy it right in.
    if (useRawNames)
    {
        blaze_strnzcpy(buf, memberName, buflen);
    }
    else if (tagName[0] == '\0')
    {
        convertMemberToElement(memberName, buf, buflen);
    }
    else
    {
        blaze_strnzcpy(buf, tagName, buflen);
    }
}

/*************************************************************************************************/
/*!
    \brief convertMemberToElement

    Utility method to take a member name and convert it into an element name by doing the 
    following:  convert everything to lowercase

    \param[in] memberName - Unmodified member name
    \param[in] buf - Where to write the modified member
    \param[in] buflen - Length in bytes of buf
*/
/*************************************************************************************************/
void Xml2Encoder::convertMemberToElement(const char8_t *memberName, char8_t* buf, const size_t buflen) const
{
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
}

/*************************************************************************************************/
/*!
    \brief writePrimitive

    Called by any of the type-specific method to encode primitive values (ints or strings).

    \param[in] value - the tag of the value being encoded
    \param[in] len - the value being encoded as a human readable string
*/
/*************************************************************************************************/
void Xml2Encoder::writePrimitive(const EA::TDF::Tdf& parentTdf, uint32_t tag, const char8_t* value)
{
    char8_t elementName[MAX_XML_ELEMENT_LENGTH] = "";

#ifndef BLAZE_CLIENT_SDK
    if ((mStateStack[mStateDepth].state == STATE_ARRAY) && (mFrame->useCommonListEntryName))
    {
        blaze_strnzcpy(elementName, "entry", 6);
    }
    else
    {
#endif
        buildElementName(parentTdf, tag, elementName, sizeof(elementName));

#ifndef BLAZE_CLIENT_SDK
    }
#endif

    // Spit out the value.  This will be handled differently depending on what state
    // we're in.
    if (mStateStack[mStateDepth].state == STATE_MAP)
    {
        // This is a map, it gets specially encoded because the key and value are 
        // passed in as two separate values.  We need to keep a little state to keep 
        // track of when we've got both the key and value before starting the element.
        if (mStateStack[mStateDepth].nextIsMapKey)
        {
            // Map is open and we have the key.  Open an "entry" element with the key as the
            // attribute and the data as the XML data.
            XmlAttribute attr;
            attr.name = "key";
            attr.nameLen = strlen(attr.name);
            attr.value = value;
            attr.valueLen = strlen(attr.value);

            if (mUseAttributes == true)
            {
                XmlElementAttribute attribute;
                attribute.name = attr.name;
                attribute.value = attr.value;

                openElementNode("entry", &attribute, 1);
            }
            else
            {
                mXmlBuffer.putStartElement("entry", &attr, 1);
            }
            mStateStack[mStateDepth].nextIsMapKey = false;
        }
        else
        {
            if (mUseAttributes == true)
            {
                XmlElementAttribute attribute;
                attribute.name = "value";
                attribute.value = value;
                mPendingElements.top()->mAttributes.push_back(attribute);

                closeElementNode();
            }
            else
            {
                // Got the data, just put it in there, the element is already open
                mXmlBuffer.putCharacters(value);
                mXmlBuffer.putEndElement();
            }
            mStateStack[mStateDepth].nextIsMapKey = true;
        }
    }
    else
    {
        if (mUseAttributes)
        {
            XmlElement *element = mPendingElements.top();

            XmlElementAttribute attribute;
            attribute.name = elementName;
            attribute.value = value;
            element->mAttributes.push_back(attribute);
        }
        else
        {
            mXmlBuffer.putStartElement(elementName);
            mXmlBuffer.putCharacters(value);
            mXmlBuffer.putEndElement();
        }
    }
}

/*************************************************************************************************/
/*!
    \brief popStack

    Updates the state stack and sets whether we are back at the top level
*/
/*************************************************************************************************/
void Xml2Encoder::popStack()
{
    EA_ASSERT(mStateDepth > 0);
    --mStateDepth;

    // For maps, writePrimitive will open the "entry" block that holds the key-value pair,
    // when processing the key, the "entry" will also be closed out in writePrimitive if the
    // value is also a primitive, all other value types close out the "entry" block here
    if (mStateStack[mStateDepth].state == STATE_MAP)
    {
        endElement();
        mStateStack[mStateDepth].nextIsMapKey = true;
    }
}


/*************************************************************************************************/
/*!
    \brief pushStack

    Pushes a new encoder state onto the stack.
*/
/*************************************************************************************************/
void Xml2Encoder::pushStack(State state)
{
    ++mStateDepth;
    EA_ASSERT(mStateDepth < MAX_STATE_DEPTH);

    mStateStack[mStateDepth].state = state;
}

/*************************************************************************************************/
/*!
    \brief openElementNode

    Create node for storing element data when formating XML data by attributes

    \param[in] elementName - Name of element
    \param[in] attributes - List of attributes associated with node
    \param[in] attributeCount - Number of attributes in list    
*/
/*************************************************************************************************/
void Xml2Encoder::openElementNode(const char8_t* elementName, const XmlElementAttribute *attributes /*= nullptr*/, 
                                  uint32_t attributeCount /*= 0*/, bool writeIfEmpty /*= false*/)
{
    // Allocate new element
    mXmlElements.push_back();
    XmlElement *newElement = &(mXmlElements.back());

    newElement->mElementName = elementName;
    newElement->mWriteIfEmpty = writeIfEmpty;

    // If this is not the root element, add it as a child of the current top element
    if (mPendingElements.empty() == false)
    {
        mPendingElements.top()->mChildren.push_back(newElement);
    }

    // Push new element onto the pending element stack
    mPendingElements.push(newElement);

    // Add specified attributes to the current element node
    for (uint32_t attributeIdx=0; attributeIdx < attributeCount; ++attributeIdx)
    {
        newElement->mAttributes.push_back(attributes[attributeIdx]);
    }
}

/*************************************************************************************************/
/*!
    \brief closeElementNode

    Close element node and pop from pending element stack when formating XML data by attributes
*/
/*************************************************************************************************/
void Xml2Encoder::closeElementNode()
{
    EA_ASSERT(mPendingElements.empty() == false);
    mPendingElements.pop();
}

/*************************************************************************************************/
/*!
    \brief writeElementNode

    Write an element and all it's children to XML when formating XML data by attributes

    \param[in] element - Element to write out to XML buffer  
*/
/*************************************************************************************************/
void Xml2Encoder::writeElementNode(const XmlElement *element)
{
    XmlAttribute elementAttributes[XmlBuffer::MAX_ATTRIBUTES];
    const XmlElementAttribute *attribute = nullptr;

    // Create list of attributes for element
    for (uint32_t attributeIdx = 0; attributeIdx < element->mAttributes.size(); ++attributeIdx)
    {
        attribute = &(element->mAttributes[attributeIdx]);

        elementAttributes[attributeIdx].name = attribute->name.c_str();
        elementAttributes[attributeIdx].nameLen = attribute->name.length();

        elementAttributes[attributeIdx].value = attribute->value.c_str();
        elementAttributes[attributeIdx].valueLen = attribute->value.length();
    }

    // Add element start tag and associated attributes to XML
    mXmlBuffer.putStartElement(element->mElementName.c_str(), elementAttributes, element->mAttributes.size(), element->mWriteIfEmpty);

    // Process any children elements
    const uint32_t numChildElements = static_cast<uint32_t>(element->mChildren.size());
    if (numChildElements > 0)
    {
        for (uint32_t childIdx=0; childIdx < numChildElements; ++childIdx)
        {
            writeElementNode(element->mChildren[childIdx]);
        }
    }
    // Check for element values
    else
    {
        switch (element->mElementValueType)
        {
            case ELEMENT_VALUE_NONE:
                // Nothing to do, everything is in the attributes
                break;

            case ELEMENT_VALUE_STRING:
                // Add element string value to XML
                mXmlBuffer.putCharacters(element->mElementValueString.c_str());
                break;

            case ELEMENT_VALUE_RAW_DATA:
                {
                    // Encode the data to the XmlBuffer
                    mXmlBuffer.putCharactersRaw("\r\n", 2);
                    ByteArrayInputStream input(&element->mElementValueRaw[0], static_cast<uint32_t>(element->mElementValueRaw.size()));
                    Base64::encode(&input, &mXmlBuffer);
                }
                break;

            default:
                // Invalid element type
                EA_ASSERT(false);
                break;
        }
    }

    // Add element end tag
    mXmlBuffer.putEndElement();
}

void Xml2Encoder::startDocument()
{
    mXmlBuffer.putStartDocument("UTF-8");
}

void Xml2Encoder::endDocument()
{
    mXmlBuffer.putEndDocument();
}

} // Blaze

#else // ENABLE_LEGACY_CODECS

#include "framework/blaze.h"
#include "framework/protocol/shared/xml2encoder.h"
#include "EAIO/EAStream.h"
#include "framework/util/shared/rawbufferistream.h"
#include "EATDF/tdfbasetypes.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief Xml3Encoder

    Construct a new XML encoder that encodes into the given buffer

    \param[in]  buffer - the buffer to encode into

*/
/*************************************************************************************************/
    bool Xml2Encoder::encode(RawBuffer& buffer, const EA::TDF::Tdf& tdf
#ifndef BLAZE_CLIENT_SDK
        , const RpcProtocol::Frame* frame
#endif
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        , bool onlyEncodeChanged
#endif
        )
    {
        RawBufferIStream istream(buffer);

        EA::TDF::EncodeOptions encOptions;
#ifndef BLAZE_CLIENT_SDK
        if (frame != nullptr)
        {
            encOptions.useCommonEntryName = frame->useCommonListEntryName;
            switch (frame->format)
            {
                case RpcProtocol::FORMAT_USE_TDF_NAMES:
                    encOptions.format = EA::TDF::EncodeOptions::FORMAT_USE_TDF_NAMES;
                    break;
                case RpcProtocol::FORMAT_USE_TDF_RAW_NAMES:
                    encOptions.format = EA::TDF::EncodeOptions::FORMAT_USE_TDF_RAW_NAMES;
                    break;
                default:
                    break;
            }

            switch (frame->enumFormat)
            {
                case RpcProtocol::ENUM_FORMAT_IDENTIFIER:
                    encOptions.enumFormat = EA::TDF::EncodeOptions::ENUM_FORMAT_IDENTIFIER;
                    break;
                case RpcProtocol::ENUM_FORMAT_VALUE:
                    encOptions.enumFormat = EA::TDF::EncodeOptions::ENUM_FORMAT_VALUE;
                    break;
                default:
                    break;
            }
        }
#endif

        EA::TDF::MemberVisitOptions visitOpt;
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        visitOpt.onlyIfSet = onlyEncodeChanged;
#endif
        visitOpt.onlyIfNotDefault = mDefaultDiff;
        return mEncoder.encode(istream, tdf, &encOptions, visitOpt);
    }

} // Blaze

#endif // ENABLE_LEGACY_CODECS
