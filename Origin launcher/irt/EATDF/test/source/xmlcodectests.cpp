#include <test.h>
#include <testdata.h>
#include <test/tdf/alltypes.h>
#include <test/tdf/codectypes.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EACType.h>
#include <EAAssert/eaassert.h>
#include <EASTL/fixed_vector.h>
#include <EAIO/EAFileBase.h>

#include <UTFXml/internal/Config.h>

#include <EAIO/EAStream.h>
#include <EAIO/EAStreamMemory.h>

#include <EATDF/codec/tdfxmlencoder.h>
#include <EATDF/codec/tdfxmldecoder.h>

using namespace EA::TDF;
using namespace EA::TDF::Test;


typedef size_t size_type;
class XMLStringIStream : public EA::IO::IStream
{
public:
    XMLStringIStream()
        : mReadData(nullptr),
        mReadLength(0),
        mReadPosition(0),
        mRefCount(0)
    {
    }
    virtual ~XMLStringIStream()
    {
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
        EA_ASSERT(nSize > 0);
        const size_type nBytesAvailable(nSize - mReadPosition);
        if (nBytesAvailable > 0)
        {
            if (nSize > nBytesAvailable)
                nSize = nBytesAvailable;

            memcpy(pData, &mReadData[mReadPosition], (size_t)nSize);
            mReadPosition += nSize;

            return nSize;
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

    virtual int AddRef() 
    { 
        mRefCount++;
        return mRefCount;
    }
    virtual int Release()
    {
        if (--mRefCount == 0)
        {
            mReadData     = nullptr;
            mReadLength   = 0;
            mReadPosition = 0;
            mReadCopied   = false;
        }
        return mRefCount;
    }

    // NO-OP
    virtual void SetReleaseAllocator(EA::Allocator::ICoreAllocator*) {} 
    virtual uint32_t GetType() const { return 0; }
    virtual int GetAccessFlags() const { return 0; }
    virtual int GetState() const { return 0; }
    virtual bool Close() { return true; }
    virtual size_type GetSize() const { return mReadLength; }
    virtual bool SetSize(size_type size) { return true; }
    virtual EA::IO::off_type GetPosition(EA::IO::PositionType positionType = EA::IO::kPositionTypeBegin) const { return 0; }
    virtual bool SetPosition(EA::IO::off_type position, EA::IO::PositionType positionType = EA::IO::kPositionTypeBegin) { return true; }
    virtual size_type GetAvailable() const { return 0; }
    virtual bool Flush() { return true; }

    eastl::string  mString;
    const char8_t* mReadData;          // Pointer to char data. Doesn't require 0-termination.
    size_t         mReadLength;        // Length of the data pointed to by mpData. Does not include a terminating 0 char.
    size_t         mReadPosition;      // Current position within mpData.
    bool           mReadCopied;        // True if mpData's contents copied from user-specified data as opposed to mpData being the pointer the user supplied.
    uint32_t mRefCount;
};

// list of emtpy union will be print as <union/> in the list
TEST(TestXMLCodec, ListOfEmptyUnions)
{
    ListOfUnion input, output;
    StringOrInt32 stringOrInt32Union1;
    StringOrInt32 stringOrInt32Union2;
    input.getListofUnion().push_back(&stringOrInt32Union1);
    input.getListofUnion().push_back(&stringOrInt32Union2);

    XMLStringIStream stream;
    XmlEncoder enc;
    EXPECT_TRUE(enc.encode(stream, input));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    XmlEncoder enc2;
    XMLStringIStream stream2;
    EXPECT_TRUE(enc2.encode(stream2, output));
    EXPECT_TRUE(input.equalsValue(output));
}


// empty union will be skipped
TEST(TestXMLCodec, EmptyUnion)
{
    SimpleUnion input, output;

    XMLStringIStream stream;
    XmlEncoder enc;
    EXPECT_TRUE(enc.encode(stream, input));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    XmlEncoder enc2;
    XMLStringIStream stream2;
    EXPECT_TRUE(enc2.encode(stream2, output));
    EXPECT_TRUE(input.equalsValue(output));
}

TEST(TestXMLCodec, MapOfEmptyLists)
{
    MapOfList input, output;
    EA::TDF::TdfPrimitiveVector<int32_t> innerList1;
    EA::TDF::TdfPrimitiveVector<int32_t> innerList2;
    input.getMapOfLists().insert(eastl::make_pair(EA::TDF::TdfString("outterMapKey1"), &innerList1));
    input.getMapOfLists().insert(eastl::make_pair(EA::TDF::TdfString("outterMapKey2"), &innerList2));

    XMLStringIStream stream;
    XmlEncoder enc;
    EXPECT_TRUE(enc.encode(stream, input));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    XmlEncoder enc2;
    XMLStringIStream stream2;
    EXPECT_TRUE(enc2.encode(stream2, output));
    EXPECT_TRUE(input.equalsValue(output));
}

TEST(TestXMLCodec, MapOfEmptyMaps)
{
    MapOfMap input, output;
    EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, int32_t, EA::TDF::TdfStringCompareIgnoreCase, true > innerMap1;
    EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, int32_t, EA::TDF::TdfStringCompareIgnoreCase, true > innerMap2;
    input.getMapOfMaps().insert(eastl::make_pair(EA::TDF::TdfString("outterMapKey1"), &innerMap1));
    input.getMapOfMaps().insert(eastl::make_pair(EA::TDF::TdfString("outterMapKey2"), &innerMap2));

    XMLStringIStream stream;
    XmlEncoder enc;
    EXPECT_TRUE(enc.encode(stream, input));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    XmlEncoder enc2;
    XMLStringIStream stream2;
    EXPECT_TRUE(enc2.encode(stream2, output));
    EXPECT_TRUE(input.equalsValue(output));
}

TEST(TestXMLCodec, MapOfEmptyUnions)
{
    MapofUnion input, output;
    StringOrInt32 string_or_int32_union;
    StringOrInt32 string_or_int32_union2;
    input.getMapOfUnionTest().insert(eastl::make_pair(EA::TDF::TdfString("MapKey1"), &string_or_int32_union));
    input.getMapOfUnionTest().insert(eastl::make_pair(EA::TDF::TdfString("MapKey2"), &string_or_int32_union2));

    XMLStringIStream stream;
    XmlEncoder enc;
    EXPECT_TRUE(enc.encode(stream, input));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    XmlEncoder enc2;
    XMLStringIStream stream2;
    EXPECT_TRUE(enc2.encode(stream2, output));
    EXPECT_TRUE(input.equalsValue(output));
}

TEST(TestXMLCodec, MapOfUnions)
{
    MapofUnion input, output;
    StringOrInt32 string_or_int32_union;
    string_or_int32_union.setInt(123);
    input.getMapOfUnionTest().insert(eastl::make_pair(EA::TDF::TdfString("MapKey"), &string_or_int32_union));

    XMLStringIStream stream;
    XmlEncoder enc;
    EXPECT_TRUE(enc.encode(stream, input));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    XmlEncoder enc2;
    XMLStringIStream stream2;
    EXPECT_TRUE(enc2.encode(stream2, output));
    EXPECT_TRUE(input.equalsValue(output));
}

TEST(TestXMLCodec, EmptyListofMaps)
{
    ListOfMap input, output;
    EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, int32_t, EA::TDF::TdfStringCompareIgnoreCase, true> innerMap1;
    EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, int32_t, EA::TDF::TdfStringCompareIgnoreCase, true> innerMap2;
    input.getListOfMaps().push_back(&innerMap1);
    input.getListOfMaps().push_back(&innerMap2);

    XMLStringIStream stream;
    XmlEncoder enc;
    EXPECT_TRUE(enc.encode(stream, input));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    XmlEncoder enc2;
    XMLStringIStream stream2;
    EXPECT_TRUE(enc2.encode(stream2, output));
    EXPECT_TRUE(input.equalsValue(output));
}


TEST(TestXMLCodec, EmptyList)
{
    ListOfList input, output;
    EA::TDF::TdfPrimitiveVector<int32_t> innerList1;
    input.getListOfLists().push_back(&innerList1);

    XMLStringIStream stream;
    XmlEncoder enc;
    EXPECT_TRUE(enc.encode(stream, input));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    XmlEncoder enc2;
    XMLStringIStream stream2;
    EXPECT_TRUE(enc2.encode(stream2, output));
    EXPECT_TRUE(input.equalsValue(output));
}


TEST(TestXMLCodec, ListOfListOfUnion)
{
    ListOfUnionList input, output;

    StringOrInt32 stringOrInt32Union_string_1;
    StringOrInt32 stringOrInt32Union_int32_1;

    stringOrInt32Union_string_1.setString("Arson1");
    stringOrInt32Union_int32_1.setInt(123);

    EA::TDF::TdfStructVector<StringOrInt32> unionList1;

    unionList1.push_back(&stringOrInt32Union_string_1);
    unionList1.push_back(&stringOrInt32Union_int32_1);

    input.getListOfLists().push_back(&unionList1);

    XMLStringIStream stream;
    XmlEncoder enc;
    EXPECT_TRUE(enc.encode(stream, input));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    XmlEncoder enc2;
    XMLStringIStream stream2;
    EXPECT_TRUE(enc2.encode(stream2, output));
    EXPECT_TRUE(input.equalsValue(output));
}




template <class T>
static int encodeDecodeTdf(const char8_t* name)
{
    int nErrorCount = 0;
    T input, output;

    TestData testData;
    testData.BLOB_VAL = (uint8_t *) "testblobdata";
    testData.BLOB_VAL_SIZE = static_cast<uint8_t>(strlen("testblobdata") + 1);
    
    fillTestData(input, testData);

    XMLStringIStream stream;
    XmlEncoder enc;
    EXPECT_TRUE(enc.encode(stream, input));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    XmlEncoder enc2;
    XMLStringIStream stream2;
    EXPECT_TRUE(enc2.encode(stream2, output));


    EXPECT_TRUE(input.equalsValue(output));
    EXPECT_TRUE(compareChangeBits(input, output, testData));

    return nErrorCount;
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

        XMLStringIStream stream;
        XmlEncoder enc;
        EXPECT_TRUE(enc.encode(stream, input));

        T output;
        XmlDecoder decoder;
        XMLStringIStream decoderStream;
        decoderStream.SetString(stream.mString.c_str(), stream.mString.length());
        EXPECT_TRUE(decoder.decode(decoderStream, output));

        XmlEncoder enc2;
        XMLStringIStream stream2;
        EXPECT_TRUE(enc2.encode(stream2, output));

        EXPECT_TRUE(input.equalsValue(output));
        EXPECT_TRUE(compareChangeBits(input, output, testData));
    }

    return nErrorCount;
}


#define TEST_VISIT_TDF(_name) \
    TEST(TestXMLCodec, Tdf##_name) { ASSERT_EQ(0, encodeDecodeTdf<_name>(#_name)); }

#define TEST_VISIT_UNION(_name) \
    TEST(TestXMLCodec, Union##_name) { ASSERT_EQ(0, encodeDecodeUnion<_name>(#_name)); }

TEST_VISIT_TDF(AllPrimitivesClassSimple);
TEST_VISIT_TDF(AllPrimitivesClass);
TEST_VISIT_TDF(AllComplexClassXML);
TEST_VISIT_UNION(AllPrimitivesUnion);
TEST_VISIT_TDF(AllComplexClass);
TEST_VISIT_UNION(AllPrimitivesUnionAllocInPlace);
TEST_VISIT_UNION(AllComplexUnion);
TEST_VISIT_UNION(AllComplexUnionAllocInPlace);
TEST_VISIT_TDF(AllListsPrimitivesClass);

TEST_VISIT_TDF(AllListOfUnionTest);
TEST_VISIT_TDF(AllListsComplexClassTest);

TEST_VISIT_UNION(AllListsPrimitivesUnion);
TEST_VISIT_UNION(AllListsPrimitivesUnionAllocInPlace);
TEST_VISIT_UNION(AllListsComplexUnion);
TEST_VISIT_UNION(AllListsComplexUnionAllocInPlace);


TEST_VISIT_TDF(AllMapsPrimitivesClass);
TEST_VISIT_TDF(AllMapsComplexClassTest);
TEST_VISIT_TDF(AllMapsComplexClass);
TEST_VISIT_UNION(AllMapsPrimitivesUnion);
TEST_VISIT_UNION(AllMapsPrimitivesUnionAllocInPlace);
TEST_VISIT_UNION(AllMapsComplexUnion);
TEST_VISIT_UNION(AllMapsComplexUnionAllocInPlace);

// Error handling test cases
template <class T1, class T2>
int32_t skippingXmlElementsHelper(T1& input, T2& baseline, T2& output)
{
    int nErrorCount = 0;
    TestData testData;
    testData.BLOB_VAL = (uint8_t *) "testblobdata";
    testData.BLOB_VAL_SIZE = static_cast<uint8_t>(strlen("testblobdata") + 1);

    fillTestData(input, testData);


    XMLStringIStream stream;
    XmlEncoder enc;
    EXPECT_TRUE(enc.encode(stream, input));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    fillTestData(baseline, testData);
    EXPECT_TRUE(baseline.equalsValue(output));

    return nErrorCount;
}

TEST(TestXMLCodec, SkipElementsInPrimTdf)
{
    AllPrimitivesClass input;
    AllPrimitivesClassSkipPrim baseline, output;

    int nErrorCount = skippingXmlElementsHelper<AllPrimitivesClass, AllPrimitivesClassSkipPrim>(input, baseline, output);
    ASSERT_EQ(0, nErrorCount);
}

TEST(TestXMLCodec, SkipElementsInComplexTdf)
{
    AllComplexClass input;
    AllComplexClassSkipMember baseline, output;

    int nErrorCount = skippingXmlElementsHelper<AllComplexClass, AllComplexClassSkipMember>(input, baseline, output);
    ASSERT_EQ(0, nErrorCount);
}

TEST(TestXMLCodec, SkipListOfPrim)
{
    AllListsPrimitivesClass input;
    AllListsPrimitivesClassSkipList baseline, output;

    int nErrorCount = skippingXmlElementsHelper<AllListsPrimitivesClass, AllListsPrimitivesClassSkipList>(input, baseline, output);
    ASSERT_EQ(0, nErrorCount);
}

TEST(TestXMLCodec, SkipListOfComplexTdf)
{
    AllListsComplexClass input;
    AllListsComplexClassSkipList baseline, output;

    int nErrorCount = skippingXmlElementsHelper<AllListsComplexClass, AllListsComplexClassSkipList>(input, baseline, output);
    ASSERT_EQ(0, nErrorCount);
}

TEST(TestXMLCodec, SkipMapOfPrimTdf)
{
    AllMapsPrimitivesClass input;
    AllMapsPrimitivesClassSkipMap baseline, output;

    int nErrorCount = skippingXmlElementsHelper<AllMapsPrimitivesClass, AllMapsPrimitivesClassSkipMap>(input, baseline, output);
    ASSERT_EQ(0, nErrorCount);
}

TEST(TestXMLCodec, SkipMapOfComplexTdf)
{
    AllMapsComplexClass input;
    AllMapsComplexClassSkipMap baseline, output;

    int nErrorCount = skippingXmlElementsHelper<AllMapsComplexClass, AllMapsComplexClassSkipMap>(input, baseline, output);
    ASSERT_EQ(0, nErrorCount);
}

TEST(TestXMLCodec, EmptyStream)
{
    AllPrimitivesClass input, output;

    TestData testData;

    fillTestData(input, testData);

    XMLStringIStream stream;

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

}

TEST(TestXMLCodec, NonUTF8Char)
{
    AllPrimitivesClass input, output;

    TestData testData;

    // this non-UTF8 character ("FOO"+0xFFFE) will be replaced with '???'
    char invalidString[7] = {'F', 'O', 'O', '\xEF', '\xBF', '\xBE', 0};
    testData.STRING_VAL = &invalidString[0];
    
    fillTestData(input, testData);

    XMLStringIStream stream;
    XmlEncoder enc;
    EXPECT_TRUE(enc.encode(stream, input));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    EXPECT_TRUE(strcmp(output.getString(), "FOO???") == 0);
}

TEST(TestXMLCodec, EncodeEnumAsValue)
{
    AllPrimitivesClass input, output;

    TestData testData;
    fillTestData(input, testData);

    XMLStringIStream stream;
    XmlEncoder enc;
    EA::TDF::EncodeOptions encOptions;
    encOptions.enumFormat = EA::TDF::EncodeOptions::ENUM_FORMAT_VALUE;
    EXPECT_TRUE(enc.encode(stream, input, &encOptions));

    XmlDecoder decoder;
    XMLStringIStream decoderStream;
    decoderStream.SetString(stream.mString.c_str(), stream.mString.length());

    EXPECT_TRUE(decoder.decode(decoderStream, output));

    XmlEncoder enc2;
    XMLStringIStream stream2;
    EXPECT_TRUE(enc2.encode(stream2, output));


    EXPECT_TRUE(input.equalsValue(output));
    EXPECT_TRUE(compareChangeBits(input, output, testData));
}
