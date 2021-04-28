/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_XML2ENCODER_H
#define BLAZE_XML2ENCODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#ifdef ENABLE_LEGACY_CODECS
/*** Include files *******************************************************************************/

#include "framework/protocol/shared/legacytdfencoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/util/shared/xmlbuffer.h"

// EASTL Includes
#include "EASTL/vector.h"
#include "EASTL/stack.h"
#include "EASTL/fixed_string.h"
#include "EASTL/list.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
struct XmlAttribute;

class BLAZESDK_API Xml2Encoder : public LegacyTdfEncoder
{
public:
    const static int32_t MAX_XML_ELEMENT_LENGTH = 128;
    const static int32_t XML_TAG_PREALLOCATION_SIZE = 32;

    Xml2Encoder();

    virtual size_t encodeSize() const { return mBuffer->datasize(); }

    virtual bool visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue);
    virtual bool visit(EA::TDF::TdfUnion &tdf, const EA::TDF::TdfUnion &referenceValue) { return visit((EA::TDF::Tdf&) tdf, (EA::TDF::Tdf &) referenceValue); }

    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue);

    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue);

    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue);
    
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue);
    // enumeration visitor
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue = 0 );

    virtual bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue);
    virtual bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue);
    virtual bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue);
    virtual bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue);
    virtual void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TimeValue &value, const EA::TDF::TimeValue &referenceValue, const EA::TDF::TimeValue defaultValue);

    virtual const char8_t* getName() const { return Xml2Encoder::getClassName(); }
    virtual Encoder::Type getType() const { return Encoder::XML2; }

    static const char8_t* getClassName() { return "xml2"; }

protected:
    virtual void setBuffer(RawBuffer* buffer);

    virtual void startDocument();
    virtual void endDocument();

    typedef enum { STATE_NORMAL, STATE_ARRAY, STATE_MAP, STATE_UNION, STATE_VARIABLE } State;
    const static int32_t MAX_STATE_DEPTH = 32;
    const static int32_t MAX_FLOAT_NUM_CHARACTERS = 64;

    XmlBuffer mXmlBuffer;
    int32_t mStateDepth;
    char mChars[MAX_FLOAT_NUM_CHARACTERS];

    struct
    {
        State state;                               // Normal, array, map, etc.
        uint32_t activeMember;                     // active member (if union)
        bool encodeMemberIndex;                    // encode member index (if union)
        EA::TDF::TdfId variableTdfId;                       // or tdf id of variable tdf member (if variable)
        char8_t variableTdfName[EA::TDF::MAX_TDF_NAME_LEN]; // Holds the TDF class name for the STATE_VARIABLE
        bool nextIsMapKey;
    } mStateStack[MAX_STATE_DEPTH];
   
    typedef enum { ELEMENT_VALUE_NONE, ELEMENT_VALUE_STRING, ELEMENT_VALUE_RAW_DATA } ElementValueType;

    struct XmlElementAttribute
    {
#ifndef BLAZE_CLIENT_SDK
        eastl::fixed_string<char8_t, XML_TAG_PREALLOCATION_SIZE, true> name;
        eastl::fixed_string<char8_t, XML_TAG_PREALLOCATION_SIZE, true> value;
#else
        eastl::fixed_string<char8_t, XML_TAG_PREALLOCATION_SIZE, true, blaze_eastl_allocator> name;
        eastl::fixed_string<char8_t, XML_TAG_PREALLOCATION_SIZE, true, blaze_eastl_allocator> value;
#endif
    };
    typedef BLAZE_EASTL_VECTOR<XmlElementAttribute> ElementAttributes;

    struct XmlElement
    {
#ifndef BLAZE_CLIENT_SDK
        eastl::fixed_string<char8_t, XML_TAG_PREALLOCATION_SIZE, true> mElementName;
        eastl::fixed_string<char8_t, XML_TAG_PREALLOCATION_SIZE, true> mElementValueString;
#else
        eastl::fixed_string<char8_t, XML_TAG_PREALLOCATION_SIZE, true, blaze_eastl_allocator> mElementName;
        eastl::fixed_string<char8_t, XML_TAG_PREALLOCATION_SIZE, true, blaze_eastl_allocator> mElementValueString;
#endif
        BLAZE_EASTL_VECTOR<uint8_t> mElementValueRaw;
        ElementValueType mElementValueType;
        ElementAttributes mAttributes;
        BLAZE_EASTL_VECTOR<XmlElement *> mChildren;
        bool mWriteIfEmpty;

        XmlElement()
#ifdef BLAZE_CLIENT_SDK
          : mElementValueRaw(MEM_GROUP_FRAMEWORK, "Xml2Encoder::XmlElement::mElementValueRaw"),
            mAttributes(MEM_GROUP_FRAMEWORK, "Xml2Encoder::XmlElement::mAttributes"),
            mChildren(MEM_GROUP_FRAMEWORK, "Xml2Encoder::XmlElement::mChildren")
#endif
        {
            mElementValueType = ELEMENT_VALUE_NONE;
            mWriteIfEmpty = false;
        }
    };
    typedef BLAZE_EASTL_LIST<XmlElement> ElementList;
           
    ElementList mXmlElements; // Used to keep track of created nodes when formating XML data by attributes  
#ifndef BLAZE_CLIENT_SDK
    eastl::stack< XmlElement *, eastl::vector<XmlElement *> > mPendingElements; // Used to keep track of the current element we are working with when formating XML data by attributes
#else
    eastl::stack< XmlElement *, eastl::vector<XmlElement *, blaze_eastl_allocator> > mPendingElements;
#endif
    bool mUseAttributes;

    void beginElement(const char8_t* objectName);
    void putStartElement(const char8_t* elementName, bool writeIfEmpty = false);
    void endElement();
    void buildElementName(const EA::TDF::Tdf& parentTdf, const uint32_t tag, char8_t* buf, const size_t buflen);
    void convertMemberToElement(const char8_t *memberName, char8_t* buf, const size_t buflen) const;
    void writePrimitive(const EA::TDF::Tdf& parentTdf, uint32_t tag, const char8_t* value);
    void popStack();
    void pushStack(State state);

    // XML attribute helper functions
    void openElementNode(const char8_t* elementName, const XmlElementAttribute *attributes = nullptr, uint32_t attributeCount = 0, bool writeIfEmpty = false);
    void closeElementNode();
    void writeElementNode(const XmlElement *element);
};

} // Blaze

#else // ENABLE_LEGACY_CODECS

#include "framework/protocol/shared/tdfencoder.h"
#include "EATDF/codec/tdfxmlencoder.h"

namespace Blaze
{
    class BLAZESDK_API Xml2Encoder : public Blaze::TdfEncoder
    {
    public:
        Xml2Encoder() : mDefaultDiff(false){}
        ~Xml2Encoder() override {}

        bool encode(RawBuffer& buffer, const EA::TDF::Tdf& tdf
#ifndef BLAZE_CLIENT_SDK
            , const RpcProtocol::Frame* frame = nullptr
#endif
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
            , bool onlyEncodeChanged = false
#endif
            ) override;

        static const char8_t* getClassName() { return "xml2"; }
        Encoder::Type getType() const override { return Encoder::XML2; }
        const char8_t* getName() const override { return Xml2Encoder::getClassName(); }
        void setUseDefaultDiff(bool useDefaultDiff) { mDefaultDiff = useDefaultDiff; }
    private:
        EA::TDF::XmlEncoder mEncoder;
        bool mDefaultDiff;
    };

} // Blaze

#endif // ENABLE_LEGACY_CODECS
#endif // BLAZE_XML2ENCODER_H

