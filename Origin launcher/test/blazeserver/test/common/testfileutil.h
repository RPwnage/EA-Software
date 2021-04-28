/*************************************************************************************************/
/*!
    \file testfileutil.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef TESTINGUTILS_TEST_FILE_UTIL_H
#define TESTINGUTILS_TEST_FILE_UTIL_H

#include <EASTL/string.h>
#include <EASTL/list.h>
#include "test/common/testinghooks.h"
#include "test/common/pb2xml/xml_format.h" // for PrintToXmlString() in writeToXmlFile()
#include "test/gen/common/fileoutputoptions.pb.h"
#include "google/protobuf/util/json_util.h" // for JsonPrintOptions in PrintOpts

namespace google { namespace protobuf { class Message; class Descriptor; } }
namespace EA { namespace IO { namespace Path { class PathString8; } } }

namespace TestingUtils
{
    class TestIdentification;
    typedef eastl::pair<eastl::string, int> FileLineLocation;
    typedef eastl::list<FileLineLocation> FileLineLocationList;
    typedef eastl::list<eastl::string> SuppressionRegexList;

    class TestFileUtil
    {
    public:
        static const char8_t* XML_WITH_XSL_FILE_EXT_DEFAULT;
        static const char8_t* DEFAULT_TEST_ID;
        typedef eastl::list<eastl::string> FileNameList;
        typedef eastl::pair<eastl::string, TestingUtils::OutputOptions> FileNameOutputType;
        typedef eastl::list<FileNameOutputType> FileNameOutputTypeList;
        struct ReadLinesOptions
        {
            ReadLinesOptions() : mRegex(nullptr) {}
            const char8_t* mRegex;
            SuppressionRegexList mSuppressionList;
            bool isMatch(const char8_t* str) const;
        };

        //reading
        static bool readLinesFromFile(FileLineLocationList& lines, const eastl::string& fileFullPath, const ReadLinesOptions& options);
        static bool readJsonFile(google::protobuf::Message& result, const eastl::string& jsonFilePath);
        static bool readJsonFile(eastl::string& jsonData, const eastl::string& filePath);
        static bool readFile(eastl::string& data, const eastl::string& fileFullPath);

        //writing
        struct PrintOpts : public google::protobuf::util::JsonPrintOptions,
            public google::protobuf::XmlFormat::XmlPrintOptions
        {
            PrintOpts() : mType(TestingUtils::JSON), mLineBreakClass(nullptr)
            {
                always_print_primitive_fields = true;
                preserve_proto_field_names = true;
            }
            OutputFormatType mType;
            const google::protobuf::Descriptor* mLineBreakClass;
        };

        static bool writeReportMsgFiles(FileNameOutputTypeList& savedFiles, const google::protobuf::Message& msg, const TestingUtils::OutputOptions& options, const eastl::string defaultDir, const google::protobuf::Descriptor* resultListItemDescriptor);
        static bool writeMsgFile(const google::protobuf::Message& msg, const eastl::string& fileName, const eastl::string& fileDir, const PrintOpts* printOptions);
        static bool writeMsgFile(const google::protobuf::Message& msg, const eastl::string& filePath, const PrintOpts* printOptions);
        static bool writeFile(const eastl::string& data, const eastl::string& filePath);



        static bool findDir(eastl::string& foundAbsolutePath, const eastl::string& searchPathExpression, bool failIfMissing = true);
        static bool findDir(eastl::string& foundAbsolutePath, const eastl::string& searchPathExpression, bool failIfMissing, const eastl::string startingSearchDir);
        static bool findFile(eastl::string& foundAbsolutePath, const char8_t* searchPathExpression, const eastl::string startingSearchDir);
        static void cutString(eastl::string& first, eastl::string& second, const eastl::string from, const char8_t ch);
        static void replaceInString(eastl::string& inString, const eastl::string& oldSubstr, const eastl::string& newSubstr);
        static bool endsWith(const eastl::string& str, const eastl::string& suffix) { return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix); }

        static void getDirFilelist(FileNameList& possDataFiles, const eastl::string& dirName, const char8_t* extFilter, const char8_t* prefixFilter = nullptr, bool stripExt = true);
        static bool ensureDirExists(const eastl::string& absolutePath);
        static void getCwd(eastl::string& cwd);
        static void getCwd(EA::IO::Path::PathString8& cwdpath);
        static void concatPath(eastl::string& result, const char8_t* prefixPath, const char8_t* suffixPath);


        static void openFilesInOs(const FileNameOutputTypeList& files, TestingUtils::OutputFormatType onlyType = TestingUtils::XML);
        static void openFileInOS(const eastl::string fullFilePath, const TestingUtils::OutputOptionsXml& options);

        // misc
        static void defaultTestIdIfUnset(TestingUtils::TestIdentification& testIdentification);
    };

}

#endif