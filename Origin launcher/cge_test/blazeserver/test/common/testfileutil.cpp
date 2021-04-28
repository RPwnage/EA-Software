/*************************************************************************************************/
/*!
    \file

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "test/common/testfileutil.h"
#include "test/gen/common/testidentification.pb.h"
#include "test/common/regexutil.h"
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/wire_format_lite.h>
#include <EAIO/PathString.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileDirectory.h>
#include <EAIO/EAStreamAdapter.h> //for EA::IO::ReadLine in readLinesFromFile
#include <EAStdC/EAProcess.h>// for ExecuteShellCommand in openFileInOS
#if defined EA_PLATFORM_WINDOWS
#include <Windows.h>
#endif
#include <EAStdC/EADateTime.h>

namespace TestingUtils
{
    const char8_t* TestFileUtil::XML_WITH_XSL_FILE_EXT_DEFAULT = "xhtml";
    const char8_t* TestFileUtil::DEFAULT_TEST_ID = "test";

    bool TestFileUtil::ReadLinesOptions::isMatch(const char8_t* str) const
    {
        for (auto itr : mSuppressionList)
        {
            if (TestingUtils::RegexUtil::isMatch(str, itr.c_str()))
                return false;
        }
        return ((mRegex != nullptr) && TestingUtils::RegexUtil::isMatch(str, mRegex));
    }
    
    bool TestFileUtil::readLinesFromFile(FileLineLocationList& lines, const eastl::string& fileFullPath,
        const ReadLinesOptions& options)
    {
        EA::IO::FileStream fs(fileFullPath.c_str());
        if (!fs.Open())
        {
            testerr("unable to open file(%s).\n", fileFullPath.c_str());
            return false;
        }
        int currLineNum = 1;

        eastl::string currLine;
        while (EA::IO::ReadLine(&fs, currLine, EA::IO::kEndianLocal) < EA::IO::kSizeTypeDone) // While there were more lines and there was no error...
        {
            //trim any null terminators ReadLine may add.
            currLine.erase(std::remove(currLine.begin(), currLine.end(), '\0'), currLine.end());

            if (options.isMatch(currLine.c_str()))
            {
                lines.emplace_back(eastl::make_pair(currLine, currLineNum));
            }
            currLine.clear();
            ++currLineNum;
        }
        return true;
    }

    bool TestFileUtil::readJsonFile(google::protobuf::Message& result, const eastl::string& filePath)
    {
        eastl::string jsonData;
        if (!readJsonFile(jsonData, filePath))
            return false;

        google::protobuf::util::JsonParseOptions jsonOpts;
        jsonOpts.ignore_unknown_fields = true;
        auto jsonResult = google::protobuf::util::JsonStringToMessage(std::string(jsonData.c_str()), &result, jsonOpts);
        if (jsonResult.error_code() != google::protobuf::util::error::OK)
        {
            testerr("failed parsing type(%s) from file(%s), err: %s.",
                result.GetTypeName().c_str(), filePath.c_str(), jsonResult.ToString().c_str());
            return false;
        }
        return true;
    }

    bool TestFileUtil::readJsonFile(eastl::string& jsonData, const eastl::string& filePath)
    {
        EA::IO::FileStream fs(filePath.c_str());
        if (!fs.Open())
        {
            testerr("unable to open file(%s).\n", filePath.c_str());
            return false;
        }

        size_t dataSize = EA::IO::File::GetSize(filePath.c_str());
        if (dataSize == 0)
        {
            testerr("empty file(%s).\n", filePath.c_str());
            return false;
        }
        jsonData.resize(dataSize);

        size_t readSize = fs.Read((void*)jsonData.data(), dataSize);
        fs.Flush();
        fs.Close();
        if (readSize != dataSize)
        {
            testerr("read file(%s), expected result(%" PRIu64 "), actual(%" PRIu64 ").",
                filePath.c_str(), dataSize, readSize);
            return false;
        }
        return true;
    }

    // regular full read
    bool TestFileUtil::readFile(eastl::string& data, const eastl::string& fileFullPath)
    {
        EA::IO::FileStream fs(fileFullPath.c_str());
        if (!fs.Open())
        {
            testerr("unable to open file(%s).\n", fileFullPath.c_str());
            return false;
        }
        size_t dataSize = EA::IO::File::GetSize(fileFullPath.c_str());
        if (dataSize == 0)
        {
            testerr("empty file(%s).\n", fileFullPath.c_str());
            return false;
        }
        data.resize(dataSize);
        size_t readSize = fs.Read((void*)data.data(), dataSize);
        fs.Flush();
        fs.Close();
        if (readSize != dataSize)
        {
            testerr("read file(%s), expected result(%" PRIu64 "), actual(%" PRIu64 ").", 
                fileFullPath.c_str(), dataSize, readSize);
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief writes serialized proto msg to file. Ensures dir exists
        \param[out] savedFiles stores list of the generated file names created
        \return Returns whether succeeded
    ***************************************************************************************************/
    bool TestFileUtil::writeReportMsgFiles(FileNameOutputTypeList& savedFiles, const google::protobuf::Message& msg,
        const TestingUtils::OutputOptions& options,
        const eastl::string defaultDir, const google::protobuf::Descriptor* resultListItemDescriptor)
    {
        const auto& fileNoExt = options.filenoext();
        if (fileNoExt.empty())
        {
            return true;
        }
        const auto& dir = (options.dir().empty() ? defaultDir.c_str() : options.dir().c_str());

        //pretty proto printout:
        PrintOpts printOptions;
        if (resultListItemDescriptor != nullptr)
        {
            printOptions.mLineBreakClass = resultListItemDescriptor;
        }
        else
        {
            printOptions.add_whitespace = true;
        }

        //json
        if ((options.type() == OutputFormatType::JSON) || (options.type() == OutputFormatType::UNSPECIFIED))
        {
            printOptions.mType = OutputFormatType::JSON;
            eastl::string fileName(eastl::string::CtorSprintf(), "%s.json", fileNoExt.c_str());
            if (!writeMsgFile(msg, fileName.c_str(), dir, &printOptions))
            {
                testerr("Failed to write json file(%s), dir(%s).", fileName.c_str(), dir);
                return false;
            }
            savedFiles.push_back().first = eastl::string(eastl::string::CtorSprintf(), "%s/%s", dir, fileName.c_str());
            savedFiles.back().second = options;
        }

        //xml
        if ((options.type() == OutputFormatType::XML) || (options.type() == OutputFormatType::UNSPECIFIED))
        {
            printOptions.mType = OutputFormatType::XML;

            if (!options.html().xslfile().empty())
            {
                eastl::string xslFileAbs;
                bool ok = findFile(xslFileAbs, options.html().xslfile().c_str(), dir);
                if (!ok)
                {
                    testerr("Failed to find xsl file(%s).", options.html().xslfile().c_str());
                    return false;
                }
                replaceInString(xslFileAbs, "\\", "/");
                printOptions.styleSheetAttribs.assign(eastl::string(eastl::string::CtorSprintf(),
                    "type=\"text/xsl\" href=\"%s\"", xslFileAbs.c_str()).c_str());
            }

            const char8_t* fileExt = (options.html().xslfile().empty() ? "xml" : XML_WITH_XSL_FILE_EXT_DEFAULT);//todo implement correctly from cfg
            eastl::string fileName(eastl::string::CtorSprintf(), "%s.%s", fileNoExt.c_str(), fileExt);
            if (!writeMsgFile(msg, fileName.c_str(), dir, &printOptions))
            {
                testerr("Failed to write xml file(%s). ", fileName.c_str());
                return false;
            }
            savedFiles.push_back().first = eastl::string(eastl::string::CtorSprintf(), "%s/%s", dir, fileName.c_str());
            savedFiles.back().second = options;
        }
        return true;
    }

    // Returns whether succeeded
    bool TestFileUtil::writeMsgFile(const google::protobuf::Message& msg, const eastl::string& fileName,
        const eastl::string& fileDir, const PrintOpts* printOptions)
    {
        if (fileDir.empty())
        {
            testerr("Test issue: No target dir name specified. Cannot write log(%s).", fileName.c_str());
            return false;
        }
        EA::IO::Path::PathString8 filePath(fileDir.c_str());
        EA::IO::Path::Join(filePath, fileName.c_str());
        return writeMsgFile(msg, filePath.c_str(), printOptions);
    }
    bool TestFileUtil::writeMsgFile(const google::protobuf::Message& msg, const eastl::string& filePath,
        const PrintOpts* printOptions)
    {
        PrintOpts defaultOpts;
        if (printOptions == nullptr)
        {
            printOptions = &defaultOpts;
        }

        // to json or xml format
        std::string formattedData;
        const char8_t* recordStartChars = nullptr;
        switch (printOptions->mType)
        {
        case OutputFormatType::UNSPECIFIED:
        case OutputFormatType::JSON:
        {
            auto encodeResult = google::protobuf::util::MessageToJsonString(msg, &formattedData, *printOptions);
            if (encodeResult.error_code() != google::protobuf::util::error::OK)
            {
                testerr("Failed serializing type(%s) to json, cannot write file(%s), err: %s",
                    msg.GetTypeName().c_str(), filePath.c_str(), encodeResult.ToString().c_str());
                return false;
            }
            recordStartChars = "{\"";
            break;
        }
        case OutputFormatType::XML:
            google::protobuf::XmlFormat::PrintToXmlString(msg, &formattedData, *printOptions);
            if (formattedData.empty())
            {
                testerr("Failed serializing type(%s) to xml, cannot write file(%s). Check code and logs.",
                    msg.GetTypeName().c_str(), filePath.c_str());
                return false;
            }
            recordStartChars = "<";
            break;
        default:
            testerr("Internal error, unhandled PrintOpts::Format(%s), cannot write file(%s). Update code.",
                OutputFormatType_Name(printOptions->mType).c_str(), filePath.c_str());
            return false;
        };

        // pretty the format, adding line breaks at each record
        eastl::string prettiedData(formattedData.c_str());
        if ((printOptions->mLineBreakClass != nullptr) && (printOptions->mType != OutputFormatType::XML))//XML not yet supported
        {
            if (printOptions->mLineBreakClass->field_count() == 0)
            {
                testerr("Internal error: invalid/no-field descriptor generated for type(%s), cannot write pretty format to file.", printOptions->mLineBreakClass->full_name().c_str());
                return false;
            }
            const eastl::string recordStartOld(eastl::string::CtorSprintf(), "%s%s\":", recordStartChars, printOptions->mLineBreakClass->field(0)->json_name().c_str()); //demarcates records
            const eastl::string recordStartNew(eastl::string::CtorSprintf(), "\n%s", recordStartOld.c_str());//with newline prepended
            TestFileUtil::replaceInString(prettiedData, recordStartOld, recordStartNew);
        }
        
        // write
        return writeFile(prettiedData, filePath);
    }

    /*! ************************************************************************************************/
    /*! \brief file write helper. Ensures dir exists. Returns whether succeeded
    ***************************************************************************************************/
    bool TestFileUtil::writeFile(const eastl::string& data, const eastl::string& filePath)
    {
        const auto& dir = EA::IO::Path::GetDirectoryString(filePath.c_str());
        if (!ensureDirExists(dir.c_str()))
        {
            testerr("Failed dir check, before writing to file(%s)", filePath.c_str());
            return true;
        }
        EA::IO::File::Remove(filePath.c_str());
        EA::IO::FileStream fs(filePath.c_str());
        if (!fs.Open(EA::IO::kAccessFlagWrite, EA::IO::kCDCreateAlways, EA::IO::FileStream::kShareWrite))
        {
            testerr("writeFile: cannot open file(%s)", filePath.c_str());
            return false;
        }
        bool success = fs.Write((void*)data.data(), data.length());
        fs.Close();
        if (!success)
        {
            testerr("writeFile: write to file(%s) failed, for data(%s)", filePath.c_str(), data.c_str());
        }
        return success;
    }


    bool TestFileUtil::findDir(eastl::string& foundAbsolutePath, const eastl::string& searchPathExpression,
        bool failIfMissing)
    {
        // passing in empty to search from file or working dir
        return findDir(foundAbsolutePath, searchPathExpression, failIfMissing, "");
    }

    // returns whether found
    bool TestFileUtil::findDir(eastl::string& foundAbsolutePath, const eastl::string& searchPathExpression,
        bool failIfMissing, const eastl::string startingSearchDir)
    {
        EA::IO::Path::PathString8 inputPath(EA::IO::Path::PathString8(searchPathExpression.c_str()));
        EA::IO::Path::Normalize(inputPath);
        const bool isRelativeDir = EA::IO::Path::IsRelative(inputPath);
        if (isRelativeDir)
        {
            EA::IO::Path::PathString8 cwdpath;
            if (!startingSearchDir.empty())
                cwdpath = startingSearchDir.c_str();
            else
            {
                // else try starting from source code dir
                cwdpath = EA::IO::Path::GetDirectoryString(__FILE__);
                if (findDir(foundAbsolutePath, searchPathExpression, false, cwdpath.c_str()))
                {
                    return true;
                }
                // else try starting from current working dir
                getCwd(cwdpath);
                if (has_testerr())
                {
                    return false;//logged
                }
                if (findDir(foundAbsolutePath, searchPathExpression, failIfMissing, cwdpath.c_str()))
                {
                    return true;
                }
            }

            EA::IO::Path::PathString8 cwdpathScratch(cwdpath);
            do
            {
                EA::IO::Path::PathString8 tryconfigpath(cwdpathScratch);
                EA::IO::Path::Join(tryconfigpath, inputPath);

                if (EA::IO::Directory::Exists(tryconfigpath.c_str()))
                {
                    foundAbsolutePath = tryconfigpath.c_str();
                    break;
                }
                EA::IO::Path::TruncateComponent(cwdpathScratch, -1);
            } while (!cwdpathScratch.empty());

        }
        else if (EA::IO::Directory::Exists(inputPath.c_str()))
        {
            foundAbsolutePath = searchPathExpression;
        }

        if (failIfMissing && foundAbsolutePath.empty())
        {
            testerr("findDirPath: failed to find (%s) path('%s').",
                (isRelativeDir ? "relative" : "absolute"), inputPath.c_str());
        }
        return !foundAbsolutePath.empty();
    }


    // find file. Based on original toolmain
    bool TestFileUtil::findFile(eastl::string& foundAbsolutePath, const char8_t* searchPathExpression,
        const eastl::string startingSearchDir)
    {
        EA::IO::Path::PathString8 configfilepathNormalized(searchPathExpression);
        EA::IO::Path::Normalize(configfilepathNormalized);
        if (!EA::IO::Path::IsRelative(configfilepathNormalized))
        {
            foundAbsolutePath = configfilepathNormalized.c_str();
        }
        else
        {
            EA::IO::Path::PathString8 cwdpath;
            if (!startingSearchDir.empty())
                cwdpath = startingSearchDir.c_str();
            else
            {
                // else try starting from source code dir
                cwdpath = EA::IO::Path::GetDirectoryString(__FILE__);
                if (findFile(foundAbsolutePath, searchPathExpression, cwdpath.c_str()))
                {
                    return true;
                }
                // else try starting from current working dir
                getCwd(cwdpath);
                if (has_testerr())
                {
                    return false;//logged
                }
                return findFile(foundAbsolutePath, searchPathExpression, cwdpath.c_str());
            }

            EA::IO::Path::PathString8 cwdpathScratch(cwdpath);
            do
            {
                EA::IO::Path::PathString8 tryconfigpath(cwdpathScratch);
                EA::IO::Path::Join(tryconfigpath, configfilepathNormalized);

                if (EA::IO::File::Exists(tryconfigpath.c_str()))
                {
                    foundAbsolutePath = tryconfigpath.c_str();
                    break;
                }
                EA::IO::Path::TruncateComponent(cwdpathScratch, -1);
            } while (!cwdpathScratch.empty());
            if (foundAbsolutePath.empty())
            {
                testerr("findFile: failed to find file: %s via recursive upward search starting at: %s",
                    configfilepathNormalized.c_str(), cwdpath.c_str());
                return false;
            }
        }

        if (!EA::IO::File::Exists(foundAbsolutePath.c_str()))
        {
            testerr("findFile: config file(%s) not found.", searchPathExpression);
            return false;
        }
        return true;
    }

    // cut string util method. Based on original toolmain
    void TestFileUtil::cutString(eastl::string& first, eastl::string& second, const eastl::string from, const char8_t ch)
    {
        // extract factortype
        eastl_size_t offset = from.find_first_of(ch);
        if (offset == eastl::string::npos)
        {
            first = from;
            second.clear();
        }
        else
        {
            first = from.substr(0, offset);
            second = from.substr(offset + 1);
        }
    }

    void TestFileUtil::replaceInString(eastl::string& inString, const eastl::string& oldSubstr,
        const eastl::string& replacementSubstr)
    {
        size_t n = 0;
        while ((n = inString.find(oldSubstr, n)) != eastl::string::npos)
        {
            inString.replace(n, oldSubstr.size(), replacementSubstr);
            n += replacementSubstr.size();
        }
    }

    void TestFileUtil::getDirFilelist(FileNameList& possDataFiles, const eastl::string& dirName,
        const char8_t* extFilter,
        const char8_t* prefixFilter, bool stripExt)
    {
        EA::IO::DirectoryIterator di;
        EA::IO::DirectoryIterator::EntryList entryList;

        eastl::wstring wstr;
        wstr.assign_convert(dirName);

        const bool fileWasFound = (di.Read(wstr.c_str(), entryList, L"*", EA::IO::kDirectoryEntryAll, 1000, false) > 0);
        if (fileWasFound)
        {
            for (EA::IO::DirectoryIterator::EntryList::const_iterator itr = entryList.begin(),
                end = entryList.end(); itr != end; ++itr)
            {
                // validate extension
                eastl::string fileName(eastl::string::CtorSprintf(), "%ls", itr->msName.c_str());
                eastl::string::size_type pos = fileName.find_last_of(".");
                if (extFilter != nullptr)
                {
                    eastl::string ext = ((pos == eastl::string::npos) ? "" : fileName.substr(pos + 1));
                    if (extFilter[0] == '.')
                        ++extFilter;
                    if (ext.comparei(extFilter) != 0)
                    {
                        continue;
                    }
                }
                //gtest requires parameterized tests' param names to strip non alphanumerics.
                //Caller must keep track of the extension and re-append as needed
                if (stripExt && pos != eastl::string::npos)
                {
                    fileName = fileName.substr(0, pos);
                }
                if (fileName.empty())
                    continue;

                //optional prefix filter
                if ((prefixFilter != nullptr) && (fileName.substr(0, strlen(prefixFilter)).compare(prefixFilter) != 0))
                {
                    continue;
                }
                possDataFiles.push_back(fileName);
            }
        }
    }

    bool TestFileUtil::ensureDirExists(const eastl::string& absolutePath)
    {
        if (!EA::IO::Directory::EnsureExists(absolutePath.c_str()))
        {
            testlog("ensureDirExists: warning: failed on path(%s)", absolutePath.c_str());
            return false;
        }
        return true;
    }

    void TestFileUtil::getCwd(eastl::string& cwd)
    {
        EA::IO::Path::PathString8 cwdpath;
        getCwd(cwdpath);
        cwd = cwdpath.c_str();
    }

    void TestFileUtil::getCwd(EA::IO::Path::PathString8& cwdpath)
    {
        auto count = EA::IO::Directory::GetCurrentWorkingDirectory(cwdpath.begin(), cwdpath.kMaxSize);
        if (count <= 0)
        {
            testerr("getCwd: failed to get current working directory.");
            return;
        }
        cwdpath.force_size(count);
    }

    // append suffix path to prefix path
    void TestFileUtil::concatPath(eastl::string& result, const char8_t* prefixPath, const char8_t* suffixPath)
    {
        if (prefixPath == nullptr || prefixPath[0] == '\0' || suffixPath == nullptr)
            return;

        //Note: in case appending suffix to same buffer, *copy* input prefixPath to sep tmp 1st
        EA::IO::Path::PathString8 tmp(prefixPath);
        EA::IO::Path::Join(tmp, suffixPath);
        result = tmp.c_str();
    }

    void TestFileUtil::openFilesInOs(const FileNameOutputTypeList& files, TestingUtils::OutputFormatType onlyType)
    {
#if defined EA_PLATFORM_WINDOWS
        for (auto& file : files)
        {
            if ((onlyType != TestingUtils::UNSPECIFIED) && (file.second.type() != onlyType))
                continue;
            switch (file.second.type())
            {
            case XML:
                openFileInOS(file.first, file.second.html());
                Sleep(300);//workaround for IE usability to ensure opening each in new tab/window (else sometimes reuses an old tab)
                break;
            case JSON:
            case UNSPECIFIED:
            default:
                // To avoid desktop clutter, not opening anything else but HTML/XML UIs currently
                testlog("openFilesInOs: auto-opening of file type(%s) not supported currently, skipping(%s)",
                    TestingUtils::OutputFormatType_Name(file.second.type()).c_str(), file.first.c_str());
                continue;
            }
        }
        //Side: Batching would be more efficient, but this doesn't actually work (though MS doc'd as workable):
        //eastl::string cmds;
        //for (auto& file : files)
        //{
        //    const auto& fullFilePath = file.first;
        //    const auto& options = file.second.xml();
        //    if (!options.winopencmd().empty())
        //        cmds.append_sprintf("%s %s &\n", options.winopencmd().c_str(), fullFilePath.c_str());//batch with newlines separating
        //}
        //if (!cmds.empty() && (-1 == EA::StdC::ExecuteShellCommand(cmds.c_str())))
        //    testerr("openFilesInOs: Failed to open (%s)", cmds.c_str());
#endif
    }

    void TestFileUtil::openFileInOS(const eastl::string fullFilePath, const TestingUtils::OutputOptionsXml& options)
    {
#if defined EA_PLATFORM_WINDOWS
        if (!options.winopencmd().empty())
        {
            if (-1 == EA::StdC::ExecuteShellCommand(eastl::string(eastl::string::CtorSprintf(),
                "call %s %s &", options.winopencmd().c_str(), fullFilePath.c_str()).c_str()))
            {
                testerr("openFileInOS: Failed to open (%s)", fullFilePath.c_str());
            }
        }
#endif
    }

    void TestFileUtil::defaultTestIdIfUnset(TestingUtils::TestIdentification& testIdentification)
    {
        if (testIdentification.id().empty())
        {
            testIdentification.set_id(DEFAULT_TEST_ID);
        }
        if (testIdentification.rundate().empty())
        {
            EA::StdC::DateTime date;
            eastl::string runDate(eastl::string::CtorSprintf(), "%d/%d/%02d %02d:%02d:%02d.%03d",
                date.GetParameter(EA::StdC::kParameterYear), date.GetParameter(EA::StdC::kParameterMonth),
                date.GetParameter(EA::StdC::kParameterDayOfMonth), date.GetParameter(EA::StdC::kParameterHour),
                date.GetParameter(EA::StdC::kParameterMinute), date.GetParameter(EA::StdC::kParameterSecond),
                date.GetParameter(EA::StdC::kParameterNanosecond) / 1000000);
            testIdentification.set_rundate(runDate.c_str());
        }
    }

}//ns
