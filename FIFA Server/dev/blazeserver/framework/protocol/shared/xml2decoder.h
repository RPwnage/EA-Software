/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_XML2DECODER_H
#define BLAZE_XML2DECODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#ifdef ENABLE_LEGACY_CODECS
/*** Include files *******************************************************************************/

#include "EASTL/hash_map.h"
#include "framework/protocol/shared/legacytdfdecoder.h"
#include "framework/util/shared/xmlbuffer.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RawBuffer;

class BLAZESDK_API Xml2Decoder : public LegacyTdfDecoder, XmlCallback
{
public:
    const static int32_t MAX_XML_ELEMENT_LENGTH = 128;

    Xml2Decoder();
    ~Xml2Decoder();

    virtual bool visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue);
    virtual bool visit(EA::TDF::TdfUnion &tdf, const EA::TDF::TdfUnion &referenceValue);

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

    virtual const char8_t* getName() const { return Xml2Decoder::getClassName(); }
    virtual Decoder::Type getType() const { return Decoder::XML2; }
    static const char8_t* getClassName() { return "xml2"; }

    // XmlCallback implementations for parsing
    virtual void startDocument();
    virtual void endDocument();
    virtual void startElement(const char *name, size_t nameLen, const XmlAttribute *attributes, size_t attributeCount);
    virtual void endElement(const char *name, size_t nameLen);
    virtual void characters(const char *characters, size_t charLen);
    virtual void cdata(const char *data, size_t dataLen);

protected:
    virtual void setBuffer(RawBuffer* buffer);

    const char8_t* getKeyValue();
    size_t getKeyValueLen();
    bool convertMemberToElement(const char8_t *memberName, char8_t* buf, const size_t buflen) const;
    void growString(char8_t*& buf, size_t& bufSize, size_t bufCount, size_t bytesNeeded);
    bool pushKey(const EA::TDF::Tdf &parentTdf, uint32_t tag);
    void pushKey(const char8_t* name, size_t nameLen);
    void pushArrayIndex(uint32_t index);
    void pushMapIndex(const char8_t* name, size_t nameLen);
    bool popKey();
    bool popIndex();

    char8_t* mKey;
    size_t mKeyBufSize; // buffer size (capacity) for key
    size_t mKeyLen; // used amount for key

    struct XmlData
    {
        XmlData() : mData(nullptr), mDataLen(0), mOwnedData(nullptr)
        {
        }

        ~XmlData()
        {
            deleteOwnedData();
        }

        // sets pointer to new data buffer. Owns the data.
        char* allocateOwnedData(size_t dataLen)
        {
            // Note: if for some reason reallocating, any previously owned data will be cleared
            deleteOwnedData();
#ifdef BLAZE_CLIENT_SDK
            mOwnedData = BLAZE_NEW_ARRAY(char8_t, dataLen + 1, MEM_GROUP_FRAMEWORK_TEMP, "XmlData::allocateOwnedData::mOwnedData");
#else
            mOwnedData = BLAZE_NEW_ARRAY(char8_t, dataLen + 1);
#endif
            memset(mOwnedData, 0, dataLen + 1);
            set(mOwnedData, dataLen);
            return mOwnedData;
        }

        // sets pointer to input data.
        void set(const char* data, size_t dataLen)
        {
            mData = data;
            mDataLen = dataLen;
        }

        const char* getData()
        {
            return mData;
        }

        size_t getDataLen()
        {
            return mDataLen;
        }

    private:
        const char* mData;
        size_t mDataLen;
        char* mOwnedData;
        void deleteOwnedData()
        {
#ifdef BLAZE_CLIENT_SDK
            BLAZE_DELETE_ARRAY(MEM_GROUP_FRAMEWORK_TEMP, mOwnedData);
#else
            delete [] mOwnedData;
#endif
        }
    };
    typedef BLAZE_EASTL_HASH_MAP<BLAZE_EASTL_STRING, XmlData, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> ParamMap;
    ParamMap mParamMap;
    typedef BLAZE_EASTL_HASH_MAP<BLAZE_EASTL_STRING, uint32_t, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> ArrayMap;
    ArrayMap mArraySizeMap;
    ArrayMap mArrayIndexMap;
