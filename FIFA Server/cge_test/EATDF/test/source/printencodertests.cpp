/*! *********************************************************************************************/
/*!
    \file 


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include <test.h>
#include <testdata.h>
#include <test/tdf/alltypes.h>
#include <EATDF/printencoder.h>

using namespace EA::TDF;
using namespace EA::TDF::Test;

template <class T>
void verifyPrintEncoderPrintedString(T& value, const char8_t* expected)
{
    EA::TDF::PrintEncoder printEncoder;
    EA::TDF::TdfPrintHelperString printHelper;
    EA::TDF::TdfGenericReferenceConst ref(value);
    printEncoder.print(printHelper, ref);
    EXPECT_STREQ(printHelper.getString().c_str(), expected);
}

//returns: true - for types with test cases; false - for types with no test cases
bool printEncoderPrintTestByType(EA::TDF::TdfType tdfType)
{
    switch (tdfType)
    {
    case TDF_ACTUAL_TYPE_INT8:
    {
        int8_t value = -128;
        verifyPrintEncoderPrintedString(value, "-128");
        value = 127;
        verifyPrintEncoderPrintedString(value, "127");
        return true;
    }
    case TDF_ACTUAL_TYPE_UINT8:
    {
        uint8_t value = 0;
        verifyPrintEncoderPrintedString(value, "0");
        value = 255;
        verifyPrintEncoderPrintedString(value, "255");
        return true;
    }
    case TDF_ACTUAL_TYPE_INT16:
    {
        int16_t value = SHRT_MIN;
        verifyPrintEncoderPrintedString(value, "-32768");
        value = SHRT_MAX;
        verifyPrintEncoderPrintedString(value, "32767");
        return true;
    }
    case TDF_ACTUAL_TYPE_UINT16:
    {
        uint16_t value = 0;
        verifyPrintEncoderPrintedString(value, "0");
        value = USHRT_MAX;
        verifyPrintEncoderPrintedString(value, "65535");
        return true;
    }
    case TDF_ACTUAL_TYPE_INT32:
    {
        int32_t value = INT_MIN;
        verifyPrintEncoderPrintedString(value, "-2147483648");
        value = INT_MAX;
        verifyPrintEncoderPrintedString(value, "2147483647");
        return true;
    }
    case TDF_ACTUAL_TYPE_UINT32:
    {
        uint32_t value = 0;
        verifyPrintEncoderPrintedString(value, "0");
        value = UINT_MAX;
        verifyPrintEncoderPrintedString(value, "4294967295");
        return true;
    }
    case TDF_ACTUAL_TYPE_INT64:
    {
        int64_t value = LLONG_MIN;
        verifyPrintEncoderPrintedString(value, "-9223372036854775808");
        value = LLONG_MAX;
        verifyPrintEncoderPrintedString(value, "9223372036854775807");
        return true;
    }
    case TDF_ACTUAL_TYPE_UINT64:
    {
        uint64_t value = 0;
        verifyPrintEncoderPrintedString(value, "0");
        value = ULLONG_MAX;
        verifyPrintEncoderPrintedString(value, "18446744073709551615");
        return true;
    }
    case TDF_ACTUAL_TYPE_FLOAT:
    {
        float_t value = 12345.54f;
        EA::TDF::PrintEncoder printEncoder;
        EA::TDF::TdfPrintHelperString printHelper;
        EA::TDF::TdfGenericReferenceConst ref(value);
        printEncoder.print(printHelper, ref);
        EXPECT_STREQ(printHelper.getString().substr(0, 8).c_str(), "12345.54");    //float has only 7 accurate digits
        return true;
    }
    case TDF_ACTUAL_TYPE_TIMEVALUE:
    {
        EA::TDF::TimeValue value(12345678);
        verifyPrintEncoderPrintedString(value, "12345678");
        EA::TDF::PrintEncoder printEncoder;
        EA::TDF::TdfPrintHelperString printHelper;
        EA::TDF::TdfGenericReferenceConst ref(value);
        printEncoder.print(printHelper, ref, 0, false);
        EXPECT_STREQ(printHelper.getString().c_str(), "12345678 (0x0000000000BC614E)");
        return true;
    }
    case TDF_ACTUAL_TYPE_STRING:
    {
        EA::TDF::TdfString value1("");
        verifyPrintEncoderPrintedString(value1, "\"\"");
        EA::TDF::TdfString value2("test text");
        verifyPrintEncoderPrintedString(value2, "\"test text\"");
        return true;
    }
    case TDF_ACTUAL_TYPE_LIST:
    {
        EA::TDF::TdfPrimitiveVector<EA::TDF::TdfString> list;
        verifyPrintEncoderPrintedString(list, "[]");
        list.push_back("value1");
        verifyPrintEncoderPrintedString(list, "[ [0] = \"value1\" ]");
        list.push_back("value2");
        verifyPrintEncoderPrintedString(list, "[ [0] = \"value1\" [1] = \"value2\" ]");
        list.push_back("value3");
        verifyPrintEncoderPrintedString(list, "[ [0] = \"value1\" [1] = \"value2\" [2] = \"value3\" ]");
        list.push_back("");
        verifyPrintEncoderPrintedString(list, "[ [0] = \"value1\" [1] = \"value2\" [2] = \"value3\" [3] = \"\" ]");
        return true;
    }
    case TDF_ACTUAL_TYPE_MAP:
    {
        EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, EA::TDF::TdfString> map;
        verifyPrintEncoderPrintedString(map, "[]");
        map["key1"] = "value1";
        verifyPrintEncoderPrintedString(map, "[ (\"key1\", \"value1\") ]");
        map["key2"] = "value2";
        verifyPrintEncoderPrintedString(map, "[ (\"key1\", \"value1\") (\"key2\", \"value2\") ]");
        map["key0"] = "value0";
        verifyPrintEncoderPrintedString(map, "[ (\"key0\", \"value0\") (\"key1\", \"value1\") (\"key2\", \"value2\") ]");
        map["key3"] = "";
        verifyPrintEncoderPrintedString(map, "[ (\"key0\", \"value0\") (\"key1\", \"value1\") (\"key2\", \"value2\") (\"key3\", \"\") ]");
        return true;
    }
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
    {
        EA::TDF::GenericTdfType value;
        verifyPrintEncoderPrintedString(value, "{}");
        return true;
    }
    case TDF_ACTUAL_TYPE_BLOB:
    {
        EA::TDF::TdfBlob blob;
        verifyPrintEncoderPrintedString(blob, "{}");
        uint8_t data[] = {65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90};
        blob.setData(data, sizeof(data) / sizeof(data[0]));
        verifyPrintEncoderPrintedString(blob, "{ 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 50 ABCDEFGHIJKLMNOP 51 52 53 54 55 56 57 58 59 5a                   QRSTUVWXYZ       }");
        return true;
    }
    default:
        break;
    }

    return false;
}

//Test PrintEncoder::print can print various types of objects to strings with expected formats
TEST(PrintEncoderTest, PrintEncoderPrintTest)
{
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_INT8));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_UINT8));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_INT16));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_UINT16));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_INT32));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_UINT32));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_INT64));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_UINT64));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_FLOAT));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_TIMEVALUE));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_STRING));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_LIST));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_MAP));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_GENERIC_TYPE));
    EXPECT_TRUE(printEncoderPrintTestByType(TDF_ACTUAL_TYPE_BLOB));
}
