/*************************************************************************************************/
/*!
\file sanitygtest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_SANITY_GT_TEST
#define BLAZE_SERVER_TEST_SANITY_GT_TEST

#include "gtest/gtest.h"
#include "test/common/gtest/gtestfixtureutil.h"

namespace BlazeServerTest
{
    // The fixture for testing class Foo. This sample fixture sanity tests the basic gt test fwk helpers.
    class SanityGtest : public testing::Test, public TestingUtils::GtestFixtureUtil
    {
    protected:
        // You can remove any or all of the following functions if its body
        // is empty.

        SanityGtest() {
            // You can do set-up work for each test here.
        }

        ~SanityGtest() override {
            // You can do clean-up work that doesn't throw exceptions here.
        }

        // If the constructor and destructor are not enough for setting up
        // and cleaning up each test, you can define the following methods:

        virtual void SetUp() override {} 

        virtual void TearDown() override {
            // Code here will be called immediately after each test (right
            // before the destructor).
        }

        // Objects declared here can be used by all tests in the test case for this fixture.
        void dummyFailFn();
    };

} //ns

#endif