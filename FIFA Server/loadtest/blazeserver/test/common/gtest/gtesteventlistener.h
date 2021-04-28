/*************************************************************************************************/
/*! \file gtesteventlistener.h
    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef TESTINGUTILS_GTEST_EVENT_LISTENER_H
#define TESTINGUTILS_GTEST_EVENT_LISTENER_H

#include "gtest/gtest.h" // for testing::EmptyTestEventListener parent class
#include <EASTL/hash_map.h>// for TestCaseNameGtestEventListenerMap
#include <EASTL/string.h>
#include <EASTL/set.h> // for GtestEventListenerSet

namespace TestingUtils
{

    // Subclass GtestEventListener to hook into the global google test events
    class GtestEventListener
    {
    public:
        GtestEventListener(const char8_t* fixtureName = "", const char8_t* testName = "");
        virtual ~GtestEventListener();
        virtual void onPreTest(const ::testing::TestInfo& test_info) = 0;
        virtual void onPostTest(const ::testing::TestInfo& test_info) = 0;
        const eastl::string& getTestFilter() const { return mTestFilter; }
        static void makeTestFilterStr(eastl::string& filter, const char8_t* fixtureName, const char8_t* testName);
    private:
        eastl::string mTestFilter;
    };


    class GtestEventListenerAdapter : public ::testing::EmptyTestEventListener
    {
    public:
        GtestEventListenerAdapter() {}
        ~GtestEventListenerAdapter() override {}
        static void initMainGoogleTestAdapter();
        static void registerListener(GtestEventListener& listener);
        static void removeListener(GtestEventListener& listener);

    private:

        //::testing::TestEventListener interface
        void OnEnvironmentsTearDownStart(const testing::UnitTest& unit_test) override;
        void OnTestStart(const ::testing::TestInfo& test_info) override;
        void OnTestEnd(const ::testing::TestInfo& test_info) override;
        void dispatch(bool onStart, const char8_t* fixtureFilter, const ::testing::TestInfo& test_info);

        typedef eastl::set<GtestEventListener*> GtestEventListenerSet;
        typedef eastl::hash_map<eastl::string, GtestEventListenerSet> TestCaseNameGtestEventListenerMap;
        static TestCaseNameGtestEventListenerMap* mGtestEventListeners;
    };
}

#endif
//TESTINGUTILS_GTEST_EVENT_LISTENER_H