#ifndef BLAZE_CLIENT_SDK
    typedef eastl::vector<eastl::string> ClassnameArray;
#else
    class ClassnameArray : public BLAZE_EASTL_VECTOR<BLAZE_EASTL_STRING>
    {
    public:
        ClassnameArray() : BLAZE_EASTL_VECTOR<BLAZE_EASTL_STRING>(MEM_GROUP_FRAMEWORK, "Xml2Decoder::ClassnameArray") {}
    };
#endif
    typedef BLAZE_EASTL_HASH_MAP<BLAZE_EASTL_STRING, ClassnameArray, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> VarArrayMap;
    VarArrayMap mVarArrayMap;

    char8_t mIndex[1024];

    typedef enum { STATE_NORMAL, STATE_ARRAY, STATE_MAP, STATE_UNION, STATE_ERROR } State;
    const static int32_t MAX_STATE_DEPTH = 32;

    typedef BLAZE_EASTL_VECTOR<BLAZE_EASTL_STRING> XmlMapKeyList;
    struct StateStruct
    {
        State state; // normal, array, map, etc.
        uint32_t dimensionSize; // size of dimension (if array or map)
        uint32_t dimensionIndex; // num elements decoded so far (if array or map)
        XmlMapKeyList mapKeyList; // Stores map keys in an indexed list (if map)
        bool readValue; // Determines whether get operation reads the key (false) or value (true)

        enum { VARTDF_NULL = 0, VARTDF_TDFID = 1, VARTDF_TDF = 2 };
        uint32_t readVariableTdf;   // Determines whether visiting a variable tdf. 0 = none, 1 = tdfid, 2 = tdf

        StateStruct()
#ifdef BLAZE_CLIENT_SDK
            : mapKeyList(MEM_GROUP_FRAMEWORK, "StateStruct::mapKeyList")
#endif
        {}
    } mStateStack[MAX_STATE_DEPTH];
    int32_t mStateDepth;

    void pushStack(State state);
    bool popStack();
};

} // Blaze

#else // ENABLE_LEGACY_CODECS

#include "framework/protocol/shared/tdfdecoder.h"
#include "EATDF/codec/tdfxmldecoder.h"

namespace Blaze
{
    class BLAZESDK_API Xml2Decoder : public TdfDecoder
    {
    public:
        Xml2Decoder() {}
        ~Xml2Decoder() override {}

#ifdef BLAZE_CLIENT_SDK
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        virtual Blaze::BlazeError decode(RawBuffer& buffer, EA::TDF::Tdf& tdf, bool onlyDecodeChanged = false) override;
#else
        virtual Blaze::BlazeError decode(RawBuffer& buffer, EA::TDF::Tdf& tdf);
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
#else
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        BlazeRpcError DEFINE_ASYNC_RET(decode(RawBuffer& buffer, EA::TDF::Tdf& tdf, bool onlyDecodeChanged = false) override);
#else
        virtual BlazeRpcError DEFINE_ASYNC_RET(decode(RawBuffer& buffer, EA::TDF::Tdf& tdf));
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
#endif //BLAZE_CLIENT_SDK

        static const char8_t* getClassName() { return "xml2"; }
        Decoder::Type getType() const override { return Decoder::XML2; }
        const char8_t* getName() const override { return Xml2Decoder::getClassName(); }
        const char8_t* getErrorMessage() const override { return mDecoder.getErrorMessage(); }

    private:
        EA::TDF::XmlDecoder mDecoder;
    };
} // Blaze
#endif // ENABLE_LEGACY_CODECS

#endif // BLAZE_XML2DECODER_H
