/*************************************************************************************************/
/*!
\file frameworkutiltest.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "test/framework/frameworkutiltest.h"
#include "framework/util/shared/blazestring.h"

namespace BlazeServerTest
{
namespace Framework
{
    //test blaze_stltokenize
    TEST_F(FrameworkUtilTest, StringTokenizeTest)
    {
        const eastl::vector<eastl::vector<eastl::vector<eastl::string>>> testData =
        {
            //{{inputString, delimiters, allowEmptyToken}, {expectedTokens}}
            {{"", "", "true"}, {""}},
            {{"", "", "false"}, {}},
            {{"", " ", "true"}, {""}},
            {{"", " ", "false"}, {}},
            {{"", " +-*/", "true"}, {""}},
            {{"", " +-*/", "false"}, {}},
            {{"abc", " ", "true"}, {"abc"}},
            {{"abc", " ", "false"}, {"abc"}},
            {{"abc def", " ", "true"}, {"abc", "def"}},
            {{"abc def", " ", "false"}, {"abc", "def"}},
            {{"abc def-mm nn", " -", "true"}, {"abc", "def", "mm", "nn"}},
            {{"abc def-mm nn", " -", "false"}, {"abc", "def", "mm", "nn"}},
            {{"abc--def", "-", "true"}, {"abc", "", "def"}},
            {{"abc--def", "-", "false"}, {"abc", "def"}},
            {{"-abc-def---mm-", "-", "true"}, {"", "abc", "def", "", "", "mm", ""}},
            {{"-abc-def---mm-", "-", "false"}, {"abc", "def", "mm"}},
            {{"---", "-", "true"}, {"", "", "", ""}},
            {{"---", "-", "false"}, {}},
            {{" ++abc", " +-", "true"}, {"", "", "", "abc"}},
            {{" ++abc", " +-", "false"}, {"abc"}},
            {{"abc+def-*/mm nn+opq  end", " +-*/", "true"}, {"abc", "def", "", "", "mm", "nn", "opq", "", "end"}},
            {{"abc+def-*/mm nn+opq  end", " +-*/", "false"}, {"abc", "def", "mm", "nn", "opq", "end"}},
            {{"gosblaze-neoxu-pc", "-", "true"}, {"gosblaze", "neoxu", "pc"}},
            {{"gosblaze-neoxu-pc", "-", "false"}, {"gosblaze", "neoxu", "pc"}},
            {{"gosblaze-neoxu-pc-test", "-", "true"}, {"gosblaze", "neoxu", "pc","test"}},
            {{"gosblaze-neoxu-pc-test", "-", "false"}, {"gosblaze", "neoxu", "pc","test"}},
            {{"-neoxu-pc", "-", "true"}, {"", "neoxu", "pc"}},
            {{"-neoxu-pc", "-", "false"}, {"neoxu", "pc"}},
            {{"gosblaze-neoxu--pc-", "-", "true"}, {"gosblaze", "neoxu", "", "pc", ""}},
            {{"gosblaze-neoxu--pc-", "-", "false"}, {"gosblaze", "neoxu", "pc"}},
            {{" ea  play ", " ", "true"}, {"", "ea", "", "play", ""}},
            {{" ea  play ", " ", "false"}, {"ea", "play"}},
            {{"test aeiou", "ea", "true"}, {"t", "st ", "", "iou"}},
            {{"test aeiou", "ea", "false"}, {"t", "st ", "iou"}},
            {{"done", "ea", "true"}, {"don", ""}},
            {{"done", "ea", "false"}, {"don"}}
        };

        for (const auto& data : testData)
        {
            const eastl::string& inputString = data[0][0];
            const eastl::string& delimiters = data[0][1];
            const bool allowEmptyToken = data[0][2] == "true";
            const eastl::vector<eastl::string>& expectedTokens = data[1];
            eastl::vector<eastl::string> actualTokens;

            const size_t result = blaze_stltokenize(inputString, delimiters.c_str(), actualTokens, allowEmptyToken);
            EXPECT_TRUE(result == expectedTokens.size()) << inputString.c_str();
            EXPECT_TRUE(actualTokens == expectedTokens) << inputString.c_str();
        }
    }

    //test blaze_str2dbl
    TEST_F(FrameworkUtilTest, StringToDoubleTest)
    {
        struct TestEntry
        {
            const char8_t* string;
            const double value;
            const uint64_t offset;
            TestEntry(const char8_t* str, const double val, const uint64_t off) : string(str), value(val), offset(off) {}
        };

        eastl::vector<TestEntry> testData;
        testData.emplace_back("", 0.0, 0);
        testData.emplace_back("0", 0.0, 1);
        testData.emplace_back("0.0", 0.0, 3);
        testData.emplace_back(".0", 0.0, 2);
        testData.emplace_back(".01000", 0.01, 6);
        testData.emplace_back("0.0000000001", 0.0000000001, 12);
        testData.emplace_back("123", 123.0, 3);
        testData.emplace_back("123.456", 123.456, 7);
        testData.emplace_back("123.456000", 123.456, 10);
        testData.emplace_back("1234567890123", 1234567890123.0, 13);
        testData.emplace_back("100000000000000", 100000000000000.0, 15);
        testData.emplace_back("123.12345678901", 123.12345678901, 15);
        testData.emplace_back("   000123.456", 123.456, 13);
        testData.emplace_back("123.456xyz", 123.456, 7);
        testData.emplace_back("123test", 123.0, 3);
        testData.emplace_back("test123", 0.0, 0);
        testData.emplace_back("+123.456789", 123.456789, 11);
        testData.emplace_back("   +00", 0.0, 6);
        testData.emplace_back("-0", 0.0, 2);
        testData.emplace_back("-0.0", 0.0, 4);
        testData.emplace_back("-.500", -0.5, 5);
        testData.emplace_back("   -0.00123test", -0.00123, 11);
        testData.emplace_back("-1234567890123.45", -1234567890123.45, 17);
        testData.emplace_back("     -1234567890123.45 xyz", -1234567890123.45, 22);
        testData.emplace_back("0x123", 291.0, 5);           //supports hex values
        testData.emplace_back("0x123abcxyz", 1194684.0, 8);
        testData.emplace_back("-0x123", -291.0, 6);
        testData.emplace_back("  +0x123hijk", 291.0, 8);
        testData.emplace_back("0123", 123.0, 4);            //does not support oct values
        testData.emplace_back("0b10101", 0.0, 1);           //does not support binary values

        for (const auto& data : testData)
        {
            const char8_t* inputString = data.string;
            const char8_t* expectedPointer = data.string + data.offset;
            const double expectedValue = data.value;
            double actualValue;
            char8_t* actualPointer = blaze_str2dbl(inputString, actualValue);
            EXPECT_TRUE(actualPointer == expectedPointer) << inputString;
            EXPECT_TRUE(actualValue == expectedValue) << inputString;
        }
    }
   
} //namespace Framework
} //namespace BlazeServerTest
