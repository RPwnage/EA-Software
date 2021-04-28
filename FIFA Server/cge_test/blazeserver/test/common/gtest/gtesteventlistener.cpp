/*************************************************************************************************/
/*! \file gtesteventlistener.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "test/common/gtest/gtesteventlistener.h"
#include "test/common/gtest/gtestfixtureutil.h"

namespace TestingUtils
{
    ///////////////////////////////////////////////////////////////////////////
    //////////////////////// GtestEventListener ///////////////////////////////

    // \param[in] fixtureName/testName to dispatch for. Empty/null dispatches for all.
    GtestEventListener::GtestEventListener(const char8_t* fixtureName /*= ""*/, const char8_t* testName /*= ""*/)
    {
        // To allow subclasses of GtestEventListener to auto hook into the global google test events,
        // at subsclass construction, the base GtestEventListener ctor calls registerListener().

        makeTestFilterStr(mTestFilter, fixtureName, testName);
        GtestEventListenerAdapter::registerListener(*this);
    }

    GtestEventListener::~GtestEventListener()
    {
        GtestEventListenerAdapter::removeListener(*this);
    }

    void GtestEventListener::makeTestFilterStr(eastl::string& filter, const char8_t* fixtureName, const char8_t* testName)
    {
        filter.sprintf("%s%s%s",
            (fixtureName ? fixtureName : ""),
            (((fixtureName && fixtureName[0] != '\0') && (testName && testName[0] != '\0')) ? "." : ""),
            (testName ? testName : ""));
    }

    ///////////////////////////////////////////////////////////////////////////
    //////////////////////// GtestEventListenerAdapter ////////////////////////

    GtestEventListenerAdapter::TestCaseNameGtestEventListenerMap* 
        GtestEventListenerAdapter::mGtestEventListeners = nullptr;

    // For proper gtest events hookup, this must be called before testing::InitGoogleTest().
    // Test code can attach custom handlers later (like when fixtures exist) via registerListener()
    void GtestEventListenerAdapter::initMainGoogleTestAdapter()
    {
        ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();
        
        // Adds a listener to the end.  googletest takes the ownership:
        listeners.Append(new TestingUtils::GtestEventListenerAdapter);
    }

    void GtestEventListenerAdapter::registerListener(GtestEventListener& listener)
    {
        if (mGtestEventListeners == nullptr)
        {
            //GtestEventListeners *may* need to be constructed at static init time, before tests/fixtures that need them.
            //As C++ can't ensure any static init ordering, this global owner of them gets lazy constructed by 1st one:
            mGtestEventListeners = new TestCaseNameGtestEventListenerMap();
        }

        (*mGtestEventListeners)[listener.getTestFilter()].insert(&listener);
    }

    void GtestEventListenerAdapter::removeListener(GtestEventListener& listener)
    {
        if (mGtestEventListeners == nullptr)
        {
            return;
        }
        auto found = mGtestEventListeners->find(listener.getTestFilter());
        if (found == mGtestEventListeners->end())
        {
            return;//TODO: looks like not removing correctly?
        }
        found->second.erase(&listener);

        // can cleanup map entry on empty
        if (found->second.empty())
        {
            mGtestEventListeners->erase(found->first);
        }
    }

    void GtestEventListenerAdapter::OnEnvironmentsTearDownStart(const testing::UnitTest& unit_test)
    {
        if (mGtestEventListeners != nullptr)
        {
            delete mGtestEventListeners;
            mGtestEventListeners = nullptr;
        }
    }

    // note: google test calls this BEFORE fixture constructed (even though this is per test)
    // (Side: separate from the local per-file testing::Test::Setup/TearDown)
    void GtestEventListenerAdapter::OnTestStart(const ::testing::TestInfo& test_info)
    {
        const char8_t* fixtureName = test_info.test_case_name();
        const char8_t* testName = test_info.name();
        eastl::string testFilter;
        GtestEventListener::makeTestFilterStr(testFilter, fixtureName, testName);
        testlog("*** Test %s starting.\n", testFilter.c_str());

        // Note: order here is relevant, may prevent tests crashing, etc.

        dispatch(true, "", test_info);                  //NON case-specific first
        dispatch(true, fixtureName, test_info);         //All tests in fixture
        dispatch(true, testFilter.c_str(), test_info);  //Specific test in fixture
    }

    // gtest calls this AFTER gtest fixture/test destructed
    void GtestEventListenerAdapter::OnTestEnd(const ::testing::TestInfo& test_info)
    {
        const char8_t* fixtureName = test_info.test_case_name();
        const char8_t* testName = test_info.name();
        eastl::string testFilter;
        GtestEventListener::makeTestFilterStr(testFilter, fixtureName, testName);
        testlog("*** Test %s ending.\n", testFilter.c_str());

        // reverse order from OnTestStart

        dispatch(false, testFilter.c_str(), test_info);
        dispatch(false, fixtureName, test_info);
        dispatch(false, "", test_info);
    }

    void GtestEventListenerAdapter::dispatch(bool onStart, const char8_t* testFilter,
        const ::testing::TestInfo& test_info)
    {
        if (mGtestEventListeners == nullptr || testFilter == nullptr)
        {
            testlog("note: custom gtest event listeners N/A for filter(%s).", (testFilter ? testFilter : "<nullptr>"));
            return;
        }
        auto foundFixt = mGtestEventListeners->find(testFilter);
        if (foundFixt != mGtestEventListeners->end())
        {
            for (auto listenerItr : foundFixt->second)
            {
                if (onStart)
                    listenerItr->onPreTest(test_info);
                else
                    listenerItr->onPostTest(test_info);
            }
        }
    }

}//ns
