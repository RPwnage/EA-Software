/*! *********************************************************************************************/
/*!
    \file jsoncodectests.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include <test.h>
#include <testdata.h>
#include <test/tdf/alltypes.h>
#include <test/tdf/codectypes.h>
#include <EAJson/JsonWriter.h>
#include <EAJson/JsonCallbackReader.h>
#include <EAJson/Json.h>
#include <EAStdC/EASprintf.h>
#include <EAAssert/eaassert.h>
#include <EASTL/fixed_vector.h>

#include <EATDF/codec/tdfjsonencoder.h>
#include <EATDF/codec/tdfjsondecoder.h>

using namespace EA::TDF;
using namespace EA::TDF::Test;

typedef size_t size_type;
class StringIStream : public EA::IO::IStream
{
public:
    StringIStream()
        : mReadData(nullptr),
        mReadLength(0),
        mReadPosition(0)
    {
    }
    ~StringIStream()
    {
        resetStream();
    }

    bool Write(const void* pData, size_type nSize)
    {
        mString.append((const char8_t*)pData, (eastl_size_t)nSize);
        return true;
    }

    // for simplify, we just do the copy here
    bool SetString(const char8_t* pString, size_t stringLength)
    {
        mReadData = pString;
        mReadLength = stringLength;
        return true;
    }

    virtual size_type Read(void* pData, size_type nSize) 
    { 
        EA_ASSERT(nSize == 1); EA_UNUSED(nSize);

        if(mReadPosition < mReadLength)
        {
            *(char8_t*)pData = mReadData[mReadPosition];
            mReadPosition++;
            return 1;
        }

        return 0; // EOF
    }

    void resetStream()
    {
        mReadData     = nullptr;
        mReadLength   = 0;
        mReadPosition = 0;
        mReadCopied   = false;
        mString = "";
    }

    // NO-OP
    virtual int AddRef() { return 0; }
    virtual int Release() { return 0; }
    virtual void SetReleaseAllocator(EA::Allocator::ICoreAllocator*) {} 
    virtual uint32_t GetType() const { return 0; }
    virtual int GetAccessFlags() const { return 0; }
    virtual int GetState() const { return 0; }
    virtual bool Close() { return true; }
    virtual size_type GetSize() const { return 0; }
    virtual bool SetSize(size_type size) { return true; }
    virtual EA::Json::off_type GetPosition(EA::IO::PositionType positionType = EA::IO::kPositionTypeBegin) const { return 0; }
    virtual bool SetPosition(EA::IO::off_type position, EA::IO::PositionType positionType = EA::IO::kPositionTypeBegin) { return true; }
    virtual size_type GetAvailable() const { return 0; }
    virtual bool Flush() { return true; }

    eastl::string  mString;
    const char8_t* mReadData;          // Pointer to char data. Doesn't require 0-termination.
    size_t         mReadLength;        // Length of the data pointed to by mpData. Does not include a terminating 0 char.
    size_t         mReadPosition;      // Current position within mpData.
    bool           mReadCopied;        // True if mpData's contents copied from user-specified data as opposed to mpData being the pointer the user supplied.
};


TEST(TestJsonCodec, Subfield)
{
    AllPrimitivesClassSimple input, output;
    TestData testData;
    testData.BOOL_VAL = true;
    fillTestData(input, testData);

    StringIStream encoderStream;
    JsonEncoder enc;
    EncodeOptions encOptions;
    MemberVisitOptions visOpt;
    visOpt.subFieldTag = AllPrimitivesClassSimple::TAG_UINT8;
    EXPECT_TRUE(enc.encode(encoderStream, input, &encOptions, visOpt));

    JsonDecoder decoder;
    MemberVisitOptions visOpt2;
    visOpt2.subFieldTag = AllPrimitivesClassSimple::TAG_UINT8;
    StringIStream decoderStream;
    decoderStream.SetString(encoderStream.mString.c_str(), encoderStream.mString.length());
    EXPECT_TRUE(decoder.decode(decoderStream, output, visOpt2)); // this buffer will only contain the subfield member we want to decode

    StringIStream encoderStream2;
    JsonEncoder enc2;
    MemberVisitOptions visOpt3;
    visOpt3.subFieldTag = AllPrimitivesClassSimple::TAG_UINT8;
    enc2.encode(encoderStream2, output, nullptr, visOpt3);

    EXPECT_STREQ(encoderStream.mString.c_str(), encoderStream2.mString.c_str());
}

TEST(TestJsonCodec, SubfieldinComplexTdf)
{
    TestSubfield input, output;
    TestData testData;
    testData.BOOL_VAL = true;
    fillTestData(input, testData);

    StringIStream encoderStream;
    JsonEncoder enc;
    EncodeOptions encOptions;
    MemberVisitOptions visOpt;
    visOpt.subFieldTag = TestSubfield::TAG_FOO;
    EXPECT_TRUE(enc.encode(encoderStream, input, &encOptions, visOpt));

    JsonDecoder decoder;
    MemberVisitOptions visOpt2;
    visOpt2.subFieldTag = TestSubfield::TAG_FOO;
    StringIStream decoderStream;
    decoderStream.SetString(encoderStream.mString.c_str(), encoderStream.mString.length());
    EXPECT_TRUE(decoder.decode(decoderStream, output, visOpt2));

    StringIStream encoderStream2;
    JsonEncoder enc2;
    MemberVisitOptions visOpt3;
    visOpt3.subFieldTag = TestSubfield::TAG_FOO;
    enc2.encode(encoderStream2, output, nullptr, visOpt3);

    EXPECT_STREQ(encoderStream.mString.c_str(), encoderStream2.mString.c_str());
}

template <class T>
static int encodeDecodeTdf(const char8_t* name)
{
    T input, output;

    TestData testData;
    testData.BLOB_VAL = (uint8_t *) "testblobdata";
    testData.BLOB_VAL_SIZE = static_cast<uint8_t>(strlen("testblobdata") + 1);

    fillTestData(input, testData);

    StringIStream encoderStream;
    JsonEncoder enc;
    enc.encode(encoderStream, input);

    JsonDecoder decoder;
    StringIStream decoderStream;
    decoderStream.SetString(encoderStream.mString.c_str(), encoderStream.mString.length());
    bool result;
    result = decoder.decode(decoderStream, output);

    StringIStream encoderStream2;
    JsonEncoder enc2;
    enc2.encode(encoderStream2, output);

    EXPECT_TRUE(result);
    EXPECT_TRUE(input.equalsValue(output));
    EXPECT_TRUE(compareChangeBits(input, output, testData));

    return 0;
}

template <class T>
static int encodeDecodeUnion(const char8_t* name)
{
    int nErrorCount = 0;
    T input;

    TestData testData;
    testData.BLOB_VAL = (uint8_t *) "testblobdata";
    testData.BLOB_VAL_SIZE = static_cast<uint8_t>(strlen("testblobdata") + 1);

    for (uint32_t memberIndex = 0; memberIndex < TdfUnion::INVALID_MEMBER_INDEX ; ++memberIndex)
    {        
        input.switchActiveMember(memberIndex);
        if (input.getActiveMemberIndex() == TdfUnion::INVALID_MEMBER_INDEX)
            break;

        fillTestData(input, testData);


        StringIStream encoderStream;
        JsonEncoder enc;
        enc.encode(encoderStream, input);

        T output;
        JsonDecoder decoder;
        StringIStream decoderStream;
        decoderStream.SetString(encoderStream.mString.c_str(), encoderStream.mString.length());
        EXPECT_TRUE(decoder.decode(decoderStream, output));

        EXPECT_TRUE(input.equalsValue(output));

        EXPECT_TRUE(compareChangeBits(input, output, testData));
    }

    return nErrorCount;
}

#define TEST_VISIT_TDF(_name) \
    TEST(TestJsonCodec, Tdf##_name) { ASSERT_EQ(0, encodeDecodeTdf<_name>(#_name)); }

#define TEST_VISIT_UNION(_name) \
    TEST(TestJsonCodec, Union##_name) { ASSERT_EQ(0, encodeDecodeUnion<_name>(#_name)); } 


TEST_VISIT_TDF(AllPrimitivesClass);
TEST_VISIT_TDF(AllComplexClass);
TEST_VISIT_UNION(AllPrimitivesUnion);
TEST_VISIT_UNION(AllPrimitivesUnionAllocInPlace);
TEST_VISIT_UNION(AllComplexUnion);
TEST_VISIT_UNION(AllComplexUnionAllocInPlace);

TEST_VISIT_TDF(AllListsPrimitivesClass);
TEST_VISIT_TDF(AllListsComplexClassTest);
TEST_VISIT_UNION(AllListsPrimitivesUnion);
TEST_VISIT_UNION(AllListsPrimitivesUnionAllocInPlace);
TEST_VISIT_UNION(AllListsComplexUnion);
TEST_VISIT_UNION(AllListsComplexUnionAllocInPlace);


TEST_VISIT_TDF(AllMapsPrimitivesClass);
TEST_VISIT_TDF(AllMapsComplexClass);
TEST_VISIT_UNION(AllMapsPrimitivesUnion);
TEST_VISIT_UNION(AllMapsPrimitivesUnionAllocInPlace);
TEST_VISIT_UNION(AllMapsComplexUnion);
TEST_VISIT_UNION(AllMapsComplexUnionAllocInPlace);


template <class T>
static int testVisitTdf()
{
    int nErrorCount = 0;
    T input, output;

    TestData testData;
    testData.BLOB_VAL = (uint8_t *) "testblobdata";
    testData.BLOB_VAL_SIZE = static_cast<uint8_t>(strlen("testblobdata") + 1);

    fillTestData(input, testData);

    StringIStream encoderStream;
    JsonEncoder enc;
    enc.encode(encoderStream, input);

    JsonDecoder decoder;
    StringIStream decoderStream;
    decoderStream.SetString(encoderStream.mString.c_str(), encoderStream.mString.length());
    bool result;
    result = decoder.decode(decoderStream, output);

    EXPECT_TRUE(result);
    result = input.equalsValue(output);
    EXPECT_TRUE(result);
    result = compareChangeBits(input, output, testData);
    EXPECT_TRUE(result);

    return nErrorCount;
}


template <class T>
static int testVisitUnion()
{
    int nErrorCount = 0;
    T input;

    TestData testData;
    testData.BLOB_VAL = (uint8_t *) "testblobdata";
    testData.BLOB_VAL_SIZE = static_cast<uint8_t>(strlen("testblobdata") + 1);

    for (uint32_t memberIndex = 0; memberIndex < TdfUnion::INVALID_MEMBER_INDEX ; ++memberIndex)
    {        
        input.switchActiveMember(memberIndex);
        if (input.getActiveMemberIndex() == TdfUnion::INVALID_MEMBER_INDEX)
            break;

        fillTestData(input, testData);


        StringIStream encoderStream;
        JsonEncoder enc;
        enc.encode(encoderStream, input);

        T output;
        JsonDecoder decoder;
        StringIStream decoderStream;
        decoderStream.SetString(encoderStream.mString.c_str(), encoderStream.mString.length());
        EXPECT_TRUE(decoder.decode(decoderStream, output));

        EXPECT_TRUE(input.equalsValue(output));

        EXPECT_TRUE(compareChangeBits(input, output, testData));
    }

    return nErrorCount;
}
TEST_ALL_TDFS(testVisitTdf, testVisitUnion);

// Error handling test cases
template <class T1, class T2>
int32_t skippingElementsHelper(T1& input, T2& baseline, T2& output)
{
    TestData testData;
    testData.BLOB_VAL = (uint8_t *) "testblobdata";
    testData.BLOB_VAL_SIZE = static_cast<uint8_t>(strlen("testblobdata") + 1);

    fillTestData(input, testData);


    StringIStream encoderStream;
    JsonEncoder enc;
    enc.encode(encoderStream, input);

    JsonDecoder decoder;
    StringIStream decoderStream;
    decoderStream.SetString(encoderStream.mString.c_str(), encoderStream.mString.length());
    EXPECT_TRUE(decoder.decode(decoderStream, output));

    fillTestData(baseline, testData);
    EXPECT_TRUE(baseline.equalsValue(output));
    EXPECT_TRUE(compareChangeBits(baseline, output, testData));

    return 0;
}

TEST(TestJsonCodec, SkipElementsInPrimTdf)
{
    AllPrimitivesClass input;
    AllPrimitivesClassSkipPrim baseline, output;

    int nErrorCount = skippingElementsHelper<AllPrimitivesClass, AllPrimitivesClassSkipPrim>(input, baseline, output);
    EXPECT_EQ(0, nErrorCount);
}

TEST(TestJsonCodec, SkipElementsInComplexTdf)
{
    AllComplexClass input;
    AllComplexClassSkipMember baseline, output;

    int nErrorCount = skippingElementsHelper<AllComplexClass, AllComplexClassSkipMember>(input, baseline, output);
    EXPECT_EQ(0, nErrorCount);
}

TEST(TestJsonCodec, SkipListOfPrim)
{
    AllListsPrimitivesClass input;
    AllListsPrimitivesClassSkipList baseline, output;

    int nErrorCount = skippingElementsHelper<AllListsPrimitivesClass, AllListsPrimitivesClassSkipList>(input, baseline, output);
    EXPECT_EQ(0, nErrorCount);
}

TEST(TestJsonCodec, SkipListOfComplexTdf)
{
    AllListsComplexClass input;
    AllListsComplexClassSkipList baseline, output;

    int nErrorCount = skippingElementsHelper<AllListsComplexClass, AllListsComplexClassSkipList>(input, baseline, output);
    EXPECT_EQ(0, nErrorCount);
}

TEST(TestJsonCodec, SkipListOfComplexTdfInBadFormat)
{
    // Skip a wrong JSON format array: [{123},{123}]
    MalformedListClass output;

    int nErrorCount = 0;
    eastl::string errJsonArray = "{\n\"uInt8\":\n[\n{123},\n{123}\n]\n}\n";

    StringIStream stream;
    stream.SetString(errJsonArray.c_str(), errJsonArray.length());
    JsonDecoder decoder;
    EXPECT_TRUE(!decoder.decode(stream, output)); // decoder should fail to decode the wrong format

    EXPECT_EQ(0, nErrorCount);
}

TEST(TestJsonCodec, SkipMapOfPrimTdf)
{
    AllMapsPrimitivesClass input;
    AllMapsPrimitivesClassSkipMap baseline, output;

    int nErrorCount = skippingElementsHelper<AllMapsPrimitivesClass, AllMapsPrimitivesClassSkipMap>(input, baseline, output);
    EXPECT_EQ(0, nErrorCount);
}

TEST(TestJsonCodec, SkipMapOfComplexTdf)
{
    AllMapsComplexClass input;
    AllMapsComplexClassSkipMap baseline, output;

    int nErrorCount = skippingElementsHelper<AllMapsComplexClass, AllMapsComplexClassSkipMap>(input, baseline, output);
    EXPECT_EQ(0, nErrorCount);
}

// skip Tdf members in union
TEST(TestJsonCodec, SkipElementsInPrimUnion)
{
    AllPrimitivesUnion input;
    AllPrimitivesUnionSkipPrim baseline, output;

    int nErrorCount = skippingElementsHelper<AllPrimitivesUnion, AllPrimitivesUnionSkipPrim>(input, baseline, output);
    EXPECT_EQ(0, nErrorCount);
}

TEST(TestJsonCodec, SkipElementsInComplexUnion)
{
    AllComplexUnion input;
    AllComplexUnionSkipMems baseline, output;

    int nErrorCount = skippingElementsHelper<AllComplexUnion, AllComplexUnionSkipMems>(input, baseline, output);
    EXPECT_EQ(0, nErrorCount);
}

TEST(TestJsonCodec, EmptyVariable)
{
    simpleVaribleTdf input, output;

    int nErrorCount = 0;
    StringIStream encoderStream;
    JsonEncoder enc;
    EncodeOptions encOptions;
    EXPECT_TRUE(enc.encode(encoderStream, input, &encOptions));

    JsonDecoder decoder;
    StringIStream decoderStream;
    decoderStream.SetString(encoderStream.mString.c_str(), encoderStream.mString.length());
    EXPECT_TRUE(decoder.decode(decoderStream, output));

    EXPECT_TRUE(input.equalsValue(output));
    EXPECT_EQ(0, nErrorCount);
}

TEST(TestJsonCodec, EmptyListOfMaps)
{

    ListOfMap input, output;
    EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, int32_t, EA::TDF::TdfStringCompareIgnoreCase, true> innerMap1;
    EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, int32_t, EA::TDF::TdfStringCompareIgnoreCase, true> innerMap2;
    input.getListOfMaps().push_back(&innerMap1);
    input.getListOfMaps().push_back(&innerMap2);

    int nErrorCount = 0;
    StringIStream encoderStream;
    JsonEncoder enc;
    EncodeOptions encOptions;
    EXPECT_TRUE(enc.encode(encoderStream, input, &encOptions));

    JsonDecoder decoder;
    StringIStream decoderStream;
    decoderStream.SetString(encoderStream.mString.c_str(), encoderStream.mString.length());
    EXPECT_TRUE(decoder.decode(decoderStream, output));

    EXPECT_TRUE(input.equalsValue(output));
    EXPECT_EQ(0, nErrorCount);
}

TEST(TestJsonCodec, EmptyMap)
{
    SimpleMap input, output;
    int nErrorCount = 0;
    StringIStream encoderStream;
    JsonEncoder enc;
    EncodeOptions encOptions;
    EXPECT_TRUE(enc.encode(encoderStream, input, &encOptions));

    JsonDecoder decoder;
    StringIStream decoderStream;
    decoderStream.SetString(encoderStream.mString.c_str(), encoderStream.mString.length());
    EXPECT_TRUE(decoder.decode(decoderStream, output));

    EXPECT_TRUE(input.equalsValue(output));
    EXPECT_EQ(0, nErrorCount);
}

TEST(TestJsonCodec, EmptyMapOfVariables)
{
    simpleVariableMap input, output;
    int nErrorCount = 0;
    StringIStream encoderStream;
    JsonEncoder enc;
    EncodeOptions encOptions;
    EXPECT_TRUE(enc.encode(encoderStream, input, &encOptions));

    JsonDecoder decoder;
    StringIStream decoderStream;
    decoderStream.SetString(encoderStream.mString.c_str(), encoderStream.mString.length());
    EXPECT_TRUE(decoder.decode(decoderStream, output));

    EXPECT_TRUE(input.equalsValue(output));
    EXPECT_EQ(0, nErrorCount);
}


TEST(TestJsonCodec, TypeConversion)
{
    // All basic types should attempt conversion. 
    // We do not fail in the case that a string cannot be converted to a number/bool. 
    eastl::string input = "{\"string\": 123, \"int16\": \"not an error 321\", \"int32\": \"456\", \"uint32\": 789.0, \"float\": \"1234.5\", \"bool\": 1}";
    AllPrimitivesClass output;

    int nErrorCount = 0;
    StringIStream encoderStream;
    JsonDecoder decoder;
    StringIStream decoderStream;
    decoderStream.SetString(input.c_str(), input.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));
    EXPECT_TRUE(output.getStringAsTdfString() == "123");
    EXPECT_TRUE(output.getInt16() == 0);            // Not decoded, but not an error.
    EXPECT_TRUE(output.getInt32() == 456);
    EXPECT_TRUE(output.getUInt32() == 789);
    EXPECT_TRUE(output.getFloat() == 1234.5);
    EXPECT_TRUE(output.getBool() == true);

    EXPECT_EQ(0, nErrorCount);
}

/********************************** Tests *************************************/
