/*************************************************************************************************/
/*!
    \file gtestfixtureutil.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef TESTINGUTILS_GTEST_FIXTURE_UTIL_H
#define TESTINGUTILS_GTEST_FIXTURE_UTIL_H

#include <EASTL/string.h>
#include "test/common/testinghooks.h"//for TestingHooks::LogLevel in gtestLogFunction
#include "gtest/gtest.h"

namespace TestingUtils
{
    class GTMenuParams;

    // gtest fixture base
    class GtestFixtureUtil
    {
    public:
        // Test inputs root
        static const char8_t* TESTARTIFACTS_ROOTDIR_NAME;

        GtestFixtureUtil();
        virtual ~GtestFixtureUtil();

        // NOTE: methods must be public for gtest test bodies to access
        const eastl::string& getCurrFixtureDir() const;
        const eastl::string& getCurrFixtureName() const;    
        const eastl::string getCurrTestArtifactsDir() const;
        const eastl::string getCurrTestResultsDir() const;
        static eastl::string getFixtureDir(const char8_t* fixtureName = getFixtureName(false).c_str());
        static eastl::string getFixtureName(bool includePrefixTag = false);
        static eastl::string getTestName();
        static eastl::string getTestArtifactsDir(const char8_t* fixtureName = getFixtureName(false).c_str(), const char8_t* testName = getTestName().c_str());
        static eastl::string getTestResultsDir(const char8_t* fixtureName = getFixtureName(false).c_str(), const char8_t* testName = getTestName().c_str(), bool fixtureDirOnly = false);

        // call this before ::testing::InitGoogleTest
        static void preInitGoogleTest(int argc, char **argv);
        static void gtestLogFunction(const char8_t* file, int line, const char8_t* msg, TestingHooks::LogLevel level);
        static void gtestPrintAllTestsFunction(int argc, char **argv, const TestingUtils::GTMenuParams& params);

    private:
        mutable eastl::string mFixtureName;
        mutable eastl::string mFixtureDir;
    };

}  // namespace

#endif