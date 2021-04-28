/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_JSONENCODER_H
#define BLAZE_JSONENCODER_H

#ifdef ENABLE_LEGACY_CODECS
/*** Include files *******************************************************************************/

#include "framework/protocol/shared/legacytdfencoder.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/util/shared/outputstream.h"

// EASTL Includes
#include "EASTL/vector.h"
#include "EASTL/stack.h"
#include "EASTL/fixed_string.h"
#include "EAJson/JsonWriter.h"

using namespace EA::Json;

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class JsonBuffer : public IWriteStream, 
    public OutputStream
{
public:
    JsonBuffer() : mRawBuffer(nullptr) {};
    virtual ~JsonBuffer(){};

    void initialize(RawBuffer* rawBuffer)    
    {
        mRawBuffer = rawBuffer;
    }

    // IWriteStream Interface
    virtual bool Write(const void* pData, size_type nSize)
    {
        char8_t* t = (char8_t*)mRawBuffer->acquire(nSize + 1);
        if (t != nullptr)
        {
            if (pData != nullptr)
                memcpy(t, pData, nSize);
            t[nSize] = '\0';
            mRawBuffer->put(nSize);
        }

        return true;
    }

    // OutputStream interface
    virtual uint32_t write(uint8_t data)
    {
        Write((const char8_t*)&data, 1);
        return 1;
    }

    virtual uint32_t write(uint8_t* data, uint32_t length)
    {
        Write((const char8_t*)data, length);
        return length;
    }

    virtual uint32_t write(uint8_t* data, uint32_t offset, uint32_t length)
    {
        Write((const char8_t*)&data[offset], length);
        return length;
    }

    RawBuffer * getBuffer() {return mRawBuffer;}

private:
    // Copy constructor 
    JsonBuffer(const JsonBuffer& otherObj);

    // Assignment operator
    JsonBuffer& operator=(const JsonBuffer& otherObj);

    void printChar(char c)
    {
        uint8_t* t = mRawBuffer->acquire(2);
        if (t != nullptr)
        {
            t[0] = c;
            t[1] = '\0';

            mRawBuffer->put(1);
        }
    }

private:
    RawBuffer *mRawBuffer;          // buffer to receive output data
};

class BLAZESDK_API JsonEncoder : public LegacyTdfEncoder
{
public:
    const static int32_t MAX_XML_ELEMENT_LENGTH = 128;
    const static int32_t XML_TAG_PREALLOCATION_SIZE = 32;

    JsonEncoder();
    ~JsonEncoder();

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

    virtual const char8_t* getName() const { return JsonEncoder::getClassName(); }
    virtual Encoder::Type getType() const { return Encoder::JSON; }

    static const char8_t* getClassName() { return "json"; }

    typedef BLAZE_EASTL_VECTOR<uint32_t> FilterTagList;
    void setSubField(const uint32_t* tagArray, size_t count);
    void setSubField(uint32_t tag);
    void clearSubField();

    void setFormatOptions(int indentSpacing, int lineEnd) { mIndentSpacing = indentSpacing; mLineEnd = lineEnd; }

protected:
    virtual void setBuffer(RawBuffer* buffer);

    virtual void startDocument();
    virtual void endDocument();

    JsonBuffer mJsonBuffer;
    JsonWriter mWriter;

    int32_t mStateDepth;
    char mChars[32];
    
    FilterTagList mFilterTags;
    uint32_t mFilterIndex;
    bool mEnabled;

    int mIndentSpacing;
    int mLineEnd;

    typedef enum { 
        STATE_NORMAL, 
        STATE_ARRAY, 
        STATE_MAP, 
        STATE_UNION, 
        STATE_VARIABLE 
    } State;

    const static int32_t MAX_STATE_DEPTH = 32;

    struct StateStruct
    {
        State state;              // Normal, array, or map
        union {
            uint32_t dimensions;            // Number of dimensions (if array)
            uint32_t totalNumMapElements;   // or number of elements (if map)
            uint32_t activeMember;          // or active member (if union)
            uint32_t variableTdfId;         // or tdf id of variable tdf member (if variable)
        };
        uint32_t currentNumMapElements; // or current number of elements encoded (if map)
        EA::TDF::TdfSizeVal sizes;                // Size of each dimension (if array)        
        union {
            char8_t mapKey[MAX_MAP_ELEMENT_KEY_LENGTH];   // Holds the map key attribute for key/value pair (if map)
            char8_t variableTdfName[EA::TDF::MAX_TDF_NAME_LEN]; // Holds the TDF class name for the STATE_VARIABLE
            char8_t memberName[EA::TDF::MAX_TDF_NAME_LEN]; // Holds the member name for the UNION
            bool nextIsMapKey;
        };
        const EA::TDF::Tdf* mParentTdf;

    } mStateStack[MAX_STATE_DEPTH]; //NOTE: all StateStruct members are memset initialized to zeros at JsonEncoder ctor (see)
   
    typedef enum { ELEMENT_VALUE_NONE, ELEMENT_VALUE_STRING, ELEMENT_VALUE_RAW_DATA } ElementValueType; 
    
    bool beginElement(const char8_t* objectName = nullptr, bool includeName = true);
    bool endElement();
    bool buildElementName(const uint32_t tag, char8_t* buf, const size_t buflen);
    bool convertMemberToElement(const char8_t *memberName, char8_t* buf, const size_t buflen) const;
    bool writePrimitiveMapKey(const char8_t* value);
    bool writePrimitive(uint32_t tag);
    void updateMapState();
    void updateArrayState();
    bool isOk();
    bool popStack();
    bool matchFilterForPrimitive(uint32_t tag);
    void clearFilterForPrimitive(bool isMatch);
};

} // Blaze

#else // ENABLE_LEGACY_CODECS

/*** Include files *******************************************************************************/
#include "EAIO/EAStream.h"
#include "framework/protocol/shared/tdfencoder.h"
#include "EATDF/codec/tdfjsonencoder.h"

namespace Blaze
{
    class BLAZESDK_API JsonEncoder : public TdfEncoder
    {
    public:
        JsonEncoder() : mSubfieldTag(0), mDefaultDiff(false), mEncodeVariableGenericTdfInfo(true) {}
        ~JsonEncoder() override {}

        bool encode(RawBuffer& buffer, const EA::TDF::Tdf& tdf
#ifndef BLAZE_CLIENT_SDK
            , const RpcProtocol::Frame* frame = nullptr
#endif
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
            , bool onlyEncodeChanged = false
#endif
            ) override;

        static const char8_t* getClassName() { return "json"; }
        Encoder::Type getType() const override { return Encoder::JSON; }
        const char8_t* getName() const override { return JsonEncoder::getClassName(); }
        void setSubField(uint32_t tag);
        void setUseDefaultDiff(bool useDefaultDiff) { mDefaultDiff = useDefaultDiff; }
        void setEncodeVariableGenericTdfInfo(bool encodeVariableGenericTdfInfo) { mEncodeVariableGenericTdfInfo = encodeVariableGenericTdfInfo; }
        void clearSubField();
    private:
        uint32_t mSubfieldTag;
        bool mDefaultDiff;
        bool mEncodeVariableGenericTdfInfo;
        EA::TDF::JsonEncoder mEncoder;
    };

} // Blaze
#endif // ENABLE_LEGACY_CODECS

#endif // BLAZE_JSONENCODER_H

