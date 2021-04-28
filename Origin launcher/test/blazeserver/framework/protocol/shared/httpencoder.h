/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_HTTPENCODER_H
#define BLAZE_HTTPENCODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "framework/protocol/shared/legacytdfencoder.h"
#include "framework/util/shared/rawbuffer.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
//struct XmlAttribute;

class BLAZESDK_API HttpEncoder : public LegacyTdfEncoder
{
public:
    const static int32_t MAX_XML_ELEMENT_LENGTH = 128;
    const static int32_t XML_TAG_PREALLOCATION_SIZE = 32;

    HttpEncoder();
    ~HttpEncoder() override;

    size_t encodeSize() const override { return mBuffer->datasize(); }

    bool visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue) override;
    bool visit(EA::TDF::TdfUnion &tdf, const EA::TDF::TdfUnion &referenceValue) override { return visit((EA::TDF::Tdf&) tdf, (EA::TDF::Tdf &) referenceValue); }

    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue) override;

    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue) override;

    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue) override;
    
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue) override;
    // enumeration visitor
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue = 0 ) override;

    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TimeValue &value, const EA::TDF::TimeValue &referenceValue, const EA::TDF::TimeValue defaultValue) override;

    const char8_t* getName() const override { return HttpEncoder::getClassName(); }
    Encoder::Type getType() const override { return Encoder::HTTP; }

    static const char8_t* getClassName() { return "http"; }

protected:
    void setBuffer(RawBuffer* buffer) override;

    int32_t mStateDepth;
    char mChars[32];
    char8_t mKey[1024];

    typedef enum { STATE_NORMAL, STATE_ARRAY, STATE_MAP, STATE_UNION, STATE_VARIABLE } State;
    const static int32_t MAX_STATE_DEPTH = 32;

    struct
    {
        State state;                    // Normal, array, or map
        uint32_t dimension;             // Number of dimensions (if array)
        EA::TDF::TdfSizeVal sizes;      // Size of each dimension (if array or map)
        EA::TDF::TdfSizeVal counts;     // Num elements encoded so far per dimension (if array or map)
        BLAZE_EASTL_STRING mapKey;      // Holds the map key attribute for key/value pair (if map)
        bool parseVariableTdfInfo;      // informs encoder that we're parsing the variable tdf ID which is a special case.
    } mStateStack[MAX_STATE_DEPTH];
   
    typedef enum { ELEMENT_VALUE_NONE, ELEMENT_VALUE_STRING, ELEMENT_VALUE_RAW_DATA } ElementValueType;

    bool writePrimitive(uint32_t tag, const char8_t* value);
    bool isOk();
    bool popStack();
    void pushStack(State state);

private:
    inline void printChar(unsigned char c, bool skipSafeCheck = false);
    void printString(const char *s, bool skipSafeCheck = false);
    void printString(const char *s, size_t len, bool skipSafeCheck = false);

    bool pushKey(uint32_t tag);
    bool pushTagKey(uint32_t tag);
    bool pushNumericKey(uint32_t key);
    bool popKey();
    bool popRawKey();

    char8_t getNestDelim() const { return '|'; }
    const BLAZE_EASTL_STRING sSafeChars;
};


} // Blaze

#endif // BLAZE_HTTPENCODER_H

