/*************************************************************************************************/
/*!
    \file

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "test/common/gtest/gtestfixtureutil.h"
#include "test/common/gtest/gtesteventlistener.h" // for preInitGoogleTest
#include "test/common/gtmenu/gtmenuintegration.h" // For gGTMenu in getTestResultsDir
#include "test/common/gtmenu/gtmenuparams.h" // For GTMenuParams arg
#include "test/common/testfileutil.h"
#include <EAIO/PathString.h> // for PathString8 in getCurrTestResultsDir()
#include "gtest/gtest.h"


namespace TestingUtils 
{
    const char8_t* GtestFixtureUtil::TESTARTIFACTS_ROOTDIR_NAME = "etc.test";

    GtestFixtureUtil::GtestFixtureUtil()
    {
    }

    GtestFixtureUtil::~GtestFixtureUtil()
    {
    }

    const eastl::string& GtestFixtureUtil::getCurrFixtureDir() const
    {
        // For efficiency cache
        if (mFixtureDir.empty())
        {
            auto fixtureName = getCurrFixtureName();
            if (has_testerr())
            {
                testerr("Cant get fixture directory: Failed getting fixture name.");
                return mFixtureDir;
            }
            mFixtureDir = getFixtureDir(fixtureName.c_str());
        }
        return mFixtureDir;
    }

    // get current fixture's test artifacts directory. If not found, create it under working dir.//TODO rename/merge with getTestArtifacts dir. Delete the above overload
    eastl::string GtestFixtureUtil::getFixtureDir(const char8_t* fixtureName)
    {
        eastl::string fixtureDir;
        if (fixtureName == nullptr || fixtureName[0] == '\0')
        {
            testerr("Cant get fixture directory: empty fixture name.");
            return fixtureDir;
        }
        const eastl::string searchPath(eastl::string::CtorSprintf(), "%s/%s", TESTARTIFACTS_ROOTDIR_NAME, fixtureName);
        if (!TestFileUtil::findDir(fixtureDir, searchPath.c_str(), false))
        {
            // need to create it.
            TestFileUtil::getCwd(fixtureDir);
            TestFileUtil::concatPath(fixtureDir, fixtureDir.c_str(), searchPath.c_str());
            if (!TestingUtils::TestFileUtil::ensureDirExists(fixtureDir))
            {
                testlog("Warning: Failed to create test fixture dir.");
                return "";
            }
        }
        return fixtureDir;
    }

    const eastl::string& GtestFixtureUtil::getCurrFixtureName() const
    {
        if (mFixtureName.empty())
        {
            mFixtureName = GtestFixtureUtil::getFixtureName(false);
        }
        return mFixtureName;
    }
    // Note: to avoid inf loop, doesn't use macros to log, since the macros may depend on this method
    // \param[in] includePrefixTag Whether to include gtest names' prefixes which may appear before a '/'
    eastl::string GtestFixtureUtil::getFixtureName(bool includePrefixTag /*= false*/)
    {
        eastl::string fixtureName;
        auto* currTestInfo = (::testing::UnitTest::GetInstance() == nullptr ? 
            nullptr : testing::UnitTest::GetInstance()->current_test_info());
        if (currTestInfo == nullptr)
        {
            testerr("Cant get fixture name due to internal error: current test info was null. ");
        }
        else
        {
            fixtureName = currTestInfo->test_case_name();
            if (!fixtureName.empty() && !includePrefixTag)
            {
                auto index = fixtureName.find_last_of('/');
                if (index != fixtureName.npos)
                    fixtureName = fixtureName.substr(index + 1);
            }
        }
        return fixtureName;
    }

    // return a copy of current running test's name (not a ref, since its a local generated value)
    eastl::string GtestFixtureUtil::getTestName()
    {
        eastl::string errMsg;
        auto* currTestInfo = (testing::UnitTest::GetInstance() == nullptr ?
            nullptr : testing::UnitTest::GetInstance()->current_test_info());
        if (currTestInfo == nullptr)
        {
            testerr("Cant get test name due to internal error: current test info was null. ");
            return "";
        }
        eastl::string testName = currTestInfo->name();
        auto index = testName.find_last_of('/');
        if (index++ != testName.npos)//++ to skip the '/'
        {
            if (testName.length() <= index)
            {
                testerr("Internal error: invalid current test name(%s), can't parse a tail name after last '/'. ",
                    currTestInfo->name());
                return "";
            }
            testName = testName.substr(index);
        }
        //(not much point caching 'current' test name off)
        return testName;
    }

    const eastl::string GtestFixtureUtil::getCurrTestArtifactsDir() const
    {
        auto fixtureDir = getCurrFixtureDir();
        if (has_testerr())
        {
            return "";
        }
        eastl::string testDirBuf;
        TestFileUtil::concatPath(testDirBuf, fixtureDir.c_str(), getTestName().c_str());
        return testDirBuf;
    }

    eastl::string GtestFixtureUtil::getTestArtifactsDir(const char8_t* fixtureName, const char8_t* testName)
    {
        eastl::string testDirBuf;
        if (fixtureName == nullptr || fixtureName[0] == '\0' || testName == nullptr)
        {
            return testDirBuf;
        }
        auto fixtureDir = getFixtureDir(fixtureName);
        if (has_testerr())
        {
            return testDirBuf;
        }
        TestFileUtil::concatPath(testDirBuf, fixtureDir.c_str(), getTestName().c_str());
        return testDirBuf;
    }

    const eastl::string GtestFixtureUtil::getCurrTestResultsDir() const
    {
        auto fixtureName = getCurrFixtureName();
        if (has_testerr())
            return "";
        return getTestResultsDir(fixtureName.c_str(), getTestName().c_str(), false);
    }

    eastl::string GtestFixtureUtil::getTestResultsDir(const char8_t* fixtureName,
        const char8_t* testName, bool fixtureDirOnly /*= false*/)
    {
        if (!gGTMenu.mArgs.mReportRoot.empty())
        {
            // Use explicitly specified gtest menu options, by default.
            if (fixtureName == nullptr || fixtureName[0] == '\0')
                return "";
            if (!fixtureDirOnly && testName == nullptr)
                return "";
            const EA::IO::Path::PathString8 fixtureName8(fixtureName);
            const EA::IO::Path::PathString8 testName8(fixtureDirOnly ? "" : testName);
            // Use explicitly specified gtest menu options, by default
            EA::IO::Path::PathString8 path(gGTMenu.mArgs.mReportRoot.c_str());
            EA::IO::Path::Append(path, fixtureName8);
            EA::IO::Path::Append(path, testName8);
            TestFileUtil::ensureDirExists(path.c_str());//test using TestFileUtil to ensure directory created.
            return path.c_str();
        }
        else
        {
            return getTestArtifactsDir(fixtureName, testName);
        }
    }

    // Call before InitGoogleTest, to install our logging hooks and tools integration/capabilities
    void GtestFixtureUtil::preInitGoogleTest(int argc, char **argv)
    {
        gTesting.init(&gtestLogFunction, &::testing::Test::HasFailure);
        gGTMenu.init(argc, argv, &gtestPrintAllTestsFunction);

        GtestEventListenerAdapter::initMainGoogleTestAdapter();
    }

    // Logs lines/errors that can be parsed by gt menu.
    // Do not change printed format, without consulting gt menu tool.
    void GtestFixtureUtil::gtestLogFunction(const char8_t* file, int line, const char8_t* msg, TestingHooks::LogLevel level)
    {
        if (msg == nullptr)
            return;
        switch (level)
        {
        case TestingHooks::LogLevel::TRACE:
        {
            //This is similar to SCOPED_TRACE(msg)
            ::testing::ScopedTrace GTEST_CONCAT_TOKEN_(gtest_trace_, line)(file, __LINE__, (msg));
            printf("\n[GtestFixtureUtil] %s", msg);
            break;
        }
        case TestingHooks::LogLevel::ERR:
        {
            //This is similar to GTEST_NONFATAL_FAILURE_(msg)
            GTEST_MESSAGE_AT_(file, line, msg, ::testing::TestPartResult::kNonFatalFailure);
            printf("\n[GtestFixtureUtil] ERR: %s", msg);
            break;
        }
        default:
            break;
        }
    }

    void GtestFixtureUtil::gtestPrintAllTestsFunction(int argc, char **argv, 
        const TestingUtils::GTMenuParams& params)
    {
        // Do not change printed format below, without consulting gt menu tool.

        auto testLocParams = params.paramnames().printtestlocation();

        // To ensure parameterized tests are registered, init google test.
        // Ok to call this twice, will simply NOOP any 2nd time its called:
        ::testing::InitGoogleTest(&argc, argv);

        const int fixtureCount = testing::UnitTest::GetInstance()->total_test_case_count();
        for (int i = 0; i < fixtureCount; ++i)
        {
            const auto fixture = testing::UnitTest::GetInstance()->GetTestCase(i);
            eastl::string fixtureName(fixture->name());
            if (fixtureName.empty())
            {
                fixtureName.sprintf("UNKNOWN_FIXTURE%07d", i);
            }

            const int testCount = fixture->total_test_count();
            for (int j = 0; j < testCount; ++j)
            {
                auto test = fixture->GetTestInfo(j);
                if (test == nullptr)
                    continue;

                eastl::string testName(test->name());//TODO: can gtest ever have this differ from test->test_case_name()
                if (testName.empty())
                {
                    testName.sprintf("UNKNOWN_TEST%07d", j);
                }
                auto typeParam = (test->type_param() != nullptr ? test->type_param() : "");
                auto valuParam = (test->value_param() != nullptr ? test->value_param() : "");
                EA::IO::Path::PathString8 fileNormalized(test->file());
                EA::IO::Path::Normalize(fileNormalized);

                //TODO: may replace with proper protobufs (w/JSON) later

                printf("%s {", params.paramnames().printtestlocationslinestart().c_str());
              //printf("\"%s\":\"%s\"", testLocParams.projectparam().c_str(), );//not required, derived gtmenu side from file names
                printf("\"%s\":\"%s\"", testLocParams.fixtureparam().c_str(), fixtureName.c_str());
                printf(",\"%s\":\"%s\"", testLocParams.testparam().c_str(), testName.c_str());

                printf(",\"%s\":[", testLocParams.genparams().c_str());
                if (typeParam[0] != '\0')
                    printf("\"%s\"", typeParam);
                if (typeParam[0] != '\0' && valuParam[0] != '\0')
                    printf(",");
                if (valuParam[0] != '\0')
                    printf("\"%s\"", valuParam);
                printf("]");

                printf(",\"%s\":\"%s\"", testLocParams.fileparam().c_str(), fileNormalized.c_str());
                printf(",\"%s\":%d", testLocParams.lineparam().c_str(), test->line());
                printf("}\n");
            }
        }
    }

}//ns
