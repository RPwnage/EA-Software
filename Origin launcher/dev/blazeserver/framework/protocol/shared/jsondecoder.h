/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_JSONDECODER_H
#define BLAZE_JSONDECODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#ifdef ENABLE_LEGACY_CODECS

/*** Include files *******************************************************************************/

#include "EASTL/hash_map.h"
#include "framework/protocol/shared/legacytdfdecoder.h"
#include "EAJson/JsonDomReader.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

using namespace EA::Json;
namespace Blaze
{
    class RawBuffer;

    class BLAZESDK_API JsonDecoder : public LegacyTdfDecoder
    {
    public:
        const static int32_t MAX_XML_ELEMENT_LENGTH = 128;

        JsonDecoder();
        JsonDecoder(MemoryGroupId mMemGroupId);
        ~JsonDecoder();

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

        virtual const char8_t* getName() const { return JsonDecoder::getClassName(); }
        virtual Decoder::Type getType() const { return Decoder::JSON; }
        static const char8_t* getClassName() { return "json"; }

        typedef BLAZE_EASTL_VECTOR<uint32_t> FilterTagList;
        void setSubField(const uint32_t* tagArray, size_t count);
        void setSubField(uint32_t tag);
        void clearSubField();

    protected:
        virtual void setBuffer(RawBuffer* buffer);
        bool convertMemberToElement(const char8_t *memberName, char8_t* buf, const size_t buflen) const;
        bool pushJsonNode(const EA::TDF::Tdf &parentTdf, uint32_t tag);
        bool popJsonNode();

        typedef BLAZE_EASTL_VECTOR<BLAZE_EASTL_STRING> MapKeyList;

        typedef enum { STATE_NORMAL, STATE_ARRAY, STATE_MAP, STATE_UNION, STATE_VARIABLE } State;
        const static int32_t MAX_STATE_DEPTH = 32;

        struct StateStruct
        {
            State state; // normal, array, map, etc.
            uint32_t dimensionSize; // size of dimension (if array or map)
            uint32_t dimensionIndex; // num elements decoded so far (if array or map)
            bool readValue; // Determines whether get operation reads the key (false) or value (true)
            MapKeyList mapKeyList; // Stores map keys in an indexed list (if map)

            StateStruct() :
                state(STATE_NORMAL),
                dimensionSize(0),
                dimensionIndex(0),
                readValue(false)
#ifdef BLAZE_CLIENT_SDK
                ,mapKeyList(MEM_GROUP_FRAMEWORK, "JsonDecoder::StateStruct::mapKeyList")
#endif
            {}
        } mStateStack[MAX_STATE_DEPTH];
        int32_t mStateDepth;

        typedef BLAZE_EASTL_VECTOR<JsonDomNode*> DomNodeVector;
        DomNodeVector mDomNodeStack;

        FilterTagList mFilterTags;
        uint32_t mFilterIndex;
        bool mEnabled;

        MemoryGroupId mMemGroupId;

        void pushStack(State state);
        bool popStack();

        int64_t getJsonDomInteger(JsonDomNode* node, int64_t defaultValue = 0);
        double getJsonDomDouble(JsonDomNode* node, double defaultValue = 0);
        bool getJsonDomBool(JsonDomNode* node, bool defaultValue = false);
        const char8_t* getJsonDomString(JsonDomNode* node);
        size_t getJsonDomNodeArraySize(JsonDomNode* node);
        JsonDomNode*  getJsonDomNodeFromArray(JsonDomNode* node, size_t index);
        JsonDomObjectValue* getFirstJsonDomObjectValue();
        JsonDomObjectValue* getJsonDomObjectValue(const char8_t* pName, bool bCaseSensitive);
        JsonDomObjectValueArray::iterator getNodeIterator(const char8_t* pName, bool bCaseSensitive);
        JsonDomObjectValueArray::iterator invalidObjectIterator();

    private:
        struct EnumKeyCompare
        {
            EnumKeyCompare(const EA::TDF::TdfEnumMap* enumMap) : mEnumMap(enumMap) {}
            bool operator() (const BLAZE_EASTL_STRING& a, const BLAZE_EASTL_STRING& b);
        private:
            const EA::TDF::TdfEnumMap* mEnumMap;
        };
    };

} // Blaze


#else // ENABLE_LEGACY_CODECS

/*** Include files *******************************************************************************/
#include "EAIO/EAStream.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "EATDF/codec/tdfjsondecoder.h"

namespace Blaze
{
    class BLAZESDK_API JsonDecoder : public Blaze::TdfDecoder
    {
    public:
        JsonDecoder() : mSubfieldTag(0) {}
        ~JsonDecoder() override {}

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

        static const char8_t* getClassName() { return "json"; }
        Decoder::Type getType() const override { return Decoder::JSON; }
        const char8_t* getName() const override { return JsonDecoder::getClassName(); }
        const char8_t* getErrorMessage() const override { return mDecoder.getErrorMessage(); }
        void setSubField(uint32_t tag) 
        { 
            mSubfieldTag = tag; 
        }

    private:
        uint32_t mSubfieldTag;
        EA::TDF::JsonDecoder mDecoder;
    };
} // Blaze
#endif // ENABLE_LEGACY_CODECS

#endif // BLAZE_XML2DECODER_H
